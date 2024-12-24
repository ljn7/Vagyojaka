#pragma once

#include "qitemselectionmodel.h"
#include "tts/customdelegates.h"
#include "tts/lazyloadingmodel.h"
#include "ui_ttsannotator.h"
#include <QWidget>
#include <QUrl>
#include <QSettings>
#include <memory>
#include <algorithm>

namespace Ui {
class TTSAnnotator;
}

class LazyLoadingModel;
class QTableView;

class TTSAnnotator : public QWidget
{
    Q_OBJECT

public:
    explicit TTSAnnotator(QWidget *parent = nullptr);
    ~TTSAnnotator();
    void openTTSTranscript();

    static const QColor SoundQualityColor;
    static const QColor TTSQualityColor;
    QTableView* tableView;
    TextEditDelegate* textDelegate = nullptr;

signals:
    void transcriptWordsEdited(int64_t wordCount);
    void mispronunciationWordsEdited(int64_t wordCount);
    void tagWordsEdited(int64_t wordCount);
    void openMessage(QString text);

private slots:
    void on_saveAsTableButton_clicked();
    void on_InsertRowButton_clicked();
    void on_deleteRowButton_clicked();
    void on_saveTableButton_clicked();
    void on_actionOpen_triggered();
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onCellClicked(const QModelIndex &index);
    void onItemSelectionChanged();
    void onHeaderResized(int logicalIndex, int oldSize, int newSize);

public slots:
    void updateTranscriptEditedWords(int64_t count) {
        totalTranscriptEditedWords = std::max(0LL, totalTranscriptEditedWords + count);
        ui->totalTranscriptEditedWordsLbl->setText(
            QString("Total Edited Transcript words: %1").arg(totalTranscriptEditedWords));
    }

    void updateMispronunciationEditedWords(int64_t count) {
        totalMispronouncedEditedWords = std::max(0LL, totalMispronouncedEditedWords + count);
        ui->totalMispronouncedEditedWordsLbl->setText(
            QString("Total Edited Mispronounced words: %1").arg(totalMispronouncedEditedWords));
    }

    void updateTagEditedWords(int64_t count) {
        totalTaggedEditedWords = std::max(0LL, totalTaggedEditedWords + count);
        ui->totalTaggedEditedWordsLbl->setText(
            QString("Total Edited Tagged words: %1").arg(totalTaggedEditedWords));
    }

public slots:
    void updateEditedWordCounts() {
        if (!m_model) return;

        // Calculate edits for each column
        totalTranscriptEditedWords += m_model->calculateColumnEditedWords(1);
        totalMispronouncedEditedWords += m_model->calculateColumnEditedWords(2);
        totalTaggedEditedWords += m_model->calculateColumnEditedWords(3);

        // Update the labels
        ui->totalTranscriptEditedWordsLbl->setText(
            QString("Total Edited Transcript words: %1").arg(totalTranscriptEditedWords));
        ui->totalMispronouncedEditedWordsLbl->setText(
            QString("Total Edited Mispronounced words: %1").arg(totalMispronouncedEditedWords));
        ui->totalTaggedEditedWordsLbl->setText(
            QString("Total Edited Tagged words: %1").arg(totalTaggedEditedWords));
    }

private:
    void parseXML();
    void setupUI();
    void save();
    void saveAs();
    void saveToFile(const QString& fileName);
    void insertRow();
    void deleteRow();
    void setDefaultFontOnTableView();
    QString getWordCountFilename();
    void saveWordCounts();
    void loadWordCounts();

    Ui::TTSAnnotator* ui;
    std::unique_ptr<LazyLoadingModel> m_model;
    QUrl fileUrl;
    QString xmlDirectory;
    std::unique_ptr<QSettings> settings = nullptr;
    QStringList supportedFormats;
    AudioPlayerDelegate* m_audioPlayerDelegate = nullptr;
    int64_t totalTranscriptEditedWords = 0;
    int64_t totalMispronouncedEditedWords = 0;
    int64_t totalTaggedEditedWords = 0;
    int64_t totalTranscriptWords = 0;
    int64_t totalMispronouncedWords = 0;
    int64_t totalTaggedWords = 0;
};
