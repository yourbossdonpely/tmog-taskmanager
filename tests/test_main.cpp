#include "core/settings.h"
#include "core/version.h"
#include "ui/theme.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include <cstdio>

namespace {

int failures = 0;

void check(bool condition, const char *expression, const char *file, int line)
{
    if (condition) {
        return;
    }
    std::fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expression);
    ++failures;
}

#define CHECK(expression) check((expression), #expression, __FILE__, __LINE__)

void testSettingsRoundTrip(const QString &directory)
{
    const QString path = directory + QStringLiteral("/settings.json");

    tmog::Settings settings(path);
    CHECK(settings.getString(QStringLiteral("appearance/theme")) == QStringLiteral("dark"));
    CHECK(settings.getInt(QStringLiteral("appearance/text_scale")) == 100);
    CHECK(settings.getInt(QStringLiteral("sampling/refresh_ms")) == 1000);
    CHECK(settings.getBool(QStringLiteral("navigation/collapsed")) == false);

    settings.set(QStringLiteral("appearance/theme"), QStringLiteral("green"));
    settings.set(QStringLiteral("appearance/text_scale"), 150);
    settings.set(QStringLiteral("custom/future_key"),
                 QJsonObject{{QStringLiteral("keep"), true}});
    CHECK(settings.save());
    CHECK(QFile::exists(path));

    tmog::Settings loaded(path);
    CHECK(loaded.load());
    CHECK(loaded.getString(QStringLiteral("appearance/theme")) == QStringLiteral("green"));
    CHECK(loaded.getInt(QStringLiteral("appearance/text_scale")) == 150);
    CHECK(loaded.getBool(QStringLiteral("custom/future_key/keep")) == false);
    CHECK(loaded.getString(QStringLiteral("sampling/refresh_ms")).isEmpty());
    CHECK(loaded.getInt(QStringLiteral("sampling/refresh_ms")) == 1000);
}

void testCorruptSettingsDoNotCrash(const QString &directory)
{
    const QString path = directory + QStringLiteral("/corrupt.json");
    QFile file(path);
    CHECK(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream(&file) << "this is not json";
    file.close();

    tmog::Settings settings(path);
    CHECK(!settings.load());
    CHECK(settings.getString(QStringLiteral("appearance/theme")) == QStringLiteral("dark"));
}

void testThemes()
{
    const auto &all = tmog::ui::themes();
    CHECK(all.size() == 6);
    CHECK(tmog::ui::themeIds().size() == 6);
    CHECK(tmog::ui::themeNames().size() == 6);
    CHECK(tmog::ui::themeById(QStringLiteral("missing")).id == QStringLiteral("dark"));

    const QString sheet = tmog::ui::styleSheet(tmog::ui::themeById(QStringLiteral("dark")), 100);
    CHECK(sheet.contains(QStringLiteral("#0b0e13")));
    CHECK(sheet.contains(QStringLiteral("13px")));
    CHECK(sheet.contains(QStringLiteral("#5b8cff")));
    CHECK(!sheet.contains(QStringLiteral("@surface")));

    const QString large = tmog::ui::styleSheet(tmog::ui::themeById(QStringLiteral("clear")), 200);
    CHECK(large.contains(QStringLiteral("26px")));
    CHECK(tmog::ui::themeById(QStringLiteral("clear")).id == QStringLiteral("dark"));
}

void testIdentity()
{
    CHECK(tmog::productName() == QStringLiteral("Task Manager OG by Don PELY"));
    CHECK(!tmog::version().isEmpty());
    CHECK(tmog::versionLine().startsWith(tmog::productName()));
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QTemporaryDir temporary;
    CHECK(temporary.isValid());
    if (temporary.isValid()) {
        testSettingsRoundTrip(temporary.path());
        testCorruptSettingsDoNotCrash(temporary.path());
    }

    testThemes();
    testIdentity();

    if (failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    std::printf("All checks passed\n");
    return 0;
}
