				  
							
				  

#include "homedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include <QApplication>
#include <QRegularExpression>
#include <QFrame>
#include <QResizeEvent> 
#include <QFontMetrics> 
									
#include "jinglemanager.h"
#include "settingsdialog.h"

// --- WIDGET PROJET ---
class ProjectItemWidget : public QWidget {
    Q_OBJECT
public:
    QPushButton *btnDelete;
    QLabel *lblName;
    QString fullText;

    ProjectItemWidget(const QString &name, const QDateTime &date, QWidget *parent = nullptr) : QWidget(parent), fullText(name) {
		
        // 1. Layout principal du widget (contient la marge externe)
        QVBoxLayout *outerLayout = new QVBoxLayout(this);
        outerLayout->setContentsMargins(5, 5, 5, 5); // Espace entre les cartes
        outerLayout->setSpacing(0);

        // 2. La Carte (Fond blanc, coins arrondis)
        QFrame *card = new QFrame(this);
        card->setStyleSheet(
            "QFrame { "
            "   background-color: #FFFFFF; "
            "   border-radius: 8px; "
            "   border: 1px solid #E0E0E0; "
            "}"
            // Rend les enfants transparents pour éviter les fonds gris parasites
            "QLabel { background-color: transparent; border: none; }" 
        );

        // 3. Layout interne de la carte
        QHBoxLayout *innerLayout = new QHBoxLayout(card);
        innerLayout->setContentsMargins(15, 15, 15, 15);
        innerLayout->setSpacing(10);

        // Textes
        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(4);
        
										  
        lblName = new QLabel(name, card);
        lblName->setStyleSheet("font-size: 18px; font-weight: bold; color: #212121;");
		lblName->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred); 
																			  
        
        QLabel *lblDate = new QLabel("Modifié : " + date.toString("dd/MM/yyyy HH:mm"), card);
        lblDate->setStyleSheet("font-size: 12px; color: #757575;");
        
        textLayout->addWidget(lblName);
        textLayout->addWidget(lblDate);
        
        innerLayout->addLayout(textLayout, 1);
        
        // Bouton Poubelle
        btnDelete = new QPushButton(card);
        btnDelete->setIcon(QIcon(":/icones/ic_delete.png"));
        btnDelete->setIconSize(QSize(24, 24));
        btnDelete->setFixedSize(40, 40);
        btnDelete->setCursor(Qt::PointingHandCursor);
        btnDelete->setStyleSheet(
            "QPushButton { background-color: transparent; border: none; border-radius: 20px; }"
            "QPushButton:hover { background-color: #FFEBEE; }"
        );
        btnDelete->setToolTip("Supprimer cette émission");
        
        innerLayout->addWidget(btnDelete);

        // Assemblage
        outerLayout->addWidget(card);
    }
    
protected:
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        if (lblName) {
            // Calcul des "..." si le texte est trop long pour la largeur actuelle du label
            QString elidedText = lblName->fontMetrics().elidedText(fullText, Qt::ElideRight, lblName->width());
            lblName->setText(elidedText);
        }
    }
};

HomeDialog::HomeDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(QString("Podcast Createur - v%1").arg(APP_VERSION));
    resize(550, 700);
    setStyleSheet("QDialog { background-color: #F5F5F5; }");
    
    // --- MODIFICATION ICI ---
    // On récupère le chemin depuis les paramètres
    QString globalPath = SettingsDialog::getGlobalPath();
    rootPath = globalPath + "/Emissions";
    
    // On s'assure que le dossier existe
    QDir dir(globalPath);
    if (!dir.exists()) dir.mkpath(".");           // Crée la racine si absente
    if (!dir.exists("Emissions")) dir.mkdir("Emissions"); // Crée le sous-dossier
    // ------------------------

    setupUi();
    refreshProjectList();
}

void HomeDialog::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);

    // --- EN-TÊTE HORIZONTAL ---
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10); // Espace entre le bouton Jingle et Paramètres

    // 1. Titre
    QLabel *title = new QLabel("Mes Émissions", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #212121;");
    headerLayout->addWidget(title, 0, Qt::AlignVCenter);

    // 2. Espace extensible (Pousse tout vers la droite)
    headerLayout->addStretch();

    // 3. Bouton Bibliothèque Jingles
    QPushButton *btnLib = new QPushButton("BIBLIOTHÈQUE JINGLES", this);
    btnLib->setCursor(Qt::PointingHandCursor);
    btnLib->setFixedHeight(40);
    btnLib->setStyleSheet(
        "QPushButton { "
        "  background-color: #7B1FA2; color: white; "
        "  font-size: 13px; font-weight: bold; "
        "  padding: 0 15px; border-radius: 4px; border: none;"
        "}"
        "QPushButton:hover { background-color: #8E24AA; }"
    );
    connect(btnLib, &QPushButton::clicked, this, [this](){
        JingleManager manager(this);
        manager.exec();
    });
    headerLayout->addWidget(btnLib, 0, Qt::AlignVCenter);

    // 4. Bouton Paramètres (Roue dentée) - NOUVEAU
    QPushButton *btnSettings = new QPushButton("⚙", this); 
    
    btnSettings->setFixedSize(40, 40); // Bouton carré
    btnSettings->setCursor(Qt::PointingHandCursor);
    btnSettings->setToolTip("Paramètres");
    
    // On utilise le style pour le faire ressembler à une icône :
    // - font-size: 22px (pour la taille de la roue)
    // - color: #424242 (gris foncé)
    btnSettings->setStyleSheet(
        "QPushButton { "
        "  background-color: #E0E0E0; "
        "  border: none; border-radius: 4px;"
        "  font-size: 24px; color: #424242;" /* Taille et couleur du symbole */
        "  padding-bottom: 2px;"             /* Petit ajustement visuel vertical */
        "}"
        "QPushButton:hover { background-color: #D6D6D6; }"
    );
    
    // Action : Ouvrir une fenêtre de paramètres (Placeholder)
    connect(btnSettings, &QPushButton::clicked, this, [this](){
        SettingsDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            
            // On recalcule le chemin attendu
            QString newGlobal = SettingsDialog::getGlobalPath();
            QString newRoot = newGlobal + "/Emissions";
            
            if (newRoot != rootPath) {
                 rootPath = newRoot;
                 
                 // Création de l'arborescence si le dossier a changé
                 QDir dir(newGlobal);
                 if (!dir.exists()) dir.mkpath(".");
                 if (!dir.exists("Emissions")) dir.mkdir("Emissions");
                 
                 refreshProjectList();
            }
        }
    });

    headerLayout->addWidget(btnSettings, 0, Qt::AlignVCenter);
    // Ajout de l'en-tête au layout principal
    layout->addLayout(headerLayout);


    // --- RESTE DE L'INTERFACE (Inchangé) ---
    
    QPushButton *btnNew = new QPushButton("+ NOUVELLE ÉMISSION", this);
    btnNew->setCursor(Qt::PointingHandCursor);
    btnNew->setFixedHeight(50);
    btnNew->setStyleSheet(
        "QPushButton { "
        "  background-color: #2196F3; color: white; "
        "  font-size: 14px; font-weight: bold; "
        "  border-radius: 4px; border: none;"
        "}"
        "QPushButton:hover { background-color: #1976D2; }"
    );
    connect(btnNew, &QPushButton::clicked, this, &HomeDialog::createNewProject);
    layout->addWidget(btnNew);

    projectList = new QListWidget(this);
    projectList->setSpacing(0); 
    projectList->setStyleSheet(
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item { background: transparent; padding: 0px; border: none; }"
        "QListWidget::item:selected { background: transparent; }" 
        "QListWidget::item:hover { background: transparent; }"
    );
    projectList->setFocusPolicy(Qt::NoFocus);
    connect(projectList, &QListWidget::itemClicked, this, &HomeDialog::openSelectedProject);
    layout->addWidget(projectList);
}

void HomeDialog::refreshProjectList()
{
    projectList->setUpdatesEnabled(false);
    projectList->clear();
    QDir dir(rootPath);
    QStringList folders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    
    for (const QString &folder : folders) {
        QFileInfo fi(dir.filePath(folder));
        
        QListWidgetItem *item = new QListWidgetItem(projectList);
        ProjectItemWidget *widget = new ProjectItemWidget(folder, fi.lastModified());
        
        // Ajustement hauteur : contenu + marges externes
        item->setSizeHint(QSize(widget->sizeHint().width(), 95));
        item->setData(Qt::UserRole, folder);
        
        // Connexion suppression
        connect(widget->btnDelete, &QPushButton::clicked, this, [this, folder]() {
            this->deleteSpecificProject(folder);
        });
        
        projectList->setItemWidget(item, widget);
    }
    projectList->setUpdatesEnabled(true);
}

void HomeDialog::createNewProject()
{
    QInputDialog dialog(this);
    dialog.setWindowTitle("Nouvelle Émission");
    dialog.setLabelText("Nom de l'émission :");
    dialog.setTextValue("Mon Emission");
    dialog.setOkButtonText("Créer");       // Au lieu de OK
    dialog.setCancelButtonText("Annuler"); // Au lieu de Cancel
    
    if (dialog.exec() == QDialog::Accepted) {
        QString text = dialog.textValue();
        if (!text.isEmpty()) {
            QString safeName = text.replace(QRegularExpression("[^a-zA-Z0-9 _-]"), "");
            QDir dir(rootPath);
            if (dir.mkdir(safeName)) {
                refreshProjectList();
            } else {
                QMessageBox::warning(this, "Erreur", "Impossible de créer le dossier.");
            }
        }
    }
}

void HomeDialog::openSelectedProject(QListWidgetItem *item)
{
    if (!item) return;
    m_selectedName = item->data(Qt::UserRole).toString();
    m_selectedPath = rootPath + "/" + m_selectedName;
    accept();
}

void HomeDialog::deleteSpecificProject(const QString &name)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Supprimer ?");
    msgBox.setText("Supprimer l'émission <b>" + name + "</b> ?");
    msgBox.setInformativeText("Cette action est définitive.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStyleSheet("background-color: #FFFFFF; color: #333;");

    QPushButton *btnOui = msgBox.addButton("Oui, supprimer", QMessageBox::YesRole);
    QPushButton *btnNon = msgBox.addButton("Annuler", QMessageBox::RejectRole);
    
    msgBox.setDefaultButton(btnNon);

    msgBox.exec();

    if (msgBox.clickedButton() == btnOui) {
        // 1. Suppression physique (Fichiers)
        QDir dir(rootPath + "/" + name);
        if (dir.exists()) {
            if (!dir.removeRecursively()) {
                QMessageBox::warning(this, "Erreur", "Impossible de supprimer certains fichiers. Vérifiez qu'ils ne sont pas ouverts ailleurs.");
                return;
            }
        }

        // 2. Suppression visuelle (Optimisation : on ne recharge pas tout)
        // On cherche l'item qui contient ce nom dans ses données
        for(int i = 0; i < projectList->count(); i++) {
            QListWidgetItem *item = projectList->item(i);
            if (item->data(Qt::UserRole).toString() == name) {
                // takeItem retire l'item de la liste (sans le supprimer)
                // delete supprime l'item ET le widget associé (ProjectItemWidget)
                delete projectList->takeItem(i);
                break; 
            }
        }
    }
}

#include "homedialog.moc"