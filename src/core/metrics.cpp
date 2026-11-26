#include "core/metrics.h"

#include "core/metrics_internal.h"

#include <QStringList>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

namespace tmog {

Reading Reading::available(double value, QString detail)
{
    return {Availability::Available, value, std::move(detail)};
}

Reading Reading::pending(QString detail)
{
    return {Availability::Pending, 0.0, std::move(detail)};
}

Reading Reading::unavailable(QString detail)
{
    return {Availability::Unavailable, 0.0, std::move(detail)};
}

Reading Reading::denied(QString detail)
{
    return {Availability::Denied, 0.0, std::move(detail)};
}

struct MetricsSampler::State
{
    detail::PlatformState platform;
    std::chrono::steady_clock::time_point lastSample;
    bool hasLastSample = false;
};

MetricsSampler::MetricsSampler()
    : m_state(std::make_unique<State>())
{
}

MetricsSampler::~MetricsSampler() = default;

MetricsSnapshot MetricsSampler::sample()
{
    const auto now = std::chrono::steady_clock::now();
    double elapsed = 0.0;
    if (m_state->hasLastSample) {
        elapsed = std::chrono::duration<double>(now - m_state->lastSample).count();
    }
    m_state->hasLastSample = true;
    m_state->lastSample = now;

    return detail::samplePlatform(m_state->platform, elapsed);
}

void MetricsSampler::reset()
{
    m_state->platform = detail::PlatformState{};
    m_state->hasLastSample = false;
}

std::optional<double> percentage(quint64 part, quint64 whole)
{
    if (whole == 0) {
        return std::nullopt;
    }
    const double value = 100.0 * static_cast<double>(part) / static_cast<double>(whole);
    return std::clamp(value, 0.0, 100.0);
}

std::optional<double> counterRate(quint64 current, quint64 previous, double seconds)
{
    if (current < previous || seconds <= 0.0) {
        return std::nullopt;
    }
    return static_cast<double>(current - previous) / seconds;
}

QString formatBytes(quint64 bytes)
{
    static const QStringList units{
        QStringLiteral("B"),
        QStringLiteral("KiB"),
        QStringLiteral("MiB"),
        QStringLiteral("GiB"),
        QStringLiteral("TiB"),
        QStringLiteral("PiB"),
    };

    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < units.size() - 1) {
        value /= 1024.0;
        ++unit;
    }

    if (unit == 0) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    return QStringLiteral("%1 %2").arg(value, 0, 'f', value < 100.0 ? 1 : 0).arg(units.at(unit));
}

QString formatRate(double bytesPerSecond)
{
    if (!std::isfinite(bytesPerSecond) || bytesPerSecond <= 0.0) {
        return QStringLiteral("0 B/s");
    }
    return formatBytes(static_cast<quint64>(std::llround(bytesPerSecond))) + QStringLiteral("/s");
}

QString formatPercent(double value)
{
    if (!std::isfinite(value)) {
        return QStringLiteral("—");
    }
    const double clamped = std::clamp(value, 0.0, 100.0);
    if (clamped > 0.0 && clamped < 10.0) {
        return QStringLiteral("%1%").arg(clamped, 0, 'f', 1);
    }
    return QStringLiteral("%1%").arg(clamped, 0, 'f', 0);
}

QString formatRatio(double value)
{
    if (!std::isfinite(value) || value < 0.0) {
        return QStringLiteral("—");
    }
    return QStringLiteral("%1×").arg(value, 0, 'f', value < 10.0 ? 2 : 1);
}

} // namespace tmog
