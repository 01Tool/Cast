#include "discovery/dlnadescription.h"

#include <QXmlStreamReader>

namespace {

QString localName(const QXmlStreamReader &xml)
{
    return xml.name().toString();
}

QUrl resolveControlUrl(const QUrl &base, const QString &href)
{
    const QString trimmed = href.trimmed();
    if (trimmed.isEmpty())
        return {};
    const QUrl relative(trimmed);
    if (relative.isRelative())
        return base.resolved(relative);
    return relative;
}

void parseServiceList(QXmlStreamReader &xml, const QUrl &base, DlnaRendererDesc *out)
{
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement() && localName(xml) == QLatin1String("serviceList"))
            return;
        if (!xml.isStartElement() || localName(xml) != QLatin1String("service"))
            continue;

        QString type;
        QString control;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isEndElement() && localName(xml) == QLatin1String("service"))
                break;
            if (!xml.isStartElement())
                continue;
            const QString name = localName(xml);
            if (name == QLatin1String("serviceType"))
                type = xml.readElementText(QXmlStreamReader::SkipChildElements);
            else if (name == QLatin1String("controlURL"))
                control = xml.readElementText(QXmlStreamReader::SkipChildElements);
            else
                xml.skipCurrentElement();
        }
        const QUrl url = resolveControlUrl(base, control);
        if (type.contains(QLatin1String("AVTransport")))
            out->avTransport = url;
        else if (type.contains(QLatin1String("ConnectionManager")))
            out->connectionManager = url;
    }
}

void parseDevice(QXmlStreamReader &xml, const QUrl &base, const QUrl &location,
                 QVector<DlnaRendererDesc> *out);

void parseDeviceList(QXmlStreamReader &xml, const QUrl &base, const QUrl &location,
                     QVector<DlnaRendererDesc> *out)
{
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement() && localName(xml) == QLatin1String("deviceList"))
            return;
        if (xml.isStartElement() && localName(xml) == QLatin1String("device"))
            parseDevice(xml, base, location, out);
    }
}

void parseDevice(QXmlStreamReader &xml, const QUrl &base, const QUrl &location,
                 QVector<DlnaRendererDesc> *out)
{
    DlnaRendererDesc desc;
    desc.location = location;
    QString deviceType;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement() && localName(xml) == QLatin1String("device"))
            break;
        if (!xml.isStartElement())
            continue;
        const QString name = localName(xml);
        if (name == QLatin1String("deviceType"))
            deviceType = xml.readElementText(QXmlStreamReader::SkipChildElements);
        else if (name == QLatin1String("friendlyName"))
            desc.name = xml.readElementText(QXmlStreamReader::SkipChildElements).trimmed();
        else if (name == QLatin1String("UDN"))
            desc.udn = xml.readElementText(QXmlStreamReader::SkipChildElements).trimmed();
        else if (name == QLatin1String("serviceList"))
            parseServiceList(xml, base, &desc);
        else if (name == QLatin1String("deviceList"))
            parseDeviceList(xml, base, location, out);
        else
            xml.skipCurrentElement();
    }

    if (!deviceType.contains(QLatin1String("MediaRenderer")))
        return;
    if (!desc.avTransport.isValid())
        return;
    if (desc.udn.isEmpty())
        desc.udn = desc.avTransport.toString();
    if (desc.name.isEmpty())
        desc.name = desc.udn;
    out->append(desc);
}

} // namespace

QVector<DlnaRendererDesc> parseMediaRenderers(const QByteArray &xml, const QUrl &location)
{
    QVector<DlnaRendererDesc> result;
    QXmlStreamReader reader(xml);
    QUrl base = location;
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement())
            continue;
        const QString name = localName(reader);
        if (name == QLatin1String("URLBase")) {
            const QUrl declared(reader.readElementText(QXmlStreamReader::SkipChildElements).trimmed());
            if (declared.isValid() && !declared.host().isEmpty())
                base = declared;
        } else if (name == QLatin1String("device")) {
            parseDevice(reader, base, location, &result);
        }
    }
    return result;
}