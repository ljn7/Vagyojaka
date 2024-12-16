#pragma once

#include <QWidget>
#include <QMediaPlayer>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTime>
#include <QSettings>

//New Include
#include <QAudioOutput>
#include<QBuffer>
#include"fftw3.h"
#include"qcustomplot.h"
#include "qmediadevices.h"
#include<QProcess>
#include<QAudioFormat>
#include<QAudioDecoder>
#include <QFileInfo>
#include <QDebug>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

class MediaPlayer : public QMediaPlayer
{
    Q_OBJECT
public:
    explicit MediaPlayer(QWidget *parent = nullptr);
    QTime elapsedTime();
    QTime durationTime();
    void setPositionToTime(const QTime& time);
    QString getMediaFileName();
    QString getPositionInfo();
    QStringList supportedFormats;
    QAudioOutput *audioOutput = nullptr;

    void loadMediaFromUrl(QUrl *fileUrl);

    void resampleAudio(const char* inputPath, const char* outputPath, int targetSamplingRate);

public slots:
    void open();
    void seek(int seconds);
    void togglePlayback();
    void setVolume(float volume) { audioOutput->setVolume(volume); };
    void setMuted(bool muted) { audioOutput->setMuted(muted); };
    // void setMuted(bool muted);

signals:
    void message(QString text, int timeout = 5000);
    void openMessage(QString text);
    void sendMediaUrl(QUrl mediaUrl);

    void sendBuffer(QBuffer& audio);
    void sendDuration(qint64 total_duration);
    void sendSampleRate(qint64 sampleRate, QBuffer& audioBuffer, qint64 totalDuration);
    void sendingSampleRateStatus(bool status);

private:
    static QTime getTimeFromPosition(const qint64& position);
    void setDefaultAudioOutputDevice();
    QString m_mediaFileName;
    QSettings* settings;

    bool isAudioFile(const QString& filePath);
    bool isVideoFile(const QString& filePath);
    // QBuffer audioBuffer;
    QMediaPlayer *p;
    qlonglong sampleRate;
    QMediaDevices m_mediaDevices;
    float m_amplification = 1.0f;


};
