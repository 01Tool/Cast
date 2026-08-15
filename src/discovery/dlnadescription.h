#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QVector>

struct DlnaRendererDesc {
    QString udn;
    QString name;
    QUrl location;
    QUrl avTransport;
    QUrl connectionManager;
};

// Walk a device description (including embedded devices). One row per MediaRenderer
// that exposes AVTransport.
QVector<DlnaRendererDesc> parseMediaRenderers(const QByteArray &xml, const QUrl &location);