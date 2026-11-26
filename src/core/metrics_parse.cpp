#include "core/metrics_parse.h"

#include <QList>

namespace tmog {

quint64 CpuTimes::total() const
{
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

quint64 CpuTimes::idleTotal() const
{
    return idle + iowait;
}

quint64 CpuTimes::busy() const
{
    return total() - idleTotal();
}

namespace {

QList<QByteArray> fields(const QByteArray &line)
{
    QList<QByteArray> result;
    for (const QByteArray &field : line.simplified().split(' ')) {
        if (!field.isEmpty()) {
            result.append(field);
        }
    }
    return result;
}

bool numberAt(const QList<QByteArray> &values, int index, quint64 &out)
{
    if (index < 0 || index >= values.size()) {
        return false;
    }
    bool ok = false;
    const quint64 value = values.at(index).toULongLong(&ok);
    if (!ok) {
        return false;
    }
    out = value;
    return true;
}

} // namespace

bool parseProcStat(const QByteArray &contents, CpuTimes &out)
{
    for (const QByteArray &line : contents.split('\n')) {
        if (!line.startsWith("cpu ")) {
            continue;
        }

        const QList<QByteArray> values = fields(line);
        CpuTimes parsed;
        if (!numberAt(values, 1, parsed.user)
            || !numberAt(values, 2, parsed.nice)
            || !numberAt(values, 3, parsed.system)
            || !numberAt(values, 4, parsed.idle)) {
            return false;
        }
        numberAt(values, 5, parsed.iowait);
        numberAt(values, 6, parsed.irq);
        numberAt(values, 7, parsed.softirq);
        numberAt(values, 8, parsed.steal);
        out = parsed;
        return out.total() > 0;
    }
    return false;
}

bool parseMeminfo(const QByteArray &contents, MemInfo &out)
{
    quint64 totalKb = 0;
    quint64 availableKb = 0;
    quint64 freeKb = 0;
    quint64 buffersKb = 0;
    quint64 cachedKb = 0;

    for (const QByteArray &line : contents.split('\n')) {
        const QList<QByteArray> values = fields(line);
        if (values.size() < 2) {
            continue;
        }
        bool ok = false;
        const quint64 value = values.at(1).toULongLong(&ok);
        if (!ok) {
            continue;
        }
        const QByteArray key = values.at(0).chopped(1);
        if (key == "MemTotal") {
            totalKb = value;
        } else if (key == "MemAvailable") {
            availableKb = value;
        } else if (key == "MemFree") {
            freeKb = value;
        } else if (key == "Buffers") {
            buffersKb = value;
        } else if (key == "Cached") {
            cachedKb = value;
        }
    }

    if (totalKb == 0) {
        return false;
    }
    if (availableKb == 0) {
        availableKb = freeKb + buffersKb + cachedKb;
    }

    MemInfo parsed;
    parsed.totalBytes = totalKb * 1024ULL;
    parsed.availableBytes = qMin(availableKb, totalKb) * 1024ULL;
    out = parsed;
    return out.totalBytes > 0;
}

bool parseNetDev(const QByteArray &contents, NetCounters &out)
{
    NetCounters parsed;
    int interfaces = 0;

    const QList<QByteArray> lines = contents.split('\n');
    for (int index = 2; index < lines.size(); ++index) {
        const QByteArray &line = lines.at(index);
        const int colon = line.indexOf(':');
        if (colon < 0) {
            continue;
        }
        const QByteArray name = line.left(colon).trimmed();
        if (name.isEmpty() || name == "lo") {
            continue;
        }

        const QList<QByteArray> values = fields(line.mid(colon + 1));
        quint64 receive = 0;
        quint64 transmit = 0;
        if (!numberAt(values, 0, receive) || !numberAt(values, 8, transmit)) {
            continue;
        }
        parsed.receiveBytes += receive;
        parsed.transmitBytes += transmit;
        ++interfaces;
    }

    out = parsed;
    return interfaces > 0;
}

} // namespace tmog
