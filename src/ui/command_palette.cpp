#include "ui/command_palette.h"

#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

namespace tmog::ui {

CommandPalette::CommandPalette(QStringList entries, QWidget *parent)
    : QDialog(parent)
    , m_entries(std::move(entries))
{
    setWindowTitle(QStringLiteral("Go to"));
    setModal(true);
    resize(460, 320);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(QStringLiteral("Jump to a destination…"));
    m_search->setClearButtonEnabled(true);
    layout->addWidget(m_search);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("Nav"));
    for (int index = 0; index < m_entries.size(); ++index) {
        auto *item = new QListWidgetItem(m_entries.at(index), m_list);
        item->setData(Qt::UserRole, index);
    }
    layout->addWidget(m_list, 1);

    connect(m_search, &QLineEdit::textChanged, this, &CommandPalette::filter);
    connect(m_list, &QListWidget::itemActivated, this, &QDialog::accept);
    connect(m_list, &QListWidget::itemClicked, this, &QDialog::accept);

    m_search->setFocus();
}

int CommandPalette::selectedIndex() const
{
    const auto *item = m_list->currentItem();
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void CommandPalette::filter(const QString &text)
{
    const QString needle = text.trimmed();
    int firstVisible = -1;

    for (int row = 0; row < m_list->count(); ++row) {
        auto *item = m_list->item(row);
        const bool matches = needle.isEmpty()
            || item->text().contains(needle, Qt::CaseInsensitive);
        item->setHidden(!matches);
        if (matches && firstVisible < 0) {
            firstVisible = row;
        }
    }

    if (firstVisible >= 0) {
        m_list->setCurrentRow(firstVisible);
    }
}

} // namespace tmog::ui
