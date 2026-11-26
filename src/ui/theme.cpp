#include "ui/theme.h"

#include <QApplication>
#include <QFont>
#include <QStringList>

namespace tmog::ui {

namespace {

ThemeTokens makeDark(const QString &id, const QString &name, const QColor &accent)
{
    return ThemeTokens{
        .id = id,
        .name = name,
        .dark = true,
        .surface = QColor("#0b0e13"),
        .panel = QColor("#11161d"),
        .panelRaised = QColor("#161d26"),
        .line = QColor("#24303c"),
        .text = QColor("#e7edf5"),
        .textMuted = QColor("#8b9aab"),
        .accent = accent,
        .cpu = QColor("#69e06b"),
        .memory = QColor("#d178ff"),
        .disk = QColor("#59d98e"),
        .network = QColor("#5b9dff"),
        .energy = QColor("#ffe066"),
        .thermal = QColor("#ff9d5c"),
        .gpu = QColor("#7fa7ff"),
        .danger = QColor("#ff6b6b"),
    };
}

ThemeTokens makeLight()
{
    return ThemeTokens{
        .id = QStringLiteral("light"),
        .name = QStringLiteral("Light"),
        .dark = false,
        .surface = QColor("#f4f5f7"),
        .panel = QColor("#ffffff"),
        .panelRaised = QColor("#eef1f5"),
        .line = QColor("#d7dce3"),
        .text = QColor("#10151c"),
        .textMuted = QColor("#5a6675"),
        .accent = QColor("#2f5fd0"),
        .cpu = QColor("#1c8a3a"),
        .memory = QColor("#8a2bb8"),
        .disk = QColor("#177a4a"),
        .network = QColor("#235fbf"),
        .energy = QColor("#9a7500"),
        .thermal = QColor("#b34d00"),
        .gpu = QColor("#3559b8"),
        .danger = QColor("#c02626"),
    };
}

QString rgba(const QColor &color, double alpha)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(alpha, 0, 'f', 3);
}

int scaled(int base, int scalePercent)
{
    return qMax(8, (base * scalePercent + 50) / 100);
}

} // namespace

const std::array<ThemeTokens, 6> &themes()
{
    static const std::array<ThemeTokens, 6> value{
        makeDark(QStringLiteral("dark"), QStringLiteral("Dark"), QColor("#5b8cff")),
        makeLight(),
        makeDark(QStringLiteral("green"), QStringLiteral("Green"), QColor("#69e06b")),
        makeDark(QStringLiteral("amber"), QStringLiteral("Amber"), QColor("#ffb257")),
        makeDark(QStringLiteral("blue"), QStringLiteral("Blue"), QColor("#5b9dff")),
        makeDark(QStringLiteral("mono"), QStringLiteral("Mono"), QColor("#c8d3df")),
    };
    return value;
}

const ThemeTokens &themeById(const QString &id)
{
    for (const auto &theme : themes()) {
        if (theme.id == id) {
            return theme;
        }
    }
    return themes().front();
}

QStringList themeIds()
{
    QStringList ids;
    for (const auto &theme : themes()) {
        ids.append(theme.id);
    }
    return ids;
}

QStringList themeNames()
{
    QStringList names;
    for (const auto &theme : themes()) {
        names.append(theme.name);
    }
    return names;
}

QString styleSheet(const ThemeTokens &theme, int textScalePercent)
{
    const int body = scaled(13, textScalePercent);
    const int small = scaled(11, textScalePercent);
    const int tiny = scaled(10, textScalePercent);
    const int heading = scaled(26, textScalePercent);
    const int subheading = scaled(15, textScalePercent);
    const QString selection = rgba(theme.accent, theme.dark ? 0.26 : 0.16);
    const QString hover = rgba(theme.accent, theme.dark ? 0.12 : 0.08);
    const QString card = rgba(theme.panelRaised, theme.dark ? 0.55 : 0.75);

    QString sheet = QStringLiteral(R"(
        QWidget {
            color: @text;
            font-family: "Segoe UI", "Inter", "Helvetica Neue", sans-serif;
            font-size: @bodypx;
        }
        QMainWindow, QDialog {
            background: @surface;
        }
        QToolTip {
            background: @panelRaised;
            color: @text;
            border: 1px solid @line;
            padding: 6px 8px;
        }
        QMenuBar {
            background: @panel;
            color: @textMuted;
            border-bottom: 1px solid @line;
            padding: 2px 6px;
        }
        QMenuBar::item {
            background: transparent;
            padding: 5px 10px;
        }
        QMenuBar::item:selected {
            background: @hover;
            color: @text;
            border-radius: 5px;
        }
        QMenu {
            background: @panelRaised;
            color: @text;
            border: 1px solid @line;
            padding: 6px;
        }
        QMenu::item {
            padding: 7px 24px 7px 12px;
            border-radius: 5px;
        }
        QMenu::item:selected {
            background: @selection;
        }
        #Sidebar {
            background: @panel;
            border-right: 1px solid @line;
        }
        #BrandBlock {
            padding: 18px 16px 14px 18px;
        }
        #BrandMark {
            color: @accent;
            font-family: "Cascadia Mono", "IBM Plex Mono", "Consolas", monospace;
            font-size: @subheadingpx;
            font-weight: 700;
        }
        #BrandLine {
            color: @text;
            font-size: @smallpx;
            font-weight: 600;
        }
        #BrandByline {
            color: @textMuted;
            font-size: @tinypx;
        }
        QListWidget#Nav {
            background: transparent;
            border: none;
            outline: 0;
            padding: 2px 0;
        }
        QListWidget#Nav::item {
            color: @textMuted;
            font-size: 12px;
            padding: 4px 10px;
            margin: 0 8px;
            border-radius: 6px;
        }
        QListWidget#Nav::item:hover {
            background: @hover;
            color: @text;
        }
        QListWidget#Nav::item:selected {
            background: @selection;
            color: @text;
        }
        QListWidget#Nav::item:disabled {
            color: @line;
            font-size: @tinypx;
            font-weight: 600;
            padding-top: 7px;
            padding-bottom: 1px;
        }
        #SidebarFooter {
            border-top: 1px solid @line;
            padding: 6px;
        }
        QPushButton {
            background: @panelRaised;
            color: @text;
            border: 1px solid @line;
            border-radius: 6px;
            padding: 7px 12px;
        }
        QPushButton:hover {
            border-color: @accent;
        }
        QPushButton:pressed {
            background: @selection;
        }
        QPushButton#GhostButton {
            background: transparent;
            border: 1px solid transparent;
            color: @textMuted;
            text-align: left;
            padding: 7px 10px;
        }
        QPushButton#GhostButton:hover {
            background: @hover;
            color: @text;
        }
        #PageHeader {
            padding: 22px 26px 12px 26px;
        }
        #PageTitle {
            color: @text;
            font-size: @headingpx;
            font-weight: 600;
        }
        #PageSubtitle {
            color: @textMuted;
            font-size: @smallpx;
        }
        #StatusPill {
            background: @card;
            border: 1px solid @line;
            border-radius: 10px;
            color: @textMuted;
            font-size: @tinypx;
            padding: 4px 10px;
        }
        #Card {
            background: @card;
            border: 1px solid @line;
            border-radius: 10px;
        }
        #CardTitle {
            color: @text;
            font-size: @subheadingpx;
            font-weight: 600;
        }
        #CardBody {
            color: @textMuted;
            font-size: @smallpx;
        }
        #CardAccent {
            font-family: "Cascadia Mono", "IBM Plex Mono", "Consolas", monospace;
            font-size: @tinypx;
            font-weight: 600;
            color: @accent;
        }
        #EmptyState {
            background: @card;
            border: 1px dashed @line;
            border-radius: 10px;
        }
        #EmptyTitle {
            color: @text;
            font-size: @subheadingpx;
            font-weight: 600;
        }
        #EmptyBody {
            color: @textMuted;
            font-size: @smallpx;
        }
        #SignalGrid {
            background: transparent;
        }
        #SignalCell {
            background: @card;
            border: 1px solid @line;
            border-radius: 8px;
        }
        #SignalLabel {
            color: @textMuted;
            font-size: @tinypx;
        }
        #SignalValue {
            color: @text;
            font-family: "Cascadia Mono", "IBM Plex Mono", "Consolas", monospace;
            font-size: @subheadingpx;
            font-weight: 600;
        }
        QStatusBar {
            background: @panel;
            color: @textMuted;
            border-top: 1px solid @line;
            font-size: @tinypx;
        }
        QStatusBar::item {
            border: none;
        }
        QLineEdit, QComboBox {
            background: @panelRaised;
            color: @text;
            border: 1px solid @line;
            border-radius: 6px;
            padding: 7px 9px;
            selection-background-color: @accent;
        }
        QLineEdit:focus, QComboBox:focus {
            border-color: @accent;
        }
        QComboBox QAbstractItemView {
            background: @panelRaised;
            color: @text;
            border: 1px solid @line;
            selection-background-color: @selection;
        }
        QDialogButtonBox QPushButton {
            min-width: 84px;
        }
        QScrollArea {
            border: none;
            background: transparent;
        }
        QScrollArea > QWidget > QWidget, #ScrollViewport, #Pages {
            background: transparent;
        }
    )");

    sheet.replace(QStringLiteral("@surface"), theme.surface.name());
    sheet.replace(QStringLiteral("@panelRaised"), theme.panelRaised.name());
    sheet.replace(QStringLiteral("@panel"), theme.panel.name());
    sheet.replace(QStringLiteral("@line"), theme.line.name());
    sheet.replace(QStringLiteral("@textMuted"), theme.textMuted.name());
    sheet.replace(QStringLiteral("@text"), theme.text.name());
    sheet.replace(QStringLiteral("@accent"), theme.accent.name());
    sheet.replace(QStringLiteral("@selection"), selection);
    sheet.replace(QStringLiteral("@hover"), hover);
    sheet.replace(QStringLiteral("@card"), card);
    sheet.replace(QStringLiteral("@bodypx"), QStringLiteral("%1px").arg(body));
    sheet.replace(QStringLiteral("@smallpx"), QStringLiteral("%1px").arg(small));
    sheet.replace(QStringLiteral("@tinypx"), QStringLiteral("%1px").arg(tiny));
    sheet.replace(QStringLiteral("@headingpx"), QStringLiteral("%1px").arg(heading));
    sheet.replace(QStringLiteral("@subheadingpx"), QStringLiteral("%1px").arg(subheading));
    return sheet;
}

void apply(QApplication &app, const QString &themeId, int textScalePercent)
{
    app.setStyleSheet(styleSheet(themeById(themeId), textScalePercent));
}

} // namespace tmog::ui
