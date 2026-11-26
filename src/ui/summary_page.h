#pragma once

#include "core/metrics.h"
#include "ui/theme.h"

#include <QColor>
#include <QVector>
#include <QWidget>

#include <optional>

class QFrame;
class QLabel;

namespace tmog::ui {

class HistoryChart;
class SegmentBar;
class VerticalGauge;

class SummaryPage : public QWidget
{
    Q_OBJECT

public:
    explicit SummaryPage(QWidget *parent = nullptr);

    void apply(const MetricsSnapshot &snapshot);
    void setTheme(const ThemeTokens &theme);

private:
    struct MiniCard
    {
        QFrame *frame = nullptr;
        QLabel *value = nullptr;
        QLabel *detail = nullptr;
        QLabel *state = nullptr;
        SegmentBar *bar = nullptr;
        QColor accent;
    };

    QFrame *createMini(const QString &title,
                       const QString &iconKey,
                       const QColor &accent,
                       bool hasBar,
                       MiniCard &card,
                       QWidget *parent);
    void styleMini(MiniCard &card);
    void applyMini(MiniCard &card,
                   const Reading &reading,
                   const QString &value,
                   const QString &detail,
                   double fraction);
    void refreshCharts();
    void applyReadings();

    VerticalGauge *m_cpuGauge = nullptr;
    VerticalGauge *m_memoryGauge = nullptr;
    VerticalGauge *m_storageGauge = nullptr;
    VerticalGauge *m_pressureGauge = nullptr;

    HistoryChart *m_cpuChart = nullptr;
    HistoryChart *m_memoryChart = nullptr;
    HistoryChart *m_pressureChart = nullptr;

    QLabel *m_cpuValue = nullptr;
    QLabel *m_cpuState = nullptr;
    QLabel *m_cpuDetail = nullptr;
    QLabel *m_pressureValue = nullptr;
    QLabel *m_pressureState = nullptr;
    QLabel *m_pressureDetail = nullptr;
    QLabel *m_memoryValue = nullptr;
    QLabel *m_memoryDetail = nullptr;

    MiniCard m_storageCard;
    MiniCard m_networkCard;
    MiniCard m_processCard;
    MiniCard m_pressureCard;
    MiniCard m_cpuCard;
    MiniCard m_memoryCard;

    QVector<std::optional<double>> m_cpuHistory;
    QVector<std::optional<double>> m_memoryHistory;
    QVector<std::optional<double>> m_pressureHistory;

    ThemeTokens m_theme;
    MetricsSnapshot m_snapshot;
    bool m_hasSnapshot = false;
};

} // namespace tmog::ui
