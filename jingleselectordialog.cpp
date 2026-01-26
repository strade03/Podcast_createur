#include "jingleselectordialog.h"
#include "jinglemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QDir>
#include <QLabel>
#include <QCheckBox>
#include <QFileInfo>
#include <QFrame>
#include "jinglewidget.h" 
#include "projectutils.h"

// ============================================================================
// IMPLÉMENTATION DIALOGUE
// ============================================================================

JingleSelectorDialog::JingleSelectorDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Ajouter des Jingles");
    resize(550, 550); 
    // setStyleSheet("QDialog { background-color: #F5F5F5; }"); 

    player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);
    audioOutput->setVolume(1.0);

    connect(player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::EndOfMedia) {
            currentPlayingPath.clear();
            updateWidgetsStyle(); 
        }
    });

    setupUi();
    loadJingles();
}

JingleSelectorDialog::~JingleSelectorDialog()
{
    player->stop();
}

void JingleSelectorDialog::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(15, 15, 15, 15);

    // HEADER
    QHBoxLayout *headerLayout = new QHBoxLayout();
    lblInfo = new QLabel("Cliquez sur les jingles pour les sélectionner.", this);
    lblInfo->setStyleSheet("color: #555; font-size: 12px;");
    headerLayout->addWidget(lblInfo);
    headerLayout->addStretch(); 

    btnSelectAll = new QPushButton("Tout Sélectionner", this);
    btnSelectAll->setCursor(Qt::PointingHandCursor);
    btnSelectAll->setStyleSheet(
        "QPushButton { "
        "  background-color: #E0E0E0; color: #333; "
        "  padding: 5px 10px; border-radius: 4px; border: none; font-size: 11px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #D6D6D6; }"
    );
    connect(btnSelectAll, &QPushButton::clicked, this, &JingleSelectorDialog::selectAll);
    headerLayout->addWidget(btnSelectAll);
    layout->addLayout(headerLayout);

    // LISTE
    listWidget = new QListWidget(this);
    listWidget->setFrameShape(QFrame::NoFrame);
    listWidget->setStyleSheet("background: transparent;");
    listWidget->setSpacing(2);
    listWidget->setSelectionMode(QAbstractItemView::MultiSelection); 
    connect(listWidget, &QListWidget::itemSelectionChanged, this, &JingleSelectorDialog::updateWidgetsStyle);

    layout->addWidget(listWidget);

    // FOOTER
    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    
    QPushButton *btnAdd = buttonBox->addButton("Ajouter", QDialogButtonBox::AcceptRole);
    btnAdd->setCursor(Qt::PointingHandCursor);
    btnAdd->setMinimumHeight(36);
    btnAdd->setStyleSheet(
        "QPushButton { "
        "  background-color: #2196F3; color: white; "
        "  padding: 0 20px; border-radius: 4px; border: none; font-weight: bold; font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #1976D2; }"
    );

    QPushButton *btnCancel = buttonBox->addButton("Annuler", QDialogButtonBox::RejectRole);
    btnCancel->setCursor(Qt::PointingHandCursor);
    btnCancel->setMinimumHeight(36);
    btnCancel->setStyleSheet(
        "QPushButton { "
        "  background-color: transparent; color: #555; "
        "  padding: 0 10px; border: 1px solid #CCC; border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #EEE; }"
    );

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    layout->addWidget(buttonBox);
}

void JingleSelectorDialog::loadJingles()
{
    QDir libDir(JingleManager::getLibraryPath());
    QStringList files = libDir.entryList(ProjectUtils::getSupportedExtensions().replaceInStrings(".", "*."), QDir::Files);
    
    for(const QString &f : files) {
        QString fullPath = libDir.absoluteFilePath(f);
        
        QListWidgetItem *item = new QListWidgetItem(listWidget);
        JingleWidget *w = new JingleWidget(fullPath, JingleWidget::SelectorMode);
        
        item->setSizeHint(QSize(0, 42));
        item->setData(Qt::UserRole, fullPath);
        listWidget->setItemWidget(item, w);

        connect(w->playBtn(), &QPushButton::clicked, [this, fullPath, item]() {
            playPreview(fullPath);
        });
    }
}

void JingleSelectorDialog::playPreview(const QString &path)
{
    // Si on clique sur le même fichier : STOP
    if (currentPlayingPath == path) {
        player->stop();
        currentPlayingPath.clear();
    } 
    // Sinon : LECTURE
    else {
        player->stop();
        currentPlayingPath = path;
        player->setSource(QUrl::fromLocalFile(path));
        player->play();
    }
    
    // On met à jour les icônes immédiatement
    updateWidgetsStyle(); 
}

void JingleSelectorDialog::selectAll()
{
    listWidget->selectAll();
}

void JingleSelectorDialog::updateWidgetsStyle()
{
    for(int i = 0; i < listWidget->count(); i++) {
        QListWidgetItem *item = listWidget->item(i);
        JingleWidget *w = dynamic_cast<JingleWidget*>(listWidget->itemWidget(item));
        if (w) {
            w->setSelectedState(item->isSelected());
            
            // --- CORRECTION LOGIQUE ---
            // On se base uniquement sur le chemin pour l'affichage immédiat
            // On ne vérifie plus l'état "PlayingState" du player car il peut avoir un micro-retard
            bool isThisPlaying = (currentPlayingPath == w->getFilePath());
            // bool isThisPlaying = (currentPlayingPath == w->filePath);
            w->setPlayingState(isThisPlaying);
        }
    }
    
    int count = listWidget->selectedItems().count();
    if (count > 0) lblInfo->setText(QString("%1 jingle(s) sélectionné(s)").arg(count));
    else lblInfo->setText("Cliquez sur les jingles pour les sélectionner.");
}

QStringList JingleSelectorDialog::getSelectedJingles() const
{
    QStringList result;
    for(auto item : listWidget->selectedItems()) {
        result.append(item->data(Qt::UserRole).toString());
    }
    return result;
}