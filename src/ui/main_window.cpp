#include "ui/main_window.h"

#include "core/settings.h"
#include "core/version.h"
#include "ui/command_palette.h"
#include "ui/destination_page.h"
#include "ui/icons.h"
#include "ui/nav_delegate.h"
#include "ui/performance_page.h"
#include "ui/reading_style.h"
#include "ui/settings_dialog.h"
#include "ui/summary_page.h"
#include "ui/theme.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
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
#include <QPainter>
#include <QPixmap>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>
#include <QToolButton>
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

QIcon makeLineIcon(const QString &key, const QColor &color, int logicalSize = 18)
{
    const int pixelSize = logicalSize * 2;
    QPixmap pixmap(pixelSize, pixelSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    drawLineIcon(painter,
                 key,
                 QRectF(4.0, 4.0, pixelSize - 8.0, pixelSize - 8.0),
                 color,
                 2.8);
    pixmap.setDevicePixelRatio(2.0);
    return QIcon(pixmap);
}

} // namespace

MainWindow::MainWindow(Settings &settings, QWidget *parent)
    : QMainWindow(parent)
    , m_settings(settings)
{
    setWindowTitle(tmog::productName());
    resize(1800, 1156);
    setMinimumSize(1200, 760);

    buildDestinations();
    buildSidebar();
    buildPages();
    buildMenus();
    buildStatusBar();

    m_themeId = m_settings.getString(QStringLiteral("appearance/theme"), QStringLiteral("dark"));
    m_textScale = qBound(100, m_settings.getInt(QStringLiteral("appearance/text_scale"), 100), 200);
    applyAppearance(m_themeId, m_textScale);

    m_refreshMs = qBound(250, m_settings.getInt(QStringLiteral("sampling/refresh_ms"), 1000), 10000);
    m_sampleTimer = new QTimer(this);
    m_sampleTimer->setInterval(m_refreshMs);
    connect(m_sampleTimer, &QTimer::timeout, this, &MainWindow::sampleNow);
    m_sampleTimer->start();
    sampleNow();

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
         QStringLiteral("CPU, memory, storage, network and process pressure at a glance."),
         QStringLiteral("OBSERVE"), QStringLiteral("summary"), QColor("#70e06a")},
        {QStringLiteral("performance"), QStringLiteral("Performance"),
         QStringLiteral("History for every subsystem."),
         QStringLiteral("Per-subsystem history for CPU, memory, storage, network and process pressure."),
         QStringLiteral("OBSERVE"), QStringLiteral("performance"), QColor("#70e06a")},
        {QStringLiteral("processes"), QStringLiteral("Processes"),
         QStringLiteral("What is spending your machine."),
         QStringLiteral("A sortable, filterable process tree with identity-safe actions."),
         QStringLiteral("OBSERVE"), QStringLiteral("processes"), QColor("#d178ff")},
        {QStringLiteral("system-info"), QStringLiteral("System Information"),
         QStringLiteral("Hardware, firmware and the running system."),
         QStringLiteral("Hardware, firmware, operating system and boot details."),
         QStringLiteral("SYSTEM"), QStringLiteral("system-info"), QColor("#7fa7ff")},
        {QStringLiteral("app-history"), QStringLiteral("App History"),
         QStringLiteral("How applications spend resources over time."),
         QStringLiteral("Per-application CPU, memory, network and energy history across sessions."),
         QStringLiteral("SYSTEM"), QStringLiteral("app-history"), QColor("#5b9dff")},
        {QStringLiteral("startup-apps"), QStringLiteral("Startup Apps"),
         QStringLiteral("What starts with your session."),
         QStringLiteral("See and control what starts with your session."),
         QStringLiteral("SYSTEM"), QStringLiteral("startup-apps"), QColor("#59d98e")},
        {QStringLiteral("users"), QStringLiteral("Users"),
         QStringLiteral("Local accounts and active sessions."),
         QStringLiteral("Local accounts, session type and login time."),
         QStringLiteral("SYSTEM"), QStringLiteral("users"), QColor("#ffe066")},
        {QStringLiteral("services"), QStringLiteral("Services"),
         QStringLiteral("Background services and their state."),
         QStringLiteral("Service state and startup type, with permission-aware controls."),
         QStringLiteral("SYSTEM"), QStringLiteral("services"), QColor("#ff9d5c")},
        {QStringLiteral("power-freq"), QStringLiteral("Power and Frequency"),
         QStringLiteral("Watts, clocks and battery."),
         QStringLiteral("Watts, battery state, clocks and per-process energy where the OS exposes it."),
         QStringLiteral("POWER & NETWORK"), QStringLiteral("power-freq"), QColor("#ffe066")},
        {QStringLiteral("flight-recorder"), QStringLiteral("Flight Recorder"),
         QStringLiteral("Record a slowdown, then replay it."),
         QStringLiteral("Record the live snapshot stream and scrub back through the cause."),
         QStringLiteral("POWER & NETWORK"), QStringLiteral("flight-recorder"), QColor("#ff6b6b")},
        {QStringLiteral("connections"), QStringLiteral("Connections"),
         QStringLiteral("Endpoints and the processes that own them."),
         QStringLiteral("TCP and UDP endpoints with the process that owns them."),
         QStringLiteral("POWER & NETWORK"), QStringLiteral("connections"), QColor("#5b9dff")},
        {QStringLiteral("installed-apps"), QStringLiteral("Installed Apps"),
         QStringLiteral("What is installed and how large it is."),
         QStringLiteral("Installed software inventory and uninstall entry points."),
         QStringLiteral("STORAGE & APPS"), QStringLiteral("installed-apps"), QColor("#7fa7ff")},
        {QStringLiteral("drivers"), QStringLiteral("Drivers"),
         QStringLiteral("Loaded drivers, versions and signing."),
         QStringLiteral("Loaded kernel drivers, publishers, versions and signature state."),
         QStringLiteral("STORAGE & APPS"), QStringLiteral("drivers"), QColor("#ff9d5c")},
        {QStringLiteral("deep-space"), QStringLiteral("Deep Space"),
         QStringLiteral("Map every byte on the disk."),
         QStringLiteral("Find the largest folders and files with a local, cancellable scan."),
         QStringLiteral("STORAGE & APPS"), QStringLiteral("deep-space"), QColor("#59d98e")},
        {QStringLiteral("benchmarks"), QStringLiteral("Benchmarks"),
         QStringLiteral("Local measurements with a stated method."),
         QStringLiteral("Local CPU, memory, disk and GPU measurements with published methodology."),
         QStringLiteral("TOOLS"), QStringLiteral("benchmarks"), QColor("#ff9d5c")},
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
    sidebar->setFixedWidth(346);
    m_sidebar = sidebar;
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);
    sideLayout->addSpacing(18);

    m_nav = new QListWidget(sidebar);
    m_nav->setObjectName(QStringLiteral("Nav"));
    m_nav->setFrameShape(QFrame::NoFrame);
    m_nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_nav->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_nav->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_nav->setMinimumHeight(0);
    m_nav->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    m_nav->setSelectionMode(QAbstractItemView::SingleSelection);
    m_nav->setIconSize(QSize(20, 20));
    m_nav->setSpacing(0);
    m_navDelegate = new NavDelegate(m_nav);
    m_nav->setItemDelegate(m_navDelegate);

    QString currentGroup;
    for (int index = 0; index < m_destinations.size(); ++index) {
        const auto &destination = m_destinations.at(index);
        if (destination.group != currentGroup) {
            currentGroup = destination.group;
            auto *header = new QListWidgetItem(currentGroup, m_nav);
            header->setFlags(Qt::NoItemFlags);
            header->setData(NavDelegate::TypeRole, NavDelegate::Header);
        }
        auto *item = new QListWidgetItem(destination.title, m_nav);
        item->setData(Qt::UserRole, index);
        item->setData(NavDelegate::TypeRole, NavDelegate::Destination);
        item->setData(NavDelegate::IconRole, destination.iconKey);
    }
    sideLayout->addWidget(m_nav, 1);

    auto *brand = new QWidget(sidebar);
    brand->setObjectName(QStringLiteral("BrandBlock"));
    auto *brandLayout = new QVBoxLayout(brand);
    brandLayout->setContentsMargins(24, 12, 20, 8);
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

    auto *footer = new QWidget(sidebar);
    footer->setObjectName(QStringLiteral("SidebarFooter"));
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(10, 8, 10, 10);
    footerLayout->setSpacing(8);

    const QColor footerIcon = themeById(QStringLiteral("dark")).textMuted;
    auto *settingsButton = new QPushButton(QStringLiteral("Settings"), footer);
    settingsButton->setObjectName(QStringLiteral("FooterButton"));
    settingsButton->setProperty("iconKey", QStringLiteral("settings"));
    settingsButton->setIcon(makeLineIcon(QStringLiteral("settings"), footerIcon));
    settingsButton->setIconSize(QSize(18, 18));
    auto *colorsButton = new QPushButton(QStringLiteral("Colors"), footer);
    colorsButton->setObjectName(QStringLiteral("FooterButton"));
    colorsButton->setProperty("iconKey", QStringLiteral("colors"));
    colorsButton->setIcon(makeLineIcon(QStringLiteral("colors"), footerIcon));
    colorsButton->setIconSize(QSize(18, 18));
    footerLayout->addWidget(settingsButton, 1);
    footerLayout->addWidget(colorsButton, 1);
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

    auto *right = new QWidget(central);
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    buildPageHeader();
    rightLayout->addWidget(m_pageHeader);

    m_pages = new QStackedWidget(right);
    m_pages->setObjectName(QStringLiteral("Pages"));
    rightLayout->addWidget(m_pages, 1);
    layout->addWidget(right, 1);
    setCentralWidget(central);
}

void MainWindow::buildPageHeader()
{
    m_pageHeader = new QWidget(this);
    m_pageHeader->setObjectName(QStringLiteral("PageHeader"));
    m_pageHeader->setFixedHeight(94);

    auto *layout = new QHBoxLayout(m_pageHeader);
    layout->setContentsMargins(38, 0, 22, 0);
    layout->setSpacing(26);

    m_pageIcon = new LineIcon(QStringLiteral("summary"), m_pageHeader);
    m_pageIcon->setIconSize(22);
    m_pageIcon->setColor(themeById(QStringLiteral("dark")).textMuted);
    layout->addWidget(m_pageIcon, 0, Qt::AlignVCenter);

    m_pageTitle = new QLabel(QStringLiteral("Summary"), m_pageHeader);
    m_pageTitle->setObjectName(QStringLiteral("PageTitle"));
    layout->addWidget(m_pageTitle, 0, Qt::AlignVCenter);
    layout->addStretch();

#ifndef Q_OS_MACOS
    m_menuButton = new QToolButton(m_pageHeader);
    m_menuButton->setObjectName(QStringLiteral("HeaderMenuButton"));
    m_menuButton->setToolTip(QStringLiteral("Application menu"));
    m_menuButton->setIcon(makeLineIcon(QStringLiteral("menu"),
                                       themeById(QStringLiteral("dark")).textMuted));
    m_menuButton->setIconSize(QSize(18, 18));
    m_menuButton->setFixedSize(28, 28);
    connect(m_menuButton, &QToolButton::clicked, this, &MainWindow::showAppMenu);
    layout->addWidget(m_menuButton, 0, Qt::AlignVCenter);
#endif
}

void MainWindow::showAppMenu()
{
    if (!m_menuButton) {
        return;
    }
    QMenu menu(this);
    for (QAction *top : menuBar()->actions()) {
        if (QMenu *submenu = top->menu()) {
            QMenu *copy = menu.addMenu(submenu->title());
            copy->addActions(submenu->actions());
        }
    }
    menu.exec(m_menuButton->mapToGlobal(QPoint(0, m_menuButton->height() + 4)));
}

void MainWindow::buildPages()
{
    for (const auto &destination : m_destinations) {
        if (destination.id == QStringLiteral("summary")) {
            m_summaryPage = new SummaryPage(m_pages);
            m_pages->addWidget(m_summaryPage);
        } else if (destination.id == QStringLiteral("performance")) {
            m_performancePage = new PerformancePage(m_pages);
            m_pages->addWidget(m_performancePage);
        } else {
            m_pages->addWidget(new DestinationPage(destination.title,
                                                   destination.note,
                                                   destination.accent,
                                                   destination.iconKey,
                                                   m_pages));
        }
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
        sampleNow();
        statusBar()->showMessage(QStringLiteral("Sampled now."), 1500);
    });

    auto *helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    addMenuAction(helpMenu, QStringLiteral("Project on GitHub"), {}, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/yourbossdonpely/tmog-taskmanager")));
    });
    addMenuAction(helpMenu, QStringLiteral("About Qt"), {}, qApp, []() {
        QMessageBox::aboutQt(qApp->activeWindow());
    });

#ifdef Q_OS_MACOS
    menuBar()->setNativeMenuBar(true);
#else
    menuBar()->setVisible(false);
#endif
}

void MainWindow::buildStatusBar()
{
    statusBar()->setFixedHeight(38);
    statusBar()->setSizeGripEnabled(false);

    m_statusDot = new QLabel(this);
    m_statusDot->setObjectName(QStringLiteral("StatusDot"));
    m_statusDot->setFixedSize(8, 8);
    m_statusDot->setStyleSheet(
        QStringLiteral("background-color: %1; border-radius: 4px;")
            .arg(themeById(QStringLiteral("dark")).textMuted.name()));

    m_statusLeft = new QLabel(QStringLiteral("Starting measurement providers…"), this);
    m_statusLeft->setObjectName(QStringLiteral("StatusText"));
    m_statusRight = new QLabel(tmog::versionLine(), this);
    m_statusRight->setObjectName(QStringLiteral("StatusText"));
    statusBar()->addWidget(m_statusDot, 0);
    statusBar()->addWidget(m_statusLeft, 1);
    statusBar()->addPermanentWidget(m_statusRight);
}

void MainWindow::sampleNow()
{
    if (!m_summaryPage || !m_performancePage) {
        return;
    }

    m_lastSnapshot = m_sampler.sample();
    m_hasSnapshot = true;
    m_summaryPage->apply(m_lastSnapshot);
    m_performancePage->apply(m_lastSnapshot);
    updateProviderStatus(m_lastSnapshot);
}

void MainWindow::updateProviderStatus(const MetricsSnapshot &snapshot)
{
    if (!m_statusLeft) {
        return;
    }

    const Reading network =
        combineReadings(snapshot.networkDownPerSecond, snapshot.networkUpPerSecond);
    const bool anyDenied = snapshot.cpuPercent.state == Availability::Denied
        || snapshot.memoryPercent.state == Availability::Denied
        || snapshot.storagePercent.state == Availability::Denied
        || network.state == Availability::Denied
        || snapshot.processPressure.state == Availability::Denied;
    const bool anyUnavailable = snapshot.cpuPercent.state == Availability::Unavailable
        || snapshot.memoryPercent.state == Availability::Unavailable
        || snapshot.storagePercent.state == Availability::Unavailable
        || network.state == Availability::Unavailable
        || snapshot.processPressure.state == Availability::Unavailable;
    const bool anyPending = snapshot.cpuPercent.state == Availability::Pending
        || snapshot.memoryPercent.state == Availability::Pending
        || snapshot.storagePercent.state == Availability::Pending
        || network.state == Availability::Pending
        || snapshot.processPressure.state == Availability::Pending;

    const ThemeTokens &theme = themeById(m_themeId);
    QColor dot = theme.cpu;
    QString summary;
    if (anyDenied) {
        dot = theme.danger;
        summary = QStringLiteral("Restricted · a provider was denied");
    } else if (anyUnavailable) {
        dot = theme.energy;
        summary = QStringLiteral("Degraded · one or more providers unavailable");
    } else if (anyPending) {
        dot = theme.textMuted;
        summary = QStringLiteral("Sampling · collecting first deltas");
    } else {
        summary = QStringLiteral("Native providers healthy");
    }

    if (m_statusDot) {
        m_statusDot->setStyleSheet(
            QStringLiteral("background-color: %1; border-radius: 4px;").arg(dot.name()));
    }

    QString processes;
    if (snapshot.processCount > 0) {
        processes = QStringLiteral(" · %1 processes").arg(snapshot.processCount);
    }
    const QString cadence = m_refreshMs >= 1000 && m_refreshMs % 1000 == 0
        ? QStringLiteral("%1 s").arg(m_refreshMs / 1000)
        : QStringLiteral("%1 ms").arg(m_refreshMs);
    m_statusLeft->setText(QStringLiteral("%1%2 · %3 sampling")
                              .arg(summary, processes, cadence));
}

void MainWindow::navigateTo(int destinationIndex)
{
    if (destinationIndex < 0 || destinationIndex >= m_destinations.size()) {
        return;
    }
    m_pages->setCurrentIndex(destinationIndex);
    updatePageHeader();

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

void MainWindow::updatePageHeader()
{
    if (!m_pageTitle || !m_pageIcon || !m_pages) {
        return;
    }
    const int index = m_pages->currentIndex();
    if (index < 0 || index >= m_destinations.size()) {
        return;
    }
    const Destination &destination = m_destinations.at(index);
    m_pageTitle->setText(destination.title);
    m_pageIcon->setIconKey(destination.iconKey);
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

    if (m_sidebar) {
        m_sidebar->setFixedWidth(qBound(346, 346 + (m_textScale - 100) * 2, 520));
    }
    if (m_nav) {
        m_nav->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_nav->viewport()->update();
    }

    const ThemeTokens &activeTheme = themeById(m_themeId);
    if (m_navDelegate) {
        m_navDelegate->setTheme(activeTheme, m_textScale);
    }
    if (m_pageIcon) {
        m_pageIcon->setColor(activeTheme.textMuted);
    }
    if (m_menuButton) {
        m_menuButton->setIcon(makeLineIcon(QStringLiteral("menu"), activeTheme.textMuted));
    }
    for (QPushButton *button : findChildren<QPushButton *>(QStringLiteral("FooterButton"))) {
        button->setIcon(makeLineIcon(button->property("iconKey").toString(),
                                     activeTheme.textMuted));
    }
    if (m_summaryPage) {
        m_summaryPage->setTheme(activeTheme);
    }
    if (m_performancePage) {
        m_performancePage->setTheme(activeTheme);
    }
    if (m_statusLeft) {
        if (m_hasSnapshot) {
            updateProviderStatus(m_lastSnapshot);
        } else {
            m_statusLeft->setText(QStringLiteral("Starting measurement providers…"));
        }
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
