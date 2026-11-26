#pragma once

#include "core/metrics.h"
#include "ui/theme.h"

#include <QFrame>
#include <QVector>
#include <QWidget>

#include <optional>

class QEvent;
class QLabel;

namespace tmog::ui {

class HistoryChart;

class PerformancePage : public QWidget
{
    Q_OBJECT

public:
    explicit PerformancePage(QWidget *parent = nullptr);

    void apply(const MetricsSnapshot &snapshot);
    void setTheme(const ThemeTokens &theme);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class Subsystem {
        Cpu = 0,
        Memory,
        Network,
        Storage,
        Pressure,
    };

    struct Row
    {
        QFrame *frame = nullptr;
        QLabel *value = nullptr;
        QLabel *detail = nullptr;
        QColor accent;
    };

    void buildRail(QWidget *parent);
    void buildDetail(QWidget *parent);
    void selectSubsystem(Subsystem subsystem);
    void refreshDetail();
    void refreshRows();
    void styleRow(Subsystem subsystem);
    void appendSample(QVector<std::optional<double>> &history, const Reading &reading);

    QFrame *m_rail = nullptr;
    QFrame *m_detailCard = nullptr;
    Row m_rows[5];
    Subsystem m_selected = Subsystem::Cpu;

    HistoryChart *m_chart = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_value = nullptr;
    QLabel *m_state = nullptr;
    QLabel *m_detail = nullptr;
    QLabel *m_source = nullptr;
    QLabel *m_legendPrimary = nullptr;
    QLabel *m_legendSecondary = nullptr;
    QLabel *m_statCurrent = nullptr;
    QLabel *m_statAverage = nullptr;
    QLabel *m_statPeak = nullptr;
    QLabel *m_statSamples = nullptr;
    QLabel *m_railProcesses = nullptr;
    QLabel *m_railCpus = nullptr;
    QLabel *m_railSamples = nullptr;

    QVector<std::optional<double>> m_cpuHistory;
    QVector<std::optional<double>> m_memoryHistory;
    QVector<std::optional<double>> m_downHistory;
    QVector<std::optional<double>> m_upHistory;
    QVector<std::optional<double>> m_storageHistory;
    QVector<std::optional<double>> m_pressureHistory;

    ThemeTokens m_theme;
    MetricsSnapshot m_snapshot;
    bool m_hasSnapshot = false;
};

} // namespace tmog::ui
