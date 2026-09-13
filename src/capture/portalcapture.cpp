#include "capture/portalcapture.h"

#include "capture/screencastportal.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QDBusVariant>
#include <QDebug>
#include <QEventLoop>
#include <QMetaType>
#include <QRandomGenerator>
#include <QTimer>
#include <QVariantMap>

#include <unistd.h>

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
        got = true;
        emit finished();
    }

Q_SIGNALS:
    void finished();
};

namespace {

bool parseStreams(const QVariant &streamsVar, uint *nodeId, int *width, int *height)
{
    if (!nodeId)
        return false;
    QDBusArgument arg = streamsVar.value<QDBusArgument>();
    arg.beginArray();
    if (arg.atEnd()) {
        arg.endArray();
        return false;
    }
    arg.beginStructure();
    uint node = 0;
    QVariantMap props;
    arg >> node >> props;
    arg.endStructure();
    arg.endArray();
    *nodeId = node;
    if (width && height && props.contains(QStringLiteral("size"))) {
        const QDBusArgument sizeArg = props.value(QStringLiteral("size")).value<QDBusArgument>();
        int w = 0;
        int h = 0;
        sizeArg.beginStructure();
        sizeArg >> w >> h;
        sizeArg.endStructure();
        if (w > 0 && h > 0) {
            *width = w;
            *height = h;
        }
    }
    return node != 0;
}

QString makeToken()
{
    return QStringLiteral("cast%1").arg(QRandomGenerator::global()->generate(), 8, 16,
                                        QLatin1Char('0'));
}

} // namespace

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

uint PortalCapture::availableSourceTypes() const
{
    auto bus = QDBusConnection::sessionBus();
    QDBusMessage get = QDBusMessage::createMethodCall(
        QString::fromLatin1(ScreenCastPortal::desktopService),
        QString::fromLatin1(ScreenCastPortal::desktopPath),
        QString::fromLatin1(ScreenCastPortal::propertiesInterface), QStringLiteral("Get"));
    get << QString::fromLatin1(ScreenCastPortal::screenCastInterface)
        << QStringLiteral("AvailableSourceTypes");
    const QDBusMessage reply = bus.call(get, QDBus::Block, 3000);
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty())
        return 0;
    const QVariant value = reply.arguments().constFirst();
    if (value.canConvert<QDBusVariant>())
        return qvariant_cast<QDBusVariant>(value).variant().toUInt();
    return value.toUInt();
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
    if (types == 0) {
        m_lastError = tr("Screen capture is unavailable on this session. "
                         "xdg-desktop-portal ScreenCast has no sources "
                         "(needs a Wayland portal backend).");
        qWarning() << m_lastError;
        return false;
    }

    if (!createSession())
        return false;
    if (!selectSources()) {
        closeSession();
        return false;
    }
    if (!startSession()) {
        closeSession();
        return false;
    }
    if (!openPipeWireRemote()) {
        closeSession();
        return false;
    }
    qInfo() << "ScreenCast pipewire node" << m_pipewireNode << "fd" << m_pipewireFd;
    return true;
}

bool PortalCapture::callRequest(const QString &method, const QVariantList &args, int timeoutMs,
                                QVariantMap *results)
{
    auto bus = QDBusConnection::sessionBus();
    const QString token = makeToken();
    const QString predicted = ScreenCastPortal::objectPath(
        QStringLiteral("request"), bus.baseService(), token);

    PortalRequestWaiter waiter;
    if (!bus.connect(QString::fromLatin1(ScreenCastPortal::desktopService), predicted,
                     QString::fromLatin1(ScreenCastPortal::requestInterface),
                     QStringLiteral("Response"), &waiter, SLOT(onResponse(uint,QVariantMap)))) {
        m_lastError = tr("Could not listen for the ScreenCast portal reply.");
        return false;
    }

    QVariantMap options;
    options.insert(QStringLiteral("handle_token"), token);

    QDBusMessage msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(ScreenCastPortal::desktopService),
        QString::fromLatin1(ScreenCastPortal::desktopPath),
        QString::fromLatin1(ScreenCastPortal::screenCastInterface), method);
    QVariantList callArgs = args;
    if (method == QLatin1String("CreateSession")) {
        QVariantMap create = options;
        create.insert(QStringLiteral("session_handle_token"), makeToken());
        callArgs << QVariant::fromValue(create);
    } else {
        callArgs << QVariant::fromValue(options);
    }
    msg.setArguments(callArgs);

    const QDBusMessage reply = bus.call(msg, QDBus::Block, 15000);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        m_lastError = tr("ScreenCast %1 failed: %2").arg(method, reply.errorMessage());
        return false;
    }
    if (!reply.arguments().isEmpty()) {
        const QDBusObjectPath actual = qvariant_cast<QDBusObjectPath>(reply.arguments().constFirst());
        if (!actual.path().isEmpty() && actual.path() != predicted) {
            bus.connect(QString::fromLatin1(ScreenCastPortal::desktopService), actual.path(),
                        QString::fromLatin1(ScreenCastPortal::requestInterface),
                        QStringLiteral("Response"), &waiter, SLOT(onResponse(uint,QVariantMap)));
        }
    }

    QEventLoop loop;
    QObject::connect(&waiter, &PortalRequestWaiter::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    if (!waiter.got)
        loop.exec();

    if (!waiter.got) {
        m_lastError = tr("The ScreenCast portal did not reply in time.");
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

bool PortalCapture::selectSources()
{
    QVariantMap options;
    options.insert(QStringLiteral("types"), ScreenCastPortal::monitorSource);
    options.insert(QStringLiteral("multiple"), false);
    options.insert(QStringLiteral("cursor"), ScreenCastPortal::cursorEmbedded);

    // SelectSources extra options are merged in callRequest's handle_token map.
    // Pass types via a dedicated options argument: (o, a{sv}).
    auto bus = QDBusConnection::sessionBus();
    const QString token = makeToken();
    const QString predicted = ScreenCastPortal::objectPath(
        QStringLiteral("request"), bus.baseService(), token);
    PortalRequestWaiter waiter;
    if (!bus.connect(QString::fromLatin1(ScreenCastPortal::desktopService), predicted,
                     QString::fromLatin1(ScreenCastPortal::requestInterface),
                     QStringLiteral("Response"), &waiter, SLOT(onResponse(uint,QVariantMap)))) {
        m_lastError = tr("Could not listen for the ScreenCast portal reply.");
        return false;
    }
    options.insert(QStringLiteral("handle_token"), token);

    QDBusMessage msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(ScreenCastPortal::desktopService),
        QString::fromLatin1(ScreenCastPortal::desktopPath),
        QString::fromLatin1(ScreenCastPortal::screenCastInterface), QStringLiteral("SelectSources"));
    msg << QVariant::fromValue(m_session) << QVariant::fromValue(options);
    const QDBusMessage reply = bus.call(msg, QDBus::Block, 15000);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        m_lastError = tr("ScreenCast SelectSources failed: %1").arg(reply.errorMessage());
        return false;
    }

    QEventLoop loop;
    QObject::connect(&waiter, &PortalRequestWaiter::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(15000, &loop, &QEventLoop::quit);
    if (!waiter.got)
        loop.exec();
    if (!waiter.got || waiter.code != 0) {
        m_lastError = waiter.code == 1 ? tr("Screen share was cancelled.")
                                       : tr("The ScreenCast portal refused the sources.");
        return false;
    }
    return true;
}

bool PortalCapture::startSession()
{
    auto bus = QDBusConnection::sessionBus();
    const QString token = makeToken();
    const QString predicted = ScreenCastPortal::objectPath(
        QStringLiteral("request"), bus.baseService(), token);
    PortalRequestWaiter waiter;
    if (!bus.connect(QString::fromLatin1(ScreenCastPortal::desktopService), predicted,
                     QString::fromLatin1(ScreenCastPortal::requestInterface),
                     QStringLiteral("Response"), &waiter, SLOT(onResponse(uint,QVariantMap)))) {
        m_lastError = tr("Could not listen for the ScreenCast portal reply.");
        return false;
    }

    QVariantMap options;
    options.insert(QStringLiteral("handle_token"), token);
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(ScreenCastPortal::desktopService),
        QString::fromLatin1(ScreenCastPortal::desktopPath),
        QString::fromLatin1(ScreenCastPortal::screenCastInterface), QStringLiteral("Start"));
    msg << QVariant::fromValue(m_session) << QString() << QVariant::fromValue(options);
    const QDBusMessage reply = bus.call(msg, QDBus::Block, 15000);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        m_lastError = tr("ScreenCast Start failed: %1").arg(reply.errorMessage());
        return false;
    }

    QEventLoop loop;
    QObject::connect(&waiter, &PortalRequestWaiter::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(180000, &loop, &QEventLoop::quit);
    if (!waiter.got)
        loop.exec();
    if (!waiter.got) {
        m_lastError = tr("Timed out waiting to choose a screen.");
        return false;
    }
    if (waiter.code == 1) {
        m_lastError = tr("Screen share was cancelled.");
        return false;
    }
    if (waiter.code != 0) {
        m_lastError = tr("The ScreenCast portal could not start the stream.");
        return false;
    }

    if (!parseStreams(waiter.results.value(QStringLiteral("streams")), &m_pipewireNode,
                      &m_streamWidth, &m_streamHeight)) {
        m_lastError = tr("ScreenCast portal returned no PipeWire stream.");
        return false;
    }
    return true;
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
    const QDBusMessage reply = bus.call(msg, QDBus::Block, 10000);
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty()) {
        m_lastError = tr("Could not open the PipeWire remote: %1")
                          .arg(reply.errorMessage());
        return false;
    }
    const auto ufd = qvariant_cast<QDBusUnixFileDescriptor>(reply.arguments().constFirst());
    if (!ufd.isValid()) {
        m_lastError = tr("ScreenCast portal returned an invalid PipeWire fd.");
        return false;
    }
    m_pipewireFd = ::dup(ufd.fileDescriptor());
    if (m_pipewireFd < 0) {
        m_lastError = tr("Could not duplicate the PipeWire fd.");
        return false;
    }
    return true;
}

#include "portalcapture.moc"
