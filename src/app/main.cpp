#include "core/settings.h"
#include "core/version.h"
#include "ui/main_window.h"
#include "ui/theme.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include <cstdio>
#include <cstring>

#ifdef Q_OS_WIN
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace {

bool hasArgument(int argc, char **argv, const char *name)
{
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], name) == 0) {
            return true;
        }
    }
    return false;
}

void attachParentConsole()
{
#ifdef Q_OS_WIN
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        return;
    }
#  ifdef _MSC_VER
    FILE *stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    freopen_s(&stream, "CONIN$", "r", stdin);
#  endif
#endif
}

void writeOutput(const char *text)
{
#ifdef Q_OS_WIN
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        attachParentConsole();
        handle = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(handle, text, static_cast<DWORD>(std::strlen(text)), &written, nullptr);
        return;
    }
#endif
    std::fputs(text, stdout);
}

void printUsage()
{
    const QByteArray product = tmog::productName().toLocal8Bit();
    writeOutput(product.constData());
    writeOutput("\n\nUsage: TaskManagerOG [options]\n\n");
    writeOutput("Options:\n");
    writeOutput("  --version              Print the product name and version, then exit\n");
    writeOutput("  --smoke-gui            Build and render the window, then exit\n");
    writeOutput("  --screenshot <path>    Render the window to a PNG and exit\n");
    writeOutput("  --help                 Show this help\n");
}

} // namespace

int main(int argc, char **argv)
{
    if (hasArgument(argc, argv, "--version") || hasArgument(argc, argv, "--help")) {
        attachParentConsole();
        if (hasArgument(argc, argv, "--version")) {
            writeOutput(tmog::versionLine().toLocal8Bit().constData());
            writeOutput("\n");
        } else {
            printUsage();
        }
        return 0;
    }

    const bool screenshotRequested = hasArgument(argc, argv, "--screenshot");
    if (screenshotRequested) {
        QCoreApplication::setAttribute(Qt::AA_Use96Dpi);
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("TaskManagerOG"));
    QApplication::setApplicationDisplayName(tmog::productName());
    QApplication::setOrganizationName(QStringLiteral("Don Pely"));
    QApplication::setOrganizationDomain(QStringLiteral("donpely.com"));
    QApplication::setApplicationVersion(tmog::version());

    QCommandLineParser parser;
    parser.setApplicationDescription(tmog::productName());
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption smokeOption(QStringLiteral("smoke-gui"),
                                   QStringLiteral("Render the window and exit."));
    QCommandLineOption screenshotOption(QStringLiteral("screenshot"),
                                        QStringLiteral("Render the window to a PNG and exit."),
                                        QStringLiteral("path"));
    parser.addOption(smokeOption);
    parser.addOption(screenshotOption);
    parser.process(app);

    tmog::Settings settings;
    settings.load();

    tmog::ui::MainWindow window(settings);
    if (parser.isSet(screenshotOption)) {
        window.setFixedSize(1800, 1156);
    }
    window.show();

    if (parser.isSet(screenshotOption)) {
        const QString path = QFileInfo(parser.value(screenshotOption)).absoluteFilePath();
        QDir().mkpath(QFileInfo(path).absolutePath());
        QTimer::singleShot(1600, &app, [&window, path]() {
            window.grab().save(path, "PNG");
            QCoreApplication::quit();
        });
    } else if (parser.isSet(smokeOption)) {
        QTimer::singleShot(400, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
