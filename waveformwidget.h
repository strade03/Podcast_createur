#pragma once

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QScrollBar>
#include <QSharedPointer> // ✅ AJOUTER

class WaveformWidget : public QWidget {
    Q_OBJECT
public:
    explicit WaveformWidget(QWidget *parent = nullptr);

    void setColors(const QColor &backgroundColor, const QColor &penColor, const QColor &penTextColor);

    // ✅ MODIFIER : Accepte un pointeur partagé
    void setFullWaveform(QSharedPointer<QVector<qint16>> waveformPtr);

    void resetSelection(const qint64 startIndex);
    qint64 getSelectionStart() const;
    qint64 getSelectionEnd() const;
    bool hasSelection() const;
    void setPlayheadPosition(qint64 sampleIndex);
    qint64 getPlayheadPosition() const;
    void setStartAndEnd(qint64 pt1, qint64 pt2);
    void setisLoaded(bool v);
    void zoomIn();
    void zoomOut();
    void resetZoom();
    double getSamplesPerPixel() const { return samplesPerPixel; }
    int getScrollOffset() const { return offsetPixels; }
    int sampleToPixel(qint64 sample) const;
    void scrollToPixel(int x);
    void restoreZoomState(double spp, int offset);
    void setLoading(bool loading);
    void clearData();

signals:
    void selectionChanged(qint64 start, qint64 end);
    void playbackFinished();
    void zoomChanged(const QString &zoomFactor);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
private slots:
    void handleScrollChanged(int value);

private:
    void recalcCache();
    void updateScrollBar();

    // ✅ MODIFIER : Pointeur partagé au lieu de copie
    QSharedPointer<QVector<qint16>> fullWaveform;
    
    QVector<float> displayWaveform;
    bool cacheValid;

    qint64 totalSamples;
    qint64 playheadSample;
    qint64 selectionStartSample;
    qint64 selectionEndSample;
    qint64 fixedSelectionEdgeSample;

    bool isSelecting;
    bool isDragging;
    bool isLoaded;
    int pressStartX;
    double samplesPerPixel;
    int offsetPixels;

    QColor pen;
    QColor background;
    QColor penText;

    QVector<QLine> m_lines;
    QScrollBar *scrollBar;
    bool isLoading;
};