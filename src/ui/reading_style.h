#pragma once

#include "core/metrics.h"
#include "ui/theme.h"

#include <QLabel>

namespace tmog::ui {

inline QString readingStateText(const Reading &reading)
{
    switch (reading.state) {
    case Availability::Available:
        return QStringLiteral("LIVE");
    case Availability::Pending:
        return QStringLiteral("SAMPLING");
    case Availability::Unavailable:
        return QStringLiteral("UNAVAILABLE");
    case Availability::Denied:
        return QStringLiteral("DENIED");
    }
    return QStringLiteral("UNAVAILABLE");
}

inline QColor readingStateColor(const Reading &reading,
                                const ThemeTokens &theme,
                                const QColor &accent)
{
    switch (reading.state) {
    case Availability::Available:
        return accent;
    case Availability::Pending:
        return theme.textMuted;
    case Availability::Unavailable:
        return theme.textMuted;
    case Availability::Denied:
        return theme.danger;
    }
    return theme.textMuted;
}

inline void applyReadingState(QLabel *label,
                              const Reading &reading,
                              const ThemeTokens &theme,
                              const QColor &accent)
{
    label->setText(readingStateText(reading));
    label->setStyleSheet(
        QStringLiteral("color: %1;").arg(readingStateColor(reading, theme, accent).name()));
}

inline Reading combineReadings(const Reading &first, const Reading &second)
{
    if (first.state == Availability::Denied || second.state == Availability::Denied) {
        return Reading::denied(first.state == Availability::Denied ? first.detail : second.detail);
    }
    if (first.state == Availability::Unavailable || second.state == Availability::Unavailable) {
        return Reading::unavailable(
            first.state == Availability::Unavailable ? first.detail : second.detail);
    }
    if (first.state == Availability::Pending || second.state == Availability::Pending) {
        return Reading::pending(first.state == Availability::Pending ? first.detail : second.detail);
    }
    return Reading::available(0.0);
}

} // namespace tmog::ui
