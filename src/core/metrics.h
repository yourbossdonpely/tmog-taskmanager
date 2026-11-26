#pragma once

#include <QString>

#include <memory>
#include <optional>

namespace tmog {

enum class Availability {
    Available,
    Pending,
    Unavailable,
    Denied,
};

struct Reading
{
    Availability state = Availability::Unavailable;
    double value = 0.0;
    QString detail;

    static Reading available(double value, QString detail = {});
    static Reading pending(QString detail);
    static Reading unavailable(QString detail);
    static Reading denied(QString detail);

    bool hasValue() const { return state == Availability::Available; }
};

struct MetricsSnapshot
{
    Reading cpuPercent;
    Reading memoryPercent;
    Reading storagePercent;
    Reading networkDownPerSecond;
    Reading networkUpPerSecond;

    quint64 memoryUsedBytes = 0;
    quint64 memoryTotalBytes = 0;
    quint64 storageUsedBytes = 0;
    quint64 storageTotalBytes = 0;
    quint64 networkDownTotalBytes = 0;
    quint64 networkUpTotalBytes = 0;
    QString storageName;
};

class MetricsSampler
{
public:
    MetricsSampler();
    ~MetricsSampler();

    MetricsSampler(const MetricsSampler &) = delete;
    MetricsSampler &operator=(const MetricsSampler &) = delete;

    MetricsSnapshot sample();
    void reset();

private:
    struct State;
    std::unique_ptr<State> m_state;
};

std::optional<double> percentage(quint64 part, quint64 whole);
std::optional<double> counterRate(quint64 current, quint64 previous, double seconds);

QString formatBytes(quint64 bytes);
QString formatRate(double bytesPerSecond);
QString formatPercent(double value);

} // namespace tmog
