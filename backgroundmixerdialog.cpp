#include "backgroundmixerdialog.h"
#include "jinglemanager.h"
#include "projectutils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QTimer>

BackgroundMixerDialog::BackgroundMixerDialog(const QString &voiceFilePath, QWidget *parent)
    : QDialog(parent), m_voicePath(voiceFilePath), isPreviewing(false)
{
    setWindowTitle(tr("Ajouter un tapis sonore"));
    resize(450, 280);  // Un peu plus grand pour le nouveau slider

    // Setup Audio
    playerVoice = new QMediaPlayer(this);
    outVoice = new QAudioOutput(this);
    playerVoice->setAudioOutput(outVoice);
    outVoice->setVolume(1.0); // Voix toujours à 100%

    playerMusic = new QMediaPlayer(this);
    outMusic = new QAudioOutput(this);
    playerMusic->setAudioOutput(outMusic);
    
    // La musique doit boucler
    playerMusic->setLoops(QMediaPlayer::Infinite);

    // Synchro : Quand la voix s'arrête, la musique s'arrête
    connect(playerVoice, &QMediaPlayer::mediaStatusChanged, this, &BackgroundMixerDialog::onVoiceStatusChanged);

    setupUi();
    
    // Chargement initial
    playerVoice->setSource(QUrl::fromLocalFile(m_voicePath));
    if (comboJingles->count() > 0) {
        updateJingleSelection(0);
    }
}

BackgroundMixerDialog::~BackgroundMixerDialog() {
    stopPreview();
}

void BackgroundMixerDialog::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // 1. Choix du Jingle
    mainLayout->addWidget(new QLabel(tr("Choisir le jingle (Bibliothèque) :")));
    comboJingles = new QComboBox(this);
    
    QDir libDir(JingleManager::getLibraryPath());
    QStringList filters = ProjectUtils::getSupportedExtensions().replaceInStrings(".", "*.");
    QFileInfoList files = libDir.entryInfoList(filters, QDir::Files);
    
    for(const QFileInfo &fi : files) {
        comboJingles->addItem(fi.completeBaseName(), fi.absoluteFilePath());
    }
    connect(comboJingles, &QComboBox::currentIndexChanged, this, &BackgroundMixerDialog::updateJingleSelection);
    mainLayout->addWidget(comboJingles);

    // 2. Volume
    mainLayout->addWidget(new QLabel(tr("Volume du tapis sonore :")));
    QHBoxLayout *volLayout = new QHBoxLayout();
    
    sliderVolume = new QSlider(Qt::Horizontal, this);
    sliderVolume->setRange(0, 100);
    sliderVolume->setValue(20); // Défaut 20%
    
    lblVolumeValue = new QLabel("20 %", this);
    lblVolumeValue->setFixedWidth(50);
    lblVolumeValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    volLayout->addWidget(sliderVolume);
    volLayout->addWidget(lblVolumeValue);
    mainLayout->addLayout(volLayout);

    connect(sliderVolume, &QSlider::valueChanged, this, &BackgroundMixerDialog::onVolumeChanged);

    // ════════════════════════════════════════════════════════════════════════
    // 3. NOUVEAU : Décalage temporel
    // ════════════════════════════════════════════════════════════════════════
    mainLayout->addWidget(new QLabel(tr("Décalage du tapis (négatif = tapis en avance) :")));
    QHBoxLayout *offsetLayout = new QHBoxLayout();
    
    sliderOffset = new QSlider(Qt::Horizontal, this);
    sliderOffset->setRange(-100, 100);  // -10.0s à +10.0s (en dixièmes de seconde)
    sliderOffset->setValue(0);          // Défaut : pas de décalage
    sliderOffset->setTickPosition(QSlider::TicksBelow);
    sliderOffset->setTickInterval(10);  // Tick tous les 1 seconde
    
    lblOffsetValue = new QLabel("0.0 s", this);
    lblOffsetValue->setFixedWidth(50);
    lblOffsetValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    offsetLayout->addWidget(sliderOffset);
    offsetLayout->addWidget(lblOffsetValue);
    mainLayout->addLayout(offsetLayout);

    connect(sliderOffset, &QSlider::valueChanged, this, &BackgroundMixerDialog::onOffsetChanged);

    // ════════════════════════════════════════════════════════════════════════
    // 4. Option Ducking (atténuation automatique)
    // ════════════════════════════════════════════════════════════════════════
    QHBoxLayout *duckingLayout = new QHBoxLayout();
    
    chkDucking = new QCheckBox(tr("Atténuer le tapis quand je parle"), this);
    chkDucking->setChecked(false);
    duckingLayout->addWidget(chkDucking);
    duckingLayout->addStretch();
    
    mainLayout->addLayout(duckingLayout);
    
    // Label d'info (affiché sous le bouton prévisualiser quand l'option est cochée)
    lblDuckingInfo = new QLabel(this);
    lblDuckingInfo->setText(tr("⚠ L'atténuation auto n'est pas incluse dans la prévisualisation"));
    lblDuckingInfo->setStyleSheet("QLabel { color: #888888; font-style: italic; font-size: 11px; }");
    lblDuckingInfo->setVisible(false);
    
    connect(chkDucking, &QCheckBox::toggled, lblDuckingInfo, &QLabel::setVisible);

    // ════════════════════════════════════════════════════════════════════════
    // 5. Prévisualisation
    btnPreview = new QPushButton(tr("Prévisualiser"), this);
    btnPreview->setIcon(QIcon(":/icones/play.png"));
    btnPreview->setCheckable(true);
    connect(btnPreview, &QPushButton::clicked, this, &BackgroundMixerDialog::togglePreview);
    mainLayout->addWidget(btnPreview);
    mainLayout->addWidget(lblDuckingInfo);

    mainLayout->addStretch();

    // 5. Boutons Dialog
    QDialogButtonBox *btnBox = new QDialogButtonBox(this);
    btnBox->addButton(tr("Valider"), QDialogButtonBox::AcceptRole);
    btnBox->addButton(tr("Annuler"), QDialogButtonBox::RejectRole);

    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(btnBox);
    
    // Init valeurs
    onVolumeChanged(20);
    onOffsetChanged(0);
}

void BackgroundMixerDialog::updateJingleSelection(int index) {
    stopPreview();
    QString path = comboJingles->itemData(index).toString();
    if (!path.isEmpty()) {
        playerMusic->setSource(QUrl::fromLocalFile(path));
    }
}

void BackgroundMixerDialog::onVolumeChanged(int value) {
    lblVolumeValue->setText(QString::number(value) + " %");
    qreal vol = value / 100.0;
    outMusic->setVolume(vol);
}

// ════════════════════════════════════════════════════════════════════════
// NOUVEAU : Gestion du changement de décalage
// ════════════════════════════════════════════════════════════════════════
void BackgroundMixerDialog::onOffsetChanged(int value) {
    // value est en dixièmes de seconde (-100 à +100 → -10.0s à +10.0s)
    float seconds = value / 10.0f;
    lblOffsetValue->setText(QString("%1 s").arg(seconds, 0, 'f', 1));
}

void BackgroundMixerDialog::togglePreview() {
    if (isPreviewing) {
        stopPreview();
    } else {
        if (comboJingles->count() == 0) return;
        
        // Récupérer le décalage en millisecondes
        float offsetSec = getOffsetSeconds();
        qint64 offsetMs = static_cast<qint64>(offsetSec * 1000);
        
        // On rembobine
        playerVoice->setPosition(0);
        playerMusic->setPosition(0);
        
        // ════════════════════════════════════════════════════════════════════
        // NOUVEAU : Appliquer le décalage pour la prévisualisation
        // ════════════════════════════════════════════════════════════════════
        if (offsetMs > 0) {
            // Décalage positif : le tapis démarre EN RETARD
            // → On démarre la voix immédiatement, le tapis après un délai
            playerVoice->play();
            QTimer::singleShot(offsetMs, this, [this]() {
                if (isPreviewing) {
                    playerMusic->play();
                }
            });
        } else if (offsetMs < 0) {
            // Décalage négatif : le tapis démarre EN AVANCE
            // → On démarre le tapis immédiatement, la voix après un délai
            playerMusic->play();
            QTimer::singleShot(-offsetMs, this, [this]() {
                if (isPreviewing) {
                    playerVoice->play();
                }
            });
        } else {
            // Pas de décalage : les deux démarrent ensemble
            playerMusic->play();
            playerVoice->play();
        }
        
        btnPreview->setText(tr("Arrêter"));
        btnPreview->setIcon(QIcon(":/icones/stop.png"));
        isPreviewing = true;
    }
}

bool BackgroundMixerDialog::isDuckingEnabled() const {
    return chkDucking->isChecked();
}


void BackgroundMixerDialog::stopPreview() {
    playerVoice->stop();
    playerMusic->stop();
    btnPreview->setText(tr("Prévisualiser"));
    btnPreview->setIcon(QIcon(":/icones/play.png"));
    btnPreview->setChecked(false);
    isPreviewing = false;
}

void BackgroundMixerDialog::onVoiceStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {
        stopPreview();
    }
}

QString BackgroundMixerDialog::getSelectedJingle() const {
    return comboJingles->currentData().toString();
}

float BackgroundMixerDialog::getVolume() const {
    return sliderVolume->value() / 100.0f;
}

// ════════════════════════════════════════════════════════════════════════
// NOUVEAU : Récupérer le décalage en secondes
// ════════════════════════════════════════════════════════════════════════
float BackgroundMixerDialog::getOffsetSeconds() const {
    return sliderOffset->value() / 10.0f;  // Convertir dixièmes en secondes
}
