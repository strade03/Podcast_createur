#include "waveformloader.h"
#include "audiomerger.h" // Pour getFFmpegPath
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QThread>
#include <cmath>

WaveformLoader::WaveformLoader(const QString &filePath, QObject *parent)
    : QObject(parent), m_filePath(filePath)
{
}

void WaveformLoader::process()
{
    // 1. Estimation initiale (conservatrice pour éviter de trop réserver d'un coup)
    QFileInfo fi(m_filePath);
    qint64 inputSize = fi.size();
    
    // On estime la taille mais on reste prudent
    qint64 estimatedTotalBytes = inputSize * 5; // Moins agressif que *10
    QString ext = fi.suffix().toLower();
    if (ext == "wav" || ext == "aiff") estimatedTotalBytes = inputSize + 1024;

    emit progress(0);

    QString ffmpegPath = AudioMerger::getFFmpegPath();
    int channels = AudioMerger::getChannelCount(m_filePath);

    QProcess ff;
    QStringList args;
    args << "-v" << "error" << "-i" << m_filePath;

    if (channels > 1) args << "-af" << "pan=mono|c0=0.5*c0+0.5*c1";
    else args << "-ac" << "1"; 
    
    args << "-ar" << "44100" << "-f" << "s16le" << "-";

    ff.start(ffmpegPath, args);
    
    if (!ff.waitForStarted()) {
        emit error("Impossible de lancer FFmpeg");
        return;
    }

    QVector<qint16> samples;
    
    // Tentative de réservation initiale sécurisée
    try {
        // On ne réserve pas plus de 500Mo d'un coup au démarrage par sécurité
        qint64 safeReserve = std::min(estimatedTotalBytes / 4, (qint64)(125 * 1024 * 1024)); 
        samples.reserve(safeReserve);
    } catch (const std::bad_alloc&) {
        emit error("Mémoire insuffisante dès le départ.");
        return;
    }

    qint64 totalBytesRead = 0;
    
    // Pour l'estimation de la progression, on utilise une valeur théorique pour les MP3
    // 2h = ~1.2Go de PCM. 
    qint64 targetPcmSize = (ext == "wav") ? inputSize : (inputSize * 10); 

    while (ff.state() == QProcess::Running || ff.bytesAvailable() > 0) {
        ff.waitForReadyRead(50);
        
        QByteArray chunk = ff.readAllStandardOutput();
        if (chunk.isEmpty()) {
            if (ff.state() != QProcess::Running) break;
            continue;
        }

        int sampleCount = chunk.size() / sizeof(qint16);
        int oldSize = samples.size();
        
        // --- PROTECTION CRITIQUE CONTRE LE CRASH ---
        if (samples.capacity() < oldSize + sampleCount) {
            try {
                // Au lieu de laisser QVector doubler (ex: 1Go -> 2Go), 
                // on ajoute juste ce qu'il faut + une marge raisonnable (ex: 10 Mo)
                // Cela garde la mémoire "compacte".
                int growBy = std::max(sampleCount, 10 * 1024 * 1024); // +10 millions de samples max
                samples.reserve(oldSize + growBy);
            } catch (const std::bad_alloc&) {
                ff.kill();
                emit error("Fichier trop gros pour la mémoire disponible (RAM saturée).");
                return;
            }
        }
        // -------------------------------------------

        samples.resize(oldSize + sampleCount);
        memcpy(samples.data() + oldSize, chunk.constData(), chunk.size());

        totalBytesRead += chunk.size();
        
        // Progression
        if (targetPcmSize > 0) {
            int p = (int)((totalBytesRead * 100) / targetPcmSize);
            if (p > 99) p = 99;
            emit progress(p);
        }
    }

    if (ff.exitStatus() != QProcess::NormalExit && totalBytesRead == 0) {
        emit error("Erreur lecture audio");
        return;
    }

    // Libération mémoire finale si on a trop pris
    samples.squeeze();
    auto sharedSamples = QSharedPointer<QVector<qint16>>::create(std::move(samples));

    emit progress(100);
    emit finished(sharedSamples, sharedSamples->size());
}
