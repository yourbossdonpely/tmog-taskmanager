#include "core/metrics_internal.h"

#include <QDir>
#include <QFile>
#include <QStringList>
#include <QThread>

#include <sys/statvfs.h>

namespace tmog::detail {

namespace {

QByteArray readFile(const char *path)
{
    QFile file(QString::fromLatin1(path));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
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

} // namespace

MetricsSnapshot samplePlatform(PlatformState &state, double elapsedSeconds)
{
    MetricsSnapshot snapshot;

    const QByteArray stat = readFile("/proc/stat");
    CpuTimes times;
    if (parseProcStat(stat, times)) {
        if (state.hasCpu) {
            if (times.total() >= state.cpu.total() && times.idleTotal() >= state.cpu.idleTotal()) {
                const quint64 total = times.total() - state.cpu.total();
                const quint64 idle = times.idleTotal() - state.cpu.idleTotal();
                const auto percent = total > 0 && idle <= total
                    ? percentage(total - idle, total)
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
        state.cpu = times;
        state.hasCpu = true;
    } else {
        snapshot.cpuPercent = Reading::unavailable(QStringLiteral("Cannot read /proc/stat"));
    }

    ProcPressure pressure;
    if (parseProcPressure(stat, pressure)) {
        const quint32 cpus = static_cast<quint32>(qMax(1, QThread::idealThreadCount()));
        quint64 processCount = 0;
        const QStringList entries = QDir(QStringLiteral("/proc"))
                                        .entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &entry : entries) {
            bool ok = false;
            entry.toUInt(&ok);
            if (ok) {
                ++processCount;
            }
        }

        snapshot.logicalCpus = cpus;
        snapshot.runnableCount = pressure.running;
        snapshot.processCount = processCount;
        QString detail = QStringLiteral("%1 runnable threads / %2 CPUs")
                             .arg(pressure.running)
                             .arg(cpus);
        if (processCount > 0) {
            detail += QStringLiteral(" · %1 processes").arg(processCount);
        } else {
            detail += QStringLiteral(" · process count unavailable");
        }
        snapshot.processPressure = Reading::available(
            static_cast<double>(pressure.running) / static_cast<double>(cpus), detail);
    } else {
        snapshot.processPressure =
            Reading::unavailable(QStringLiteral("Cannot read /proc/stat process counters"));
    }

    const QByteArray meminfo = readFile("/proc/meminfo");
    MemInfo memory;
    if (parseMeminfo(meminfo, memory)) {
        const quint64 used = memory.totalBytes >= memory.availableBytes
            ? memory.totalBytes - memory.availableBytes
            : 0;
        snapshot.memoryTotalBytes = memory.totalBytes;
        snapshot.memoryUsedBytes = used;
        const auto percent = percentage(used, memory.totalBytes);
        snapshot.memoryPercent = percent
            ? Reading::available(
                  *percent,
                  QStringLiteral("%1 of %2").arg(formatBytes(used), formatBytes(memory.totalBytes)))
            : Reading::unavailable(QStringLiteral("Memory counters report no capacity"));
    } else {
        snapshot.memoryPercent = Reading::unavailable(QStringLiteral("Cannot read /proc/meminfo"));
    }

    readStorage(snapshot);

    const QByteArray netdev = readFile("/proc/net/dev");
    NetCounters network;
    if (parseNetDev(netdev, network)) {
        snapshot.networkDownTotalBytes = network.receiveBytes;
        snapshot.networkUpTotalBytes = network.transmitBytes;

        if (state.hasNetwork) {
            const auto down = counterRate(network.receiveBytes, state.networkReceive, elapsedSeconds);
            const auto up = counterRate(network.transmitBytes, state.networkTransmit, elapsedSeconds);
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
        state.networkReceive = network.receiveBytes;
        state.networkTransmit = network.transmitBytes;
        state.hasNetwork = true;
    } else {
        const QString reason = QStringLiteral("Cannot read /proc/net/dev");
        snapshot.networkDownPerSecond = Reading::unavailable(reason);
        snapshot.networkUpPerSecond = Reading::unavailable(reason);
    }

    return snapshot;
}

} // namespace tmog::detail
