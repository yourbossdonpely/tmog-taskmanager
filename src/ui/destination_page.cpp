#include "ui/destination_page.h"

#include "ui/icons.h"
#include "ui/theme.h"

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

void SegmentBar::setReadingAvailable(bool available)
{
    m_available = available;
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
        color.setAlpha(index < litCount ? 235 : (m_available ? 38 : 16));
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

QFrame *featureCard(const QString &iconKey,
                    const QString &title,
                    const QString &body,
                    const QColor &accent,
                    QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("FeatureCard"));
    card->setMinimumHeight(190);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);

    auto *icon = new LineIcon(iconKey, card);
    icon->setIconSize(26);
    icon->setColor(accent);
    layout->addWidget(icon, 0, Qt::AlignLeft);

    layout->addWidget(label(title, "CardTitle", card));
    layout->addWidget(label(body, "CardBody", card));
    layout->addStretch();

    return card;
}

} // namespace

DestinationPage::DestinationPage(const QString &title,
                                 const QString &note,
                                 const QColor &accent,
                                 const QString &iconKey,
                                 QWidget *parent)
    : QWidget(parent)
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
    contentLayout->setContentsMargins(22, 0, 16, 20);
    contentLayout->setSpacing(0);

    auto *hero = new QFrame(content);
    hero->setObjectName(QStringLiteral("ModuleHero"));
    hero->setFixedHeight(176);
    auto *heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(26, 22, 26, 22);
    heroLayout->setSpacing(20);

    auto *iconTile = new QFrame(hero);
    iconTile->setObjectName(QStringLiteral("ModuleIconTile"));
    iconTile->setFixedSize(72, 72);
    auto *iconLayout = new QVBoxLayout(iconTile);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    auto *icon = new LineIcon(iconKey, iconTile);
    icon->setIconSize(34);
    icon->setColor(accent);
    iconLayout->addWidget(icon, 0, Qt::AlignCenter);
    heroLayout->addWidget(iconTile, 0, Qt::AlignVCenter);

    auto *heroText = new QVBoxLayout();
    heroText->setSpacing(8);
    auto *badge = label(QStringLiteral("FREE · INCLUDED"), "ModuleBadge", hero);
    badge->setStyleSheet(QStringLiteral("color: %1;").arg(accent.name()));
    heroText->addWidget(badge, 0, Qt::AlignLeft);
    heroText->addWidget(label(title, "ModuleTitle", hero));
    auto *body = label(note, "ModuleBody", hero);
    body->setMaximumWidth(860);
    heroText->addWidget(body);
    heroText->addStretch();
    heroLayout->addLayout(heroText, 1);

    auto *state = label(QStringLiteral("PROVIDER PLANNED"), "StatusPill", hero);
    heroLayout->addWidget(state, 0, Qt::AlignTop);
    contentLayout->addWidget(hero);
    contentLayout->addSpacing(12);

    auto *features = new QGridLayout();
    features->setSpacing(12);
    features->addWidget(featureCard(iconKey,
                                    QStringLiteral("One place for the whole story"),
                                    note,
                                    accent,
                                    content),
                        0,
                        0);
    features->addWidget(featureCard(QStringLiteral("app-history"),
                                    QStringLiteral("Free in every build"),
                                    QStringLiteral("No Pro gate, no account and no licence server. "
                                                   "Every destination in this app is part of the "
                                                   "same free product."),
                                    accent,
                                    content),
                        0,
                        1);
    features->addWidget(featureCard(QStringLiteral("system-info"),
                                    QStringLiteral("Measured, never invented"),
                                    QStringLiteral("When a provider is unavailable or denied, this "
                                                   "page says why. A missing value is never filled "
                                                   "with a plausible zero."),
                                    accent,
                                    content),
                        0,
                        2);
    contentLayout->addLayout(features);
    contentLayout->addSpacing(12);

    auto *status = new QFrame(content);
    status->setObjectName(QStringLiteral("Card"));
    auto *statusLayout = new QHBoxLayout(status);
    statusLayout->setContentsMargins(20, 16, 20, 16);
    statusLayout->setSpacing(14);
    auto *statusIcon = new LineIcon(QStringLiteral("settings"), status);
    statusIcon->setIconSize(20);
    statusIcon->setColor(accent);
    statusLayout->addWidget(statusIcon, 0, Qt::AlignTop);
    auto *statusText = new QVBoxLayout();
    statusText->setSpacing(3);
    statusText->addWidget(label(QStringLiteral("This module is wired into the shell."), "CardTitle", status));
    statusText->addWidget(label(
        QStringLiteral("Its provider is the next implementation step. Until then the page reports "
                       "the work as planned instead of showing sample data."),
        "CardBody",
        status));
    statusLayout->addLayout(statusText, 1);
    contentLayout->addWidget(status);
    contentLayout->addStretch();

    scroll->setWidget(content);
    outer->addWidget(scroll, 1);
}

} // namespace tmog::ui
