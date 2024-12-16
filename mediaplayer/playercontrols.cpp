#include "playercontrols.h"

#include <QBoxLayout>
#include <QSlider>
#include <QStyle>
#include <QToolButton>
#include <QComboBox>
#include <QAudio>

PlayerControls::PlayerControls(QWidget *parent)
    : QWidget(parent)
{
    m_playButton = new QToolButton(this);
    m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playButton->setToolTip("Play");

    connect(m_playButton, &QAbstractButton::clicked, this, &PlayerControls::playClicked);

    m_stopButton = new QToolButton(this);
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_stopButton->setEnabled(false);
    m_stopButton->setToolTip("Stop");

    connect(m_stopButton, &QAbstractButton::clicked, this, &PlayerControls::stop);

    m_seekForwardButton = new QToolButton(this);
    m_seekForwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
    m_seekForwardButton->setToolTip("Forward");


    connect(m_seekForwardButton, &QAbstractButton::clicked, this, &PlayerControls::seekForward);

    m_seekBackwardButton = new QToolButton(this);
    m_seekBackwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
    m_seekBackwardButton->setToolTip("Backward");

    connect(m_seekBackwardButton, &QAbstractButton::clicked, this, &PlayerControls::seekBackward);

    m_muteButton = new QToolButton(this);
    m_muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    m_muteButton->setToolTip("Mute");

    connect(m_muteButton, &QAbstractButton::clicked, this, &PlayerControls::muteClicked);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setToolTip("Volume");

    connect(m_volumeSlider, &QSlider::valueChanged, this, &PlayerControls::onVolumeSliderValueChanged);

    m_rateBox = new QComboBox(this);

    m_rateBox->addItem("0.8x", QVariant(0.8));
    m_rateBox->addItem("0.9x", QVariant(0.9));
    m_rateBox->addItem("1.0x", QVariant(1.0));
    m_rateBox->addItem("1.1x", QVariant(1.1));
    m_rateBox->addItem("1.2x", QVariant(1.2));
    m_rateBox->setCurrentIndex(2);
    m_rateBox->setToolTip("Speed");

    connect(m_rateBox, QOverload<int>::of(&QComboBox::activated), this, &PlayerControls::updateRate);

    m_splitButton = new QToolButton(this);
    m_splitButton->setIcon(QIcon(":/images/images/cutting-tool.png"));
    connect(m_splitButton, &QAbstractButton::clicked, this, &PlayerControls::splitClicked);
    m_splitButton->setToolTip("Split Media");


    QBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stopButton);
    layout->addWidget(m_seekBackwardButton);
    layout->addWidget(m_playButton);
    layout->addWidget(m_seekForwardButton);
    layout->addWidget(m_muteButton);
    layout->addWidget(m_volumeSlider);
    layout->addWidget(m_rateBox);
    layout->addWidget(m_splitButton);

    setLayout(layout);
}

//Qt6
//State => PlaybackState
QMediaPlayer::PlaybackState PlayerControls::state() const
{
    return m_playerState;
}

//Qt6
//State => PlaybackState
void PlayerControls::setState(QMediaPlayer::PlaybackState state)
{
    if (state != m_playerState) {
        m_playerState = state;
        switch (state) {
        case QMediaPlayer::StoppedState:
            m_stopButton->setEnabled(false);
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            break;
        case QMediaPlayer::PlayingState:
            m_stopButton->setEnabled(true);
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
            break;
        case QMediaPlayer::PausedState:
            m_stopButton->setEnabled(true);
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            break;
        }
    }
}

float PlayerControls::volume() const
{
    qreal linearVolume =  QAudio::convertVolume(m_volumeSlider->value() / qreal(100),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);

    return linearVolume;
}

void PlayerControls::setVolume(float volume)
{
    qreal logarithmicVolume = QAudio::convertVolume(volume,
                                                    QAudio::LinearVolumeScale,
                                                    QAudio::LogarithmicVolumeScale);

    m_volumeSlider->setValue(qRound(logarithmicVolume * 100));
}

bool PlayerControls::isMuted() const
{
    return m_playerMuted;
}

void PlayerControls::setMuted(bool muted)
{
    if (muted != m_playerMuted) {
        m_playerMuted = muted;

        m_muteButton->setIcon(style()->standardIcon(muted
                                                        ? QStyle::SP_MediaVolumeMuted
                                                        : QStyle::SP_MediaVolume));
    }
}

void PlayerControls::playClicked()
{
    switch (m_playerState) {
    case QMediaPlayer::StoppedState:
    case QMediaPlayer::PausedState:
        emit play();
        break;
    case QMediaPlayer::PlayingState:
        emit pause();
        break;
    }
}

void PlayerControls::muteClicked()
{
    emit changeMuting(!m_playerMuted);
}

qreal PlayerControls::playbackRate() const
{
    return m_rateBox->itemData(m_rateBox->currentIndex()).toDouble();
}

void PlayerControls::setPlaybackRate(float rate)
{
    for (int i = 0; i < m_rateBox->count(); ++i) {
        if (qFuzzyCompare(rate, float(m_rateBox->itemData(i).toDouble()))) {
            m_rateBox->setCurrentIndex(i);
            return;
        }
    }

    m_rateBox->addItem(QString("%1x").arg(rate), QVariant(rate));
    m_rateBox->setCurrentIndex(m_rateBox->count() - 1);
}

void PlayerControls::updateRate()
{
    emit changeRate(playbackRate());
}

void PlayerControls::onVolumeSliderValueChanged()
{
    emit changeVolume(volume());
}

void PlayerControls::splitClicked()
{
    emit splitClick();
}
