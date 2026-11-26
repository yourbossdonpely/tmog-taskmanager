#pragma once

#include "ui/theme.h"

#include <QStyledItemDelegate>

namespace tmog::ui {

class NavDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    enum ItemType {
        Header = 0,
        Destination = 1,
    };

    static constexpr int TypeRole = Qt::UserRole + 20;
    static constexpr int IconRole = Qt::UserRole + 21;

    explicit NavDelegate(QObject *parent = nullptr);

    void setTheme(const ThemeTokens &theme, int textScalePercent);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    ThemeTokens m_theme;
    int m_textScale = 100;
};

} // namespace tmog::ui
