#include "draggablelistwidget.h"
#include "chroniclewidget.h"
#include <QMouseEvent>
#include <QScrollBar>
#include <QApplication>
#include <QDebug>
// #include <QMessageBox>

DraggableListWidget::DraggableListWidget(QWidget *parent)
    : QListWidget(parent)
    , m_isDragging(false)
    , m_originItem(nullptr)
    , m_originWidget(nullptr)
    , m_dummyItem(nullptr)
    , m_floatingWidget(nullptr)
    , m_scrollSpeed(0)
{
    setDragEnabled(false);
    setAcceptDrops(false);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    
    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(20);
    connect(m_scrollTimer, &QTimer::timeout, this, &DraggableListWidget::doAutoScroll);
}

void DraggableListWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
        m_originItem = itemAt(event->pos());
        
        if (m_originItem) {
            m_originWidget = qobject_cast<QWidget*>(itemWidget(m_originItem));
            if (m_originWidget) {
                m_dragRowOffset = event->pos().y() - visualItemRect(m_originItem).top();
            }
        }
    }
    QListWidget::mousePressEvent(event);
}

void DraggableListWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        if (m_isDragging) finishDragState();
        return;
    }

    if (!m_originItem || !m_originWidget) return;

    if (!m_isDragging && (event->pos() - m_dragStartPos).manhattanLength() < 10) {
        return;
    }

    if (!m_isDragging) {
        startDragState();
    }

    updateDragPosition(event->pos());
}

void DraggableListWidget::startDragState()
{
    m_isDragging = true;

    // Image fantôme
    QPixmap pixmap(m_originWidget->size());
    pixmap.fill(Qt::transparent);
    m_originWidget->render(&pixmap);
    
    m_floatingWidget = new QLabel(this);
    m_floatingWidget->setPixmap(pixmap);
    m_floatingWidget->resize(m_originWidget->size());
    m_floatingWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_floatingWidget->show();
    m_floatingWidget->setStyleSheet("border: 2px solid #2196F3; border-radius: 4px; opacity: 0.8;");
    m_floatingWidget->raise();

    // On cache l'original (on garde votre logique qui fonctionnait)
    int originalRow = row(m_originItem);
    m_originItem->setHidden(true);
    
    m_dummyItem = new QListWidgetItem();
    m_dummyItem->setSizeHint(m_originItem->sizeHint());
    m_dummyItem->setFlags(Qt::NoItemFlags);
    insertItem(originalRow, m_dummyItem);
}

// === C'EST ICI QUE SE TROUVE LA CORRECTION POUR LE DÉPLACEMENT ===
void DraggableListWidget::updateDragPosition(const QPoint &mousePos)
{
    if (!m_floatingWidget) return;

    // 1. Déplacement visuel de l'image
    int y = mousePos.y() - m_dragRowOffset;
    int contentH = viewport()->height();
    if (y < 0) y = 0;
    if (y > contentH - m_floatingWidget->height()) 
        y = contentH - m_floatingWidget->height();

    // On déplace l'image fantôme (avec un petit décalage X pour le style)
    m_floatingWidget->move(5, y);

    // 2. Calcul de la ligne cible
    int targetRow = -1;
    int centerY = m_floatingWidget->geometry().center().y();
    
    // On parcourt la liste pour trouver où s'insérer
    for (int i = 0; i < count(); i++) {
        QListWidgetItem *it = item(i);
        // On ignore le dummy lui-même et l'original caché pour le calcul géométrique
        if (it == m_dummyItem || it == m_originItem) continue;
        
        QRect r = visualItemRect(it);
        // Si la souris est au-dessus du centre de cet item, on prend sa place
        if (centerY < r.center().y()) {
            targetRow = i;
            break;
        }
    }
    
    // Si on n'a rien trouvé en dessous, c'est qu'on vise la toute FIN de la liste
    if (targetRow == -1) {
        targetRow = count();
    }

    // 3. Déplacement du Dummy (Le Trou)
    int currentDummyRow = row(m_dummyItem);
    
    // On ne bouge que si l'index cible est différent
    if (targetRow != currentDummyRow) {
        
        // On retire le dummy de sa position actuelle
        takeItem(currentDummyRow);
        
        // CORRECTION DU BUG "DERNIÈRE PLACE" :
        // Si on déplace vers le bas (target > current), le fait d'avoir retiré
        // le dummy décale tous les index suivants de -1.
        // Il faut donc réduire targetRow de 1 pour atterrir au bon endroit visuel.
        if (targetRow > currentDummyRow) {
            targetRow--;
        }
        
        // On réinsère le dummy
        insertItem(targetRow, m_dummyItem);
    }

    // 4. Auto-scroll
    int zone = 30;
    if (mousePos.y() < zone) m_scrollSpeed = -15;
    else if (mousePos.y() > contentH - zone) m_scrollSpeed = 15;
    else m_scrollSpeed = 0;
    
    if (m_scrollSpeed != 0 && !m_scrollTimer->isActive()) m_scrollTimer->start();
    if (m_scrollSpeed == 0) m_scrollTimer->stop();
}

void DraggableListWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        finishDragState();
        event->accept();
    } else {
        QListWidget::mouseReleaseEvent(event);
    }
}

// void DraggableListWidget::finishDragState()
// {
//     m_isDragging = false;
//     m_scrollTimer->stop();

//     // 1. Nettoyage visuel immédiat
//     if (m_floatingWidget) {
//         m_floatingWidget->deleteLater(); // Plus sûr que delete direct
//         m_floatingWidget = nullptr;
//     }

//     // Vérification de sécurité critique
//     if (m_dummyItem && m_originItem) {
//         setUpdatesEnabled(false);

//         QWidget* rawWidget = itemWidget(m_originItem);
//         ChronicleWidget* oldCW = qobject_cast<ChronicleWidget*>(rawWidget);
        
//         // SÉCURITÉ : Si le widget d'origine est perdu ou invalide, on annule tout
//         if (!oldCW) {
//              m_originItem->setHidden(false);
//              delete takeItem(row(m_dummyItem));
//              m_originItem = nullptr; 
//              m_dummyItem = nullptr;
//              m_originWidget = nullptr;
//              setUpdatesEnabled(true);
//              return;
//         }

//         QString name = oldCW->getName();
//         QString path = oldCW->getProjectPath();

//         // 2. Transformation du Dummy
//         m_dummyItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
        
//         ChronicleWidget* newCW = new ChronicleWidget(name, path);
//         m_dummyItem->setSizeHint(newCW->sizeHint());
//         setItemWidget(m_dummyItem, newCW);
        
//         // 3. Suppression de l'ancien
//         // Note: takeItem transfère la propriété, delete libère la mémoire
//         delete takeItem(row(m_originItem));

//         // 4. Finalisation
//         setCurrentItem(m_dummyItem);
        
//         // RAZ des pointeurs
//         m_originItem = nullptr;
//         m_dummyItem = nullptr;

//         setUpdatesEnabled(true);
//         emit orderChanged();
//     }
//     else {
//         // Annulation propre
//         if (m_originItem) m_originItem->setHidden(false);
//         if (m_dummyItem) delete takeItem(row(m_dummyItem));
//         m_originItem = nullptr;
//         m_dummyItem = nullptr;
//     }

//     m_originWidget = nullptr;
// }

void DraggableListWidget::finishDragState()
{
    m_isDragging = false;
    m_scrollTimer->stop();

    // 1. Nettoyage visuel immédiat de l'image fantôme
    if (m_floatingWidget) {
        m_floatingWidget->deleteLater();
        m_floatingWidget = nullptr;
    }

    // Vérification de sécurité
    if (m_dummyItem && m_originItem) {
        setUpdatesEnabled(false);

        QWidget* rawWidget = itemWidget(m_originItem);
        ChronicleWidget* oldCW = qobject_cast<ChronicleWidget*>(rawWidget);
        
        // SÉCURITÉ : Si le widget d'origine est perdu ou invalide
        if (!oldCW) {
             m_originItem->setHidden(false);
             delete takeItem(row(m_dummyItem));
             m_originItem = nullptr; 
             m_dummyItem = nullptr;
             m_originWidget = nullptr;
             setUpdatesEnabled(true);
             return;
        }

        // On récupère les infos importantes
        QString name = oldCW->getName();
        QSize itemSize = oldCW->sizeHint(); // On garde la taille pour que l'item ne s'écrase pas

        // 2. CONFIGURATION DU DUMMY (L'ITEM DE DESTINATION)
        m_dummyItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
        
        // --- CORRECTION MAJEURE ICI ---
        // On transfère le NOM dans les données de l'item (UserRole).
        // C'est ce que ProjectWindow va lire pour recréer le widget.
        m_dummyItem->setData(Qt::UserRole, name);
        
        // On définit la taille (hauteur) de l'item, sinon il va faire 0px de haut sans widget
        m_dummyItem->setSizeHint(itemSize);

        // ON NE CRÉE PAS LE WIDGET ICI !
        // On laisse setItemWidget(m_dummyItem, ...) vide.
        // Cela va permettre à ProjectWindow::onListOrderChanged de détecter
        // que cet item est "orphelin" et de le recréer proprement avec les connexions.
        
        // 3. Suppression de l'ancien item (celui qu'on a déplacé)
        delete takeItem(row(m_originItem));

        // 4. Finalisation
        setCurrentItem(m_dummyItem);
        
        // RAZ des pointeurs
        m_originItem = nullptr;
        m_dummyItem = nullptr;

        setUpdatesEnabled(true);
        
        // Ceci va déclencher ProjectWindow::onListOrderChanged
        emit orderChanged();
    }
    else {
        // Annulation propre
        if (m_originItem) m_originItem->setHidden(false);
        if (m_dummyItem) delete takeItem(row(m_dummyItem));
        m_originItem = nullptr;
        m_dummyItem = nullptr;
    }

    m_originWidget = nullptr;
}

void DraggableListWidget::doAutoScroll()
{
    if (m_scrollSpeed == 0) return;
    verticalScrollBar()->setValue(verticalScrollBar()->value() + m_scrollSpeed);
    QPoint localPos = mapFromGlobal(QCursor::pos());
    updateDragPosition(localPos);
}

void DraggableListWidget::leaveEvent(QEvent *event)
{
    QListWidget::leaveEvent(event);
    m_scrollTimer->stop();
}

// version ok sauf pour dernier