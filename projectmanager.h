#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>

class ProjectManager {
public:
    // --- GESTION DES CHEMINS ---
    static QString getScriptPath(const QString &projectPath, const QString &name);
    // Retourne le chemin audio (wav/mp3...) s'il existe, sinon le chemin wav par défaut
    static QString getAudioPath(const QString &projectPath, const QString &name);
    static QString getPlaylistPath(const QString &projectPath);

    // --- GESTION DE LA PLAYLIST ---
    static QStringList loadPlaylist(const QString &projectPath);
    static bool savePlaylist(const QString &projectPath, const QStringList &items);
    static bool appendToPlaylist(const QString &projectPath, const QString &itemName);
    static bool renameInPlaylist(const QString &projectPath, const QString &oldName, const QString &newName);
    static bool removeFromPlaylist(const QString &projectPath, const QString &name);
};

#endif // PROJECTMANAGER_H