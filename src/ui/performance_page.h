#pragma once

#include "core/metrics.h"
#include "ui/theme.h"

#include <QVector>
#include <QWidget>

#include <optional>

class QLabel;

namespace tmog::ui {

class HistoryChart;
class SegmentBar;

class PerformancePage : public QWidget
{
    Q_OBJECT

public:
    explicit PerformancePage(QWidget *parent = nullptr);

    void apply(const MetricsSnapshot &snapshot);
    void setTheme(const ThemeTokens &theme);

private:
    void applyReadings();
    void appendSample(QVector<std::optional<double>> &history, const Reading &reading);
    void refreshCharts();

    HistoryChart *m_cpuChart = nullptr;
    HistoryChart *m_memoryChart = nullptr;
    HistoryChart *m_networkChart = nullptr;
    HistoryChart *m_processChart = nullptr;
    SegmentBar *m_storageBar = nullptr;

    QLabel *m_pill = nullptr;
    QLabel *m_cpuValue = nullptr;
    QLabel *m_cpuState = nullptr;
    QLabel *m_cpuDetail = nullptr;
    QLabel *m_memoryValue = nullptr;
    QLabel *m_memoryState = nullptr;
    QLabel *m_memoryDetail = nullptr;
    QLabel *m_networkValue = nullptr;
    QLabel *m_networkState = nullptr;
    QLabel *m_networkDetail = nullptr;
    QLabel *m_processValue = nullptr;
    QLabel *m_processState = nullptr;
    QLabel *m_processDetail = nullptr;
    QLabel *m_storageValue = nullptr;
    QLabel *m_storageState = nullptr;
    QLabel *m_storageDetail = nullptr;

    QVector<std::optional<double>> m_cpuHistory;
    QVector<std::optional<double>> m_memoryHistory;
    QVector<std::optional<double>> m_downHistory;
    QVector<std::optional<double>> m_upHistory;
    QVector<std::optional<double>> m_processHistory;

    ThemeTokens m_theme;
    MetricsSnapshot m_snapshot;
    bool m_hasSnapshot = false;
};

} // namespace tmog::ui
