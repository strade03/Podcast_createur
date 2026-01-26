#ifndef DRAGGABLELISTWIDGET_H
#define DRAGGABLELISTWIDGET_H

#include <QListWidget>
#include <QLabel>
#include <QTimer>

class DraggableListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit DraggableListWidget(QWidget *parent = nullptr);

signals:
    void orderChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void doAutoScroll();

private:
    void startDragState();
    void updateDragPosition(const QPoint &pos);
    void finishDragState();
    
    // État
    bool m_isDragging;
    QPoint m_dragStartPos;
    int m_dragRowOffset;

    // L'item qu'on a cliqué à l'origine
    QListWidgetItem *m_originItem;
    // Le widget (contenu) qu'on a cliqué
    QWidget *m_originWidget;

    // L'item vide qui sert de prévisualisation (le "trou")
    QListWidgetItem *m_dummyItem;
    
    // L'image flottante
    QLabel *m_floatingWidget;
    
    // Scroll
    QTimer *m_scrollTimer;
    int m_scrollSpeed;
};

#endif // DRAGGABLELISTWIDGET_H