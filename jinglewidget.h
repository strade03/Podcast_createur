#ifndef JINGLEWIDGET_H
#define JINGLEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

class JingleWidget : public QWidget {
    Q_OBJECT
public:
    enum Mode { LibraryMode, SelectorMode };

    explicit JingleWidget(const QString &path, Mode mode, QWidget *parent = nullptr);

    QString getFilePath() const { return m_filePath; }
    
    // Accesseurs pour les connexions
    QPushButton* playBtn() const { return m_btnPlay; }
    QPushButton* deleteBtn() const { return m_btnDelete; }
    QPushButton* menuBtn() const { return m_btnMenu; }

    // États visuels
    void setPlayingState(bool isPlaying);
    void setSelectedState(bool selected); // Pour le SelectorMode uniquement

private:
    QString m_filePath;
    Mode m_mode;
    
    QFrame *m_bgFrame;
    QLabel *m_lblName;
    QPushButton *m_btnPlay;
    QPushButton *m_btnDelete;
    QPushButton *m_btnMenu;

    void setupUi();
};

#endif // JINGLEWIDGET_H