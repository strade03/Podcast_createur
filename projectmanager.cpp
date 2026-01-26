#include "projectmanager.h"
#include "projectutils.h" // On réutilise vos utilitaires existants pour les extensions

QString ProjectManager::getScriptPath(const QString &projectPath, const QString &name) {
    return QDir(projectPath).filePath(name + ".txt");
}

QString ProjectManager::getAudioPath(const QString &projectPath, const QString &name) {
    QString base = QDir(projectPath).filePath(name);
    QString existing = ProjectUtils::findExistingAudioFile(base);
    if (!existing.isEmpty()) return existing;
    return base + ".wav"; // Défaut
}

QString ProjectManager::getPlaylistPath(const QString &projectPath) {
    return QDir(projectPath).filePath("playlist.txt");
}

QStringList ProjectManager::loadPlaylist(const QString &projectPath) {
    QStringList items;
    QFile f(getPlaylistPath(projectPath));
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        in.setEncoding(QStringConverter::Utf8);
        #else
        in.setCodec("UTF-8");
        #endif
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) items.append(line);
        }
        f.close();
    }
    return items;
}

bool ProjectManager::savePlaylist(const QString &projectPath, const QStringList &items) {
    QFile f(getPlaylistPath(projectPath));
    if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&f);
        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        out.setEncoding(QStringConverter::Utf8);
        #else
        out.setCodec("UTF-8");
        #endif
        for (const QString &item : items) {
            out << item << "\n";
        }
        f.close();
        return true;
    }
    return false;
}

bool ProjectManager::appendToPlaylist(const QString &projectPath, const QString &itemName) {
    QStringList current = loadPlaylist(projectPath);
    current.append(itemName);
    return savePlaylist(projectPath, current);
}

bool ProjectManager::renameInPlaylist(const QString &projectPath, const QString &oldName, const QString &newName) {
    QStringList items = loadPlaylist(projectPath);
    int idx = -1;
    
    // Recherche exacte
    idx = items.indexOf(oldName);
    // Fallback insensible à la casse
    if (idx == -1) {
        for(int i=0; i<items.size(); i++) {
            if (items[i].compare(oldName, Qt::CaseInsensitive) == 0) {
                idx = i;
                break;
            }
        }
    }
    
    if (idx != -1) {
        items[idx] = newName;
        return savePlaylist(projectPath, items);
    }
    // Si pas trouvé, on ajoute
    return appendToPlaylist(projectPath, newName);
}

bool ProjectManager::removeFromPlaylist(const QString &projectPath, const QString &name) {
    QStringList items = loadPlaylist(projectPath);
    items.removeAll(name);
    return savePlaylist(projectPath, items);
}