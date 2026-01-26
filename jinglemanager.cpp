#include "jinglemanager.h"
#include "audioeditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QStandardPaths>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QFrame>
#include <QFileInfo>
#include <QMenu>      
#include <QInputDialog> 
#include "settingsdialog.h"
#include "jinglewidget.h" 
#include "projectutils.h"

// ============================================================================
// IMPLÉMENTATION JINGLE MANAGER
// ============================================================================

JingleManager::JingleManager(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Bibliothèque de Jingles");
    resize(600, 700);
    
    // Style global de la fenêtre (sécurité)
    setStyleSheet(
        "QDialog { background-color: #F5F5F5; }"
        "QToolTip { color: #000000; background-color: #FFFFFF; border: 1px solid #888888; }"
    );

    // Init Audio
    player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);
    audioOutput->setVolume(1.0);

    connect(player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::EndOfMedia) {
            currentPlayingPath.clear();
            updatePlayIcons();
        }
    });

    setupUi();
    refreshList();
}

JingleManager::~JingleManager() {
    player->stop();
}

QString JingleManager::getLibraryPath()
{
    QString globalPath = SettingsDialog::getGlobalPath();
    QString libPath = globalPath + "/Library";
    
    QDir dir(libPath);
    if (!dir.exists()) dir.mkpath(".");
    
    return libPath;
}

void JingleManager::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);

    // ============================================================
    // HEADER
    // ============================================================
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    QVBoxLayout *titleLayout = new QVBoxLayout();
    QLabel *lblTitle = new QLabel("Bibliothèque de jingles", this);
    lblTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #212121;");
    QLabel *lblSub = new QLabel("Ces jingles sont partagés entre tous les projets.\nSi vous les supprimez ici, ils disparaîtront des projets.", this);
    lblSub->setStyleSheet("color: #757575; font-size: 12px;");
    titleLayout->addWidget(lblTitle);
    titleLayout->addWidget(lblSub);
    
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    // Bouton IMPORTER
    QPushButton *btnImport = new QPushButton("+ Importer un son", this);
    btnImport->setCursor(Qt::PointingHandCursor);
    btnImport->setFixedHeight(40);
    btnImport->setStyleSheet(
        "QPushButton { "
        "  background-color: #7B1FA2; color: white; " /* Violet */
        "  font-size: 13px; font-weight: bold; "
        "  padding: 0 20px; border-radius: 4px; border: none;"
        "}"
        "QPushButton:hover { background-color: #8E24AA; }"
    );
    connect(btnImport, &QPushButton::clicked, this, &JingleManager::importJingle);
    headerLayout->addWidget(btnImport);

    layout->addLayout(headerLayout);
    // ============================================================

    // LISTE
    listWidget = new QListWidget(this);
    listWidget->setFrameShape(QFrame::NoFrame);
    listWidget->setStyleSheet("background: transparent; outline: none;");
    listWidget->setSpacing(2);
    listWidget->setSelectionMode(QAbstractItemView::NoSelection); 
    listWidget->setFocusPolicy(Qt::NoFocus);

    layout->addWidget(listWidget);
}

void JingleManager::refreshList()
{
    listWidget->clear();
    QDir dir(getLibraryPath());
    QStringList files = dir.entryList(ProjectUtils::getSupportedExtensions().replaceInStrings(".", "*."), QDir::Files);
    
    for (const QString &fileName : files) {
        QString fullPath = dir.absoluteFilePath(fileName);
        
        QListWidgetItem *item = new QListWidgetItem(listWidget);
        JingleWidget *w = new JingleWidget(fullPath, JingleWidget::LibraryMode);
        
        item->setSizeHint(QSize(0, 42)); 
        listWidget->setItemWidget(item, w);

        // --- CONNEXIONS ---
        
        // 1. Lecture
        connect(w->playBtn(), &QPushButton::clicked, [this, fullPath]() {
            playPreview(fullPath);
        });

        // 2. Suppression (Bouton direct)
        connect(w->deleteBtn(), &QPushButton::clicked, [this, fullPath]() {
            deleteJingle(fullPath);
        });

        // 3. Menu (Éditer / Renommer)
        connect(w->menuBtn(), &QPushButton::clicked, [this, fullPath, w](){
            QMenu menu(w);
            menu.setStyleSheet(
                "QMenu { background-color: #FFF; border: 1px solid #CCC; }"
                "QMenu::item { padding: 6px 24px; color: #333; }"
                "QMenu::item:selected { background-color: #EEE; }"
            );

            // Action Éditer
            menu.addAction("Éditer l'audio", [this, fullPath](){
                editJingleAudio(fullPath);
            });
            
            // Action Renommer
            menu.addAction("Renommer", [this, fullPath](){
                renameJingle(fullPath);
            });
            
            menu.exec(QCursor::pos());
        });
    }
}

void JingleManager::playPreview(const QString &filePath)
{
    if (currentPlayingPath == filePath) {
        player->stop();
        currentPlayingPath.clear();
    } else {
        player->stop();
        currentPlayingPath = filePath;
        player->setSource(QUrl::fromLocalFile(filePath));
        player->play();
    }
    updatePlayIcons();
}

void JingleManager::updatePlayIcons()
{
    for(int i = 0; i < listWidget->count(); i++) {
        QListWidgetItem *item = listWidget->item(i);
        JingleWidget *w = dynamic_cast<JingleWidget*>(listWidget->itemWidget(item));
        if (w) {
            bool isPlaying = (currentPlayingPath == w->getFilePath());
            w->setPlayingState(isPlaying);
        }
    }
}

void JingleManager::importJingle()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Importer Audio", "", ProjectUtils::getFileDialogFilter());
    if (files.isEmpty()) return;

    QString libPath = getLibraryPath();
    for (const QString &file : files) {
        QFileInfo fi(file);
        QString dest = libPath + "/" + fi.fileName();
        if (QFile::exists(dest)) {
            QMessageBox::warning(this, "Existant", "Le fichier " + fi.fileName() + " existe déjà.");
            continue;
        }
        QFile::copy(file, dest);
    }
    refreshList();
}

void JingleManager::editJingleAudio(const QString &filePath)
{
    if (player) {
        player->stop();
        player->setSource(QUrl()); 
        currentPlayingPath.clear();
        updatePlayIcons();
    }

    AudioEditor editor(filePath, getLibraryPath(), this, false);
    editor.exec();
    
    // On recharge l'audio si nécessaire
}

void JingleManager::renameJingle(const QString &filePath)
{
    if (player) {
        player->stop();
        player->setSource(QUrl());
        currentPlayingPath.clear();
        updatePlayIcons();
    }

    QFileInfo fi(filePath);
    QString oldName = fi.completeBaseName();
    QString ext = fi.suffix();

    bool ok;
    QString newName = QInputDialog::getText(this, "Renommer le jingle",
                                            "Nouveau nom :", QLineEdit::Normal,
                                            oldName, &ok);
    
    if (ok && !newName.isEmpty() && newName != oldName) {
        // Nettoyage du nom pour éviter les caractères interdits
        QString safeName = newName; // On peut ajouter une regex ici si on veut forcer un format
        QString newPath = fi.absolutePath() + "/" + safeName + "." + ext;

        if (QFile::exists(newPath)) {
            QMessageBox::warning(this, "Erreur", "Un fichier avec ce nom existe déjà.");
            return;
        }

        if (QFile::rename(filePath, newPath)) {
            refreshList();
        } else {
            QMessageBox::critical(this, "Erreur", "Impossible de renommer le fichier.\nVérifiez qu'il n'est pas utilisé ailleurs.");
        }
    }
}

void JingleManager::deleteJingle(const QString &filePath)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Supprimer");
    msgBox.setText("Voulez-vous supprimer ce jingle de la bibliothèque ?\n(Il ne sera plus accessible dans les projets qui l'utilisent)");
    msgBox.setIcon(QMessageBox::Warning);
    
    QPushButton *btnOui = msgBox.addButton("Oui", QMessageBox::YesRole);
    QPushButton *btnNon = msgBox.addButton("Non", QMessageBox::NoRole);
    msgBox.setDefaultButton(btnNon);
    
    msgBox.exec();

    if (msgBox.clickedButton() == btnOui) {
        if (currentPlayingPath == filePath) {
            player->stop();
            player->setSource(QUrl());
            currentPlayingPath.clear();
        }

        if (QFile::remove(filePath)) {
            refreshList();
        } else {
            QMessageBox::warning(this, "Erreur", "Impossible de supprimer le fichier.\nIl est peut-être utilisé.");
        }
    }
}