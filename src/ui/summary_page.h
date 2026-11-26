#pragma once

#include "core/metrics.h"
#include "ui/theme.h"

#include <QColor>
#include <QWidget>

class QFrame;
class QLabel;

namespace tmog::ui {

class SegmentBar;

class SummaryPage : public QWidget
{
    Q_OBJECT

public:
    explicit SummaryPage(QWidget *parent = nullptr);

    void apply(const MetricsSnapshot &snapshot);
    void setTheme(const ThemeTokens &theme);

private:
    struct Card
    {
        Card() = default;
        QLabel *state = nullptr;
        QLabel *value = nullptr;
        QLabel *detail = nullptr;
        SegmentBar *bar = nullptr;
        QColor accent;
        bool hasBar = true;
    };

    QFrame *createCard(const QString &title,
                       const QColor &accent,
                       bool hasBar,
                       bool compactValue,
                       Card &card,
                       QWidget *parent);
    void applyCards();

    Card m_cpuCard;
    Card m_memoryCard;
    Card m_storageCard;
    Card m_networkCard;
    QLabel *m_pill = nullptr;
    ThemeTokens m_theme;
    MetricsSnapshot m_snapshot;
    bool m_hasSnapshot = false;
};

} // namespace tmog::ui
