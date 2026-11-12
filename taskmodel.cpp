#include "taskmodel.h"

#include <QFileInfo>

TaskModel::TaskModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return visibleCount();
}

bool TaskModel::visible(const ConversionTask &task) const
{
    return m_filter.isEmpty() || m_filter == QLatin1String("all") || task.kind == m_filter;
}

int TaskModel::visibleCount() const
{
    int n = 0;
    for (const auto &t : m_tasks) {
        if (visible(t))
            ++n;
    }
    return n;
}

int TaskModel::visibleRow(const QString &id) const
{
    int row = 0;
    for (const auto &t : m_tasks) {
        if (!visible(t))
            continue;
        if (t.id == id)
            return row;
        ++row;
    }
    return -1;
}

void TaskModel::setKindFilter(const QString &kind)
{
    if (m_filter == kind)
        return;
    beginResetModel();
    m_filter = kind;
    endResetModel();
    emit countsChanged();
}

static QString statusText(TaskStatus status)
{
    switch (status) {
    case TaskStatus::Queued:   return QStringLiteral("等待中");
    case TaskStatus::Running:  return QStringLiteral("转换中");
    case TaskStatus::Done:     return QStringLiteral("已完成");
    case TaskStatus::Failed:   return QStringLiteral("失败");
    case TaskStatus::Canceled: return QStringLiteral("已取消");
    case TaskStatus::Skipped:  return QStringLiteral("已跳过");
    }
    return QString();
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    int row = 0;
    const ConversionTask *task = nullptr;
    for (const auto &t : m_tasks) {
        if (!visible(t))
            continue;
        if (row == index.row()) {
            task = &t;
            break;
        }
        ++row;
    }
    if (!task)
        return {};

    switch (role) {
    case IdRole:         return task->id;
    case InputPathRole:  return task->inputPath;
    case OutputPathRole: return task->outputPath;
    case FileNameRole:   return QFileInfo(task->inputPath).fileName();
    case KindRole:       return task->kind;
    case FormatRole:     return task->format;
    case StatusRole:     return static_cast<int>(task->status);
    case ProgressRole:   return task->progress;
    case SpeedRole:      return task->speed;
    case EtaRole:        return task->etaMs;
    case ErrorRole:      return task->error;
    case ThumbnailRole:  return task->thumbnail;
    case DurationRole:   return task->durationMs;
    case InputSizeRole:  return task->inputSize;
    case OutputSizeRole: return task->outputSize;
    case DetailRole:     return task->detail;
    case StatusTextRole: return statusText(task->status);
    case LogRole:        return task->logTail;
    default:             return {};
    }
}

QHash<int, QByteArray> TaskModel::roleNames() const
{
    return {
        { IdRole, "id" },
        { InputPathRole, "inputPath" },
        { OutputPathRole, "outputPath" },
        { FileNameRole, "fileName" },
        { KindRole, "kind" },
        { FormatRole, "format" },
        { StatusRole, "status" },
        { ProgressRole, "progress" },
        { SpeedRole, "speed" },
        { EtaRole, "eta" },
        { ErrorRole, "errorText" },
        { ThumbnailRole, "thumbnail" },
        { DurationRole, "durationMs" },
        { InputSizeRole, "inputSize" },
        { OutputSizeRole, "outputSize" },
        { DetailRole, "detail" },
        { StatusTextRole, "statusText" },
        { LogRole, "logTail" },
    };
}

int TaskModel::countByStatus(TaskStatus status) const
{
    int n = 0;
    for (const auto &t : m_tasks) {
        if (t.status == status)
            ++n;
    }
    return n;
}

int TaskModel::visibleCountByStatus(TaskStatus status) const
{
    int n = 0;
    for (const auto &t : m_tasks) {
        if (visible(t) && t.status == status)
            ++n;
    }
    return n;
}

int TaskModel::visibleProgress() const
{
    int total = 0;
    double sum = 0.0;
    for (const auto &t : m_tasks) {
        if (!visible(t))
            continue;
        ++total;
        if (t.status == TaskStatus::Done || t.status == TaskStatus::Failed
            || t.status == TaskStatus::Canceled || t.status == TaskStatus::Skipped) {
            sum += 100.0;
        } else {
            sum += qBound(0.0, t.progress, 100.0);
        }
    }
    if (total == 0)
        return 0;
    return int(sum / total);
}

bool TaskModel::hasStatus(TaskStatus status) const
{
    return countByStatus(status) > 0;
}

ConversionTask *TaskModel::find(const QString &id)
{
    for (auto &t : m_tasks) {
        if (t.id == id)
            return &t;
    }
    return nullptr;
}

const ConversionTask *TaskModel::find(const QString &id) const
{
    for (const auto &t : m_tasks) {
        if (t.id == id)
            return &t;
    }
    return nullptr;
}

QString TaskModel::rowId(int row) const
{
    if (row < 0 || row >= visibleCount())
        return QString();
    int i = 0;
    for (const auto &t : m_tasks) {
        if (!visible(t))
            continue;
        if (i == row)
            return t.id;
        ++i;
    }
    return QString();
}

void TaskModel::appendTask(const ConversionTask &task)
{
    const int row = visibleCount();
    const bool isVisible = visible(task);
    if (isVisible)
        beginInsertRows(QModelIndex(), row, row);
    m_tasks.append(task);
    if (isVisible)
        endInsertRows();
    emit countsChanged();
}

void TaskModel::removeTask(const QString &id)
{
    const int row = visibleRow(id);
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks.at(i).id == id) {
            if (row >= 0) {
                beginRemoveRows(QModelIndex(), row, row);
                m_tasks.removeAt(i);
                endRemoveRows();
            } else {
                m_tasks.removeAt(i);
            }
            emit countsChanged();
            return;
        }
    }
}

void TaskModel::clearAll()
{
    beginResetModel();
    m_tasks.clear();
    endResetModel();
    emit countsChanged();
}

QStringList TaskModel::ids() const
{
    QStringList list;
    list.reserve(m_tasks.size());
    for (const auto &t : m_tasks)
        list << t.id;
    return list;
}

void TaskModel::updateTask(const QString &id,
                           const std::function<void(ConversionTask &)> &mutator,
                           const QList<int> &roles)
{
    ConversionTask *task = find(id);
    if (!task)
        return;

    const int row = visibleRow(id);
    mutator(*task);

    QList<int> changed = roles;
    if (changed.isEmpty()) {
        changed = { IdRole, InputPathRole, OutputPathRole, FileNameRole, KindRole, FormatRole,
                    StatusRole, ProgressRole, SpeedRole, EtaRole, ErrorRole, ThumbnailRole,
                    DurationRole, InputSizeRole, OutputSizeRole, DetailRole, StatusTextRole,
                    LogRole };
    }

    if (row >= 0) {
        const QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx, changed);
    }
    emit taskChanged(id);
    emit countsChanged();
}
