#pragma once

#include <QColor>
#include <QRectF>
#include <QString>
#include <QWidget>

class QPainter;

namespace tmog::ui {

void drawLineIcon(QPainter &painter,
                  const QString &key,
                  const QRectF &rect,
                  const QColor &color,
                  qreal strokeWidth = 1.5);

class LineIcon : public QWidget
{
    Q_OBJECT

public:
    explicit LineIcon(const QString &key = QStringLiteral("summary"), QWidget *parent = nullptr);

    void setIconKey(const QString &key);
    void setColor(const QColor &color);
    void setIconSize(int pixels);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_key;
    QColor m_color;
    int m_size = 22;
};

} // namespace tmog::ui
