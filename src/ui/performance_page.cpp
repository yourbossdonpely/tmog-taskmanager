#include "ui/performance_page.h"

#include "ui/destination_page.h"
#include "ui/history_chart.h"
#include "ui/reading_style.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace tmog::ui {

namespace {

constexpr int kHistorySamples = 90;

QLabel *label(const QString &text, const char *objectName, QWidget *parent)
{
    auto *item = new QLabel(text, parent);
    item->setObjectName(QString::fromLatin1(objectName));
    item->setWordWrap(true);
    return item;
}

QWidget *legendItem(const QColor &color, const QString &text, QWidget *parent)
{
    auto *item = new QWidget(parent);
    auto *layout = new QHBoxLayout(item);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *dot = new QLabel(item);
    dot->setFixedSize(8, 8);
    dot->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 4px;")
                           .arg(color.name()));
    layout->addWidget(dot);
    layout->addWidget(label(text, "ChartLegend", item));
    return item;
}

} // namespace

PerformancePage::PerformancePage(QWidget *parent)
    : QWidget(parent)
    , m_theme(themeById(QStringLiteral("dark")))
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("PageHeader"));
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(26, 22, 26, 12);
    headerLayout->setSpacing(12);

    auto *titles = new QVBoxLayout();
    titles->setSpacing(4);
    titles->addWidget(label(QStringLiteral("Performance"), "PageTitle", header));
    titles->addWidget(label(QStringLiteral("History for every subsystem."), "PageSubtitle", header));
    headerLayout->addLayout(titles, 1);

    m_pill = label(QStringLiteral("STARTING"), "StatusPill", header);
    headerLayout->addWidget(m_pill, 0, Qt::AlignTop);
    outer->addWidget(header);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setObjectName(QStringLiteral("ScrollViewport"));
    scroll->viewport()->setAutoFillBackground(false);

    auto *content = new QWidget(scroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(26, 8, 26, 26);
    contentLayout->setSpacing(12);

    auto *grid = new QGridLayout();
    grid->setSpacing(12);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    const auto createChartCard = [this](const QString &title,
                                        HistoryChart *chart,
                                        QLabel *&value,
                                        QLabel *&state,
                                        QLabel *&detail,
                                        bool withLegend,
                                        QWidget *parent) {
        auto *card = new QFrame(parent);
        card->setObjectName(QStringLiteral("Card"));
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(16, 13, 16, 13);
        layout->setSpacing(5);

        auto *top = new QHBoxLayout();
        top->setSpacing(8);
        top->addWidget(label(title, "CardTitle", card));
        top->addStretch();
        value = label(QStringLiteral("—"), "MetricValueCompact", card);
        value->setWordWrap(false);
        state = label(QString(), "MetricState", card);
        state->setText(QStringLiteral("WAITING"));
        state->setStyleSheet(QStringLiteral("color: %1;").arg(m_theme.textMuted.name()));
        top->addWidget(value);
        top->addWidget(state);
        layout->addLayout(top);

        detail = label(QStringLiteral("Starting measurement providers…"), "MetricDetail", card);
        layout->addWidget(detail);
        layout->addWidget(chart);

        if (withLegend) {
            auto *legend = new QHBoxLayout();
            legend->setSpacing(14);
            legend->addWidget(legendItem(m_theme.network, QStringLiteral("DOWN"), card));
            legend->addWidget(legendItem(m_theme.memory, QStringLiteral("UP"), card));
            legend->addStretch();
            layout->addLayout(legend);
        }
        return card;
    };

    m_cpuChart = new HistoryChart(HistoryChart::Scale::Percent, content);
    m_memoryChart = new HistoryChart(HistoryChart::Scale::Percent, content);
    m_networkChart = new HistoryChart(HistoryChart::Scale::Rate, content);
    m_processChart = new HistoryChart(HistoryChart::Scale::Ratio, content);

    grid->addWidget(createChartCard(QStringLiteral("CPU"),
                                     m_cpuChart,
                                     m_cpuValue,
                                     m_cpuState,
                                     m_cpuDetail,
                                     false,
                                     content),
                    0,
                    0);
    grid->addWidget(createChartCard(QStringLiteral("Memory"),
                                     m_memoryChart,
                                     m_memoryValue,
                                     m_memoryState,
                                     m_memoryDetail,
                                     false,
                                     content),
                    0,
                    1);
    grid->addWidget(createChartCard(QStringLiteral("Network"),
                                     m_networkChart,
                                     m_networkValue,
                                     m_networkState,
                                     m_networkDetail,
                                     true,
                                     content),
                    1,
                    0);
    grid->addWidget(createChartCard(QStringLiteral("Process pressure"),
                                     m_processChart,
                                     m_processValue,
                                     m_processState,
                                     m_processDetail,
                                     false,
                                     content),
                    1,
                    1);

    auto *storageCard = new QFrame(content);
    storageCard->setObjectName(QStringLiteral("Card"));
    storageCard->setMinimumHeight(108);
    auto *storageLayout = new QVBoxLayout(storageCard);
    storageLayout->setContentsMargins(16, 13, 16, 13);
    storageLayout->setSpacing(5);
    auto *storageTop = new QHBoxLayout();
    storageTop->setSpacing(8);
    storageTop->addWidget(label(QStringLiteral("Storage"), "CardTitle", storageCard));
    storageTop->addStretch();
    m_storageValue = label(QStringLiteral("—"), "MetricValueCompact", storageCard);
    m_storageValue->setWordWrap(false);
    storageTop->addWidget(m_storageValue);
    m_storageState = label(QString(), "MetricState", storageCard);
    m_storageState->setText(QStringLiteral("WAITING"));
    m_storageState->setStyleSheet(
        QStringLiteral("color: %1;").arg(m_theme.textMuted.name()));
    storageTop->addWidget(m_storageState);
    storageLayout->addLayout(storageTop);

    m_storageDetail = label(QStringLiteral("Starting measurement providers…"),
                            "MetricDetail",
                            storageCard);
    storageLayout->addWidget(m_storageDetail);

    m_storageBar = new SegmentBar(storageCard);
    m_storageBar->setAccent(m_theme.disk);
    m_storageBar->setReadingAvailable(false);
    storageLayout->addWidget(m_storageBar);

    grid->addWidget(storageCard, 2, 0, 1, 2);
    contentLayout->addLayout(grid);

    auto *note = label(
        QStringLiteral("History keeps a gap when a provider is unavailable. "
                       "No sample is replaced by zero."),
        "CardBody",
        content);
    note->setContentsMargins(2, 2, 2, 0);
    contentLayout->addWidget(note);
    contentLayout->addStretch();

    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    refreshCharts();
}

void PerformancePage::apply(const MetricsSnapshot &snapshot)
{
    m_snapshot = snapshot;
    m_hasSnapshot = true;

    appendSample(m_cpuHistory, snapshot.cpuPercent);
    appendSample(m_memoryHistory, snapshot.memoryPercent);
    appendSample(m_downHistory, snapshot.networkDownPerSecond);
    appendSample(m_upHistory, snapshot.networkUpPerSecond);
    appendSample(m_processHistory, snapshot.processPressure);

    applyReadings();
    refreshCharts();
}

void PerformancePage::setTheme(const ThemeTokens &theme)
{
    m_theme = theme;
    if (m_storageBar) {
        m_storageBar->setAccent(theme.disk);
    }
    refreshCharts();
    if (m_hasSnapshot) {
        applyReadings();
    }
}

void PerformancePage::appendSample(QVector<std::optional<double>> &history,
                                   const Reading &reading)
{
    history.append(reading.hasValue() ? std::optional<double>(reading.value) : std::nullopt);
    while (history.size() > kHistorySamples) {
        history.removeFirst();
    }
}

void PerformancePage::refreshCharts()
{
    if (!m_cpuChart) {
        return;
    }
    m_cpuChart->setGridColor(m_theme.line);
    m_memoryChart->setGridColor(m_theme.line);
    m_networkChart->setGridColor(m_theme.line);
    m_processChart->setGridColor(m_theme.line);

    m_cpuChart->setPrimary(m_cpuHistory, m_theme.cpu);
    m_memoryChart->setPrimary(m_memoryHistory, m_theme.memory);
    m_networkChart->setPrimary(m_downHistory, m_theme.network);
    m_networkChart->setSecondary(m_upHistory, m_theme.memory);
    m_processChart->setPrimary(m_processHistory, m_theme.thermal);
}

void PerformancePage::applyReadings()
{
    applyReadingState(m_cpuState, m_snapshot.cpuPercent, m_theme, m_theme.cpu);
    m_cpuValue->setText(m_snapshot.cpuPercent.hasValue()
                            ? formatPercent(m_snapshot.cpuPercent.value)
                            : QStringLiteral("—"));
    m_cpuDetail->setText(m_snapshot.cpuPercent.hasValue()
                             ? QStringLiteral("System-wide utilization")
                             : m_snapshot.cpuPercent.detail);

    applyReadingState(m_memoryState, m_snapshot.memoryPercent, m_theme, m_theme.memory);
    m_memoryValue->setText(m_snapshot.memoryPercent.hasValue()
                               ? formatPercent(m_snapshot.memoryPercent.value)
                               : QStringLiteral("—"));
    m_memoryDetail->setText(m_snapshot.memoryPercent.detail);

    const Reading network =
        combineReadings(m_snapshot.networkDownPerSecond, m_snapshot.networkUpPerSecond);
    applyReadingState(m_networkState, network, m_theme, m_theme.network);
    m_networkValue->setText(network.hasValue()
                                ? QStringLiteral("↓ %1   ↑ %2")
                                      .arg(formatRate(m_snapshot.networkDownPerSecond.value),
                                           formatRate(m_snapshot.networkUpPerSecond.value))
                                : QStringLiteral("—"));
    m_networkDetail->setText(network.hasValue()
                                 ? QStringLiteral("Session total ↓ %1   ↑ %2")
                                       .arg(formatBytes(m_snapshot.networkDownTotalBytes),
                                            formatBytes(m_snapshot.networkUpTotalBytes))
                                 : network.detail);

    applyReadingState(m_processState,
                      m_snapshot.processPressure,
                      m_theme,
                      m_theme.thermal);
    m_processValue->setText(m_snapshot.processPressure.hasValue()
                                ? formatRatio(m_snapshot.processPressure.value)
                                : QStringLiteral("—"));
    m_processDetail->setText(m_snapshot.processPressure.detail);

    applyReadingState(m_storageState, m_snapshot.storagePercent, m_theme, m_theme.disk);
    m_storageValue->setText(m_snapshot.storagePercent.hasValue()
                                ? formatPercent(m_snapshot.storagePercent.value)
                                : QStringLiteral("—"));
    QString storageDetail = m_snapshot.storagePercent.detail;
    if (m_snapshot.storagePercent.hasValue() && !m_snapshot.storageName.isEmpty()) {
        storageDetail += QStringLiteral(" · ") + m_snapshot.storageName;
    }
    m_storageDetail->setText(storageDetail);
    m_storageBar->setReadingAvailable(m_snapshot.storagePercent.hasValue());
    m_storageBar->setValue(m_snapshot.storagePercent.hasValue()
                               ? m_snapshot.storagePercent.value / 100.0
                               : 0.0);

    const bool anyDenied = m_snapshot.cpuPercent.state == Availability::Denied
        || m_snapshot.memoryPercent.state == Availability::Denied
        || m_snapshot.storagePercent.state == Availability::Denied
        || network.state == Availability::Denied
        || m_snapshot.processPressure.state == Availability::Denied;
    const bool anyUnavailable = m_snapshot.cpuPercent.state == Availability::Unavailable
        || m_snapshot.memoryPercent.state == Availability::Unavailable
        || m_snapshot.storagePercent.state == Availability::Unavailable
        || network.state == Availability::Unavailable
        || m_snapshot.processPressure.state == Availability::Unavailable;
    const bool allAvailable = m_snapshot.cpuPercent.hasValue()
        && m_snapshot.memoryPercent.hasValue()
        && m_snapshot.storagePercent.hasValue()
        && network.hasValue()
        && m_snapshot.processPressure.hasValue();

    if (anyDenied) {
        m_pill->setText(QStringLiteral("RESTRICTED"));
        m_pill->setStyleSheet(QStringLiteral("color: %1;").arg(m_theme.danger.name()));
    } else if (allAvailable) {
        m_pill->setText(QStringLiteral("LIVE"));
        m_pill->setStyleSheet(QStringLiteral("color: %1;").arg(m_theme.cpu.name()));
    } else if (anyUnavailable) {
        m_pill->setText(QStringLiteral("DEGRADED"));
        m_pill->setStyleSheet(QStringLiteral("color: %1;").arg(m_theme.energy.name()));
    } else {
        m_pill->setText(QStringLiteral("SAMPLING"));
        m_pill->setStyleSheet(QStringLiteral("color: %1;").arg(m_theme.textMuted.name()));
    }
}

} // namespace tmog::ui
