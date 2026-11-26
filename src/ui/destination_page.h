#pragma once

#include <QColor>
#include <QString>
#include <QWidget>

class QLabel;

namespace tmog::ui {

class SegmentBar : public QWidget
{
    Q_OBJECT

public:
    explicit SegmentBar(QWidget *parent = nullptr);

    void setAccent(const QColor &color);
    void setValue(double value);
    void setReadingAvailable(bool available);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_accent;
    double m_value = 0.0;
    bool m_available = true;
};

class DestinationPage : public QWidget
{
    Q_OBJECT

public:
    DestinationPage(const QString &title,
                    const QString &note,
                    const QColor &accent,
                    const QString &iconKey,
                    QWidget *parent = nullptr);
};

} // namespace tmog::ui
