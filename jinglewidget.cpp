#include "jinglewidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileInfo>

JingleWidget::JingleWidget(const QString &path, Mode mode, QWidget *parent)
    : QWidget(parent), m_filePath(path), m_mode(mode)
{
    setupUi();
}

void JingleWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(m_mode == SelectorMode ? 2 : 4, 
                               m_mode == SelectorMode ? 0 : 2, 
                               m_mode == SelectorMode ? 2 : 4, 
                               m_mode == SelectorMode ? 0 : 2);

    m_bgFrame = new QFrame(this);
    m_bgFrame->setObjectName("CardFrame");
   
    // Style de base
    QString baseStyle = "#CardFrame { background-color: #FFFFFF; border-radius: 8px; border: 1px solid #E0E0E0; }";
    if (m_mode == SelectorMode) {
        baseStyle += "#CardFrame { border-radius: 4px; border: 1px solid #DDD; }";
    }
    
    baseStyle +="QToolTip {color: #000000; background-color: #FFFFFF; border: 1px solid #888888; }";

    m_bgFrame->setStyleSheet(baseStyle);

    QHBoxLayout *innerLayout = new QHBoxLayout(m_bgFrame);
    innerLayout->setContentsMargins(10, 0, 10, 0);
    innerLayout->setSpacing(10);

    // 1. Bouton Play (Commun)
    m_btnPlay = new QPushButton(m_bgFrame);
    m_btnPlay->setIcon(QIcon(":/icones/ic_play.png"));
    m_btnPlay->setIconSize(QSize(24, 24));
    m_btnPlay->setFixedSize(36, 36);
    m_btnPlay->setCursor(Qt::PointingHandCursor);
    m_btnPlay->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 18px; } QPushButton:hover { background-color: #E3F2FD; }");
    m_btnPlay->setToolTip("Lecture");
    
    if (m_mode == SelectorMode) { innerLayout->addWidget(m_btnPlay);} // Ajout lecture à gauche si mode selection

    // 2. Nom (Commun)
    m_lblName = new QLabel(QFileInfo(m_filePath).completeBaseName(), m_bgFrame);
    m_lblName->setStyleSheet("font-size: 14px; font-weight: bold; color: #212121;");
    innerLayout->addWidget(m_lblName, 1, Qt::AlignVCenter);

    // 3. Éléments spécifiques au mode Bibliothèque
    if (m_mode == LibraryMode) {
        innerLayout->addWidget(m_btnPlay);
        // Bouton Supprimer
        m_btnDelete = new QPushButton(m_bgFrame);
        m_btnDelete->setIcon(QIcon(":/icones/ic_delete.png"));
        m_btnDelete->setIconSize(QSize(24, 24));
        m_btnDelete->setFixedSize(36, 36);
        m_btnDelete->setCursor(Qt::PointingHandCursor);
        m_btnDelete->setToolTip("Supprimer");
        m_btnDelete->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 18px; } QPushButton:hover { background-color: #FFEBEE; }");
        innerLayout->addWidget(m_btnDelete);

        // Bouton Menu
        m_btnMenu = new QPushButton(m_bgFrame);
        m_btnMenu->setIcon(QIcon(":/icones/ic_more.png"));
        m_btnMenu->setIconSize(QSize(20, 20));
        m_btnMenu->setFixedSize(30, 36);
        m_btnMenu->setCursor(Qt::PointingHandCursor);
        m_btnMenu->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:hover { background-color: #EEEEEE; }");
        innerLayout->addWidget(m_btnMenu);
    } else {
        m_btnDelete = nullptr;
        m_btnMenu = nullptr;
    }

    layout->addWidget(m_bgFrame);
}

void JingleWidget::setPlayingState(bool isPlaying) {
    if (isPlaying) 
        m_btnPlay->setIcon(QIcon(":/icones/ic_stop_read.png")); 
    else 
        m_btnPlay->setIcon(QIcon(":/icones/ic_play.png"));
}

void JingleWidget::setSelectedState(bool selected) {
    if (m_mode != SelectorMode) return;
    
    if (selected) {
        m_bgFrame->setStyleSheet("#CardFrame { background-color: #C8E6C9; border: 1px solid #4CAF50; border-radius: 4px; }"); 
        m_lblName->setStyleSheet("font-size: 14px; font-weight: bold; color: #2E7D32;");
    } else {
        m_bgFrame->setStyleSheet("#CardFrame { background-color: #FFFFFF; border: 1px solid #DDD; border-radius: 4px; }"); 
        m_lblName->setStyleSheet("font-size: 14px; font-weight: 500; color: #333;");
    }
}
