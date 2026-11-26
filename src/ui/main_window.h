#pragma once

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

namespace tmog {
class Settings;
}

namespace tmog::ui {

struct Destination
{
    QString id;
    QString title;
    QString subtitle;
    QString note;
    QString group;
    QColor accent;
    bool showcase = false;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Settings &settings, QWidget *parent = nullptr);

private:
    void buildDestinations();
    void buildSidebar();
    void buildMenus();
    void buildStatusBar();
    void buildPages();

    void navigateTo(int destinationIndex);
    void openPalette();
    void openSettings();
    void showAbout();
    void applyAppearance(const QString &themeId, int textScalePercent);
    void updateThemeChecks();

    Settings &m_settings;
    QVector<Destination> m_destinations;
    QListWidget *m_nav = nullptr;
    QStackedWidget *m_pages = nullptr;
    QActionGroup *m_themeGroup = nullptr;
    QActionGroup *m_scaleGroup = nullptr;
    QLabel *m_statusLeft = nullptr;
    QLabel *m_statusRight = nullptr;
    QString m_themeId;
    int m_textScale = 100;
};

} // namespace tmog::ui
