#pragma once

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QQuickImageProvider>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QSvgRenderer>

/// Renders the bundled monochrome SVG icon set in any colour.
/// URL form: image://icons/<name>/<rrggbb>
class IconProvider : public QQuickImageProvider
{
public:
    IconProvider()
        : QQuickImageProvider(QQuickImageProvider::Pixmap)
    {
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        const QString name = id.section(QLatin1Char('/'), 0, 0);
        QString colorHex = id.section(QLatin1Char('/'), 1, 1);
        if (colorHex.size() == 6)
            colorHex.prepend(QLatin1Char('#'));
        const QColor color(colorHex.isEmpty() ? QStringLiteral("#FFFFFF") : colorHex);

        QSize target = requestedSize;
        if (target.isEmpty() || target.width() <= 0 || target.height() <= 0)
            target = QSize(24, 24);
        target = target.expandedTo(QSize(8, 8));

        QImage image(target, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QSvgRenderer renderer(QStringLiteral(":/icons/%1.svg").arg(name));
        if (renderer.isValid()) {
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing, true);
            renderer.render(&painter, QRectF(0, 0, target.width(), target.height()));
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(image.rect(), color);
        }

        if (size)
            *size = target;
        return QPixmap::fromImage(image);
    }
};
