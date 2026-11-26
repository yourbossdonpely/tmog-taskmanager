#include "core/settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

namespace tmog {

namespace {
constexpr auto kTheme = "appearance/theme";
constexpr auto kTextScale = "appearance/text_scale";
constexpr auto kRefreshMs = "sampling/refresh_ms";
constexpr auto kStartDestination = "navigation/start_destination";
constexpr auto kLastDestination = "navigation/last_destination";
constexpr auto kNavCollapsed = "navigation/collapsed";
constexpr auto kReducedMotion = "appearance/reduced_motion";
} // namespace

Settings::Settings(QString filePath)
    : m_filePath(std::move(filePath))
{
    setDefaults();
}

QString Settings::defaultFilePath()
{
    if (isPortable()) {
        return QCoreApplication::applicationDirPath() + QStringLiteral("/data/settings.json");
    }

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + QStringLiteral("/.config/task-manager-og");
    }
    return dir + QStringLiteral("/settings.json");
}

bool Settings::isPortable()
{
    if (QCoreApplication::applicationDirPath().isEmpty()) {
        return false;
    }
    return QFileInfo::exists(QCoreApplication::applicationDirPath()
                             + QStringLiteral("/portable.flag"));
}

void Settings::setDefaults()
{
    m_values.insert(QString::fromLatin1(kTheme), QStringLiteral("dark"));
    m_values.insert(QString::fromLatin1(kTextScale), 100);
    m_values.insert(QString::fromLatin1(kRefreshMs), 1000);
    m_values.insert(QString::fromLatin1(kStartDestination), QStringLiteral("summary"));
    m_values.insert(QString::fromLatin1(kLastDestination), QStringLiteral("summary"));
    m_values.insert(QString::fromLatin1(kNavCollapsed), false);
    m_values.insert(QString::fromLatin1(kReducedMotion), false);
}

bool Settings::load()
{
    QFile file(m_filePath);
    if (!file.exists()) {
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }

    const QJsonObject stored = document.object();
    for (auto it = stored.constBegin(); it != stored.constEnd(); ++it) {
        m_values.insert(it.key(), it.value());
    }
    return true;
}

bool Settings::save() const
{
    const QFileInfo info(m_filePath);
    if (!QDir().mkpath(info.absolutePath())) {
        return false;
    }

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    const QJsonDocument document(m_values);
    if (file.write(document.toJson(QJsonDocument::Indented)) < 0) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

QString Settings::getString(const QString &key, const QString &fallback) const
{
    const QJsonValue value = m_values.value(key);
    return value.isString() ? value.toString() : fallback;
}

int Settings::getInt(const QString &key, int fallback) const
{
    const QJsonValue value = m_values.value(key);
    return value.isDouble() ? value.toInt(fallback) : fallback;
}

double Settings::getDouble(const QString &key, double fallback) const
{
    const QJsonValue value = m_values.value(key);
    return value.isDouble() ? value.toDouble(fallback) : fallback;
}

bool Settings::getBool(const QString &key, bool fallback) const
{
    const QJsonValue value = m_values.value(key);
    return value.isBool() ? value.toBool() : fallback;
}

void Settings::set(const QString &key, const QJsonValue &value)
{
    m_values.insert(key, value);
}

} // namespace tmog
