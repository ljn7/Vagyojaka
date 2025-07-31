#include "audioplayerwidget.h"
#include "qfileinfo.h"
#include <QBoxLayout>
#include <QToolButton>
#include <QStyle>
#include <QTime>

AudioPlayerWidget::AudioPlayerWidget(const QString &audioFilePath, QWidget *parent)
    : QWidget(parent), m_audioFilePath(audioFilePath), m_playerState(QMediaPlayer::StoppedState)
{
    m_audioOutput = std::make_unique<QAudioOutput>(this);
    m_mediaPlayer = std::make_unique<QMediaPlayer>(this);
    m_mediaPlayer->setSource(QUrl::fromLocalFile(audioFilePath));
    m_mediaPlayer->setAudioOutput(m_audioOutput.get());

    setupUi();
    connectSignalsAndSlots();
}

void AudioPlayerWidget::setupUi()
{
    m_playButton = new QToolButton(this);
    m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playButton->setIconSize(QSize(10, 10));

    m_stopButton = new QToolButton(this);
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_stopButton->setIconSize(QSize(10, 10));
    m_stopButton->setEnabled(false);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_rateBox = new QComboBox(this);
    m_rateBox->addItem("0.8x", QVariant(0.8));
    m_rateBox->addItem("0.9x", QVariant(0.9));
    m_rateBox->addItem("1.0x", QVariant(1.0));
    m_rateBox->addItem("1.1x", QVariant(1.1));
    m_rateBox->addItem("1.2x", QVariant(1.2));
    m_rateBox->addItem("1.3", QVariant(1.3));
    m_rateBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_rateBox->setFont(QFont(m_rateBox->font().family(), 8));
    m_rateBox->setCurrentIndex(2);

    m_labelDuration = new QLabel(this);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(m_playButton);
    hLayout->addWidget(m_stopButton);
    hLayout->addWidget(m_slider);
    hLayout->addWidget(m_rateBox);
    hLayout->addWidget(m_labelDuration);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(hLayout);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

void AudioPlayerWidget::connectSignalsAndSlots()
{
    connect(m_playButton, &QAbstractButton::clicked, this, &AudioPlayerWidget::playClicked);
    connect(m_stopButton, &QAbstractButton::clicked, m_mediaPlayer.get(), &QMediaPlayer::stop);
    connect(m_slider, &QSlider::sliderMoved, this, &AudioPlayerWidget::seek);
    connect(m_rateBox, QOverload<int>::of(&QComboBox::activated), this, &AudioPlayerWidget::updateRate);

    connect(m_mediaPlayer.get(), &QMediaPlayer::positionChanged, this, &AudioPlayerWidget::positionChanged);
    connect(m_mediaPlayer.get(), &QMediaPlayer::durationChanged, this, &AudioPlayerWidget::durationChanged);
    connect(m_mediaPlayer.get(), &QMediaPlayer::playbackStateChanged, this, &AudioPlayerWidget::setState);
    connect(m_mediaPlayer.get(), &QMediaPlayer::errorOccurred, this, &AudioPlayerWidget::handleError);
}

void AudioPlayerWidget::playClicked()
{
    switch (m_playerState) {
    case QMediaPlayer::StoppedState:
    case QMediaPlayer::PausedState:
        m_mediaPlayer->play();
        break;
    case QMediaPlayer::PlayingState:
        m_mediaPlayer->pause();
        break;
    }
}

void AudioPlayerWidget::seek(int mseconds)
{
    m_mediaPlayer->setPosition(mseconds);
}

void AudioPlayerWidget::positionChanged(qint64 progress)
{
    m_slider->setValue(progress);
    updateDurationInfo(progress / 1000);
}

void AudioPlayerWidget::durationChanged(qint64 duration)
{
    m_slider->setRange(0, duration);
    m_duration = duration / 1000;
    m_slider->setMaximum(duration);
}

void AudioPlayerWidget::updateDurationInfo(qint64 currentInfo)
{
    QString tStr;
    if (currentInfo || m_duration) {
        QTime currentTime((currentInfo / 3600) % 60, (currentInfo / 60) % 60, currentInfo % 60,
                          (currentInfo * 1000) % 1000);
        QTime totalTime((m_duration / 3600) % 60, (m_duration / 60) % 60, m_duration % 60,
                        (m_duration * 1000) % 1000);
        QString format = "mm:ss";
        if (m_duration > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    m_labelDuration->setText(tStr);
}

void AudioPlayerWidget::updateRate()
{
    m_mediaPlayer->setPlaybackRate(playbackRate());
}

qreal AudioPlayerWidget::playbackRate() const
{
    return m_rateBox->itemData(m_rateBox->currentIndex()).toDouble();
}

void AudioPlayerWidget::setState(QMediaPlayer::PlaybackState state)
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

QMediaPlayer::PlaybackState AudioPlayerWidget::state() const
{
    return m_playerState;
}

void AudioPlayerWidget::handleError(QMediaPlayer::Error error, const QString &errorString)
{
    qWarning() << "Media player error:" << error << errorString;
    // Here you could also emit a signal or show an error message to the user
}

QString AudioPlayerWidget::getAudioFileName(bool includeExtension) const
{
    QFileInfo fileInfo(m_audioFilePath);
    if (includeExtension) {
        return fileInfo.fileName();
    } else {
        return fileInfo.completeBaseName();
    }
}

AudioPlayerWidget::~AudioPlayerWidget() = default;

void AudioPlayerWidget::stop() {
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }
}

bool AudioPlayerWidget::isPlaying() const {
    return m_playerState == QMediaPlayer::PlayingState;
}

void AudioPlayerWidget::togglePlayPause() {
    playClicked();
}
