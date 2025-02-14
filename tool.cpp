#include "tool.h"
#include "./ui_tool.h"
#include "about.h"
#include "audioplayer/audioplayerwidget.h"
#include "editor/utilities/keyboardshortcutguide.h"
#include "tts/ttsrow.h"
#include <QProgressBar>

#include <QFontDialog>
#include <QMessageBox>

#include <QStyle>
#include <QIcon>

#include <QMediaPlayer>
#include <QActionGroup>

#include <git/git.h>
#include "qmediadevices.h"
#include "qaudiodevice.h"
#include <algorithm>

// #include <config/settingsManager.h>

Tool::Tool(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Tool)
{
    ui->setupUi(this);
    // auto& settings = SettingsManager::getInstance();
    // settings.initializeAction(ui->Show_Time_Stamps, "showTimeStamps", true);

    // // Connect QAction changes
    // connect(ui->Show_Time_Stamps, &QAction::toggled, [&settings, this](bool) {
    //     settings.saveActionState(ui->Show_Time_Stamps, "showTimeStamps");
    // });

    QString iniPath = QApplication::applicationDirPath() + "/" + "config.ini";
    settings = new QSettings(iniPath, QSettings::IniFormat);
    if(settings->value("showTimeStamps").toString()=="") {
        ui->Show_Time_Stamps->setChecked(true);
    }
    else {
        ui->Show_Time_Stamps->setChecked(settings->value("showTimeStamps").toString() == "true");
    }

    seekSpeed = std::clamp(settings->value("seekSpeed").toInt(), 1, 5);

    player = new MediaPlayer(this);
    player->setVideoOutput(ui->m_videoWidget);
    // m_audioOutput = new QAudioOutput(this);
    // player->setAudioOutput(m_audioOutput);

    connect(&m_mediaDevices, &QMediaDevices::audioOutputsChanged, [this]() {
        setDefaultAudioOutputDevice();
    });

    ui->splitter_tool->setCollapsible(0, false);
    ui->splitter_tool->setCollapsible(1, false);
    ui->splitter_tool->setSizes(QList<int>({static_cast<int>(0.15 * sizeHint().height()),
                                            static_cast<int>(0.85 * sizeHint().height())}));

    ui->splitter_editor->setCollapsible(0, false);
    ui->splitter_editor->setCollapsible(1, false);
    ui->splitter_editor->setSizes(QList<int>({static_cast<int>(0.7 * sizeHint().height()),
                                              static_cast<int>(0.3 * sizeHint().height())}));

    ui->m_wordEditor->setHidden(true);

    setTransliterationLangCodes();
    //Upload_and_generate_Transcript
    connect(ui->close, &QAction::triggered, this, &QMainWindow::close);

    //ToolBar icons
    //    connect(ui->add_video, &QAction::clicked, player, &MediaPlayer::open);


    // Connect Player Controls and Media Player
    connect(ui->player_open, &QAction::triggered, player, &MediaPlayer::open);
    connect(ui->player_togglePlay, &QAction::triggered, player, &MediaPlayer::togglePlayback);
    connect(ui->player_seekForward, &QAction::triggered, player, [&]() {player->seek(seekSpeed);});
    connect(ui->player_seekBackward, &QAction::triggered, player, [&]() {player->seek(-seekSpeed);});
    connect(ui->m_playerControls, &PlayerControls::play, player, &QMediaPlayer::play);
    connect(ui->m_playerControls, &PlayerControls::pause, player, &QMediaPlayer::pause);
    connect(ui->m_playerControls, &PlayerControls::stop, player, &QMediaPlayer::stop);
    connect(ui->m_playerControls, &PlayerControls::seekForward, player, [&]() {player->seek(seekSpeed);});
    connect(ui->m_playerControls, &PlayerControls::seekBackward, player, [&]() {player->seek(-seekSpeed);});
    connect(ui->m_playerControls, &PlayerControls::changeVolume, player, &MediaPlayer::setVolume);
    connect(ui->m_playerControls, &PlayerControls::changeMuting, player, &MediaPlayer::setMuted);
    connect(ui->m_playerControls, &PlayerControls::changeRate, player, &QMediaPlayer::setPlaybackRate);
    connect(ui->m_playerControls, &PlayerControls::stop, ui->m_videoWidget, QOverload<>::of(&QVideoWidget::update));
    connect(player, &MediaPlayer::playbackStateChanged, ui->m_playerControls, &PlayerControls::setState);
    connect(player->audioOutput, &QAudioOutput::volumeChanged, ui->m_playerControls, &PlayerControls::setVolume);
    connect(player->audioOutput, &QAudioOutput::mutedChanged, ui->m_playerControls, &PlayerControls::setMuted);
    connect(player, &MediaPlayer::message, this->statusBar(), &QStatusBar::showMessage);

    connect(player, &MediaPlayer::openMessage, [this](const QString& text) {
        this->ui->mediaFilenameLbl->setText(text);
        QString message = text;
        if (text.size() > 30)
            message = text.left(30) + "...";
        this->ui->mediaFilenameLbl->setText("           Media: " + message);
    });

    connect(ui->m_editor, &Editor::openMessage, [this](const QString& text) {
        this->ui->transcriptFilenameLbl->setText(text);
        QString message = text;
        if (text.size() > 30)
            message = text.left(30) + "...";
        this->ui->transcriptFilenameLbl->setText("Transcript: " + message + "           ");
    });
    // Qt6
    // connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &Tool::handleMediaPlayerError);
    connect(player, &MediaPlayer::errorChanged, this, &Tool::handleMediaPlayerError);

    //Waveofrm
    // connect(player, &MediaPlayer::sendBuffer, ui->widget, &AudioWaveForm::processBuffer);

    connect(player, &MediaPlayer::sendDuration, ui->widget, &AudioWaveForm::getDuration);

    connect(ui->m_editor, &Editor::sendBlockTime, ui->widget, &AudioWaveForm::getTimeArray);

    connect(ui->widget, &AudioWaveForm::updateTime, ui->m_editor, &Editor::updateTimeStamp);

    // connect(player, &MediaPlayer::sendSampleRate, ui->widget, &AudioWaveForm::getSampleRate);

    connect(ui->widget, &AudioWaveForm::updateTimeStampsBlock, ui->m_editor, &Editor::updateTimeStampsBlock);

    connect(player, &MediaPlayer::sendingSampleRateStatus, this, [&](bool status) {
        isSendingSampleRateSuccess = status;
        connectWaveformAndMediaplayer();
    });

    connect(ui->widget, &AudioWaveForm::samplingStatus, this, [&](bool status) {
        isSamplingSuccess = status;
        connectWaveformAndMediaplayer();
    });

    connect(ui->m_editor, &Editor::sendBlockText, ui->widget, &AudioWaveForm::getBlockText);

    // Connect components dependent on Player's position change to player
    connect(player, &QMediaPlayer::positionChanged, this,
            [&]()
            {
                ui->slider_position->setValue(player->position());
                ui->label_position->setText(player->getPositionInfo());
                ui->m_editor->highlightTranscript(player->elapsedTime());
            }
            );

    connect(player, &QMediaPlayer::durationChanged, this,
            [&]()
            {
                ui->slider_position->setRange(0, player->duration());
                ui->label_position->setText(player->getPositionInfo());
            }
            );

    // Connect edit menu actions
    connect(ui->edit_undo, &QAction::triggered, ui->m_editor, &Editor::undo);
    connect(ui->edit_redo, &QAction::triggered, ui->m_editor, &Editor::redo);
    connect(ui->edit_cut, &QAction::triggered, ui->m_editor, &Editor::cut);
    connect(ui->edit_copy, &QAction::triggered, ui->m_editor, &Editor::copy);
    connect(ui->edit_paste, &QAction::triggered, ui->m_editor, &Editor::paste);
    connect(ui->edit_findReplace, &QAction::triggered, ui->m_editor, &Editor::findReplace);

    // Connect view menu actions
    connect(ui->view_incFont, &QAction::triggered, this, [&]() {changeFontSize(+1);});
    connect(ui->view_decFont, &QAction::triggered, this, [&]() {changeFontSize(-1);});
    connect(ui->view_font, &QAction::triggered, this, &Tool::changeFont);
    connect(ui->view_toggleTagList, &QAction::triggered, this, [&]() {ui->m_tagListDisplay->setVisible(!ui->m_tagListDisplay->isVisible());});

    // Connect Editor menu actions and editor controls
    ui->m_editor->setWordEditor(ui->m_wordEditor);
    connect(ui->Save_as_PDF,&QAction::triggered,ui->m_editor,&Editor::saveAsPDF);
    connect(ui->actionSave_as_Text,&QAction::triggered,ui->m_editor,&Editor::saveAsTXT);
    connect(ui->Real_Time_Data_Saver,&QAction::triggered,ui->m_editor,&Editor::realTimeDataSavingToggle);
    connect(ui->Add_Custom_Dictonary, &QAction::triggered, ui->m_editor, &Editor::addCustomDictonary);

    //    connect(ui->editor_openTranscript, &QAction::triggered, ui->m_editor, &Editor::transcriptOpen);
    connect(ui->editor_debugBlocks, &QAction::triggered, ui->m_editor, &Editor::showBlocksFromData);
    connect(ui->editor_save, &QAction::triggered, ui->m_editor, &Editor::transcriptSave);
    connect(ui->editor_saveAs, &QAction::triggered, ui->m_editor, &Editor::transcriptSaveAs);
    connect(ui->editor_close, &QAction::triggered, ui->m_editor, &Editor::transcriptClose);
    connect(ui->editor_jumpToLine, &QAction::triggered, ui->m_editor, &Editor::jumpToHighlightedLine);
    connect(ui->editor_splitLine, &QAction::triggered, ui->m_editor, [&]() {ui->m_editor->splitLine(player->elapsedTime());});
    connect(ui->editor_mergeUp, &QAction::triggered, ui->m_editor, &Editor::mergeUp);
    connect(ui->editor_mergeDown, &QAction::triggered, ui->m_editor, &Editor::mergeDown);
    connect(ui->editor_toggleWords, &QAction::triggered, ui->m_wordEditor, [&](){ui->m_wordEditor->setVisible(!ui->m_wordEditor->isVisible());});
    connect(ui->editor_changeLang, &QAction::triggered, ui->m_editor, &Editor::changeTranscriptLang);
    connect(ui->editor_changeSpeaker, &QAction::triggered, ui->m_editor, &Editor::createChangeSpeakerDialog);
    connect(ui->editor_propagateTime, &QAction::triggered, ui->m_editor, &Editor::createTimePropagationDialog);
    connect(ui->editor_editTags, &QAction::triggered, ui->m_editor, &Editor::createTagSelectionDialog);
    connect(ui->editor_autoSave, &QAction::triggered, ui->m_editor, [this](){ui->m_editor->useAutoSave(ui->editor_autoSave->isChecked());});
    connect(ui->m_editor, &Editor::message, this->statusBar(), &QStatusBar::showMessage);
    connect(ui->m_editor, &Editor::jumpToPlayer, player, &MediaPlayer::setPositionToTime);
    connect(ui->m_editor, &Editor::refreshTagList, ui->m_tagListDisplay, &TagListDisplayWidget::refreshTags);
    connect(ui->Show_Time_Stamps, &QAction::triggered, ui->m_editor,&Editor::setShowTimeStamp );
    connect(ui->move_along_timestamps, &QAction::triggered, ui->m_editor,&Editor::setMoveAlongTimeStamps );


    connect(ui->Open_File_in_Editor1, &QAction::triggered, ui->m_editor_2, &Editor::transcriptOpen);
    //    connect(ui->editor_debugBlocks, &QAction::triggered, ui->m_editor_2, &Editor::showBlocksFromData);
    connect(ui->editor_save2, &QAction::triggered, ui->m_editor_2, &Editor::transcriptSave);
    //    connect(ui->editor_saveAs, &QAction::triggered, ui->m_editor_2, &Editor::transcriptSaveAs);
    //    connect(ui->editor_close, &QAction::triggered, ui->m_editor_2, &Editor::transcriptClose);
    //    connect(ui->editor_jumpToLine, &QAction::triggered, ui->m_editor_2, &Editor::jumpToHighlightedLine);
    //    connect(ui->editor_splitLine, &QAction::triggered, ui->m_editor_2, [&]() {ui->m_editor_2->splitLine(player->elapsedTime());});
    //    connect(ui->editor_mergeUp, &QAction::triggered, ui->m_editor_2, &Editor::mergeUp);
    //    connect(ui->editor_mergeDown, &QAction::triggered, ui->m_editor_2, &Editor::mergeDown);
    //    connect(ui->editor_changeLang, &QAction::triggered, ui->m_editor_2, &Editor::changeTranscriptLang);
    //    connect(ui->editor_changeSpeaker, &QAction::triggered, ui->m_editor_2, &Editor::createChangeSpeakerDialog);
    //    connect(ui->editor_propagateTime, &QAction::triggered, ui->m_editor_2, &Editor::createTimePropagationDialog);
    //    connect(ui->editor_editTags, &QAction::triggered, ui->m_editor_2, &Editor::createTagSelectionDialog);
    //    connect(ui->editor_autoSave, &QAction::triggered, ui->m_editor_2, [this](){ui->m_editor_2->useAutoSave(ui->editor_autoSave->isChecked());});
    //    connect(ui->m_editor_2, &Editor::message, this->statusBar(), &QStatusBar::showMessage);
    //    connect(ui->m_editor_2, &Editor::jumpToPlayer, player, &MediaPlayer::setPositionToTime);
    //    connect(ui->m_editor_2, &Editor::refreshTagList, ui->m_tagListDisplay, &TagListDisplayWidget::refreshTags);
    //    connect(ui->Show_Time_Stamps, &QAction::triggered, ui->m_editor_2,&Editor::setShowTimeStamp );


    connect(ui->Open_File_in_Editor2, &QAction::triggered, ui->m_editor_3, &Editor::transcriptOpen);
    //    connect(ui->editor_debugBlocks, &QAction::triggered, ui->m_editor_2, &Editor::showBlocksFromData);
    connect(ui->editor_save3, &QAction::triggered, ui->m_editor_3, &Editor::transcriptSave);
    //    connect(ui->editor_saveAs, &QAction::triggered, ui->m_editor_2, &Editor::transcriptSaveAs);
    //    connect(ui->editor_close, &QAction::triggered, ui->m_editor_2, &Editor::transcriptClose);
    //    connect(ui->editor_jumpToLine, &QAction::triggered, ui->m_editor_2, &Editor::jumpToHighlightedLine);
    //    connect(ui->editor_splitLine, &QAction::triggered, ui->m_editor_2, [&]() {ui->m_editor_2->splitLine(player->elapsedTime());});
    //    connect(ui->editor_mergeUp, &QAction::triggered, ui->m_editor_2, &Editor::mergeUp);
    //    connect(ui->editor_mergeDown, &QAction::triggered, ui->m_editor_2, &Editor::mergeDown);
    //    connect(ui->editor_changeLang, &QAction::triggered, ui->m_editor_2, &Editor::changeTranscriptLang);
    //    connect(ui->editor_changeSpeaker, &QAction::triggered, ui->m_editor_2, &Editor::createChangeSpeakerDialog);
    //    connect(ui->editor_propagateTime, &QAction::triggered, ui->m_editor_2, &Editor::createTimePropagationDialog);
    //    connect(ui->editor_editTags, &QAction::triggered, ui->m_editor_2, &Editor::createTagSelectionDialog);
    //    connect(ui->editor_autoSave, &QAction::triggered, ui->m_editor_2, [this](){ui->m_editor_2->useAutoSave(ui->editor_autoSave->isChecked());});
    //    connect(ui->m_editor_2, &Editor::message, this->statusBar(), &QStatusBar::showMessage);
    //    connect(ui->m_editor_2, &Editor::jumpToPlayer, player, &MediaPlayer::setPositionToTime);
    //    connect(ui->m_editor_2, &Editor::refreshTagList, ui->m_tagListDisplay, &TagListDisplayWidget::refreshTags);
    //    connect(ui->Show_Time_Stamps, &QAction::triggered, ui->m_editor_2,&Editor::setShowTimeStamp );


    auto useTransliterationMenu = new QMenu("Use Transliteration", ui->menuEditor);
    auto group = new QActionGroup(this);
    auto langs = m_transliterationLang.keys();

    auto none = new QAction("None");
    none->setActionGroup(group);
    none->setCheckable(true);
    none->setChecked(true);
    none->setActionGroup(group);
    useTransliterationMenu->addAction(none);

    for(int i = 0; i < langs.size(); i++) {
        auto action = new QAction(langs[i]);
        action->setCheckable(true);
        action->setActionGroup(group);
        useTransliterationMenu->addAction(action);
    }
    group->setExclusive(true);
    ui->menuEditor->addMenu(useTransliterationMenu);

    connect(group, &QActionGroup::triggered, this, &Tool::transliterationSelected);


    // Connect keyboard shortcuts guide to help action
    connect(ui->help_keyboardShortcuts, &QAction::triggered, this, &Tool::createKeyboardShortcutGuide);

    // Connect position slider change to player position
    connect(ui->slider_position, &QSlider::sliderMoved, player, &MediaPlayer::setPosition);

    connect(ui->m_playerControls, &PlayerControls::splitClick, this, &Tool::createMediaSplitter);
    connect(ui->m_videoWidget, &VideoWidget::droppedFile, player, &MediaPlayer::loadMediaFromUrl);
    font = QFont( "Monospace", 10 );
    setFontForElements();
    git = new Git();
    git->init();
    setAcceptDrops(true);
    // installEventFilter(this);
    ui->m_editor->setAcceptDrops(false);
    // tableWidget = ui->tableWidget;
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &Tool::onTabChanged);
    connect(player, &MediaPlayer::sendMediaUrl, ui->widget, &AudioWaveForm::setMediaUrl);

}

Tool::~Tool()
{
    delete player;
    delete ui;
    delete git;
}

void Tool::connectWaveformAndMediaplayer()
{
    if (isSamplingSuccess) {
        connect(player, &QMediaPlayer::positionChanged, ui->widget, &AudioWaveForm::setPlayerPosition);
        connect(ui->widget, &AudioWaveForm::positionChanged, player, &QMediaPlayer::setPosition);
    } else {
        disconnect(player, &QMediaPlayer::positionChanged, ui->widget, &AudioWaveForm::setPlayerPosition);
        disconnect(ui->widget, &AudioWaveForm::positionChanged, player, &QMediaPlayer::setPosition);
    }
}

void Tool::handleMediaPlayerError()
{
    if (player->error() == QMediaPlayer::NoError)
        return;

    const QString errorString = player->errorString();
    QString message = "Error: ";
    if (errorString.isEmpty())
        message += " #" + QString::number(int(player->error()));
    else
        message += errorString;
    statusBar()->showMessage(message);
}

void Tool::keyPressEvent(QKeyEvent *event)
{

    if (event->key() == Qt::Key_Up && event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
        ui->m_editor->speakerWiseJump("up");
    else if (event->key() == Qt::Key_Down && event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
        ui->m_editor->speakerWiseJump("down");
    else if (event->key() == Qt::Key_Left && event->modifiers() == Qt::AltModifier)
        ui->m_editor->wordWiseJump("left");
    else if (event->key() == Qt::Key_Right && event->modifiers() == Qt::AltModifier)
        ui->m_editor->wordWiseJump("right");
    else if (event->key() == Qt::Key_Up && event->modifiers() == Qt::AltModifier )
        ui->m_editor->blockWiseJump("up");
    else if (event->key() == Qt::Key_Down && event->modifiers() == Qt::AltModifier)
        ui->m_editor->blockWiseJump("down");
    else if (event->key() == Qt::Key_I && event->modifiers() == Qt::ControlModifier) {
        if (ui->m_editor->hasFocus())
            ui->m_editor->insertTimeStamp(player->elapsedTime());
        else if(ui->m_wordEditor->hasFocus())
            ui->m_wordEditor->insertTimeStamp(player->elapsedTime());
    }
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    else if (event->key() == Qt::Key_Space && event->modifiers() == Qt::ControlModifier) {
        player->togglePlayback();
    }
#elif defined(Q_OS_MAC)
    else if (event->key() == Qt::Key_Space && event->modifiers() == Qt::AltModifier) {
        player->togglePlayback();
    }
#endif
    else
        QMainWindow::keyPressEvent(event);
}

void Tool::createKeyboardShortcutGuide()
{
    if (!help_keyshortcuts)
    {
        help_keyshortcuts = new KeyboardShortcutGuide(this);
        help_keyshortcuts->setAttribute(Qt::WA_DeleteOnClose);
        help_keyshortcuts->show();

        connect(help_keyshortcuts, &QObject::destroyed, [this]() {
            help_keyshortcuts = nullptr;
        });

    } else {
        help_keyshortcuts->raise();
        help_keyshortcuts->activateWindow();
    }

}

void Tool::createMediaSplitter() {

    QString mediaFileName = player->getMediaFileName();
    if (mediaFileName.isNull() || mediaFileName.isEmpty()) {
        this->statusBar()->showMessage("Please select a media file", 5000);
        return;
    }

    QList<QTime> timeStamps = ui->m_editor->getTimeStamps();

    if (timeStamps.isEmpty() || timeStamps.size() <= 0) {
        this->statusBar()->showMessage("Please select a transcription file", 5000);
        return;
    }

    auto mediaSplitter = new MediaSplitter(nullptr, mediaFileName, timeStamps);

    setEnabled(false);
    mediaSplitter->exec();
    setEnabled(true);


}

void Tool::changeFont()
{
    bool fontSelected;
    font = QFontDialog::getFont(&fontSelected, QFont( font.family(), font.pointSize() ), this, tr("Pick a font") );

    if (fontSelected)
        setFontForElements();
}

void Tool::changeFontSize(int change)
{
    font.setPointSize(font.pointSize() + change);
    setFontForElements();
}

void Tool::setFontForElements()
{
    ui->m_editor->setEditorFont(font);
    ui->m_editor_2->setEditorFont(font);
    ui->m_editor_3->setEditorFont(font);
    ui->m_wordEditor->setFont(font);
    ui->m_wordEditor->fitTableContents();
    ui->tableWidget->textDelegate->setFont(font);
    ui->tableWidget->tableView->setFont(font);
    ui->tableWidget->tableView->horizontalHeader()->setFont(font);  // Set font for header
    ui->tableWidget->tableView->verticalHeader()->setFont(font);    // Set font for row headers
    ui->tableWidget->tableView->viewport()->update();
    ui->m_tagListDisplay->setFont(font);
}

void Tool::setTransliterationLangCodes()
{
    QStringList languages = QString("ENGLISH AMHARIC ARABIC BENGALI CHINESE GREEK GUJARATI HINDI KANNADA MALAYALAM MARATHI NEPALI ORIYA PERSIAN PUNJABI RUSSIAN SANSKRIT SINHALESE SERBIAN TAMIL TELUGU TIGRINYA URDU").split(" ");
    QStringList langCodes = QString("en am ar bn zh el gu hi kn ml mr ne or fa pa ru sa si sr ta te ti ur").split(" ");

    for (int i = 0; i < languages.size(); i++)
        m_transliterationLang.insert(languages[i], langCodes[i]);
}

void Tool::transliterationSelected(QAction* action)
{
    if (action->text() == "None") {
        ui->m_editor->useTransliteration(false);
        return;
    }


    auto langCode = m_transliterationLang[action->text()];
    qDebug() << langCode;
    ui->m_editor->useTransliteration(true, langCode);
}




void Tool::on_Upload_and_generate_Transcript_triggered()
{
    //Qt 6 (disabled since not utilized in app yet)
    // QFileDialog fileDialog(this);
    // fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    // fileDialog.setWindowTitle(tr("Open File"));
    // fileDialog.setDirectory(QStandardPaths::standardLocations(
    //                             QStandardPaths::DocumentsLocation).value(0, QDir::homePath()));


    // if (fileDialog.exec() == QDialog::Accepted) {
    //     QUrl *fileUrl = new QUrl(fileDialog.selectedUrls().constFirst());
    //     TranscriptGenerator tr(fileUrl);
    //     qInfo()<<QThread::currentThread();
    //     //running transcript generator on another thread (parallel processing)
    //     QtConcurrent::run(&tr, &TranscriptGenerator::Upload_and_generate_Transcript);


    //     QFile myfile(fileUrl->toLocalFile());
    //     QFileInfo fileInfo(myfile);
    //     QString filename(fileInfo.fileName());
    //     QString filepaths=fileInfo.dir().path();

    //     qInfo()<<filepaths+"/transcript.xml";

    //     bool fileExists = QFileInfo::exists(filepaths+"/transcript.xml") && QFileInfo(filepaths+"/transcript.xml").isFile();
    //     while(!fileExists){
    //         fileExists = QFileInfo::exists(filepaths+"/transcript.xml") && QFileInfo(filepaths+"/transcript.xml").isFile();
    //     }

    //     QFile transcriptFile(filepaths+"/transcript.xml");
    //     if (!transcriptFile.open(QIODevice::ReadOnly)) {
    //         qInfo()<<(transcriptFile.errorString());
    //         return;
    //     }
    //     ui->m_editor->loadTranscriptData(transcriptFile);
    //     ui->m_editor->setContent();
    //     transcriptFile.close();

    //     QFile transcriptFile2(filepaths+"/transcript.xml");
    //     if (!transcriptFile2.open(QIODevice::ReadOnly)) {
    //         qInfo()<<(transcriptFile2.errorString());
    //         return;
    //     }
    //     ui->m_editor_2->loadTranscriptData(transcriptFile2);
    //     ui->m_editor_2->setContent();

    // }

}


void Tool::on_btn_translate_clicked()
{

    QProgressBar progressBar;
    progressBar.setMinimum(0);
    progressBar.setMaximum(100);
    progressBar.setValue(10);
    progressBar.show();
    progressBar.raise();
    progressBar.activateWindow();

    if(!QFile::exists("Translate.py")){
        QFile mapper("Translate.py");
        QFileInfo mapperFileInfo(mapper);
        QFile aligner(":/Translate.py");
        if(!aligner.open(QIODevice::OpenModeFlag::ReadOnly)){
            return;
        }
        aligner.seek(0);
        QString cp=aligner.readAll();
        aligner.close();

        if(!mapper.open(QIODevice::OpenModeFlag::WriteOnly|QIODevice::Truncate)){

            return;
        }
        mapper.write(QByteArray(cp.toUtf8()));
        mapper.close();
        std::string makingexec="chmod +x "+mapperFileInfo.absoluteFilePath().replace(" ", "\\ ").toStdString();
        int result = system(makingexec.c_str());
        // qInfo()<<result; Disabled Debug
    }




    QFile translate("toTranslate.txt");
    if(!translate.open(QIODevice::OpenModeFlag::WriteOnly)){
        QMessageBox::critical(this,"Error",translate.errorString());
        return;
    }
    QString x("");
    for (auto& a_block : std::as_const(ui->m_editor_2->m_blocks)) {
        auto blockText = a_block.text + " " ;
        //            qInfo()<<a_block.text;
        x.append(blockText + "\n");
    }
    QTextStream outer(&translate);
    outer<<x;
    translate.close();

    QFileInfo TranslateFile(translate);
    QString Translatefilepaths=TranslateFile.dir().path();

    QFile myfile(ui->m_editor_2->m_transcriptUrl.toLocalFile());

    QFileInfo fileInfo(myfile);
    QString filename(fileInfo.fileName());
    QString filepaths=fileInfo.dir().path();
    QString translatedFile=filepaths+"/HindiTranslated.xml";
    QString filepaths2=filepaths;
    // qInfo()<<filepaths2;  Disabled Debug



    QFile mapper("Translate.py");
    QFileInfo mapperFileInfo(mapper);

    QFile finalFile(translatedFile);
    QFileInfo finalFileInfo(finalFile);
    QFile HindiTranslated("HindiTranslated.txt");
    QFileInfo HindiTranslate(HindiTranslated);
    QFileInfo initialDictFileInfo(translate);
    int result;
    QFile transcriptFileToTranslate(ui->m_editor_2->m_transcriptUrl.toLocalFile());
    QFileInfo FromTranscriptFileToTranslate(transcriptFileToTranslate);

    QFile tempXML("temp.xml");
    QFileInfo tempXMLinfo(tempXML);

    std::string translatorStr="python3 "  +mapperFileInfo.absoluteFilePath().toStdString()
                                +" "+ '\"'+initialDictFileInfo.absoluteFilePath().toStdString()+ '\"'
                                +" "+ '\"'+HindiTranslate.absoluteFilePath().toStdString()+ '\"'
                                +" "+ '\"'+finalFileInfo.absoluteFilePath().toStdString()+ '\"'
                                +" "+ '\"'+FromTranscriptFileToTranslate.absoluteFilePath().toStdString()+ '\"'
                                +" "+ '\"'+tempXMLinfo.absoluteFilePath().toStdString()+ '\"';

    // qInfo()<<translatorStr.c_str(); // Disabled debug

    int result2 = system(translatorStr.c_str());
    // qInfo()<<result2; // Disabled debug

    // qInfo()<<"Save Pressed"; // Disabled debug
    bool fileExists = QFileInfo::exists(filepaths2+"/HindiTranslated.xml") && QFileInfo(filepaths2+"/HindiTranslated.xml").isFile();
    while(!fileExists){
        fileExists = QFileInfo::exists(filepaths2+"/HindiTranslated.xml") && QFileInfo(filepaths2+"/HindiTranslated.xml").isFile();
    }
    // qInfo()<<"path exists now"; // Disabled debug
    QFile transcriptFile3(filepaths2+"/HindiTranslated.xml");
    if (!transcriptFile3.open(QIODevice::ReadOnly)) {
        qInfo()<<(transcriptFile3.errorString());
        return;
    }
    ui->m_editor_3->loadTranscriptData(transcriptFile3);
    ui->m_editor_3->setContent();
    // qInfo()<<"translated"; // Disabled debug
    progressBar.setValue(100);
    progressBar.hide();


}


void Tool::on_editor_openTranscript_triggered()
{
    ui->m_editor->m_transcriptUrl.clear();
    ui->m_editor->transcriptOpen();
    ui->m_editor_2->m_transcriptUrl=ui->m_editor->m_transcriptUrl;
    QFile transcriptFile(ui->m_editor->m_transcriptUrl.toLocalFile());
    if (!transcriptFile.open(QIODevice::ReadOnly)) {
        qInfo()<<(transcriptFile.errorString());
        return;
    }
    ui->m_editor_2->loadTranscriptData(transcriptFile);
    ui->m_editor_2->setContent();
}





void Tool::on_add_video_clicked()
{
    player->open();
}


void Tool::on_open_transcript_clicked()
{
    Tool::on_editor_openTranscript_triggered();
}


void Tool::on_new_transcript_clicked()
{
    ui->m_editor->close();
}


void Tool::on_save_transcript_clicked()
{
    ui->m_editor->transcriptSave();
}


void Tool::on_save_as_transcript_clicked()
{
    ui->m_editor->transcriptSaveAs();
}


void Tool::on_load_a_custom_dict_clicked()
{
    ui->m_editor->addCustomDictonary();
}


void Tool::on_get_PDF_clicked()
{
    ui->m_editor->saveAsPDF();
}


void Tool::on_decreseFontSize_clicked()
{
    this->changeFontSize(-1);
}


void Tool::on_increaseFontSize_clicked()
{
    this->changeFontSize(+1);

}


void Tool::on_toggleWordEditor_clicked()
{
    ui->m_wordEditor->setVisible(!ui->m_wordEditor->isVisible());
}


void Tool::on_keyboard_shortcuts_clicked()
{
    this->createKeyboardShortcutGuide();
}


void Tool::on_fontComboBox_currentFontChanged(const QFont &f)
{
    //    bool fontSelected;
    font = f;

    //    if (fontSelected)
    setFontForElements();
}

void Tool::on_actionAbout_triggered()
{

    if (!about)
    {
        about = new About(this);
        about->setAttribute(Qt::WA_DeleteOnClose);
        about->show();

        connect(about, &QObject::destroyed, [this]() {
            about = nullptr;
        });
    } else {
        about->raise();
        about->activateWindow();
    }
}


void Tool::on_actionInit_triggered()
{
    git->init();
}


void Tool::on_actionAdd_All_triggered()
{
    git->add();
}


void Tool::on_actionCommit_triggered()
{
    git->commit();
}


void Tool::on_actionAdd_Remote_URL_triggered()
{
    git->addRemoteUrl();
}


void Tool::on_actionPush_triggered()
{
    git->push();
}


void Tool::on_actionPull_triggered()
{
    git->pull();
}


// bool Tool::eventFilter(QObject *obj, QEvent *event)
// {
//     if (event->type() == QEvent::Drop) {
//         QDropEvent *dropEvent = static_cast<QDropEvent*>(event);
//         const QMimeData* mimeData = dropEvent->mimeData();
//         if (mimeData->hasUrls()) {
//             QList<QUrl> urls = mimeData->urls();
//             // Check which layout the drop event occurred on
//             if (obj == ui->playerLayout) {
//                 // Handle drop event for layout1
//             } else if (obj == ui->verticalLayout) {
//                 // Handle drop event for layout2
//             }
//             // Handle dropped files as needed
//             for (const QUrl& url : urls) {
//             }
//         }
//         return true;
//     }
//     return QMainWindow::eventFilter(obj, event);
// }

void Tool::dragEnterEvent(QDragEnterEvent *event) {
    QDragEnterEvent* dragEnterEvent = static_cast<QDragEnterEvent*>(event);
    if (dragEnterEvent->mimeData()->hasUrls()) {
        dragEnterEvent->acceptProposedAction();
    }
}

void Tool::dragMoveEvent(QDragMoveEvent *event) {
    event->accept();
    event->acceptProposedAction();
}

void Tool::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QUrl* url = new QUrl(mimeData->urls().first());
        QString filePath = url->toLocalFile();
        QFileInfo fileInfo(filePath);
        QString extension = fileInfo.suffix().toLower();

        if (extension == "xml") {
            ui->m_editor->loadTranscriptFromUrl(url);
        }
        else {
            player->loadMediaFromUrl(url);
        }
        event->acceptProposedAction();


    }
}

bool Tool::isDroppedOnLayout(const QPoint &pos, QVBoxLayout *layout) {
    for (int i = 0; i < layout->count(); ++i) {
        if (layout->itemAt(i)->geometry().contains(pos)) {
            return true;
        }
    }
    return false;
}

void Tool::on_actionShow_Waveform_triggered()
{
    ui->widget->showWaveForm();
}

void Tool::onTabChanged(int index) {
    if (index > 0) {  // Tab 2 index
        ui->mediaFilenameLbl->hide();
        ui->transcriptFilenameLbl->hide();
    } else {
        ui->mediaFilenameLbl->show();
        ui->transcriptFilenameLbl->show();
    }
}

void Tool::resizeEvent(QResizeEvent *event)  {
    QMainWindow::resizeEvent(event);
}



// To prevent auto updates on timestamps
void Tool::on_actionSave_Timestamps_triggered()
{
    ui->widget->updateTimestampsToggle();
}


void Tool::on_actionUpdate_Timestamps_triggered()
{
    ui->widget->updateTimeStamps();
}


void Tool::on_actionShow_Endlines_triggered()
{
    ui->m_editor->showWaveform();
}

void Tool::setDefaultAudioOutputDevice() {

    // QAudioDevice device = QMediaDevices::defaultAudioOutput();
    // if (!device.isNull()) {
    //     m_audioOutput->setDevice(device);
    //     qDebug() << "Audio output device set to:" << device.description();
    // } else {
    //     qDebug() << "No audio output devices available.";
    // }
}


void Tool::on_actionOpen_triggered()
{
    ui->tableWidget->openTTSTranscript();
}


void Tool::on_actionIncrease_speed_by_1_triggered()
{
    seekSpeed = std::min(seekSpeed + 1, (int64_t)5);
    settings->setValue("seekSpeed", seekSpeed);

    QString message = QString("Speed increased to %1").arg(seekSpeed);
    QToolTip::showText(QCursor::pos(), message, this, QRect(), 1500);
}


void Tool::on_actionDecrease_speed_by_1_triggered()
{
    seekSpeed = std::max(seekSpeed - 1, (int64_t)1);
    settings->setValue("seekSpeed", seekSpeed);

    QString message = QString("Speed decreased to %1").arg(seekSpeed);
    QToolTip::showText(QCursor::pos(), message, this, QRect(), 1500);
}

