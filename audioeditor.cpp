#include "audioeditor.h"
#include "ui_audioeditor.h"
#include "projectutils.h"
#include "waveformloader.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QTime>
#include <QTimer>
#include <QCoreApplication>
#include <cmath>
#include <cstring>
//#include <QInputDialog>

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QThread> 
#include <QtEndian>

// ============================================================================
// CODE EXISTANT AVEC MODIFICATIONS
// ============================================================================

AudioEditor::AudioEditor(QWidget *parent)
    : AUDIOEDITOR_BASE(parent)
    , modeAutonome(true)
{
    qRegisterMetaType<QSharedPointer<QVector<qint16>>>("QSharedPointer<QVector<qint16>>");
    init();
}

AudioEditor::AudioEditor(const QString &filePath, const QString &workingDir, QWidget *parent, bool isTempRecording)
    : AUDIOEDITOR_BASE(parent)
    , modeAutonome(false)
    , currentAudioFile(filePath)
    , isTempRecording(isTempRecording)
    , workingDirectory(workingDir)
    , isLoadingFile(false)
{
    init();
    
    if (isTempRecording) {
        isModified = true;
    }
}

AudioEditor::~AudioEditor()
{
    releaseResources();
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir dir(tempDir);
    dir.setNameFilters({"temp_playback_*", "temp_modified*", "temp.raw", "temp_save.raw"});
    for (const QString &f : dir.entryList(QDir::Files))
        dir.remove(f);
    delete ui;
}

void AudioEditor::showEvent(QShowEvent *event)
{
    AUDIOEDITOR_BASE::showEvent(event);
    if (!modeAutonome && !fichierCharge) {
        fichierCharge = true;
        QTimer::singleShot(50, this, &AudioEditor::openFile);
        // openFile();
    }
}

void AudioEditor::init()
{
    ui = new Ui::AudioEditorWidget;
    ui->setupUi(this);
    isModified = false;

    player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    totalSamples = 0;

    waveformWidget = ui->waveformWidget;
    waveformWidget->setisLoaded(false);

    setButtonsEnabled(false);
    audioOutput->setVolume(0.5);
    player->setAudioOutput(audioOutput);

    connect(ui->btnOpen,      &QPushButton::clicked, this, &AudioEditor::openFile);
    connect(ui->btnPlay,      &QPushButton::clicked, this, &AudioEditor::playPause);
    connect(ui->btnReset,     &QPushButton::clicked, this, &AudioEditor::resetPosition);
    connect(ui->btnStop,      &QPushButton::clicked, this, &AudioEditor::stopPlayback);
    connect(ui->btnCut,       &QPushButton::clicked, this, &AudioEditor::cutSelection);
    connect(ui->btnSave,      &QPushButton::clicked, this, &AudioEditor::saveModifiedAudio);
    connect(ui->btnNormalizeAll, &QPushButton::clicked, this, &AudioEditor::normalizeSelection);
    connect(ui->btnNormalize,    &QPushButton::clicked, this, &AudioEditor::normalizeSelection);
    connect(ui->btnZoomIn,       &QPushButton::clicked, waveformWidget, &WaveformWidget::zoomIn);
    connect(ui->btnZoomOut,      &QPushButton::clicked, waveformWidget, &WaveformWidget::zoomOut);

    connect(player, &QMediaPlayer::positionChanged,    this, &AudioEditor::updatePlayhead);
    connect(player, &QMediaPlayer::mediaStatusChanged, this, &AudioEditor::handleMediaStatusChanged);
    connect(player, &QMediaPlayer::durationChanged,    this, [this](qint64 d){
        ui->lengthLabel->setText(QTime::fromMSecsSinceStartOfDay(d).toString("hh:mm:ss"));
    });
    connect(player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state){
        if (state == QMediaPlayer::StoppedState) {
            ui->btnPlay->setIcon(QIcon(":/icones/play.png"));
            ui->btnStop->setEnabled(false);
        }
    });

    connect(waveformWidget, &WaveformWidget::zoomChanged,      this, &AudioEditor::handleZoomChanged);
    connect(waveformWidget, &WaveformWidget::selectionChanged, this, &AudioEditor::handleSelectionChanged);
    connect(waveformWidget, &WaveformWidget::playbackFinished, this, &AudioEditor::stopPlayback);

    if (!modeAutonome)
        ui->btnOpen->setVisible(false);
}

void AudioEditor::setButtonsEnabled(bool e)
{
    ui->btnPlay->setEnabled(e);
    ui->btnReset->setEnabled(e);
    ui->btnNormalizeAll->setEnabled(e);
    ui->btnStop->setEnabled(false);
    ui->btnCut->setEnabled(false);
    ui->btnNormalize->setEnabled(false);
    ui->btnSave->setEnabled(e);
    ui->btnZoomIn->setEnabled(e);
    ui->btnZoomOut->setEnabled(e);
    ui->selectionLabel->clear();
}

void AudioEditor::updatePositionLabel(qint64 pos)
{
    ui->positionLabel->setText(QTime(0,0,0).addMSecs(pos).toString("hh:mm:ss"));
}

QString AudioEditor::timeToPosition(qint64 sampleIdx, const QString &fmt)
{
    qint64 dur = player->duration();
    if (dur <= 0 || totalSamples <= 0) return "00:00:00";
    qint64 ms = static_cast<qint64>((double)sampleIdx / totalSamples * dur);
    return QTime(0,0,0).addMSecs(ms).toString(fmt);
}


void AudioEditor::openFile()
{
    // Mode autonome : demande fichier
    if (modeAutonome) {
        currentAudioFile = QFileDialog::getOpenFileName(this,
            tr("Ouvrir un fichier audio"), {}, ProjectUtils::getFileDialogFilter());
    }
    
    if (currentAudioFile.isEmpty()) return;

    // Nettoyage avant chargement
    totalSamples = 0;
    
    // AU LIEU DE : extractWaveformWithFFmpeg(currentAudioFile);
    // ON FAIT :
    startLoadingInThread(currentAudioFile);
}

void AudioEditor::startLoadingInThread(const QString &filePath)
{
    // 1. UI : Afficher chargement
    waveformWidget->setLoading(true);
    setButtonsEnabled(false);
    
    QProgressDialog *pd = new QProgressDialog("Analyse audio...", "Annuler", 0, 100, this);
    pd->setWindowModality(Qt::WindowModal);
    pd->setMinimumDuration(0);
    pd->setValue(0);
    pd->show();

    // 2. Threading Setup
    QThread *thread = new QThread;
    WaveformLoader *loader = new WaveformLoader(filePath); 
    loader->moveToThread(thread);

    // 3. Connexions
    connect(thread, &QThread::started, loader, &WaveformLoader::process);
    
    connect(loader, &WaveformLoader::progress, pd, &QProgressDialog::setValue);
    
    connect(loader, &WaveformLoader::error, this, [this, pd, thread, loader](QString msg){
        pd->close();
        QMessageBox::warning(this, "Erreur", msg);
        waveformWidget->setLoading(false);
        thread->quit();
        thread->wait();
        loader->deleteLater();
        thread->deleteLater();
        pd->deleteLater();
    });

connect(loader, &WaveformLoader::finished, this, 
    [this, pd, thread, loader](QSharedPointer<QVector<qint16>> samples, qint64 count){
    // [this, pd, thread, loader](QVector<qint16> samples, qint64 count){
        
        // ✅ Création directe du pointeur partagé
        this->audioSamplesPtr = samples;
        // audioSamplesPtr = QSharedPointer<QVector<qint16>>::create(std::move(samples));
        this->totalSamples = count;

        // ✅ Passage correct
        waveformWidget->setFullWaveform(audioSamplesPtr);
        
        waveformWidget->setLoading(false);
        setButtonsEnabled(true);
        waveformWidget->setisLoaded(true);

        pd->close();
        thread->quit();
        thread->wait();
        loader->deleteLater();
        thread->deleteLater();
        pd->deleteLater();
        
        player->setSource(QUrl::fromLocalFile(currentAudioFile));
    });

    // 4. Lancer
    thread->start();
}

// std::pair<int,int> AudioEditor::getSelectionSampleRange()
// {
//     int s = waveformWidget->getSelectionStart();
//     int e = waveformWidget->getSelectionEnd();
//     return {
//         qBound(0, s , (int)totalSamples), 
//         qBound(0, e , (int)totalSamples) 
//         //  qBound(0, s, totalSamples), qBound(0, e, totalSamples) 
//     };
// }

std::pair<int, int> AudioEditor::getSelectionSampleRange()
{
    // On récupère les positions directement du widget
    // (Comme on lui a envoyé les données brutes, ses indices = indices réels)
    int s = waveformWidget->getSelectionStart();
    int e = waveformWidget->getSelectionEnd();
    
    // Sécurité : on s'assure de ne pas dépasser la taille du fichier
    // (totalSamples est maintenant correct grâce à la correction 1)
    int maxSample = static_cast<int>(totalSamples);

    return {
        std::clamp(s, 0, maxSample),
        std::clamp(e, 0, maxSample)
    };
}


void AudioEditor::cutSelection()
{
    if (!waveformWidget->hasSelection()) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    waveformWidget->setUpdatesEnabled(false);

    double oldZoom = waveformWidget->getSamplesPerPixel();
    int oldScroll = waveformWidget->getScrollOffset();
    
    auto range = getSelectionSampleRange();
    int s = range.first;
    int e = range.second;
    
    if (s >= e || !audioSamplesPtr || audioSamplesPtr->isEmpty()) {
        waveformWidget->setUpdatesEnabled(true);
        QApplication::restoreOverrideCursor();
        return;
    }

    player->stop();

    player->setSource(QUrl());
    QCoreApplication::processEvents();
    
    // ========================================================================
    // ALGORITHME OPTIMISÉ : DÉPLACEMENT IN-PLACE (400 Mo au lieu de 1,6 Go)
    // ========================================================================
    
    // 1️⃣ On garde le pointeur original (pas de copie complète)
    QVector<qint16>* workData = audioSamplesPtr.data();
    
    // 2️⃣ On calcule les tailles
    qint64 tailStart = e;                    // Début de la partie à garder
    qint64 tailSize = workData->size() - e;  // Taille de la queue
    qint64 finalSize = workData->size() - (e - s);
    
    // 3️⃣ Déplacement mémoire IN-PLACE (ultra rapide, pas de duplication)
    if (tailSize > 0) {
        // On déplace la queue vers l'avant pour écraser la sélection
        // Source : données après la sélection (position e)
        // Destination : position s (début de la sélection)
        std::memmove(workData->data() + s,      // Destination
                     workData->constData() + e, // Source
                     tailSize * sizeof(qint16));
    }
    
    // 4️⃣ On réduit la taille du vecteur (libère la fin)
    workData->resize(finalSize);
    workData->squeeze(); // Compactage pour libérer la mémoire inutilisée
    
    // 5️⃣ Mise à jour métadonnées
    totalSamples = workData->size();
    isModified = true;

    // ✅ Pas de réaffectation du pointeur, on a travaillé directement dessus
    // Le widget garde la même référence, donc pas de flash visuel
    
    // 6️⃣ Notification au widget que les données ont changé
    waveformWidget->setFullWaveform(audioSamplesPtr); // Rafraîchit l'affichage
    waveformWidget->restoreZoomState(oldZoom, oldScroll);
    
    int newPos = std::clamp(s, 0, (int)totalSamples);
    waveformWidget->resetSelection(newPos);
    waveformWidget->setPlayheadPosition(newPos);

    setButtonsEnabled(true);
    ui->btnCut->setEnabled(false);
    ui->btnNormalize->setEnabled(false);
    
    updatePlaybackFromModifiedData();
    
    waveformWidget->setUpdatesEnabled(true);
    QApplication::restoreOverrideCursor();
}

void AudioEditor::normalizeSelection()
{
    if (!audioSamplesPtr || audioSamplesPtr->isEmpty()) return;

    int startIndex, endIndex;
    if (!waveformWidget->hasSelection()) {
        startIndex = 0; 
        endIndex = audioSamplesPtr->size();
    } else {
        std::tie(startIndex, endIndex) = getSelectionSampleRange();
    }
    
    if (startIndex >= endIndex) return;

    double oldZoom = waveformWidget->getSamplesPerPixel();
    int oldScroll = waveformWidget->getScrollOffset();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    player->stop();
    player->setSource(QUrl());
    QCoreApplication::processEvents();
    
    // ✅ CORRECTION : On ne vide PAS l'affichage, juste les données internes
    // Ancien code qui causait le flash :
    // waveformWidget->clearData();
    
    // Nouveau : On détache juste le pointeur sans toucher à l'affichage
    QVector<qint16> localData = *audioSamplesPtr;
    audioSamplesPtr.reset();
    QCoreApplication::processEvents();
    
    // Calcul Max
    qint16 maxVal = 0;
    const qint16* raw = localData.constData();
    for (int i = startIndex; i < endIndex; ++i) {
        qint16 absVal = std::abs(raw[i]);
        if (absVal > maxVal) maxVal = absVal;
    }
    
    // Application Gain
    if (maxVal > 0) {
        double scale = 32700.0 / maxVal;
        qint16* rawWrite = localData.data();
        
        for (int i = startIndex; i < endIndex; ++i) {
            rawWrite[i] = static_cast<qint16>(rawWrite[i] * scale);
        }
        isModified = true;
    }

    // Recréation du pointeur
    audioSamplesPtr = QSharedPointer<QVector<qint16>>::create(std::move(localData));

    // Réinjection (pas de flash car l'ancien affichage reste jusqu'au dernier moment)
    waveformWidget->setFullWaveform(audioSamplesPtr);
    waveformWidget->restoreZoomState(oldZoom, oldScroll);
    
    if (waveformWidget->hasSelection()) {
        waveformWidget->setStartAndEnd(startIndex, endIndex);
    }
    waveformWidget->setPlayheadPosition(startIndex);
    
    updatePlaybackFromModifiedData();
    QApplication::restoreOverrideCursor();
}


void AudioEditor::updatePlaybackFromModifiedData()
{
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    
 
    QString tempPlaybackFile = tempDir + "/temp_playback_current.wav";
    
    if (QFile::exists(tempPlaybackFile)) {
        player->stop();
        player->setSource(QUrl()); // Libère le verrou
        QFile::remove(tempPlaybackFile);
    }

    if (!writeWavFromInt16Buffer(tempPlaybackFile)) {
        QMessageBox::warning(this, tr("Erreur"), tr("Impossible de générer le fichier WAV temporaire."));
        return;
    }

    player->stop();
    ui->btnPlay->setIcon(QIcon(":/icones/play.png"));
    ui->btnStop->setEnabled(false);
    player->setSource(QUrl::fromLocalFile(tempPlaybackFile));
}

void AudioEditor::saveModifiedAudio()
{
    if (player) {
        player->stop();
        player->setSource(QUrl()); 
    }

    if (!audioSamplesPtr || audioSamplesPtr->isEmpty()) {
        QMessageBox::warning(this, tr("Erreur"), tr("Aucun signal audio à sauvegarder."));
        
        // Si on annule, on recharge le fichier original pour réactiver le player
        if (player) player->setSource(QUrl::fromLocalFile(currentAudioFile));
        return;
    }

    QString targetFile;


    if (isTempRecording) {
        QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
        QString defaultName = QString("Enregistrement_%1").arg(timeStamp);
        QDialog dialog(this);
        dialog.setWindowTitle(tr("Sauvegarder l'enregistrement"));
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        QLabel *label = new QLabel(tr("Nom du fichier :"), &dialog);
        layout->addWidget(label);
        QLineEdit *lineEdit = new QLineEdit(&dialog);
        lineEdit->setText(defaultName);
        layout->addWidget(lineEdit);
        // QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        QDialogButtonBox *buttons = new QDialogButtonBox(&dialog);
        buttons->addButton("Sauvegarder", QDialogButtonBox::AcceptRole);
        buttons->addButton("Annuler", QDialogButtonBox::RejectRole);

        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        dialog.resize(280, dialog.height());
        bool ok = (dialog.exec() == QDialog::Accepted);
        QString fileName = lineEdit->text();
        if (!ok || fileName.isEmpty()) {
            // Si annulation, on recharge le son pour ne pas planter
             player->setSource(QUrl::fromLocalFile(currentAudioFile));
             return;
        }
        if (!fileName.endsWith(".wav", Qt::CaseInsensitive)) fileName += ".wav";
        targetFile = QDir(workingDirectory).filePath(fileName);
        if (QFile::exists(targetFile)) {
             if (QMessageBox::question(this, tr("Existe"), tr("Écraser ?"), QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) {
                 player->setSource(QUrl::fromLocalFile(currentAudioFile));
                 return;
             }
        }
    } else {
        targetFile = currentAudioFile;
        if (targetFile.isEmpty()) {
            QMessageBox::warning(this, tr("Erreur"), tr("Aucun fichier cible défini."));
            return;
        }
        
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Confirmer la sauvegarde"));
        msgBox.setText(tr("Voulez-vous vraiment enregistrer les modifications sur ce fichier audio ? "
                        "Assurez-vous d'avoir une sauvegarde de l'original"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        
        msgBox.button(QMessageBox::Yes)->setText(tr("Oui"));
        msgBox.button(QMessageBox::No)->setText(tr("Non"));
        
        int reply = msgBox.exec();
        if (reply != QMessageBox::Yes) return;
    }
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString tempWav = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/temp_save.wav";

    // Écriture du fichier temporaire
    if (!writeWavFromInt16Buffer(tempWav)) {
        QMessageBox::warning(this, tr("Erreur"), tr("Impossible d'écrire le WAV temporaire."));
        QApplication::restoreOverrideCursor();
        player->setSource(QUrl::fromLocalFile(currentAudioFile));
        return;
    }

    bool success = false;

    // ========================================================================
    // CORRECTION 3 : BOUCLE DE RÉESSAI POUR LE FICHIER VERROUILLÉ (WAV)
    // ========================================================================
    if (targetFile.endsWith(".wav", Qt::CaseInsensitive)) {
        
        // Si le fichier existe, on essaie de le supprimer avec insistance
        if (QFile::exists(targetFile)) {
            bool removed = false;
            // On essaie 10 fois avec 100ms de pause (1 seconde max)
            for (int i = 0; i < 10; ++i) {
                if (QFile::remove(targetFile)) {
                    removed = true;
                    break;
                }
                QThread::msleep(100); // Pause pour laisser Windows libérer le fichier
            }

            if (!removed) {
                QMessageBox::warning(this, tr("Erreur"), 
                    tr("Impossible d'écraser le fichier.\nIl est peut-être utilisé par une autre application ou l'antivirus."));
                QFile::remove(tempWav);
                QApplication::restoreOverrideCursor();
                player->setSource(QUrl::fromLocalFile(currentAudioFile)); // On rétablit
                return;
            }
        }
        
        // Copie du nouveau fichier
        if (QFile::copy(tempWav, targetFile)) {
            success = true;
        } else {
            QMessageBox::warning(this, tr("Erreur"), tr("Impossible de copier le fichier final."));
        }
        QFile::remove(tempWav);
    } 
    else {
        // ====================================================================
        // GESTION MP3 / AUTRES (Conversion FFmpeg)
        // ====================================================================
        // Pour le MP3, comme on a déjà appliqué l'Auto-Limiter au début,
        // le son envoyé à FFmpeg est propre et ne saturera pas.
        
        QString ffmpegPath = AudioMerger::getFFmpegPath();
        QStringList args;
        args << "-y" << "-i" << tempWav;
        // Paramètres spécifiques selon le format de sortie
        if (targetFile.endsWith(".mp3", Qt::CaseInsensitive)) {
            args << "-codec:a" << "libmp3lame" << "-q:a" << "2"; 
        } 
        else if (targetFile.endsWith(".m4a", Qt::CaseInsensitive) || targetFile.endsWith(".aac", Qt::CaseInsensitive)) {
            args << "-codec:a" << "aac" << "-b:a" << "192k";
        }
        else if (targetFile.endsWith(".ogg", Qt::CaseInsensitive)) {
            args << "-codec:a" << "libvorbis" << "-q:a" << "5";
        }
        else if (targetFile.endsWith(".flac", Qt::CaseInsensitive)) {
            args << "-codec:a" << "flac";
        }
        else if (targetFile.endsWith(".opus", Qt::CaseInsensitive)) {
            args << "-codec:a" << "libopus" << "-b:a" << "128k";
        }
        else if (targetFile.endsWith(".ac3", Qt::CaseInsensitive)) {
            args << "-codec:a" << "ac3" << "-b:a" << "192k";
        }
        else if (targetFile.endsWith(".wma", Qt::CaseInsensitive)) {
            args << "-codec:a" << "wmav2" << "-b:a" << "192k";
        }
        // Pour aiff et autres, FFmpeg détecte souvent automatiquement, 
        // ou on laisse par défaut.
        
        args << targetFile;
        QProcess ff;
        ff.start(ffmpegPath, args);
        
        if (ff.waitForFinished(30000) && ff.exitStatus() == QProcess::NormalExit) {
            success = true;
        } else {
            QString err = ff.readAllStandardError();
            QMessageBox::warning(this, tr("Erreur FFmpeg"), tr("Échec encodage: %1").arg(err));
        }
        QFile::remove(tempWav);
    }

    QApplication::restoreOverrideCursor();

    if (success) {
        isModified = false;
        if (isTempRecording) isTempRecording = false;
        currentAudioFile = targetFile;
        setWindowTitle(tr("Éditeur Audio - ") + QFileInfo(currentAudioFile).fileName());
        QMessageBox::information(this, tr("Succès"), tr("Fichier sauvegardé avec succès."));
    }
    
    // IMPORTANT : On recharge le fichier frais
    player->setSource(QUrl::fromLocalFile(currentAudioFile));
}

void AudioEditor::updatePlayhead(qint64 ms)
{
    qint64 dur = player->duration();
    if (dur <= 0) return;
    qint64 realIdx  = static_cast<qint64>((double)ms / dur * totalSamples);
    if (waveformWidget->hasSelection() && realIdx >= waveformWidget->getSelectionEnd()) {
    // if (waveformWidget->hasSelection() && idx >= waveformWidget->getSelectionEnd()) {
        stopPlayback();
        return;
    }

    waveformWidget->setPlayheadPosition(realIdx);
    updatePositionLabel(ms);

    int visibleStart = waveformWidget->getScrollOffset();
    int visibleEnd   = visibleStart + waveformWidget->width();
    int playheadPixel = waveformWidget->sampleToPixel(realIdx);
    if (playheadPixel < visibleStart || playheadPixel > visibleEnd - 30) {
        waveformWidget->scrollToPixel(playheadPixel - waveformWidget->width() / 3);
    }
}


bool AudioEditor::writeWavFromInt16Buffer(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) return false;

    // ✅ Accès via le pointeur
    if (!audioSamplesPtr || audioSamplesPtr->isEmpty()) return false;

    const int sampleRate = 44100; 
    const int channels = 1;
    const int bitsPerSample = 16;
    const int byteRate = sampleRate * channels * bitsPerSample / 8;
    const int blockAlign = channels * bitsPerSample / 8;
    
    qint64 dataSize = static_cast<qint64>(audioSamplesPtr->size()) * sizeof(qint16);
    qint64 fileSize = 36 + dataSize;

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);

    out.writeRawData("RIFF", 4);
    out << quint32(fileSize);
    out.writeRawData("WAVE", 4);
    out.writeRawData("fmt ", 4);
    out << quint32(16);
    out << quint16(1);
    out << quint16(channels);
    out << quint32(sampleRate);
    out << quint32(byteRate);
    out << quint16(blockAlign);
    out << quint16(bitsPerSample);
    out.writeRawData("data", 4);
    out << quint32(dataSize);

    const int CHUNK_SIZE = 4096;
    QByteArray buffer;
    buffer.resize(CHUNK_SIZE * sizeof(qint16));
    qint16* rawData = reinterpret_cast<qint16*>(buffer.data());
    
    int currentChunkIndex = 0;
    
    for (int i = 0; i < audioSamplesPtr->size(); ++i) {
        rawData[currentChunkIndex] = qToLittleEndian((*audioSamplesPtr)[i]);
        currentChunkIndex++;
        
        if (currentChunkIndex >= CHUNK_SIZE) {
            file.write(buffer);
            currentChunkIndex = 0;
        }
    }
    
    if (currentChunkIndex > 0) {
        file.write(buffer.constData(), currentChunkIndex * sizeof(qint16));
    }
    
    file.close();
    return true;
}

void AudioEditor::playPause()
{
    if (player->playbackState() == QMediaPlayer::PlayingState) {
        ui->btnPlay->setIcon(QIcon(":/icones/play.png"));
        player->pause();
    } else {
        ui->btnPlay->setIcon(QIcon(":/icones/pause.png"));

        if (player->playbackState() == QMediaPlayer::StoppedState) {
            // 1. On récupère la position VISUELLE
            qint64 sVisual = waveformWidget->getSelectionStart();
            if (sVisual < 0) {
                sVisual = waveformWidget->getPlayheadPosition();
            }
            
            if (sVisual >= 0 && totalSamples > 0 && player->duration() > 0) {
                
                qint64 posMs = static_cast<qint64>((double)sVisual / totalSamples * player->duration());
                player->setPosition(posMs);
            }
        }
        
        player->play();
    }

    ui->btnStop->setEnabled(true);
    ui->btnNormalizeAll->setEnabled(false);
}

void AudioEditor::stopPlayback()
{
    player->blockSignals(true);
    player->stop();
    ui->btnPlay->setIcon(QIcon(":/icones/play.png"));
    ui->btnStop->setEnabled(false);
    player->blockSignals(false);

    ui->btnStop->setEnabled(false);
    if (!waveformWidget->hasSelection()) ui->btnNormalizeAll->setEnabled(true);


    const qint64 startposition = waveformWidget->getSelectionStart();
    qint64 targetPos = (startposition > 0) ? startposition : 0;
    
    waveformWidget->setPlayheadPosition(targetPos);
    ui->positionLabel->setText(timeToPosition(targetPos, "hh:mm:ss"));
}

void AudioEditor::resetPosition()
{
    waveformWidget->resetSelection(-1);
    waveformWidget->setPlayheadPosition(0);
    ui->selectionLabel->clear();
    updatePositionLabel(0);
    stopPlayback();
}

void AudioEditor::handleZoomChanged(const QString &zf)
{
    ui->infoZoom->setText(zf);
}

void AudioEditor::handleSelectionChanged(qint64 s, qint64 e)
{
    qint64 realS = s ;
    qint64 realE = e ;

    if (s >= 0 && e > s) {
        ui->selectionLabel->setText(tr("Sélection: %1 - %2 (Durée: %3)")
            .arg(timeToPosition(realS, "hh:mm:ss.zzz"))
            .arg(timeToPosition(realE, "hh:mm:ss.zzz"))
            .arg(timeToPosition(realE - realS, "hh:mm:ss.zzz")));
        ui->btnCut->setEnabled(true);
        ui->btnNormalize->setEnabled(true);
        ui->btnNormalizeAll->setEnabled(false);
    } else {
        ui->selectionLabel->clear();
        ui->btnCut->setEnabled(false);
        ui->btnNormalize->setEnabled(false);
        ui->btnNormalizeAll->setEnabled(true);
    }
    ui->positionLabel->setText(timeToPosition(realS,"hh:mm:ss"));
}

void AudioEditor::handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        qint64 pos = waveformWidget->hasSelection() ? waveformWidget->getSelectionStart() : 0;
        waveformWidget->setPlayheadPosition(pos);
        ui->btnPlay->setIcon(QIcon(":/icones/play.png"));
        ui->btnStop->setEnabled(false);
    }
}

void AudioEditor::releaseResources()
{
    if (player) {
        player->stop();
        player->setSource(QUrl()); 
    }
}

void AudioEditor::closeEvent(QCloseEvent *event)
{
    if (isLoadingFile) {
        event->ignore(); // Interdit de fermer pendant le chargement
        return;
    }

    if (isModified) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Modifications non enregistrées"));
        msgBox.setText(tr("Vous avez des modifications non sauvegardées.\nVoulez-vous les enregistrer avant de quitter ?"));
        msgBox.setIcon(QMessageBox::Question);
        
        QPushButton *btnOui = msgBox.addButton(tr("Oui"), QMessageBox::YesRole);
        QPushButton *btnNon = msgBox.addButton(tr("Non"), QMessageBox::NoRole); 
        QPushButton *btnAnnuler = msgBox.addButton(tr("Annuler"), QMessageBox::RejectRole);
        
        msgBox.setDefaultButton(btnOui);
        msgBox.exec();

        if (msgBox.clickedButton() == btnOui) {
            saveModifiedAudio();
            if (isModified) {
                event->ignore();
                return;
            }
        } else if (msgBox.clickedButton() == btnAnnuler) {
            event->ignore();
            return;
        }
    }

    releaseResources();
    AUDIOEDITOR_BASE::closeEvent(event);
}
