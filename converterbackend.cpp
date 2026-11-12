#include "converterbackend.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLinearGradient>
#include <QPainter>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>

#include <utility>

namespace {

bool isAudioSuffix(const QString &s)
{
    static const QSet<QString> set = {
        "mp3", "wav", "flac", "aac", "m4a", "ogg", "opus", "wma", "aiff", "aif", "amr"
    };
    return set.contains(s);
}

bool isVideoSuffix(const QString &s)
{
    static const QSet<QString> set = {
        "mp4", "mkv", "avi", "mov", "flv", "wmv", "webm", "m4v", "ts",
        "mpg", "mpeg", "3gp", "3g2", "ogv", "vob", "m2ts", "mts", "gif"
    };
    return set.contains(s);
}

bool isImageSuffix(const QString &s)
{
    static const QSet<QString> set = {
        "png", "jpg", "jpeg", "webp", "bmp", "tif", "tiff", "ico", "heic", "avif", "jfif"
    };
    return set.contains(s);
}

QVariantMap fmtEntry(const QString &ext, const QString &label, const QString &note)
{
    return { { "ext", ext }, { "label", label }, { "note", note } };
}

QString humanError(const QString &stderrText, int exitCode)
{
    const QStringList lines = stderrText.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (int i = lines.size() - 1; i >= 0; --i) {
        const QString line = lines.at(i).trimmed();
        if (line.isEmpty())
            continue;
        if (line.contains(QLatin1String("Invalid data found"), Qt::CaseInsensitive))
            return QStringLiteral("文件损坏或格式无法识别");
        if (line.contains(QLatin1String("No such file"), Qt::CaseInsensitive))
            return QStringLiteral("找不到输入文件");
        if (line.contains(QLatin1String("Permission denied"), Qt::CaseInsensitive))
            return QStringLiteral("没有写入权限，请更换输出目录");
        if (line.contains(QLatin1String("No space left"), Qt::CaseInsensitive))
            return QStringLiteral("磁盘空间不足");
        if (line.contains(QLatin1String("Unknown encoder"), Qt::CaseInsensitive))
            return QStringLiteral("当前 ffmpeg 缺少所需编码器");
        if (line.contains(QLatin1String("Invalid argument"), Qt::CaseInsensitive))
            return QStringLiteral("参数无效，目标格式可能不受支持");
        if (line.contains(QLatin1String("error"), Qt::CaseInsensitive))
            return line.left(160);
    }
    return QStringLiteral("转换失败（ffmpeg 退出码 %1）").arg(exitCode);
}

} // namespace

ConverterBackend::ConverterBackend(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("FormatStudio"), QStringLiteral("FormatStudio"))
    , m_model(new TaskModel(this))
{
    m_defaultFormats.insert(QStringLiteral("video"), QStringLiteral("mp4"));
    m_defaultFormats.insert(QStringLiteral("audio"), QStringLiteral("mp3"));
    m_defaultFormats.insert(QStringLiteral("image"), QStringLiteral("png"));

    readSettings();
    m_model->setKindFilter(m_kindFilter);
    detectFfmpeg();

    connect(m_model, &TaskModel::countsChanged, this, &ConverterBackend::countsChanged);
}

ConverterBackend::~ConverterBackend()
{
    const auto processes = m_running;
    for (QProcess *proc : processes) {
        proc->disconnect(this);
        proc->kill();
    }
}

void ConverterBackend::readSettings()
{
    m_outputDir = m_settings.value(QStringLiteral("output/dir")).toString();
    if (m_outputDir.isEmpty())
        m_outputDir = defaultOutputDir();

    m_quality = qBound(0, m_settings.value(QStringLiteral("quality"), 1).toInt(), 2);
    m_concurrency = qBound(1, m_settings.value(QStringLiteral("concurrency"), 2).toInt(), 4);
    m_themeMode = m_settings.value(QStringLiteral("theme"), QStringLiteral("auto")).toString();
    m_overwritePolicy = qBound(0, m_settings.value(QStringLiteral("overwrite"), 0).toInt(), 2);
    m_ffmpegDir = m_settings.value(QStringLiteral("ffmpeg/dir")).toString();

    const QStringList kinds = { QStringLiteral("video"), QStringLiteral("audio"), QStringLiteral("image") };
    for (const QString &kind : kinds) {
        const QString saved = m_settings.value(QStringLiteral("format/") + kind).toString();
        if (!saved.isEmpty())
            m_defaultFormats.insert(kind, saved);
    }
}

void ConverterBackend::persist(const QString &key, const QVariant &value)
{
    m_settings.setValue(key, value);
}

QString ConverterBackend::defaultOutputDir() const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (base.isEmpty())
        base = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return QDir(base).filePath(QStringLiteral("FormatStudio"));
}

QString ConverterBackend::outputDir() const
{
    return m_outputDir;
}

void ConverterBackend::setOutputDir(const QString &dir)
{
    if (dir.isEmpty() || dir == m_outputDir)
        return;
    m_outputDir = dir;
    persist(QStringLiteral("output/dir"), dir);
    emit outputDirChanged();
}

void ConverterBackend::setQuality(int quality)
{
    quality = qBound(0, quality, 2);
    if (quality == m_quality)
        return;
    m_quality = quality;
    persist(QStringLiteral("quality"), quality);
    emit qualityChanged();
}

void ConverterBackend::setConcurrency(int value)
{
    value = qBound(1, value, 4);
    if (value == m_concurrency)
        return;
    m_concurrency = value;
    persist(QStringLiteral("concurrency"), value);
    emit concurrencyChanged();
    schedule();
}

void ConverterBackend::setThemeMode(const QString &mode)
{
    if (mode == m_themeMode)
        return;
    m_themeMode = mode;
    persist(QStringLiteral("theme"), mode);
    emit themeModeChanged();
}

void ConverterBackend::setOverwritePolicy(int policy)
{
    policy = qBound(0, policy, 2);
    if (policy == m_overwritePolicy)
        return;
    m_overwritePolicy = policy;
    persist(QStringLiteral("overwrite"), policy);
    emit overwritePolicyChanged();
}

void ConverterBackend::setKindFilter(const QString &kind)
{
    const QString normalized = kind.isEmpty() ? QStringLiteral("all") : kind;
    if (normalized == m_kindFilter)
        return;
    m_kindFilter = normalized;
    m_model->setKindFilter(normalized);
    emit kindFilterChanged();
    emit currentFormatChanged();
}

int ConverterBackend::totalCount() const { return m_model->countAll(); }
int ConverterBackend::queuedCount() const { return m_model->countByStatus(TaskStatus::Queued); }
int ConverterBackend::runningCount() const { return m_model->countByStatus(TaskStatus::Running); }
int ConverterBackend::doneCount() const { return m_model->countByStatus(TaskStatus::Done); }
int ConverterBackend::failedCount() const { return m_model->countByStatus(TaskStatus::Failed); }
bool ConverterBackend::hasQueued() const { return queuedCount() > 0; }

int ConverterBackend::overallProgress() const
{
    const auto tasks = m_model->tasks();
    if (tasks.isEmpty())
        return 0;
    double sum = 0.0;
    for (const auto &t : tasks) {
        if (t.status == TaskStatus::Done || t.status == TaskStatus::Failed
            || t.status == TaskStatus::Canceled || t.status == TaskStatus::Skipped) {
            sum += 100.0;
        } else {
            sum += qBound(0.0, t.progress, 100.0);
        }
    }
    return int(sum / tasks.size());
}

QVariantList ConverterBackend::formatCatalog() const
{
    QVariantList list;
    const QStringList kinds = { QStringLiteral("video"), QStringLiteral("audio"), QStringLiteral("image") };
    for (const QString &kind : kinds) {
        QString label;
        if (kind == QLatin1String("video"))
            label = QStringLiteral("视频");
        else if (kind == QLatin1String("audio"))
            label = QStringLiteral("音频");
        else
            label = QStringLiteral("图片");
        list << QVariantMap{ { "kind", kind }, { "label", label }, { "formats", formatsFor(kind) } };
    }
    return list;
}

QVariantList ConverterBackend::formatsFor(const QString &kind) const
{
    QVariantList list;
    if (kind == QLatin1String("video")) {
        list << fmtEntry(QStringLiteral("mp4"), QStringLiteral("MP4"), QStringLiteral("H.264 · 通用兼容"))
             << fmtEntry(QStringLiteral("mkv"), QStringLiteral("MKV"), QStringLiteral("H.264 · 多音轨"))
             << fmtEntry(QStringLiteral("mov"), QStringLiteral("MOV"), QStringLiteral("H.264 · 苹果生态"))
             << fmtEntry(QStringLiteral("webm"), QStringLiteral("WebM"), QStringLiteral("VP9 · 网页分享"))
             << fmtEntry(QStringLiteral("avi"), QStringLiteral("AVI"), QStringLiteral("MPEG-4 · 老设备"))
             << fmtEntry(QStringLiteral("gif"), QStringLiteral("GIF"), QStringLiteral("动图 · 无声"))
             << fmtEntry(QStringLiteral("mp3"), QStringLiteral("MP3"), QStringLiteral("仅提取音频"))
             << fmtEntry(QStringLiteral("wav"), QStringLiteral("WAV"), QStringLiteral("仅提取音频 · 无损"));
    } else if (kind == QLatin1String("audio")) {
        list << fmtEntry(QStringLiteral("mp3"), QStringLiteral("MP3"), QStringLiteral("通用 · 有损"))
             << fmtEntry(QStringLiteral("m4a"), QStringLiteral("M4A"), QStringLiteral("AAC 编码"))
             << fmtEntry(QStringLiteral("wav"), QStringLiteral("WAV"), QStringLiteral("PCM 无损"))
             << fmtEntry(QStringLiteral("flac"), QStringLiteral("FLAC"), QStringLiteral("无损压缩"))
             << fmtEntry(QStringLiteral("ogg"), QStringLiteral("OGG"), QStringLiteral("Vorbis"))
             << fmtEntry(QStringLiteral("opus"), QStringLiteral("OPUS"), QStringLiteral("低码率首选"));
    } else if (kind == QLatin1String("image")) {
        list << fmtEntry(QStringLiteral("png"), QStringLiteral("PNG"), QStringLiteral("无损 · 支持透明"))
             << fmtEntry(QStringLiteral("jpg"), QStringLiteral("JPG"), QStringLiteral("有损 · 体积小"))
             << fmtEntry(QStringLiteral("webp"), QStringLiteral("WebP"), QStringLiteral("高压缩比"))
             << fmtEntry(QStringLiteral("bmp"), QStringLiteral("BMP"), QStringLiteral("未压缩位图"));
    }
    return list;
}

QString ConverterBackend::defaultFormat(const QString &kind) const
{
    return m_defaultFormats.value(kind, QStringLiteral("mp4"));
}

QString ConverterBackend::currentKind() const
{
    if (m_kindFilter == QLatin1String("all") || m_kindFilter == QLatin1String("doc"))
        return QStringLiteral("video");
    return m_kindFilter;
}

QString ConverterBackend::currentFormat() const
{
    return m_defaultFormats.value(currentKind(), QStringLiteral("mp4"));
}

void ConverterBackend::setCurrentFormat(const QString &format)
{
    applyFormatToKind(currentKind(), format);
}

void ConverterBackend::setDefaultFormat(const QString &kind, const QString &format)
{
    if (format.isEmpty() || m_defaultFormats.value(kind) == format)
        return;
    m_defaultFormats.insert(kind, format);
    persist(QStringLiteral("format/") + kind, format);
    emit currentFormatChanged();
}

void ConverterBackend::applyFormatToKind(const QString &kind, const QString &format)
{
    if (format.isEmpty())
        return;
    setDefaultFormat(kind, format);

    int changed = 0;
    const QStringList ids = m_model->ids();
    for (const QString &id : ids) {
        ConversionTask *task = m_model->find(id);
        if (!task || task->kind != kind)
            continue;
        if (task->status == TaskStatus::Queued || task->status == TaskStatus::Failed
            || task->status == TaskStatus::Canceled) {
            m_model->updateTask(id, [&](ConversionTask &t) {
                t.format = format;
                t.status = TaskStatus::Queued;
                t.error.clear();
                t.progress = 0.0;
            }, { TaskModel::FormatRole, TaskModel::StatusRole, TaskModel::StatusTextRole,
                 TaskModel::ErrorRole, TaskModel::ProgressRole });
            ++changed;
        }
    }
    if (changed > 0) {
        emit toast(QStringLiteral("已将 %1 个任务的目标格式设为 %2").arg(changed).arg(format.toUpper()),
                   QStringLiteral("info"));
    }
}

QString ConverterBackend::nameFilter(const QString &kind) const
{
    if (kind == QLatin1String("video"))
        return QStringLiteral("视频文件 (*.mp4 *.mkv *.avi *.mov *.flv *.wmv *.webm *.m4v *.ts *.mpg *.mpeg *.3gp)");
    if (kind == QLatin1String("audio"))
        return QStringLiteral("音频文件 (*.mp3 *.wav *.flac *.aac *.m4a *.ogg *.opus *.wma *.aiff)");
    if (kind == QLatin1String("image"))
        return QStringLiteral("图片文件 (*.png *.jpg *.jpeg *.webp *.bmp *.tif *.tiff *.gif)");
    return QStringLiteral("媒体文件 (*.mp4 *.mkv *.mov *.avi *.webm *.mp3 *.wav *.flac *.m4a *.ogg *.png *.jpg *.jpeg *.webp *.bmp *.gif)");
}

QString ConverterBackend::kindForFile(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (isVideoSuffix(suffix))
        return QStringLiteral("video");
    if (isAudioSuffix(suffix))
        return QStringLiteral("audio");
    if (isImageSuffix(suffix))
        return QStringLiteral("image");
    return QString();
}

bool ConverterBackend::isMediaExtension(const QString &suffix)
{
    const QString s = suffix.toLower();
    return isVideoSuffix(s) || isAudioSuffix(s) || isImageSuffix(s);
}

void ConverterBackend::addFiles(const QVariantList &urls)
{
    QStringList paths;
    for (const QVariant &value : urls) {
        const QUrl url = value.toUrl();
        const QString local = url.isLocalFile() ? url.toLocalFile() : value.toString();
        if (!local.isEmpty())
            paths << local;
    }
    addPaths(paths);
}

void ConverterBackend::addPaths(const QStringList &paths)
{
    QStringList files;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (info.isDir()) {
            QDirIterator it(path, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
            int guard = 0;
            while (it.hasNext() && guard < 800) {
                const QString candidate = it.next();
                if (isMediaExtension(QFileInfo(candidate).suffix()))
                    files << candidate;
                ++guard;
            }
        } else if (info.isFile()) {
            files << info.absoluteFilePath();
        }
    }

    files.removeDuplicates();

    int added = 0;
    int skipped = 0;
    int elsewhere = 0;
    QString firstKind;
    const QString currentFilter = m_kindFilter;

    for (const QString &file : std::as_const(files)) {
        const QString kind = kindForFile(file);
        if (kind.isEmpty()) {
            ++skipped;
            continue;
        }

        bool duplicate = false;
        const auto existing = m_model->tasks();
        for (const auto &t : existing) {
            if (t.inputPath.compare(file, Qt::CaseInsensitive) == 0) {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
            continue;

        ConversionTask task;
        task.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        task.inputPath = file;
        task.kind = kind;
        task.format = m_defaultFormats.value(kind, QStringLiteral("mp4"));
        task.status = TaskStatus::Queued;
        task.progress = 0.0;
        task.inputSize = QFileInfo(file).size();
        m_model->appendTask(task);
        probeTask(task.id);
        ++added;
        if (firstKind.isEmpty())
            firstKind = kind;
        if (kind != currentFilter)
            ++elsewhere;
    }

    if (added > 0) {
        if (elsewhere == added && !firstKind.isEmpty()) {
            setKindFilter(firstKind);
            emit toast(QStringLiteral("已添加 %1 个%2任务，已切换到对应分类")
                           .arg(added).arg(kindLabel(firstKind)),
                       QStringLiteral("success"));
        } else if (elsewhere > 0) {
            emit toast(QStringLiteral("已添加 %1 个任务，其中 %2 个在其他分类").arg(added).arg(elsewhere),
                       QStringLiteral("success"));
        } else {
            emit toast(QStringLiteral("已添加 %1 个任务").arg(added), QStringLiteral("success"));
        }
        if (m_queueActive)
            schedule();
    } else if (skipped > 0) {
        emit toast(QStringLiteral("没有识别到可转换的媒体文件"), QStringLiteral("warning"));
    } else {
        emit toast(QStringLiteral("这些文件已经在任务列表中了"), QStringLiteral("info"));
    }
}

QString ConverterBackend::kindLabel(const QString &kind)
{
    if (kind == QLatin1String("audio"))
        return QStringLiteral("音频");
    if (kind == QLatin1String("image"))
        return QStringLiteral("图片");
    if (kind == QLatin1String("doc"))
        return QStringLiteral("文档");
    return QStringLiteral("视频");
}

QString ConverterBackend::thumbDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (dir.isEmpty())
        dir = QDir::tempPath() + QStringLiteral("/formatstudio");
    dir = QDir(dir).filePath(QStringLiteral("thumbs"));
    QDir().mkpath(dir);
    return dir;
}

void ConverterBackend::probeTask(const QString &id)
{
    ConversionTask *task = m_model->find(id);
    if (!task)
        return;

    if (task->kind == QLatin1String("image")) {
        m_model->updateTask(id, [](ConversionTask &t) {
            t.thumbnail = QUrl::fromLocalFile(t.inputPath).toString();
        }, { TaskModel::ThumbnailRole });
    }

    if (m_ffprobe.isEmpty()) {
        m_model->updateTask(id, [](ConversionTask &t) {
            t.detail = QStringLiteral("等待转换");
            t.probed = true;
        }, { TaskModel::DetailRole });
        return;
    }

    auto *proc = new QProcess(this);
    const QStringList args{ QStringLiteral("-v"), QStringLiteral("error"),
                            QStringLiteral("-print_format"), QStringLiteral("json"),
                            QStringLiteral("-show_format"), QStringLiteral("-show_streams"),
                            task->inputPath };
    connect(proc, &QProcess::finished, this, [this, proc, id](int exitCode, QProcess::ExitStatus) {
        const QByteArray payload = proc->readAllStandardOutput();
        proc->deleteLater();
        if (exitCode != 0)
            return;

        const QJsonObject root = QJsonDocument::fromJson(payload).object();
        const QJsonObject format = root.value(QStringLiteral("format")).toObject();
        const QJsonArray streams = root.value(QStringLiteral("streams")).toArray();

        const qint64 durationMs = qint64(format.value(QStringLiteral("duration")).toString().toDouble() * 1000.0);
        int width = 0;
        int height = 0;
        QString videoCodec;
        QString audioCodec;
        int sampleRate = 0;
        int channels = 0;

        for (const QJsonValue &value : streams) {
            const QJsonObject stream = value.toObject();
            const QString type = stream.value(QStringLiteral("codec_type")).toString();
            const QJsonObject disposition = stream.value(QStringLiteral("disposition")).toObject();
            const bool attachedPic = disposition.value(QStringLiteral("attached_pic")).toInt() == 1;

            if (type == QLatin1String("video") && width == 0 && !attachedPic) {
                width = stream.value(QStringLiteral("width")).toInt();
                height = stream.value(QStringLiteral("height")).toInt();
                videoCodec = stream.value(QStringLiteral("codec_name")).toString().toUpper();
            } else if (type == QLatin1String("audio") && audioCodec.isEmpty()) {
                audioCodec = stream.value(QStringLiteral("codec_name")).toString().toUpper();
                sampleRate = stream.value(QStringLiteral("sample_rate")).toString().toInt();
                channels = stream.value(QStringLiteral("channels")).toInt();
            }
        }

        m_model->updateTask(id, [&](ConversionTask &t) {
            t.durationMs = durationMs;
            t.probed = true;
            QStringList parts;
            if (t.kind == QLatin1String("video")) {
                if (width > 0)
                    parts << QStringLiteral("%1×%2").arg(width).arg(height);
                if (!videoCodec.isEmpty())
                    parts << videoCodec;
                if (!audioCodec.isEmpty())
                    parts << audioCodec;
                if (durationMs > 0)
                    parts << formatDuration(durationMs);
            } else if (t.kind == QLatin1String("audio")) {
                if (sampleRate > 0)
                    parts << QStringLiteral("%1 kHz").arg(QString::number(sampleRate / 1000.0, 'g', 3));
                if (channels == 1)
                    parts << QStringLiteral("单声道");
                else if (channels == 2)
                    parts << QStringLiteral("立体声");
                else if (channels > 2)
                    parts << QStringLiteral("%1 声道").arg(channels);
                if (!audioCodec.isEmpty())
                    parts << audioCodec;
                if (durationMs > 0)
                    parts << formatDuration(durationMs);
            } else {
                if (width > 0)
                    parts << QStringLiteral("%1×%2").arg(width).arg(height);
                parts << t.format.toUpper();
            }
            t.detail = parts.join(QStringLiteral(" · "));
        }, { TaskModel::DetailRole, TaskModel::DurationRole });

        makeThumbnail(id);
    });

    proc->start(m_ffprobe, args);
}

void ConverterBackend::makeThumbnail(const QString &id)
{
    ConversionTask *task = m_model->find(id);
    if (!task || task->kind == QLatin1String("image") || m_ffmpeg.isEmpty())
        return;

    const QString out = QDir(thumbDir()).filePath(id + QStringLiteral(".jpg"));
    double position = 0.0;
    if (task->durationMs > 2000)
        position = qMin(3.0, task->durationMs / 1000.0 * 0.25);

    QStringList args{ QStringLiteral("-hide_banner"), QStringLiteral("-y"), QStringLiteral("-nostdin") };
    if (position > 0.0)
        args << QStringLiteral("-ss") << QString::number(position, 'f', 2);
    args << QStringLiteral("-i") << task->inputPath;
    if (task->kind == QLatin1String("audio"))
        args << QStringLiteral("-map") << QStringLiteral("0:v:0?");
    else
        args << QStringLiteral("-map") << QStringLiteral("0:v:0");
    args << QStringLiteral("-frames:v") << QStringLiteral("1")
         << QStringLiteral("-vf") << QStringLiteral("scale=360:-2")
         << QStringLiteral("-q:v") << QStringLiteral("4") << out;

    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc, id, out](int exitCode, QProcess::ExitStatus) {
        proc->deleteLater();
        if (exitCode != 0 || !QFileInfo::exists(out))
            return;
        m_model->updateTask(id, [out](ConversionTask &t) {
            t.thumbnail = QUrl::fromLocalFile(out).toString();
        }, { TaskModel::ThumbnailRole });
    });
    proc->start(m_ffmpeg, args);
}

QString ConverterBackend::formatDuration(qint64 ms)
{
    if (ms <= 0)
        return QString();
    const qint64 totalSeconds = ms / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;
    if (hours > 0) {
        return QStringLiteral("%1:%2:%3").arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

QString ConverterBackend::resolveOutputPath(const ConversionTask &task) const
{
    QDir dir(m_outputDir);
    if (!dir.exists())
        QDir().mkpath(m_outputDir);

    const QFileInfo info(task.inputPath);
    const QString base = info.completeBaseName();
    const QString ext = task.format.toLower();
    QString candidate = dir.filePath(base + QLatin1Char('.') + ext);

    if (m_overwritePolicy == 1)
        return candidate;
    if (m_overwritePolicy == 2 && QFileInfo::exists(candidate))
        return QString();

    int counter = 1;
    while (QFileInfo::exists(candidate) && counter <= 500) {
        candidate = dir.filePath(QStringLiteral("%1 (%2).%3").arg(base).arg(counter++).arg(ext));
    }
    return candidate;
}

QStringList ConverterBackend::buildArguments(const ConversionTask &task, const QString &outPath, QString *error) const
{
    const QString fmt = task.format.toLower();
    const QString videoPreset = m_quality == 2 ? QStringLiteral("veryfast") : QStringLiteral("medium");
    const int crf = m_quality == 0 ? 18 : (m_quality == 1 ? 23 : 28);
    const QString audioBitrate = m_quality == 0 ? QStringLiteral("320k")
                             : (m_quality == 1 ? QStringLiteral("192k") : QStringLiteral("128k"));

    QStringList args{ QStringLiteral("-hide_banner"), QStringLiteral("-nostdin"),
                      QStringLiteral("-y"), QStringLiteral("-i"), task.inputPath };

    if (task.kind == QLatin1String("image")) {
        if (fmt == QLatin1String("jpg") || fmt == QLatin1String("jpeg")) {
            args << QStringLiteral("-c:v") << QStringLiteral("mjpeg")
                 << QStringLiteral("-q:v") << QString::number(m_quality == 0 ? 2 : (m_quality == 1 ? 4 : 7));
        } else if (fmt == QLatin1String("webp")) {
            args << QStringLiteral("-c:v") << QStringLiteral("libwebp")
                 << QStringLiteral("-quality") << QString::number(m_quality == 0 ? 95 : (m_quality == 1 ? 85 : 72))
                 << QStringLiteral("-compression_level") << QStringLiteral("6");
        } else if (fmt == QLatin1String("bmp")) {
            args << QStringLiteral("-c:v") << QStringLiteral("bmp");
        } else {
            args << QStringLiteral("-c:v") << QStringLiteral("png");
        }
        args << QStringLiteral("-frames:v") << QStringLiteral("1");
    } else if (fmt == QLatin1String("mp3") || fmt == QLatin1String("m4a") || fmt == QLatin1String("aac")
               || fmt == QLatin1String("wav") || fmt == QLatin1String("flac") || fmt == QLatin1String("ogg")
               || fmt == QLatin1String("opus")) {
        args << QStringLiteral("-vn");
        if (fmt == QLatin1String("mp3")) {
            args << QStringLiteral("-c:a") << QStringLiteral("libmp3lame")
                 << QStringLiteral("-b:a") << audioBitrate;
        } else if (fmt == QLatin1String("m4a") || fmt == QLatin1String("aac")) {
            args << QStringLiteral("-c:a") << QStringLiteral("aac")
                 << QStringLiteral("-b:a") << audioBitrate;
        } else if (fmt == QLatin1String("wav")) {
            args << QStringLiteral("-c:a") << QStringLiteral("pcm_s16le");
        } else if (fmt == QLatin1String("flac")) {
            args << QStringLiteral("-c:a") << QStringLiteral("flac")
                 << QStringLiteral("-compression_level") << QStringLiteral("8");
        } else if (fmt == QLatin1String("ogg")) {
            args << QStringLiteral("-c:a") << QStringLiteral("libvorbis")
                 << QStringLiteral("-q:a") << QStringLiteral("6");
        } else {
            args << QStringLiteral("-c:a") << QStringLiteral("libopus")
                 << QStringLiteral("-b:a") << QStringLiteral("128k");
        }
    } else if (fmt == QLatin1String("gif")) {
        const int width = m_quality == 2 ? 320 : 480;
        const QString filter = QStringLiteral(
            "fps=12,scale=%1:-1:flags=lanczos,split[a][b];"
            "[a]palettegen=max_colors=256:stats_mode=diff[p];"
            "[b][p]paletteuse=dither=bayer:bayer_scale=3").arg(width);
        args << QStringLiteral("-filter_complex") << filter
             << QStringLiteral("-loop") << QStringLiteral("0")
             << QStringLiteral("-an");
    } else if (fmt == QLatin1String("mp4") || fmt == QLatin1String("mov") || fmt == QLatin1String("m4v")) {
        args << QStringLiteral("-map") << QStringLiteral("0:v:0?")
             << QStringLiteral("-map") << QStringLiteral("0:a:0?")
             << QStringLiteral("-c:v") << QStringLiteral("libx264")
             << QStringLiteral("-preset") << videoPreset
             << QStringLiteral("-crf") << QString::number(crf)
             << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p")
             << QStringLiteral("-c:a") << QStringLiteral("aac")
             << QStringLiteral("-b:a") << audioBitrate;
        if (fmt != QLatin1String("m4v"))
            args << QStringLiteral("-movflags") << QStringLiteral("+faststart");
    } else if (fmt == QLatin1String("mkv")) {
        args << QStringLiteral("-map") << QStringLiteral("0:v:0?")
             << QStringLiteral("-map") << QStringLiteral("0:a:0?")
             << QStringLiteral("-c:v") << QStringLiteral("libx264")
             << QStringLiteral("-preset") << videoPreset
             << QStringLiteral("-crf") << QString::number(crf)
             << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p")
             << QStringLiteral("-c:a") << QStringLiteral("aac")
             << QStringLiteral("-b:a") << audioBitrate;
    } else if (fmt == QLatin1String("webm")) {
        args << QStringLiteral("-map") << QStringLiteral("0:v:0?")
             << QStringLiteral("-map") << QStringLiteral("0:a:0?")
             << QStringLiteral("-c:v") << QStringLiteral("libvpx-vp9")
             << QStringLiteral("-crf") << QString::number(crf + 7)
             << QStringLiteral("-b:v") << QStringLiteral("0")
             << QStringLiteral("-row-mt") << QStringLiteral("1")
             << QStringLiteral("-c:a") << QStringLiteral("libopus")
             << QStringLiteral("-b:a") << QStringLiteral("128k");
    } else if (fmt == QLatin1String("avi")) {
        args << QStringLiteral("-map") << QStringLiteral("0:v:0?")
             << QStringLiteral("-map") << QStringLiteral("0:a:0?")
             << QStringLiteral("-c:v") << QStringLiteral("mpeg4")
             << QStringLiteral("-q:v") << QStringLiteral("3")
             << QStringLiteral("-c:a") << QStringLiteral("libmp3lame")
             << QStringLiteral("-b:a") << audioBitrate;
    } else {
        if (error)
            *error = QStringLiteral("暂不支持输出为 %1").arg(fmt.toUpper());
        return {};
    }

    args << QStringLiteral("-progress") << QStringLiteral("pipe:1")
         << QStringLiteral("-nostats")
         << QStringLiteral("-loglevel") << QStringLiteral("error")
         << outPath;
    return args;
}

void ConverterBackend::startTask(const QString &id)
{
    if (m_running.contains(id)) {
        emit toast(QStringLiteral("该任务正在转换中"), QStringLiteral("info"));
        return;
    }
    startTaskInternal(id);
}

void ConverterBackend::startTaskInternal(const QString &id)
{
    ConversionTask *task = m_model->find(id);
    if (!task || task->status == TaskStatus::Done)
        return;
    if (m_ffmpeg.isEmpty()) {
        emit toast(QStringLiteral("未找到 ffmpeg，请先在设置中指定路径"), QStringLiteral("error"));
        return;
    }

    const QString outPath = resolveOutputPath(*task);
    if (outPath.isEmpty()) {
        m_model->updateTask(id, [](ConversionTask &t) {
            t.status = TaskStatus::Skipped;
            t.error = QStringLiteral("输出文件已存在，已按设置跳过");
        }, { TaskModel::StatusRole, TaskModel::StatusTextRole, TaskModel::ErrorRole });
        return;
    }

    QString error;
    const QStringList args = buildArguments(*task, outPath, &error);
    if (args.isEmpty()) {
        failTask(id, error);
        return;
    }

    auto *proc = new QProcess(this);
    proc->setProgram(m_ffmpeg);
    proc->setArguments(args);
    proc->setProcessChannelMode(QProcess::SeparateChannels);
    proc->setWorkingDirectory(QFileInfo(outPath).absolutePath());

    m_running.insert(id, proc);
    m_canceled.remove(id);
    m_stdoutBuffers.insert(id, QString());
    m_stderrTails.insert(id, QString());
    m_progressUs.insert(id, 0);

    m_model->updateTask(id, [&](ConversionTask &t) {
        t.status = TaskStatus::Running;
        t.progress = 0.0;
        t.speed.clear();
        t.etaMs = -1;
        t.error.clear();
        t.outputPath = outPath;
        t.outputSize = 0;
    }, { TaskModel::StatusRole, TaskModel::StatusTextRole, TaskModel::ProgressRole,
         TaskModel::SpeedRole, TaskModel::EtaRole, TaskModel::ErrorRole,
         TaskModel::OutputPathRole, TaskModel::OutputSizeRole });

    connect(proc, &QProcess::readyReadStandardOutput, this, [this, id]() { handleProgressChunk(id); });
    connect(proc, &QProcess::readyReadStandardError, this, [this, id]() { handleLogChunk(id); });
    connect(proc, &QProcess::finished, this, [this, id](int exitCode, QProcess::ExitStatus status) {
        finishTask(id, exitCode, status);
    });
    connect(proc, &QProcess::errorOccurred, this, [this, id](QProcess::ProcessError err) {
        if (err == QProcess::FailedToStart)
            failTask(id, QStringLiteral("无法启动 ffmpeg，请检查设置中的路径"));
    });

    proc->start();
}

void ConverterBackend::failTask(const QString &id, const QString &reason)
{
    if (QProcess *proc = m_running.take(id)) {
        proc->disconnect(this);
        proc->kill();
        proc->deleteLater();
    }
    m_stdoutBuffers.remove(id);
    m_stderrTails.remove(id);
    m_progressUs.remove(id);
    m_canceled.remove(id);

    m_model->updateTask(id, [&](ConversionTask &t) {
        t.status = TaskStatus::Failed;
        t.error = reason;
        t.speed.clear();
        t.etaMs = -1;
    }, { TaskModel::StatusRole, TaskModel::StatusTextRole, TaskModel::ErrorRole,
         TaskModel::SpeedRole, TaskModel::EtaRole });
    emit toast(reason, QStringLiteral("error"));
    schedule();
}

void ConverterBackend::handleProgressChunk(const QString &id)
{
    QProcess *proc = m_running.value(id);
    if (!proc)
        return;

    QString &buffer = m_stdoutBuffers[id];
    buffer += QString::fromUtf8(proc->readAllStandardOutput());

    int newline = -1;
    while ((newline = buffer.indexOf(QLatin1Char('\n'))) >= 0) {
        const QString line = buffer.left(newline).trimmed();
        buffer.remove(0, newline + 1);
        if (line.isEmpty())
            continue;

        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        const QString key = line.left(eq);
        const QString value = line.mid(eq + 1);

        if (key == QLatin1String("out_time_us") || key == QLatin1String("out_time_ms")) {
            m_progressUs[id] = value.toLongLong();
        } else if (key == QLatin1String("progress")) {
            const ConversionTask *task = m_model->find(id);
            if (!task)
                continue;
            const qint64 us = m_progressUs.value(id, 0);
            const double positionMs = double(us) / 1000.0;
            double percent = -1.0;
            if (task->durationMs > 0)
                percent = qBound(0.0, positionMs / double(task->durationMs) * 100.0, 99.9);

            m_model->updateTask(id, [&](ConversionTask &t) {
                if (percent >= 0.0)
                    t.progress = percent;
                const double rate = t.speed.isEmpty() ? 0.0 : t.speed.chopped(1).toDouble();
                if (t.durationMs > 0 && rate > 0.05) {
                    const double remaining = double(t.durationMs) - positionMs;
                    t.etaMs = qMax<qint64>(0, qint64(remaining / rate));
                }
            }, { TaskModel::ProgressRole, TaskModel::EtaRole });
        } else if (key == QLatin1String("speed")) {
            m_model->updateTask(id, [&](ConversionTask &t) { t.speed = value; }, { TaskModel::SpeedRole });
        }
    }
}

void ConverterBackend::handleLogChunk(const QString &id)
{
    QProcess *proc = m_running.value(id);
    if (!proc)
        return;
    QString &tail = m_stderrTails[id];
    tail += QString::fromUtf8(proc->readAllStandardError());
    if (tail.size() > 4000)
        tail = tail.right(4000);
}

void ConverterBackend::finishTask(const QString &id, int exitCode, QProcess::ExitStatus status)
{
    QProcess *proc = m_running.take(id);
    if (!proc)
        return;

    const bool canceled = m_canceled.remove(id);
    const QString tail = m_stderrTails.value(id);
    m_stdoutBuffers.remove(id);
    m_stderrTails.remove(id);
    m_progressUs.remove(id);

    proc->deleteLater();

    ConversionTask *task = m_model->find(id);
    if (!task) {
        schedule();
        return;
    }
    const QString outPath = task->outputPath;

    if (canceled) {
        m_model->updateTask(id, [](ConversionTask &t) {
            t.status = TaskStatus::Canceled;
            t.speed.clear();
            t.etaMs = -1;
            t.progress = qBound(0.0, t.progress, 100.0);
        }, { TaskModel::StatusRole, TaskModel::StatusTextRole, TaskModel::SpeedRole,
             TaskModel::EtaRole, TaskModel::ProgressRole });
        schedule();
        return;
    }

    if (exitCode != 0 || status == QProcess::CrashExit) {
        const QString reason = humanError(tail, exitCode);
        m_model->updateTask(id, [&](ConversionTask &t) {
            t.status = TaskStatus::Failed;
            t.error = reason;
            t.logTail = tail.right(1200);
            t.speed.clear();
            t.etaMs = -1;
        }, { TaskModel::StatusRole, TaskModel::StatusTextRole, TaskModel::ErrorRole,
             TaskModel::LogRole, TaskModel::SpeedRole, TaskModel::EtaRole });
        emit toast(reason, QStringLiteral("error"));
        schedule();
        return;
    }

    if (outPath.isEmpty() || !QFileInfo::exists(outPath)) {
        m_model->updateTask(id, [](ConversionTask &t) {
            t.status = TaskStatus::Failed;
            t.error = QStringLiteral("输出文件未生成");
            t.speed.clear();
            t.etaMs = -1;
        }, { TaskModel::StatusRole, TaskModel::StatusTextRole, TaskModel::ErrorRole,
             TaskModel::SpeedRole, TaskModel::EtaRole });
        emit toast(QStringLiteral("输出文件未生成"), QStringLiteral("error"));
        schedule();
        return;
    }

    m_model->updateTask(id, [&](ConversionTask &t) {
        t.status = TaskStatus::Done;
        t.progress = 100.0;
        t.speed.clear();
        t.etaMs = 0;
        t.outputSize = QFileInfo(outPath).size();
        t.error.clear();
    }, { TaskModel::StatusRole, TaskModel::StatusTextRole, TaskModel::ProgressRole,
         TaskModel::SpeedRole, TaskModel::EtaRole, TaskModel::OutputSizeRole,
         TaskModel::ErrorRole });

    schedule();
}

bool ConverterBackend::hasQueuedFor(const QString &kind) const
{
    const auto tasks = m_model->tasks();
    for (const auto &task : tasks) {
        if (task.status != TaskStatus::Queued)
            continue;
        if (kind.isEmpty() || task.kind == kind)
            return true;
    }
    return false;
}

void ConverterBackend::schedule()
{
    if (!m_queueActive)
        return;

    while (m_running.size() < m_concurrency) {
        QString nextId;
        const auto tasks = m_model->tasks();
        for (const auto &task : tasks) {
            if (task.status != TaskStatus::Queued)
                continue;
            if (!m_activeKind.isEmpty() && task.kind != m_activeKind)
                continue;
            nextId = task.id;
            break;
        }
        if (nextId.isEmpty())
            break;

        startTaskInternal(nextId);

        ConversionTask *started = m_model->find(nextId);
        if (!m_running.contains(nextId) && started && started->status == TaskStatus::Queued)
            break;
    }

    if (m_queueActive && m_running.isEmpty() && !hasQueuedFor(m_activeKind)) {
        const QString finishedKind = m_activeKind;
        m_queueActive = false;
        m_activeKind.clear();
        emit queueActiveChanged();

        int ok = 0;
        int bad = 0;
        const auto tasks = m_model->tasks();
        for (const auto &task : tasks) {
            if (!finishedKind.isEmpty() && task.kind != finishedKind)
                continue;
            if (task.status == TaskStatus::Done)
                ++ok;
            else if (task.status == TaskStatus::Failed)
                ++bad;
        }

        if (bad > 0) {
            emit toast(QStringLiteral("队列结束：成功 %1 个，失败 %2 个").arg(ok).arg(bad),
                       QStringLiteral("warning"));
        } else {
            emit toast(QStringLiteral("全部完成，共 %1 个任务").arg(ok), QStringLiteral("success"));
        }
    }
}

void ConverterBackend::startAll()
{
    const QString kind = currentKind();
    if (!hasQueuedFor(kind)) {
        emit toast(QStringLiteral("「%1」分类没有等待转换的任务").arg(kindLabel(kind)),
                   QStringLiteral("info"));
        return;
    }
    if (m_ffmpeg.isEmpty()) {
        emit toast(QStringLiteral("未找到 ffmpeg，请先在设置中指定路径"), QStringLiteral("error"));
        return;
    }
    m_activeKind = kind;
    m_queueActive = true;
    emit queueActiveChanged();
    schedule();
}

void ConverterBackend::toggleQueue()
{
    const QString kind = currentKind();
    if (m_queueActive) {
        pauseQueue();
    } else if (hasQueuedFor(kind) || runningCount() > 0) {
        m_activeKind = kind;
        m_queueActive = true;
        emit queueActiveChanged();
        schedule();
    } else {
        emit toast(QStringLiteral("「%1」分类没有等待转换的任务").arg(kindLabel(kind)),
                   QStringLiteral("info"));
    }
}

void ConverterBackend::pauseQueue()
{
    if (!m_queueActive)
        return;
    m_queueActive = false;
    emit queueActiveChanged();
    emit toast(QStringLiteral("队列已暂停，当前任务将继续完成"), QStringLiteral("info"));
}

void ConverterBackend::resumeQueue()
{
    const QString kind = currentKind();
    if (m_queueActive || (!hasQueuedFor(kind) && runningCount() == 0))
        return;
    m_activeKind = kind;
    m_queueActive = true;
    emit queueActiveChanged();
    schedule();
}

void ConverterBackend::stopAll()
{
    if (m_running.isEmpty() && !m_queueActive) {
        emit toast(QStringLiteral("当前没有正在进行的转换"), QStringLiteral("info"));
        return;
    }
    m_queueActive = false;
    m_activeKind.clear();
    emit queueActiveChanged();

    const auto ids = m_running.keys();
    for (const QString &id : ids) {
        m_canceled.insert(id);
        if (QProcess *proc = m_running.value(id))
            proc->kill();
    }
    emit toast(QStringLiteral("已停止当前转换"), QStringLiteral("warning"));
}

void ConverterBackend::cancelTask(const QString &id)
{
    ConversionTask *task = m_model->find(id);
    if (!task)
        return;

    if (m_running.contains(id)) {
        m_canceled.insert(id);
        if (QProcess *proc = m_running.value(id))
            proc->kill();
        return;
    }

    if (task->status == TaskStatus::Queued || task->status == TaskStatus::Failed) {
        m_model->updateTask(id, [](ConversionTask &t) {
            t.status = TaskStatus::Canceled;
            t.speed.clear();
            t.etaMs = -1;
        }, { TaskModel::StatusRole, TaskModel::StatusTextRole,
             TaskModel::SpeedRole, TaskModel::EtaRole });
    }
}

void ConverterBackend::retryTask(const QString &id)
{
    ConversionTask *task = m_model->find(id);
    if (!task)
        return;
    m_model->updateTask(id, [](ConversionTask &t) {
        t.status = TaskStatus::Queued;
        t.progress = 0.0;
        t.speed.clear();
        t.etaMs = -1;
        t.error.clear();
    }, { TaskModel::StatusRole, TaskModel::StatusTextRole, TaskModel::ProgressRole,
         TaskModel::SpeedRole, TaskModel::EtaRole, TaskModel::ErrorRole });

    if (m_queueActive)
        schedule();
    else
        startTaskInternal(id);
}

void ConverterBackend::removeTask(const QString &id)
{
    if (m_running.contains(id)) {
        m_canceled.insert(id);
        if (QProcess *proc = m_running.value(id))
            proc->kill();
    }
    m_model->removeTask(id);
}

void ConverterBackend::removeFinished()
{
    removeFinishedInternal(QString());
}

void ConverterBackend::removeFinishedKind(const QString &kind)
{
    removeFinishedInternal(kind);
}

void ConverterBackend::removeFinishedInternal(const QString &kind)
{
    int removed = 0;
    const QStringList ids = m_model->ids();
    for (const QString &id : ids) {
        ConversionTask *task = m_model->find(id);
        if (!task)
            continue;
        if (!kind.isEmpty() && task->kind != kind)
            continue;
        if (task->status == TaskStatus::Done || task->status == TaskStatus::Failed
            || task->status == TaskStatus::Canceled || task->status == TaskStatus::Skipped) {
            m_model->removeTask(id);
            ++removed;
        }
    }
    if (removed > 0)
        emit toast(QStringLiteral("已清除 %1 个已结束任务").arg(removed), QStringLiteral("info"));
}

void ConverterBackend::clearAll()
{
    clearInternal(QString());
}

void ConverterBackend::clearKind(const QString &kind)
{
    clearInternal(kind);
}

void ConverterBackend::clearInternal(const QString &kind)
{
    if (!m_running.isEmpty()) {
        emit toast(QStringLiteral("还有任务正在转换，请先停止"), QStringLiteral("warning"));
        return;
    }
    if (kind.isEmpty()) {
        m_model->clearAll();
        return;
    }
    const QStringList ids = m_model->ids();
    for (const QString &id : ids) {
        ConversionTask *task = m_model->find(id);
        if (task && task->kind == kind)
            m_model->removeTask(id);
    }
}

void ConverterBackend::setTaskFormat(const QString &id, const QString &format)
{
    ConversionTask *task = m_model->find(id);
    if (!task || format.isEmpty() || task->status == TaskStatus::Running)
        return;
    m_model->updateTask(id, [&](ConversionTask &t) {
        t.format = format;
        if (t.status != TaskStatus::Queued) {
            t.status = TaskStatus::Queued;
            t.progress = 0.0;
            t.error.clear();
        }
    }, { TaskModel::FormatRole, TaskModel::StatusRole, TaskModel::StatusTextRole,
         TaskModel::ProgressRole, TaskModel::ErrorRole });
}

void ConverterBackend::revealOutput(const QString &id)
{
    ConversionTask *task = m_model->find(id);
    if (!task)
        return;
    if (QFileInfo::exists(task->outputPath))
        QDesktopServices::openUrl(QUrl::fromLocalFile(task->outputPath));
    else
        openOutputDir();
}

void ConverterBackend::openOutputDir()
{
    QDir().mkpath(m_outputDir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_outputDir));
}

void ConverterBackend::chooseOutputDir(const QVariant &urlOrPath)
{
    const QUrl url = urlOrPath.toUrl();
    const QString path = url.isLocalFile() ? url.toLocalFile() : urlOrPath.toString();
    if (!path.isEmpty())
        setOutputDir(path);
}

int ConverterBackend::countKind(const QString &kind) const
{
    int count = 0;
    const auto tasks = m_model->tasks();
    for (const auto &task : tasks) {
        if (task.kind == kind)
            ++count;
    }
    return count;
}

void ConverterBackend::setFfmpegDir(const QString &dir)
{
    if (dir == m_ffmpegDir)
        return;
    m_ffmpegDir = dir;
    persist(QStringLiteral("ffmpeg/dir"), dir);
    detectFfmpeg();
    emit ffmpegChanged();
}

void ConverterBackend::detectFfmpeg()
{
    const QString exeName = QStringLiteral("ffmpeg.exe");
    const QString appDir = QCoreApplication::applicationDirPath();

    QStringList dirs;
    if (!m_ffmpegDir.isEmpty())
        dirs << m_ffmpegDir;
    dirs << appDir
         << QDir(appDir).filePath(QStringLiteral("ffmpeg"))
         << QDir(appDir).filePath(QStringLiteral("tools/ffmpeg"))
         << QDir(appDir).filePath(QStringLiteral("tools/ffmpeg/bin"))
         << QDir(appDir).filePath(QStringLiteral("../tools/ffmpeg/bin"))
         << QDir(appDir).filePath(QStringLiteral("../third_party/ffmpeg/bin"));

    QString ffmpeg;
    for (const QString &dir : std::as_const(dirs)) {
        const QFileInfo info(QDir(dir).filePath(exeName));
        if (info.exists()) {
            ffmpeg = info.absoluteFilePath();
            break;
        }
    }
    if (ffmpeg.isEmpty()) {
        const QString found = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
        if (!found.isEmpty())
            ffmpeg = found;
    }

    QString ffprobe;
    if (!ffmpeg.isEmpty()) {
        const QDir dir = QFileInfo(ffmpeg).absoluteDir();
        const QFileInfo probe(dir.filePath(QStringLiteral("ffprobe.exe")));
        if (probe.exists())
            ffprobe = probe.absoluteFilePath();
    }

    const bool changed = ffmpeg != m_ffmpeg || ffprobe != m_ffprobe;
    m_ffmpeg = ffmpeg;
    m_ffprobe = ffprobe;
    m_ffmpegVersion.clear();

    if (!m_ffmpeg.isEmpty()) {
        auto *proc = new QProcess(this);
        connect(proc, &QProcess::finished, this, [this, proc]() {
            const QString first = QString::fromUtf8(proc->readAllStandardOutput())
                                      .split(QLatin1Char('\n')).value(0).simplified();
            proc->deleteLater();
            m_ffmpegVersion = first;
            if (m_ffmpegVersion.startsWith(QLatin1String("ffmpeg version ")))
                m_ffmpegVersion = m_ffmpegVersion.mid(15).section(QLatin1Char(' '), 0, 0);
            emit ffmpegChanged();
        });
        proc->start(m_ffmpeg, { QStringLiteral("-version") });
    }

    if (changed || !m_ffmpeg.isEmpty())
        emit ffmpegChanged();
}

void ConverterBackend::seedDemoData()
{
    const QString dir = thumbDir();
    const QColor palette[4][2] = {
        { QColor("#2C4A7C"), QColor("#0F1726") },
        { QColor("#7C4A2C"), QColor("#1F0F0A") },
        { QColor("#2C7C5E"), QColor("#0A1F18") },
        { QColor("#4A2C7C"), QColor("#120A1F") },
    };

    QStringList posters;
    for (int i = 0; i < 4; ++i) {
        const QString path = QDir(dir).filePath(QStringLiteral("demo-%1.png").arg(i + 1));
        QImage image(480, 270, QImage::Format_RGB32);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient gradient(0, 0, 480, 270);
        gradient.setColorAt(0.0, palette[i][0]);
        gradient.setColorAt(1.0, palette[i][1]);
        painter.fillRect(image.rect(), gradient);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 26));
        painter.drawEllipse(QPointF(392, 54), 96, 96);
        painter.setBrush(QColor(0, 0, 0, 48));
        painter.drawEllipse(QPointF(84, 232), 132, 132);
        painter.setBrush(QColor(255, 255, 255, 18));
        painter.drawRoundedRect(QRectF(24, 196, 132, 10), 5, 5);
        painter.end();
        image.save(path);
        posters << QUrl::fromLocalFile(path).toString();
    }

    struct Demo {
        const char *name;
        const char *kind;
        const char *format;
        int status;
        double progress;
        const char *speed;
        qint64 etaMs;
        const char *detail;
        qint64 durationMs;
        qint64 size;
        int poster;
        const char *error;
    };

    const Demo demos[] = {
        { "海边日落_4K.mp4", "video", "mp4", 1, 46.0, "1.82x", 74000,
          "3840×2160 · H.264 · AAC · 00:02:13", 133000, 1288490188LL, 0, "" },
        { "产品演示_v3.mov", "video", "mp4", 2, 100.0, "", 0,
          "1920×1080 · H.264 · AAC · 00:00:47", 47000, 268435456LL, 1, "" },
        { "Podcast_EP12.wav", "audio", "mp3", 0, 0.0, "", -1,
          "48 kHz · 立体声 · PCM · 00:32:10", 1930000, 369098752LL, 2, "" },
        { "海报_poster.png", "image", "webp", 2, 100.0, "", 0,
          "2400×1600 · PNG", 0, 18874368LL, 3, "" },
        { "婚礼剪辑.mkv", "video", "mp4", 3, 0.0, "", -1,
          "1280×720 · H.264 · AAC · 00:05:02", 302000, 734003200LL, 0,
          "文件损坏或格式无法识别" },
        { "背景音乐.flac", "audio", "wav", 0, 0.0, "", -1,
          "44.1 kHz · 立体声 · FLAC · 00:03:28", 208000, 41943040LL, 2, "" },
    };

    int index = 0;
    for (const Demo &demo : demos) {
        ConversionTask task;
        task.id = QStringLiteral("demo-%1").arg(++index);
        task.inputPath = QDir(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation))
                             .filePath(QString::fromUtf8(demo.name));
        task.kind = QString::fromUtf8(demo.kind);
        task.format = QString::fromUtf8(demo.format);
        task.status = static_cast<TaskStatus>(demo.status);
        task.progress = demo.progress;
        task.speed = QString::fromUtf8(demo.speed);
        task.etaMs = demo.etaMs;
        task.detail = QString::fromUtf8(demo.detail);
        task.inputSize = demo.size;
        task.outputSize = demo.status == 2 ? qint64(demo.size * 62 / 100) : 0;
        task.thumbnail = posters.value(demo.poster);
        task.error = QString::fromUtf8(demo.error);
        task.durationMs = demo.durationMs;
        task.probed = true;
        m_model->appendTask(task);
    }
}
