#include "core/metrics_internal.h"

#include <QByteArray>
#include <QThread>

#include <cerrno>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <net/if.h>
#include <net/route.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/sysctl.h>

namespace tmog::detail {

namespace {

void readProcessPressure(MetricsSnapshot &snapshot)
{
    int cpuCount = 0;
    size_t cpuSize = sizeof(cpuCount);
    if (sysctlbyname("hw.logicalcpu", &cpuCount, &cpuSize, nullptr, 0) != 0
        || cpuCount <= 0) {
        cpuCount = QThread::idealThreadCount();
    }
    const quint32 cpus = static_cast<quint32>(qMax(1, cpuCount));

    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t length = 0;
    if (sysctl(mib, 4, nullptr, &length, nullptr, 0) != 0 || length == 0) {
        snapshot.processPressure =
            Reading::unavailable(QStringLiteral("Cannot enumerate processes"));
        return;
    }

    QByteArray buffer;
    size_t actual = 0;
    bool enumerated = false;
    for (int attempt = 0; attempt < 3; ++attempt) {
        buffer.resize(static_cast<int>(length), Qt::Uninitialized);
        actual = length;
        if (sysctl(mib, 4, buffer.data(), &actual, nullptr, 0) == 0) {
            enumerated = true;
            break;
        }
        if (errno != ENOMEM) {
            break;
        }
        length += length / 2 + 4096;
    }

    if (!enumerated || actual < sizeof(kinfo_proc)) {
        snapshot.processPressure =
            Reading::unavailable(QStringLiteral("Process enumeration failed"));
        return;
    }

    const auto *processes = reinterpret_cast<const kinfo_proc *>(buffer.constData());
    const quint64 processCount = actual / sizeof(kinfo_proc);
    quint64 runningCount = 0;
    for (quint64 index = 0; index < processCount; ++index) {
        if (processes[index].kp_proc.p_stat == SRUN) {
            ++runningCount;
        }
    }

    snapshot.logicalCpus = cpus;
    snapshot.processCount = processCount;
    snapshot.runnableCount = runningCount;
    snapshot.processPressure = Reading::available(
        static_cast<double>(runningCount) / static_cast<double>(cpus),
        QStringLiteral("%1 running processes / %2 CPUs · %3 processes")
            .arg(runningCount)
            .arg(cpus)
            .arg(processCount));
}

void readStorage(MetricsSnapshot &snapshot)
{
    struct statvfs stats
    {
    };
    if (::statvfs("/", &stats) != 0) {
        snapshot.storagePercent = Reading::unavailable(QStringLiteral("statvfs failed for /"));
        return;
    }

    const quint64 blockSize = stats.f_frsize > 0 ? stats.f_frsize : stats.f_bsize;
    const quint64 total = static_cast<quint64>(stats.f_blocks) * blockSize;
    const quint64 free = static_cast<quint64>(stats.f_bfree) * blockSize;
    const quint64 used = total >= free ? total - free : 0;

    snapshot.storageName = QStringLiteral("Root volume (/)");
    snapshot.storageTotalBytes = total;
    snapshot.storageUsedBytes = used;

    const auto percent = percentage(used, total);
    if (!percent) {
        snapshot.storagePercent = Reading::unavailable(QStringLiteral("Root volume reports no capacity"));
        return;
    }
    snapshot.storagePercent = Reading::available(
        *percent, QStringLiteral("%1 of %2").arg(formatBytes(used), formatBytes(total)));
}

std::optional<NetCounters> readNetworkCounters()
{
    int mib[6] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST2, 0};
    size_t length = 0;
    if (sysctl(mib, 6, nullptr, &length, nullptr, 0) != 0 || length == 0) {
        return std::nullopt;
    }

    QByteArray buffer(static_cast<int>(length), Qt::Uninitialized);
    if (sysctl(mib, 6, buffer.data(), &length, nullptr, 0) != 0) {
        return std::nullopt;
    }

    NetCounters counters;
    int interfaces = 0;
    const char *cursor = buffer.constData();
    const char *end = cursor + length;
    while (cursor + sizeof(if_msghdr) <= end) {
        const auto *header = reinterpret_cast<const if_msghdr *>(cursor);
        if (header->ifm_msglen < sizeof(if_msghdr)) {
            break;
        }
        if (header->ifm_type == RTM_IFINFO2) {
            const auto *info = reinterpret_cast<const if_msghdr2 *>(cursor);
            if (!(info->ifm_flags & IFF_LOOPBACK)) {
                counters.receiveBytes += info->ifm_data.ifi_ibytes;
                counters.transmitBytes += info->ifm_data.ifi_obytes;
                ++interfaces;
            }
        }
        cursor += header->ifm_msglen;
    }

    if (interfaces == 0) {
        return std::nullopt;
    }
    return counters;
}

} // namespace

MetricsSnapshot samplePlatform(PlatformState &state, double elapsedSeconds)
{
    MetricsSnapshot snapshot;

    host_cpu_load_info_data_t load{};
    mach_msg_type_number_t loadCount = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(),
                        HOST_CPU_LOAD_INFO,
                        reinterpret_cast<host_info_t>(&load),
                        &loadCount) == KERN_SUCCESS) {
        const quint64 user = load.cpu_ticks[CPU_STATE_USER];
        const quint64 system = load.cpu_ticks[CPU_STATE_SYSTEM];
        const quint64 idle = load.cpu_ticks[CPU_STATE_IDLE];
        const quint64 nice = load.cpu_ticks[CPU_STATE_NICE];

        if (state.hasCpu) {
            if (user >= state.cpu.user && system >= state.cpu.system && idle >= state.cpu.idle
                && nice >= state.cpu.nice) {
                const quint64 userDelta = user - state.cpu.user;
                const quint64 systemDelta = system - state.cpu.system;
                const quint64 niceDelta = nice - state.cpu.nice;
                const quint64 idleDelta = idle - state.cpu.idle;
                const quint64 totalDelta = userDelta + systemDelta + niceDelta + idleDelta;
                const quint64 busyDelta = userDelta + systemDelta + niceDelta;
                const auto percent = totalDelta > 0 && busyDelta <= totalDelta
                    ? percentage(busyDelta, totalDelta)
                    : std::nullopt;
                snapshot.cpuPercent = percent
                    ? Reading::available(*percent)
                    : Reading::unavailable(QStringLiteral("CPU counters did not advance"));
            } else {
                snapshot.cpuPercent = Reading::unavailable(QStringLiteral("CPU counters reset"));
            }
        } else {
            snapshot.cpuPercent = Reading::pending(QStringLiteral("Collecting first CPU delta"));
        }

        state.cpu.user = user;
        state.cpu.system = system;
        state.cpu.idle = idle;
        state.cpu.nice = nice;
        state.hasCpu = true;
    } else {
        snapshot.cpuPercent = Reading::unavailable(QStringLiteral("host_statistics failed for CPU"));
    }

    quint64 totalMemory = 0;
    size_t memorySize = sizeof(totalMemory);
    if (sysctlbyname("hw.memsize", &totalMemory, &memorySize, nullptr, 0) == 0
        && totalMemory > 0) {
        vm_statistics64_data_t memory{};
        mach_msg_type_number_t memoryCount = HOST_VM_INFO64_COUNT;
        vm_size_t pageSize = 0;
        if (host_statistics64(mach_host_self(),
                              HOST_VM_INFO64,
                              reinterpret_cast<host_info64_t>(&memory),
                              &memoryCount) == KERN_SUCCESS
            && host_page_size(mach_host_self(), &pageSize) == KERN_SUCCESS) {
            const quint64 usedPages = static_cast<quint64>(memory.active_count)
                + static_cast<quint64>(memory.wire_count)
                + static_cast<quint64>(memory.compressor_page_count);
            const quint64 used = qMin(usedPages * pageSize, totalMemory);
            snapshot.memoryTotalBytes = totalMemory;
            snapshot.memoryUsedBytes = used;
            const auto percent = percentage(used, totalMemory);
            snapshot.memoryPercent = percent
                ? Reading::available(
                      *percent,
                      QStringLiteral("%1 of %2").arg(formatBytes(used), formatBytes(totalMemory)))
                : Reading::unavailable(QStringLiteral("Memory reports no capacity"));
        } else {
            snapshot.memoryPercent =
                Reading::unavailable(QStringLiteral("host_statistics64 failed for memory"));
        }
    } else {
        snapshot.memoryPercent = Reading::unavailable(QStringLiteral("hw.memsize is unavailable"));
    }

    readStorage(snapshot);
    readProcessPressure(snapshot);

    const auto network = readNetworkCounters();
    if (network) {
        snapshot.networkDownTotalBytes = network->receiveBytes;
        snapshot.networkUpTotalBytes = network->transmitBytes;

        if (state.hasNetwork) {
            const auto down =
                counterRate(network->receiveBytes, state.networkReceive, elapsedSeconds);
            const auto up =
                counterRate(network->transmitBytes, state.networkTransmit, elapsedSeconds);
            if (down && up) {
                snapshot.networkDownPerSecond = Reading::available(*down);
                snapshot.networkUpPerSecond = Reading::available(*up);
            } else {
                const QString reason = QStringLiteral("Network counters reset or clock did not advance");
                snapshot.networkDownPerSecond = Reading::unavailable(reason);
                snapshot.networkUpPerSecond = Reading::unavailable(reason);
            }
        } else {
            snapshot.networkDownPerSecond =
                Reading::pending(QStringLiteral("Collecting first network delta"));
            snapshot.networkUpPerSecond =
                Reading::pending(QStringLiteral("Collecting first network delta"));
        }

        state.networkReceive = network->receiveBytes;
        state.networkTransmit = network->transmitBytes;
        state.hasNetwork = true;
    } else {
        const QString reason = QStringLiteral("Cannot read interface counters");
        snapshot.networkDownPerSecond = Reading::unavailable(reason);
        snapshot.networkUpPerSecond = Reading::unavailable(reason);
    }

    return snapshot;
}

} // namespace tmog::detail
