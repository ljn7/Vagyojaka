#ifndef AUDIOPLAYERWIDGET_H
#define AUDIOPLAYERWIDGET_H

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QToolButton>
#include <memory>

class AudioPlayerWidget : public QWidget {
    Q_OBJECT

public:
    explicit AudioPlayerWidget(const QString &audioFilePath, QWidget *parent = nullptr);
    ~AudioPlayerWidget();

    QMediaPlayer::PlaybackState state() const;
    QString getAudioFileName(bool includeExtension = true) const;
    bool isPlaying() const;
    void togglePlayPause();

public slots:
    void playClicked();
    void seek(int mseconds);
    void updateRate();
    void stop();

private slots:
    void positionChanged(qint64 progress);
    void durationChanged(qint64 duration);
    void setState(QMediaPlayer::PlaybackState state);
    void handleError(QMediaPlayer::Error error, const QString &errorString);

private:
    void setupUi();
    void connectSignalsAndSlots();
    void updateDurationInfo(qint64 currentInfo);
    qreal playbackRate() const;

    QString m_audioFilePath;
    std::unique_ptr<QMediaPlayer> m_mediaPlayer;
    std::unique_ptr<QAudioOutput> m_audioOutput;
    QToolButton *m_playButton;
    QToolButton *m_stopButton;
    QSlider *m_slider;
    QLabel *m_labelDuration;
    QComboBox *m_rateBox;
    qint64 m_duration = 0;
    QMediaPlayer::PlaybackState m_playerState = QMediaPlayer::StoppedState;
};

#endif // AUDIOPLAYERWIDGET_H
