#ifndef JINGLEMANAGER_H
#define JINGLEMANAGER_H

#include <QDialog>
#include <QListWidget>
#include <QMediaPlayer>
#include <QAudioOutput>

class JingleManager : public QDialog
{
    Q_OBJECT
public:
    explicit JingleManager(QWidget *parent = nullptr);
    ~JingleManager();
    
    static QString getLibraryPath();

private slots:
    void importJingle();
    void deleteJingle(const QString &filePath);
    void editJingleAudio(const QString &filePath);
    void renameJingle(const QString &filePath); // <--- NOUVEAU
    void playPreview(const QString &filePath);
    void refreshList();
    void updatePlayIcons();

private:
    QListWidget *listWidget;
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    QString currentPlayingPath;

    void setupUi();
};

#endif // JINGLEMANAGER_H