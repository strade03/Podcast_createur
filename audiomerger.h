#ifndef AUDIOMERGER_H
#define AUDIOMERGER_H

#include <QObject>
#include <QProcess>
#include <QStringList>

class AudioMerger : public QObject {
    Q_OBJECT
public:
    AudioMerger(QObject* parent = nullptr);
    
    static void init();

    static QString getFFmpegPath();
    static bool checkFFmpeg(QWidget *parent = nullptr);
    static int getChannelCount(const QString &filePath); 
    
    void mergeFiles(const QStringList& inputFiles, const QString& outputFile);
    static void invalidateCache(const QString &filePath);

signals:
    void started();
    void finished(bool success);
    void error(const QString& message);
    void statusMessage(const QString& message); 

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error);

private:
    QProcess* ffmpegProcess;
    double totalDurationInSeconds;
    double getFileDuration(const QString &file);
    double getWavDuration(const QString &file);
    QString formatTime(double seconds);

    static QString m_cachedFFmpegPath;
    static bool m_isFFmpegAvailable;
    static bool m_isInitialized;
    static QMap<QString, int> m_channelCache; 
};

#endif // AUDIOMERGER_H