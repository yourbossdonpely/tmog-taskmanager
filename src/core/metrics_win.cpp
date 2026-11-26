#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601
#endif
#ifndef NTDDI_VERSION
#  define NTDDI_VERSION 0x06010000
#endif

#include "core/metrics_internal.h"

#include <QString>

#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <tlhelp32.h>

namespace tmog::detail {

namespace {

quint64 fileTimeValue(const FILETIME &time)
{
    ULARGE_INTEGER value;
    value.LowPart = time.dwLowDateTime;
    value.HighPart = time.dwHighDateTime;
    return value.QuadPart;
}

void readStorage(MetricsSnapshot &snapshot)
{
    wchar_t windowsDirectory[MAX_PATH]{};
    QString root = QStringLiteral("C:\\");
    if (GetWindowsDirectoryW(windowsDirectory, MAX_PATH) > 0) {
        root = QString::fromWCharArray(windowsDirectory, 3);
        if (!root.endsWith(QLatin1Char('\\'))) {
            root.append(QLatin1Char('\\'));
        }
    }

    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER freeBytes{};
    if (!GetDiskFreeSpaceExW(reinterpret_cast<LPCWSTR>(root.utf16()),
                             nullptr,
                             &totalBytes,
                             &freeBytes)) {
        snapshot.storagePercent =
            Reading::unavailable(QStringLiteral("GetDiskFreeSpaceEx failed for %1").arg(root));
        return;
    }

    const quint64 used = totalBytes.QuadPart >= freeBytes.QuadPart
        ? totalBytes.QuadPart - freeBytes.QuadPart
        : 0;

    snapshot.storageName = QStringLiteral("System volume (%1)").arg(root.left(2));
    snapshot.storageTotalBytes = totalBytes.QuadPart;
    snapshot.storageUsedBytes = used;

    const auto percent = percentage(used, totalBytes.QuadPart);
    if (!percent) {
        snapshot.storagePercent =
            Reading::unavailable(QStringLiteral("System volume reports no capacity"));
        return;
    }
    snapshot.storagePercent = Reading::available(
        *percent,
        QStringLiteral("%1 of %2").arg(formatBytes(used), formatBytes(totalBytes.QuadPart)));
}

void readProcessPressure(MetricsSnapshot &snapshot)
{
    quint64 processCount = 0;
    bool processCountAvailable = false;
    HANDLE processes = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (processes != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (Process32FirstW(processes, &entry)) {
            do {
                ++processCount;
            } while (Process32NextW(processes, &entry));
            processCountAvailable = true;
        }
        CloseHandle(processes);
    }

    quint32 cpus = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    if (cpus == 0) {
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        cpus = info.dwNumberOfProcessors;
    }
    cpus = qMax<quint32>(1, cpus);

    snapshot.logicalCpus = cpus;
    snapshot.processCount = processCountAvailable ? processCount : 0;

    bool queueAvailable = false;
    double queueLength = 0.0;
    PDH_HQUERY query = nullptr;
    PDH_HCOUNTER counter = nullptr;
    if (PdhOpenQueryW(nullptr, 0, &query) == ERROR_SUCCESS) {
        PDH_STATUS status = PdhAddEnglishCounterW(
            query, L"\\System\\Processor Queue Length", 0, &counter);
        if (status != ERROR_SUCCESS) {
            status = PdhAddCounterW(
                query, L"\\System\\Processor Queue Length", 0, &counter);
        }
        if (status == ERROR_SUCCESS && PdhCollectQueryData(query) == ERROR_SUCCESS) {
            PDH_FMT_COUNTERVALUE value{};
            if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value)
                    == ERROR_SUCCESS
                && value.CStatus == PDH_CSTATUS_VALID_DATA) {
                queueLength = value.doubleValue;
                queueAvailable = true;
            }
        }
        PdhCloseQuery(query);
    }

    if (!queueAvailable) {
        snapshot.processPressure = Reading::unavailable(
            processCountAvailable
                ? QStringLiteral("Processor queue counter unavailable · %1 processes")
                      .arg(processCount)
                : QStringLiteral("Processor queue counter unavailable"));
        return;
    }

    snapshot.runnableCount = static_cast<quint64>(qMax(0.0, queueLength));
    QString detail = QStringLiteral("%1 queued threads / %2 CPUs")
                         .arg(qRound64(queueLength))
                         .arg(cpus);
    if (processCountAvailable) {
        detail += QStringLiteral(" · %1 processes").arg(processCount);
    }
    snapshot.processPressure = Reading::available(
        queueLength / static_cast<double>(cpus), detail);
}

void readNetwork(PlatformState &state, MetricsSnapshot &snapshot, double elapsedSeconds)
{
    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) != NO_ERROR || table == nullptr) {
        const QString reason = QStringLiteral("GetIfTable2 failed");
        snapshot.networkDownPerSecond = Reading::unavailable(reason);
        snapshot.networkUpPerSecond = Reading::unavailable(reason);
        return;
    }

    quint64 receive = 0;
    quint64 transmit = 0;
    for (ULONG index = 0; index < table->NumEntries; ++index) {
        const MIB_IF_ROW2 &row = table->Table[index];
        if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK) {
            continue;
        }
        receive += row.InOctets;
        transmit += row.OutOctets;
    }
    FreeMibTable(table);

    snapshot.networkDownTotalBytes = receive;
    snapshot.networkUpTotalBytes = transmit;

    if (state.hasNetwork) {
        const auto down = counterRate(receive, state.networkReceive, elapsedSeconds);
        const auto up = counterRate(transmit, state.networkTransmit, elapsedSeconds);
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

    state.networkReceive = receive;
    state.networkTransmit = transmit;
    state.hasNetwork = true;
}

} // namespace

MetricsSnapshot samplePlatform(PlatformState &state, double elapsedSeconds)
{
    MetricsSnapshot snapshot;

    FILETIME idleTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        const quint64 idle = fileTimeValue(idleTime);
        const quint64 kernel = fileTimeValue(kernelTime);
        const quint64 user = fileTimeValue(userTime);

        if (state.hasCpu) {
            if (idle >= state.winIdle && kernel >= state.winKernel && user >= state.winUser) {
                const quint64 idleDelta = idle - state.winIdle;
                const quint64 totalDelta = (kernel - state.winKernel) + (user - state.winUser);
                const auto percent = totalDelta > 0 && idleDelta <= totalDelta
                    ? percentage(totalDelta - idleDelta, totalDelta)
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

        state.winIdle = idle;
        state.winKernel = kernel;
        state.winUser = user;
        state.hasCpu = true;
    } else {
        snapshot.cpuPercent = Reading::unavailable(QStringLiteral("GetSystemTimes failed"));
    }

    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory)) {
        const quint64 used = memory.ullTotalPhys >= memory.ullAvailPhys
            ? memory.ullTotalPhys - memory.ullAvailPhys
            : 0;
        snapshot.memoryTotalBytes = memory.ullTotalPhys;
        snapshot.memoryUsedBytes = used;
        const auto percent = percentage(used, memory.ullTotalPhys);
        snapshot.memoryPercent = percent
            ? Reading::available(
                  *percent,
                  QStringLiteral("%1 of %2").arg(formatBytes(used),
                                                 formatBytes(memory.ullTotalPhys)))
            : Reading::unavailable(QStringLiteral("Memory reports no capacity"));
    } else {
        snapshot.memoryPercent = Reading::unavailable(QStringLiteral("GlobalMemoryStatusEx failed"));
    }

    readStorage(snapshot);
    readProcessPressure(snapshot);
    readNetwork(state, snapshot, elapsedSeconds);

    return snapshot;
}

} // namespace tmog::detail
