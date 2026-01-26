#ifndef WAVEFORMLOADER_H
#define WAVEFORMLOADER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QSharedPointer>

class WaveformLoader : public QObject
{
    Q_OBJECT
public:
    explicit WaveformLoader(const QString &filePath, QObject *parent = nullptr);

public slots:
    void process(); // La méthode qui sera exécutée dans le thread

signals:
    void progress(int percent);
    // void finished(const QVector<qint16> &samples, qint64 totalSamples);
    void error(const QString &msg);
    void finished(QSharedPointer<QVector<qint16>> samples, qint64 totalSamples);

private:
    QString m_filePath;
};

#endif // WAVEFORMLOADER_H