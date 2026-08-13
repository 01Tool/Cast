#include "session/wfdserver.h"

#include <QDebug>
#include <QHostAddress>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>

namespace {

constexpr quint16 kWfdPort = 7236;

QByteArray headerValue(const QByteArray &raw, const QByteArray &name)
{
    const QByteArray prefix = name + ":";
    for (const QByteArray &line : raw.split('\n')) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.toLower().startsWith(prefix.toLower()))
            return trimmed.mid(prefix.size()).trimmed();
    }
    return {};
}

int headerInt(const QByteArray &raw, const QByteArray &name, int fallback = 0)
{
    bool ok = false;
    const int value = headerValue(raw, name).toInt(&ok);
    return ok ? value : fallback;
}

} // namespace

WfdSession::WfdSession(QTcpSocket *socket, const QString &localIpv4, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_localIpv4(localIpv4)
{
    m_socket->setParent(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &WfdSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &WfdSession::onDisconnected);
    QTimer::singleShot(400, this, &WfdSession::sendSourceOptions);
}

void WfdSession::onReadyRead()
{
    m_buffer += m_socket->readAll();
    while (true) {
        const int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            return;
        const QByteArray header = m_buffer.left(headerEnd);
        const int contentLength = headerInt(header, "Content-Length");
        const int total = headerEnd + 4 + contentLength;
        if (m_buffer.size() < total)
            return;
        const QByteArray raw = m_buffer.left(total);
        m_buffer.remove(0, total);
        handleMessage(raw);
    }
}

void WfdSession::onDisconnected()
{
    Q_EMIT sessionClosed();
}

void WfdSession::sendSourceOptions()
{
    if (m_sentM1 || m_gotM2)
        return;
    m_sentM1 = true;
    QByteArray extra = "Require: org.wfa.wfd1.0\r\n";
    const QByteArray req = "OPTIONS * RTSP/1.0\r\n"
                           "CSeq: " + QByteArray::number(m_nextCseq++) + "\r\n"
                           + extra + "\r\n";
    m_socket->write(req);
    qInfo() << "WFD M1 OPTIONS sent";
}

void WfdSession::handleMessage(const QByteArray &raw)
{
    const int headerEnd = raw.indexOf("\r\n\r\n");
    const QByteArray header = raw.left(headerEnd);
    const QByteArray body = raw.mid(headerEnd + 4);
    const QList<QByteArray> lines = header.split('\n');
    if (lines.isEmpty())
        return;

    const QByteArray start = lines.first().trimmed();
    const int cseq = headerInt(header, "CSeq");
    if (start.startsWith("RTSP/1.0")) {
        handleResponse(cseq, body);
        return;
    }

    const QString method = QString::fromLatin1(start.split(' ').value(0));
    handleRequest(method, cseq, body, QString::fromLatin1(headerValue(header, "Transport")));
}

void WfdSession::handleRequest(const QString &method, int cseq, const QByteArray &body,
                              const QString &transport)
{
    Q_UNUSED(body);
    if (method == QLatin1String("OPTIONS")) {
        m_gotM2 = true;
        sendResponse(cseq,
                     "Public: org.wfa.wfd1.0, GET_PARAMETER, SET_PARAMETER, "
                     "SETUP, PLAY, PAUSE, TEARDOWN\r\n");
        Q_EMIT statusChanged(QStringLiteral("WFD OPTIONS, querying sink…"));
        sendGetParameter();
        return;
    }

    if (method == QLatin1String("GET_PARAMETER")) {
        const QByteArray reply =
            "wfd_video_formats: 00 00 01 01 00000081 00000000 00000000 00 0000 0000 00 none none\r\n"
            "wfd_audio_codecs: none\r\n"
            "wfd_client_rtp_ports: RTP/AVP/UDP;unicast 1028 0 mode=play\r\n";
        sendResponse(cseq, "Content-Type: text/parameters\r\n", reply);
        return;
    }

    if (method == QLatin1String("SET_PARAMETER"))
        return sendResponse(cseq);

    if (method == QLatin1String("SETUP")) {
        static const QRegularExpression portRe(QStringLiteral("client_port=(\\d+)"));
        const auto match = portRe.match(transport);
        if (match.hasMatch())
            m_rtpPort = static_cast<quint16>(match.captured(1).toUInt());
        const QByteArray extra =
            "Transport: RTP/AVP/UDP;unicast;client_port=" + QByteArray::number(m_rtpPort)
            + ";server_port=19000\r\n"
              "Session: 1;timeout=30\r\n";
        sendResponse(cseq, extra);
        Q_EMIT statusChanged(QStringLiteral("WFD SETUP, RTP port %1").arg(m_rtpPort));
        return;
    }

    if (method == QLatin1String("PLAY")) {
        sendResponse(cseq, "Session: 1\r\nRange: npt=now-\r\n");
        const QString ip = m_socket->peerAddress().toString();
        qInfo() << "WFD PLAY" << ip << m_rtpPort;
        Q_EMIT playRequested(ip, m_rtpPort);
        return;
    }

    if (method == QLatin1String("PAUSE") || method == QLatin1String("TEARDOWN")) {
        sendResponse(cseq, "Session: 1\r\n");
        if (method == QLatin1String("TEARDOWN"))
            m_socket->disconnectFromHost();
        return;
    }

    sendResponse(cseq);
}

void WfdSession::handleResponse(int cseq, const QByteArray &body)
{
    if (cseq == m_pendingGetCseq) {
        m_pendingGetCseq = -1;
        parseSinkParams(body);
        sendSetParameter();
        return;
    }
    if (cseq == m_pendingSetCseq) {
        m_pendingSetCseq = -1;
        sendTriggerSetup();
        return;
    }
    if (cseq == m_pendingTriggerCseq)
        m_pendingTriggerCseq = -1;
}

void WfdSession::sendResponse(int cseq, const QByteArray &extraHeaders, const QByteArray &body)
{
    QByteArray msg = "RTSP/1.0 200 OK\r\nCSeq: " + QByteArray::number(cseq) + "\r\n";
    msg += extraHeaders;
    if (!body.isEmpty())
        msg += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    msg += "\r\n";
    msg += body;
    m_socket->write(msg);
}

void WfdSession::sendRequest(const QByteArray &method, const QByteArray &body)
{
    const int cseq = m_nextCseq++;
    QByteArray msg = method + " rtsp://localhost/wfd1.0 RTSP/1.0\r\n"
                     "CSeq: " + QByteArray::number(cseq) + "\r\n";
    if (!body.isEmpty()) {
        msg += "Content-Type: text/parameters\r\n";
        msg += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    }
    msg += "\r\n";
    msg += body;
    m_socket->write(msg);
    if (method == "GET_PARAMETER")
        m_pendingGetCseq = cseq;
    else if (method == "SET_PARAMETER" && body.contains("wfd_trigger_method"))
        m_pendingTriggerCseq = cseq;
    else if (method == "SET_PARAMETER")
        m_pendingSetCseq = cseq;
}

void WfdSession::sendGetParameter()
{
    Q_EMIT statusChanged(QStringLiteral("WFD GET_PARAMETER…"));
    sendRequest("GET_PARAMETER",
                "wfd_video_formats\r\nwfd_audio_codecs\r\nwfd_client_rtp_ports\r\n");
}

void WfdSession::sendSetParameter()
{
    const QString uri = QStringLiteral("rtsp://%1:%2/wfd1.0/streamid=0")
                            .arg(m_localIpv4.isEmpty() ? QStringLiteral("127.0.0.1") : m_localIpv4)
                            .arg(kWfdPort);
    const QByteArray body =
        "wfd_video_formats: 00 00 01 01 00000081 00000000 00000000 00 0000 0000 00 none none\r\n"
        "wfd_audio_codecs: none\r\n"
        "wfd_presentation_URL: " + uri.toUtf8() + " none\r\n"
        "wfd_client_rtp_ports: RTP/AVP/UDP;unicast " + QByteArray::number(m_rtpPort ? m_rtpPort : 1028)
        + " 0 mode=play\r\n";
    Q_EMIT statusChanged(QStringLiteral("WFD SET_PARAMETER…"));
    sendRequest("SET_PARAMETER", body);
}

void WfdSession::sendTriggerSetup()
{
    sendRequest("SET_PARAMETER", "wfd_trigger_method: SETUP\r\n");
}

void WfdSession::parseSinkParams(const QByteArray &body)
{
    static const QRegularExpression portsRe(
        QStringLiteral("wfd_client_rtp_ports:[^\\n]*unicast\\s+(\\d+)"));
    const auto match = portsRe.match(QString::fromLatin1(body));
    if (match.hasMatch())
        m_rtpPort = static_cast<quint16>(match.captured(1).toUInt());
    qInfo() << "WFD sink RTP port" << m_rtpPort;
}

WfdServer::WfdServer(QObject *parent)
    : QObject(parent)
{
}

WfdServer::~WfdServer()
{
    stop();
}

bool WfdServer::listen(const QString &localIpv4)
{
    stop();
    m_localIpv4 = localIpv4;
    if (!m_server.listen(QHostAddress::Any, kWfdPort)) {
        Q_EMIT failed(QStringLiteral("Cannot listen on RTSP port %1: %2")
                          .arg(kWfdPort)
                          .arg(m_server.errorString()));
        return false;
    }
    connect(&m_server, &QTcpServer::newConnection, this, &WfdServer::onNewConnection);
    Q_EMIT statusChanged(QStringLiteral("Waiting for the display on port %1…").arg(kWfdPort));
    qInfo() << "WFD RTSP listening on" << kWfdPort << "local" << localIpv4;
    return true;
}

void WfdServer::stop()
{
    if (m_session) {
        m_session->deleteLater();
        m_session = nullptr;
    }
    m_server.close();
    m_server.disconnect(this);
}

void WfdServer::onNewConnection()
{
    QTcpSocket *socket = m_server.nextPendingConnection();
    if (!socket)
        return;
    if (m_session) {
        socket->disconnectFromHost();
        socket->deleteLater();
        return;
    }
    m_session = new WfdSession(socket, m_localIpv4, this);
    connect(m_session, &WfdSession::playRequested, this, &WfdServer::playRequested);
    connect(m_session, &WfdSession::statusChanged, this, &WfdServer::statusChanged);
    connect(m_session, &WfdSession::sessionClosed, this, [this]() {
        if (m_session) {
            m_session->deleteLater();
            m_session = nullptr;
        }
    });
    Q_EMIT statusChanged(QStringLiteral("Display connected, starting WFD handshake…"));
}
