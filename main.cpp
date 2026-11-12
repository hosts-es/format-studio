#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>
#include <QVariant>

#include "converterbackend.h"
#include "githubservice.h"
#include "iconprovider.h"
#include "taskmodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("FormatStudio"));
    app.setApplicationName(QStringLiteral("Format Studio"));
    app.setApplicationVersion(QStringLiteral("0.2.0"));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QFont baseFont = app.font();
    baseFont.setFamilies({ QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Microsoft YaHei"),
                           QStringLiteral("Segoe UI") });
    baseFont.setPointSizeF(10.0);
    app.setFont(baseFont);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Format Studio - 格式转换工作台"));
    parser.addHelpOption();
    parser.addOption({ QStringLiteral("demo"), QStringLiteral("载入演示任务（用于截图/预览）") });
    parser.addOption({ QStringLiteral("screenshot"), QStringLiteral("渲染后保存窗口截图"), QStringLiteral("path") });
    parser.addOption({ QStringLiteral("theme"), QStringLiteral("auto / light / dark"), QStringLiteral("mode"), QStringLiteral("auto") });
    parser.addOption({ QStringLiteral("size"), QStringLiteral("窗口尺寸，例如 1240x800"), QStringLiteral("size"), QStringLiteral("1240x800") });
    parser.addOption({ QStringLiteral("page"), QStringLiteral("初始页面: convert / settings / about / docs"), QStringLiteral("name"), QStringLiteral("convert") });
    parser.addOption({ QStringLiteral("smoke"), QStringLiteral("冒烟测试：分号分隔的输入文件，转换完成后退出"), QStringLiteral("files") });
    parser.addOption({ QStringLiteral("to"), QStringLiteral("冒烟测试的目标格式，例如 mkv"), QStringLiteral("format") });
    parser.addOption({ QStringLiteral("repo"), QStringLiteral("临时覆盖 GitHub 仓库 owner/repo"), QStringLiteral("repo") });
    parser.addOption({ QStringLiteral("check-update"), QStringLiteral("检查更新后退出（开发用）") });
    parser.addOption({ QStringLiteral("fake-update"), QStringLiteral("预览「发现新版本」对话框（开发用）") });
    parser.process(app);

    ConverterBackend backend;
    GithubService updates;
    if (parser.isSet(QStringLiteral("repo")))
        updates.setRepo(parser.value(QStringLiteral("repo")));
    if (parser.isSet(QStringLiteral("theme")))
        backend.setThemeMode(parser.value(QStringLiteral("theme")));
    if (parser.isSet(QStringLiteral("demo")))
        backend.seedDemoData();

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("icons"), new IconProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    engine.rootContext()->setContextProperty(QStringLiteral("updates"), &updates);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("FormatStudio"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return -1;

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (!window)
        return -1;

    const QStringList sizeParts = parser.value(QStringLiteral("size")).split(QLatin1Char('x'));
    if (sizeParts.size() == 2)
        window->resize(sizeParts.at(0).toInt(), sizeParts.at(1).toInt());

    const QString initialPage = parser.value(QStringLiteral("page"));
    if (initialPage != QLatin1String("convert"))
        QMetaObject::invokeMethod(window, "selectPageByName", Q_ARG(QVariant, QVariant(initialPage)));

    if (parser.isSet(QStringLiteral("screenshot"))) {
        const QString target = parser.value(QStringLiteral("screenshot"));
        QDir().mkpath(QFileInfo(target).absolutePath());
        QTimer::singleShot(1500, &app, [window, target, &backend]() {
            const QImage image = window->grabWindow();
            if (!image.isNull()) {
                image.save(target);
                qInfo("screenshot written: %s (%dx%d) state[filter=%s format=%s total=%d]",
                      qPrintable(target), image.width(), image.height(),
                      qPrintable(backend.kindFilter()), qPrintable(backend.currentFormat()),
                      backend.totalCount());
            } else {
                qWarning("screenshot failed: grabWindow() returned a null image");
            }
            QCoreApplication::quit();
        });
    }

    if (parser.isSet(QStringLiteral("fake-update"))) {
        QTimer::singleShot(400, &updates, [&updates]() {
            updates.previewUpdate(QStringLiteral("0.2.1"),
                                  QStringLiteral("· 新增批量重命名\n"
                                                 "· 修复 WebM 转换进度不准确\n"
                                                 "· 优化长任务的内存占用"));
        });
    }

    if (parser.isSet(QStringLiteral("check-update"))) {
        QObject::connect(&updates, &GithubService::toast, &app,
                         [](const QString &text, const QString &level) {
                             qInfo("update-toast [%s] %s", qPrintable(level), qPrintable(text));
                         });
        QObject::connect(&updates, &GithubService::releaseInfoChanged, &app, [&updates]() {
            qInfo("release: latest=%s available=%d notes=%d chars",
                  qPrintable(updates.latestVersion()),
                  updates.updateAvailable() ? 1 : 0,
                  int(updates.releaseNotes().size()));
            QCoreApplication::exit(0);
        });
        QTimer::singleShot(300, &updates, &GithubService::checkForUpdates);
        QTimer::singleShot(12000, &app, []() { QCoreApplication::exit(3); });
    }

    if (parser.isSet(QStringLiteral("smoke"))) {
        const QStringList inputs = parser.value(QStringLiteral("smoke"))
                                       .split(QLatin1Char(';'), Qt::SkipEmptyParts);
        backend.addPaths(inputs);
        const QString target = parser.value(QStringLiteral("to"));
        if (!target.isEmpty()) {
            const QStringList ids = backend.tasks()->ids();
            for (const QString &id : ids)
                backend.setTaskFormat(id, target);
        }
        QObject::connect(&backend, &ConverterBackend::countsChanged, &app, [&backend]() {
            if (backend.totalCount() == 0)
                return;
            if (backend.runningCount() == 0 && backend.queuedCount() == 0) {
                qInfo("smoke result: total=%d done=%d failed=%d",
                      backend.totalCount(), backend.doneCount(), backend.failedCount());
                QCoreApplication::exit(backend.failedCount() > 0 ? 2 : 0);
            }
        });
        QTimer::singleShot(300, &app, [&backend]() { backend.startAll(); });
    }

    return app.exec();
}
