#include "ui/icons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <cmath>

namespace tmog::ui {

namespace {

QPointF at(qreal x, qreal y)
{
    return {x, y};
}

void drawSummary(QPainter &p)
{
    p.drawEllipse(QRectF(4.0, 4.0, 16.0, 16.0));
    p.drawArc(QRectF(7.0, 7.0, 10.0, 10.0), 30 * 16, 210 * 16);
    p.drawLine(at(12.0, 12.0), at(15.4, 8.6));
}

void drawPerformance(QPainter &p)
{
    QPainterPath path;
    path.moveTo(2.5, 12.5);
    path.lineTo(5.0, 12.5);
    path.lineTo(7.0, 6.5);
    path.lineTo(9.5, 17.0);
    path.lineTo(12.0, 9.5);
    path.lineTo(14.0, 12.5);
    path.lineTo(21.5, 12.5);
    p.drawPath(path);
}

void drawProcesses(QPainter &p)
{
    const QRectF rows[3] = {
        QRectF(3.0, 4.0, 18.0, 4.0),
        QRectF(3.0, 10.0, 18.0, 4.0),
        QRectF(3.0, 16.0, 18.0, 4.0),
    };
    for (const QRectF &row : rows) {
        p.drawRoundedRect(row, 1.2, 1.2);
        p.drawLine(row.topLeft() + QPointF(2.2, 2.0), row.topLeft() + QPointF(4.6, 2.0));
    }
}

void drawInfo(QPainter &p)
{
    p.drawEllipse(QRectF(3.5, 3.5, 17.0, 17.0));
    p.drawLine(at(12.0, 10.5), at(12.0, 17.0));
    p.drawPoint(at(12.0, 7.0));
}

void drawHistory(QPainter &p)
{
    p.drawEllipse(QRectF(3.5, 3.5, 17.0, 17.0));
    p.drawLine(at(12.0, 7.0), at(12.0, 12.0));
    p.drawLine(at(12.0, 12.0), at(15.8, 14.3));
    p.drawLine(at(2.0, 8.0), at(2.0, 16.0));
    p.drawLine(at(2.0, 12.0), at(4.5, 12.0));
}

void drawStartup(QPainter &p)
{
    p.drawArc(QRectF(4.5, 4.5, 15.0, 15.0), 55 * 16, 250 * 16);
    p.drawLine(at(12.0, 2.8), at(12.0, 11.0));
}

void drawUsers(QPainter &p)
{
    p.drawEllipse(QRectF(4.0, 4.0, 6.0, 6.0));
    p.drawArc(QRectF(1.8, 11.0, 10.5, 9.0), 0, 180 * 16);
    p.drawEllipse(QRectF(13.5, 6.0, 5.0, 5.0));
    p.drawArc(QRectF(12.5, 12.5, 9.5, 7.5), 0, 180 * 16);
}

void drawServices(QPainter &p)
{
    p.drawLine(at(4.0, 6.5), at(20.0, 6.5));
    p.drawLine(at(4.0, 12.0), at(20.0, 12.0));
    p.drawLine(at(4.0, 17.5), at(20.0, 17.5));
    p.drawEllipse(QRectF(7.0, 4.6, 3.8, 3.8));
    p.drawEllipse(QRectF(13.0, 10.1, 3.8, 3.8));
    p.drawEllipse(QRectF(8.0, 15.6, 3.8, 3.8));
}

void drawPower(QPainter &p)
{
    QPainterPath bolt;
    bolt.moveTo(13.5, 2.0);
    bolt.lineTo(5.5, 13.0);
    bolt.lineTo(11.0, 13.0);
    bolt.lineTo(10.0, 22.0);
    bolt.lineTo(18.5, 10.0);
    bolt.lineTo(13.0, 10.0);
    bolt.closeSubpath();
    p.drawPath(bolt);
}

void drawRecorder(QPainter &p)
{
    p.drawEllipse(QRectF(3.5, 3.5, 17.0, 17.0));
    p.setBrush(p.pen().color());
    p.drawEllipse(QRectF(9.0, 9.0, 6.0, 6.0));
    p.setBrush(Qt::NoBrush);
}

void drawConnections(QPainter &p)
{
    p.drawLine(at(6.0, 7.0), at(12.0, 12.0));
    p.drawLine(at(12.0, 12.0), at(18.5, 7.0));
    p.drawLine(at(12.0, 12.0), at(12.0, 19.0));
    p.drawEllipse(QRectF(3.5, 4.5, 5.0, 5.0));
    p.drawEllipse(QRectF(16.0, 4.5, 5.0, 5.0));
    p.drawEllipse(QRectF(9.5, 17.0, 5.0, 5.0));
}

void drawApps(QPainter &p)
{
    p.drawRoundedRect(QRectF(4.0, 4.0, 6.5, 6.5), 1.4, 1.4);
    p.drawRoundedRect(QRectF(13.5, 4.0, 6.5, 6.5), 1.4, 1.4);
    p.drawRoundedRect(QRectF(4.0, 13.5, 6.5, 6.5), 1.4, 1.4);
    p.drawRoundedRect(QRectF(13.5, 13.5, 6.5, 6.5), 1.4, 1.4);
}

void drawDrivers(QPainter &p)
{
    p.drawRoundedRect(QRectF(6.0, 6.0, 12.0, 12.0), 2.0, 2.0);
    p.drawRoundedRect(QRectF(9.0, 9.0, 6.0, 6.0), 1.0, 1.0);
    for (int i = 0; i < 3; ++i) {
        const qreal offset = 8.5 + i * 3.5;
        p.drawLine(at(offset, 2.5), at(offset, 6.0));
        p.drawLine(at(offset, 18.0), at(offset, 21.5));
        p.drawLine(at(2.5, offset), at(6.0, offset));
        p.drawLine(at(18.0, offset), at(21.5, offset));
    }
}

void drawDeepSpace(QPainter &p)
{
    p.drawEllipse(QRectF(3.0, 8.0, 18.0, 8.0));
    p.drawEllipse(QRectF(9.5, 9.5, 5.0, 5.0));
    p.setBrush(p.pen().color());
    p.drawEllipse(QRectF(18.0, 6.0, 2.6, 2.6));
    p.setBrush(Qt::NoBrush);
}

void drawBenchmarks(QPainter &p)
{
    p.drawLine(at(3.0, 20.0), at(21.0, 20.0));
    p.drawLine(at(5.0, 18.0), at(5.0, 13.0));
    p.drawLine(at(9.5, 18.0), at(9.5, 9.0));
    p.drawLine(at(14.0, 18.0), at(14.0, 5.0));
    p.drawLine(at(18.5, 18.0), at(18.5, 11.0));
}

void drawSettings(QPainter &p)
{
    p.drawEllipse(QRectF(8.0, 8.0, 8.0, 8.0));
    for (int i = 0; i < 8; ++i) {
        const qreal angle = (3.14159265358979323846 / 4.0) * i;
        const QPointF inner = at(12.0 + std::cos(angle) * 6.2, 12.0 + std::sin(angle) * 6.2);
        const QPointF outer = at(12.0 + std::cos(angle) * 8.5, 12.0 + std::sin(angle) * 8.5);
        p.drawLine(inner, outer);
    }
}

void drawColors(QPainter &p)
{
    p.drawEllipse(QRectF(3.5, 3.5, 17.0, 17.0));
    p.setBrush(p.pen().color());
    p.drawEllipse(QRectF(7.0, 7.0, 2.2, 2.2));
    p.drawEllipse(QRectF(12.0, 6.2, 2.2, 2.2));
    p.drawEllipse(QRectF(16.2, 10.5, 2.2, 2.2));
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(7.5, 10.0, 9.0, 8.5), 300 * 16, 170 * 16);
}

void drawMenu(QPainter &p)
{
    p.drawLine(at(4.0, 7.0), at(20.0, 7.0));
    p.drawLine(at(4.0, 12.0), at(20.0, 12.0));
    p.drawLine(at(4.0, 17.0), at(20.0, 17.0));
}

} // namespace

void drawLineIcon(QPainter &painter,
                  const QString &key,
                  const QRectF &rect,
                  const QColor &color,
                  qreal strokeWidth)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(rect.topLeft());
    painter.scale(rect.width() / 24.0, rect.height() / 24.0);
    painter.setPen(QPen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    if (key == QStringLiteral("performance")) {
        drawPerformance(painter);
    } else if (key == QStringLiteral("processes")) {
        drawProcesses(painter);
    } else if (key == QStringLiteral("system-info")) {
        drawInfo(painter);
    } else if (key == QStringLiteral("app-history")) {
        drawHistory(painter);
    } else if (key == QStringLiteral("startup-apps")) {
        drawStartup(painter);
    } else if (key == QStringLiteral("users")) {
        drawUsers(painter);
    } else if (key == QStringLiteral("services")) {
        drawServices(painter);
    } else if (key == QStringLiteral("power-freq")) {
        drawPower(painter);
    } else if (key == QStringLiteral("flight-recorder")) {
        drawRecorder(painter);
    } else if (key == QStringLiteral("connections")) {
        drawConnections(painter);
    } else if (key == QStringLiteral("installed-apps")) {
        drawApps(painter);
    } else if (key == QStringLiteral("drivers")) {
        drawDrivers(painter);
    } else if (key == QStringLiteral("deep-space")) {
        drawDeepSpace(painter);
    } else if (key == QStringLiteral("benchmarks")) {
        drawBenchmarks(painter);
    } else if (key == QStringLiteral("settings")) {
        drawSettings(painter);
    } else if (key == QStringLiteral("colors")) {
        drawColors(painter);
    } else if (key == QStringLiteral("menu")) {
        drawMenu(painter);
    } else {
        drawSummary(painter);
    }

    painter.restore();
}

LineIcon::LineIcon(const QString &key, QWidget *parent)
    : QWidget(parent)
    , m_key(key)
    , m_color(QColor(QStringLiteral("#a0a09a")))
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void LineIcon::setIconKey(const QString &key)
{
    if (m_key == key) {
        return;
    }
    m_key = key;
    update();
}

void LineIcon::setColor(const QColor &color)
{
    if (m_color == color) {
        return;
    }
    m_color = color;
    update();
}

void LineIcon::setIconSize(int pixels)
{
    m_size = qMax(8, pixels);
    setFixedSize(m_size, m_size);
    updateGeometry();
    update();
}

QSize LineIcon::sizeHint() const
{
    return {m_size, m_size};
}

void LineIcon::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    drawLineIcon(painter,
                 m_key,
                 QRectF(0.0, 0.0, width(), height()),
                 m_color,
                 qMax(1.0, m_size / 14.0));
}

} // namespace tmog::ui
