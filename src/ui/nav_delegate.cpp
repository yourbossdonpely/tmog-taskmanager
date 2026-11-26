#include "ui/nav_delegate.h"

#include "ui/icons.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QStyleOptionViewItem>

namespace tmog::ui {

namespace {

QColor mix(const QColor &from, const QColor &to, qreal amount)
{
    return QColor::fromRgbF(from.redF() * (1.0 - amount) + to.redF() * amount,
                            from.greenF() * (1.0 - amount) + to.greenF() * amount,
                            from.blueF() * (1.0 - amount) + to.blueF() * amount);
}

int scaled(int base, int percent)
{
    return qMax(8, (base * percent + 50) / 100);
}

} // namespace

NavDelegate::NavDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_theme(themeById(QStringLiteral("dark")))
{
}

void NavDelegate::setTheme(const ThemeTokens &theme, int textScalePercent)
{
    m_theme = theme;
    m_textScale = textScalePercent;
}

QSize NavDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const int type = index.data(TypeRole).toInt();
    const int height = type == Header ? scaled(28, m_textScale) : scaled(54, m_textScale);
    return {option.rect.width(), height};
}

void NavDelegate::paint(QPainter *painter,
                        const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setClipRect(option.rect);

    const int type = index.data(TypeRole).toInt();
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;

    if (type == Header) {
        const QString label = index.data(Qt::DisplayRole).toString();
        QFont font = QApplication::font();
        font.setPixelSize(scaled(10, m_textScale));
        font.setWeight(QFont::DemiBold);
        font.setLetterSpacing(QFont::AbsoluteSpacing, 0.9);
        painter->setFont(font);

        const QRect textRect(option.rect.left() + 26,
                             option.rect.top(),
                             option.rect.width() - 52,
                             option.rect.height());
        const int textWidth = QFontMetrics(font).horizontalAdvance(label);
        const int textY = textRect.center().y() + QFontMetrics(font).ascent() / 2 - 1;
        painter->setPen(m_theme.accent);
        painter->drawText(option.rect.left() + 26, textY, label);

        const int lineStart = option.rect.left() + 26 + textWidth + 12;
        const int lineEnd = option.rect.right() - 22;
        if (lineEnd > lineStart) {
            QColor line = m_theme.line;
            line.setAlpha(170);
            painter->setPen(QPen(line, 1.0));
            painter->drawLine(lineStart, textRect.center().y(), lineEnd, textRect.center().y());
        }
        painter->restore();
        return;
    }

    const QRectF row = QRectF(option.rect).adjusted(10.0, 1.0, -16.0, -1.0);
    const qreal radius = 8.0;
    if (selected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(mix(m_theme.panel, m_theme.accent, m_theme.dark ? 0.24 : 0.18));
        painter->drawRoundedRect(row, radius, radius);
    } else if (hovered) {
        QColor hover = m_theme.text;
        hover.setAlpha(m_theme.dark ? 16 : 10);
        painter->setPen(Qt::NoPen);
        painter->setBrush(hover);
        painter->drawRoundedRect(row, radius, radius);
    }

    const QColor foreground = selected || hovered ? m_theme.text : m_theme.textMuted;
    const qreal iconSize = 20.0;
    const QRectF iconRect(row.left() + 26.0,
                          row.center().y() - iconSize / 2.0,
                          iconSize,
                          iconSize);
    drawLineIcon(*painter,
                 index.data(IconRole).toString(),
                 iconRect,
                 selected ? m_theme.text : (hovered ? m_theme.text : m_theme.textMuted),
                 1.45);

    QFont font = QApplication::font();
    font.setPixelSize(scaled(16, m_textScale));
    font.setWeight(QFont::Medium);
    painter->setFont(font);
    painter->setPen(foreground);

    const QRectF textRect(row.left() + 64.0,
                          row.top(),
                          row.width() - 76.0,
                          row.height());
    const QString label = QFontMetrics(font).elidedText(index.data(Qt::DisplayRole).toString(),
                                                        Qt::ElideRight,
                                                        qRound(textRect.width()));
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, label);

    if (option.state & QStyle::State_HasFocus) {
        QColor focus = m_theme.accent;
        focus.setAlpha(150);
        painter->setPen(QPen(focus, 1.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(row.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
    }

    painter->restore();
}

} // namespace tmog::ui
