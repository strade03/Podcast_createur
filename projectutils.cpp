#include "projectutils.h"

QStringList ProjectUtils::getSupportedExtensions() {
    // Liste complète des formats demandés
    return {
        ".wav", ".mp3", ".m4a", ".aac", ".ac3", 
        ".aiff", ".flac", ".ogg", ".opus", ".wma"
    };
}

QString ProjectUtils::getFileDialogFilter() {
    QStringList exts = getSupportedExtensions();
    QStringList filters;
    
    // On transforme ".wav" en "*.wav"
    for(const QString &ext : exts) {
        filters << "*" + ext;
    }
    
    // On joint tout avec des espaces
    return QString("Audio (%1)").arg(filters.join(" "));
}

QString ProjectUtils::findExistingAudioFile(const QString &basePathWithoutExt) {
    for (const QString &ext : getSupportedExtensions()) {
        QString fullPath = basePathWithoutExt + ext;
        if (QFile::exists(fullPath)) {
            return fullPath;
        }
    }
    return QString(); 
}