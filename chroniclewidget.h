#ifndef CHRONICLEWIDGET_H
#define CHRONICLEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class ChronicleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChronicleWidget(const QString &name, 
                             const QString &projectPath, QWidget *parent = nullptr);

    QString getName() const { return m_name; }
    QString getBaseName() const { return m_name; } 
    
    // Met à jour l'UI selon la présence des fichiers
    void refreshStatus(); 
    
    // Getters fichiers
    QString getScriptPath() const;
    QString getAudioPath() const; // Retourne le chemin s'il existe (wav/mp3/m4a), sinon chemin wav par défaut


    void setPlaybackState(bool playing); 
    QString getProjectPath() const { return m_projectPath; }
    bool isJingleItem() const; 
    QString getEffectiveAudioPath() const;
signals:
    void openScriptRequested();
    void duplicateRequested();
    void recordRequested();
    void playRequested();
    void editAudioRequested();
    void renameRequested();
    void deleteRequested();
    void moveUpRequested();
    void moveDownRequested();
    void stopRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;		  

private:
    QString m_name;
    QString m_projectPath;

    QLabel *lblName;
    QLabel *lblStatus;
    QPushButton *btnScript;
    QPushButton *btnAction; // Record ou Play/Stop
    QPushButton *btnMenu;
    QPushButton *btnUp;
    QPushButton *btnDown;
    
    bool isPlaying;
    
    void setupUi();
};

#endif // CHRONICLEWIDGET_H