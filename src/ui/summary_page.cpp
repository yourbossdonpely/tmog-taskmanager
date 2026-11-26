#include "ui/summary_page.h"

#include "ui/destination_page.h"
#include "ui/reading_style.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace tmog::ui {

namespace {

QLabel *label(const QString &text, const char *objectName, QWidget *parent)
{
    auto *item = new QLabel(text, parent);
    item->setObjectName(QString::fromLatin1(objectName));
    item->setWordWrap(true);
    return item;
}

} // namespace

QFrame *SummaryPage::createCard(const QString &title,
                                const QColor &accent,
                                bool hasBar,
                                bool compactValue,
                                Card &card,
                                QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("Card"));
    frame->setMinimumHeight(150);
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 13, 16, 13);
    layout->setSpacing(5);

    auto *top = new QHBoxLayout();
    top->setSpacing(8);
    top->addWidget(label(title, "SignalLabel", frame));
    top->addStretch();
    card.state = label(QString(), "MetricState", frame);
    top->addWidget(card.state);
    layout->addLayout(top);

    card.value = label(QStringLiteral("—"), compactValue ? "MetricValueCompact" : "MetricValue", frame);
    card.value->setWordWrap(false);
    layout->addWidget(card.value);

    card.detail = label(QStringLiteral("Starting measurement providers…"), "MetricDetail", frame);
    layout->addWidget(card.detail);

    if (hasBar) {
        card.bar = new SegmentBar(frame);
        card.bar->setAccent(accent);
        card.bar->setReadingAvailable(false);
        layout->addWidget(card.bar);
    }

    card.accent = accent;
    card.hasBar = hasBar;
    return frame;
}

SummaryPage::SummaryPage(QWidget *parent)
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
    titles->addWidget(label(QStringLiteral("Summary"), "PageTitle", header));
    titles->addWidget(label(QStringLiteral("One glance at the whole machine."), "PageSubtitle", header));
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
    contentLayout->setSpacing(14);

    auto *grid = new QGridLayout();
    grid->setSpacing(12);
    grid->addWidget(createCard(QStringLiteral("CPU"),
                               m_theme.cpu,
                               true,
                               false,
                               m_cpuCard,
                               content),
                    0,
                    0);
    grid->addWidget(createCard(QStringLiteral("Memory"),
                               m_theme.memory,
                               true,
                               false,
                               m_memoryCard,
                               content),
                    0,
                    1);
    grid->addWidget(createCard(QStringLiteral("Storage"),
                               m_theme.disk,
                               true,
                               false,
                               m_storageCard,
                               content),
                    1,
                    0);
    grid->addWidget(createCard(QStringLiteral("Network"),
                               m_theme.network,
                               false,
                               true,
                               m_networkCard,
                               content),
                    1,
                    1);
    grid->addWidget(createCard(QStringLiteral("Process pressure"),
                               m_theme.thermal,
                               false,
                               false,
                               m_processCard,
                               content),
                    2,
                    0,
                    1,
                    2);
    contentLayout->addLayout(grid);

    auto *note = new QFrame(content);
    note->setObjectName(QStringLiteral("Card"));
    auto *noteLayout = new QVBoxLayout(note);
    noteLayout->setContentsMargins(18, 15, 18, 15);
    noteLayout->setSpacing(6);
    noteLayout->addWidget(label(QStringLiteral("Measured, never invented."), "CardTitle", note));
    noteLayout->addWidget(label(
        QStringLiteral("A provider that cannot read a subsystem reports UNAVAILABLE with the reason. "
                       "The first sample is marked SAMPLING, and history keeps a gap instead of a "
                       "fabricated zero."),
        "CardBody",
        note));
    contentLayout->addWidget(note);
    contentLayout->addStretch();

    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    for (Card *card :
         {&m_cpuCard, &m_memoryCard, &m_storageCard, &m_networkCard, &m_processCard}) {
        card->state->setText(QStringLiteral("WAITING"));
        card->state->setStyleSheet(
            QStringLiteral("color: %1;").arg(m_theme.textMuted.name()));
    }
}

void SummaryPage::apply(const MetricsSnapshot &snapshot)
{
    m_snapshot = snapshot;
    m_hasSnapshot = true;
    applyCards();
}

void SummaryPage::setTheme(const ThemeTokens &theme)
{
    m_theme = theme;
    m_cpuCard.accent = theme.cpu;
    m_memoryCard.accent = theme.memory;
    m_storageCard.accent = theme.disk;
    m_networkCard.accent = theme.network;
    m_processCard.accent = theme.thermal;
    for (Card *card :
         {&m_cpuCard, &m_memoryCard, &m_storageCard, &m_networkCard, &m_processCard}) {
        if (card->bar) {
            card->bar->setAccent(card->accent);
        }
    }
    if (m_hasSnapshot) {
        applyCards();
    }
}

void SummaryPage::applyCards()
{
    const auto applyMetric = [this](Card &card,
                                    const Reading &reading,
                                    const QString &value,
                                    const QString &detail) {
        applyReadingState(card.state, reading, m_theme, card.accent);
        card.value->setText(reading.hasValue() ? value : QStringLiteral("—"));
        card.detail->setText(reading.hasValue() ? detail : reading.detail);
        if (card.bar) {
            card.bar->setReadingAvailable(reading.hasValue());
            card.bar->setValue(reading.hasValue() ? reading.value / 100.0 : 0.0);
        }
    };

    applyMetric(m_cpuCard,
                m_snapshot.cpuPercent,
                formatPercent(m_snapshot.cpuPercent.value),
                QStringLiteral("System-wide utilization"));

    applyMetric(m_memoryCard,
                m_snapshot.memoryPercent,
                formatPercent(m_snapshot.memoryPercent.value),
                m_snapshot.memoryPercent.detail);

    QString storageDetail = m_snapshot.storagePercent.detail;
    if (m_snapshot.storagePercent.hasValue() && !m_snapshot.storageName.isEmpty()) {
        storageDetail += QStringLiteral(" · ") + m_snapshot.storageName;
    }
    applyMetric(m_storageCard,
                m_snapshot.storagePercent,
                formatPercent(m_snapshot.storagePercent.value),
                storageDetail);

    const Reading network =
        combineReadings(m_snapshot.networkDownPerSecond, m_snapshot.networkUpPerSecond);
    const QString networkValue = network.hasValue()
        ? QStringLiteral("↓ %1   ↑ %2")
              .arg(formatRate(m_snapshot.networkDownPerSecond.value),
                   formatRate(m_snapshot.networkUpPerSecond.value))
        : QStringLiteral("—");
    const QString networkDetail = network.hasValue()
        ? QStringLiteral("Session total ↓ %1   ↑ %2")
              .arg(formatBytes(m_snapshot.networkDownTotalBytes),
                   formatBytes(m_snapshot.networkUpTotalBytes))
        : network.detail;
    applyMetric(m_networkCard, network, networkValue, networkDetail);

    applyMetric(m_processCard,
                m_snapshot.processPressure,
                formatRatio(m_snapshot.processPressure.value),
                m_snapshot.processPressure.detail);

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
