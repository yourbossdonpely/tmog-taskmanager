#include "ui/performance_page.h"

#include "ui/history_chart.h"
#include "ui/icons.h"
#include "ui/reading_style.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>

namespace tmog::ui {

namespace {

QLabel *label(const QString &text, const char *objectName, QWidget *parent)
{
    auto *item = new QLabel(text, parent);
    item->setObjectName(QString::fromLatin1(objectName));
    item->setWordWrap(true);
    return item;
}

void appendLimited(QVector<std::optional<double>> &history, const Reading &reading)
{
    history.append(reading.hasValue() ? std::optional<double>(reading.value) : std::nullopt);
    while (history.size() > 60) {
        history.removeFirst();
    }
}

int presentSamples(const QVector<std::optional<double>> &history)
{
    return static_cast<int>(std::count_if(history.cbegin(), history.cend(), [](const auto &value) {
        return value.has_value();
    }));
}

double averageOf(const QVector<std::optional<double>> &history)
{
    double total = 0.0;
    int count = 0;
    for (const auto &value : history) {
        if (value) {
            total += *value;
            ++count;
        }
    }
    return count > 0 ? total / count : 0.0;
}

double peakOf(const QVector<std::optional<double>> &history)
{
    double peak = 0.0;
    for (const auto &value : history) {
        if (value) {
            peak = std::max(peak, *value);
        }
    }
    return peak;
}

QString formatForScale(double value, HistoryChart::Scale scale)
{
    if (scale == HistoryChart::Scale::Percent) {
        return formatPercent(value);
    }
    if (scale == HistoryChart::Scale::Ratio) {
        return formatRatio(value);
    }
    return formatRate(value);
}

} // namespace

PerformancePage::PerformancePage(QWidget *parent)
    : QWidget(parent)
    , m_theme(themeById(QStringLiteral("dark")))
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setObjectName(QStringLiteral("ScrollViewport"));
    scroll->viewport()->setAutoFillBackground(false);

    auto *content = new QWidget(scroll);
    content->setMinimumHeight(900);
    auto *layout = new QHBoxLayout(content);
    layout->setContentsMargins(22, 0, 16, 18);
    layout->setSpacing(12);

    m_rail = new QFrame(content);
    m_rail->setObjectName(QStringLiteral("Card"));
    m_rail->setFixedWidth(416);
    buildRail(m_rail);
    layout->addWidget(m_rail, 0);

    m_detailCard = new QFrame(content);
    m_detailCard->setObjectName(QStringLiteral("Card"));
    buildDetail(m_detailCard);
    layout->addWidget(m_detailCard, 1);

    scroll->setWidget(content);
    outer->addWidget(scroll, 1);
    selectSubsystem(Subsystem::Cpu);
}

void PerformancePage::buildRail(QWidget *parent)
{
    auto *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(16, 18, 16, 16);
    layout->setSpacing(0);
    layout->addWidget(label(QStringLiteral("Subsystems"), "CardTitle", parent));
    layout->addWidget(label(QStringLiteral("Choose a history to inspect."), "MetricDetail", parent));
    layout->addSpacing(16);

    const QString titles[5] = {
        QStringLiteral("CPU"),
        QStringLiteral("Memory"),
        QStringLiteral("Network"),
        QStringLiteral("Storage"),
        QStringLiteral("Process pressure"),
    };
    const QString icons[5] = {
        QStringLiteral("summary"),
        QStringLiteral("app-history"),
        QStringLiteral("connections"),
        QStringLiteral("deep-space"),
        QStringLiteral("benchmarks"),
    };
    const QColor accents[5] = {
        m_theme.cpu,
        m_theme.memory,
        m_theme.network,
        m_theme.disk,
        m_theme.thermal,
    };

    for (int index = 0; index < 5; ++index) {
        auto *row = new QFrame(parent);
        row->setObjectName(QStringLiteral("SubsystemRow"));
        row->setFixedHeight(90);
        row->setCursor(Qt::PointingHandCursor);
        row->setProperty("rowIndex", index);
        row->installEventFilter(this);

        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(14, 11, 12, 11);
        rowLayout->setSpacing(12);

        auto *icon = new LineIcon(icons[index], row);
        icon->setIconSize(20);
        icon->setColor(accents[index]);
        rowLayout->addWidget(icon, 0, Qt::AlignTop);

        auto *text = new QVBoxLayout();
        text->setSpacing(2);
        text->addWidget(label(titles[index], "SignalLabel", row));
        m_rows[index].value = label(QStringLiteral("—"), "SignalValue", row);
        m_rows[index].value->setWordWrap(false);
        text->addWidget(m_rows[index].value);
        m_rows[index].detail = label(QStringLiteral("Waiting for samples"), "MetricDetail", row);
        text->addWidget(m_rows[index].detail);
        rowLayout->addLayout(text, 1);

        m_rows[index].frame = row;
        m_rows[index].accent = accents[index];
        styleRow(static_cast<Subsystem>(index));
        layout->addWidget(row);
        if (index < 4) {
            layout->addSpacing(7);
        }
    }

    layout->addSpacing(16);

    auto *snapshot = new QFrame(parent);
    snapshot->setObjectName(QStringLiteral("SnapshotPanel"));
    auto *snapshotLayout = new QVBoxLayout(snapshot);
    snapshotLayout->setContentsMargins(14, 12, 14, 12);
    snapshotLayout->setSpacing(7);
    snapshotLayout->addWidget(label(QStringLiteral("Live snapshot"), "CardTitle", snapshot));
    const auto addSnapshotRow = [&](const QString &name, QLabel *&value) {
        auto *row = new QHBoxLayout();
        row->setSpacing(8);
        row->addWidget(label(name, "MetricDetail", snapshot), 1);
        value = label(QStringLiteral("—"), "SignalValue", snapshot);
        value->setWordWrap(false);
        row->addWidget(value, 0);
        snapshotLayout->addLayout(row);
    };
    addSnapshotRow(QStringLiteral("Processes"), m_railProcesses);
    addSnapshotRow(QStringLiteral("Logical CPUs"), m_railCpus);
    addSnapshotRow(QStringLiteral("Samples in view"), m_railSamples);
    layout->addWidget(snapshot);

    layout->addStretch(1);
    auto *note = label(QStringLiteral("History keeps a gap when a reading is unavailable."),
                       "MetricDetail",
                       parent);
    note->setWordWrap(true);
    layout->addWidget(note);
}

void PerformancePage::buildDetail(QWidget *parent)
{
    auto *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(26, 22, 26, 20);
    layout->setSpacing(4);

    auto *top = new QHBoxLayout();
    top->setSpacing(12);
    auto *titleBlock = new QVBoxLayout();
    titleBlock->setSpacing(2);
    m_title = label(QStringLiteral("CPU"), "DetailTitle", parent);
    titleBlock->addWidget(m_title);
    m_detail = label(QStringLiteral("System-wide utilization"), "MetricDetail", parent);
    titleBlock->addWidget(m_detail);
    top->addLayout(titleBlock, 1);
    m_state = label(QStringLiteral("WAITING"), "MetricState", parent);
    top->addWidget(m_state, 0, Qt::AlignTop);
    layout->addLayout(top);

    m_value = label(QStringLiteral("—"), "MetricValue", parent);
    m_value->setWordWrap(false);
    layout->addWidget(m_value);
    layout->addSpacing(8);

    m_chart = new HistoryChart(HistoryChart::Scale::Percent, parent);
    m_chart->setMinimumHeight(270);
    layout->addWidget(m_chart, 1);

    auto *legend = new QHBoxLayout();
    legend->setSpacing(18);
    m_legendPrimary = label(QStringLiteral("● Receive"), "ChartLegend", parent);
    m_legendSecondary = label(QStringLiteral("● Transmit"), "ChartLegend", parent);
    legend->addWidget(m_legendPrimary);
    legend->addWidget(m_legendSecondary);
    legend->addStretch();
    layout->addLayout(legend);
    layout->addSpacing(12);

    auto *stats = new QHBoxLayout();
    stats->setSpacing(24);
    const QString statTitles[4] = {
        QStringLiteral("Current"),
        QStringLiteral("Average"),
        QStringLiteral("Peak"),
        QStringLiteral("Samples"),
    };
    QLabel **statValues[4] = {
        &m_statCurrent,
        &m_statAverage,
        &m_statPeak,
        &m_statSamples,
    };
    for (int index = 0; index < 4; ++index) {
        auto *column = new QVBoxLayout();
        column->setSpacing(2);
        column->addWidget(label(statTitles[index], "MetricDetail", parent));
        *statValues[index] = label(QStringLiteral("—"), "SignalValue", parent);
        (*statValues[index])->setWordWrap(false);
        column->addWidget(*statValues[index]);
        stats->addLayout(column, 1);
    }
    layout->addLayout(stats);

    m_source = label(QStringLiteral("Waiting for the first sample."), "MetricDetail", parent);
    m_source->setWordWrap(true);
    layout->addWidget(m_source);
}

void PerformancePage::styleRow(Subsystem subsystem)
{
    const int index = static_cast<int>(subsystem);
    Row &row = m_rows[index];
    if (!row.frame) {
        return;
    }
    const bool selected = subsystem == m_selected;
    QColor tint = row.accent;
    tint.setAlphaF(selected ? (m_theme.dark ? 0.15 : 0.11) : 0.0);
    QColor border = row.accent;
    border.setAlphaF(selected ? (m_theme.dark ? 0.42 : 0.34) : 0.0);
    row.frame->setStyleSheet(
        QStringLiteral("QFrame#SubsystemRow { background: rgba(%1,%2,%3,%4); "
                       "border: 1px solid rgba(%5,%6,%7,%8); border-left: 3px solid %9; "
                       "border-radius: 7px; } "
                       "QFrame#SubsystemRow:hover { background: rgba(%1,%2,%3,%10); }")
            .arg(tint.red())
            .arg(tint.green())
            .arg(tint.blue())
            .arg(tint.alphaF(), 0, 'f', 3)
            .arg(border.red())
            .arg(border.green())
            .arg(border.blue())
            .arg(border.alphaF(), 0, 'f', 3)
            .arg(selected ? row.accent.name() : m_theme.line.name())
            .arg(m_theme.dark ? 0.08 : 0.06, 0, 'f', 3));
    row.frame->style()->unpolish(row.frame);
    row.frame->style()->polish(row.frame);
}

void PerformancePage::selectSubsystem(Subsystem subsystem)
{
    m_selected = subsystem;
    for (int index = 0; index < 5; ++index) {
        styleRow(static_cast<Subsystem>(index));
    }
    refreshDetail();
}

bool PerformancePage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton) {
            for (int index = 0; index < 5; ++index) {
                if (watched == m_rows[index].frame) {
                    selectSubsystem(static_cast<Subsystem>(index));
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void PerformancePage::apply(const MetricsSnapshot &snapshot)
{
    m_snapshot = snapshot;
    m_hasSnapshot = true;
    appendLimited(m_cpuHistory, snapshot.cpuPercent);
    appendLimited(m_memoryHistory, snapshot.memoryPercent);
    appendLimited(m_downHistory, snapshot.networkDownPerSecond);
    appendLimited(m_upHistory, snapshot.networkUpPerSecond);
    appendLimited(m_storageHistory, snapshot.storagePercent);
    appendLimited(m_pressureHistory, snapshot.processPressure);
    refreshRows();
    refreshDetail();
}

void PerformancePage::setTheme(const ThemeTokens &theme)
{
    m_theme = theme;
    const QColor accents[5] = {
        theme.cpu,
        theme.memory,
        theme.network,
        theme.disk,
        theme.thermal,
    };
    for (int index = 0; index < 5; ++index) {
        m_rows[index].accent = accents[index];
        styleRow(static_cast<Subsystem>(index));
    }
    if (m_hasSnapshot) {
        refreshRows();
        refreshDetail();
    }
}

void PerformancePage::refreshRows()
{
    const MetricsSnapshot &snapshot = m_snapshot;
    const auto setRow = [this](Subsystem subsystem,
                               const QString &value,
                               const QString &detail,
                               bool available) {
        Row &row = m_rows[static_cast<int>(subsystem)];
        row.value->setText(value);
        row.detail->setText(detail);
        row.value->setStyleSheet(
            QStringLiteral("color: %1;").arg(available ? row.accent.name()
                                                       : m_theme.textMuted.name()));
    };

    setRow(Subsystem::Cpu,
           snapshot.cpuPercent.hasValue() ? formatPercent(snapshot.cpuPercent.value)
                                          : QStringLiteral("—"),
           snapshot.cpuPercent.hasValue()
               ? QStringLiteral("%1 logical processors").arg(snapshot.logicalCpus)
               : snapshot.cpuPercent.detail,
           snapshot.cpuPercent.hasValue());
    setRow(Subsystem::Memory,
           snapshot.memoryPercent.hasValue() ? formatPercent(snapshot.memoryPercent.value)
                                             : QStringLiteral("—"),
           snapshot.memoryPercent.hasValue() ? snapshot.memoryPercent.detail
                                             : snapshot.memoryPercent.detail,
           snapshot.memoryPercent.hasValue());

    const Reading network =
        combineReadings(snapshot.networkDownPerSecond, snapshot.networkUpPerSecond);
    setRow(Subsystem::Network,
           network.hasValue()
               ? QStringLiteral("↓ %1  ↑ %2")
                     .arg(formatRate(snapshot.networkDownPerSecond.value),
                          formatRate(snapshot.networkUpPerSecond.value))
               : QStringLiteral("—"),
           network.hasValue()
               ? QStringLiteral("Non-loopback traffic")
               : network.detail,
           network.hasValue());
    setRow(Subsystem::Storage,
           snapshot.storagePercent.hasValue() ? formatPercent(snapshot.storagePercent.value)
                                              : QStringLiteral("—"),
           snapshot.storagePercent.hasValue() ? snapshot.storagePercent.detail
                                              : snapshot.storagePercent.detail,
           snapshot.storagePercent.hasValue());
    setRow(Subsystem::Pressure,
           snapshot.processPressure.hasValue() ? formatRatio(snapshot.processPressure.value)
                                               : QStringLiteral("—"),
           snapshot.processPressure.hasValue() ? snapshot.processPressure.detail
                                               : snapshot.processPressure.detail,
           snapshot.processPressure.hasValue());

    if (m_railProcesses) {
        m_railProcesses->setText(snapshot.processCount > 0
                                     ? QString::number(snapshot.processCount)
                                     : QStringLiteral("—"));
    }
    if (m_railCpus) {
        m_railCpus->setText(snapshot.logicalCpus > 0
                                ? QString::number(snapshot.logicalCpus)
                                : QStringLiteral("—"));
    }
    if (m_railSamples) {
        m_railSamples->setText(QString::number(presentSamples(m_cpuHistory)));
    }
}

void PerformancePage::refreshDetail()
{
    if (!m_chart) {
        return;
    }

    m_chart->setGridColor(m_theme.line);
    const MetricsSnapshot &snapshot = m_snapshot;
    const Reading network =
        combineReadings(snapshot.networkDownPerSecond, snapshot.networkUpPerSecond);

    Reading reading;
    QColor accent = m_theme.cpu;
    HistoryChart::Scale scale = HistoryChart::Scale::Percent;
    const QVector<std::optional<double>> *primary = &m_cpuHistory;
    const QVector<std::optional<double>> *secondary = nullptr;
    QString title;
    QString value = QStringLiteral("—");
    QString detail;
    QString source;

    switch (m_selected) {
    case Subsystem::Cpu:
        reading = snapshot.cpuPercent;
        accent = m_theme.cpu;
        scale = HistoryChart::Scale::Percent;
        primary = &m_cpuHistory;
        title = QStringLiteral("CPU");
        value = reading.hasValue() ? formatPercent(reading.value) : QStringLiteral("—");
        detail = QStringLiteral("System-wide utilization");
        source = reading.hasValue()
            ? QStringLiteral("%1 logical processors")
                  .arg(snapshot.logicalCpus > 0 ? snapshot.logicalCpus : 0)
            : reading.detail;
        break;
    case Subsystem::Memory:
        reading = snapshot.memoryPercent;
        accent = m_theme.memory;
        scale = HistoryChart::Scale::Percent;
        primary = &m_memoryHistory;
        title = QStringLiteral("Memory");
        value = reading.hasValue() ? formatPercent(reading.value) : QStringLiteral("—");
        detail = QStringLiteral("Physical memory in use");
        source = reading.detail;
        break;
    case Subsystem::Network:
        reading = network;
        accent = m_theme.network;
        scale = HistoryChart::Scale::Rate;
        primary = &m_downHistory;
        secondary = &m_upHistory;
        title = QStringLiteral("Network");
        value = reading.hasValue()
            ? QStringLiteral("↓ %1   ↑ %2")
                  .arg(formatRate(snapshot.networkDownPerSecond.value),
                       formatRate(snapshot.networkUpPerSecond.value))
            : QStringLiteral("—");
        detail = QStringLiteral("Receive and transmit, non-loopback interfaces");
        source = reading.hasValue()
            ? QStringLiteral("Session total ↓ %1   ↑ %2")
                  .arg(formatBytes(snapshot.networkDownTotalBytes),
                       formatBytes(snapshot.networkUpTotalBytes))
            : reading.detail;
        break;
    case Subsystem::Storage:
        reading = snapshot.storagePercent;
        accent = m_theme.disk;
        scale = HistoryChart::Scale::Percent;
        primary = &m_storageHistory;
        title = QStringLiteral("Storage");
        value = reading.hasValue() ? formatPercent(reading.value) : QStringLiteral("—");
        detail = QStringLiteral("System volume capacity");
        source = reading.detail;
        break;
    case Subsystem::Pressure:
        reading = snapshot.processPressure;
        accent = m_theme.thermal;
        scale = HistoryChart::Scale::Ratio;
        primary = &m_pressureHistory;
        title = QStringLiteral("Process pressure");
        value = reading.hasValue() ? formatRatio(reading.value) : QStringLiteral("—");
        detail = QStringLiteral("Runnable or queued work per logical CPU");
        source = reading.detail;
        break;
    }

    m_title->setText(title);
    m_detail->setText(detail);
    m_value->setText(value);
    m_value->setStyleSheet(
        QStringLiteral("color: %1;").arg(reading.hasValue() ? accent.name()
                                                             : m_theme.textMuted.name()));
    m_source->setText(source);
    applyReadingState(m_state, reading, m_theme, accent);

    m_chart->setPrimary(*primary, accent);
    if (secondary) {
        m_chart->setSecondary(*secondary, m_theme.memory);
    } else {
        m_chart->setSecondary(QVector<std::optional<double>>(), m_theme.memory);
    }

    const int samples = presentSamples(*primary);
    m_statCurrent->setText(value);
    m_statCurrent->setStyleSheet(
        QStringLiteral("color: %1;").arg(reading.hasValue() ? accent.name()
                                                             : m_theme.textMuted.name()));
    if (m_selected == Subsystem::Network) {
        m_statAverage->setText(QStringLiteral("↓ %1").arg(formatRate(averageOf(m_downHistory))));
        m_statPeak->setText(QStringLiteral("↓ %1").arg(formatRate(peakOf(m_downHistory))));
    } else {
        m_statAverage->setText(formatForScale(averageOf(*primary), scale));
        m_statPeak->setText(formatForScale(peakOf(*primary), scale));
    }
    m_statSamples->setText(QString::number(samples));

    if (m_legendPrimary && m_legendSecondary) {
        const bool showLegend = m_selected == Subsystem::Network;
        m_legendPrimary->setVisible(showLegend);
        m_legendSecondary->setVisible(showLegend);
        m_legendPrimary->setStyleSheet(
            QStringLiteral("color: %1;").arg(m_theme.network.name()));
        m_legendSecondary->setStyleSheet(
            QStringLiteral("color: %1;").arg(m_theme.memory.name()));
    }
}

} // namespace tmog::ui
