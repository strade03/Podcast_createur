#include "projectwindow.h"
#include "scripteditor.h"
#include "audioeditor.h"
#include "audiorecorder.h"

#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QRegularExpression>

#include <QMediaPlayer>
#include <QAudioOutput>

#include "audiomerger.h"
#include "jinglemanager.h"
#include "jingleselectordialog.h"
#include "settingsdialog.h"
#include "projectutils.h" 
#include "projectmanager.h"

ProjectWindow::ProjectWindow(const QString &name, const QString &path, QWidget *parent)
    : QMainWindow(parent), m_projectName(name), m_projectPath(path), player(nullptr), audioOutput(nullptr)
{
    setWindowTitle(m_projectName + " - Podcast Createur");
    resize(850, 700);
    setAttribute(Qt::WA_DeleteOnClose); 
    
    // OPTIMISATION : On NE charge PAS le player ici.
    // Cela rend l'ouverture de la fenêtre instantanée.
    //player = new QMediaPlayer(this); // <-- SUPPRIMÉ
    
    // merger = new AudioMerger(this);
    // connect(merger, &AudioMerger::finished, this, &ProjectWindow::onMergeFinished);
    merger = nullptr;

    setupUi();
    QTimer::singleShot(100, this, &ProjectWindow::refreshList); // on retarde le refresh liste pour ne pas être bloqué par l'antivirus
    
}

ProjectWindow::~ProjectWindow() {
    if (player) {
        player->stop();          // Arrête la lecture immédiatement
        player->setSource(QUrl()); // Libère le fichier (file lock)
        delete player;           // Détruit l'objet et libère le backend audio
        player = nullptr;
    }
}

void ProjectWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    central->setStyleSheet("background-color: #F5F5F5;"); // Même gris clair que l'accueil pour cohérence

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    // Header Toolbar
    QWidget *toolbar = new QWidget(this);
    toolbar->setStyleSheet("background-color: #37474F; padding: 10px;");
    QHBoxLayout *toolsLayout = new QHBoxLayout(toolbar);

    // BOUTON RETOUR ===== TODO A reprendre pour appeler de main showHome
    QPushButton *btnBack = new QPushButton("← Retour", this);
    btnBack->setStyleSheet(
        "QPushButton { color: white; padding: 8px 15px; font-weight: bold; "
        "border-radius: 4px; border: none; background-color: #546E7A; }"
        "QPushButton:hover { background-color: #607D8B; }"
    );
    connect(btnBack, &QPushButton::clicked, this, [this]() {
        // Fermer la fenêtre actuelle pour retourner à l'accueil
        close();
    });
    toolsLayout->addWidget(btnBack);
    // ==================================    



    QLabel *lblTitle = new QLabel(m_projectName, this);
    lblTitle->setStyleSheet("color: white; font-size: 18px; font-weight: bold;");
    toolsLayout->addWidget(lblTitle);
    
    toolsLayout->addStretch();

    // Style boutons header
    QString btnStyle = "QPushButton { color: white; padding: 8px 15px; font-weight: bold; border-radius: 4px; border: none; }"
                       "QPushButton:hover { opacity: 0.8; }";

    QPushButton *btnAdd = new QPushButton("+ Chronique", this);
    btnAdd->setStyleSheet(btnStyle + "QPushButton { background-color: #D32F2F; }");
    connect(btnAdd, &QPushButton::clicked, this, &ProjectWindow::addChronicle);
    
    QPushButton *btnImport = new QPushButton("Importer", this);
    btnImport->setStyleSheet(btnStyle + "QPushButton { background-color: #1976D2; }");
    connect(btnImport, &QPushButton::clicked, this, &ProjectWindow::importAudioFile);

    QPushButton *btnMerge = new QPushButton("Fusionner", this);
    btnMerge->setStyleSheet(btnStyle + "QPushButton { background-color: #388E3C; }");
    connect(btnMerge, &QPushButton::clicked, this, &ProjectWindow::mergeProject);

    QPushButton *btnAddJingle = new QPushButton("+ Jingle", this);
    btnAddJingle->setStyleSheet(btnStyle + "QPushButton { background-color: #7B1FA2; }"); // Violet
    connect(btnAddJingle, &QPushButton::clicked, this, &ProjectWindow::addJingle); // CORRECTION : Connexion manquante ajoutée

    toolsLayout->addWidget(btnAddJingle);

    toolsLayout->addWidget(btnAdd);
    toolsLayout->addWidget(btnImport);
    toolsLayout->addWidget(btnMerge);
    
    mainLayout->addWidget(toolbar);

    // Hint Reorder
    // QLabel *hint = new QLabel("Maintenez cliqué pour déplacer l'ordre des chroniques", this); TODO voir quel texte on garde
	QLabel *hint = new QLabel("Glissez-déposez pour réorganiser les chroniques", this);	
    hint->setStyleSheet("color: #888888; font-size: 11px; padding: 5px;");
    hint->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(hint);

    // ListWidget
    chronicleListWidget = new DraggableListWidget(this);
    chronicleListWidget->setFrameShape(QFrame::NoFrame);

    chronicleListWidget->setStyleSheet(
        "QListWidget { background-color: #F5F5F5; border: none; outline: none; } "
        "QListWidget::item { background: transparent; padding: 0px; border: none; }" 
        "QListWidget::item:selected { background: transparent; }"
        "QListWidget::item:hover { background: transparent; }"
    );

    chronicleListWidget->setSpacing(10);     
											 
    connect(chronicleListWidget, &DraggableListWidget::orderChanged, 
            this, &ProjectWindow::onListOrderChanged);

    mainLayout->addWidget(chronicleListWidget);
}												   
									
QStringList ProjectWindow::getAssociatedAudioFiles(const QString &baseName) const
{
    QStringList files;
    QString base = QDir(m_projectPath).filePath(baseName); 
    
    // Utilisation de la liste centralisée
    const QStringList extensions = ProjectUtils::getSupportedExtensions();
    
    for (const QString &ext : extensions) {
        QString fullPath = base + ext;
        if (QFile::exists(fullPath)) {
            files << fullPath;
        }
    }
    return files;
}  

void ProjectWindow::connectChronicleWidget(ChronicleWidget *w)
{
    if (!w) return;

    // 1. SÉCURITÉ : On coupe les anciens fils vers 'this' pour éviter les doublons
    // (Utile si on rappelle la fonction sur un widget déjà connecté)
    w->disconnect(this);

    // 2. RECONNEXION COMPLÈTE
    connect(w, &ChronicleWidget::openScriptRequested, this, [this, w](){ onOpenScript(w); });
    connect(w, &ChronicleWidget::duplicateRequested, this, [this, w](){ onDuplicate(w); });
    connect(w, &ChronicleWidget::recordRequested, this, [this, w](){ onRecord(w); });
    connect(w, &ChronicleWidget::playRequested, this, [this, w](){ onPlay(w); });
    connect(w, &ChronicleWidget::stopRequested, this, [this, w](){ onStop(w); });
    connect(w, &ChronicleWidget::editAudioRequested, this, [this, w](){ onEditAudio(w); });
    connect(w, &ChronicleWidget::renameRequested, this, [this, w](){ onRename(w); });
    connect(w, &ChronicleWidget::deleteRequested, this, [this, w](){ onDelete(w); });
}

void ProjectWindow::onDuplicate(ChronicleWidget *w)
{
    QString originalName = w->getName();
    QString sourceTxtPath = w->getScriptPath();

    // 1. Lire le contenu (le lien)
    QString content;
    QFile fSource(sourceTxtPath);
    if (fSource.open(QIODevice::ReadOnly | QIODevice::Text)) {
        content = QString::fromUtf8(fSource.readAll());
        fSource.close();
    } else {
        QMessageBox::warning(this, "Erreur", "Impossible de lire le fichier source.");
        return;
    }

    // 2. Trouver un nom unique
    QString baseName = originalName;
    QRegularExpression re("_\\d+$"); 
    baseName.remove(re);

    int counter = 1;
    QString finalName = baseName;
    QString destTxtPath = ProjectManager::getScriptPath(m_projectPath, finalName);
    
    while(QFile::exists(destTxtPath)) {
         finalName = baseName + QString("_%1").arg(counter++);
         destTxtPath = ProjectManager::getScriptPath(m_projectPath, finalName);
    }
    
    // 3. Créer le fichier sur le disque
    QFile fDest(destTxtPath);
    if (fDest.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&fDest);
        out << content;
        fDest.close();
    } else {
        QMessageBox::warning(this, "Erreur", "Impossible de créer la copie.");
        return;
    }

    // ========================================================================
    // NOUVELLE LOGIQUE D'INSERTION
    // ========================================================================
    
    // 4. Repérer la position actuelle de l'élément à dupliquer
    int currentRow = -1;
    for(int i = 0; i < chronicleListWidget->count(); i++) {
        QListWidgetItem *item = chronicleListWidget->item(i);
        if (chronicleListWidget->itemWidget(item) == w) {
            currentRow = i;
            break;
        }
    }

    // 5. Mettre à jour la playlist.txt
    QStringList playlist = ProjectManager::loadPlaylist(m_projectPath);
    
    // Insertion intelligente
    if (currentRow != -1 && currentRow < playlist.size()) {
        playlist.insert(currentRow + 1, finalName);
    } else {
        playlist.append(finalName);
    }
    
    ProjectManager::savePlaylist(m_projectPath, playlist);

    // --- TOUT LE BLOC SUIVANT DOIT ÊTRE SUPPRIMÉ ---
    // Il contenait "if (currentRow != -1 && currentRow < playlistLines.size())"
    // et la réécriture manuelle du fichier.
    // SUPPRIMEZ JUSQU'À : fPlaylist.close(); inclus.
    // -----------------------------------------------

    // 6. Rafraîchir
    refreshList();
    
    // Selectionner le nouvel item
    if (currentRow + 1 < chronicleListWidget->count()) {
        chronicleListWidget->setCurrentRow(currentRow + 1);
        chronicleListWidget->scrollToItem(chronicleListWidget->item(currentRow + 1));
    } else {
        chronicleListWidget->scrollToBottom();
    }
}

// sauvegarder quand le drag est fini 
void ProjectWindow::onListOrderChanged()
{
    // 1. On sauvegarde le nouvel ordre dans le fichier playlist.txt
    saveOrder(); 

    // 2. OPTIMISATION : On ne fait plus refreshList().
    // On cherche l'item qui a perdu son widget (celui qu'on vient de déplacer)
    // et on le recrée uniquement lui.
    
    for(int i = 0; i < chronicleListWidget->count(); i++) {
        QListWidgetItem *item = chronicleListWidget->item(i);
        
        // Si cet item n'a plus de widget visuel (c'est celui qui a bougé)
        if (chronicleListWidget->itemWidget(item) == nullptr) {
            
            // On récupère le nom qu'on avait stocké dans les données cachées
            QString baseName = item->data(Qt::UserRole).toString();
            
            // On recrée le widget
            ChronicleWidget *w = new ChronicleWidget(baseName, m_projectPath, nullptr);
            item->setSizeHint(w->sizeHint()); // Important pour la hauteur
            
            // On le remet en place
            chronicleListWidget->setItemWidget(item, w);
            
            // On reconnecte les signaux (Play, Edit, etc.)
            connectChronicleWidget(w);
            
            // Pas besoin de continuer la boucle, on a trouvé le coupable
            break; 
        }
    }
}

void ProjectWindow::refreshList()
{
    // Bloquer les mises à jour visuelles pour éviter le scintillement
    chronicleListWidget->setUpdatesEnabled(false);
    chronicleListWidget->clear();
    
    // --- LIGNE MANQUANTE AJOUTÉE ICI ---
    QDir dir(m_projectPath);
    QStringList diskFiles = dir.entryList({"*.txt"}, QDir::Files);
    diskFiles.removeAll("playlist.txt"); 
    // -----------------------------------

    QStringList playlistNames = ProjectManager::loadPlaylist(m_projectPath);

    // Helper pour créer l'item
    auto createItem = [&](const QString &baseName) {
        QListWidgetItem *item = new QListWidgetItem(chronicleListWidget);
        ChronicleWidget *w = new ChronicleWidget(baseName, m_projectPath, nullptr);
        item->setSizeHint(w->sizeHint());
        chronicleListWidget->setItemWidget(item, w);
        connectChronicleWidget(w);
    };

    // BOUCLE PRINCIPALE
    for (const QString &wantedName : playlistNames) {
        QString expectedFilename = wantedName + ".txt";
        
        int foundIndex = -1;
        for (int i = 0; i < diskFiles.size(); ++i) {
            if (diskFiles[i].compare(expectedFilename, Qt::CaseInsensitive) == 0) {
                foundIndex = i;
                break;
            }
        }

        if (foundIndex != -1) {
            QString realFileName = diskFiles[foundIndex];
            QString baseName = QFileInfo(realFileName).completeBaseName();
            createItem(baseName);
            diskFiles.removeAt(foundIndex);
        }
    }

    // BOUCLE ORPHELINS
    for (const QString &orphanFile : diskFiles) {
        QString baseName = QFileInfo(orphanFile).completeBaseName();
        createItem(baseName);
    }
    
    chronicleListWidget->setUpdatesEnabled(true);
    saveOrder(); 
}

void ProjectWindow::addChronicle()
{
    QInputDialog dialog(this);
    
    dialog.setWindowTitle("Ajouter");
    dialog.setLabelText("Titre de la chronique :");
    dialog.setOkButtonText("Ajouter");
    dialog.setCancelButtonText("Annuler");
    
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.textValue();
        if (!name.isEmpty()) {
            QString safeName = name.replace(QRegularExpression("[^a-zA-Z0-9 _-]"), "");
            
            QString filePath = ProjectManager::getScriptPath(m_projectPath, safeName);
            QFile f(filePath);
            f.open(QIODevice::WriteOnly);
            f.close();
            
            refreshList();
            chronicleListWidget->scrollToBottom();
            saveOrder(); 
        }
    }
}

void ProjectWindow::importAudioFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Importer Audio", "", ProjectUtils::getFileDialogFilter());
    if (filePath.isEmpty()) return;
    
    QFileInfo fi(filePath);
    QString originalName = fi.completeBaseName();
    QString ext = fi.suffix();
    
    // CORRECTION : Plus de préfixe "001_", juste le nom nettoyé
    QString safeName = originalName.replace(QRegularExpression("[^a-zA-Z0-9 _-]"), "");
    
    // Chemin cible : DossierProjet/Nom.ext
    // QString destBase = m_projectPath + "/" + safeName;
    QString destBase = QDir(m_projectPath).filePath(safeName);
    
    // Copie de l'audio
    QFile::copy(filePath, destBase + "." + ext);
    
    // Création du script vide associé
    QFile f(ProjectManager::getScriptPath(m_projectPath, safeName));
    f.open(QIODevice::WriteOnly);
    f.close();
    
    refreshList();
    chronicleListWidget->scrollToBottom();
    
    // Ajout explicite à la playlist à la fin
    saveOrder();
}

void ProjectWindow::onOpenScript(ChronicleWidget *w)
{
    ScriptEditor dialog(w->getScriptPath(), this);
    dialog.exec();
    w->refreshStatus();
}

void ProjectWindow::onRecord(ChronicleWidget *w)
{
    if (player) {
        player->stop();
        player->setSource(QUrl()); // Libère le verrou sur le fichier et la carte son
    }
    AudioRecorder rec(w->getScriptPath(), this);
    QString destFile = m_projectPath + "/" + w->getBaseName() + ".wav";
    rec.setOutputFile(destFile);
    
    if (rec.exec() == QDialog::Accepted) {
        w->refreshStatus();
        if (QFile::exists(destFile)) {
             // Stop lecture preview si en cours
             if (player) player->stop();
             AudioEditor editor(destFile, m_projectPath, this, false); 
             editor.exec();
        }
    }
}

void ProjectWindow::onPlay(ChronicleWidget *w)
{
    // Initialisation Lazy (si le player n'existe pas encore)
    if (!player) {
        player = new QMediaPlayer(this);
        audioOutput = new QAudioOutput(this);
        player->setAudioOutput(audioOutput);
        
        // 1. Cas où la lecture arrive au bout toute seule (Fin du fichier)
        connect(player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status){
            if (status == QMediaPlayer::EndOfMedia) {
                // On remet tout le monde à l'état STOP
                for(int i=0; i < chronicleListWidget->count(); i++) {
                    QListWidgetItem *item = chronicleListWidget->item(i);
                    if (ChronicleWidget *cw = qobject_cast<ChronicleWidget*>(chronicleListWidget->itemWidget(item))) {
                        cw->setPlaybackState(false);
                    }
                }
            }
        });

        // 2. Cas où l'utilisateur clique sur STOP (Changement d'état)
        connect(player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state){
            if (state == QMediaPlayer::StoppedState) {
                // On remet tout le monde à l'état STOP
                for(int i=0; i < chronicleListWidget->count(); i++) {
                    QListWidgetItem *item = chronicleListWidget->item(i);
                    if (ChronicleWidget *cw = qobject_cast<ChronicleWidget*>(chronicleListWidget->itemWidget(item))) {
                        cw->setPlaybackState(false);
                    }
                }
            }
        });
    } 

    // 1. On arrête tout le monde visuellement (avant de lancer le nouveau son)
    for(int i=0; i < chronicleListWidget->count(); i++) {
        QListWidgetItem *item = chronicleListWidget->item(i);
        if (ChronicleWidget *cw = qobject_cast<ChronicleWidget*>(chronicleListWidget->itemWidget(item))) {
            cw->setPlaybackState(false);
        }
    }

    // 2. On lance le player
    player->stop();
    // player->setSource(QUrl::fromLocalFile(w->getAudioPath()));
    player->setSource(QUrl::fromLocalFile(w->getEffectiveAudioPath()));
    player->play();

    // 3. On met l'icône STOP uniquement sur celui qui joue
    w->setPlaybackState(true);    
}

void ProjectWindow::onStop(ChronicleWidget *)
{
    if (player) {
        player->stop();
        // L'arrêt du player va déclencher le signal mediaStatusChanged connecté plus haut
        // qui remettra l'icône à jour via setPlaybackState(false)
    }
}

void ProjectWindow::onEditAudio(ChronicleWidget *w)
{
    if (player) {
        player->stop();
        player->setSource(QUrl()); 
    }
    
    AudioEditor editor(w->getAudioPath(), m_projectPath, this);
    editor.exec();
    w->refreshStatus();
}

void ProjectWindow::onRename(ChronicleWidget *w)
{
    QString oldName = w->getName();
 
    QInputDialog dialog(this); // La ligne manquante est là !
    dialog.setWindowTitle("Renommer");
    dialog.setLabelText("Nouveau titre :");
    dialog.setTextValue(oldName);
    dialog.setOkButtonText("Renommer");
    dialog.setCancelButtonText("Annuler");

    if (dialog.exec() != QDialog::Accepted) return;
    QString newName = dialog.textValue();
    
    if (newName.isEmpty() || newName == oldName) return;

    QString safeName = newName.replace(QRegularExpression("[^a-zA-Z0-9 _-]"), "").trimmed();
    if (safeName.length() > 100) safeName = safeName.left(100).trimmed();
    if (safeName.isEmpty()) return;

    QString oldBase = QDir(m_projectPath).filePath(oldName); 
    QString newBase = QDir(m_projectPath).filePath(safeName);
    
    // Vérification existence via ProjectManager pour le script
    if (QFile::exists(ProjectManager::getScriptPath(m_projectPath, safeName))) {
        QMessageBox::warning(this, "Erreur", "Un fichier porte déjà ce nom.");
        return;
    }

    if (QFile::exists(newBase + ".txt")) {
        QMessageBox::warning(this, "Erreur", "Un fichier porte déjà ce nom.");
        return;
    }

    // 1. Renommage Script
    if (!QFile::rename(oldBase + ".txt", newBase + ".txt")) {
        QMessageBox::critical(this, "Erreur", "Impossible de renommer le fichier script.");
        return;
    }

    // 2. Renommage Audio
    const QStringList extensions = ProjectUtils::getSupportedExtensions();
    for(const QString &ext : extensions) {
        if (QFile::exists(oldBase + ext)) {
            QFile::rename(oldBase + ext, newBase + ext);
        }
    }    

    // 3. Mise à jour Playlist via Manager
    ProjectManager::renameInPlaylist(m_projectPath, oldName, safeName);
    
    // --- SUPPRIMEZ TOUT LE RESTE DE LA FONCTION ---
    // Supprimez tout le bloc "int index = -1;" jusqu'à "f.close();"
    // Car renameInPlaylist fait déjà tout ça !
    // -----------------------------------------------
    
    refreshList();
}

void ProjectWindow::onDelete(ChronicleWidget *w)
{
    if (!w) return;

    // --- CORRECTION : Utilisation de addButton au lieu de setButtonText ---
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Supprimer");
    msgBox.setText("Supprimer " + w->getName() + " ?");
    msgBox.setIcon(QMessageBox::Question);
    
    // On ajoute les boutons manuellement
    QPushButton *btnOui = msgBox.addButton("Oui", QMessageBox::YesRole);
    QPushButton *btnNon = msgBox.addButton("Non", QMessageBox::NoRole);
    
    msgBox.setDefaultButton(btnNon);
    
    msgBox.exec();

    // On vérifie quel bouton a été cliqué
    if (msgBox.clickedButton() == btnOui) {
        
        // --- ÉTAPE CRITIQUE 1 : LIBÉRER LE FICHIER ---
        if (player) {
            player->stop();
            player->setSource(QUrl()); 
        }
        
        // --- ÉTAPE 2 : PRÉPARATION ---
        QString name = w->getName();
        QString txtPath = ProjectManager::getScriptPath(m_projectPath, name);
        
        // --- ÉTAPE 3 : SUPPRESSION DES AUDIOS ---
        QStringList audios = getAssociatedAudioFiles(name);
        
        for(const QString &f : audios) {
            if (!QFile::remove(f)) {
                QFile debugFile(f);
                qWarning() << "Echec suppression audio :" << f << "Erreur :" << debugFile.errorString();
            }
        }
        
        // --- ÉTAPE 4 : SUPPRESSION DU SCRIPT ---
        if (!QFile::remove(txtPath)) {
             qWarning() << "Echec suppression script :" << txtPath;
        }

        // --- ÉTAPE 5 : NETTOYAGE VISUEL ---
        for(int i = 0; i < chronicleListWidget->count(); i++) {
            QListWidgetItem *item = chronicleListWidget->item(i);
            if (chronicleListWidget->itemWidget(item) == w) {
                delete chronicleListWidget->takeItem(i);
                break; 
            }
        }
    
        saveOrder();
    }
}

void ProjectWindow::onMoveUp(ChronicleWidget *){ }
void ProjectWindow::onMoveDown(ChronicleWidget *){ }

void ProjectWindow::mergeProject()
{
    QStringList inputs;
    for(int i=0; i<chronicleListWidget->count(); i++) {
        QListWidgetItem *item = chronicleListWidget->item(i);
        ChronicleWidget *w = qobject_cast<ChronicleWidget*>(chronicleListWidget->itemWidget(item));
        if (QFile::exists(w->getEffectiveAudioPath())) {
            inputs << w->getEffectiveAudioPath(); //w->getAudioPath();
        }
    }
    
    if (inputs.isEmpty()) {
        QMessageBox::warning(this, "Export", "Aucun audio à fusionner.");
        return;
    }
    
    // QString musicDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    // QString dest = musicDir + "/PodcastCreator_" + m_projectName + ".mp3";
    QString exportDir = SettingsDialog::getExportPath();
    QString format = SettingsDialog::getOutputFormat(); // "mp3", "wav", etc.
    
    // Construction du nom final
    // Ex: C:/Music/PodcastCreator_MonProjet.mp3
    QString dest = QString("%1/PodcastCreator_%2.%3")
                   .arg(exportDir)
                   .arg(m_projectName)
                   .arg(format);
                   
    // S'assurer que le dossier d'export existe
    QDir dir(exportDir);
    if (!dir.exists()) dir.mkpath(".");    
    if (merger == nullptr) {
        merger = new AudioMerger(this);
        connect(merger, &AudioMerger::finished, this, &ProjectWindow::onMergeFinished);
        // Optionnel : si vous voulez afficher les erreurs ou le statut
        // connect(merger, &AudioMerger::error, this, [](const QString &msg){ qDebug() << msg; });
    }

    // 1. Création de la fenêtre de progression
    if (m_progressDialog) delete m_progressDialog; // Sécurité
    m_progressDialog = new QProgressDialog("Préparation de l'export...", QString(), 0, 0, this);
    m_progressDialog->setWindowTitle("Export en cours");
    m_progressDialog->setWindowModality(Qt::WindowModal); // Bloque la fenêtre principale
    m_progressDialog->setCancelButton(nullptr); // On cache le bouton Annuler pour simplifier (pas d'annulation FFMpeg gérée ici)
    m_progressDialog->setMinimumDuration(0); // S'affiche immédiatement
    
    // 2. Connexion pour voir l'avancement (FFmpeg time)
    // AudioMerger émet statusMessage, on le connecte au label du dialog
    connect(merger, &AudioMerger::statusMessage, m_progressDialog, &QProgressDialog::setLabelText);
    
    m_progressDialog->show();

    merger->mergeFiles(inputs, dest);
    // QMessageBox::information(this, "Export", "Export lancé vers " + dest);
}

// void ProjectWindow::onMergeFinished(bool success) {
//     if(success) QMessageBox::information(this, "Succès", "Export terminé !");
//     else QMessageBox::critical(this, "Erreur", "Echec de l'export ffmpeg");
// }

void ProjectWindow::onMergeFinished(bool success) {
    // --- DEBUT MODIFICATION ---
    
    // 1. Fermer la fenêtre de progression
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    // 2. Afficher le résultat
    if(success) {
        // On récupère le chemin théorique pour l'afficher (recréé ici pour l'affichage)
        QString exportDir = SettingsDialog::getExportPath();
        QString format = SettingsDialog::getOutputFormat();
        QString dest = QString("%1/PodcastCreator_%2.%3")
                       .arg(exportDir)
                       .arg(m_projectName)
                       .arg(format);

        QMessageBox::information(this, "Succès", 
            QString("Export terminé avec succès !\n\nFichier créé :\n%1").arg(dest));
    }
    else {
        QMessageBox::critical(this, "Erreur", "Echec de l'export ffmpeg.");
    }
    // --- FIN MODIFICATION ---
}

void ProjectWindow::saveOrder()
{
    QStringList items;
    for(int i = 0; i < chronicleListWidget->count(); i++) {
        QListWidgetItem *item = chronicleListWidget->item(i);
        if (ChronicleWidget *w = qobject_cast<ChronicleWidget*>(chronicleListWidget->itemWidget(item))) {
            items << w->getName();
        }
    }
    ProjectManager::savePlaylist(m_projectPath, items);
}

void ProjectWindow::addJingle()
{
    // 1. Ouvrir la fenêtre de sélection multiple
    JingleSelectorDialog dlg(this);
    
    // Si l'utilisateur annule, on sort
    if (dlg.exec() != QDialog::Accepted) return;

    // 2. Récupérer la liste des fichiers choisis
    QStringList selectedPaths = dlg.getSelectedJingles();

    if (selectedPaths.isEmpty()) return;

    // 3. Boucle d'insertion pour chaque jingle sélectionné
    for (const QString &fullPath : selectedPaths) {
        QFileInfo fi(fullPath);
        
        // Nom de base souhaité
        QString baseName = fi.completeBaseName(); 
        
        // --- Gestion des doublons de noms ---
        QString finalName = baseName;
        QString txtPath = ProjectManager::getScriptPath(m_projectPath, finalName);
        int counter = 1;
        while(QFile::exists(txtPath)) {
             finalName = baseName + QString("_%1").arg(counter++);
             txtPath = ProjectManager::getScriptPath(m_projectPath, finalName);
        }
        

        // --- Création du lien ---
        QFile f(txtPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out << "LINK:" << fullPath; 
            f.close();
        }

        // --- Ajout à la playlist (à la fin) ---
        ProjectManager::appendToPlaylist(m_projectPath, finalName);
    }
    // 4. Rafraîchissement global
    refreshList();
    chronicleListWidget->scrollToBottom();
    // saveOrder() n'est pas nécessaire ici car on a déjà écrit dans le fichier playlist manuellement ci-dessus
}
