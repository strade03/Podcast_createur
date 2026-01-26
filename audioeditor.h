#pragma once

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QCoreApplication>
#include "waveformwidget.h"
#include <QLabel>
#include <QWidget>
#include <QCloseEvent>
#include <QProgressDialog>

#include "audiomerger.h" 
// debut modif
namespace Ui {
    class AudioEditorWidget;
}

// Pour changer le mode autonome ou boite de dialogue pour son_fusion
// ajouter DEFINES += USE_DIALOG_MODE dans .pro pour passer en mode dialog
#ifdef USE_DIALOG_MODE
#include <QDialog>
#define AUDIOEDITOR_BASE QDialog
#else
#include <QMainWindow>
#define AUDIOEDITOR_BASE QWidget
#endif

class AudioEditor : public AUDIOEDITOR_BASE
{
    Q_OBJECT

public:
    explicit AudioEditor(QWidget *parent = nullptr);
    explicit AudioEditor(const QString &filePath, const QString &workingDir, QWidget *parent, bool isTempRecording = false);
    ~AudioEditor();
    QString getCurrentFilePath() const { return currentAudioFile; }
    bool isContentModified() const { return isModified; }
private slots:
    void openFile();
    void updatePositionLabel(qint64 position);
    QString timeToPosition(qint64 x, const QString &format);
    void playPause();
    void resetPosition();
    void stopPlayback();
    void updatePlayhead(qint64 position);
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void handleSelectionChanged(qint64 start, qint64 end);
    void handleZoomChanged(const QString &zoomFactor);
    void cutSelection();
    void saveModifiedAudio();
    void normalizeSelection();

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    // ========================================================================
    // MÉTHODES MULTIPLATEFORME FFMPEG
    // ========================================================================
    bool isTempRecording; // Si on arrive avec un fichier temporaire d'enregistrement
    
    bool reloadAudioSamplesSync(const QString &filePath);
    // ========================================================================
    // MÉTHODES EXISTANTES
    // ========================================================================
    
    bool writeWavFromInt16Buffer(const QString &filename);
    void init();
    Ui::AudioEditorWidget *ui;
    QMediaPlayer   *player;
    QAudioOutput   *audioOutput;
    WaveformWidget *waveformWidget;
    QSharedPointer<QVector<qint16>> audioSamplesPtr;
    qint64          totalSamples;
    QString         currentAudioFile;
    bool            modeAutonome;   
    bool isLoadingFile;
    void extractWaveformWithFFmpeg(const QString &filePath);
    
    void updatePlaybackFromModifiedData();

    std::pair<int, int> getSelectionSampleRange();
    void setButtonsEnabled(bool enabled);
    bool fichierCharge = false;
    bool isModified; 
    void releaseResources();
    QString workingDirectory;
    void startLoadingInThread(const QString &filePath);
};