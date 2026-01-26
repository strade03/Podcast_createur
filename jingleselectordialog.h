#ifndef JINGLESELECTORDIALOG_H
#define JINGLESELECTORDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPushButton>
#include <QLabel>

class JingleSelectorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit JingleSelectorDialog(QWidget *parent = nullptr);
    ~JingleSelectorDialog(); // Destructeur pour arrêter le son
    
    QStringList getSelectedJingles() const;

private slots:
    void playPreview(const QString &path);
    void updateWidgetsStyle(); // Met à jour l'aspect visuel (vert/blanc) selon la sélection
    void selectAll(); // Slot pour le bouton "Ajouter tout"

private:
    QListWidget *listWidget;
    QLabel *lblInfo;
    QPushButton *btnSelectAll;
    
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    QString currentPlayingPath; // Pour savoir quelle icône mettre en "Stop"

    void setupUi();
    void loadJingles();
};

#endif // JINGLESELECTORDIALOG_H