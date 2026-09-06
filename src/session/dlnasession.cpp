#include "session/dlnasession.h"

#include "session/gstencoder.h"

#include <QDebug>
#include <QFileInfo>
#include <QHostAddress>
#include <QIODevice>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpSocket>

namespace {

constexpr auto kAvTransport = "urn:schemas-upnp-org:service:AVTransport:1";
constexpr auto kConnectionManager = "urn:schemas-upnp-org:service:ConnectionManager:1";

bool pathIsStream(const QByteArray &target)
{
    return target == "/" || target == "/cast.ts" || target == "/stream.ts"
        || target.startsWith("/cast.") || target == "/media" || target.startsWith("/media.");
}

} // namespace

DlnaSession::DlnaSession(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &DlnaSession::onNewConnection);
}

DlnaSession::~DlnaSession()
{
    stop();
}

bool DlnaSession::running() const
{
    return m_running;
}

QUrl DlnaSession::streamUrl() const
{
    return m_streamUrl;
}

void DlnaSession::start(const SinkDevice &sink, const DisplaySource &source, bool audioEnabled,
                        GstEncoder *encoder, const MediaSource &media)
{
    stop();
    m_sink = sink;
    m_source = source;
    m_media = media;
    m_encoder = encoder;
    m_audioEnabled = audioEnabled;
    m_stopping = false;
    if (m_media.isFile())
        m_profile = pickDlnaFileProfile(sink.protocolInfo, m_media.mime);
    else {
        m_profile = pickDlnaProfile(sink.protocolInfo);
        applyDlnaOutputMode(&m_profile, dlnaVideoMode(source));
    }

    if (sink.protocol != CastProtocol::Dlna || !sink.avTransportUrl.isValid()) {
        fail(tr("This display is not a DLNA renderer."));
        return;
    }
    if (!m_media.isFile() && !encoder) {
        fail(tr("Encoder is missing."));
        return;
    }
    if (m_media.isFile() && !m_media.isValidFile()) {
        fail(tr("The selected file is missing or unreadable."));
        return;
    }

    const QHostAddress peer(sink.address);
    const QHostAddress local = pickLocalIpv4(peer);
    if (local.isNull()) {
        fail(tr("No LAN IPv4 address the TV can reach. Stay on the same Wi-Fi as the renderer."));
        return;
    }

    if (!m_server.listen(QHostAddress::AnyIPv4, 0)) {
        fail(tr("Could not start the HTTP media server (%1).").arg(m_server.errorString()));
        return;
    }

    QString leaf = QStringLiteral("cast.ts");
    if (m_media.isFile()) {
        const QString suffix = QFileInfo(m_media.path).suffix().toLower();
        leaf = suffix.isEmpty() ? QStringLiteral("media") : QStringLiteral("cast.%1").arg(suffix);
    }
    m_streamUrl = QUrl(QStringLiteral("http://%1:%2/%3")
                           .arg(local.toString())
                           .arg(m_server.serverPort())
                           .arg(leaf));
    m_running = true;
    qInfo() << "DLNA HTTP listening" << m_streamUrl << (m_media.isFile() ? m_media.mime : "live-ts");
    Q_EMIT statusChanged(tr("Offering stream at %1").arg(m_streamUrl.toString()));
    beginControl();
}

void DlnaSession::stop()
{
    if (!m_running && !m_server.isListening() && !m_client)
        return;
    m_stopping = true;
    soapStop();
    detachClient();
    if (m_encoder) {
        if (QIODevice *pipe = m_encoder->tsPipe())
            disconnect(pipe, &QIODevice::readyRead, this, &DlnaSession::pumpTs);
        m_encoder->stop();
    }
    m_server.close();
    m_streamUrl.clear();
    m_running = false;
    m_stopping = false;
}

void DlnaSession::fail(const QString &message)
{
    if (m_stopping)
        return;
    qWarning() << "DLNA session" << message;
    stop();
    Q_EMIT failed(message);
}

void DlnaSession::beginControl()
{
    if (m_sink.connectionManagerUrl.isValid())
        queryProtocolInfo();
    else
        setUriAndPlay();
}

void DlnaSession::queryProtocolInfo()
{
    Q_EMIT statusChanged(tr("Asking %1 which video types it accepts…").arg(m_sink.name));
    invoke(m_sink.connectionManagerUrl, QString::fromLatin1(kConnectionManager),
           QStringLiteral("GetProtocolInfo"), QString(),
           [this](bool ok, const QByteArray &body) {
               if (!m_running)
                   return;
               if (ok) {
                   const QString sinkInfo = parseConnectionManagerSink(body);
                   if (!sinkInfo.isEmpty()) {
                       applyDlnaProtocolInfo(&m_sink, sinkInfo);
                       if (m_media.isFile())
                           m_profile = pickDlnaFileProfile(sinkInfo, m_media.mime);
                       else {
                           m_profile = pickDlnaProfile(sinkInfo);
                           applyDlnaOutputMode(&m_profile, dlnaVideoMode(m_source));
                       }
                       qInfo() << "DLNA ProtocolInfo" << m_profile.protocolInfo
                               << dlnaMediaKindKey(m_sink.dlnaMedia)
                               << m_sink.dlnaMediaSummary;
                   }
               } else {
                   qWarning() << "GetProtocolInfo failed, using default profile";
               }
               setUriAndPlay();
           });
}

void DlnaSession::setUriAndPlay()
{
    if (!m_media.isFile())
        applyDlnaOutputMode(&m_profile, dlnaVideoMode(m_source));
    if (!m_media.isFile() && m_sink.dlnaMedia == DlnaMediaKind::FileOnlyLikely) {
        Q_EMIT statusChanged(tr("%1 looks file-only (%2). Live MPEG-TS may fail.")
                                 .arg(m_sink.name, m_sink.dlnaMediaSummary));
    }
    const QString title = m_media.isFile() ? m_media.title : QStringLiteral("Cast");
    const QString didl = buildDidlLite(m_streamUrl, m_profile, title,
                                       m_media.isFile() ? upnpClassForMedia(m_media.kind)
                                                        : QString());
    const QString setUri = QStringLiteral(
                               "<InstanceID>0</InstanceID>"
                               "<CurrentURI>%1</CurrentURI>"
                               "<CurrentURIMetaData>%2</CurrentURIMetaData>")
                               .arg(xmlEscape(m_streamUrl.toString()), xmlEscape(didl));
    Q_EMIT statusChanged(tr("Sending the stream URL to %1…").arg(m_sink.name));
    invoke(m_sink.avTransportUrl, QString::fromLatin1(kAvTransport),
           QStringLiteral("SetAVTransportURI"), setUri,
           [this](bool ok, const QByteArray &body) {
               if (!m_running)
                   return;
               if (!ok) {
                   if (m_media.isFile()) {
                       fail(tr("The TV rejected SetAVTransportURI for this file (%1).")
                                .arg(m_profile.mime));
                   } else if (m_sink.dlnaMedia == DlnaMediaKind::FileOnlyLikely) {
                       fail(tr("The TV rejected SetAVTransportURI. It looks file-only (%1), "
                               "not a live MPEG-TS renderer.")
                                .arg(m_sink.dlnaMediaSummary));
                   } else {
                       fail(tr("The TV rejected SetAVTransportURI. It may not play a live MPEG-TS stream."));
                   }
                   qWarning() << body;
                   return;
               }
               invoke(m_sink.avTransportUrl, QString::fromLatin1(kAvTransport),
                      QStringLiteral("Play"),
                      QStringLiteral("<InstanceID>0</InstanceID><Speed>1</Speed>"),
                      [this](bool playOk, const QByteArray &playBody) {
                          if (!m_running)
                              return;
                          if (!playOk) {
                              fail(tr("The TV rejected Play."));
                              qWarning() << playBody;
                              return;
                          }
                          Q_EMIT statusChanged(
                              tr("Waiting for %1 to pull the HTTP stream…").arg(m_sink.name));
                          Q_EMIT playIssued();
                          if (!m_media.isFile())
                              startLiveEncoder();
                      });
           });
}

void DlnaSession::invoke(const QUrl &controlUrl, const QString &serviceType,
                         const QString &action, const QString &innerXml,
                         const std::function<void(bool, QByteArray)> &done)
{
    QNetworkRequest request(controlUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("text/xml; charset=\"utf-8\""));
    request.setRawHeader("SOAPAction",
                         QStringLiteral("\"%1#%2\"").arg(serviceType, action).toUtf8());
    request.setTransferTimeout(8000);
    QNetworkReply *reply = m_nam.post(request, buildSoapEnvelope(serviceType, action, innerXml));
    connect(reply, &QNetworkReply::finished, this, [this, reply, action, done]() {
        reply->deleteLater();
        const QByteArray body = reply->readAll();
        const bool ok = reply->error() == QNetworkReply::NoError
            && !body.contains("s:Fault") && !body.contains("UPnPError");
        if (!ok)
            qWarning() << "SOAP" << action << reply->errorString() << body.left(400);
        done(ok, body);
    });
}

void DlnaSession::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket *socket = m_server.nextPendingConnection();
        if (!socket)
            return;
        socket->setParent(this);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            handleClient(socket);
        });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            socket->deleteLater();
            if (m_client == socket)
                detachClient();
        });
    }
}

void DlnaSession::handleClient(QTcpSocket *socket)
{
    const QByteArray peek = socket->peek(8192);
    int headerEnd = peek.indexOf("\r\n\r\n");
    int headerSkip = 4;
    int lineEnd = peek.indexOf("\r\n");
    if (headerEnd < 0) {
        headerEnd = peek.indexOf("\n\n");
        headerSkip = 2;
        lineEnd = peek.indexOf('\n');
    }
    if (headerEnd < 0) {
        if (peek.size() > 8192)
            socket->disconnectFromHost();
        return;
    }
    socket->read(headerEnd + headerSkip);
    const QByteArray requestLine = peek.left(lineEnd);
    const QList<QByteArray> parts = requestLine.split(' ');
    if (parts.size() < 2) {
        socket->disconnectFromHost();
        return;
    }
    const QByteArray method = parts.at(0).toUpper();
    QByteArray target = parts.at(1);
    const int qpos = target.indexOf('?');
    if (qpos >= 0)
        target = target.left(qpos);
    if (!pathIsStream(target)) {
        socket->write("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
        socket->disconnectFromHost();
        return;
    }

    const QByteArray headerBlock = peek.left(headerEnd);
    const HttpByteRange range = m_media.isFile()
        ? parseHttpByteRange(requestHeaderValue(headerBlock, "Range"), m_media.size)
        : HttpByteRange{};
    if (m_media.isFile() && range.specified && !range.valid) {
        socket->write("HTTP/1.1 416 Range Not Satisfiable\r\nConnection: close\r\n\r\n");
        socket->disconnectFromHost();
        return;
    }

    qInfo() << "DLNA HTTP" << method << target << "from" << socket->peerAddress().toString()
            << (m_media.isFile() ? "file" : "live-ts");

    if (method == "HEAD") {
        writeHeaders(socket, false, m_media.isFile() ? m_media.size : -1, range);
        socket->disconnectFromHost();
        return;
    }
    if (method != "GET") {
        socket->write("HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
        socket->disconnectFromHost();
        return;
    }

    if (m_client && m_client != socket) {
        m_client->disconnectFromHost();
        detachClient();
    }
    m_client = socket;
    if (m_media.isFile())
        serveFile(socket, range);
    else {
        writeHeaders(socket, true);
        attachEncoder(socket);
    }
}

void DlnaSession::writeHeaders(QTcpSocket *socket, bool withBodyHint, qint64 contentLength,
                               const HttpByteRange &range)
{
    Q_UNUSED(withBodyHint);
    const bool partial = m_media.isFile() && range.specified && range.valid;
    QByteArray out;
    out += partial ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n";
    out += "Content-Type: ";
    out += m_profile.mime.toUtf8();
    out += "\r\n";
    out += "Server: ot-cast/0.2\r\n";
    out += m_media.kind == MediaKind::Image ? "transferMode.dlna.org: Interactive\r\n"
                                            : "transferMode.dlna.org: Streaming\r\n";
    out += "contentFeatures.dlna.org: ";
    out += m_profile.contentFeatures.toUtf8();
    out += "\r\n";
    out += "EXT:\r\n";
    if (!m_media.isFile())
        out += "realTimeInfo.dlna.org: DLNA.ORG_TLAG=*\r\n";
    if (m_media.isFile()) {
        out += "Accept-Ranges: bytes\r\n";
        qint64 length = contentLength;
        if (partial)
            length = range.end - range.start + 1;
        if (length >= 0)
            out += "Content-Length: " + QByteArray::number(length) + "\r\n";
        if (partial && m_media.size >= 0) {
            out += "Content-Range: bytes " + QByteArray::number(range.start) + "-"
                + QByteArray::number(range.end) + "/" + QByteArray::number(m_media.size)
                + "\r\n";
        }
    }
    out += "Cache-Control: no-cache\r\n";
    out += "Connection: close\r\n";
    out += "\r\n";
    socket->write(out);
    socket->flush();
}

QByteArray DlnaSession::requestHeaderValue(const QByteArray &headerBlock, const QByteArray &name) const
{
    const QByteArray prefix = name + ":";
    for (QByteArray line : headerBlock.split('\n')) {
        line = line.trimmed();
        if (line.toLower().startsWith(prefix.toLower()))
            return line.mid(prefix.size()).trimmed();
    }
    return {};
}

void DlnaSession::serveFile(QTcpSocket *socket, const HttpByteRange &range)
{
    m_file = std::make_unique<QFile>(m_media.path);
    if (!m_file->open(QIODevice::ReadOnly)) {
        socket->write("HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        socket->disconnectFromHost();
        m_file.reset();
        return;
    }
    HttpByteRange used = range;
    if (!used.specified) {
        used.start = 0;
        used.end = m_media.size > 0 ? m_media.size - 1 : -1;
        used.valid = true;
    }
    if (used.start > 0 && !m_file->seek(used.start)) {
        socket->write("HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        socket->disconnectFromHost();
        m_file.reset();
        return;
    }
    m_fileEnd = used.end;
    writeHeaders(socket, true, m_media.size, used);
    Q_EMIT statusChanged(tr("%1 is fetching the file.").arg(m_sink.name));
    connect(socket, &QTcpSocket::bytesWritten, this, &DlnaSession::onClientBytesWritten,
            Qt::UniqueConnection);
    writeFileChunk(socket);
}

void DlnaSession::writeFileChunk(QTcpSocket *socket)
{
    if (!m_file || m_client != socket || socket->state() != QAbstractSocket::ConnectedState)
        return;
    if (socket->bytesToWrite() > 256 * 1024)
        return;
    if (m_fileEnd >= 0 && m_file->pos() > m_fileEnd) {
        socket->disconnectFromHost();
        return;
    }
    qint64 chunk = 64 * 1024;
    if (m_fileEnd >= 0)
        chunk = qMin(chunk, m_fileEnd - m_file->pos() + 1);
    if (chunk <= 0) {
        socket->disconnectFromHost();
        return;
    }
    const QByteArray data = m_file->read(chunk);
    if (data.isEmpty()) {
        socket->disconnectFromHost();
        return;
    }
    socket->write(data);
}

void DlnaSession::startLiveEncoder()
{
    if (!m_encoder)
        return;
    if (!m_encoder->running()) {
        Q_EMIT statusChanged(tr("Starting encoder for %1…").arg(m_sink.name));
        m_encoder->startMpegTsPipe(dlnaVideoMode(m_source), dlnaAudioMode(m_audioEnabled),
                                   m_source, m_media);
    }
    bindTsPipe();
}

void DlnaSession::bindTsPipe()
{
    QIODevice *pipe = m_encoder ? m_encoder->tsPipe() : nullptr;
    if (!pipe)
        return;
    // Qt::UniqueConnection is a no-op for lambdas (Qt 6.5+). Use a member slot.
    connect(pipe, &QIODevice::readyRead, this, &DlnaSession::pumpTs, Qt::UniqueConnection);
}

void DlnaSession::attachEncoder(QTcpSocket *socket)
{
    startLiveEncoder();
    connect(socket, &QTcpSocket::bytesWritten, this, &DlnaSession::onClientBytesWritten,
            Qt::UniqueConnection);
    pumpTs();
}

void DlnaSession::pumpTs()
{
    if (!m_encoder || !m_client)
        return;
    QIODevice *pipe = m_encoder->tsPipe();
    if (!pipe)
        return;
    if (m_client->state() != QAbstractSocket::ConnectedState)
        return;
    if (m_client->bytesToWrite() > 256 * 1024)
        return;
    const QByteArray chunk = pipe->readAll();
    if (chunk.isEmpty())
        return;
    m_client->write(chunk);
}

void DlnaSession::onClientBytesWritten()
{
    if (!m_client)
        return;
    if (m_media.isFile())
        writeFileChunk(m_client);
    else
        pumpTs();
}

void DlnaSession::detachClient()
{
    if (m_client)
        disconnect(m_client, &QTcpSocket::bytesWritten, this, &DlnaSession::onClientBytesWritten);
    m_file.reset();
    m_fileEnd = -1;
    m_client = nullptr;
}

void DlnaSession::soapStop()
{
    if (!m_sink.avTransportUrl.isValid())
        return;
    QNetworkRequest request(m_sink.avTransportUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("text/xml; charset=\"utf-8\""));
    request.setRawHeader("SOAPAction",
                         QByteArray("\"urn:schemas-upnp-org:service:AVTransport:1#Stop\""));
    request.setTransferTimeout(2000);
    QNetworkReply *reply = m_nam.post(
        request,
        buildSoapEnvelope(QString::fromLatin1(kAvTransport), QStringLiteral("Stop"),
                          QStringLiteral("<InstanceID>0</InstanceID>")));
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}