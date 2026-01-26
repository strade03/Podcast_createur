#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSettings>
#include <QProgressDialog> 

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // Méthodes statiques pour récupérer les valeurs facilement depuis n'importe où dans l'app
    static QString getGlobalPath();    
    static QString getExportPath();
    static QString getOutputFormat();

private slots:
    void browseWorkspace();
    void browseExport();
    void saveSettings();

private:
    QLineEdit *editWorkspace;
    QLineEdit *editExport;
    QComboBox *comboFormat;
    
    void setupUi();
    void loadCurrentSettings();

    bool moveGlobalDirectory(const QString &oldPath, const QString &newPath);
    bool copyRecursively(const QString &srcFilePath, const QString &tgtFilePath);

    int countTotalFiles(const QString &path);
    bool copyRecursively(const QString &src, const QString &tgt, QProgressDialog &progress, int &count);
};

#endif // SETTINGSDIALOG_H