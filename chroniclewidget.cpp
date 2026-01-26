				  
								 
				  

#include "chroniclewidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QDir>
#include <QFrame>
#include <QResizeEvent>

#include "projectutils.h"

ChronicleWidget::ChronicleWidget(const QString &name, 
                                 const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_name(name), m_projectPath(projectPath), isPlaying(false)
{
    setupUi();
    refreshStatus();
}

void ChronicleWidget::setupUi()
{
    // Layout externe (marge autour de la carte)
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(5, 0, 5, 0); 
    outerLayout->setSpacing(0);
    
    // LA CARTE BLANCHE
    QFrame *cardFrame = new QFrame(this);
    cardFrame->setObjectName("CardFrame");
    
    // CORRECTION DESIGN : Utilisation d'une bordure complète (border) au lieu de border-bottom
    // Cela corrige les artefacts dans les coins arrondis.
    cardFrame->setStyleSheet(
        "#CardFrame {"
        "   background-color: #FFFFFF;"
        "   border-radius: 15px;" 
        "   border: 1px solid #E0E0E0;" /* Bordure uniforme propre comme l'accueil */
        "}"
        "QLabel { background-color: transparent; border: none; }"
        "QPushButton { background-color: transparent; border: none; }"
    );
    
    // Layout INTERNE (horizontal)
    QHBoxLayout *mainLayout = new QHBoxLayout(cardFrame);
    // Marges internes ajustées pour l'équilibre
    mainLayout->setContentsMargins(15, 4, 10, 4); 
    mainLayout->setSpacing(10);

    // 1. POIGNÉE DRAG
    QLabel *dragHandle = new QLabel(this);
    dragHandle->setPixmap(QIcon(":/icones/ic_menu_reorder.png").pixmap(20, 20));
    dragHandle->setFixedSize(24, 24);
    dragHandle->setStyleSheet("opacity: 0.4;"); 
    mainLayout->addWidget(dragHandle);

    // 2. TEXTES (Titre + Statut)
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2); 
    infoLayout->setAlignment(Qt::AlignVCenter);
    
    lblName = new QLabel(m_name, this);
    // Police un poil plus grosse pour correspondre à l'accueil
    lblName->setStyleSheet("font-size: 16px; font-weight: bold; color: #212121;");
    
	// On autorise le label à être compressé si l'espace manque								  
	lblName->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);															  
																		 
	
    lblStatus = new QLabel("...", this);
    lblStatus->setStyleSheet("color: #888888; font-size: 12px;");
    
    infoLayout->addWidget(lblName);
    infoLayout->addWidget(lblStatus);
    
    mainLayout->addLayout(infoLayout, 1);

    // 3. BOUTONS ACTIONS
    
    // Script
    btnScript = new QPushButton(this);
    btnScript->setIcon(QIcon(":/icones/ic_script.png"));
    btnScript->setIconSize(QSize(22, 22));
    btnScript->setFixedSize(36, 36);
    btnScript->setCursor(Qt::PointingHandCursor);
    // btnScript->setToolTip("Éditer le script"); 
    // connect(btnScript, &QPushButton::clicked, this, &ChronicleWidget::openScriptRequested);
    
    connect(btnScript, &QPushButton::clicked, [this](){
        if (isJingleItem()) {
            emit duplicateRequested(); // Si Jingle -> Dupliquer
        } else {
            emit openScriptRequested(); // Si Chronique -> Ouvrir Script
        }
    });

    mainLayout->addWidget(btnScript);

    // Action (Play/Record)
    btnAction = new QPushButton(this);
    btnAction->setIconSize(QSize(32, 32)); 
    btnAction->setFixedSize(48, 48);
    btnAction->setCursor(Qt::PointingHandCursor);
    connect(btnAction, &QPushButton::clicked, [this](){
        if (isPlaying) {
            emit stopRequested(); // Si ça joue, on demande STOP
        } else {
            // AVANT (Bug) : if(QFile::exists(getAudioPath())) ...
            // MAINTENANT : On vérifie le chemin effectif (qui gère les liens Jingle)
            if(QFile::exists(getEffectiveAudioPath())) 
                emit playRequested(); // Si fichier (local ou lien), on demande PLAY
            else 
                emit recordRequested(); // Sinon, on demande RECORD
        }
    });
    mainLayout->addWidget(btnAction);
    // Menu 
    btnMenu = new QPushButton(this);
    btnMenu->setIcon(QIcon(":/icones/ic_more.png"));
    btnMenu->setIconSize(QSize(20, 20));
    btnMenu->setFixedSize(30, 40);
    btnMenu->setCursor(Qt::PointingHandCursor);
    
    connect(btnMenu, &QPushButton::clicked, [this](){
        QMenu menu(this);
        menu.setStyleSheet(
            "QMenu { background-color: #FFF; border: 1px solid #CCC; }"
            "QMenu::item { padding: 6px 24px; color: #333; }"
            "QMenu::item:selected { background-color: #EEE; }"
            "QMenu::item:disabled { color: #BBB; }" // Style pour l'item désactivé
        );

        // bool hasAudio = QFile::exists(getAudioPath());
        
        bool isJingle = isJingleItem();
        bool hasAudio = QFile::exists(getEffectiveAudioPath());

        if (!isJingle) {
            // Action Éditer : Désactivée si pas d'audio
             QAction *actEdit = menu.addAction("Éditer l'audio", this, &ChronicleWidget::editAudioRequested);
             actEdit->setEnabled(hasAudio);
        } else {
             // Pour les jingles, on peut juste proposer "Renommer" (l'entrée dans la liste) ou Supprimer
             // On n'offre pas l'édition car c'est en lecture seule dans le projet
        }
        
        menu.addAction("Renommer", this, &ChronicleWidget::renameRequested);
        menu.addSeparator();
        menu.addAction("Supprimer", this, &ChronicleWidget::deleteRequested);
        
        menu.exec(QCursor::pos());
    });
    
    mainLayout->addWidget(btnMenu);


    outerLayout->addWidget(cardFrame);
    
    // Hauteur maintenue à 72px comme demandé
    this->setFixedHeight(72); 
}

						   
// --- TRONCATURE ---
void ChronicleWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // On met à jour le texte avec des points de suspension (...) selon la largeur disponible
    if (lblName) {
        QString elidedText = lblName->fontMetrics().elidedText(m_name, Qt::ElideRight, lblName->width());
        lblName->setText(elidedText);
    }
}													  

void ChronicleWidget::refreshStatus()
{
    bool isJingle = isJingleItem();
    
    if (isJingle) {
        // Mode Jingle
        btnScript->setVisible(true); 
        btnScript->setIcon(QIcon(":/icones/duplicate.png"));
        btnScript->setToolTip("Dupliquer ce Jingle");

        lblName->setStyleSheet("font-size: 16px; font-weight: bold; color: #7B1FA2;"); // Titre violet
        lblStatus->setText("Jingle (Bibliothèque)");
        
        QString source = getEffectiveAudioPath();
        if (QFile::exists(source)) {
            btnAction->setIcon(QIcon(":/icones/ic_play.png"));
            btnAction->setToolTip("Écouter Jingle");
            btnAction->setEnabled(true);
        } else {
            lblStatus->setText("Jingle INTROUVABLE");
            lblStatus->setStyleSheet("color: red;");
            btnAction->setEnabled(false);
        }
    } else {
        btnScript->setVisible(true);
        btnScript->setIcon(QIcon(":/icones/ic_script.png")); // Icône script normale
        btnScript->setToolTip("Éditer le script");
        
        lblName->setStyleSheet("font-size: 16px; font-weight: bold; color: #212121;");
    
        QString txtPath = getScriptPath();
        QString audioPath = getAudioPath();
        
        bool hasScript = QFile::exists(txtPath);
        bool hasAudio = QFile::exists(audioPath);
        
        QString status = QString("Script %1 • %2")
                .arg(hasScript ? "OK" : "vide")
                .arg(hasAudio ? "Audio OK" : "Pas d'audio");
        
        lblStatus->setText(status);
        
        if (hasAudio) {
            btnAction->setIcon(QIcon(":/icones/ic_play.png"));
            btnAction->setToolTip("Écouter");
        } else {
            btnAction->setIcon(QIcon(":/icones/record.png"));
            btnAction->setToolTip("Enregistrer");
        }
    }
}

QString ChronicleWidget::getScriptPath() const {
    // return m_projectPath + "/" + m_name + ".txt";
     return QDir(m_projectPath).filePath(m_name + ".txt");
}

QString ChronicleWidget::getAudioPath() const {
    QString base = m_projectPath + "/" + m_name; 
    QString existing = ProjectUtils::findExistingAudioFile(base);
    if (!existing.isEmpty()) {
        return existing;
    }
    return base + ".wav";
}


void ChronicleWidget::setPlaybackState(bool playing)
{
    isPlaying = playing;
    if (isPlaying) {
        btnAction->setIcon(QIcon(":/icones/stop.png")); // Affiche STOP
        btnAction->setToolTip("Arrêter la lecture");
    } else {
        refreshStatus(); // Remet l'icône Play ou Record selon le fichier
    }
}

bool ChronicleWidget::isJingleItem() const {
    QFile f(getScriptPath());
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(f.readAll());
        f.close();
        return content.startsWith("LINK:");
    }
    return false;
}

QString ChronicleWidget::getEffectiveAudioPath() const {
    if (isJingleItem()) {
        QFile f(getScriptPath());
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(f.readAll());
            // Format "LINK:/path/to/file.mp3"
            return content.mid(5).trimmed();
        }
    }
    return getAudioPath(); // Retourne le chemin local standard
}
