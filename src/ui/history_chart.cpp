#include "ui/history_chart.h"

#include "core/metrics.h"

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QPointF>

#include <algorithm>

namespace tmog::ui {

HistoryChart::HistoryChart(Scale scale, QWidget *parent)
    : QWidget(parent)
    , m_scale(scale)
    , m_primaryColor(QColor("#69e06b"))
    , m_secondaryColor(QColor("#5b9dff"))
    , m_gridColor(QColor("#24303c"))
{
    setMinimumHeight(150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void HistoryChart::setPrimary(const QVector<std::optional<double>> &values, const QColor &color)
{
    m_primary = values;
    m_primaryColor = color;
    recomputeMaximum();
    update();
}

void HistoryChart::setSecondary(const QVector<std::optional<double>> &values, const QColor &color)
{
    m_secondary = values;
    m_secondaryColor = color;
    recomputeMaximum();
    update();
}

void HistoryChart::setGridColor(const QColor &color)
{
    m_gridColor = color;
    update();
}

QSize HistoryChart::sizeHint() const
{
    return {420, 170};
}

void HistoryChart::recomputeMaximum()
{
    if (m_scale == Scale::Percent) {
        m_maximum = 100.0;
        return;
    }

    double maximum = 0.0;
    for (const auto &value : m_primary) {
        if (value) {
            maximum = std::max(maximum, *value);
        }
    }
    for (const auto &value : m_secondary) {
        if (value) {
            maximum = std::max(maximum, *value);
        }
    }
    m_maximum = std::max(1.0, maximum * 1.15);
}

void HistoryChart::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFont axisFont = font();
    axisFont.setPixelSize(10);
    painter.setFont(axisFont);

    const int leftGutter = m_scale == Scale::Percent ? 34 : 58;
    const QRectF plot = QRectF(rect()).adjusted(leftGutter, 10, -10, -8);
    if (plot.width() <= 4.0 || plot.height() <= 4.0) {
        return;
    }

    const QColor labelColor = palette().color(QPalette::PlaceholderText);
    QColor grid = m_gridColor;
    grid.setAlpha(120);

    constexpr int divisions = 4;
    painter.setPen(QPen(grid, 1.0));
    for (int index = 0; index <= divisions; ++index) {
        const double fraction = static_cast<double>(index) / divisions;
        const double y = plot.bottom() - fraction * plot.height();
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));

        const QString label = m_scale == Scale::Percent
            ? QStringLiteral("%1%").arg(qRound(fraction * 100.0))
            : formatRate(fraction * m_maximum);
        painter.setPen(labelColor);
        painter.drawText(QRectF(0, y - 8, leftGutter - 7, 16),
                         Qt::AlignRight | Qt::AlignVCenter,
                         label);
        painter.setPen(QPen(grid, 1.0));
    }

    const bool hasData = std::any_of(m_primary.cbegin(), m_primary.cend(), [](const auto &value) {
        return value.has_value();
    }) || std::any_of(m_secondary.cbegin(), m_secondary.cend(), [](const auto &value) {
        return value.has_value();
    });

    if (!hasData) {
        painter.setPen(labelColor);
        painter.drawText(plot, Qt::AlignCenter, QStringLiteral("Waiting for samples"));
        return;
    }

    const int count = std::max(m_primary.size(), m_secondary.size());
    if (count <= 0) {
        return;
    }
    const double step = count > 1 ? plot.width() / (count - 1) : 0.0;

    const auto drawSeries = [&](const QVector<std::optional<double>> &series,
                                const QColor &color,
                                bool fill,
                                bool drawLatestDot) {
        QVector<QPointF> run;
        run.reserve(count);
        QPointF lastPoint;
        bool hasLastPoint = false;

        const auto flush = [&]() {
            if (run.isEmpty()) {
                return;
            }
            if (run.size() >= 2) {
                QPainterPath path(run.first());
                for (int index = 1; index < run.size(); ++index) {
                    path.lineTo(run.at(index));
                }
                if (fill) {
                    QPainterPath area = path;
                    area.lineTo(run.last().x(), plot.bottom());
                    area.lineTo(run.first().x(), plot.bottom());
                    area.closeSubpath();
                    QColor fillColor = color;
                    fillColor.setAlpha(30);
                    painter.fillPath(area, fillColor);
                }
                painter.setPen(QPen(color, 1.8));
                painter.setBrush(Qt::NoBrush);
                painter.drawPath(path);
            }
            run.clear();
        };

        for (int index = 0; index < series.size(); ++index) {
            if (!series.at(index)) {
                flush();
                continue;
            }
            const double value = std::clamp(*series.at(index), 0.0, m_maximum);
            const double x = plot.left() + index * step;
            const double y = plot.bottom() - (value / m_maximum) * plot.height();
            const QPointF point(x, y);
            if (run.isEmpty()) {
                run.append(point);
            } else {
                run.append(point);
            }
            lastPoint = point;
            hasLastPoint = true;
        }
        flush();

        if (drawLatestDot && hasLastPoint) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawEllipse(lastPoint, 2.6, 2.6);
        }
    };

    drawSeries(m_primary, m_primaryColor, m_secondary.isEmpty(), true);
    if (!m_secondary.isEmpty()) {
        drawSeries(m_secondary, m_secondaryColor, false, true);
    }
}

} // namespace tmog::ui
