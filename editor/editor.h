#pragma once

#include "blockandword.h"
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

/**
 * @class Editor
 * @brief Manages the editing functionalities of transcripts, including
 *        time stamps, speaker changes, and file exports.
 */
class Editor : public TextEditor
{
    Q_OBJECT

public:

    /**
     * @brief Constructs an Editor instance.
     *
     * @param parent The parent widget.
     */
    explicit Editor(QWidget *parent = nullptr);

    /**
     * @brief Sets the word editor for the current Editor instance.
     *
     * This function connects the word editor's itemChanged signal
     * to the Editor's wordEditorChanged slot.
     *
     * @param wordEditor Pointer to the WordEditor instance.
     */
    void setWordEditor(WordEditor* wordEditor)
    {
        m_wordEditor = wordEditor;
        connect(m_wordEditor, &QTableWidget::itemChanged, this, &Editor::wordEditorChanged);
    }

    /**
     * @brief Sets the font for the editor.
     *
     * @param font The font to be set for the editor.
     */
    void setEditorFont(const QFont& font);

    QRegularExpression timeStampExp; ///< Regular expression for timestamps.
    QRegularExpression speakerExp; ///< Regular expression for speaker identification.


    /**
     * @brief Configures the editor to move along timestamps.
     */
    void setMoveAlongTimeStamps();

    /**
     * @brief Toggles the visibility of timestamps in the editor.
     */
    void setShowTimeStamp();

    QVector<block> m_blocks; ///< Stores the blocks of text and their associated data.
    QUrl m_transcriptUrl; ///< URL of the loaded transcript.
    bool showTimeStamp=false; ///< Flag indicating whether to show timestamps.

    /**
     * @brief Loads transcript data from a given file.
     *
     * @param file Reference to the QFile containing the transcript data.
     */
    void loadTranscriptData(QFile& file);

    /**
     * @brief Sets the content of the editor.
     */
    void setContent();

    /**
     * @brief Checks the visibility status of timestamps.
     *
     * @return True if timestamps are visible, otherwise false.
     */
    bool timestampVisibility();

    /**
     * @brief Retrieves timestamps from the blocks.
     *
     * This function collects and returns a list of timestamps from
     * all the blocks in the editor.
     *
     * @return QList<QTime> A list of timestamps from the blocks.
     */
    QList<QTime> getTimeStamps();

    friend class Highlighter; ///< Grants Highlighter access to private members.

    /**
     * @brief Loads transcript data from a given URL.
     *
     * @param fileUrl Pointer to the QUrl containing the transcript URL.
     */
    void loadTranscriptFromUrl(QUrl* fileUrl);

    /**
     * @brief Shows the waveform based on the current blocks.
     *
     * This function emits a signal with the timestamps of the blocks
     * to display the waveform associated with the transcript.
     */
    void showWaveform();

    //    QUndoStack *undoStack=nullptr;
protected:

    /**
     * @brief Handles mouse press events, enabling specific behavior with Ctrl key.
     *
     * Overrides the default QPlainTextEdit mouse press event to provide custom behavior.
     * If the Ctrl key is held down and there are blocks in `m_blocks`, the method invokes
     * `helpJumpToPlayer()`.
     *
     * @param e Pointer to the QMouseEvent containing details of the mouse press event.
     */
    void mousePressEvent(QMouseEvent *e) override;

    /**
     * @brief Handles key press events and provides custom shortcuts and suggestions.
     *
     * Intercepts keyboard input, performing specific actions for various Ctrl + Key shortcuts.
     * - **Ctrl+R**: Opens the Change Speaker dialog.
     * - **Ctrl+T**: Opens the Time Propagation dialog.
     * - **Ctrl+M**: Marks a word as correct if conditions are met.
     * - **Ctrl+I**: Centers and focuses on the cursor for doubtful words.
     *
     * Additionally, the method integrates multiple completers (text, speaker, and transliteration)
     * to assist with typing. Uses JSON to retrieve and suggest replacements, populating a popup menu
     * if suggestions are available.
     *
     * @param event Pointer to the QKeyEvent containing details of the key press event.
     */
    void keyPressEvent(QKeyEvent *event) override;

    /**
     * @brief Creates and displays a customized context menu with additional options.
     *
     * Enhances the default context menu with:
     * - "Mark As Correct" action: Marks the current word as correct if it’s under the cursor.
     * - Suggestions sub-menu: Reads from a JSON file (`replacedTextDictionary.json`) to offer word
     *   replacements based on the selected word's content.
     * - Clipboard sub-menu: Displays a list of clipboard items stored in `allClips` for easy reuse.
     *
     * @param event Pointer to the QContextMenuEvent containing details of the context menu event.
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

signals:

    /**
     * @brief Signal emitted to jump to a specific time in the \c MediaPlayer.
     *
     * @param time The QTime object representing the time to jump to in the \c MediaPlayer::setPositionToTime.
     */
    void jumpToPlayer(const QTime& time);

    /**
     * @brief Signal emitted to refresh the tag list display in the UI.
     *
     * @param tagList The updated list of tags to display.
     */
    void refreshTagList(const QStringList& tagList);

    /**
     * @brief Signal emitted when a reply is received from an external source.
     */
    void replyCame();

    /**
     * @brief Signal emitted to open a message box or display a message in the UI.
     *
     * @param text The text message to display.
     */
    void openMessage(const QString& text);

    /**
     * @brief Signal emitted to send a vector of block timestamps for display in the \c AudioWaveForm.
     *
     * @param timeArray A QVector of QTime objects representing the timestamps of each block.
     */
    void sendBlockTime(QVector<QTime> timeArray);

    /**
     * @brief Signal emitted to send the text content of a specific block.
     *
     * @param blockText The text content of the block to send.
     */
    void sendBlockText(QString blockText);

public slots:

    /**
     * @brief Opens a transcript file, allowing the user to select a file from the file dialog.
     *
     * Displays a file dialog to choose a transcript file and sets the directory based on user settings.
     * Loads the transcript from the selected URL.
     */
    void transcriptOpen();


    /**
     * @brief Saves the current transcript.
     *
     * If the transcript URL is empty, calls `transcriptSaveAs()` to save the file. Otherwise,
     * opens the existing file and saves the XML content.
     * Also creates necessary resources if they don't already exist.
     */
    void transcriptSave();

    /**
     * @brief Saves the current transcript with a new file name.
     *
     * Opens a file dialog to select a save location and file name, then saves the transcript as XML.
     * Ensures that the file has an `.xml` extension and updates the transcript URL.
     */
    void transcriptSaveAs();

    /**
     * @brief Closes the current transcript file.
     *
     * Clears the transcript URL, removes loaded blocks, sets the transcript language to English,
     * and reloads the dictionary to clear the document.
     */
    void transcriptClose();

    /**
     * @brief Highlights a transcript block and word based on the provided elapsed time.
     *
     * Finds the block and word that match the provided elapsed time and highlights them in the
     * transcript view. Sends the block text as a signal and updates the highlighter to display
     * the highlighted block and word.
     *
     * @param elapsedTime The time used to find and highlight the appropriate block and word.
     */
    void highlightTranscript(const QTime& elapsedTime);

    /**
     * @brief Opens a file dialog to add a custom dictionary.
     *
     * Opens a dialog to select a custom dictionary file (in .txt format). Sets the path and loads
     * the dictionary if the file is successfully opened.
     */
    void addCustomDictonary();

    /**
     * @brief Displays all blocks and their content in the QDebug (Saved in Logfile).
     * Iterates through each block and prints its timestamp, speaker, text, and associated tags,
     * followed by printing word details within each block.
     */
    void showBlocksFromData();

    /**
     * @brief Moves the cursor to the highlighted line in the editor.
     * If no block is highlighted, the function returns immediately.
     */
    void jumpToHighlightedLine();

    /**
     * @brief Splits the current text line at the cursor's position.
     * Divides the block into two based on cursor position, creating a new block and adjusting
     * the timestamp and word list of each. Updates the editor content accordingly.
     * @param elapsedTime The time to associate with the split point.
     */
    void splitLine(const QTime& elapsedTime);

    /**
     * @brief Merges the current block with the previous one, combining text and word lists.
     * Only merges if the speaker of the current and previous block match.
     */
    void mergeUp();

    /**
     * @brief Merges the current block with the next block, transferring word list and text.
     * Only merges if the speakers match for both blocks.
     */
    void mergeDown();

    /**
     * @brief Opens a dialog to select a new speaker for the highlighted block.
     * Displays a modal dialog with a list of existing speakers. Updates the selected
     * speaker in the block upon confirmation.
     */
    void createChangeSpeakerDialog();

    /**
     * @brief Opens a dialog for propagating time across a range of blocks.
     * Sets up a dialog allowing users to select a time range and optionally negate it.
     * On confirmation, the specified time is propagated to the selected block range.
     */
    void createTimePropagationDialog();

    /**
     * @brief Opens a dialog for selecting and marking tags in the current block.
     * Allows users to add tags to a block, showing existing tags initially.
     */
    void createTagSelectionDialog();

    /**
     * @brief Inserts a timestamp at the current cursor position within a block.
     * Updates the timestamp associated with the block and positions the cursor at the end.
     * @param elapsedTime The timestamp to set for the block.
     */
    void insertTimeStamp(const QTime& elapsedTime);

    /**
     * @brief Changes the language of the transcript.
     * Prompts the user to enter a new language, then loads the relevant dictionary.
     */
    void changeTranscriptLang();

    /**
     * @brief Navigates to the next or previous block by the same speaker.
     * Searches for the closest block in the specified direction with the same speaker,
     * then jumps to that block's timestamp if found.
     * @param jumpDirection Direction to jump ("up" or "down").
     */
    void speakerWiseJump(const QString& jumpDirection);

    /**
     * @brief Jumps to the next or previous word in the highlighted block.
     * Adjusts position based on direction and ensures cursor remains within block limits.
     * @param jumpDirection Direction to jump ("left" or "right").
     */
    void wordWiseJump(const QString& jumpDirection);

    /**
     * @brief Jumps to the next or previous block.
     * Moves the cursor up or down to adjacent blocks and jumps to the appropriate timestamp.
     * @param jumpDirection Direction to jump ("up" or "down").
     */
    void blockWiseJump(const QString& jumpDirection);

    /**
     * @brief Enables or disables transliteration for the editor.
     * Sets the transliteration state and language code.
     * @param value True to enable transliteration, false to disable.
     * @param langCode The language code to use for transliteration.
     */
    void useTransliteration(bool value, const QString& langCode = "en");

    /**
     * @brief Enables or disables the auto-save feature for the editor.
     *
     * Sets the `m_autoSave` member variable to the specified value, enabling or disabling
     * the auto-save functionality based on the `value` parameter.
     *
     * @param value Boolean value to enable (true) or disable (false) auto-save.
     */
    void useAutoSave(bool value) {m_autoSave = value;}

    /**
     * @brief Provides word suggestions and replaces the selected word.
     * Highlights the current word under the cursor and replaces it with the suggested text.
     * @param suggest The suggested replacement text.
     */
    void suggest(QString suggest);

    /**
     * @brief Toggles real-time data saving on or off.
     * Enables or disables the data-saving feature in real-time.
     */
    void realTimeDataSavingToggle();

    /**
     * @brief Exports the transcript as a PDF file.
     *
     * This function gathers the content of the transcript and formats
     * it into HTML before exporting it as a PDF. The user is prompted
     * to select a save location.
     */
    void saveAsPDF();

    /**
     * @brief Exports the transcript as a text file.
     *
     * The function compiles the content of the transcript into a
     * plain text format and allows the user to choose a save location.
     */
    void saveAsTXT();

    /**
     * @brief Updates the current block's timestamp with a new value.
     *
     * This function sets the timestamp of the specified block to
     * the given end time.
     *
     * @param block_num The index of the block to update (0-indexed).
     * @param endTime The new timestamp to set for the block.
     */
    void updateTimeStamp(int block_num, QTime endTime);

    /**
     * @brief Updates timestamps for a range of blocks based on a
     *        provided vector of seconds.
     *
     * This function sets the timestamps of the blocks based on the
     * provided vector. New blocks are added if the vector exceeds the
     * current number of blocks.
     *
     * @param blks A QVector of integers representing the timestamps in seconds.
     */
    void updateTimeStampsBlock(QVector<int> blks);

    void handleContentChanged();

private slots:
    void contentChanged(int position, int charsRemoved, int charsAdded);
    void wordEditorChanged();

    /**
     * @brief Updates the word editor with the current block's words.
     *
     * This function refreshes the word editor display based on the
     * current block of text being edited.
     */
    void updateWordEditor();

    /**
     * @brief Changes the speaker's name in the transcript.
     *
     * This function updates the speaker's name for the current block
     * or replaces all occurrences of the old speaker name with the new
     * one, based on the specified flag.
     *
     * @param newSpeaker The new speaker's name.
     * @param replaceAllOccurrences Flag indicating whether to replace
     *        all occurrences or just the current block.
     */
    void changeSpeaker(const QString& newSpeaker, bool replaceAllOccurrences);

    /**
     * @brief Propagates a specified time adjustment to a range of blocks.
     *
     * This function adjusts the timestamps of the selected blocks
     * based on the specified time, either adding or subtracting it.
     *
     * @param time The time to be added or subtracted from the blocks' timestamps.
     * @param start The starting block number (1-indexed).
     * @param end The ending block number (1-indexed).
     * @param negateTime Flag indicating whether to negate the time (subtract).
     */
    void propagateTime(const QTime& time, int start, int end, bool negateTime);

    /**
     * @brief Selects tags for the current block.
     *
     * This function assigns a list of tags to the current block and
     * refreshes the displayed tag list.
     *
     * @param newTagList The list of tags to assign to the current block.
     */
    void selectTags(const QStringList& newTagList);

    /**
     * @brief Marks a word as correct and updates the dictionary.
     *
     * This function adds the specified word to the dictionary if it
     * is not already present and updates the highlighting for
     * invalid words.
     *
     * @param blockNumber The index of the block containing the word (0-indexed).
     * @param wordNumber The index of the word to mark as correct (0-indexed).
     */
    void markWordAsCorrect(int blockNumber, int wordNumber);

    /**
     * @brief Inserts speaker name completion into the editor.
     *
     * This function inserts the selected speaker name completion at
     * the current cursor position.
     *
     * @param completion The completed speaker name to insert.
     */
    void insertSpeakerCompletion(const QString& completion);

    /**
     * @brief Inserts text completion into the editor.
     *
     * This function inserts the selected text completion at the
     * current cursor position.
     *
     * @param completion The completed text to insert.
     */
    void insertTextCompletion(const QString& completion);

    /**
     * @brief Inserts transliteration completion into the editor.
     *
     * This function inserts the selected transliteration at the
     * current cursor position.
     *
     * @param completion The completed transliteration to insert.
     */
    void insertTransliterationCompletion(const QString &completion);

    /**
     * @brief Processes the response received from a network reply and extracts tokens.
     *
     * Reads the data from `m_reply`, parses the response string to extract meaningful
     * tokens, and stores them in `m_lastReplyList`. The function expects the response
     * to follow a specific format: it splits on a predefined delimiter pattern (`[\"`)
     * to access the content, extracts the token list, and removes extraneous characters.
     *
     * @note If the response format does not meet the expected structure
     * (less than four sections when split by `"[\""`), the function will exit early.
     */
    void handleReply();

    /**
     * @brief Sends a request to an external service for text completion.
     *
     * This function constructs and sends a network request to an
     * external API based on the input text and language code.
     *
     * @param input The input text to send for completion suggestions.
     * @param langCode The language code indicating the language of the input text.
     */
    void sendRequest(const QString& input, const QString& langCode);

private:

    /**
     * @brief Parses a time string and converts it into a QTime object.
     *
     * The function checks the format of the provided time string `text` to determine
     * whether it includes fractional seconds ('.') and whether it has hours included (based on the number of colons).
     * Based on these characteristics, it applies the appropriate format for parsing.
     *
     * @param text A QString containing the time string to be converted.
     * @return A QTime object representing the parsed time, or an invalid QTime if parsing fails.
     */
    static QTime getTime(const QString& text);

    /**
     * @brief Creates a word object with the given attributes.
     *
     * This function initializes and returns a `word` struct populated with
     * the provided time, string content, list of tags, and edit status.
     *
     * @param t The QTime representing the timestamp of the word.
     * @param s The QString representing the content of the word.
     * @param tagList A QStringList containing any tags associated with the word.
     * @param isEdited A QString indicating the edit status of the word.
     * @return A `word` struct initialized with the provided parameters.
     */
    static word makeWord(const QTime& t, const QString& s, const QStringList& tagList, const QString& isEdited);

    /**
     * @brief Creates and configures a QCompleter for use in the editor.
     *
     * This function initializes a QCompleter with case-insensitive, non-wrapping,
     * popup-style completion, sets its widget to the current editor, and returns it.
     *
     * @return A pointer to the configured QCompleter.
     */
    QCompleter* makeCompleter();

    /**
     * @brief Saves the current transcript data to an XML file.
     *
     * This function writes the contents of `m_blocks` to an XML file, with each block
     * represented as a "line" element containing a timestamp, speaker, and a list of words.
     * Words within each line are nested as "word" elements with associated timestamps, tags,
     * and edit status.
     *
     * @param file A pointer to the QFile where the XML content will be written.
     */
    void saveXml(QFile* file);

    /**
     * @brief Sends the current text block to the \c MediaPlayer, allowing a jump to the relevant timestamp with \c MediaPlayer::setPositionToTime.
     *
     * This function emits the text of the current block to the player and calculates the appropriate
     * timestamp to jump to, based on the cursor’s position within the block. If the timestamp of the
     * exact word under the cursor is valid, it is used; otherwise, the function defaults to the nearest
     * preceding valid timestamp.
     */
    void helpJumpToPlayer();

    /**
     * @brief Loads a dictionary for the editor to use for word suggestions and spell-checking.
     *
     * This function checks if a language-specific dictionary file exists locally. If found,
     * it reads the dictionary and any custom words into `m_dictionary`. If corrected words
     * exist in a saved file, it includes them for spell-checking in the editor.
     */
    void loadDictionary();

    /**
     * @brief Converts a block number into a block structure containing the timestamp,
     *        text, speaker, and a list of words.
     *
     * This method processes the block text to extract a timestamp, speaker, and the
     * corresponding text. It uses regular expressions to match the timestamp and speaker
     * formats, and splits the text accordingly. The extracted words are stored in a
     * QVector of word structures.
     *
     * @param blockNumber The block number to convert into a block structure.
     * @return block A block structure containing the extracted data.
     */
    block fromEditor(qint64 blockNumber) const;

    /**
     * @brief Loads transcript data from an XML file into the editor.
     *
     * This method reads an XML file containing transcript data, extracts the language,
     * and populates the internal block structure with information about each line of the
     * transcript. It handles timestamps, speakers, and individual words along with their
     * properties.
     *
     * @param file A reference to the QFile object from which the transcript data is loaded.
     *
     * @note This method will raise an error if the file structure is incorrect.
     */
    static QStringList listFromFile(const QString& fileName) ;

    // State flags
    bool settingContent{false}; ///< Indicates if the editor is currently in a setting content mode.
    bool updatingWordEditor{false}; ///< Indicates if the word editor is being updated.
    bool dontUpdateWordEditor{false}; ///< Flag to prevent updates to the word editor.

    // Configuration options
    bool m_transliterate{false}; ///< Indicates if transliteration is enabled.
    bool m_autoSave{false}; ///< Indicates if auto-save functionality is enabled.

    // Transcript language and punctuation settings
    QString m_transcriptLang; ///< Language of the transcript.
    QString m_punctuation{",.!;:?"}; ///< Default punctuation characters used in the editor.

    // UI and settings components
    QSettings* settings; ///< Pointer to application settings.
    Highlighter* m_highlighter = nullptr; ///< Pointer to the syntax highlighter.

    // Highlighting information
    qint64 highlightedBlock = -1; ///< Index of the currently highlighted block in the editor.
    qint64 highlightedWord = -1; ///< Index of the currently highlighted word in the editor.

    // UI components for editing
    WordEditor* m_wordEditor = nullptr; ///< Pointer to the word editor instance.
    ChangeSpeakerDialog* m_changeSpeaker = nullptr; ///< Dialog for changing speakers.
    TimePropagationDialog* m_propagateTime = nullptr; ///< Dialog for propagating time changes.
    TagSelectionDialog* m_selectTag = nullptr; ///< Dialog for selecting tags.

    // Auto-completion features
    QCompleter *m_speakerCompleter = nullptr; ///< Completer for speaker names.
    QCompleter *m_textCompleter = nullptr; ///< Completer for text suggestions.
    QCompleter *m_transliterationCompleter = nullptr; ///< Completer for transliteration suggestions.

    // Dictionaries
    QStringList m_dictionary; ///< List of user-defined dictionary entries.
    QStringList m_english_dictionary; ///< List of English dictionary entries.
    QString m_customDictonaryPath = nullptr; ///< Path to the custom dictionary file.
    std::set<QString> m_correctedWords; ///< Set of words that have been corrected.
    QString m_transliterateLangCode; ///< Language code for transliteration.

    // Network management
    QStringList m_lastReplyList; ///< List of the last replies from network requests.
    QNetworkAccessManager m_manager; ///< Network manager for handling requests.
    QNetworkReply* m_reply = nullptr; ///< Pointer to the current network reply.

    // Auto-saving configuration
    QTimer* m_saveTimer = nullptr; ///< Timer for managing save intervals.
    int m_saveInterval{20}; ///< Interval in seconds for auto-saving documents.

    // Clipboard management
    QStringList clipboardTexts; ///< List of text items in the clipboard.
    QString fileBeforeSave = "initial.txt"; ///< File path for the initial state before saving.
    QString fileAfterSave = "final.txt"; ///< File path for the final state after saving.
    QString ComparedOutputFile = "ComparedDictonary.json"; ///< File path for compared dictionary output.

    // Real-time data settings
    bool realTimeDataSaver = false; ///< Flag to indicate if real-time data saving is enabled.
    QStringList allClips; ///< List of all clipboard contents.

    // Highlighting state
    int lastHighlightedBlock = -1; ///< Index of the last highlighted block.
    bool moveAlongTimeStamps = true; ///< Flag to determine if the editor should move along timestamps.
    QStringList supportedFormats; ///< List of supported file formats.
    QTimer *debounceTimer;
    const int debounceDelay = 300;

private:
    bool isWordValid(const QString& wordText,
                     const QStringList& primaryDict,
                     const QStringList& englishDict,
                     const QString& transcriptLang);
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
