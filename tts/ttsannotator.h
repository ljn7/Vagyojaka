#pragma once

#include "qitemselectionmodel.h"
#include "tts/customdelegates.h"
#include "tts/lazyloadingmodel.h"
#include <QWidget>
#include <QUrl>
#include <QSettings>
#include <memory>
#include <qundostack.h>

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
    void openFindReplaceDialog();
    void useTransliteration(bool flag, const QString& langCode = "en");
    QUndoStack* undoStack() const { return m_undoStack.get(); }
    void undo();
    void redo();
    bool hasUnsavedChanges() const { return m_undoStack && !m_undoStack->isClean(); }
    void save();
    void useAutoSave(bool value) {m_autoSave = value;}

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
    void onUndo();
    void onRedo();

protected:
    // void keyPressEvent(QKeyEvent* event) override;

private:
    void parseXML();
    void setupUI();
    void saveAs();
    void saveToFile(const QString& fileName);
    void insertRow();
    void deleteRow();
    void setDefaultFontOnTableView();
    void setupShortcuts();
    void toggleCurrentAudioPlayer();
    std::unique_ptr<QUndoStack> m_undoStack;

    Ui::TTSAnnotator* ui;
    std::unique_ptr<LazyLoadingModel> m_model;
    QUrl fileUrl;
    QString xmlDirectory;
    std::unique_ptr<QSettings> settings = nullptr;
    QStringList supportedFormats;
    AudioPlayerDelegate* m_audioPlayerDelegate = nullptr;
    bool m_autoSave {false};
    int m_saveInterval {10};
    QTimer* m_saveTimer = nullptr;
};
