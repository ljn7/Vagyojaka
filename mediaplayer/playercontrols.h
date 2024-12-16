#pragma once

#include <QMediaPlayer>
#include <QWidget>
#include <QAbstractButton>
#include <QAbstractSlider>
#include <QComboBox>
#include <QLabel>

class PlayerControls : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerControls(QWidget *parent = nullptr);

    QMediaPlayer::PlaybackState state() const;
    float volume() const;
    bool isMuted() const;
    qreal playbackRate() const;

public slots:
    void setState(QMediaPlayer::PlaybackState state);
    void setVolume(float volume);
    void setMuted(bool muted);
    void setPlaybackRate(float rate);

signals:
    void play();
    void pause();
    void stop();
    void seekForward();
    void seekBackward();
    void changeVolume(float volume);
    void changeMuting(bool muting);
    void changeRate(qreal rate);
    void splitClick();

private slots:
    void playClicked();
    void muteClicked();
    void updateRate();
    void onVolumeSliderValueChanged();
    void splitClicked();

private:
    QMediaPlayer::PlaybackState m_playerState = QMediaPlayer::StoppedState;
    bool m_playerMuted = false;
    QAbstractButton *m_playButton = nullptr;
    QAbstractButton *m_stopButton = nullptr;
    QAbstractButton *m_seekForwardButton = nullptr;
    QAbstractButton *m_seekBackwardButton = nullptr;
    QAbstractButton *m_muteButton = nullptr;
    QAbstractSlider *m_volumeSlider = nullptr;
    QComboBox *m_rateBox = nullptr;
    QAbstractButton *m_splitButton = nullptr;
};
