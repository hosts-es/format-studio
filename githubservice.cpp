#include "githubservice.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <QSysInfo>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {
const char *kDefaultRepo = "hosts-es/format-studio";
} // namespace

GithubService::GithubService(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_settings(QStringLiteral("FormatStudio"), QStringLiteral("FormatStudio"))
{
    m_repo = m_settings.value(QStringLiteral("github/repo"), QString::fromLatin1(kDefaultRepo)).toString();
}

GithubService::~GithubService()
{
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
    }
    if (m_saveFile) {
        m_saveFile->cancelWriting();
        delete m_saveFile;
    }
}

QString GithubService::currentVersion() const
{
    return QCoreApplication::applicationVersion();
}

QString GithubService::repo() const
{
    return m_repo;
}

void GithubService::setRepo(const QString &repo)
{
    const QString trimmed = repo.trimmed();
    if (trimmed.isEmpty() || trimmed == m_repo)
        return;
    m_repo = trimmed;
    m_settings.setValue(QStringLiteral("github/repo"), trimmed);
    emit repoChanged();
}

QString GithubService::normalizedVersion(QString version)
{
    version = version.trimmed();
    if (version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        version.remove(0, 1);
    QString cleaned;
    cleaned.reserve(version.size());
    for (const QChar c : version) {
        if (c.isDigit() || c == QLatin1Char('.'))
            cleaned.append(c);
        else if (!cleaned.isEmpty())
            break;
    }
    return cleaned;
}

int GithubService::compareVersions(const QString &lhs, const QString &rhs)
{
    auto parts = [](const QString &v) {
        QList<int> list;
        const QStringList chunks = normalizedVersion(v).split(QLatin1Char('.'), Qt::SkipEmptyParts);
        for (const QString &chunk : chunks)
            list << chunk.toInt();
        while (list.size() < 3)
            list << 0;
        return list;
    };

    const QList<int> a = parts(lhs);
    const QList<int> b = parts(rhs);
    const int size = qMax(a.size(), b.size());
    for (int i = 0; i < size; ++i) {
        const int va = i < a.size() ? a.at(i) : 0;
        const int vb = i < b.size() ? b.at(i) : 0;
        if (va != vb)
            return va < vb ? -1 : 1;
    }
    return 0;
}

void GithubService::checkForUpdates()
{
    if (m_checking)
        return;

    m_checking = true;
    emit checkingChanged();

    const QUrl url(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(m_repo));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("FormatStudio/%1").arg(currentVersion()));
    request.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_checking = false;
        emit checkingChanged();
        handleReleaseReply(reply);
        reply->deleteLater();
    });
}

void GithubService::handleReleaseReply(QNetworkReply *reply)
{
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (status == 404) {
        emit toast(QStringLiteral("未找到 Release：请确认仓库地址（当前 %1）").arg(m_repo),
                   QStringLiteral("warning"));
        return;
    }
    if (status == 403) {
        emit toast(QStringLiteral("GitHub API 访问受限，请稍后再试"), QStringLiteral("warning"));
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        emit toast(QStringLiteral("检查更新失败：%1").arg(reply->errorString()), QStringLiteral("error"));
        return;
    }

    const QJsonObject release = QJsonDocument::fromJson(reply->readAll()).object();
    const QString tag = release.value(QStringLiteral("tag_name")).toString();
    if (tag.isEmpty()) {
        emit toast(QStringLiteral("Release 信息不完整，缺少版本号"), QStringLiteral("warning"));
        return;
    }

    m_latestVersion = normalizedVersion(tag);
    m_releaseNotes = release.value(QStringLiteral("body")).toString().trimmed();
    m_downloadUrl = pickInstaller(release);
    m_updateAvailable = compareVersions(m_latestVersion, currentVersion()) > 0;
    emit releaseInfoChanged();

    if (!m_updateAvailable) {
        emit toast(QStringLiteral("当前已是最新版本 V%1").arg(currentVersion()),
                   QStringLiteral("success"));
        return;
    }
    if (m_downloadUrl.isEmpty()) {
        emit toast(QStringLiteral("发现新版本 V%1，但 Release 中没有可执行安装包")
                       .arg(m_latestVersion),
                   QStringLiteral("warning"));
    }
}

QString GithubService::pickInstaller(const QJsonObject &release) const
{
    struct Candidate {
        QString name;
        QString url;
        int rank = -1;
    };
    Candidate best;

    const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue &value : assets) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        const QString url = asset.value(QStringLiteral("browser_download_url")).toString();
        const QString lower = name.toLower();
        int rank = -1;
        if (lower.endsWith(QLatin1String(".exe")))
            rank = 3;
        else if (lower.endsWith(QLatin1String(".msi")))
            rank = 2;
        else if (lower.endsWith(QLatin1String(".zip")))
            rank = 1;
        if (rank > best.rank) {
            best.name = name;
            best.url = url;
            best.rank = rank;
        }
    }
    return best.url;
}

void GithubService::downloadAndInstall()
{
    if (m_downloadReply)
        return;
    if (m_downloadUrl.isEmpty()) {
        emit toast(QStringLiteral("没有可下载的安装包"), QStringLiteral("warning"));
        return;
    }

    const QString dir = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                            .filePath(QStringLiteral("FormatStudio-update"));
    QDir().mkpath(dir);

    QString fileName = QUrl(m_downloadUrl).fileName();
    if (fileName.isEmpty())
        fileName = QStringLiteral("FormatStudio-update.exe");
    m_downloadFile = QDir(dir).filePath(fileName);

    m_saveFile = new QSaveFile(m_downloadFile, this);
    if (!m_saveFile->open(QIODevice::WriteOnly)) {
        emit toast(QStringLiteral("无法写入临时目录，更新已取消"), QStringLiteral("error"));
        delete m_saveFile;
        m_saveFile = nullptr;
        return;
    }

    m_downloadPercent = 0;
    emit downloadPercentChanged();

    QNetworkRequest request{QUrl(m_downloadUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("FormatStudio/%1").arg(currentVersion()));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    m_downloadReply = m_nam->get(request);
    emit downloadingChanged();

    connect(m_downloadReply, &QNetworkReply::readyRead, this, [this]() {
        if (m_saveFile)
            m_saveFile->write(m_downloadReply->readAll());
    });

    connect(m_downloadReply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) {
                if (total <= 0)
                    return;
                const int percent = int(qBound<qint64>(qint64(0), received * 100 / total, qint64(100)));
                if (percent == m_downloadPercent)
                    return;
                m_downloadPercent = percent;
                emit downloadPercentChanged();
            });

    connect(m_downloadReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_downloadReply;
        m_downloadReply = nullptr;

        if (m_saveFile && reply->error() == QNetworkReply::NoError)
            m_saveFile->write(reply->readAll());

        const bool ok = m_saveFile && reply->error() == QNetworkReply::NoError
                        && m_saveFile->commit();
        if (m_saveFile) {
            m_saveFile->deleteLater();
            m_saveFile = nullptr;
        }
        reply->deleteLater();
        emit downloadingChanged();

        if (!ok) {
            emit toast(QStringLiteral("下载失败：%1").arg(reply->errorString()),
                       QStringLiteral("error"));
            return;
        }

        emit toast(QStringLiteral("下载完成，正在启动安装程序"), QStringLiteral("success"));
        if (!QProcess::startDetached(m_downloadFile, {})) {
            emit toast(QStringLiteral("无法启动安装程序，请手动运行：%1").arg(m_downloadFile),
                       QStringLiteral("error"));
            return;
        }
        QTimer::singleShot(1200, QCoreApplication::instance(), &QCoreApplication::quit);
    });
}

void GithubService::submitFeedback(int type, const QString &content)
{
    const QString message = content.trimmed();
    if (message.isEmpty()) {
        emit toast(QStringLiteral("请先填写留言内容"), QStringLiteral("warning"));
        return;
    }

    const bool bug = type == 0;
    const QString kind = bug ? QStringLiteral("Bug 反馈") : QStringLiteral("功能期待");
    const QString title = bug
        ? QStringLiteral("[Bug] 反馈 · V%1").arg(currentVersion())
        : QStringLiteral("[Feature] 功能期待 · V%1").arg(currentVersion());

    const QString body = QStringLiteral(
                             "**反馈类型**：%1\n"
                             "**应用版本**：V%2\n"
                             "**系统**：%3\n\n"
                             "---\n\n"
                             "%4")
                             .arg(kind, currentVersion(), QSysInfo::prettyProductName(), message);

    if (QClipboard *clipboard = QGuiApplication::clipboard())
        clipboard->setText(body);

    QUrl url(QStringLiteral("https://github.com/%1/issues/new").arg(m_repo));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("title"), title);
    query.addQueryItem(QStringLiteral("labels"), bug ? QStringLiteral("bug")
                                                    : QStringLiteral("enhancement"));

    // Windows 的 URL 长度有限，正文过长时只预填标题，正文走剪贴板。
    QUrlQuery withBody = query;
    withBody.addQueryItem(QStringLiteral("body"), body);
    url.setQuery(withBody);

    const bool bodyInUrl = url.toString(QUrl::FullyEncoded).size() < 1800;
    if (!bodyInUrl)
        url.setQuery(query);

    if (!QDesktopServices::openUrl(url)) {
        emit toast(QStringLiteral("无法打开浏览器，内容已复制到剪贴板"), QStringLiteral("error"));
        return;
    }
    emit toast(bodyInUrl
                   ? QStringLiteral("已在浏览器打开 GitHub 提交页，确认后即发布")
                   : QStringLiteral("留言较长：已复制到剪贴板，请在打开的 Issue 页面粘贴"),
               QStringLiteral("success"));
}

void GithubService::openUrl(const QString &url)
{
    QDesktopServices::openUrl(QUrl(url));
}

void GithubService::previewUpdate(const QString &version, const QString &notes)
{
    m_latestVersion = normalizedVersion(version);
    m_releaseNotes = notes;
    m_downloadUrl = QStringLiteral("https://example.invalid/FormatStudio-setup.exe");
    m_updateAvailable = true;
    emit releaseInfoChanged();
}
