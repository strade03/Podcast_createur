#ifndef PROJECTUTILS_H
#define PROJECTUTILS_H

#include <QString>
#include <QStringList>
#include <QFile>

class ProjectUtils {
public:
    // Retourne le chemin complet du fichier audio s'il existe
    static QString findExistingAudioFile(const QString &basePathWithoutExt);

    // Retourne la liste des extensions supportées (wav, mp3, ogg, etc.)
    static QStringList getSupportedExtensions();

    // NOUVEAU : Retourne la chaîne formatée pour QFileDialog
    // Ex: "Audio (*.wav *.mp3 *.m4a *.aac *.ac3 *.aiff *.flac *.ogg *.opus *.wma)"
    static QString getFileDialogFilter();
};

#endif // PROJECTUTILS_H