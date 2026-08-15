#include "session/dlnasession.h"

#include "session/gstencoder.h"

#include <QDebug>
#include <QHostAddress>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpSocket>

namespace {

constexpr auto kAvTransport = "urn:schemas-upnp-org:service:AVTransport:1";
constexpr auto kConnectionManager = "urn:schemas-upnp-org:service:ConnectionManager:1";

QByteArray httpHeaders(const DlnaProfile &profile)
{
    QByteArray out;
    out += "HTTP/1.1 200 OK\r\n";
    out += "Content-Type: ";
    out += profile.mime.toUtf8();
    out += "\r\n";
    out += "Server: ot-cast/0.1\r\n";
    out += "transferMode.dlna.org: Streaming\r\n";
    out += "contentFeatures.dlna.org: ";
    out += profile.contentFeatures.toUtf8();
    out += "\r\n";
    out += "EXT:\r\n";
    out += "realTimeInfo.dlna.org: DLNA.ORG_TLAG=*\r\n";
    out += "Cache-Control: no-cache\r\n";
    out += "Connection: close\r\n";
    out += "\r\n";
    return out;
}

bool pathIsStream(const QByteArray &target)
{
    return target == "/" || target == "/cast.ts" || target == "/stream.ts";
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
                        GstEncoder *encoder)
{
    stop();
    m_sink = sink;
    m_source = source;
    m_encoder = encoder;
    m_audioEnabled = audioEnabled;
    m_stopping = false;
    m_profile = pickDlnaProfile(sink.protocolInfo);

    if (sink.protocol != CastProtocol::Dlna || !sink.avTransportUrl.isValid()) {
        fail(tr("This display is not a DLNA renderer."));
        return;
    }
    if (!encoder) {
        fail(tr("Encoder is missing."));
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

    m_streamUrl = QUrl(QStringLiteral("http://%1:%2/cast.ts")
                           .arg(local.toString())
                           .arg(m_server.serverPort()));
    m_running = true;
    qInfo() << "DLNA HTTP listening" << m_streamUrl;
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
    if (m_encoder)
        m_encoder->stop();
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
                       m_sink.protocolInfo = sinkInfo;
                       m_profile = pickDlnaProfile(sinkInfo);
                       qInfo() << "DLNA ProtocolInfo" << m_profile.protocolInfo;
                   }
               } else {
                   qWarning() << "GetProtocolInfo failed, using default MPEG-TS profile";
               }
               setUriAndPlay();
           });
}

void DlnaSession::setUriAndPlay()
{
    const QString didl = buildDidlLite(m_streamUrl, m_profile, QStringLiteral("Cast"));
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
                   fail(tr("The TV rejected SetAVTransportURI. It may not play a live MPEG-TS stream."));
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

    if (method == "HEAD") {
        writeHeaders(socket, false);
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
    writeHeaders(socket, true);
    attachEncoder(socket);
}

void DlnaSession::writeHeaders(QTcpSocket *socket, bool withBodyHint)
{
    Q_UNUSED(withBodyHint);
    socket->write(httpHeaders(m_profile));
    socket->flush();
}

void DlnaSession::attachEncoder(QTcpSocket *socket)
{
    if (!m_encoder)
        return;
    Q_EMIT statusChanged(tr("Starting encoder for %1…").arg(m_sink.name));
    const WfdVideoMode video = dlnaVideoMode(m_source);
    const WfdAudioMode audio = dlnaAudioMode(m_audioEnabled);
    m_encoder->startMpegTsPipe(video, audio, m_source);
    QIODevice *pipe = m_encoder->tsPipe();
    if (!pipe)
        return;
    connect(pipe, &QIODevice::readyRead, socket, [this, socket, pipe]() {
        if (!m_client || m_client != socket)
            return;
        const QByteArray chunk = pipe->readAll();
        if (chunk.isEmpty())
            return;
        if (socket->state() != QAbstractSocket::ConnectedState)
            return;
        socket->write(chunk);
    }, Qt::UniqueConnection);
}

void DlnaSession::detachClient()
{
    if (m_encoder) {
        QIODevice *pipe = m_encoder->tsPipe();
        if (pipe)
            pipe->disconnect(this);
        if (m_client)
            pipe->disconnect(m_client);
    }
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