#pragma once

#include <QHash>
#include <QObject>
#include <QProcess>
#include <QSet>
#include <QSettings>
#include <QStringList>
#include <QVariantList>

#include "taskmodel.h"

class ConverterBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(TaskModel *tasks READ tasks CONSTANT)
    Q_PROPERTY(QVariantList formatCatalog READ formatCatalog CONSTANT)

    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(int quality READ quality WRITE setQuality NOTIFY qualityChanged)
    Q_PROPERTY(int concurrency READ concurrency WRITE setConcurrency NOTIFY concurrencyChanged)
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(int overwritePolicy READ overwritePolicy WRITE setOverwritePolicy NOTIFY overwritePolicyChanged)
    Q_PROPERTY(QString kindFilter READ kindFilter WRITE setKindFilter NOTIFY kindFilterChanged)
    Q_PROPERTY(QString currentFormat READ currentFormat WRITE setCurrentFormat NOTIFY currentFormatChanged)

    Q_PROPERTY(int totalCount READ totalCount NOTIFY countsChanged)
    Q_PROPERTY(int queuedCount READ queuedCount NOTIFY countsChanged)
    Q_PROPERTY(int runningCount READ runningCount NOTIFY countsChanged)
    Q_PROPERTY(int doneCount READ doneCount NOTIFY countsChanged)
    Q_PROPERTY(int failedCount READ failedCount NOTIFY countsChanged)
    Q_PROPERTY(int overallProgress READ overallProgress NOTIFY countsChanged)
    Q_PROPERTY(bool hasQueued READ hasQueued NOTIFY countsChanged)
    Q_PROPERTY(bool queueActive READ queueActive NOTIFY queueActiveChanged)

    Q_PROPERTY(bool ffmpegReady READ ffmpegReady NOTIFY ffmpegChanged)
    Q_PROPERTY(QString ffmpegVersion READ ffmpegVersion NOTIFY ffmpegChanged)
    Q_PROPERTY(QString ffmpegPath READ ffmpegPath NOTIFY ffmpegChanged)
    Q_PROPERTY(QString ffmpegDir READ ffmpegDir WRITE setFfmpegDir NOTIFY ffmpegChanged)

public:
    explicit ConverterBackend(QObject *parent = nullptr);
    ~ConverterBackend() override;

    TaskModel *tasks() const { return m_model; }
    QVariantList formatCatalog() const;

    QString outputDir() const;
    void setOutputDir(const QString &dir);

    int quality() const { return m_quality; }
    void setQuality(int quality);

    int concurrency() const { return m_concurrency; }
    void setConcurrency(int value);

    QString themeMode() const { return m_themeMode; }
    void setThemeMode(const QString &mode);

    int overwritePolicy() const { return m_overwritePolicy; }
    void setOverwritePolicy(int policy);

    QString kindFilter() const { return m_kindFilter; }
    void setKindFilter(const QString &kind);

    QString currentFormat() const;
    void setCurrentFormat(const QString &format);
    QString currentKind() const;

    int totalCount() const;
    int queuedCount() const;
    int runningCount() const;
    int doneCount() const;
    int failedCount() const;
    int overallProgress() const;
    bool hasQueued() const;
    bool queueActive() const { return m_queueActive; }

    bool ffmpegReady() const { return !m_ffmpeg.isEmpty(); }
    QString ffmpegVersion() const { return m_ffmpegVersion; }
    QString ffmpegPath() const { return m_ffmpeg; }
    QString ffmpegDir() const { return m_ffmpegDir; }
    void setFfmpegDir(const QString &dir);

    Q_INVOKABLE QVariantList formatsFor(const QString &kind) const;
    Q_INVOKABLE QString defaultFormat(const QString &kind) const;
    Q_INVOKABLE void setDefaultFormat(const QString &kind, const QString &format);
    Q_INVOKABLE QString nameFilter(const QString &kind) const;

    Q_INVOKABLE void addFiles(const QVariantList &urls);
    Q_INVOKABLE void addPaths(const QStringList &paths);
    Q_INVOKABLE void removeTask(const QString &id);
    Q_INVOKABLE void removeFinished();
    Q_INVOKABLE void removeFinishedKind(const QString &kind);
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE void clearKind(const QString &kind);

    Q_INVOKABLE void startAll();
    Q_INVOKABLE void startTask(const QString &id);
    Q_INVOKABLE void toggleQueue();
    Q_INVOKABLE void pauseQueue();
    Q_INVOKABLE void resumeQueue();
    Q_INVOKABLE void stopAll();
    Q_INVOKABLE void cancelTask(const QString &id);
    Q_INVOKABLE void retryTask(const QString &id);

    Q_INVOKABLE void setTaskFormat(const QString &id, const QString &format);
    Q_INVOKABLE void applyFormatToKind(const QString &kind, const QString &format);

    Q_INVOKABLE void revealOutput(const QString &id);
    Q_INVOKABLE void openOutputDir();
    Q_INVOKABLE void chooseOutputDir(const QVariant &urlOrPath);
    Q_INVOKABLE int countKind(const QString &kind) const;
    Q_INVOKABLE void detectFfmpeg();
    Q_INVOKABLE void seedDemoData();

signals:
    void toast(const QString &text, const QString &level);
    void outputDirChanged();
    void qualityChanged();
    void concurrencyChanged();
    void themeModeChanged();
    void overwritePolicyChanged();
    void kindFilterChanged();
    void currentFormatChanged();
    void countsChanged();
    void queueActiveChanged();
    void ffmpegChanged();

private:
    void schedule();
    void startTaskInternal(const QString &id);
    bool hasQueuedFor(const QString &kind) const;
    static QString kindLabel(const QString &kind);
    void removeFinishedInternal(const QString &kind);
    void clearInternal(const QString &kind);
    void finishTask(const QString &id, int exitCode, QProcess::ExitStatus status);
    void handleProgressChunk(const QString &id);
    void handleLogChunk(const QString &id);
    void probeTask(const QString &id);
    void makeThumbnail(const QString &id);
    QStringList buildArguments(const ConversionTask &task, const QString &outPath, QString *error) const;
    QString resolveOutputPath(const ConversionTask &task) const;
    QString thumbDir() const;
    QString defaultOutputDir() const;
    void readSettings();
    void persist(const QString &key, const QVariant &value);
    static QString kindForFile(const QString &path);
    static bool isMediaExtension(const QString &suffix);
    static QString formatDuration(qint64 ms);
    void failTask(const QString &id, const QString &reason);

    TaskModel *m_model = nullptr;
    QSettings m_settings;

    QString m_outputDir;
    int m_quality = 1;            // 0 = high, 1 = balanced, 2 = small
    int m_concurrency = 2;
    QString m_themeMode = QStringLiteral("auto");
    int m_overwritePolicy = 0;    // 0 = rename, 1 = overwrite, 2 = skip
    QString m_kindFilter = QStringLiteral("video");
    QString m_activeKind;   // category the running queue is working on
    bool m_queueActive = false;

    QHash<QString, QString> m_defaultFormats;
    QHash<QString, QProcess *> m_running;
    QHash<QString, QString> m_stdoutBuffers;
    QHash<QString, QString> m_stderrTails;
    QHash<QString, qint64> m_progressUs;
    QSet<QString> m_canceled;

    QString m_ffmpeg;
    QString m_ffprobe;
    QString m_ffmpegVersion;
    QString m_ffmpegDir;
};
