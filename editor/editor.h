#pragma once

#include "blockandword.h"
#include "nuspell/dictionary.hxx"
#include "texteditor.h"
#include "wordeditor.h"
#include "utilities/changespeakerdialog.h"
#include "utilities/timepropagationdialog.h"
#include "utilities/tagselectiondialog.h"

#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QCompleter>
#include <QAbstractItemModel>
#include <qcompleter.h>
#include <set>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QUndoCommand>
#include <QSettings>
class Highlighter;

class Editor : public TextEditor
{
    Q_OBJECT

public:
    explicit Editor(QWidget *parent = nullptr);

    void setWordEditor(WordEditor* wordEditor)
    {
        m_wordEditor = wordEditor;
        connect(m_wordEditor, &QTableWidget::itemChanged, this, &Editor::wordEditorChanged);
    }

    void setEditorFont(const QFont& font);

    QRegularExpression timeStampExp, speakerExp;
    void setMoveAlongTimeStamps();
    void setShowTimeStamp();
    QVector<block> m_blocks;
    QUrl m_transcriptUrl;
    bool showTimeStamp=false;
    void loadTranscriptData(QFile& file);
    void setContent();
    bool timestampVisibility();
    QList<QTime> getTimeStamps();
    friend class Highlighter;
    void loadTranscriptFromUrl(QUrl* fileUrl);

    void showWaveform();

    //    QUndoStack *undoStack=nullptr;
protected:
    void mousePressEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

signals:
    void jumpToPlayer(const QTime& time);
    void refreshTagList(const QStringList& tagList);
    void replyCame();
    void openMessage(const QString& text);

    void sendBlockTime(QVector<QTime> timeArray);
    void sendBlockText(QString blockText);

public slots:
    void transcriptOpen();
    void transcriptSave();
    void transcriptSaveAs();
    void transcriptClose();
    void highlightTranscript(const QTime& elapsedTime);

    void addCustomDictonary();

    void showBlocksFromData();
    void jumpToHighlightedLine();
    void splitLine(const QTime& elapsedTime);
    void mergeUp();
    void mergeDown();
    void createChangeSpeakerDialog();
    void createTimePropagationDialog();
    void createTagSelectionDialog();
    void insertTimeStamp(const QTime& elapsedTime);
    void changeTranscriptLang();

    void speakerWiseJump(const QString& jumpDirection);
    void wordWiseJump(const QString& jumpDirection);
    void blockWiseJump(const QString& jumpDirection);

    void useTransliteration(bool value, const QString& langCode = "en");
    void useAutoSave(bool value) {m_autoSave = value;}
    void suggest(QString suggest);
    void realTimeDataSavingToggle();
    void saveAsPDF();
    void saveAsTXT();

    void updateTimeStamp(int block_num, QTime endTime);
    void updateTimeStampsBlock(QVector<int> blks);

private slots:
    void contentChanged(int position, int charsRemoved, int charsAdded);
    void wordEditorChanged();

    void updateWordEditor();

    void changeSpeaker(const QString& newSpeaker, bool replaceAllOccurrences);
    void propagateTime(const QTime& time, int start, int end, bool negateTime);
    void selectTags(const QStringList& newTagList);
    void markWordAsCorrect(int blockNumber, int wordNumber);

    void insertSpeakerCompletion(const QString& completion);
    void insertTextCompletion(const QString& completion);
    void insertTransliterationCompletion(const QString &completion);

    void handleReply();
    void sendRequest(const QString& input, const QString& langCode);

private:
    static QTime getTime(const QString& text);
    static word makeWord(const QTime& t, const QString& s, const QStringList& tagList, const QString& isEdited);
    QCompleter* makeCompleter();
    void processContentChanges();

    void saveXml(QFile* file);
    void helpJumpToPlayer();
    void loadDictionary();
    bool copyFileToPersistentLocation(const QString &resourcePath, const QString &destinationPath);


    block fromEditor(qint64 blockNumber) const;
    static QStringList listFromFile(const QString& fileName) ;

    bool settingContent{false}, updatingWordEditor{false}, dontUpdateWordEditor{false};
    bool m_transliterate{false}, m_autoSave{false};


    QString m_transcriptLang, m_punctuation{",.!;:?"};

    Highlighter* m_highlighter = nullptr;
    qint64 highlightedBlock = -1, highlightedWord = -1;
    WordEditor* m_wordEditor = nullptr;
    ChangeSpeakerDialog* m_changeSpeaker = nullptr;
    TimePropagationDialog* m_propagateTime = nullptr;
    TagSelectionDialog* m_selectTag = nullptr;
    QCompleter *m_speakerCompleter = nullptr, *m_textCompleter = nullptr, *m_transliterationCompleter = nullptr;
    QStringList m_dictionary;
    QString m_customDictonaryPath=NULL;
    std::set<QString> m_correctedWords;
    QString m_transliterateLangCode;
    QStringList m_lastReplyList;
    QNetworkAccessManager m_manager;
    QNetworkReply* m_reply = nullptr;
    QTimer* m_saveTimer = nullptr;
    int m_saveInterval{20};

    QStringList clipboardTexts;
    QString fileBeforeSave="initial.txt";
    QString fileAfterSave="final.txt";
    QString ComparedOutputFile="ComparedDictonary.json";
    bool realTimeDataSaver=false;
    QStringList allClips;
    int lastHighlightedBlock=-1;
    bool moveAlongTimeStamps=true;
    QStringList supportedFormats;
    QTimer debounceTimer;
    QSettings* settings;
    nuspell::Dictionary dictionary;

public:

};




class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit Highlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {};

    void clearHighlight()
    {
        blockToHighlight = -1;
        wordToHighlight = -1;
    }
    void setBlockToHighlight(qint64 blockNumber)
    {
        blockToHighlight = blockNumber;
        rehighlight();
    }
    void setWordToHighlight(int wordNumber)
    {
        wordToHighlight = wordNumber;
        rehighlight();
    }
    void setInvalidBlocks(const QList<int>& invalidBlocks)
    {
        invalidBlockNumbers = invalidBlocks;
        rehighlight();
    }
    void setTaggedBlocks(const QList<int>& taggedBlock)
    {
        taggedBlockNumbers = taggedBlock;
        rehighlight();
    }
    void clearTaggedBlocks()
    {
        taggedBlockNumbers.clear();
    }
    void setInvalidWords(const QMultiMap<int, int>& invalidWordsMap)
    {
        invalidWords = invalidWordsMap;
        rehighlight();
    }
    void setTaggedWords(const QMultiMap<int, int>& taggedWordsMap)
    {
        taggedWords = taggedWordsMap;
        rehighlight();
    }
    void setEditedWords(const QMultiMap<int, int>& editedWordsMap) {
        editedWords = editedWordsMap;
        rehighlight();
    }
    void clearInvalidBlocks()
    {
        invalidBlockNumbers.clear();
    }

    void highlightBlock(const QString&) override;


private:
    int blockToHighlight{-1};
    int wordToHighlight{-1};
    QList<int> invalidBlockNumbers;
    QList<int> taggedBlockNumbers;

    QMultiMap<int, int> invalidWords;
    QMultiMap<int, int> taggedWords;
    QMultiMap<int, int> editedWords;
};
