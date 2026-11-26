#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace tmog {

class Settings
{
public:
    explicit Settings(QString filePath = defaultFilePath());

    static QString defaultFilePath();
    static bool isPortable();

    bool load();
    bool save() const;

    QString filePath() const { return m_filePath; }

    QString getString(const QString &key, const QString &fallback = {}) const;
    int getInt(const QString &key, int fallback = 0) const;
    double getDouble(const QString &key, double fallback = 0.0) const;
    bool getBool(const QString &key, bool fallback = false) const;

    void set(const QString &key, const QJsonValue &value);
    void setDefaults();

private:
    QString m_filePath;
    QJsonObject m_values;
};

} // namespace tmog
