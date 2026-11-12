#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QStringList>

#include <functional>

enum class TaskStatus {
    Queued = 0,
    Running,
    Done,
    Failed,
    Canceled,
    Skipped
};

struct ConversionTask
{
    QString id;
    QString inputPath;
    QString outputPath;
    QString kind;      // video / audio / image / doc
    QString format;    // target extension, no dot
    TaskStatus status = TaskStatus::Queued;
    double progress = 0.0;      // 0..100, negative = indeterminate
    QString speed;              // "1.8x"
    qint64 etaMs = -1;          // remaining milliseconds, -1 unknown
    QString error;              // human readable failure reason
    QString thumbnail;          // absolute file path, empty when unavailable
    qint64 durationMs = 0;
    qint64 inputSize = 0;
    qint64 outputSize = 0;
    QString detail;             // "1920×1080 · H.264" style summary line
    QString logTail;            // captured ffmpeg stderr tail
    bool probed = false;
};

class TaskModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int visibleTotal READ visibleTotal NOTIFY countsChanged)
    Q_PROPERTY(int visibleQueued READ visibleQueued NOTIFY countsChanged)
    Q_PROPERTY(int visibleRunning READ visibleRunning NOTIFY countsChanged)
    Q_PROPERTY(int visibleDone READ visibleDone NOTIFY countsChanged)
    Q_PROPERTY(int visibleFailed READ visibleFailed NOTIFY countsChanged)
    Q_PROPERTY(int visibleProgress READ visibleProgress NOTIFY countsChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        InputPathRole,
        OutputPathRole,
        FileNameRole,
        KindRole,
        FormatRole,
        StatusRole,
        ProgressRole,
        SpeedRole,
        EtaRole,
        ErrorRole,
        ThumbnailRole,
        DurationRole,
        InputSizeRole,
        OutputSizeRole,
        DetailRole,
        StatusTextRole,
        LogRole
    };
    Q_ENUM(Roles)

    explicit TaskModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString kindFilter() const { return m_filter; }
    void setKindFilter(const QString &kind);

    int countByStatus(TaskStatus status) const;
    int countAll() const { return m_tasks.size(); }
    int visibleCount() const;
    int visibleCountByStatus(TaskStatus status) const;
    int visibleProgress() const;
    bool hasStatus(TaskStatus status) const;

    int visibleTotal() const { return visibleCount(); }
    int visibleQueued() const { return visibleCountByStatus(TaskStatus::Queued); }
    int visibleRunning() const { return visibleCountByStatus(TaskStatus::Running); }
    int visibleDone() const { return visibleCountByStatus(TaskStatus::Done); }
    int visibleFailed() const { return visibleCountByStatus(TaskStatus::Failed); }

    ConversionTask *find(const QString &id);
    const ConversionTask *find(const QString &id) const;
    QString rowId(int row) const;

    void appendTask(const ConversionTask &task);
    void removeTask(const QString &id);
    void clearAll();
    QStringList ids() const;
    QList<ConversionTask> tasks() const { return m_tasks; }

    /// Mutates the task and emits dataChanged with the supplied roles
    /// (all roles when `roles` is empty).
    void updateTask(const QString &id,
                    const std::function<void(ConversionTask &)> &mutator,
                    const QList<int> &roles = {});

signals:
    void countsChanged();
    void taskChanged(const QString &id);

private:
    int visibleRow(const QString &id) const;
    bool visible(const ConversionTask &task) const;

    QList<ConversionTask> m_tasks;
    QString m_filter = QStringLiteral("all");
};
