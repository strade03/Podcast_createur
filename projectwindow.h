#ifndef PROJECTWINDOW_H
#define PROJECTWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QListWidget>
#include <QProgressDialog>

#include "draggablelistwidget.h"
#include "chroniclewidget.h"


class QMediaPlayer;
class QAudioOutput;
class AudioMerger;

class ProjectWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ProjectWindow(const QString &projectName, const QString &projectPath, QWidget *parent = nullptr);
    ~ProjectWindow();

private slots:
    void addChronicle();
    void importAudioFile();
    void mergeProject(); 
    
    void onOpenScript(ChronicleWidget *w);
    void onRecord(ChronicleWidget *w);
    void onPlay(ChronicleWidget *w);
    void onDuplicate(ChronicleWidget *w);
    void onEditAudio(ChronicleWidget *w);
    void onRename(ChronicleWidget *w);
    void onDelete(ChronicleWidget *w);
    void onMoveUp(ChronicleWidget *w);
    void onMoveDown(ChronicleWidget *w);

    void refreshList(); 
    void onMergeFinished(bool success);
    
    //void onListOrderChanged(const QModelIndex &, int, int, const QModelIndex &, int);
    void onListOrderChanged();

    void onStop(ChronicleWidget *); 
    void addJingle();

private:
    QString m_projectName;
    QString m_projectPath;
    QProgressDialog *m_progressDialog = nullptr;
    
    // QListWidget *chronicleListWidget; // Remplacement QScrollArea
    DraggableListWidget *chronicleListWidget; 

    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    AudioMerger *merger;
    
    void setupUi();
    void saveOrder();
    void connectChronicleWidget(ChronicleWidget *w);
    QStringList getAssociatedAudioFiles(const QString &baseName) const;
};

#endif // PROJECTWINDOW_H