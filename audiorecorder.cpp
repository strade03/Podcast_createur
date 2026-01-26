#include "audiorecorder.h"
#include <QVBoxLayout>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QMediaFormat>
#include <QFile>
#include <QDebug>

AudioRecorder::AudioRecorder(const QString &scriptPath, QWidget *parent)
    : QDialog(parent)
    , durationSec(0)
{
    setWindowTitle(tr("Enregistrement Studio"));
    resize(400, 600);

    if (!scriptPath.isEmpty()) {
        QFile f(scriptPath);
        if (f.open(QIODevice::ReadOnly)) {
            scriptContent = QString::fromUtf8(f.readAll());
        }
    }

    audioInput = new QAudioInput(this);
    recorder = new QMediaRecorder(this);
    session.setAudioInput(audioInput);
    session.setRecorder(recorder);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &AudioRecorder::updateDuration);

    setupUi();

    const auto devices = QMediaDevices::audioInputs();
    for (const QAudioDevice &device : devices) {
        deviceCombo->addItem(device.description(), QVariant::fromValue(device));
    }
    
    if (!devices.isEmpty()) updateInputDevice(deviceCombo->currentIndex());

    connect(deviceCombo, &QComboBox::currentIndexChanged, this, &AudioRecorder::updateInputDevice);
    connect(recorder, &QMediaRecorder::recorderStateChanged, this, &AudioRecorder::onStateChanged);
}

AudioRecorder::~AudioRecorder() {}

void AudioRecorder::setOutputFile(const QString &path) {
    outputFilePath = path;
}

void AudioRecorder::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(15);

    // Script style "prompteur"
    QLabel *lblScript = new QLabel("Votre Texte :", this);
    lblScript->setStyleSheet("font-weight: bold; color: #555;");
    layout->addWidget(lblScript);

    scriptView = new QTextEdit(this);
    scriptView->setReadOnly(true);
    scriptView->setText(scriptContent.isEmpty() ? "Aucun script disponible." : scriptContent);
    scriptView->setStyleSheet("font-size: 16px; line-height: 1.5; padding: 10px; border: 1px solid #CCC; border-radius: 5px; background: #FAFAFA; color: #333;");
    layout->addWidget(scriptView, 1);

    // Contrôles
    layout->addWidget(new QLabel(tr("Microphone :"), this));
    deviceCombo = new QComboBox(this);
    layout->addWidget(deviceCombo);

    timeLabel = new QLabel("00:00", this);
    timeLabel->setAlignment(Qt::AlignCenter);
    QFont f = timeLabel->font(); f.setPointSize(36); f.setBold(true);
    timeLabel->setFont(f);
    timeLabel->setStyleSheet("color: #333;");
    layout->addWidget(timeLabel);

    recordButton = new QPushButton(this);
    recordButton->setFixedSize(80, 80);
    recordButton->setIcon(QIcon(":/icones/ic_record.png")); // Utilisation icone standard
    recordButton->setIconSize(QSize(60,60));
    recordButton->setStyleSheet("border: none; border-radius: 40px; background-color: transparent;"); 
    
    connect(recordButton, &QPushButton::clicked, this, &AudioRecorder::toggleRecord);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(recordButton);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    statusLabel = new QLabel(tr("Prêt à enregistrer"), this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #666; font-style: italic;");
    layout->addWidget(statusLabel);
}

void AudioRecorder::updateInputDevice(int index)
{
    if (index >= 0) {
        auto device = deviceCombo->itemData(index).value<QAudioDevice>();
        audioInput->setDevice(device);
    }
}

void AudioRecorder::toggleRecord()
{
    if (recorder->recorderState() == QMediaRecorder::StoppedState) {
        if (outputFilePath.isEmpty()) return;
        
        if (QFile::exists(outputFilePath)) {
            QFile::remove(outputFilePath);
        }
        // --- CONFIGURATION QUALITÉ AUDIO CRITIQUE ---
        recorder->setOutputLocation(QUrl::fromLocalFile(outputFilePath));
        
        QMediaFormat format;
        format.setFileFormat(QMediaFormat::Wave);
        format.setAudioCodec(QMediaFormat::AudioCodec::Unspecified); // PCM brut
        recorder->setMediaFormat(format);
        
        // Forcer des paramètres standards pour l'édition
        recorder->setAudioSampleRate(44100);
        recorder->setAudioChannelCount(1); // Mono (mieux pour la voix et l'affichage wave)
        recorder->setAudioBitRate(128000); 
        recorder->setQuality(QMediaRecorder::VeryHighQuality);
        
        recorder->record();

        durationSec = 0;
        timeLabel->setText("00:00");
        timer->start(1000);
        deviceCombo->setEnabled(false);
    } else {
        recorder->stop();
        timer->stop();
        deviceCombo->setEnabled(true);
        accept(); 
    }
}

void AudioRecorder::updateDuration()
{
    durationSec++;
    timeLabel->setText(QString("%1:%2").arg(durationSec / 60, 2, 10, QChar('0')).arg(durationSec % 60, 2, 10, QChar('0')));
}

void AudioRecorder::onStateChanged(QMediaRecorder::RecorderState state)
{
    if (state == QMediaRecorder::RecordingState) {
        recordButton->setIcon(QIcon(":/icones/ic_stop.png")); // Suppose ic_stop
        statusLabel->setText(tr("Enregistrement en cours..."));
        statusLabel->setStyleSheet("color: #D32F2F; font-weight: bold;");
    } else {
        recordButton->setIcon(QIcon(":/icones/ic_record.png"));
        statusLabel->setText(tr("Arrêté"));
        statusLabel->setStyleSheet("color: #666; font-style: italic;");
    }
}