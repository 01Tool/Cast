#pragma once

#include <QString>
#include <QtGlobal>

enum class MediaKind {
    Monitor,
    Video,
    Image,
    Audio,
};

struct MediaSource {
    MediaKind kind = MediaKind::Monitor;
    QString path;
    QString mime;
    QString title;
    qint64 size = -1;

    bool isFile() const { return kind != MediaKind::Monitor && !path.isEmpty(); }
    bool isValidFile() const { return isFile() && size >= 0; }
};

struct HttpByteRange {
    qint64 start = 0;
    qint64 end = -1;
    bool specified = false;
    bool valid = true;
};

MediaSource mediaSourceFromPath(const QString &path);
QString upnpClassForMedia(MediaKind kind);
QString mediaKindKey(MediaKind kind);
HttpByteRange parseHttpByteRange(const QByteArray &headerValue, qint64 size);
