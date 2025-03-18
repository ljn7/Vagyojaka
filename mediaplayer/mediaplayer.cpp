#include "mediaplayer.h"
#include "qapplication.h"
#include "qaudiodevice.h"
#include "qmediametadata.h"
#include "qmediadevices.h"
#include <iostream>

MediaPlayer::MediaPlayer(QWidget *parent)
    : QMediaPlayer(parent)
{
    audioOutput = new QAudioOutput(this);
    this->setAudioOutput(audioOutput);

    this->supportedFormats = {
        "Audio Files (*.mp3 *.ogg *.oga *.mka *.wav *.m4a *.aac *.flac *.wv *.mpc *.ape *.alac *.amr "
        "*.tta *.ac3 *.dts *.ra *.opus *.spx *.aif *.aiff *.aifc *.caf *.tak *.shr)",

        "Video Files (*.avi *.divx *.amv *.mpg *.mpeg *.mpe *.m1v *.m2v *.mpv2 "
        "*.mp2v *.m2p *.vob *.evo *.mod *.ts *.m2ts *.m2t *.mts *.pva *.tp *.tpr "
        "*.mp4 *.m4v *.mp4v *.mpv4 *.hdmov *.mov *.3gp *.3gpp *.3g2 *.3gp2 *.mkv "
        "*.webm *.ogm *.ogv *.flv *.f4v *.wmv *.asf *.rmvb *.rm *.dv *.mxf *.dav)",

        "All Files (*)"
    };
    QString iniPath = QApplication::applicationDirPath() + "/" + "config.ini";
    settings = new QSettings(iniPath, QSettings::IniFormat);
    connect(&m_mediaDevices, &QMediaDevices::audioOutputsChanged, [this]() {
        setDefaultAudioOutputDevice();
    });
}

QTime MediaPlayer::elapsedTime()
{
    return getTimeFromPosition(position());
}

QTime MediaPlayer::durationTime()
{
    return getTimeFromPosition(duration());
}

void MediaPlayer::setPositionToTime(const QTime& time)
{
    if (time.isNull())
        return;
    qint64 position = 3600000*time.hour() + 60000*time.minute() + 1000*time.second() + time.msec();
    setPosition(position);
}

QString MediaPlayer::getMediaFileName()
{
    return std::as_const(m_mediaFileName);
}

QString MediaPlayer::getPositionInfo()
{
    QString format = "mm:ss.zzz";
    if (durationTime().hour() != 0)
        format = "hh:mm:ss.zzz";

    return elapsedTime().toString(format) + " / " + durationTime().toString(format);
}

bool MediaPlayer::isAudioFile(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    QStringList audioExtensions = {/*"mp3", */"wav" /*, "ogg", "flac", "aac"*//*, "m4a"*/}; // Add more audio extensions as needed

    return audioExtensions.contains(fileInfo.suffix().toLower());
}

bool MediaPlayer::isVideoFile(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    QStringList videoExtensions = {"mp4", "avi", "mkv", "mov", "wmv"}; // Add more video extensions as needed

    return videoExtensions.contains(fileInfo.suffix().toLower());
}

void MediaPlayer::loadMediaFromUrl(QUrl *fileUrl)
{
    QFile MediaFile(fileUrl->toLocalFile());
    if(!MediaFile.open(QIODevice::ReadOnly))
    {
        qInfo()<<"Not open for readonly\n";
    }
    // p = new QMediaPlayer(this);
    // QString filepath;

    // filepath = fileUrl->toLocalFile();
    QFileInfo filedir(MediaFile);
    QString dirInString=filedir.dir().path();
    settings->setValue("mediaDir",dirInString);
    QString slash = "/";
        // (_WIN32)? ("/"): ("\\");
    m_mediaFileName = dirInString + slash + fileUrl->fileName();
    //Qt6
    // setMedia(*fileUrl);
    setSource(*fileUrl);

    emit sendMediaUrl(*fileUrl);
    //waveform
    // QByteArray audioData;
    // if(isAudioFile(filepath)){
    //     qInfo()<<"File id audio file\n";
    //     audioData = MediaFile.readAll();
    // }
    // else
    // {
    //     qInfo()<<"File is video file\n";
    //     QString ffmpegPath = "ffmpeg";
    //     // #ifdef Q_OS_WIN
    //     //             ffmpegPath = QCoreApplication::applicationDirPath() + "/ffmpeg";
    //     // #endif
    //     QProcess ffmpegProcess;
    //     QStringList ffmpegArgs = {"-i", filepath, "-vn", "-f", "wav", "-"};
    //     ffmpegProcess.start(ffmpegPath, ffmpegArgs);

    //     if (ffmpegProcess.waitForStarted() && ffmpegProcess.waitForFinished()) {
    //         audioData += ffmpegProcess.readAllStandardOutput();
    //     } else {
    //         qWarning() << "FFmpeg process failed:" << ffmpegProcess.errorString();
    //     }
    // }

    // audioBuffer.open(QIODevice::ReadWrite | QIODevice::Truncate);
    // audioBuffer.buffer().clear();
    // audioBuffer.seek(0);

    // audioBuffer.write(audioData);
    // audioBuffer.close();

    // p->setSource(*fileUrl);

    // std::cerr << QDir::currentPath().toStdString() << std::endl;

    // connect(p, &QMediaPlayer::durationChanged, [this]() {
    //     qint64 tot_duration = p->duration();
    //     qInfo()<<"size of buffer is: "<<audioBuffer.size();
    //     AVFormatContext* formatContext = avformat_alloc_context();
    //     qint64 sampleRate = 0;

    //     if ( avformat_open_input(&formatContext,
    //             strdup(m_mediaFileName.toStdString().c_str()),
    //             nullptr, nullptr) == 0)
    //     {
    //         if (avformat_find_stream_info(formatContext, nullptr) >= 0) {
    //             int audioStreamIndex = -1;
    //             for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
    //                 if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
    //                     audioStreamIndex = i;
    //                     break;
    //                 }
    //             }

    //             if (audioStreamIndex != -1) {
    //                 sampleRate = formatContext->streams[audioStreamIndex]->codecpar->sample_rate;
    //                 /*qDebug()*/std::cerr << "Sample Rate from FFMPEG:" << sampleRate;
    //             } else {
    //                 qWarning() << "No audio stream found.";
    //             }
    //         } else {
    //             qWarning() << "Failed to find stream information.";
    //         }

    //         avformat_close_input(&formatContext);
    //     }
    //     qint64 duration = p->duration();
    //     if(sampleRate) {
    //         emit sendSampleRate(sampleRate, audioBuffer, duration);
    //         emit sendingSampleRateStatus(true);
    //     }
    //     else {
    //         emit sendingSampleRateStatus(false);
    //     }
    // });

    // connect(p, &QMediaPlayer::metaDataChanged, [this]() {
    //     if (!p->metaData().isEmpty()) {
    //         qint64 sampleRate = p->metaData().AudioBitRate;
    //         qint64 duration = p->duration();
    //         std::cerr << "Sample rate from meta: " << sampleRate;
    //         // if (sampleRate) {
    //         //     emit sendSampleRate(sampleRate, audioBuffer, duration);
    //         //     emit sendingSampleRateStatus(true);
    //         // } else {
    //         //     emit sendingSampleRateStatus(false);
    //         // }
    //     }
    // });

    //=======================
    emit message("Opened file " + fileUrl->fileName());
    emit openMessage(fileUrl->fileName());
    play();
}

void MediaPlayer::open()
{
    QFileDialog fileDialog;
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open Media"));
    //Qt6 porting
    // QStringList supportedMimeTypes = QMediaPlayer::supportedMimeTypes();
    // if (!supportedMimeTypes.isEmpty())
    //     fileDialog.setMimeTypeFilters(supportedMimeTypes);

    // fileDialog.setMimeTypeFilters(supportedFormats);


    fileDialog.setNameFilters(supportedFormats);
    if(settings->value("mediaDir").toString()=="")
        fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath()));
    else
        fileDialog.setDirectory(settings->value("mediaDir").toString());
    if (fileDialog.exec() == QDialog::Accepted) {
        QUrl *fileUrl = new QUrl(fileDialog.selectedUrls().constFirst());
        loadMediaFromUrl(fileUrl);
    }
}

void MediaPlayer::seek(int seconds)
{
    if (elapsedTime().addSecs(seconds) > durationTime())
        setPosition(duration());
    else if (elapsedTime().addSecs(seconds).isNull())
        setPosition(0);
    else
        setPositionToTime(elapsedTime().addSecs(seconds));
}

QTime MediaPlayer::getTimeFromPosition(const qint64& position)
{
    auto milliseconds = position % 1000;
    auto seconds = (position/1000) % 60;
    auto minutes = (position/60000) % 60;
    auto hours = (position/3600000) % 24;

    return QTime(hours, minutes, seconds, milliseconds);
}

//Qt6
void MediaPlayer::togglePlayback()
{
    if (playbackState() == MediaPlayer::PausedState || playbackState() == MediaPlayer::StoppedState)
        play();
    else if (playbackState() == MediaPlayer::PlayingState)
        pause();
}

// // Qt6

// void MediaPlayer::setMuted(bool muted)
// {
//     audioOutput()->setMuted(muted);
// }

void MediaPlayer::setDefaultAudioOutputDevice() {

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isNull()) {
        audioOutput->setDevice(device);
        qDebug() << "Audio output device set to:" << device.description();
    } else {
        qDebug() << "No audio output devices available.";
    }
}
