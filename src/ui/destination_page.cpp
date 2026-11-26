#include "ui/destination_page.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QVBoxLayout>

namespace tmog::ui {

SegmentBar::SegmentBar(QWidget *parent)
    : QWidget(parent)
    , m_accent(QColor("#5b8cff"))
{
    setMinimumHeight(18);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void SegmentBar::setAccent(const QColor &color)
{
    m_accent = color;
    update();
}

void SegmentBar::setValue(double value)
{
    m_value = qBound(0.0, value, 1.0);
    update();
}

QSize SegmentBar::sizeHint() const
{
    return {220, 20};
}

void SegmentBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    constexpr int segments = 26;
    constexpr int gap = 3;
    const int width = qMax(1, (this->width() - gap * (segments - 1)) / segments);
    const int height = qMin(14, this->height() - 4);
    const int litCount = qRound(m_value * segments);

    for (int index = 0; index < segments; ++index) {
        const int x = index * (width + gap);
        const QRectF segment(x, (this->height() - height) / 2.0, width, height);
        QColor color = m_accent;
        color.setAlpha(index < litCount ? 235 : 38);
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawRoundedRect(segment, 2, 2);
    }
}

namespace {

QLabel *label(const QString &text, const char *objectName, QWidget *parent)
{
    auto *item = new QLabel(text, parent);
    item->setObjectName(QString::fromLatin1(objectName));
    item->setWordWrap(true);
    return item;
}

QFrame *signalCard(const QString &title, const QString &value, const QColor &accent, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("Card"));
    card->setFixedHeight(112);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(8);

    auto *top = new QHBoxLayout();
    auto *name = label(title, "SignalLabel", card);
    auto *state = label(QStringLiteral("PENDING"), "CardAccent", card);
    top->addWidget(name);
    top->addStretch();
    top->addWidget(state);
    layout->addLayout(top);

    auto *reading = label(value, "SignalValue", card);
    layout->addWidget(reading);

    auto *bar = new SegmentBar(card);
    bar->setAccent(accent);
    bar->setValue(0.0);
    layout->addWidget(bar);

    return card;
}

} // namespace

DestinationPage::DestinationPage(const QString &title,
                                 const QString &subtitle,
                                 const QString &note,
                                 const QColor &accent,
                                 bool showcase,
                                 QWidget *parent)
    : QWidget(parent)
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
    auto *titleLabel = label(title, "PageTitle", header);
    auto *subtitleLabel = label(subtitle, "PageSubtitle", header);
    titles->addWidget(titleLabel);
    titles->addWidget(subtitleLabel);
    headerLayout->addLayout(titles, 1);

    auto *pill = label(QStringLiteral("FOUNDATION BUILD"), "StatusPill", header);
    headerLayout->addWidget(pill, 0, Qt::AlignTop);
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

    if (showcase) {
        auto *grid = new QGridLayout();
        grid->setSpacing(12);
        grid->addWidget(signalCard(QStringLiteral("CPU"), QStringLiteral("—"), QColor("#69e06b"), content), 0, 0);
        grid->addWidget(signalCard(QStringLiteral("Memory"), QStringLiteral("—"), QColor("#d178ff"), content), 0, 1);
        grid->addWidget(signalCard(QStringLiteral("Storage"), QStringLiteral("—"), QColor("#59d98e"), content), 1, 0);
        grid->addWidget(signalCard(QStringLiteral("Network"), QStringLiteral("—"), QColor("#5b9dff"), content), 1, 1);
        contentLayout->addLayout(grid);

        auto *card = new QFrame(content);
        card->setObjectName(QStringLiteral("Card"));
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(18, 16, 18, 16);
        cardLayout->setSpacing(6);
        cardLayout->addWidget(label(QStringLiteral("Precision instrumentation"), "CardTitle", card));
        cardLayout->addWidget(label(
            QStringLiteral("Every reading will be real, explicitly unavailable, or explicitly denied. "
                           "A stopped provider leaves a gap in history — never a fabricated zero."),
            "CardBody",
            card));
        contentLayout->addWidget(card);
    }

    auto *empty = new QFrame(content);
    empty->setObjectName(QStringLiteral("EmptyState"));
    empty->setMinimumHeight(180);
    auto *emptyLayout = new QVBoxLayout(empty);
    emptyLayout->setContentsMargins(24, 30, 24, 30);
    emptyLayout->setSpacing(8);
    emptyLayout->setSizeConstraint(QLayout::SetMinimumSize);
    auto *emptyTitle = label(QStringLiteral("This destination is wired."), "EmptyTitle", empty);
    emptyTitle->setWordWrap(false);
    emptyLayout->addWidget(emptyTitle, 0, Qt::AlignHCenter);
    auto *body = label(note, "EmptyBody", empty);
    body->setAlignment(Qt::AlignHCenter);
    body->setFixedWidth(720);
    body->setMinimumHeight(52);
    body->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    emptyLayout->addWidget(body, 0, Qt::AlignHCenter);
    auto *status = label(QStringLiteral("Measurement providers arrive in v0.2"), "CardAccent", empty);
    status->setAlignment(Qt::AlignHCenter);
    status->setWordWrap(false);
    status->setMinimumWidth(320);
    status->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    status->setStyleSheet(QStringLiteral("color: %1;").arg(accent.name()));
    emptyLayout->addWidget(status, 0, Qt::AlignHCenter);
    contentLayout->addWidget(empty);

    contentLayout->addStretch();
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);
}

} // namespace tmog::ui
