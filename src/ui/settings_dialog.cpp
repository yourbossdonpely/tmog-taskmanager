#include "ui/settings_dialog.h"

#include "ui/theme.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace tmog::ui {

SettingsDialog::SettingsDialog(const QString &themeId, int textScalePercent, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Settings"));
    setModal(true);
    setMinimumWidth(380);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    auto *title = new QLabel(QStringLiteral("Appearance"), this);
    title->setObjectName(QStringLiteral("CardTitle"));
    layout->addWidget(title);

    auto *form = new QFormLayout();
    form->setSpacing(10);

    m_theme = new QComboBox(this);
    m_theme->addItems(themeNames());
    const int themeIndex = themeIds().indexOf(themeId);
    m_theme->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);
    form->addRow(QStringLiteral("Theme"), m_theme);

    m_scale = new QComboBox(this);
    const int scales[] = {100, 125, 150, 175, 200};
    for (int scale : scales) {
        m_scale->addItem(QStringLiteral("%1%").arg(scale), scale);
    }
    int scaleIndex = m_scale->findData(textScalePercent);
    if (scaleIndex < 0) {
        scaleIndex = 0;
    }
    m_scale->setCurrentIndex(scaleIndex);
    form->addRow(QStringLiteral("Text size"), m_scale);

    layout->addLayout(form);

    auto *note = new QLabel(
        QStringLiteral("Theme and text size are stored locally and applied immediately."),
        this);
    note->setObjectName(QStringLiteral("CardBody"));
    note->setWordWrap(true);
    layout->addWidget(note);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

QString SettingsDialog::selectedTheme() const
{
    const int index = m_theme->currentIndex();
    return themeIds().value(index, QStringLiteral("dark"));
}

int SettingsDialog::selectedTextScale() const
{
    return m_scale->currentData().toInt();
}

} // namespace tmog::ui
