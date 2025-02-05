#pragma once

#include <qmainwindow.h>
#include "about.h"
#include "editor/utilities/keyboardshortcutguide.h"
#include "git/git.h"
#include "mediaplayer/mediaplayer.h"
#include "editor/texteditor.h"
#include<QThread>
#include<QtConcurrent/QtConcurrent>
#include "./transcriptgenerator.h"
#include "mediaplayer/utilities/mediasplitter.h"
#include "qmainwindow.h"

#include"audiowaveform.h"
#include "qmediadevices.h"
#include "qtablewidget.h"
#include "tts/ttsrow.h"
QT_BEGIN_NAMESPACE
namespace Ui { class Tool; }
QT_END_NAMESPACE

/*!
 * \brief The Tool class (Main Window)
 *
 * This class serves as the main interface for the application, providing access
 * to various functionalities and managing the overall user experience.
 */
class Tool final : public QMainWindow
{
    Q_OBJECT

public:

    /*!
     * \brief Constructs a Tool object.
     *
     * Initializes the main window and sets up the UI components.
     *
     * \param parent Optional parent widget.
     */
    explicit Tool(QWidget *parent = nullptr);

    /*!
     * \brief Destroys the Tool object.
     *
     * Cleans up resources and saves any necessary state before destruction.
     */
    ~Tool() final;

public slots:

protected:

    /*!
     * \brief Handles key press events.
     *
     * Overrides the default behavior to customize key event handling.
     *
     * \param event The key event.
     */
    void keyPressEvent(QKeyEvent *event) override;

    /*!
     * \brief Handles drag enter events.
     *
     * \param event The drag enter event.
     */
    void dragEnterEvent(QDragEnterEvent *event) override;

    /*!
     * \brief Handles drop events.
     *
     * \param event The drop event.
     */
    void dropEvent(QDropEvent *event) override;

    /*!
     * \brief Handles drag move events.
     *
     * \param event The drag move event.
     */
    void dragMoveEvent(QDragMoveEvent *event) override;

    /*!
     * \brief Handles resize events.
     *
     * \param event The resize event.
     */
    void resizeEvent(QResizeEvent *event) override;


private slots:

    /*!
     * \brief Handles errors from the media player and prints at statusbar.
     *
     */
    void handleMediaPlayerError();

    /*!
     * \brief Creates a guide for keyboard shortcuts.
     *
     * If the guide is empty, it initializes a new \c KeyboardShortcutGuide.
     * Otherwise, it uses the existing member variable \c help_keyshortcuts,
     * which points to the current keyboard shortcut guide.
     */
    void createKeyboardShortcutGuide();

    /*!
     * \brief Changes the font of the application.
     */
    void changeFont();

    /*!
     * \brief Changes the font size.
     *
     * \param change The amount to change the font size by.
     */
    void changeFontSize(int change);

    /*!
     * \brief Handles the selection of a transliteration action.
     *
     * \param action The selected action.
     */
    void transliterationSelected(QAction* action);

    /*!
     * \brief Creates a media splitter.
     *
     * This method initializes a \c MediaSplitter class instance and
     * splits media files based on the provided transcript timestamps.
     */
    void createMediaSplitter();

    /*!
     * \brief Triggered when the upload and generate transcript action is invoked.
     *
     * This method uploads the media file to the web portal, generates transcripts,
     * and loads them into the application.
     *
     * Note: This feature is currently disabled.
     */
    void on_Upload_and_generate_Transcript_triggered();
    //    void on_editor_openTranscript_triggered();

    /*!
     * \brief Triggered when the translate button is clicked.
     *
     * Note: This feature is currently disabled.
     */
    void on_btn_translate_clicked();

    /*!
     * \brief Triggered when the "Open Transcript" button is clicked.
     *
     * This signal is emitted when the corresponding button in the GUI is clicked,
     * prompting the Editor class to open a specified transcript using a file dialog
     * from the file explorer for viewing or editing, by calling the
     * \c Editor::transcriptOpen() method.
     */
    void on_editor_openTranscript_triggered();

    /*!
     * \brief Triggered when the "Open Video/Audio" button is clicked in the GUI.
     *
     * This method is invoked when the user clicks the "Open Video/Audio" button,
     * prompting the MediaPlayer to open a file dialog and select a video file
     * by calling the \c MediaPlayer::open() method.
     */
    void on_add_video_clicked();

    /*!
     * \brief Triggered when the editor to open a transcript is invoked.
     *
     * This signal is emitted when the corresponding button in the GUI is clicked,
     * prompting the Editor class to open the specified transcript for viewing or editing
     * by calling the \c Editor::transcriptOpen() method.
     */
    void on_open_transcript_clicked();

    /*!
     * \brief Triggered when the "New Transcript" button is clicked.
     *
     * Note: This feature is currently disabled.
     */
    void on_new_transcript_clicked();

    /*!
     * \brief Triggered when the "Save Transcript" button is clicked.
     *
     * This method saves the current transcript from the GUI after the user clicks
     * the save button, invoking the \c Editor::transcriptSave() method.
     */
    void on_save_transcript_clicked();

    /*!
     * \brief Triggered when the "Save As Transcript" button is clicked.
     *
     * This method saves the current transcript from the GUI after the user clicks
     * the save as button, invoking the \c Editor::transcriptSaveAs() method.
     */
    void on_save_as_transcript_clicked();

    /*!
     * \brief Triggered when the "Load Custom Dictionary" button is clicked.
     *
     * This method opens a file dialog to load a custom dictionary file when the
     * corresponding button in the GUI is clicked.
     */
    void on_load_a_custom_dict_clicked();

    /*!
     * \brief Triggered when the "Get PDF" button is clicked.
     *
     * This method saves the current transcript as a PDF when the user clicks the
     * "Get PDF" button in the GUI, invoking the \c Editor::saveAsPDF() method.
     */
    void on_get_PDF_clicked();

    /*!
     * \brief Triggered when the "Decrease Font Size" button is clicked.
     *
     * This method decreases the font size of UI elements to improve readability.
     */
    void on_decreseFontSize_clicked();

    /*!
     * \brief Triggered when the "Increase Font Size" button is clicked.
     *
     * This method increases the font size of UI elements for better visibility.
     */
    void on_increaseFontSize_clicked();

    /*!
     * \brief Toggles the visibility of the word editor.
     *
     * This method shows or hides the \c WordEditor instance in the GUI based on the current state.
     * If the word editor is currently hidden, it will be displayed; if it is visible, it will be hidden.
     * The visibility state is managed through the \c m_wordEditor GUI variable.
     */
    void on_toggleWordEditor_clicked();

    /*!
     * \brief Opens the keyboard shortcuts guide.
     *
     * This method is triggered when the user clicks the "Keyboard Shortcuts" button in the GUI.
     * It invokes the \c createKeyboardShortcutGuide() method to display the available keyboard shortcuts,
     * helping users navigate the application more efficiently.
     */
    void on_keyboard_shortcuts_clicked();

    /*!
     * \brief Updates the font based on the current selection in the font combo box.
     *
     * \param f The newly selected font.
     */
    void on_fontComboBox_currentFontChanged(const QFont &f);

    /*!
     * \brief Triggered when the "About" action is invoked.
     *
     * This method displays the About dialog, providing information about the application
     * and its authors.
     */
    void on_actionAbout_triggered();

    /*!
     * \brief Triggered when the "Initialize" action is invoked.
     *
     * This method is called when the user selects the "Initialize" option in the GUI.
     * It invokes the \c Git::init() method to set up the Git repository and necessary configurations,
     * enabling version control features within the application.
     */
    void on_actionInit_triggered();

    /*!
     * \brief Triggered when the "Add All" action is invoked.
     *
     * This method is called when the user selects the "Add All" option in the GUI.
     * It invokes the \c Git::add() method to stage all changes in the repository.
     */
    void on_actionAdd_All_triggered();

    /*!
     * \brief Triggered when the "Commit" action is invoked.
     *
     * This method is called when the user selects the "Commit" option in the GUI.
     * It invokes the \c Git::commit() method to commit all staged changes to the repository.
     */
    void on_actionCommit_triggered();

    /*!
     * \brief Triggered when the "Add Remote URL" action is invoked.
     *
     * This method is called when the user selects the "Add Remote URL" option in the GUI.
     * It invokes the \c Git::addRemoteUrl() method to add or update a remote repository URL.
     */
    void on_actionAdd_Remote_URL_triggered();

    /*!
     * \brief Triggered when the "Push" action is invoked.
     *
     * This method is called when the user selects the "Push" option in the GUI.
     * It invokes the \c Git::push() method to upload local changes to the remote repository.
     */
    void on_actionPush_triggered();

    /*!
 * \brief Triggered when the "Pull" action is invoked.
     *
     * This method is called when the user selects the "Pull" option in the GUI.
     * It invokes the \c Git::pull() method to retrieve and merge changes from the remote
     * repository into the local repository, ensuring that the local state is up-to-date
     * with the latest commits.
     */
    void on_actionPull_triggered();

    /*!
     * \brief Triggered when the "Show Waveform" action is invoked.
     *
     * This method is called when the user selects the "Show Waveform" option in the GUI.
     * It invokes the \c showWaveForm() method of the \c AudioWaveForm instance,
     * displaying the waveform of the currently loaded audio for visual analysis at Waveform tab.
     */
    void on_actionShow_Waveform_triggered();

    /*!
     * \brief Triggered when the "Update Timestamps" action is invoked.
     *
     * This method is called when the user selects the "Update Timestamps" option in the GUI.
     * It invokes the \c updateTimestampsToggle() method of the \c AudioWaveForm instance,
     * updating the timestamps in the currently opened transcript block in the editor.
     */
    void on_actionSave_Timestamps_triggered();

    /*!
     * \brief Connects the waveform visualization and the media player.
     *
     * This method establishes connections between the \c MediaPlayer instance \c player and the
     * \c AudioWaveForm instance from the UI based on the success of audio sampling.
     *
     * If \c isSamplingSuccess is true, it connects the media player's position
     * changes to update the waveform position, and vice versa. If sampling is
     * not successful, it disconnects any existing connections to prevent
     * updates between the media player and the waveform.
     */
    void connectWaveformAndMediaplayer();

    /*!
     * \brief Triggered when the "Update Timestamps" action is invoked.
     *
     * This method is called when the user selects the "Update Timestamps" option in the GUI.
     * It invokes the \c updateTimeStamps() method of the \c AudioWaveForm instance,
     * which processes and emits the updated timestamps for the currently loaded audio.
     */
    void on_actionUpdate_Timestamps_triggered();

    /*!
     * \brief Triggered when the "Show Endlines" action is invoked.
     *
     * This method is called when the user selects the "Show Endlines" option in the GUI.
     * It invokes the \c showWaveform() method of the \c Editor instance, which processes
     * the current transcript blocks and prepares the waveform for display.
     */
    void on_actionShow_Endlines_triggered();

    /*!
     * \brief Triggered when the "Open" action from TTS options is invoked.
     *
     * This method is called when the user selects the "Open" option in the GUI.
     * It invokes the \c openTTSTranscript() method of the \c TTSAnnotator instance,
     * allowing the user to select a transcript file to open and process.
     */
    void on_actionOpen_triggered();

    void on_actionIncrease_speed_by_1_triggered();

    void on_actionDecrease_speed_by_1_triggered();

private:

    /*!
     * \brief Sets the font for various UI elements.
     *
     * This method applies a specified font to multiple editor components and
     * other UI elements. It ensures that the appearance of the text in the
     * editors and associated widgets is consistent by setting the same font
     * across all relevant elements. The method also calls \c fitTableContents()
     * on the word editor to adjust the display according to the new font settings.
     */
    void setFontForElements();

    /*!
     * \brief Sets up transliteration language codes.
     *
     * This method initializes the mapping of language names to their corresponding
     * ISO language codes. It creates two QStringLists: one for the language names
     * and another for the associated codes. The method iterates through the lists
     * and populates the \c m_transliterationLang map with the language names as keys
     * and their corresponding codes as values. This mapping is used for transliteration
     * functionality throughout the application.
     */
    void setTransliterationLangCodes();

    /*!
     * \brief isDroppedOnLayout
     * \param pos
     * \param layout
     * \return
     * Unused
     */
    bool isDroppedOnLayout(const QPoint &pos, QVBoxLayout *layout);

    /*!
     * \brief parseXML
     * \param fileUrl
     * \return
     * Unused
     */
    QVector<TTSRow> parseXML(const QUrl& fileUrl);

    /*!
     * \brief Sets the default audio output device.
     *
     * This method retrieves the system's default audio output device using
     * \c QMediaDevices::defaultAudioOutput(). If a valid device is found, it
     * sets this device for the audio output instance (\c m_audioOutput) and logs
     * the device description for debugging purposes. If no audio output devices
     * are available, it logs a message indicating that no devices were found.
     */
    void setDefaultAudioOutputDevice();

    /*!
     * \brief Handles tab change events in the UI.
     *
     * This method is called when the user switches between tabs in the UI.
     * It checks the index of the currently selected tab. If the index is greater
     * than 0 (indicating that a tab other than the first tab is selected), it
     * hides the media and transcript filename labels. If the first tab is selected
     * (index 0), it shows these labels.
     *
     * \param index The index of the currently selected tab.
     */
    void onTabChanged(int index);

    /*!
     * \brief Pointer to the media player instance.
     *
     * This object is responsible for playing media files and managing playback controls.
     */
    MediaPlayer *player = nullptr;

    /*!
     * \brief Pointer to the audio output instance.
     *
     * This object manages the audio output device used for playback.
     */
    QAudioOutput *m_audioOutput = nullptr;

    /*!
     * \brief Pointer to the UI components.
     *
     * This pointer provides access to the user interface elements defined in the
     * Tool class UI.
     */
    Ui::Tool *ui;

    /*!
     * \brief Font used for various text elements in the UI.
     *
     * This variable holds the QFont object representing the default font
     * used throughout the application.
     */
    QFont font;

    /*!
     * \brief Map of transliteration languages and their corresponding codes.
     *
     * This map associates language names with their ISO language codes
     * to facilitate transliteration functionality.
     */
    QMap<QString, QString> m_transliterationLang;

    /*!
     * \brief Pointer to the Git management class.
     *
     * This object handles Git operations such as init, add_url, commit, push, and pull.
     */
    Git* git = nullptr;

    /*!
     * \brief Flag indicating if sending sample rate was successful.
     *
     * This boolean variable tracks the success state of sample rate operations.
     */
    bool isSendingSampleRateSuccess = false;

    /*!
     * \brief Flag indicating if audio sampling was successful.
     *
     * This boolean variable indicates whether audio sampling was completed successfully.
     */
    bool isSamplingSuccess = false;

    /*!
     * \brief Pointer to application settings.
     *
     * This object manages user preferences and settings using QSettings.
     */
    QSettings* settings;

    /*!
     * \brief Pointer to the keyboard shortcut guide.
     *
     * This object provides assistance for using keyboard shortcuts in the application.
     */
    KeyboardShortcutGuide* help_keyshortcuts = nullptr;

    /*!
     * \brief Pointer to the About dialog.
     *
     * This object displays information about the application.
     */
    About* about = nullptr;

    /*!
     * \brief Object representing available media devices.
     *
     * This object provides access to media devices available on the system.
     */
    QMediaDevices m_mediaDevices;

    /*!
     * \brief Variable representing speed of seeking in media player.
     *
     * This variable provides access to changing speed of seeking.
    */
    int64_t seekSpeed = 0;
};
