#ifndef BACKGROUNDMIXERDIALOG_H
#define BACKGROUNDMIXERDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QMediaPlayer>
#include <QAudioOutput>

class BackgroundMixerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BackgroundMixerDialog(const QString &voiceFilePath, QWidget *parent = nullptr);
    ~BackgroundMixerDialog();

    QString getSelectedJingle() const;
    float getVolume() const; // Retourne entre 0.0 et 1.0
    float getOffsetSeconds() const;

private slots:
    void togglePreview();
    void onVolumeChanged(int value);
    void onOffsetChanged(int value); 
    void onVoiceStatusChanged(QMediaPlayer::MediaStatus status);
    void updateJingleSelection(int index);

private:
    void setupUi();
    void stopPreview();

    QString m_voicePath;
    
    // UI
    QComboBox *comboJingles;
    QSlider *sliderVolume;
    QLabel *lblVolumeValue;
    QPushButton *btnPreview;
    QSlider *sliderOffset;      
    QLabel *lblOffsetValue;
    // Audio
    QMediaPlayer *playerVoice;
    QAudioOutput *outVoice;
    
    QMediaPlayer *playerMusic;
    QAudioOutput *outMusic;
    
    bool isPreviewing;
};

#endif // BACKGROUNDMIXERDIALOG_H
