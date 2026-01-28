#include "audiomerger.h"
#include <QMessageBox>
#include <QRegularExpression>
#include <QTime>
#include <QLocale>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QTimer>
#include <QEventLoop>
#include <QDataStream>
#include <QDebug>

// A Supp pour debug
#include <QClipboard>    
#include <QApplication>  
#include <QDir>
#include <QInputDialog>   
#include <QLineEdit>

// ============================================================================
// INITIALISATION DES VARIABLES STATIQUES
// ============================================================================
QString AudioMerger::m_cachedFFmpegPath = "";
bool AudioMerger::m_isFFmpegAvailable = false;
bool AudioMerger::m_isInitialized = false;
QMap<QString, int> AudioMerger::m_channelCache;

// ============================================================================
// CONFIGURATION MULTIPLATEFORME FFMPEG (OPTIMISÉE)
// ============================================================================

void AudioMerger::init()
{
    if (m_isInitialized) return; // Déjà fait
    
    // 1. Détermination du chemin (Même logique qu'avant)
    QString ffmpegPath;
    

#ifdef Q_OS_WIN
    ffmpegPath = QCoreApplication::applicationDirPath() + "/ffmpeg.exe";
    if (!QFile::exists(ffmpegPath)) {
        ffmpegPath = "ffmpeg.exe";
    }

#elif defined(Q_OS_MAC)
    // 1. PRIORITÉ : Ta méthode (Dans le bundle de l'app)
    // Chemin : MonApp.app/Contents/MacOS/ffmpeg
    QString bundledPath = QCoreApplication::applicationDirPath() + "/ffmpeg";

    if (QFile::exists(bundledPath)) {
        ffmpegPath = bundledPath;
    } 
    else {
        // 2. FALLBACK : Utile pendant le développement si tu n'as pas encore copié le fichier
        // ou si l'utilisateur a ffmpeg installé via Homebrew.
        if (QFile::exists("/opt/homebrew/bin/ffmpeg")) {
            ffmpegPath = "/opt/homebrew/bin/ffmpeg"; // Mac M1/M2/M3
        } else if (QFile::exists("/usr/local/bin/ffmpeg")) {
            ffmpegPath = "/usr/local/bin/ffmpeg";    // Mac Intel
        } else {
            ffmpegPath = "ffmpeg"; // Espoir qu'il soit dans le PATH
        }
    }

#else
    // Sur Linux, on utilise généralement le ffmpeg du système
    if (QFile::exists("/usr/bin/ffmpeg")) {
        ffmpegPath = "/usr/bin/ffmpeg";
    } else {
        ffmpegPath = "ffmpeg"; 
    }
#endif

    m_cachedFFmpegPath = ffmpegPath;

    // 2. Vérification technique (Lancement du processus)
    QProcess process;
    process.start(m_cachedFFmpegPath, QStringList() << "-version");
    
    // On attend un peu, mais au lancement de l'app c'est moins grave d'attendre
    process.waitForFinished(3000);
    
    if (process.exitCode() == 0 && process.exitStatus() == QProcess::NormalExit) {
        m_isFFmpegAvailable = true;
    } else {
        m_isFFmpegAvailable = false;
        // On ne met pas de MessageBox ici car init() est appelé au boot (pas de fenêtre parent)
        qWarning() << "FFmpeg introuvable lors de l'initialisation.";
    }

    m_isInitialized = true;
}

QString AudioMerger::getFFmpegPath()
{
    // Sécurité : si init() n'a pas été appelé dans main.cpp, on le fait ici (Lazy loading)
    if (!m_isInitialized) init();
    return m_cachedFFmpegPath;
}

bool AudioMerger::checkFFmpeg(QWidget *parent)
{
    // Sécurité
    if (!m_isInitialized) init();

    // Si FFmpeg n'est pas dispo, on affiche l'erreur à l'utilisateur maintenant
    // car on a un "parent" (la fenêtre qui a demandé l'action)
    if (!m_isFFmpegAvailable && parent) {
         QMessageBox::warning(parent, tr("FFmpeg non trouvé"),
            tr("FFmpeg n'est pas installé ou introuvable.\n\n"
#ifdef Q_OS_WIN
               "Veuillez placer ffmpeg.exe dans le dossier de l'application,\n"
               "ou l'installer sur votre système."
#elif defined(Q_OS_MAC)
               "Veuillez installer FFmpeg via Homebrew :\n"
               "brew install ffmpeg"
#elif defined(Q_OS_LINUX)
               "Veuillez installer FFmpeg via votre gestionnaire de paquets."
#endif
            ));
    }
    
    return m_isFFmpegAvailable;
}


// ============================================================================
// CODE EXISTANT AVEC MODIFICATIONS
// ============================================================================

AudioMerger::AudioMerger(QObject* parent) : QObject(parent), totalDurationInSeconds(0) {
    // ffmpegProcess = new QProcess(this);
    ffmpegProcess = nullptr; 
    
    // ffmpegProcess->setProcessChannelMode(QProcess::MergedChannels);

    // connect(ffmpegProcess, &QProcess::finished, this, &AudioMerger::processFinished);
    // connect(ffmpegProcess, &QProcess::errorOccurred, this, &AudioMerger::processError);

    // connect(ffmpegProcess, &QProcess::readyReadStandardOutput, [this]() {
    //     QByteArray data = ffmpegProcess->readAllStandardOutput();
    //     QString output = QString::fromLatin1(data);

    //     static QRegularExpression timeRegex("time=(\\d{2}:\\d{2}:\\d{2})\\.\\d+");
    //     QRegularExpressionMatch match = timeRegex.match(output);

    //     if (match.hasMatch()) {
    //         QString currentTimeStr = match.captured(1); 

    //         if (totalDurationInSeconds > 0.1) {
    //             QString totalTimeStr = formatTime(totalDurationInSeconds);
    //             emit statusMessage(QString("Traitement : %1 / %2").arg(currentTimeStr).arg(totalTimeStr));
    //         } else {
    //             emit statusMessage(QString("Traitement : %1").arg(currentTimeStr));
    //         }
    //     }
    // });
}

QString AudioMerger::formatTime(double seconds) {
    int totalSecs = static_cast<int>(seconds);
    int h = totalSecs / 3600;
    int m = (totalSecs % 3600) / 60;
    int s = totalSecs % 60;
    
    return QString("%1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
}

double AudioMerger::getFileDuration(const QString &file) {
    QFileInfo fi(file);
    if (!fi.exists()) return 0.0;

    // OPTIMISATION : Si c'est un WAV, on lit l'en-tête directement (très rapide)
    if (fi.suffix().compare("wav", Qt::CaseInsensitive) == 0) {
        return getWavDuration(file);
    }

    // POUR LES AUTRES FORMATS (MP3, M4A, ETC.) : On utilise QMediaPlayer
    // Note : QMediaPlayer est asynchrone, nous devons utiliser une boucle d'événement locale
    // pour rendre la fonction synchrone (bloquante) comme l'était votre appel ffprobe.

    QMediaPlayer player;
    QAudioOutput audioOutput;
    player.setAudioOutput(&audioOutput);
    
    QEventLoop loop;
    
    // On quitte la boucle quand les métadonnées sont chargées
    connect(&player, &QMediaPlayer::mediaStatusChanged, [&](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::BufferedMedia || status == QMediaPlayer::LoadedMedia) {
            loop.quit();
        }
    });

    // On quitte aussi en cas d'erreur
    connect(&player, &QMediaPlayer::errorOccurred, [&](QMediaPlayer::Error error, const QString &errorString){
        Q_UNUSED(error);
        Q_UNUSED(errorString);
        loop.quit();
    });

    // Sécurité : Timeout de 2 secondes max (au cas où le fichier est corrompu)
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);

    player.setSource(QUrl::fromLocalFile(file));
    
    // Si le fichier n'est pas instantanément chargé, on attend
    if (player.mediaStatus() != QMediaPlayer::BufferedMedia && 
        player.mediaStatus() != QMediaPlayer::LoadedMedia) {
        loop.exec();
    }

    qint64 durationMs = player.duration();
    
    if (durationMs > 0) {
        return durationMs / 1000.0;
    }

    return 0.0;
}

// ----------------------------------------------------------------------------
// Lecture rapide header WAV
// ----------------------------------------------------------------------------
double AudioMerger::getWavDuration(const QString &file) {
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) return 0.0;

    QDataStream in(&f);
    in.setByteOrder(QDataStream::LittleEndian);

    char riff[4];
    in.readRawData(riff, 4); // "RIFF"
    if (strncmp(riff, "RIFF", 4) != 0) return 0.0;

    in.skipRawData(4); // File size

    char wave[4];
    in.readRawData(wave, 4); // "WAVE"
    if (strncmp(wave, "WAVE", 4) != 0) return 0.0;

    // On parcourt les chunks jusqu'à trouver "fmt " et "data"
    quint32 sampleRate = 0;
    quint16 bitsPerSample = 0;
    quint16 channels = 0;
    quint32 dataSize = 0;
    bool fmtFound = false;
    bool dataFound = false;

    while (!in.atEnd() && (!fmtFound || !dataFound)) {
        char chunkId[4];
        if (in.readRawData(chunkId, 4) != 4) break;
        
        quint32 chunkSize;
        in >> chunkSize;

        if (strncmp(chunkId, "fmt ", 4) == 0) {
            quint16 audioFormat;
            in >> audioFormat;
            in >> channels;
            in >> sampleRate;
            in.skipRawData(6); // ByteRate (4) + BlockAlign (2)
            in >> bitsPerSample;
            
            // Si le chunk fmt est plus grand que 16 octets (ex: extension header), on saute le reste
            if (chunkSize > 16) in.skipRawData(chunkSize - 16);
            
            fmtFound = true;
        } 
        else if (strncmp(chunkId, "data", 4) == 0) {
            dataSize = chunkSize;
            dataFound = true;
            // Pas besoin de lire les données, on a la taille
            break; 
        } 
        else {
            // Chunk inconnu (ex: LIST, ID3, JUNK), on saute
            in.skipRawData(chunkSize);
        }
    }

    f.close();

    if (fmtFound && dataFound && sampleRate > 0 && channels > 0 && bitsPerSample > 0) {
        // Formule : TailleData / (TauxEchantillonnage * Canaux * OctetsParEchantillon)
        double bytesPerSample = bitsPerSample / 8.0;
        return (double)dataSize / (sampleRate * channels * bytesPerSample);
    }
    
    // Si échec lecture header, fallback sur méthode lente (rare)
    return 0.0;
}

int AudioMerger::getChannelCount(const QString &filePath)
{
    // 1. Vérification du cache (INSTANTANÉ)
    if (m_channelCache.contains(filePath)) {
        return m_channelCache[filePath];
    }

    // 2. Si pas en cache, on lance le processus (LENT, mais une seule fois)
    QProcess process;
    QString ffmpegPath = getFFmpegPath();
    
    QStringList args;
    args << "-i" << filePath; 
    args << "-hide_banner"; // Petite optimisation : moins de texte à parser

    process.start(ffmpegPath, args);
    process.waitForFinished(3000); // Bloque ici, mais c'est la seule fois

    QString output = process.readAllStandardError(); 
    
    int channels = 2; // Valeur par défaut (Stéréo)
    
    // Logique de détection existante
    if (output.contains(" stereo,", Qt::CaseInsensitive)) channels = 2;
    else if (output.contains(" mono,", Qt::CaseInsensitive)) channels = 1;
    else if (output.contains("1 channels", Qt::CaseInsensitive)) channels = 1;
    else if (output.contains("2 channels", Qt::CaseInsensitive)) channels = 2;

    // 3. Stockage dans le cache
    m_channelCache.insert(filePath, channels);

    return channels;
}

// À appeler quand AudioEditor sauvegarde un fichier (car il peut passer de Stéréo à Mono)
void AudioMerger::invalidateCache(const QString &filePath) {
    m_channelCache.remove(filePath);
}

// int AudioMerger::getChannelCount(const QString &filePath)
// {
//     QProcess process;
//     QString ffmpegPath = getFFmpegPath();
    
//     // On lance juste "ffmpeg -i fichier"
//     // FFmpeg va afficher les infos et s'arrêter car il manque le fichier de sortie
//     QStringList args;
//     args << "-i" << filePath; 

//     process.start(ffmpegPath, args);
//     process.waitForFinished(3000); 

//     // FFmpeg écrit les infos techniques dans le canal d'Erreur (StandardError), pas Output !
//     QString output = process.readAllStandardError(); 

//     // On analyse le texte pour trouver "stereo" ou "mono"
//     // Ex de sortie : "Stream #0:0: Audio: mp3, 44100 Hz, stereo, fltp, 128 kb/s"
    
//     if (output.contains(" stereo,", Qt::CaseInsensitive)) {
//         return 2;
//     }
    
//     if (output.contains(" mono,", Qt::CaseInsensitive)) {
//         return 1;
//     }

//     // Sécurité pour les formats bizarres ("1 channels")
//     if (output.contains("1 channels", Qt::CaseInsensitive)) return 1;
//     if (output.contains("2 channels", Qt::CaseInsensitive)) return 2;

//     // Par défaut, si on ne sait pas, on dit 2 (Stéréo) 
//     // pour déclencher la formule de mixage sécurisée (0.5+0.5) et éviter la saturation.
//     return 2; 
// }


void AudioMerger::mergeFiles(const QStringList& inputFiles, const QString& outputFile) {
    if (inputFiles.isEmpty()) {
        emit error("Aucun fichier en entrée.");
        return;
    }

    // Calcul de la durée totale
    totalDurationInSeconds = 0;
    for (const QString& file : inputFiles) {
        totalDurationInSeconds += getFileDuration(file);
    }

    // MODIFICATION : Utiliser getFFmpegPath()
    QString ffmpegPath = getFFmpegPath();
    
    QStringList arguments;

    if (inputFiles.size() == 1) {
        arguments << "-i" << inputFiles.first();
        arguments << "-y" << outputFile;
    } else {
        for (const QString& file : inputFiles) arguments << "-i" << file;
        QString filterComplex = "";
        for (int i = 0; i < inputFiles.size(); i++) filterComplex += "[" + QString::number(i) + ":a]";
        filterComplex += "concat=n=" + QString::number(inputFiles.size()) + ":v=0:a=1[out]";
        arguments << "-filter_complex" << filterComplex;
        arguments << "-map" << "[out]";
        arguments << "-y" << outputFile;
    }

    if (!ffmpegProcess) {
        ffmpegProcess = new QProcess(this);
        ffmpegProcess->setProcessChannelMode(QProcess::MergedChannels);

        connect(ffmpegProcess, &QProcess::finished, this, &AudioMerger::processFinished);
        connect(ffmpegProcess, &QProcess::errorOccurred, this, &AudioMerger::processError);

        connect(ffmpegProcess, &QProcess::readyReadStandardOutput, [this]() {
            QByteArray data = ffmpegProcess->readAllStandardOutput();
            QString output = QString::fromLatin1(data);

            static QRegularExpression timeRegex("time=(\\d{2}:\\d{2}:\\d{2})\\.\\d+");
            QRegularExpressionMatch match = timeRegex.match(output);

            if (match.hasMatch()) {
                QString currentTimeStr = match.captured(1); 

                if (totalDurationInSeconds > 0.1) {
                    QString totalTimeStr = formatTime(totalDurationInSeconds);
                    emit statusMessage(QString("Traitement : %1 / %2").arg(currentTimeStr).arg(totalTimeStr));
                } else {
                    emit statusMessage(QString("Traitement : %1").arg(currentTimeStr));
                }
            }
        });
    }

    ffmpegProcess->start(ffmpegPath, arguments);
    ffmpegProcess->closeWriteChannel();

    emit started();
}

void AudioMerger::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    emit statusMessage("Finalisation..."); 
    emit finished(exitCode == 0 && exitStatus == QProcess::NormalExit);
}

void AudioMerger::processError(QProcess::ProcessError error) {
    emit this->error("Erreur FFmpeg code: " + QString::number(error));
}


void AudioMerger::mixWithBackground(const QString &voiceFile, const QString &musicFile, 
                                     float volumeMusic, float offsetSeconds, const QString &outputFile) 
{
    QString ffmpegPath = getFFmpegPath();
    
    QStringList args;
    args << "-y";  // Écraser si existe
    args << "-i" << voiceFile;
    args << "-stream_loop" << "-1" << "-i" << musicFile;
    
    // Force le point décimal pour les valeurs numériques
    QString volStr = QLocale::c().toString(volumeMusic, 'f', 2);
    
    // ════════════════════════════════════════════════════════════════════════
    // CONSTRUCTION DU FILTRE SELON LE DÉCALAGE
    // ════════════════════════════════════════════════════════════════════════
    //
    // offsetSeconds > 0 : Le tapis démarre EN RETARD (après la voix)
    //   → On ajoute un délai (adelay) sur le tapis [1:a]
    //
    // offsetSeconds < 0 : Le tapis démarre EN AVANCE (avant la voix)
    //   → On ajoute un délai (adelay) sur la voix [0:a]
    //   → ET on doit étendre la durée finale pour inclure le début du tapis
    //
    // offsetSeconds = 0 : Pas de décalage
    //
    // ════════════════════════════════════════════════════════════════════════
    
    QString filter;
    
    if (offsetSeconds > 0.01f) {
        // Tapis EN RETARD : ajouter délai sur la musique
        int delayMs = static_cast<int>(offsetSeconds * 1000);
        QString delayStr = QString::number(delayMs);
        
        filter = QString(
            "[0:a]aformat=sample_rates=44100:channel_layouts=stereo[v];"
            "[1:a]aformat=sample_rates=44100:channel_layouts=stereo,volume=%1,adelay=%2|%2[m];"
            "[v][m]amix=inputs=2:duration=first:dropout_transition=0:normalize=0[out]"
        ).arg(volStr).arg(delayStr);
    }
    else if (offsetSeconds < -0.01f) {
        // Tapis EN AVANCE : ajouter délai sur la voix + padding au début
        int delayMs = static_cast<int>(-offsetSeconds * 1000);  // Valeur positive
        QString delayStr = QString::number(delayMs);
        
        // On ajoute du silence au début de la voix avec adelay
        // Et on utilise duration=longest pour capturer le début du tapis
        // Puis on coupe à la fin avec atrim si nécessaire
        filter = QString(
            "[0:a]aformat=sample_rates=44100:channel_layouts=stereo,adelay=%2|%2[v];"
            "[1:a]aformat=sample_rates=44100:channel_layouts=stereo,volume=%1[m];"
            "[v][m]amix=inputs=2:duration=longest:dropout_transition=0:normalize=0[premix];"
            "[premix]atrim=start=0:duration=%3[out]"
        ).arg(volStr).arg(delayStr).arg(getFileDuration(voiceFile) + (-offsetSeconds));
        
        // Note: On calcule la durée totale = durée voix + offset négatif
    }
    else {
        // Pas de décalage significatif
        filter = QString(
            "[0:a]aformat=sample_rates=44100:channel_layouts=stereo[v];"
            "[1:a]aformat=sample_rates=44100:channel_layouts=stereo,volume=%1[m];"
            "[v][m]amix=inputs=2:duration=first:dropout_transition=0:normalize=0[out]"
        ).arg(volStr);
    }
    
    args << "-filter_complex" << filter;
    args << "-map" << "[out]";
    args << "-c:a" << "pcm_s16le";
    args << outputFile;

    if (!ffmpegProcess) {
        ffmpegProcess = new QProcess(this);
        ffmpegProcess->setProcessChannelMode(QProcess::MergedChannels);
        
        connect(ffmpegProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status){
            QString log = QString::fromLocal8Bit(ffmpegProcess->readAll());
            if (code != 0) {
                qWarning() << "FFmpeg error:" << log;
                emit error(log);
            }
            emit finished(code == 0 && status == QProcess::NormalExit);
        });
        
        connect(ffmpegProcess, &QProcess::errorOccurred, this, &AudioMerger::processError);
    }
    
    ffmpegProcess->start(ffmpegPath, args);
    emit started();
}
