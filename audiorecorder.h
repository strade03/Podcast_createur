#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QDialog>
#include <QMediaCaptureSession>
#include <QAudioInput>
#include <QMediaRecorder>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QTextEdit>

class AudioRecorder : public QDialog
{
    Q_OBJECT
public:
    // Constructeur adapté pour recevoir le script path
    explicit AudioRecorder(const QString &scriptPath = "", QWidget *parent = nullptr);
    ~AudioRecorder();
    
    void setOutputFile(const QString &path);

private slots:
    void toggleRecord();
    void updateDuration();
    void onStateChanged(QMediaRecorder::RecorderState state);
    void updateInputDevice(int index);

private:
    void setupUi();

    QMediaCaptureSession session;
    QAudioInput *audioInput;
    QMediaRecorder *recorder;
    
    QLabel *timeLabel;
    QLabel *statusLabel;
    QPushButton *recordButton;
    QComboBox *deviceCombo;
    QTextEdit *scriptView; // Pour afficher le script
    
    QTimer *timer;
    qint64 durationSec;
    QString outputFilePath;
    QString scriptContent;
};

#endif // AUDIORECORDER_H