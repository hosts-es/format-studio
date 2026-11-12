#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSaveFile>
#include <QSettings>
#include <QString>

class QNetworkReply;

/// Update checks and feedback submission against the project's GitHub repository.
class GithubService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString repo READ repo WRITE setRepo NOTIFY repoChanged)
    Q_PROPERTY(bool checking READ checking NOTIFY checkingChanged)
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged)
    Q_PROPERTY(int downloadPercent READ downloadPercent NOTIFY downloadPercentChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY releaseInfoChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY releaseInfoChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY releaseInfoChanged)

public:
    explicit GithubService(QObject *parent = nullptr);
    ~GithubService() override;

    QString currentVersion() const;
    QString repo() const;
    void setRepo(const QString &repo);

    bool checking() const { return m_checking; }
    bool downloading() const { return m_downloadReply != nullptr; }
    int downloadPercent() const { return m_downloadPercent; }
    bool updateAvailable() const { return m_updateAvailable; }
    QString latestVersion() const { return m_latestVersion; }
    QString releaseNotes() const { return m_releaseNotes; }

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void downloadAndInstall();
    Q_INVOKABLE void submitFeedback(int type, const QString &content);
    Q_INVOKABLE void openUrl(const QString &url);
    Q_INVOKABLE void previewUpdate(const QString &version, const QString &notes);

signals:
    void toast(const QString &text, const QString &level);
    void repoChanged();
    void checkingChanged();
    void downloadingChanged();
    void downloadPercentChanged();
    void releaseInfoChanged();

private:
    void handleReleaseReply(QNetworkReply *reply);
    QString pickInstaller(const QJsonObject &release) const;
    static QString normalizedVersion(QString version);
    static int compareVersions(const QString &lhs, const QString &rhs);

    QNetworkAccessManager *m_nam = nullptr;
    QSettings m_settings;

    QString m_repo;
    bool m_checking = false;
    bool m_updateAvailable = false;
    QString m_latestVersion;
    QString m_releaseNotes;
    QString m_downloadUrl;

    QNetworkReply *m_downloadReply = nullptr;
    QSaveFile *m_saveFile = nullptr;
    QString m_downloadFile;
    int m_downloadPercent = 0;
};
