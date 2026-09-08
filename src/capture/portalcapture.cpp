#include "capture/portalcapture.h"

#include "capture/screencastportal.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QDBusVariant>
#include <QDebug>
#include <QEventLoop>
#include <QMetaType>
#include <QRandomGenerator>
#include <QTimer>
#include <QVariantMap>

#include <cstdio>
#include <unistd.h>

namespace {

void skipDbusValue(QDBusArgument &arg)
{
    switch (arg.currentType()) {
    case QDBusArgument::BasicType:
    case QDBusArgument::VariantType: {
        QVariant unused;
        arg >> unused;
        break;
    }
    case QDBusArgument::ArrayType:
        arg.beginArray();
        for (int n = 0; !arg.atEnd() && n < 64; ++n)
            skipDbusValue(arg);
        arg.endArray();
        break;
    case QDBusArgument::StructureType:
        arg.beginStructure();
        for (int n = 0; !arg.atEnd() && n < 16; ++n)
            skipDbusValue(arg);
        arg.endStructure();
        break;
    case QDBusArgument::MapType:
        arg.beginMap();
        for (int n = 0; !arg.atEnd() && n < 64; ++n) {
            arg.beginMapEntry();
            skipDbusValue(arg);
            skipDbusValue(arg);
            arg.endMapEntry();
        }
        arg.endMap();
        break;
    default:
        break;
    }
}

bool parseStreamsArg(QDBusArgument &arg, uint *nodeId, int *width, int *height)
{
    if (!nodeId || arg.currentType() != QDBusArgument::ArrayType)
        return false;
    arg.beginArray();
    if (arg.atEnd() || arg.currentType() != QDBusArgument::StructureType) {
        arg.endArray();
        return false;
    }
    arg.beginStructure();
    arg >> *nodeId;
    // Optional props a{sv}. Do not walk nested (ii) size/position with
    // QVariant — that path does QDBusArgument::operator<< on a read-only
    // argument and never returns on this portal.
    if (arg.currentType() == QDBusArgument::MapType)
        skipDbusValue(arg);
    arg.endStructure();
    while (!arg.atEnd())
        skipDbusValue(arg);
    arg.endArray();
    Q_UNUSED(width);
    Q_UNUSED(height);
    return *nodeId != 0;
}

bool fillPipewireFromResults(QVariantMap *results)
{
    if (!results)
        return false;
    if (results->value(QStringLiteral("pipewire_node")).toUInt() != 0)
        return true;
    QVariant raw = results->value(QStringLiteral("streams"));
    if (raw.canConvert<QDBusVariant>())
        raw = qvariant_cast<QDBusVariant>(raw).variant();
    if (!raw.canConvert<QDBusArgument>())
        return false;
    QDBusArgument arg = raw.value<QDBusArgument>();
    uint node = 0;
    int w = 0;
    int h = 0;
    if (!parseStreamsArg(arg, &node, &w, &h) || node == 0)
        return false;
    results->insert(QStringLiteral("pipewire_node"), node);
    results->insert(QStringLiteral("stream_width"), w);
    results->insert(QStringLiteral("stream_height"), h);
    return true;
}

void decodePortalResults(const QVariant &raw, QVariantMap *results)
{
    if (!results)
        return;
    results->clear();
    if (raw.canConvert<QVariantMap>()) {
        *results = raw.toMap();
        fillPipewireFromResults(results);
        return;
    }
    if (!raw.canConvert<QDBusArgument>())
        return;
    QDBusArgument arg = raw.value<QDBusArgument>();
    if (arg.currentType() != QDBusArgument::MapType)
        return;
    arg.beginMap();
    while (!arg.atEnd()) {
        QString key;
        arg.beginMapEntry();
        arg >> key;
        if (key == QLatin1String("session_handle")) {
            QVariant value;
            arg >> value;
            if (value.canConvert<QDBusVariant>())
                value = qvariant_cast<QDBusVariant>(value).variant();
            if (value.canConvert<QDBusObjectPath>())
                results->insert(key, value);
            else
                results->insert(key, QVariant::fromValue(QDBusObjectPath(value.toString())));
        } else if (key == QLatin1String("streams")) {
            uint node = 0;
            int w = 0;
            int h = 0;
            if (parseStreamsArg(arg, &node, &w, &h)) {
                results->insert(QStringLiteral("pipewire_node"), node);
                results->insert(QStringLiteral("stream_width"), w);
                results->insert(QStringLiteral("stream_height"), h);
            }
        } else if (arg.currentType() == QDBusArgument::BasicType
                   || arg.currentType() == QDBusArgument::VariantType) {
            QVariant value;
            arg >> value;
            results->insert(key, value);
        } else {
            skipDbusValue(arg);
        }
        arg.endMapEntry();
    }
    arg.endMap();
}

} // namespace

class PortalRequestWaiter : public QObject
{
    Q_OBJECT
public:
    uint code = 2;
    QVariantMap results;
    bool got = false;

public Q_SLOTS:
    void onResponse(uint response, const QVariantMap &values)
    {
        code = response;
        results = values;
        fillPipewireFromResults(&results);
        got = true;
        emit finished();
    }

Q_SIGNALS:
    void finished();
};

namespace {

QString makeToken()
{
    return QStringLiteral("cast%1").arg(QRandomGenerator::global()->generate(), 8, 16,
                                        QLatin1Char('0'));
}

} // namespace

PortalCapture::PortalCapture(QObject *parent)
    : QObject(parent)
{
    m_startTimer.setSingleShot(true);
    connect(&m_startTimer, &QTimer::timeout, this, &PortalCapture::onStartTimeout);
}

QString PortalCapture::name() const
{
    return QStringLiteral("PortalCapture");
}

int PortalCapture::pipewireFd() const
{
    return m_pipewireFd;
}

uint PortalCapture::pipewireNode() const
{
    return m_pipewireNode;
}

int PortalCapture::streamWidth() const
{
    return m_streamWidth;
}

int PortalCapture::streamHeight() const
{
    return m_streamHeight;
}

QString PortalCapture::lastError() const
{
    return m_lastError;
}

void PortalCapture::stop()
{
    m_startTimer.stop();
    unwatchStartResponse();
    m_starting = false;
    closeSession();
}

void PortalCapture::closeSession()
{
    if (m_pipewireFd >= 0) {
        ::close(m_pipewireFd);
        m_pipewireFd = -1;
    }
    m_pipewireNode = 0;
    m_streamWidth = 0;
    m_streamHeight = 0;
    if (m_session.path().isEmpty())
        return;
    auto bus = QDBusConnection::sessionBus();
    QDBusMessage close = QDBusMessage::createMethodCall(
        QString::fromLatin1(ScreenCastPortal::desktopService), m_session.path(),
        QString::fromLatin1(ScreenCastPortal::sessionInterface), QStringLiteral("Close"));
    bus.call(close, QDBus::Block, 2000);
    m_session = QDBusObjectPath();
}

uint PortalCapture::readPortalUintProperty(const QString &name) const
{
    auto bus = QDBusConnection::sessionBus();
    QDBusMessage get = QDBusMessage::createMethodCall(
        QString::fromLatin1(ScreenCastPortal::desktopService),
        QString::fromLatin1(ScreenCastPortal::desktopPath),
        QString::fromLatin1(ScreenCastPortal::propertiesInterface), QStringLiteral("Get"));
    get << QString::fromLatin1(ScreenCastPortal::screenCastInterface) << name;
    const QDBusMessage reply = bus.call(get, QDBus::Block, 3000);
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty())
        return 0;
    const QVariant value = reply.arguments().constFirst();
    if (value.canConvert<QDBusVariant>())
        return qvariant_cast<QDBusVariant>(value).variant().toUInt();
    return value.toUInt();
}

uint PortalCapture::availableSourceTypes() const
{
    return readPortalUintProperty(QStringLiteral("AvailableSourceTypes"));
}

uint PortalCapture::availableCursorModes() const
{
    return readPortalUintProperty(QStringLiteral("AvailableCursorModes"));
}

bool PortalCapture::start(const DisplaySource &source)
{
    Q_UNUSED(source);
    stop();
    m_lastError.clear();

    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        m_lastError = tr("No session bus; cannot open the ScreenCast portal.");
        return false;
    }

    const uint types = availableSourceTypes();
    const uint cursorModes = availableCursorModes();
    qInfo() << "ScreenCast AvailableSourceTypes" << types << "AvailableCursorModes" << cursorModes;
    if (types == 0) {
        m_lastError = tr("Screen capture is unavailable on this session. "
                         "xdg-desktop-portal ScreenCast has no sources (needs a Treeland / "
                         "Wayland portal backend).");
        qWarning() << m_lastError;
        return false;
    }

    if (!createSession())
        return false;
    if (!selectSources(cursorModes)) {
        closeSession();
        return false;
    }
    if (!beginStartSession()) {
        closeSession();
        return false;
    }
    return true;
}

bool PortalCapture::callRequest(const QString &method, const QVariantList &args, int timeoutMs,
                                QVariantMap *results, const QVariantMap &extraOptions)
{
    auto bus = QDBusConnection::sessionBus();
    const QString token = makeToken();
    const QString predicted = ScreenCastPortal::objectPath(
        QStringLiteral("request"), bus.baseService(), token);

    PortalRequestWaiter waiter;
    if (!bus.connect(QString::fromLatin1(ScreenCastPortal::desktopService), predicted,
                     QString::fromLatin1(ScreenCastPortal::requestInterface),
                     QStringLiteral("Response"), &waiter,
                     SLOT(onResponse(uint,QVariantMap)))) {
        m_lastError = tr("Could not listen for the ScreenCast portal reply.");
        return false;
    }

    QVariantMap options = extraOptions;
    options.insert(QStringLiteral("handle_token"), token);

    QDBusMessage msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(ScreenCastPortal::desktopService),
        QString::fromLatin1(ScreenCastPortal::desktopPath),
        QString::fromLatin1(ScreenCastPortal::screenCastInterface), method);
    QVariantList callArgs = args;
    if (method == QLatin1String("CreateSession")) {
        options.insert(QStringLiteral("session_handle_token"), makeToken());
        callArgs << QVariant::fromValue(options);
    } else {
        callArgs << QVariant::fromValue(options);
    }
    msg.setArguments(callArgs);

    const QDBusMessage reply = bus.call(msg, QDBus::BlockWithGui, 15000);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        m_lastError = tr("ScreenCast %1 failed: %2").arg(method, reply.errorMessage());
        return false;
    }
    if (!reply.arguments().isEmpty()) {
        const QDBusObjectPath actual = qvariant_cast<QDBusObjectPath>(reply.arguments().constFirst());
        if (!actual.path().isEmpty() && actual.path() != predicted) {
            bus.connect(QString::fromLatin1(ScreenCastPortal::desktopService), actual.path(),
                        QString::fromLatin1(ScreenCastPortal::requestInterface),
                        QStringLiteral("Response"), &waiter,
                        SLOT(onResponse(uint,QVariantMap)));
        }
    }

    QEventLoop loop;
    QObject::connect(&waiter, &PortalRequestWaiter::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    if (!waiter.got)
        loop.exec();

    if (!waiter.got) {
        m_lastError = method == QLatin1String("Start")
            ? tr("Timed out waiting to choose a screen.")
            : tr("The ScreenCast portal did not reply in time.");
        return false;
    }
    if (waiter.code == 1) {
        m_lastError = tr("Screen share was cancelled.");
        return false;
    }
    if (waiter.code != 0) {
        m_lastError = tr("The ScreenCast portal refused the session.");
        return false;
    }
    qInfo() << "ScreenCast" << method << "response keys" << waiter.results.keys();
    if (results)
        *results = waiter.results;
    return true;
}

bool PortalCapture::createSession()
{
    QVariantMap results;
    if (!callRequest(QStringLiteral("CreateSession"), {}, 15000, &results))
        return false;
    const QVariant handle = results.value(QStringLiteral("session_handle"));
    if (handle.canConvert<QDBusObjectPath>())
        m_session = qvariant_cast<QDBusObjectPath>(handle);
    else if (handle.typeId() == QMetaType::QString)
        m_session = QDBusObjectPath(handle.toString());
    if (m_session.path().isEmpty()) {
        m_lastError = tr("ScreenCast portal returned no session.");
        return false;
    }
    return true;
}

bool PortalCapture::selectSources(uint cursorModes)
{
    QVariantMap extra;
    extra.insert(QStringLiteral("types"), ScreenCastPortal::monitorSource);
    extra.insert(QStringLiteral("multiple"), false);
    if (cursorModes & ScreenCastPortal::cursorEmbedded) {
        extra.insert(QString::fromLatin1(ScreenCastPortal::cursorModeOption),
                     ScreenCastPortal::cursorEmbedded);
    }
    return callRequest(QStringLiteral("SelectSources"), {QVariant::fromValue(m_session)}, 15000,
                       nullptr, extra);
}

bool PortalCapture::watchResponse(const QString &path, const char *slot)
{
    auto bus = QDBusConnection::sessionBus();
    const QString svc = QString::fromLatin1(ScreenCastPortal::desktopService);
    const QString iface = QString::fromLatin1(ScreenCastPortal::requestInterface);
    const QString name = QStringLiteral("Response");
    // Never pass signature "ua{sv}" for Start: Qt then demarshals streams
    // a(ua{sv}) on the GUI thread and the window hangs after Allow.
    if (bus.connect(svc, path, iface, name, this, slot)) {
        qInfo() << "ScreenCast watching Response" << path << slot;
        return true;
    }
    qWarning() << "ScreenCast Response connect failed" << path << slot;
    return false;
}

bool PortalCapture::beginStartSession()
{
    auto bus = QDBusConnection::sessionBus();
    const QString token = makeToken();
    const QString predicted = ScreenCastPortal::objectPath(
        QStringLiteral("request"), bus.baseService(), token);
    unwatchStartResponse();
    if (!watchResponse(predicted, SLOT(onStartMessage(QDBusMessage)))) {
        m_lastError = tr("Could not listen for the ScreenCast portal reply.");
        return false;
    }
    m_startRequestPath = predicted;

    QVariantMap options;
    options.insert(QStringLiteral("handle_token"), token);
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(ScreenCastPortal::desktopService),
        QString::fromLatin1(ScreenCastPortal::desktopPath),
        QString::fromLatin1(ScreenCastPortal::screenCastInterface), QStringLiteral("Start"));
    msg << QVariant::fromValue(m_session) << QString() << QVariant::fromValue(options);
    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &PortalCapture::onStartInvoked);

    m_starting = true;
    m_startTimer.start(180000);
    qInfo() << "ScreenCast Start sent, waiting for the monitor picker" << predicted;
    std::fprintf(stderr, "ot-cast: ScreenCast Start async %s\n", qPrintable(predicted));
    std::fflush(stderr);
    return true;
}

void PortalCapture::onStartInvoked(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        failStart(tr("ScreenCast Start failed: %1").arg(reply.error().message()));
        return;
    }
    const QString actual = reply.value().path();
    qInfo() << "ScreenCast Start handle" << actual;
    std::fprintf(stderr, "ot-cast: ScreenCast Start handle %s\n", qPrintable(actual));
    std::fflush(stderr);
    if (!actual.isEmpty() && actual != m_startRequestPath) {
        watchResponse(actual, SLOT(onStartMessage(QDBusMessage)));
        m_startRequestPath = actual;
    }
}

void PortalCapture::unwatchStartResponse()
{
    if (m_startRequestPath.isEmpty())
        return;
    auto bus = QDBusConnection::sessionBus();
    bus.disconnect(QString::fromLatin1(ScreenCastPortal::desktopService), m_startRequestPath,
                   QString::fromLatin1(ScreenCastPortal::requestInterface),
                   QStringLiteral("Response"), this, SLOT(onStartMessage(QDBusMessage)));
    m_startRequestPath.clear();
}

void PortalCapture::failStart(const QString &message)
{
    m_startTimer.stop();
    unwatchStartResponse();
    m_starting = false;
    m_lastError = message;
    closeSession();
    Q_EMIT failed(m_lastError);
}

void PortalCapture::onStartTimeout()
{
    if (!m_starting)
        return;
    qWarning() << "ScreenCast Start timed out waiting for the picker";
    failStart(tr("Timed out waiting to choose a screen."));
}

void PortalCapture::onStartMessage(const QDBusMessage &message)
{
    if (!m_starting)
        return;
    const QDBusMessage copy = message;
    QMetaObject::invokeMethod(this, [this, copy]() {
        if (!m_starting)
            return;
        const QList<QVariant> args = copy.arguments();
        if (args.size() < 2)
            return;
        QVariantMap results;
        decodePortalResults(args.at(1), &results);
        fillPipewireFromResults(&results);
        finishStart(args.at(0).toUInt(), results);
    }, Qt::QueuedConnection);
}

void PortalCapture::finishStart(uint response, const QVariantMap &results)
{
    if (!m_starting)
        return;
    m_startTimer.stop();
    unwatchStartResponse();
    m_starting = false;

    qInfo() << "ScreenCast Start response" << response << "keys" << results.keys();
    if (response == 1) {
        failStart(tr("Screen share was cancelled."));
        return;
    }
    if (response != 0) {
        failStart(tr("The ScreenCast portal could not start the stream."));
        return;
    }

    m_pipewireNode = results.value(QStringLiteral("pipewire_node")).toUInt();
    m_streamWidth = results.value(QStringLiteral("stream_width")).toInt();
    m_streamHeight = results.value(QStringLiteral("stream_height")).toInt();
    if (m_pipewireNode == 0) {
        failStart(tr("ScreenCast portal returned no PipeWire stream."));
        return;
    }
    QTimer::singleShot(0, this, &PortalCapture::completeStart);
}

void PortalCapture::completeStart()
{
    if (m_session.path().isEmpty() || m_pipewireFd >= 0)
        return;
    if (!openPipeWireRemote()) {
        failStart(m_lastError.isEmpty() ? tr("Could not open the PipeWire remote.")
                                        : m_lastError);
        return;
    }
}

void PortalCapture::onPipeWireRemote(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    if (m_session.path().isEmpty())
        return;
    QDBusPendingReply<QDBusUnixFileDescriptor> reply = *watcher;
    if (reply.isError() || !reply.isValid()) {
        failStart(tr("Could not open the PipeWire remote: %1")
                      .arg(reply.isError() ? reply.error().message() : QString()));
        return;
    }
    const QDBusUnixFileDescriptor ufd = reply.value();
    if (!ufd.isValid()) {
        failStart(tr("ScreenCast portal returned an invalid PipeWire fd."));
        return;
    }
    m_pipewireFd = ::dup(ufd.fileDescriptor());
    if (m_pipewireFd < 0) {
        failStart(tr("Could not duplicate the PipeWire fd."));
        return;
    }
    qInfo() << "ScreenCast pipewire node" << m_pipewireNode << "fd" << m_pipewireFd
            << "size" << m_streamWidth << "x" << m_streamHeight;
    Q_EMIT ready();
}

bool PortalCapture::openPipeWireRemote()
{
    auto bus = QDBusConnection::sessionBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(ScreenCastPortal::desktopService),
        QString::fromLatin1(ScreenCastPortal::desktopPath),
        QString::fromLatin1(ScreenCastPortal::screenCastInterface),
        QStringLiteral("OpenPipeWireRemote"));
    msg << QVariant::fromValue(m_session) << QVariant::fromValue(QVariantMap());
    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            &PortalCapture::onPipeWireRemote);
    return true;
}

#include "portalcapture.moc"
