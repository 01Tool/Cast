#include "discovery/dlnadescription.h"
#include "discovery/dlnassdp.h"
#include "session/dlnaprofile.h"

#include <QByteArray>
#include <cstdio>

static int g_failed = 0;

static void expectTrue(const char *name, bool ok)
{
    if (ok)
        return;
    std::fprintf(stderr, "FAIL %s\n", name);
    ++g_failed;
}

static void expectEq(const char *name, const QString &got, const QString &want)
{
    if (got == want)
        return;
    std::fprintf(stderr, "FAIL %s: got [%s] want [%s]\n", name, qPrintable(got), qPrintable(want));
    ++g_failed;
}

int main()
{
    const QByteArray msearch = buildSsdpMsearch(3);
    expectTrue("msearch verb", msearch.startsWith("M-SEARCH * HTTP/1.1"));
    expectTrue("msearch st", msearch.contains("MediaRenderer:1"));

    const SsdpResponse bad = parseSsdpDatagram("garbage");
    expectTrue("reject garbage", !bad.location.isValid());

    const SsdpResponse reply = parseSsdpDatagram(
        "HTTP/1.1 200 OK\r\n"
        "CACHE-CONTROL: max-age=1800\r\n"
        "LOCATION: http://192.168.1.20:55000/dmr/device.xml\r\n"
        "ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n"
        "USN: uuid:tv-1::urn:schemas-upnp-org:device:MediaRenderer:1\r\n"
        "\r\n");
    expectEq("reply host", reply.location.host(), QStringLiteral("192.168.1.20"));
    expectEq("reply usn", reply.usn, QStringLiteral("uuid:tv-1::urn:schemas-upnp-org:device:MediaRenderer:1"));

    const SsdpResponse notify = parseSsdpDatagram(
        "NOTIFY * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "Location: http://10.0.0.8:2869/upnphost/udhisapi.dll?content=uuid:abc\r\n"
        "NT: urn:schemas-upnp-org:device:MediaRenderer:1\r\n"
        "NTS: ssdp:alive\r\n"
        "\r\n");
    expectEq("notify host", notify.location.host(), QStringLiteral("10.0.0.8"));
    expectTrue("notify st from nt", notify.st.contains(QLatin1String("MediaRenderer")));

    const QByteArray desc =
        "<?xml version=\"1.0\"?>"
        "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
        "<URLBase>http://192.168.1.20:55000/</URLBase>"
        "<device>"
        "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
        "<friendlyName>Living Room</friendlyName>"
        "<UDN>uuid:root</UDN>"
        "<deviceList>"
        "<device>"
        "<deviceType>urn:schemas-upnp-org:device:MediaRenderer:1</deviceType>"
        "<friendlyName>Living Room TV</friendlyName>"
        "<UDN>uuid:tv-1</UDN>"
        "<serviceList>"
        "<service>"
        "<serviceType>urn:schemas-upnp-org:service:AVTransport:1</serviceType>"
        "<controlURL>/upnp/control/AVTransport</controlURL>"
        "</service>"
        "<service>"
        "<serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>"
        "<controlURL>/upnp/control/ConnectionManager</controlURL>"
        "</service>"
        "</serviceList>"
        "</device>"
        "</deviceList>"
        "</device></root>";
    const auto renderers =
        parseMediaRenderers(desc, QUrl(QStringLiteral("http://192.168.1.20:55000/dmr/device.xml")));
    expectTrue("one renderer", renderers.size() == 1);
    if (!renderers.isEmpty()) {
        expectEq("name", renderers.at(0).name, QStringLiteral("Living Room TV"));
        expectEq("udn", renderers.at(0).udn, QStringLiteral("uuid:tv-1"));
        expectEq("avt", renderers.at(0).avTransport.toString(),
                 QStringLiteral("http://192.168.1.20:55000/upnp/control/AVTransport"));
        expectEq("cm", renderers.at(0).connectionManager.toString(),
                 QStringLiteral("http://192.168.1.20:55000/upnp/control/ConnectionManager"));
    }

    expectEq("escape", xmlEscape(QStringLiteral("a<b>&\"'")),
             QStringLiteral("a&lt;b&gt;&amp;&quot;&apos;"));

    const QString sinkInfo =
        "http-get:*:audio/mpeg:*,http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_SD_EU_ISO;DLNA.ORG_OP=01";
    const DlnaProfile profile = pickDlnaProfile(sinkInfo);
    expectEq("mime", profile.mime, QStringLiteral("video/mpeg"));
    expectTrue("live op", profile.contentFeatures.contains(QLatin1String("DLNA.ORG_OP=00")));
    expectTrue("not seek op", !profile.contentFeatures.contains(QLatin1String("DLNA.ORG_OP=01")));

    const QString soap = parseConnectionManagerSink(
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
        "<s:Body><u:GetProtocolInfoResponse "
        "xmlns:u=\"urn:schemas-upnp-org:service:ConnectionManager:1\">"
        "<Sink>http-get:*:video/mpeg:*</Sink>"
        "</u:GetProtocolInfoResponse></s:Body></s:Envelope>");
    expectEq("sink parse", soap, QStringLiteral("http-get:*:video/mpeg:*"));

    DisplaySource source;
    source.width = 1920;
    source.height = 1080;
    const WfdVideoMode video = dlnaVideoMode(source);
    expectTrue("cap width", video.width <= 1280);
    expectTrue("cap height", video.height <= 720);
    expectTrue("even width", video.width % 2 == 0);
    expectTrue("30fps", video.fps == 30);

    QString summary;
    expectTrue("ts hint",
               classifyDlnaSink(sinkInfo, &summary) == DlnaMediaKind::LiveTsLikely);
    expectTrue("ts summary", summary.contains(QLatin1String("MPEG_TS")));

    expectTrue("file only",
               classifyDlnaSink(QStringLiteral(
                                    "http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_MP_SD_AAC_MULT5"),
                                &summary)
                   == DlnaMediaKind::FileOnlyLikely);
    expectTrue("mp4 summary", summary.contains(QLatin1String("AVC_MP4")));

    expectTrue("hls file-only",
               classifyDlnaSink(QStringLiteral("http-get:*:application/vnd.apple.mpegurl:*"),
                                nullptr)
                   == DlnaMediaKind::FileOnlyLikely);

    expectTrue("audio only is no-video",
               classifyDlnaSink(QStringLiteral("http-get:*:audio/mpeg:*"), nullptr)
                   == DlnaMediaKind::NoVideo);

    expectTrue("empty unknown", classifyDlnaSink(QString(), nullptr) == DlnaMediaKind::Unknown);

    expectTrue("mixed prefers ts",
               classifyDlnaSink(QStringLiteral(
                                    "http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_BL_CIF15_AAC,"
                                    "http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_SD_NA_ISO"),
                                nullptr)
                   == DlnaMediaKind::LiveTsLikely);

    SinkDevice classified;
    applyDlnaProtocolInfo(&classified, QStringLiteral("http-get:*:video/mpeg:*"));
    expectTrue("apply kind", classified.dlnaMedia == DlnaMediaKind::LiveTsLikely);
    expectEq("kind key", dlnaMediaKindKey(classified.dlnaMedia), QStringLiteral("live-ts-likely"));

    const WfdAudioMode silent = dlnaAudioMode(false);
    expectTrue("audio off", !silent.enabled());
    const WfdAudioMode aac = dlnaAudioMode(true);
    expectTrue("audio on", aac.enabled());

    const QByteArray envelope = buildSoapEnvelope(
        QStringLiteral("urn:schemas-upnp-org:service:AVTransport:1"),
        QStringLiteral("Play"), QStringLiteral("<InstanceID>0</InstanceID>"));
    expectTrue("soap action", envelope.contains("u:Play"));
    expectTrue("soap instance", envelope.contains("<InstanceID>0</InstanceID>"));

    if (g_failed) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failed);
        return 1;
    }
    std::puts("dlna-check ok");
    return 0;
}