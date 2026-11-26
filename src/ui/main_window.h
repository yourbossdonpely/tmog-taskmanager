#pragma once

#include "core/metrics.h"

#include <QColor>
#include <QMainWindow>
#include <QString>
#include <QVector>

class QAction;
class QActionGroup;
class QLabel;
class QListWidget;
class QMenu;
class QStackedWidget;
class QTimer;
class QToolButton;

namespace tmog {
class Settings;
}

namespace tmog::ui {

class LineIcon;
class NavDelegate;
class PerformancePage;
class SummaryPage;

struct Destination
{
    QString id;
    QString title;
    QString subtitle;
    QString note;
    QString group;
    QString iconKey;
    QColor accent;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Settings &settings, QWidget *parent = nullptr);

private:
    void buildDestinations();
    void buildSidebar();
    void buildPageHeader();
    void buildMenus();
    void buildStatusBar();
    void buildPages();

    void updatePageHeader();
    void showAppMenu();
    void sampleNow();
    void updateProviderStatus(const MetricsSnapshot &snapshot);
    void navigateTo(int destinationIndex);
    void openPalette();
    void openSettings();
    void showAbout();
    void applyAppearance(const QString &themeId, int textScalePercent);
    void updateThemeChecks();

    Settings &m_settings;
    QVector<Destination> m_destinations;
    QWidget *m_sidebar = nullptr;
    QListWidget *m_nav = nullptr;
    NavDelegate *m_navDelegate = nullptr;
    QWidget *m_pageHeader = nullptr;
    LineIcon *m_pageIcon = nullptr;
    QLabel *m_pageTitle = nullptr;
    QToolButton *m_menuButton = nullptr;
    QStackedWidget *m_pages = nullptr;
    QActionGroup *m_themeGroup = nullptr;
    QActionGroup *m_scaleGroup = nullptr;
    QLabel *m_statusDot = nullptr;
    QLabel *m_statusLeft = nullptr;
    QLabel *m_statusRight = nullptr;
    SummaryPage *m_summaryPage = nullptr;
    PerformancePage *m_performancePage = nullptr;
    MetricsSampler m_sampler;
    MetricsSnapshot m_lastSnapshot;
    QTimer *m_sampleTimer = nullptr;
    QString m_themeId;
    int m_textScale = 100;
    int m_refreshMs = 1000;
    bool m_hasSnapshot = false;
};

} // namespace tmog::ui
