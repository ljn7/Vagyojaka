#pragma once

#include "qitemselectionmodel.h"
#include "tts/customdelegates.h"
#include <QWidget>
#include <QUrl>
#include <QSettings>
#include <memory>

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
    static const QColor CorrectColor;
    static const QColor WrongColor;
    QTableView* tableView;
    TextEditDelegate* textDelegate = nullptr;


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

private:
    void parseXML();
    void setupUI();
    void save();
    void saveAs();
    void saveToFile(const QString& fileName);
    void insertRow();
    void deleteRow();
    void setDefaultFontOnTableView();

    Ui::TTSAnnotator* ui;
    std::unique_ptr<LazyLoadingModel> m_model;
    QUrl fileUrl;
    QString xmlDirectory;
    std::unique_ptr<QSettings> settings = nullptr;
    QStringList supportedFormats;
    AudioPlayerDelegate* m_audioPlayerDelegate = nullptr;
};
