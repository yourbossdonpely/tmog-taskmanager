#pragma once

#include <QByteArray>

namespace tmog {

struct CpuTimes
{
    quint64 user = 0;
    quint64 nice = 0;
    quint64 system = 0;
    quint64 idle = 0;
    quint64 iowait = 0;
    quint64 irq = 0;
    quint64 softirq = 0;
    quint64 steal = 0;

    quint64 total() const;
    quint64 idleTotal() const;
    quint64 busy() const;
};

struct MemInfo
{
    quint64 totalBytes = 0;
    quint64 availableBytes = 0;
};

struct NetCounters
{
    quint64 receiveBytes = 0;
    quint64 transmitBytes = 0;
};

struct ProcPressure
{
    quint64 running = 0;
    quint64 blocked = 0;
};

bool parseProcStat(const QByteArray &contents, CpuTimes &out);
bool parseMeminfo(const QByteArray &contents, MemInfo &out);
bool parseNetDev(const QByteArray &contents, NetCounters &out);
bool parseProcPressure(const QByteArray &contents, ProcPressure &out);

} // namespace tmog
