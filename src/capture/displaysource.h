#pragma once

#include <QRect>
#include <QString>
#include <QtGlobal>

// One output in the virtual desktop. Geometry is X11 root-window (physical)
// pixels for ximagesrc / x11grab. Convert QScreen DIP geometry with
// scaleToNativePixels() first; do not pass QScreen::geometry() unchanged.
struct DisplaySource {
    QString id;
    QString name;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool primary = false;

    bool isValid() const;
    QString shortName() const;
};

inline bool DisplaySource::isValid() const
{
    return width > 0 && height > 0;
}

inline QString DisplaySource::shortName() const
{
    if (!id.isEmpty())
        return id;
    if (!name.isEmpty())
        return name;
    return QStringLiteral("%1x%2+%3+%4").arg(width).arg(height).arg(x).arg(y);
}

// Inclusive endx/endy for GStreamer ximagesrc. Empty if the source is invalid
// (caller should grab the whole root).
inline QString ximagesrcRegionProperties(const DisplaySource &source)
{
    if (!source.isValid())
        return {};
    return QStringLiteral("startx=%1 starty=%2 endx=%3 endy=%4")
        .arg(source.x)
        .arg(source.y)
        .arg(source.x + source.width - 1)
        .arg(source.y + source.height - 1);
}

inline QString x11grabSize(const DisplaySource &source)
{
    if (!source.isValid())
        return {};
    return QStringLiteral("%1x%2").arg(source.width).arg(source.height);
}

inline QString x11grabInputSpecifier(const QString &display, const DisplaySource &source)
{
    if (!source.isValid())
        return display;
    return QStringLiteral("%1+%2,%3").arg(display).arg(source.x).arg(source.y);
}

// QScreen::geometry() is device-independent pixels. x11grab / ximagesrc crop the
// X11 root in physical pixels. A 4K panel at 200% scale is 1920×1080 in Qt and
// 3840×2160 on the wire; using the logical size grabs only the top-left quarter.
inline QRect scaleToNativePixels(const QRect &logical, qreal devicePixelRatio)
{
    if (!logical.isValid() || devicePixelRatio <= 0.0 || qFuzzyCompare(devicePixelRatio, qreal(1)))
        return logical;
    return QRect(qRound(logical.x() * devicePixelRatio),
                 qRound(logical.y() * devicePixelRatio),
                 qRound(logical.width() * devicePixelRatio),
                 qRound(logical.height() * devicePixelRatio));
}
