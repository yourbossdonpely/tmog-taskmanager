#pragma once

#include <QDialog>

class QComboBox;

namespace tmog::ui {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(const QString &themeId, int textScalePercent, QWidget *parent = nullptr);

    QString selectedTheme() const;
    int selectedTextScale() const;

private:
    QComboBox *m_theme = nullptr;
    QComboBox *m_scale = nullptr;
};

} // namespace tmog::ui
