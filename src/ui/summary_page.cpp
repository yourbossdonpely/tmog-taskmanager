#include "ui/summary_page.h"

#include "ui/destination_page.h"
#include "ui/history_chart.h"
#include "ui/icons.h"
#include "ui/reading_style.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>

namespace tmog::ui {

class VerticalGauge : public QWidget
{
public:
    explicit VerticalGauge(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_accent(QColor("#70e06a"))
    {
        setMinimumWidth(44);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setReading(const QString &label,
                    const QString &valueText,
                    double fraction,
                    bool available,
                    const QColor &accent)
    {
        m_label = label;
        m_valueText = valueText;
        m_fraction = std::clamp(fraction, 0.0, 1.0);
        m_available = available;
        m_accent = accent;
        setAccessibleName(QStringLiteral("%1 %2").arg(label, valueText));
        update();
    }

    void setAccent(const QColor &color)
    {
        m_accent = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QFont labelFont = font();
        labelFont.setPixelSize(12);
        labelFont.setWeight(QFont::DemiBold);
        painter.setFont(labelFont);
        painter.setPen(m_available ? m_accent : palette().color(QPalette::PlaceholderText));
        painter.drawText(QRectF(0, 2, width(), 18), Qt::AlignHCenter | Qt::AlignTop, m_label);

        const int top = 28;
        const int bottom = height() - 28;
        constexpr int segmentCount = 26;
        constexpr int gap = 4;
        const int segmentWidth = qBound(10, width() - 18, 22);
        const int usable = qMax(40, bottom - top);
        const int segmentHeight =
            qMax(3, (usable - gap * (segmentCount - 1)) / segmentCount);
        const int totalHeight = segmentCount * segmentHeight + gap * (segmentCount - 1);
        const int startY = top + qMax(0, (usable - totalHeight) / 2);
        const int x = (width() - segmentWidth) / 2;
        const int litCount = qRound(m_fraction * segmentCount);

        for (int index = 0; index < segmentCount; ++index) {
            const int y = startY + (segmentCount - 1 - index) * (segmentHeight + gap);
            const QRectF segment(x, y, segmentWidth, segmentHeight);
            const bool lit = m_available && index < litCount;

            if (lit) {
                QColor glow = m_accent;
                glow.setAlpha(38);
                painter.setPen(Qt::NoPen);
                painter.setBrush(glow);
                painter.drawRoundedRect(segment.adjusted(-2, -1, 2, 1), 3, 3);
            }

            QColor color = m_accent;
            color.setAlpha(lit ? 238 : (m_available ? 38 : 16));
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawRoundedRect(segment, 2, 2);
        }

        QFont valueFont = font();
        valueFont.setPixelSize(12);
        valueFont.setFamily(QStringLiteral("SF Mono"));
        valueFont.setStyleHint(QFont::Monospace);
        painter.setFont(valueFont);
        painter.setPen(m_available ? m_accent : palette().color(QPalette::PlaceholderText));
        painter.drawText(QRectF(0, height() - 22, width(), 18),
                         Qt::AlignHCenter | Qt::AlignTop,
                         m_valueText);
    }

private:
    QString m_label = QStringLiteral("—");
    QString m_valueText = QStringLiteral("—");
    QColor m_accent;
    double m_fraction = 0.0;
    bool m_available = false;
};

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

QFrame *topPanel(QWidget *parent, QVBoxLayout **layoutOut)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("Card"));
    frame->setFixedHeight(480);
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(18, 16, 18, 12);
    layout->setSpacing(6);
    if (layoutOut) {
        *layoutOut = layout;
    }
    return frame;
}

} // namespace

SummaryPage::SummaryPage(QWidget *parent)
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
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(22, 0, 16, 18);
    contentLayout->setSpacing(0);

    auto *top = new QHBoxLayout();
    top->setSpacing(12);

    auto *gaugeCard = new QFrame(content);
    gaugeCard->setObjectName(QStringLiteral("Card"));
    gaugeCard->setFixedHeight(480);
    auto *gaugeLayout = new QHBoxLayout(gaugeCard);
    gaugeLayout->setContentsMargins(8, 12, 8, 10);
    gaugeLayout->setSpacing(0);
    m_cpuGauge = new VerticalGauge(gaugeCard);
    m_memoryGauge = new VerticalGauge(gaugeCard);
    m_storageGauge = new VerticalGauge(gaugeCard);
    m_pressureGauge = new VerticalGauge(gaugeCard);
    gaugeLayout->addWidget(m_cpuGauge, 1);
    gaugeLayout->addWidget(m_memoryGauge, 1);
    gaugeLayout->addWidget(m_storageGauge, 1);
    gaugeLayout->addWidget(m_pressureGauge, 1);
    top->addWidget(gaugeCard, 264);

    QVBoxLayout *cpuLayout = nullptr;
    auto *cpuCard = topPanel(content, &cpuLayout);
    auto *cpuTop = new QHBoxLayout();
    cpuTop->setSpacing(10);
    auto *cpuTitles = new QVBoxLayout();
    cpuTitles->setSpacing(2);
    cpuTitles->addWidget(label(QStringLiteral("CPU Overview"), "CardTitle", cpuCard));
    cpuTitles->addWidget(label(QStringLiteral("System-wide utilization"), "MetricDetail", cpuCard));
    cpuTop->addLayout(cpuTitles, 1);
    m_cpuState = label(QStringLiteral("WAITING"), "MetricState", cpuCard);
    cpuTop->addWidget(m_cpuState, 0, Qt::AlignTop);
    cpuLayout->addLayout(cpuTop);

    m_cpuValue = label(QStringLiteral("—"), "MetricValue", cpuCard);
    m_cpuValue->setWordWrap(false);
    m_cpuValue->setStyleSheet(QStringLiteral("color: %1;").arg(m_theme.cpu.name()));
    cpuLayout->addWidget(m_cpuValue);

    m_cpuChart = new HistoryChart(HistoryChart::Scale::Percent, cpuCard);
    m_cpuChart->setMinimumHeight(300);
    cpuLayout->addWidget(m_cpuChart, 1);

    m_cpuDetail = label(QStringLiteral("Waiting for the first CPU delta"), "MetricDetail", cpuCard);
    cpuLayout->addWidget(m_cpuDetail);
    top->addWidget(cpuCard, 602);

    QVBoxLayout *pressureLayout = nullptr;
    auto *pressureCard = topPanel(content, &pressureLayout);
    auto *pressureTop = new QHBoxLayout();
    pressureTop->setSpacing(10);
    auto *pressureTitles = new QVBoxLayout();
    pressureTitles->setSpacing(2);
    pressureTitles->addWidget(label(QStringLiteral("Process pressure"), "CardTitle", pressureCard));
    pressureTitles->addWidget(label(QStringLiteral("Runnable work per logical CPU"),
                                    "MetricDetail",
                                    pressureCard));
    pressureTop->addLayout(pressureTitles, 1);
    m_pressureState = label(QStringLiteral("WAITING"), "MetricState", pressureCard);
    pressureTop->addWidget(m_pressureState, 0, Qt::AlignTop);
    pressureLayout->addLayout(pressureTop);

    m_pressureValue = label(QStringLiteral("—"), "MetricValue", pressureCard);
    m_pressureValue->setWordWrap(false);
    m_pressureValue->setStyleSheet(QStringLiteral("color: %1;").arg(m_theme.thermal.name()));
    pressureLayout->addWidget(m_pressureValue);

    m_pressureChart = new HistoryChart(HistoryChart::Scale::Ratio, pressureCard);
    m_pressureChart->setMinimumHeight(280);
    pressureLayout->addWidget(m_pressureChart, 1);

    m_pressureDetail = label(QStringLiteral("Waiting for process counts"), "MetricDetail", pressureCard);
    pressureLayout->addWidget(m_pressureDetail);
    top->addWidget(pressureCard, 523);

    contentLayout->addLayout(top);
    contentLayout->addSpacing(10);

    auto *memoryCard = new QFrame(content);
    memoryCard->setObjectName(QStringLiteral("Card"));
    memoryCard->setFixedHeight(260);
    auto *memoryLayout = new QVBoxLayout(memoryCard);
    memoryLayout->setContentsMargins(18, 14, 18, 10);
    memoryLayout->setSpacing(4);
    auto *memoryTop = new QHBoxLayout();
    memoryTop->setSpacing(10);
    memoryTop->addWidget(label(QStringLiteral("Memory Utilization"), "CardTitle", memoryCard), 1);
    m_memoryValue = label(QStringLiteral("—"), "SignalValue", memoryCard);
    m_memoryValue->setWordWrap(false);
    memoryTop->addWidget(m_memoryValue, 0, Qt::AlignTop);
    memoryLayout->addLayout(memoryTop);

    m_memoryChart = new HistoryChart(HistoryChart::Scale::Percent, memoryCard);
    m_memoryChart->setMinimumHeight(160);
    memoryLayout->addWidget(m_memoryChart, 1);

    m_memoryDetail = label(QStringLiteral("Waiting for memory details"), "MetricDetail", memoryCard);
    memoryLayout->addWidget(m_memoryDetail);
    contentLayout->addWidget(memoryCard);
    contentLayout->addSpacing(11);

    auto *gridTop = new QGridLayout();
    gridTop->setSpacing(11);
    gridTop->addWidget(createMini(QStringLiteral("Storage"),
                                  QStringLiteral("deep-space"),
                                  m_theme.disk,
                                  true,
                                  m_storageCard,
                                  content),
                       0,
                       0);
    gridTop->addWidget(createMini(QStringLiteral("Network"),
                                  QStringLiteral("connections"),
                                  m_theme.network,
                                  false,
                                  m_networkCard,
                                  content),
                       0,
                       1);
    gridTop->addWidget(createMini(QStringLiteral("Processes"),
                                  QStringLiteral("processes"),
                                  m_theme.gpu,
                                  true,
                                  m_processCard,
                                  content),
                       0,
                       2);
    contentLayout->addLayout(gridTop);
    contentLayout->addSpacing(12);

    auto *gridBottom = new QGridLayout();
    gridBottom->setSpacing(11);
    gridBottom->addWidget(createMini(QStringLiteral("Pressure"),
                                     QStringLiteral("benchmarks"),
                                     m_theme.thermal,
                                     true,
                                     m_pressureCard,
                                     content),
                          0,
                          0);
    gridBottom->addWidget(createMini(QStringLiteral("CPU"),
                                     QStringLiteral("summary"),
                                     m_theme.cpu,
                                     true,
                                     m_cpuCard,
                                     content),
                          0,
                          1);
    gridBottom->addWidget(createMini(QStringLiteral("Memory"),
                                     QStringLiteral("app-history"),
                                     m_theme.memory,
                                     true,
                                     m_memoryCard,
                                     content),
                          0,
                          2);
    contentLayout->addLayout(gridBottom);
    contentLayout->addStretch(1);

    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    refreshCharts();
}

QFrame *SummaryPage::createMini(const QString &title,
                                const QString &iconKey,
                                const QColor &accent,
                                bool hasBar,
                                MiniCard &card,
                                QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("Card"));
    frame->setFixedHeight(116);
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(14, 9, 14, 9);
    layout->setSpacing(3);

    auto *top = new QHBoxLayout();
    top->setSpacing(8);
    auto *icon = new LineIcon(iconKey, frame);
    icon->setIconSize(16);
    icon->setColor(accent);
    top->addWidget(icon, 0, Qt::AlignVCenter);
    auto *titleLabel = label(title, "MiniTitle", frame);
    titleLabel->setStyleSheet(QStringLiteral("color: %1;").arg(accent.name()));
    top->addWidget(titleLabel);
    top->addStretch();
    card.state = label(QStringLiteral("WAITING"), "MetricState", frame);
    top->addWidget(card.state, 0, Qt::AlignTop);
    layout->addLayout(top);

    card.value = label(QStringLiteral("—"), "MetricValueCompact", frame);
    card.value->setWordWrap(false);
    layout->addWidget(card.value);
    card.detail = label(QStringLiteral("Waiting for samples"), "MetricDetail", frame);
    layout->addWidget(card.detail);

    if (hasBar) {
        card.bar = new SegmentBar(frame);
        card.bar->setAccent(accent);
        card.bar->setReadingAvailable(false);
        layout->addWidget(card.bar);
    }

    card.frame = frame;
    card.accent = accent;
    styleMini(card);
    return frame;
}

void SummaryPage::styleMini(MiniCard &card)
{
    if (!card.frame) {
        return;
    }
    QColor tint = card.accent;
    tint.setAlphaF(m_theme.dark ? 0.06 : 0.05);
    QColor border = card.accent;
    border.setAlphaF(m_theme.dark ? 0.30 : 0.22);
    card.frame->setStyleSheet(
        QStringLiteral("QFrame#Card { background: rgba(%1,%2,%3,%4); border: 1px solid "
                       "rgba(%5,%6,%7,%8); border-radius: 8px; }")
            .arg(tint.red())
            .arg(tint.green())
            .arg(tint.blue())
            .arg(tint.alphaF(), 0, 'f', 3)
            .arg(border.red())
            .arg(border.green())
            .arg(border.blue())
            .arg(border.alphaF(), 0, 'f', 3));
    if (card.bar) {
        card.bar->setAccent(card.accent);
    }
}

void SummaryPage::apply(const MetricsSnapshot &snapshot)
{
    m_snapshot = snapshot;
    m_hasSnapshot = true;
    appendLimited(m_cpuHistory, snapshot.cpuPercent);
    appendLimited(m_memoryHistory, snapshot.memoryPercent);
    appendLimited(m_pressureHistory, snapshot.processPressure);
    applyReadings();
}

void SummaryPage::setTheme(const ThemeTokens &theme)
{
    m_theme = theme;
    m_cpuCard.accent = theme.cpu;
    m_memoryCard.accent = theme.memory;
    m_storageCard.accent = theme.disk;
    m_networkCard.accent = theme.network;
    m_processCard.accent = theme.gpu;
    m_pressureCard.accent = theme.thermal;
    for (MiniCard *card : {&m_cpuCard,
                           &m_memoryCard,
                           &m_storageCard,
                           &m_networkCard,
                           &m_processCard,
                           &m_pressureCard}) {
        styleMini(*card);
    }
    m_cpuValue->setStyleSheet(QStringLiteral("color: %1;").arg(theme.cpu.name()));
    m_pressureValue->setStyleSheet(QStringLiteral("color: %1;").arg(theme.thermal.name()));
    if (m_cpuGauge) {
        m_cpuGauge->setAccent(theme.cpu);
        m_memoryGauge->setAccent(theme.memory);
        m_storageGauge->setAccent(theme.disk);
        m_pressureGauge->setAccent(theme.thermal);
    }
    refreshCharts();
    if (m_hasSnapshot) {
        applyReadings();
    }
}

void SummaryPage::refreshCharts()
{
    if (!m_cpuChart) {
        return;
    }
    m_cpuChart->setGridColor(m_theme.line);
    m_memoryChart->setGridColor(m_theme.line);
    m_pressureChart->setGridColor(m_theme.line);
    m_cpuChart->setPrimary(m_cpuHistory, m_theme.cpu);
    m_memoryChart->setPrimary(m_memoryHistory, m_theme.memory);
    m_pressureChart->setPrimary(m_pressureHistory, m_theme.thermal);
}

void SummaryPage::applyMini(MiniCard &card,
                            const Reading &reading,
                            const QString &value,
                            const QString &detail,
                            double fraction)
{
    applyReadingState(card.state, reading, m_theme, card.accent);
    card.value->setText(reading.hasValue() ? value : QStringLiteral("—"));
    card.detail->setText(reading.hasValue() ? detail : reading.detail);
    if (card.bar) {
        card.bar->setReadingAvailable(reading.hasValue());
        card.bar->setValue(reading.hasValue() ? fraction : 0.0);
    }
}

void SummaryPage::applyReadings()
{
    const MetricsSnapshot &snapshot = m_snapshot;

    applyReadingState(m_cpuState, snapshot.cpuPercent, m_theme, m_theme.cpu);
    m_cpuValue->setText(snapshot.cpuPercent.hasValue()
                            ? formatPercent(snapshot.cpuPercent.value)
                            : QStringLiteral("—"));
    m_cpuValue->setStyleSheet(QStringLiteral("color: %1;")
                                  .arg(snapshot.cpuPercent.hasValue() ? m_theme.cpu.name()
                                                                      : m_theme.textMuted.name()));
    m_cpuDetail->setText(snapshot.cpuPercent.hasValue()
                             ? QStringLiteral("%1 logical processors")
                                   .arg(snapshot.logicalCpus > 0 ? snapshot.logicalCpus : 0)
                             : snapshot.cpuPercent.detail);

    applyReadingState(m_pressureState, snapshot.processPressure, m_theme, m_theme.thermal);
    m_pressureValue->setText(snapshot.processPressure.hasValue()
                                 ? formatRatio(snapshot.processPressure.value)
                                 : QStringLiteral("—"));
    m_pressureValue->setStyleSheet(
        QStringLiteral("color: %1;")
            .arg(snapshot.processPressure.hasValue() ? m_theme.thermal.name()
                                                     : m_theme.textMuted.name()));
    m_pressureDetail->setText(snapshot.processPressure.detail);

    m_memoryValue->setText(snapshot.memoryPercent.hasValue()
                               ? formatPercent(snapshot.memoryPercent.value)
                               : QStringLiteral("—"));
    m_memoryDetail->setText(snapshot.memoryPercent.detail);

    const bool cpuAvailable = snapshot.cpuPercent.hasValue();
    const bool memoryAvailable = snapshot.memoryPercent.hasValue();
    const bool storageAvailable = snapshot.storagePercent.hasValue();
    const bool pressureAvailable = snapshot.processPressure.hasValue();
    m_cpuGauge->setReading(QStringLiteral("CPU"),
                           cpuAvailable ? formatPercent(snapshot.cpuPercent.value)
                                        : QStringLiteral("—"),
                           cpuAvailable ? snapshot.cpuPercent.value / 100.0 : 0.0,
                           cpuAvailable,
                           m_theme.cpu);
    m_memoryGauge->setReading(QStringLiteral("Memory"),
                              memoryAvailable ? formatPercent(snapshot.memoryPercent.value)
                                              : QStringLiteral("—"),
                              memoryAvailable ? snapshot.memoryPercent.value / 100.0 : 0.0,
                              memoryAvailable,
                              m_theme.memory);
    m_storageGauge->setReading(QStringLiteral("Storage"),
                               storageAvailable ? formatPercent(snapshot.storagePercent.value)
                                                : QStringLiteral("—"),
                               storageAvailable ? snapshot.storagePercent.value / 100.0 : 0.0,
                               storageAvailable,
                               m_theme.disk);
    m_pressureGauge->setReading(QStringLiteral("Pressure"),
                                pressureAvailable ? formatRatio(snapshot.processPressure.value)
                                                  : QStringLiteral("—"),
                                pressureAvailable ? snapshot.processPressure.value : 0.0,
                                pressureAvailable,
                                m_theme.thermal);

    const Reading network =
        combineReadings(snapshot.networkDownPerSecond, snapshot.networkUpPerSecond);
    const QString networkValue = network.hasValue()
        ? QStringLiteral("↓ %1  ↑ %2")
              .arg(formatRate(snapshot.networkDownPerSecond.value),
                   formatRate(snapshot.networkUpPerSecond.value))
        : QStringLiteral("—");
    const QString networkDetail = network.hasValue()
        ? QStringLiteral("Session ↓ %1  ↑ %2")
              .arg(formatBytes(snapshot.networkDownTotalBytes),
                   formatBytes(snapshot.networkUpTotalBytes))
        : network.detail;

    QString storageDetail;
    if (snapshot.storagePercent.hasValue()) {
        storageDetail = snapshot.storagePercent.detail;
        if (!snapshot.storageName.isEmpty()) {
            storageDetail += QStringLiteral(" · ") + snapshot.storageName;
        }
    } else {
        storageDetail = snapshot.storagePercent.detail;
    }

    applyMini(m_storageCard,
              snapshot.storagePercent,
              formatPercent(snapshot.storagePercent.value),
              storageDetail,
              snapshot.storagePercent.value / 100.0);
    applyMini(m_networkCard, network, networkValue, networkDetail, 0.0);
    applyMini(m_processCard,
              snapshot.processPressure,
              QString::number(snapshot.processCount),
              snapshot.processPressure.hasValue()
                  ? QStringLiteral("%1 runnable / %2 CPUs")
                        .arg(snapshot.runnableCount)
                        .arg(snapshot.logicalCpus)
                  : snapshot.processPressure.detail,
              snapshot.processPressure.value);
    applyMini(m_pressureCard,
              snapshot.processPressure,
              formatRatio(snapshot.processPressure.value),
              snapshot.processPressure.detail,
              snapshot.processPressure.value);
    applyMini(m_cpuCard,
              snapshot.cpuPercent,
              formatPercent(snapshot.cpuPercent.value),
              snapshot.cpuPercent.hasValue() ? QStringLiteral("System-wide utilization")
                                             : snapshot.cpuPercent.detail,
              snapshot.cpuPercent.value / 100.0);
    applyMini(m_memoryCard,
              snapshot.memoryPercent,
              formatPercent(snapshot.memoryPercent.value),
              snapshot.memoryPercent.detail,
              snapshot.memoryPercent.value / 100.0);

    refreshCharts();
}

} // namespace tmog::ui
