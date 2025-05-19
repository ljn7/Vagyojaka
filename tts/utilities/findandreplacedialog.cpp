#include "findandreplacedialog.h"
#include <QTableView>
#include <QAbstractItemModel>
#include <QMessageBox>
#include "ui_findandreplacedialog.h"

FindAndReplaceDialog::FindAndReplaceDialog(QTableView *tableView, QWidget *parent)
    : QDialog(parent),
    m_TableView(tableView),
    ui(new Ui::FindAndReplaceDialog)
{
    ui->setupUi(this);
    connect(ui->button_find_next, &QPushButton::clicked, this, &FindAndReplaceDialog::findNext);
    connect(ui->button_find_previous, &QPushButton::clicked, this, &FindAndReplaceDialog::findPrevious);
    connect(ui->button_replace, &QPushButton::clicked, this, &FindAndReplaceDialog::replace);
    connect(ui->button_replace_all, &QPushButton::clicked, this, &FindAndReplaceDialog::replaceAll);
    connect(ui->whole_words, &QCheckBox::toggled, this, &FindAndReplaceDialog::updateFlags);
    connect(ui->case_sensitive, &QCheckBox::toggled, this, &FindAndReplaceDialog::updateFlags);

    // Set default flags
    flags = Qt::MatchFlags();

    // Set focus to find text field
    ui->text_find->setFocus();
}

FindAndReplaceDialog::~FindAndReplaceDialog()
{
    delete ui;
}

void FindAndReplaceDialog::updateFlags()
{
    Qt::MatchFlags tmp;
    if (ui->whole_words->isChecked())
        tmp = tmp | Qt::MatchExactly;
    if (ui->case_sensitive->isChecked())
        tmp = tmp | Qt::MatchCaseSensitive;
    flags = tmp;
}

void FindAndReplaceDialog::findNext()
{
    QString query = ui->text_find->text();
    if (query.isEmpty()) {
        emit message("Please enter text to search for.");
        return;
    }

    QAbstractItemModel *model = m_TableView->model();
    QModelIndex startIndex = m_TableView->currentIndex();

    // Start searching from the current index
    int startRow = startIndex.isValid() ? startIndex.row() : 0;
    int startCol = startIndex.isValid() ? startIndex.column() + 1 : 0;

    for (int row = startRow; row < model->rowCount(); ++row) {
        for (int col = startCol; col < model->columnCount(); ++col) {
            QModelIndex index = model->index(row, col);
            QString cellText = model->data(index, Qt::DisplayRole).toString();
            if (cellText.contains(query, ui->case_sensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
                m_TableView->setCurrentIndex(index);
                m_TableView->scrollTo(index);
                emit message("Found \"" + query + "\" at row " + QString::number(row + 1) + ", column " + QString::number(col + 1) + ".");
                return;
            }
        }
        startCol = 0; // Reset column for next rows
    }

    // If not found, wrap around to the beginning
    emit message("Cannot find \"" + query + "\". Wrapping to the beginning.");
    // m_TableView->setCurrentIndex(QModelIndex()); // Reset to start
    // findNext();
}

void FindAndReplaceDialog::findPrevious()
{
    QString query = ui->text_find->text();
    if (query.isEmpty()) {
        emit message("Please enter text to search for.");
        return;
    }

    QAbstractItemModel *model = m_TableView->model();
    QModelIndex startIndex = m_TableView->currentIndex();

    // Start searching from the current index
    int startRow = startIndex.isValid() ? startIndex.row() : model->rowCount() - 1;
    int startCol = startIndex.isValid() ? startIndex.column() - 1 : model->columnCount() - 1;

    for (int row = startRow; row >= 0; --row) {
        for (int col = startCol; col >= 0; --col) {
            QModelIndex index = model->index(row, col);
            QString cellText = model->data(index, Qt::DisplayRole).toString();

            if (cellText.contains(query, ui->case_sensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
                m_TableView->setCurrentIndex(index);
                m_TableView->scrollTo(index);
                emit message("Found \"" + query + "\" at row " + QString::number(row + 1) + ", column " + QString::number(col + 1) + ".");
                return;
            }
        }
        startCol = model->columnCount() - 1; // Reset column for previous rows
    }

    // If not found, wrap around to the end
    emit message("Cannot find \"" + query + "\". Wrapping to the end.");
    // m_TableView->setCurrentIndex(model->index(model->rowCount() - 1, model->columnCount() - 1)); // Reset to end
    // findPrevious();
}

void FindAndReplaceDialog::replace()
{
    QString findText = ui->text_find->text();
    QString replaceText = ui->text_replace->text();

    if (findText.isEmpty()) {
        emit message("Please enter text to search for.");
        return;
    }

    QModelIndex currentIndex = m_TableView->currentIndex();
    if (currentIndex.isValid()) {
        QString cellText = m_TableView->model()->data(currentIndex, Qt::DisplayRole).toString();
        if (cellText.contains(findText, ui->case_sensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
            QString newText = cellText.replace(findText, replaceText);
            m_TableView->model()->setData(currentIndex, newText, Qt::EditRole);
            emit message("Replaced \"" + findText + "\" with \"" + replaceText + "\".");
            findNext(); // Move to next occurrence
        } else {
            findNext(); // Find first occurrence
        }
    } else {
        findNext(); // Find first occurrence
    }
}

void FindAndReplaceDialog::replaceAll()
{
    QString findText = ui->text_find->text();
    QString replaceText = ui->text_replace->text();

    if (findText.isEmpty()) {
        emit message("Please enter text to search for.");
        return;
    }

    QAbstractItemModel *model = m_TableView->model();
    int count = 0;

    for (int row = 0; row < model->rowCount(); ++row) {
        for (int col = 0; col < model->columnCount(); ++col) {
            QModelIndex index = model->index(row, col);
            QString cellText = model->data(index, Qt::DisplayRole).toString();

            if (cellText.contains(findText, ui->case_sensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
                QString newText = cellText.replace(findText, replaceText);
                model->setData(index, newText, Qt::EditRole);
                count++;
            }
        }
    }

    emit message("Replaced " + QString::number(count) + " occurrence(s).");
}
