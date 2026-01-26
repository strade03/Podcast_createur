#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QStandardPaths>
#include <QGroupBox>
#include <QStorageInfo>

#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Paramètres");
    resize(500, 350);
    setStyleSheet("QDialog { background-color: #F5F5F5; } QLabel { color: #333; }");
    
    setupUi();
    loadCurrentSettings();
}

void SettingsDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // --- GROUPE 1 : DOSSIERS ---
    QGroupBox *groupFolders = new QGroupBox("Emplacements", this);
    groupFolders->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #CCC; border-radius: 6px; margin-top: 6px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    QVBoxLayout *layoutFolders = new QVBoxLayout(groupFolders);
    layoutFolders->setSpacing(15);

    // 1. Dossier de travail (Emissions)
    QLabel *lblWork = new QLabel("Dossier Racine (contenant Emissions et Library) :", this);
    layoutFolders->addWidget(lblWork);
    
    QHBoxLayout *hWork = new QHBoxLayout();
    editWorkspace = new QLineEdit(this);
    editWorkspace->setReadOnly(true); // Lecture seule pour éviter les erreurs manuelles
    QPushButton *btnBrowseWork = new QPushButton("Parcourir...", this);
    connect(btnBrowseWork, &QPushButton::clicked, this, &SettingsDialog::browseWorkspace);
    hWork->addWidget(editWorkspace);
    hWork->addWidget(btnBrowseWork);
    layoutFolders->addLayout(hWork);

    // 2. Dossier d'export (Sortie finale)
    QLabel *lblExport = new QLabel("Dossier d'exportation (Rendu final) :", this);
    layoutFolders->addWidget(lblExport);

    QHBoxLayout *hExport = new QHBoxLayout();
    editExport = new QLineEdit(this);
    editExport->setReadOnly(true);
    QPushButton *btnBrowseExport = new QPushButton("Parcourir...", this);
    connect(btnBrowseExport, &QPushButton::clicked, this, &SettingsDialog::browseExport);
    hExport->addWidget(editExport);
    hExport->addWidget(btnBrowseExport);
    layoutFolders->addLayout(hExport);

    mainLayout->addWidget(groupFolders);

    // --- GROUPE 2 : FORMAT ---
    QGroupBox *groupFormat = new QGroupBox("Format Audio", this);
    groupFormat->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #CCC; border-radius: 6px; margin-top: 6px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    QVBoxLayout *layoutFormat = new QVBoxLayout(groupFormat);

    QLabel *lblFormat = new QLabel("Format du fichier fusionné :", this);
    layoutFormat->addWidget(lblFormat);

    comboFormat = new QComboBox(this);
    comboFormat->addItem("MP3 (*.mp3)", "mp3");
    comboFormat->addItem("WAV (*.wav)", "wav");
    comboFormat->addItem("M4A / AAC (*.m4a)", "m4a");
    comboFormat->addItem("FLAC (Lossless) (*.flac)", "flac");
    comboFormat->addItem("OGG Vorbis (*.ogg)", "ogg");
    comboFormat->addItem("OPUS (*.opus)", "opus");
    comboFormat->addItem("AC3 (*.ac3)", "ac3");
    comboFormat->addItem("WMA (*.wma)", "wma");
    comboFormat->addItem("AIFF (*.aiff)", "aiff");
        
    layoutFormat->addWidget(comboFormat);

    mainLayout->addWidget(groupFormat);
    mainLayout->addStretch();

    // --- BOUTONS ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    QPushButton *btnSave = new QPushButton("Enregistrer", this);
    btnSave->setCursor(Qt::PointingHandCursor);
    btnSave->setStyleSheet("background-color: #2196F3; color: white; padding: 8px 20px; border-radius: 4px; font-weight: bold; border: none;");
    connect(btnSave, &QPushButton::clicked, this, &SettingsDialog::saveSettings);

    QPushButton *btnCancel = new QPushButton("Annuler", this);
    btnCancel->setCursor(Qt::PointingHandCursor);
    btnCancel->setStyleSheet("background-color: transparent; color: #555; padding: 8px 20px; border: 1px solid #CCC; border-radius: 4px;");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);
    mainLayout->addLayout(btnLayout);
}

QString SettingsDialog::getGlobalPath()
{
    QSettings settings;
    // Par défaut : Documents/PodcastCreator (sans le /Emissions)
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/PodcastCreator";
    return settings.value("globalPath", defaultPath).toString();
}


void SettingsDialog::loadCurrentSettings()
{
    editWorkspace->setText(getGlobalPath());
    editExport->setText(getExportPath());
    
    QString currentFormat = getOutputFormat();
    int index = comboFormat->findData(currentFormat);
    if (index >= 0) comboFormat->setCurrentIndex(index);
}


QString SettingsDialog::getExportPath()
{
    QSettings settings;
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation); // Music par défaut
    return settings.value("exportPath", defaultPath).toString();
}

QString SettingsDialog::getOutputFormat()
{
    QSettings settings;
    return settings.value("outputFormat", "mp3").toString();
}

// --- SLOTS ---

void SettingsDialog::browseWorkspace()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Choisir le dossier des projets", editWorkspace->text());
    if (!dir.isEmpty()) editWorkspace->setText(dir);
}

void SettingsDialog::browseExport()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Choisir le dossier d'exportation", editExport->text());
    if (!dir.isEmpty()) editExport->setText(dir);
}

// void SettingsDialog::saveSettings()
// {
//     QSettings settings;
//     settings.setValue("globalPath", editWorkspace->text());
//     settings.setValue("exportPath", editExport->text());
//     settings.setValue("outputFormat", comboFormat->currentData().toString());
    
//     accept(); // Ferme la fenêtre et renvoie Accepted
// }

void SettingsDialog::saveSettings()
{
    // 1. Récupérer l'ancien chemin (avant sauvegarde)
    QString oldPath = QDir::cleanPath(getGlobalPath());
    QString newPath = QDir::cleanPath(editWorkspace->text());

    // 2. Détection du changement
    if (oldPath != newPath) {
        
        QDir oldDir(oldPath);
        // Si l'ancien dossier existe et contient des données
        if (oldDir.exists() && (oldDir.exists("Emissions") || oldDir.exists("Library"))) {
            
            // --- DÉTECTION DU DISQUE ---
            QStorageInfo storageOld(oldPath);
            QStorageInfo storageNew(newPath);
            
            // Vérifie si c'est le même volume (ex: C: vers C:)
            // Note : isValid() vérifie que le disque est accessible
            bool isSameDrive = storageOld.isValid() && storageNew.isValid() && 
                               (storageOld.device() == storageNew.device());

            // --- PRÉPARATION DU MESSAGE ADAPTÉ ---
            QString msgTitle = "Déplacer les données ?";
            QString msgText = "Vous avez changé le dossier de travail.\n\n"
                              "Ancien : " + oldPath + "\n"
                              "Nouveau : " + newPath  ;

            if (!isSameDrive) {
                msgText += "\n\nATTENTION : Les dossiers sont sur des disques différents.\n"
                           "Les données seront COPIÉES (cela peut prendre du temps selon la taille).\n"
                           "L'ancien dossier sera ensuite renommé en '_OLD' par sécurité.";
            }
            
            msgText += "\n\nVoulez-vous déplacer vos données maintenant ?";

            // QMessageBox::StandardButton reply;
            // reply = QMessageBox::question(this, msgTitle, msgText, QMessageBox::Yes | QMessageBox::No);
            QMessageBox msgBox(this);
            msgBox.setWindowTitle(msgTitle);
            msgBox.setText(msgText);
            msgBox.setIcon(QMessageBox::Question);
            
            // Création manuelle des boutons en français
            QPushButton *btnOui = msgBox.addButton(tr("Oui"), QMessageBox::YesRole);
            QPushButton *btnNon = msgBox.addButton(tr("Non"), QMessageBox::NoRole);

            // On met "Non" par défaut pour éviter les accidents
            msgBox.setDefaultButton(btnNon);
            
            msgBox.exec();

            // On vérifie quel bouton a été cliqué
            if (msgBox.clickedButton() == btnOui) {
                bool success = false;

                // Cas 1 : Même disque -> Déplacement atomique (Rapide)
                if (isSameDrive) {
                    QDir targetDir(newPath);
                    
                    // QDir::rename échoue si le dossier cible existe déjà.
                    // Si le dossier cible est vide (créé par le sélecteur de fichier), on le supprime d'abord.
                    if (targetDir.exists()) {
                        if (targetDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty()) {
                            targetDir.rmdir("."); // On supprime le dossier vide pour permettre le renommage
                        } else {
                            QMessageBox::warning(this, tr("Erreur"), 
                                tr("Le dossier de destination existe déjà et n'est pas vide.\n"
                                "Impossible de faire un déplacement automatique."));
                            return;
                        }
                    }
                    
                    // Le vrai déplacement
                    success = QDir().rename(oldPath, newPath);
                } 
                // Cas 2 : Disque différent -> Copie + Rename (Lent)
                else {
                    // On garde votre logique existante avec la barre de progression
                    success = moveGlobalDirectory(oldPath, newPath);
                }

                if (!success) {
                    QMessageBox::critical(this, tr("Erreur"), 
                        tr("L'opération a échoué (fichiers verrouillés ou permissions insuffisantes).\n"
                        "Le changement de dossier n'a pas été sauvegardé."));
                    return; // On annule tout
                }
            }
        }
    }

    // 3. Sauvegarde classique des paramètres
    QSettings settings;
    settings.setValue("globalPath", editWorkspace->text());
    settings.setValue("exportPath", editExport->text());
    settings.setValue("outputFormat", comboFormat->currentData().toString());
    
    accept();
}

int SettingsDialog::countTotalFiles(const QString &path)
{
    int count = 0;
    QDir dir(path);
    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
    
    for (const QFileInfo &info : list) {
        if (info.isDir()) {
            count += countTotalFiles(info.absoluteFilePath()); // Récursion
        } else {
            count++;
        }
    }
    return count;
}

bool SettingsDialog::moveGlobalDirectory(const QString &oldPath, const QString &newPath)
{
    // 1. Compter les fichiers pour la barre de progression
    QApplication::setOverrideCursor(Qt::WaitCursor);
    int totalFiles = countTotalFiles(oldPath);
    QApplication::restoreOverrideCursor();

    // 2. Créer le dialogue de progression
    QProgressDialog progress("Déplacement des fichiers en cours...", "Annuler", 0, totalFiles, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0); // Afficher tout de suite
    progress.setValue(0);
    
    // Compteur partagé
    int currentCount = 0;

    // 3. Lancer la copie récursive
    // Si l'utilisateur annule ou erreur, on arrête
    if (!copyRecursively(oldPath, newPath, progress, currentCount)) {
        return false;
    }

    // 4. Renommer l'ancien dossier (Archivage)
    // On le fait seulement si tout s'est bien passé
    QDir().rename(oldPath, oldPath + "_OLD");
    
    return true;
}

bool SettingsDialog::copyRecursively(const QString &srcFilePath, const QString &tgtFilePath, QProgressDialog &progress, int &count)
{
    QFileInfo srcFileInfo(srcFilePath);

    // Vérification annulation utilisateur
    if (progress.wasCanceled()) return false;

    if (srcFileInfo.isDir()) {
        QDir targetDir(tgtFilePath);
        if (!targetDir.exists()) {
            if (!targetDir.mkpath(".")) return false;
        }

        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);

        foreach (const QString &fileName, fileNames) {
            const QString newSrc = sourceDir.filePath(fileName); // Utilisation de filePath ici aussi !
            const QString newTgt = targetDir.filePath(fileName);
            
            if (!copyRecursively(newSrc, newTgt, progress, count))
                return false;
        }
    } else {
        // C'est un fichier : on copie
        if (QFile::exists(tgtFilePath)) {
            if (!QFile::remove(tgtFilePath)) return false;
        }
        
        if (!QFile::copy(srcFilePath, tgtFilePath)) return false;

        // --- MISE À JOUR PROGRESSION ---
        count++;
        progress.setValue(count);
        
        // Permet à l'interface de ne pas geler et de détecter le clic "Annuler"
        QCoreApplication::processEvents(); 
    }
    return true;
}

// bool SettingsDialog::moveGlobalDirectory(const QString &oldPath, const QString &newPath)
// {
//     // 1. Copie récursive
//     if (!copyRecursively(oldPath, newPath)) {
//         return false;
//     }

//     // 2. Si la copie est un succès total, on supprime l'ancien dossier
//     // QDir(oldPath).removeRecursively(); 
//     // NOTE DE SÉCURITÉ : Pour l'instant, je recommande de NE PAS supprimer automatiquement 
//     // l'ancien dossier par code. Si un fichier est verrouillé, on risque de perdre des données.
//     // Mieux vaut laisser l'utilisateur supprimer l'ancien dossier "PodcastCreator" manuellement une fois sûr.
    
//     // Si vous êtes confiant, décommentez la ligne ci-dessus.
//     // Sinon, on peut renommer l'ancien pour dire qu'il est archivé :
//     QDir().rename(oldPath, oldPath + "_OLD");
    
//     return true;
// }

// bool SettingsDialog::copyRecursively(const QString &srcFilePath, const QString &tgtFilePath)
// {
//     QFileInfo srcFileInfo(srcFilePath);

//     if (srcFileInfo.isDir()) {
//         QDir targetDir(tgtFilePath);
//         // Créer le dossier cible s'il n'existe pas
//         if (!targetDir.exists()) {
//             if (!targetDir.mkpath(".")) return false;
//         }

//         QDir sourceDir(srcFilePath);
//         QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);

//         foreach (const QString &fileName, fileNames) {
//             const QString newSrcFilePath = srcFilePath + "/" + fileName;
//             const QString newTgtFilePath = tgtFilePath + "/" + fileName;
//             if (!copyRecursively(newSrcFilePath, newTgtFilePath))
//                 return false;
//         }
//     } else {
//         // C'est un fichier, on copie
//         // Si le fichier existe déjà, on le supprime avant (écrasement)
//         if (QFile::exists(tgtFilePath)) {
//             if (!QFile::remove(tgtFilePath)) return false;
//         }
//         if (!QFile::copy(srcFilePath, tgtFilePath))
//             return false;
//     }
//     return true;
// }

