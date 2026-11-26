#pragma once

#include <QColor>
#include <QString>
#include <array>

class QApplication;

namespace tmog::ui {

struct ThemeTokens
{
    QString id;
    QString name;
    bool dark = true;

    QColor surface;
    QColor panel;
    QColor panelRaised;
    QColor line;
    QColor text;
    QColor textMuted;
    QColor accent;
    QColor cpu;
    QColor memory;
    QColor disk;
    QColor network;
    QColor energy;
    QColor thermal;
    QColor gpu;
    QColor danger;
};

const std::array<ThemeTokens, 6> &themes();
const ThemeTokens &themeById(const QString &id);
QStringList themeIds();
QStringList themeNames();

QString styleSheet(const ThemeTokens &theme, int textScalePercent);
void apply(QApplication &app, const QString &themeId, int textScalePercent);

} // namespace tmog::ui
