#ifndef FINDREPLACEDIALOG_H
#define FINDREPLACEDIALOG_H

#include <QDialog>
#include <QTableView>
#include <QAbstractItemModel>
#include <QMessageBox>

namespace Ui {
class FindAndReplaceDialog;
}

class FindAndReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindAndReplaceDialog(QTableView *tableView, QWidget *parent = nullptr);
    ~FindAndReplaceDialog();

signals:
    void message(const QString &msg); // Signal to send messages to the UI

private slots:
    void findNext(); // Find the next occurrence
    void findPrevious(); // Find the previous occurrence
    void replace(); // Replace the current occurrence
    void replaceAll(); // Replace all occurrences
    void updateFlags(); // Update search flags based on UI options

private:
    Ui::FindAndReplaceDialog *ui; // UI pointer
    QTableView *m_TableView; // Pointer to the table view to search in
    Qt::MatchFlags flags; // Flags for search (case sensitivity, whole words, etc.)
};

#endif // FINDREPLACEDIALOG_H
