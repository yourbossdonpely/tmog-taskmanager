#include "core/metrics.h"
#include "core/metrics_parse.h"
#include "core/settings.h"
#include "core/version.h"
#include "ui/reading_style.h"
#include "ui/theme.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>

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

void testReadingStates()
{
    using tmog::Availability;
    using tmog::Reading;

    CHECK(tmog::ui::readingStateText(Reading::available(42.0)) == QStringLiteral("LIVE"));
    CHECK(tmog::ui::readingStateText(Reading::pending(QStringLiteral("first delta")))
          == QStringLiteral("SAMPLING"));
    CHECK(tmog::ui::readingStateText(Reading::unavailable(QStringLiteral("no sensor")))
          == QStringLiteral("UNAVAILABLE"));
    CHECK(tmog::ui::readingStateText(Reading::denied(QStringLiteral("no permission")))
          == QStringLiteral("DENIED"));

    const Reading pending = Reading::pending(QStringLiteral("first delta"));
    const Reading available = Reading::available(12.0);
    CHECK(tmog::ui::combineReadings(pending, available).state == Availability::Pending);
    CHECK(tmog::ui::combineReadings(available, pending).state == Availability::Pending);
    CHECK(tmog::ui::combineReadings(Reading::unavailable(QStringLiteral("failed")), pending).state
          == Availability::Unavailable);
    CHECK(tmog::ui::combineReadings(available, Reading::denied(QStringLiteral("denied"))).state
          == Availability::Denied);
}

void testMetricsMath()
{
    const auto half = tmog::percentage(50, 100);
    CHECK(half.has_value());
    CHECK(half && qAbs(*half - 50.0) < 0.001);
    CHECK(!tmog::percentage(1, 0).has_value());
    CHECK(tmog::percentage(200, 100).value() == 100.0);

    const auto rate = tmog::counterRate(2000, 1000, 2.0);
    CHECK(rate.has_value());
    CHECK(rate && qAbs(*rate - 500.0) < 0.001);
    CHECK(!tmog::counterRate(999, 1000, 1.0).has_value());
    CHECK(!tmog::counterRate(2000, 1000, 0.0).has_value());

    CHECK(tmog::formatBytes(0) == QStringLiteral("0 B"));
    CHECK(tmog::formatBytes(1023) == QStringLiteral("1023 B"));
    CHECK(tmog::formatBytes(1024) == QStringLiteral("1.0 KiB"));
    CHECK(tmog::formatBytes(1536) == QStringLiteral("1.5 KiB"));
    CHECK(tmog::formatBytes(1024ULL * 1024 * 1024) == QStringLiteral("1.0 GiB"));
    CHECK(tmog::formatRate(1536) == QStringLiteral("1.5 KiB/s"));
    CHECK(tmog::formatRate(0) == QStringLiteral("0 B/s"));
    CHECK(tmog::formatPercent(0) == QStringLiteral("0%"));
    CHECK(tmog::formatPercent(4.5) == QStringLiteral("4.5%"));
    CHECK(tmog::formatPercent(42.6) == QStringLiteral("43%"));
    CHECK(tmog::formatPercent(150) == QStringLiteral("100%"));
}

void testProcParsers()
{
    tmog::CpuTimes cpu;
    CHECK(tmog::parseProcStat(
        QByteArrayLiteral("cpu  100 10 50 800 30 5 5 0 0 0\n"
                          "cpu0 50 5 25 400 15 2 3 0 0 0\n"),
        cpu));
    CHECK(cpu.user == 100);
    CHECK(cpu.total() == 1000);
    CHECK(cpu.idleTotal() == 830);
    CHECK(cpu.busy() == 170);

    tmog::MemInfo memory;
    CHECK(tmog::parseMeminfo(
        QByteArrayLiteral("MemTotal: 1000 kB\n"
                          "MemFree: 100 kB\n"
                          "MemAvailable: 400 kB\n"
                          "Buffers: 50 kB\n"
                          "Cached: 150 kB\n"),
        memory));
    CHECK(memory.totalBytes == 1000ULL * 1024);
    CHECK(memory.availableBytes == 400ULL * 1024);

    tmog::MemInfo fallback;
    CHECK(tmog::parseMeminfo(
        QByteArrayLiteral("MemTotal: 1000 kB\nMemFree: 100 kB\nBuffers: 50 kB\nCached: 150 kB\n"),
        fallback));
    CHECK(fallback.availableBytes == 300ULL * 1024);

    tmog::NetCounters network;
    CHECK(tmog::parseNetDev(
        QByteArrayLiteral("Inter-|   Receive                                                |  Transmit\n"
                          " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
                          "    lo: 999 1 0 0 0 0 0 0 999 1 0 0 0 0 0 0\n"
                          "  eth0: 1000 10 0 0 0 0 0 0 2000 20 0 0 0 0 0 0\n"),
        network));
    CHECK(network.receiveBytes == 1000);
    CHECK(network.transmitBytes == 2000);

    tmog::NetCounters empty;
    CHECK(!tmog::parseNetDev(QByteArrayLiteral("no interfaces\n"), empty));
}

void testLiveSampler()
{
    tmog::MetricsSampler sampler;
    const tmog::MetricsSnapshot first = sampler.sample();
    CHECK(first.cpuPercent.state == tmog::Availability::Pending);
    CHECK(first.networkDownPerSecond.state != tmog::Availability::Available);
    CHECK(first.memoryPercent.state == tmog::Availability::Available);
    CHECK(first.storagePercent.state == tmog::Availability::Available);
    CHECK(first.memoryTotalBytes > 0);
    CHECK(first.storageTotalBytes > 0);
    CHECK(!first.storageName.isEmpty());

    QThread::msleep(250);
    const tmog::MetricsSnapshot second = sampler.sample();
    CHECK(second.cpuPercent.state == tmog::Availability::Available);
    CHECK(second.memoryPercent.state == tmog::Availability::Available);
    CHECK(second.storagePercent.state == tmog::Availability::Available);
    if (second.cpuPercent.hasValue()) {
        CHECK(second.cpuPercent.value >= 0.0 && second.cpuPercent.value <= 100.0);
    }
    if (second.memoryPercent.hasValue()) {
        CHECK(second.memoryPercent.value >= 0.0 && second.memoryPercent.value <= 100.0);
    }
    if (second.storagePercent.hasValue()) {
        CHECK(second.storagePercent.value >= 0.0 && second.storagePercent.value <= 100.0);
    }

    const auto checkDetail = [](const tmog::Reading &reading) {
        CHECK(reading.hasValue() || !reading.detail.isEmpty());
    };
    checkDetail(second.cpuPercent);
    checkDetail(second.memoryPercent);
    checkDetail(second.storagePercent);
    checkDetail(second.networkDownPerSecond);
    checkDetail(second.networkUpPerSecond);

    sampler.reset();
    const tmog::MetricsSnapshot afterReset = sampler.sample();
    CHECK(afterReset.cpuPercent.state == tmog::Availability::Pending);
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
    testReadingStates();
    testMetricsMath();
    testProcParsers();
    testLiveSampler();

    if (failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    std::printf("All checks passed\n");
    return 0;
}
