#include "ui/main_window.h"

#include "core/settings.h"
#include "core/version.h"
#include "ui/command_palette.h"
#include "ui/destination_page.h"
#include "ui/settings_dialog.h"
#include "ui/theme.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPoint>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QUrl>
#include <QVBoxLayout>

namespace tmog::ui {

namespace {

template <typename Context, typename Slot>
QAction *addMenuAction(QMenu *menu, const QString &text, const QKeySequence &shortcut, Context *context, Slot slot)
{
    auto *action = menu->addAction(text);
    if (!shortcut.isEmpty()) {
        action->setShortcut(shortcut);
    }
    QObject::connect(action, &QAction::triggered, context, slot);
    return action;
}

} // namespace

MainWindow::MainWindow(Settings &settings, QWidget *parent)
    : QMainWindow(parent)
    , m_settings(settings)
{
    setWindowTitle(tmog::productName());
    resize(1280, 900);
    setMinimumSize(980, 640);

    buildDestinations();
    buildSidebar();
    buildPages();
    buildMenus();
    buildStatusBar();

    m_themeId = m_settings.getString(QStringLiteral("appearance/theme"), QStringLiteral("dark"));
    m_textScale = qBound(100, m_settings.getInt(QStringLiteral("appearance/text_scale"), 100), 200);
    applyAppearance(m_themeId, m_textScale);

    const QString saved = m_settings.getString(
        QStringLiteral("navigation/last_destination"), QStringLiteral("summary"));
    int index = 0;
    for (int i = 0; i < m_destinations.size(); ++i) {
        if (m_destinations.at(i).id == saved) {
            index = i;
            break;
        }
    }
    navigateTo(index);
}

void MainWindow::buildDestinations()
{
    m_destinations = {
        {QStringLiteral("summary"), QStringLiteral("Summary"),
         QStringLiteral("One glance at the whole machine."),
         QStringLiteral("The landing view will combine CPU, memory, storage, network and process pressure into one glance."),
         QStringLiteral("OBSERVE"), QColor("#5b8cff"), true},
        {QStringLiteral("performance"), QStringLiteral("Performance"),
         QStringLiteral("History for every subsystem."),
         QStringLiteral("Per-subsystem history for CPU, memory, disks, network, GPU and energy."),
         QStringLiteral("OBSERVE"), QColor("#69e06b"), false},
        {QStringLiteral("processes"), QStringLiteral("Processes"),
         QStringLiteral("What is spending your machine."),
         QStringLiteral("A sortable, filterable process tree with identity-safe actions."),
         QStringLiteral("OBSERVE"), QColor("#d178ff"), false},
        {QStringLiteral("system-info"), QStringLiteral("System Info"),
         QStringLiteral("Hardware, firmware and the running system."),
         QStringLiteral("Hardware, firmware, operating system and boot details."),
         QStringLiteral("SYSTEM"), QColor("#7fa7ff"), false},
        {QStringLiteral("startup-apps"), QStringLiteral("Startup apps"),
         QStringLiteral("What starts with your session."),
         QStringLiteral("See and control what starts with your session."),
         QStringLiteral("SYSTEM"), QColor("#59d98e"), false},
        {QStringLiteral("users"), QStringLiteral("Users"),
         QStringLiteral("Local accounts and active sessions."),
         QStringLiteral("Local accounts, session type and login time."),
         QStringLiteral("SYSTEM"), QColor("#ffe066"), false},
        {QStringLiteral("services"), QStringLiteral("Services"),
         QStringLiteral("Background services and their state."),
         QStringLiteral("Service state and startup type, with permission-aware controls."),
         QStringLiteral("SYSTEM"), QColor("#ff9d5c"), false},
        {QStringLiteral("power-freq"), QStringLiteral("Power & Freq"),
         QStringLiteral("Watts, clocks and battery."),
         QStringLiteral("Watts, battery state, clocks and per-process energy where the OS exposes it."),
         QStringLiteral("POWER & NETWORK"), QColor("#ffe066"), false},
        {QStringLiteral("connections"), QStringLiteral("Connections"),
         QStringLiteral("Endpoints and the processes that own them."),
         QStringLiteral("TCP and UDP endpoints with the process that owns them."),
         QStringLiteral("POWER & NETWORK"), QColor("#5b9dff"), false},
        {QStringLiteral("installed-apps"), QStringLiteral("Installed Apps"),
         QStringLiteral("What is installed and how large it is."),
         QStringLiteral("Installed software inventory and uninstall entry points."),
         QStringLiteral("STORAGE & APPS"), QColor("#7fa7ff"), false},
        {QStringLiteral("disk-space"), QStringLiteral("Disk Space"),
         QStringLiteral("Find what is using the disk."),
         QStringLiteral("Find the largest folders and files without leaving the app."),
         QStringLiteral("STORAGE & APPS"), QColor("#59d98e"), false},
        {QStringLiteral("benchmarks"), QStringLiteral("Benchmarks"),
         QStringLiteral("Local measurements with a stated method."),
         QStringLiteral("Local CPU, memory, disk and GPU measurements with published methodology."),
         QStringLiteral("TOOLS"), QColor("#ff9d5c"), false},
    };
}

void MainWindow::buildSidebar()
{
    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *sidebar = new QWidget(central);
    sidebar->setObjectName(QStringLiteral("Sidebar"));
    sidebar->setFixedWidth(224);
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);

    auto *brand = new QWidget(sidebar);
    brand->setObjectName(QStringLiteral("BrandBlock"));
    auto *brandLayout = new QVBoxLayout(brand);
    brandLayout->setContentsMargins(18, 14, 16, 10);
    brandLayout->setSpacing(1);
    auto *mark = new QLabel(QStringLiteral("TMOG"), brand);
    mark->setObjectName(QStringLiteral("BrandMark"));
    auto *line = new QLabel(QStringLiteral("TASK MANAGER OG"), brand);
    line->setObjectName(QStringLiteral("BrandLine"));
    auto *byline = new QLabel(QStringLiteral("by Don PELY"), brand);
    byline->setObjectName(QStringLiteral("BrandByline"));
    brandLayout->addWidget(mark);
    brandLayout->addWidget(line);
    brandLayout->addWidget(byline);
    sideLayout->addWidget(brand);

    m_nav = new QListWidget(sidebar);
    m_nav->setObjectName(QStringLiteral("Nav"));
    m_nav->setFrameShape(QFrame::NoFrame);
    m_nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_nav->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QString currentGroup;
    for (int index = 0; index < m_destinations.size(); ++index) {
        const auto &destination = m_destinations.at(index);
        if (destination.group != currentGroup) {
            currentGroup = destination.group;
            auto *header = new QListWidgetItem(currentGroup, m_nav);
            header->setFlags(Qt::NoItemFlags);
        }
        auto *item = new QListWidgetItem(destination.title, m_nav);
        item->setData(Qt::UserRole, index);
    }
    sideLayout->addWidget(m_nav, 1);

    auto *footer = new QWidget(sidebar);
    footer->setObjectName(QStringLiteral("SidebarFooter"));
    auto *footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(8, 8, 8, 8);
    footerLayout->setSpacing(4);

    auto *settingsButton = new QPushButton(QStringLiteral("Settings"), footer);
    settingsButton->setObjectName(QStringLiteral("GhostButton"));
    auto *colorsButton = new QPushButton(QStringLiteral("Colors"), footer);
    colorsButton->setObjectName(QStringLiteral("GhostButton"));
    footerLayout->addWidget(settingsButton);
    footerLayout->addWidget(colorsButton);
    sideLayout->addWidget(footer);

    connect(m_nav, &QListWidget::currentRowChanged, this, [this](int row) {
        const auto *item = m_nav->item(row);
        if (!item || !(item->flags() & Qt::ItemIsEnabled)) {
            return;
        }
        navigateTo(item->data(Qt::UserRole).toInt());
    });

    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    connect(colorsButton, &QPushButton::clicked, this, [this, colorsButton]() {
        QMenu menu(colorsButton);
        menu.setTitle(QStringLiteral("Colors"));
        for (const auto &theme : themes()) {
            auto *action = menu.addAction(theme.name);
            action->setCheckable(true);
            action->setChecked(theme.id == m_themeId);
            connect(action, &QAction::triggered, this, [this, id = theme.id]() {
                applyAppearance(id, m_textScale);
            });
        }
        menu.exec(colorsButton->mapToGlobal(QPoint(0, -menu.sizeHint().height() - 4)));
    });

    layout->addWidget(sidebar);

    m_pages = new QStackedWidget(central);
    m_pages->setObjectName(QStringLiteral("Pages"));
    layout->addWidget(m_pages, 1);
    setCentralWidget(central);
}

void MainWindow::buildPages()
{
    for (const auto &destination : m_destinations) {
        m_pages->addWidget(new DestinationPage(destination.title,
                                               destination.subtitle,
                                               destination.note,
                                               destination.accent,
                                               destination.showcase,
                                               m_pages));
    }
}

void MainWindow::buildMenus()
{
    auto *appMenu = menuBar()->addMenu(QStringLiteral("&Task Manager OG"));
    addMenuAction(appMenu, QStringLiteral("About Task Manager OG by Don PELY"), {}, this, &MainWindow::showAbout);
    addMenuAction(appMenu, QStringLiteral("Settings…"), QKeySequence::Preferences, this, &MainWindow::openSettings);
    appMenu->addSeparator();
    addMenuAction(appMenu, QStringLiteral("Quit"), QKeySequence::Quit, qApp, &QApplication::quit);

    auto *viewMenu = menuBar()->addMenu(QStringLiteral("&View"));
    auto *goAction = viewMenu->addAction(QStringLiteral("Go to…"));
    goAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+K")));
    connect(goAction, &QAction::triggered, this, &MainWindow::openPalette);

    auto *themeMenu = viewMenu->addMenu(QStringLiteral("Theme"));
    m_themeGroup = new QActionGroup(this);
    m_themeGroup->setExclusive(true);
    for (const auto &theme : themes()) {
        auto *action = themeMenu->addAction(theme.name);
        action->setCheckable(true);
        action->setData(theme.id);
        action->setActionGroup(m_themeGroup);
        connect(action, &QAction::triggered, this, [this, id = theme.id]() {
            applyAppearance(id, m_textScale);
        });
    }

    auto *scaleMenu = viewMenu->addMenu(QStringLiteral("Text size"));
    m_scaleGroup = new QActionGroup(this);
    m_scaleGroup->setExclusive(true);
    for (int scale : {100, 125, 150, 175, 200}) {
        auto *action = scaleMenu->addAction(QStringLiteral("%1%").arg(scale));
        action->setCheckable(true);
        action->setData(scale);
        action->setActionGroup(m_scaleGroup);
        connect(action, &QAction::triggered, this, [this, scale]() {
            applyAppearance(m_themeId, scale);
        });
    }

    viewMenu->addSeparator();
    addMenuAction(viewMenu, QStringLiteral("Refresh now"), QKeySequence::Refresh, this, [this]() {
        statusBar()->showMessage(QStringLiteral("Waiting for measurement providers (v0.2)."), 2500);
    });

    auto *helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    addMenuAction(helpMenu, QStringLiteral("Project on GitHub"), {}, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/yourbossdonpely/tmog-taskmanager")));
    });
    addMenuAction(helpMenu, QStringLiteral("About Qt"), {}, qApp, []() {
        QMessageBox::aboutQt(qApp->activeWindow());
    });
}

void MainWindow::buildStatusBar()
{
    m_statusLeft = new QLabel(QStringLiteral("Foundation build · providers not connected"), this);
    m_statusRight = new QLabel(tmog::versionLine(), this);
    statusBar()->addWidget(m_statusLeft, 1);
    statusBar()->addPermanentWidget(m_statusRight);
}

void MainWindow::navigateTo(int destinationIndex)
{
    if (destinationIndex < 0 || destinationIndex >= m_destinations.size()) {
        return;
    }
    m_pages->setCurrentIndex(destinationIndex);

    for (int row = 0; row < m_nav->count(); ++row) {
        auto *item = m_nav->item(row);
        if (item->data(Qt::UserRole).toInt() == destinationIndex
            && (item->flags() & Qt::ItemIsEnabled)) {
            m_nav->setCurrentRow(row);
            break;
        }
    }

    m_settings.set(QStringLiteral("navigation/last_destination"),
                   m_destinations.at(destinationIndex).id);
    m_settings.save();
}

void MainWindow::openPalette()
{
    QStringList entries;
    for (const auto &destination : m_destinations) {
        entries.append(destination.title);
    }

    CommandPalette palette(entries, this);
    if (palette.exec() == QDialog::Accepted) {
        navigateTo(palette.selectedIndex());
    }
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(m_themeId, m_textScale, this);
    if (dialog.exec() == QDialog::Accepted) {
        applyAppearance(dialog.selectedTheme(), dialog.selectedTextScale());
    }
}

void MainWindow::showAbout()
{
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("About"));
    box.setTextFormat(Qt::PlainText);
    box.setText(tmog::productName() + QStringLiteral("\nVersion ") + tmog::version());
    box.setInformativeText(
        QStringLiteral("A native, privacy-first task manager for Windows, macOS and Linux.\n\n"
                       "Measured, never invented. MIT licensed.\n"
                       "Not affiliated with Dave Plummer or Plummers' Software LLC."));
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

void MainWindow::applyAppearance(const QString &themeId, int textScalePercent)
{
    m_themeId = themeById(themeId).id;
    m_textScale = qBound(100, textScalePercent, 200);

    ui::apply(*qApp, m_themeId, m_textScale);
    m_settings.set(QStringLiteral("appearance/theme"), m_themeId);
    m_settings.set(QStringLiteral("appearance/text_scale"), m_textScale);
    m_settings.save();

    if (m_statusLeft) {
        m_statusLeft->setText(QStringLiteral("Foundation build · providers not connected"));
    }
    updateThemeChecks();
}

void MainWindow::updateThemeChecks()
{
    if (m_themeGroup) {
        for (auto *action : m_themeGroup->actions()) {
            action->setChecked(action->data().toString() == m_themeId);
        }
    }
    if (m_scaleGroup) {
        for (auto *action : m_scaleGroup->actions()) {
            action->setChecked(action->data().toInt() == m_textScale);
        }
    }
}

} // namespace tmog::ui
