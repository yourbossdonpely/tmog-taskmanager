#pragma once

#include <QColor>
#include <QVector>
#include <QWidget>

#include <optional>

namespace tmog::ui {

class HistoryChart : public QWidget
{
    Q_OBJECT

public:
    enum class Scale {
        Percent,
        Rate,
    };

    explicit HistoryChart(Scale scale, QWidget *parent = nullptr);

    void setPrimary(const QVector<std::optional<double>> &values, const QColor &color);
    void setSecondary(const QVector<std::optional<double>> &values, const QColor &color);
    void setGridColor(const QColor &color);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void recomputeMaximum();

    Scale m_scale;
    QVector<std::optional<double>> m_primary;
    QVector<std::optional<double>> m_secondary;
    QColor m_primaryColor;
    QColor m_secondaryColor;
    QColor m_gridColor;
    double m_maximum = 100.0;
};

} // namespace tmog::ui
