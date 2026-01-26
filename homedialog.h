#ifndef HOMEDIALOG_H
#define HOMEDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>

class HomeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HomeDialog(QWidget *parent = nullptr);

    QString getSelectedProjectName() const { return m_selectedName; }
    QString getSelectedProjectPath() const { return m_selectedPath; }

private slots:
    void createNewProject();
    void openSelectedProject(QListWidgetItem *item);
    void refreshProjectList();
    // Suppression d'un projet spécifique (appelé depuis le bouton poubelle de la ligne)
    void deleteSpecificProject(const QString &name);

private:
    QString rootPath;
    QListWidget *projectList;
    
    QString m_selectedName;
    QString m_selectedPath;
    
    void setupUi();
};

#endif // HOMEDIALOG_H