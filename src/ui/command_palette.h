#pragma once

#include <QDialog>
#include <QStringList>

class QLineEdit;
class QListWidget;

namespace tmog::ui {

class CommandPalette : public QDialog
{
    Q_OBJECT

public:
    explicit CommandPalette(QStringList entries, QWidget *parent = nullptr);

    int selectedIndex() const;

private:
    void filter(const QString &text);

    QStringList m_entries;
    QLineEdit *m_search = nullptr;
    QListWidget *m_list = nullptr;
};

} // namespace tmog::ui
