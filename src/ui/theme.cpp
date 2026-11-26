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
        .surface = QColor("#1e1e1e"),
        .panel = QColor("#1e1e1e"),
        .panelRaised = QColor("#262724"),
        .line = QColor("#3a3b37"),
        .text = QColor("#ecece7"),
        .textMuted = QColor("#a3a39c"),
        .accent = accent,
        .cpu = QColor("#70e06a"),
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
        .surface = QColor("#f1f2ee"),
        .panel = QColor("#eceee8"),
        .panelRaised = QColor("#ffffff"),
        .line = QColor("#d7dbd2"),
        .text = QColor("#171a16"),
        .textMuted = QColor("#667068"),
        .accent = QColor("#4d6dcc"),
        .cpu = QColor("#1d8a3a"),
        .memory = QColor("#8a2bb8"),
        .disk = QColor("#177a4a"),
        .network = QColor("#235fbf"),
        .energy = QColor("#8a6800"),
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
        makeDark(QStringLiteral("dark"), QStringLiteral("Dark"), QColor("#6f8fde")),
        makeLight(),
        makeDark(QStringLiteral("green"), QStringLiteral("Green"), QColor("#70e06a")),
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
    const int body = scaled(14, textScalePercent);
    const int small = scaled(12, textScalePercent);
    const int tiny = scaled(10, textScalePercent);
    const int heading = scaled(36, textScalePercent);
    const int subheading = scaled(22, textScalePercent);
    const int detailHeading = scaled(26, textScalePercent);
    const int brand = scaled(30, textScalePercent);
    const int metric = scaled(30, textScalePercent);
    const int metricCompact = scaled(22, textScalePercent);
    const int miniTitle = scaled(13, textScalePercent);
    const QString selection = theme.dark ? QStringLiteral("#2c3550") : QStringLiteral("#dce5fb");
    const QString hover = theme.dark ? QStringLiteral("rgba(255,255,255,0.055)")
                                     : QStringLiteral("rgba(0,0,0,0.045)");
    const QString card = theme.panelRaised.name();

    QString sheet = QStringLiteral(R"(
        QWidget {
            color: @text;
            font-family: "Segoe UI Variable", "SF Pro Text", "Inter", "Segoe UI", "Helvetica Neue", sans-serif;
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
            background: transparent;
        }
        #BrandMark {
            color: @accent;
            font-family: "SF Mono", "Cascadia Mono", "IBM Plex Mono", "Consolas", monospace;
            font-size: @brandpx;
            font-weight: 700;
        }
        #BrandLine {
            color: @text;
            font-size: @tinypx;
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
            padding: 0;
        }
        QListWidget#Nav::item {
            background: transparent;
            border: none;
            padding: 0;
        }
        #SidebarFooter {
            background: @panel;
            border-top: 1px solid @line;
        }
        QPushButton#FooterButton {
            background: transparent;
            border: 1px solid transparent;
            color: @textMuted;
            text-align: left;
            padding: 0 14px;
            min-height: 44px;
            border-radius: 6px;
        }
        QPushButton#FooterButton:hover {
            background: @hover;
            color: @text;
        }
        QPushButton#FooterButton:pressed {
            background: @selection;
        }
        QToolButton#HeaderMenuButton {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 6px;
        }
        QToolButton#HeaderMenuButton:hover {
            background: @hover;
            border-color: @line;
        }
        #PageHeader {
            background: @surface;
        }
        #PageTitle {
            color: @text;
            font-family: "Segoe UI Variable Display", "SF Pro Display", "Inter", "Segoe UI", sans-serif;
            font-size: @headingpx;
            font-weight: 300;
        }
        #DetailTitle {
            color: @text;
            font-family: "Segoe UI Variable Display", "SF Pro Display", "Inter", "Segoe UI", sans-serif;
            font-size: @detailheadingpx;
            font-weight: 400;
        }
        #PageSubtitle {
            color: @textMuted;
            font-size: @smallpx;
        }
        #StatusPill {
            background: transparent;
            border: 1px solid @line;
            border-radius: 9px;
            color: @textMuted;
            font-family: "SF Mono", "Cascadia Mono", "Consolas", monospace;
            font-size: @tinypx;
            padding: 4px 9px;
        }
        #Card, #FeatureCard, #ModuleHero {
            background: @card;
            border: 1px solid @line;
            border-radius: 8px;
        }
        #ModuleIconTile {
            background: @surface;
            border: 1px solid @line;
            border-radius: 10px;
        }
        #SnapshotPanel {
            background: @surface;
            border: 1px solid @line;
            border-radius: 7px;
        }
        #ModuleBadge {
            font-family: "SF Mono", "Cascadia Mono", "Consolas", monospace;
            font-size: @tinypx;
            font-weight: 600;
        }
        #ModuleTitle {
            color: @text;
            font-size: @subheadingpx;
            font-weight: 500;
        }
        #ModuleBody {
            color: @textMuted;
            font-size: @smallpx;
        }
        #CardTitle {
            color: @text;
            font-size: @subheadingpx;
            font-weight: 500;
        }
        #CardBody {
            color: @textMuted;
            font-size: @smallpx;
        }
        #MiniTitle {
            font-size: @minititlepx;
            font-weight: 600;
        }
        #SignalLabel {
            color: @textMuted;
            font-size: @smallpx;
            font-weight: 500;
        }
        #SignalValue {
            color: @text;
            font-family: "SF Mono", "Cascadia Mono", "IBM Plex Mono", "Consolas", monospace;
            font-size: @subheadingpx;
            font-weight: 600;
        }
        #MetricValue {
            color: @text;
            font-family: "SF Mono", "Cascadia Mono", "IBM Plex Mono", "Consolas", monospace;
            font-size: @metricpx;
            font-weight: 500;
        }
        #MetricValueCompact {
            color: @text;
            font-family: "SF Mono", "Cascadia Mono", "IBM Plex Mono", "Consolas", monospace;
            font-size: @compactpx;
            font-weight: 500;
        }
        #MetricDetail {
            color: @textMuted;
            font-size: @smallpx;
        }
        #MetricState {
            font-family: "SF Mono", "Cascadia Mono", "Consolas", monospace;
            font-size: @tinypx;
            font-weight: 600;
        }
        #ChartLegend {
            color: @textMuted;
            font-size: @tinypx;
        }
        #EmptyState {
            background: @card;
            border: 1px dashed @line;
            border-radius: 8px;
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
        QStatusBar {
            background: @panel;
            color: @textMuted;
            border-top: 1px solid @line;
            font-size: @tinypx;
        }
        QStatusBar::item {
            border: none;
        }
        #StatusText {
            color: @textMuted;
            font-family: "SF Mono", "Cascadia Mono", "Consolas", monospace;
            font-size: @tinypx;
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
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: @line;
            min-height: 28px;
            border-radius: 3px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
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
    sheet.replace(QStringLiteral("@detailheadingpx"), QStringLiteral("%1px").arg(detailHeading));
    sheet.replace(QStringLiteral("@brandpx"), QStringLiteral("%1px").arg(brand));
    sheet.replace(QStringLiteral("@minititlepx"), QStringLiteral("%1px").arg(miniTitle));
    sheet.replace(QStringLiteral("@metricpx"), QStringLiteral("%1px").arg(metric));
    sheet.replace(QStringLiteral("@compactpx"), QStringLiteral("%1px").arg(metricCompact));
    return sheet;
}

void apply(QApplication &app, const QString &themeId, int textScalePercent)
{
    app.setStyleSheet(styleSheet(themeById(themeId), textScalePercent));
}

} // namespace tmog::ui
