#include "capture/treelandcapture.h"

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
#include <QSocketNotifier>
#include <QTimer>
#include <QVariantMap>

#include <dbus/dbus.h>

#include <cstdio>
#include <cstring>
#include <unistd.h>

class TreelandRequestWaiter : public QObject
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

QString makeToken()
{
    return QStringLiteral("cast%1").arg(QRandomGenerator::global()->generate(), 8, 16,
                                        QLatin1Char('0'));
}

bool parseStartResponse(DBusMessage *msg, uint *code, uint *node, int *width, int *height)
{
    if (!code || !node || !width || !height)
        return false;
    *code = 2;
    *node = 0;
    *width = 0;
    *height = 0;

    DBusMessageIter iter;
    if (!dbus_message_iter_init(msg, &iter))
        return false;
    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_UINT32)
        return false;
    dbus_uint32_t response = 2;
    dbus_message_iter_get_basic(&iter, &response);
    *code = response;
    if (!dbus_message_iter_next(&iter) || dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
        return true;

    DBusMessageIter dict;
    dbus_message_iter_recurse(&iter, &dict);
    while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry;
        dbus_message_iter_recurse(&dict, &entry);
        const char *key = nullptr;
        if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING)
            dbus_message_iter_get_basic(&entry, &key);
        if (!dbus_message_iter_next(&entry) || !key || std::strcmp(key, "streams") != 0) {
            dbus_message_iter_next(&dict);
            continue;
        }
        if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_VARIANT) {
            dbus_message_iter_next(&dict);
            continue;
        }
        DBusMessageIter variant;
        dbus_message_iter_recurse(&entry, &variant);
        if (dbus_message_iter_get_arg_type(&variant) != DBUS_TYPE_ARRAY) {
            dbus_message_iter_next(&dict);
            continue;
        }
        DBusMessageIter array;
        dbus_message_iter_recurse(&variant, &array);
        if (dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_STRUCT) {
            dbus_message_iter_next(&dict);
            continue;
        }
        DBusMessageIter stru;
        dbus_message_iter_recurse(&array, &stru);
        if (dbus_message_iter_get_arg_type(&stru) == DBUS_TYPE_UINT32) {
            dbus_uint32_t n = 0;
            dbus_message_iter_get_basic(&stru, &n);
            *node = n;
        }
        if (dbus_message_iter_next(&stru) && dbus_message_iter_get_arg_type(&stru) == DBUS_TYPE_ARRAY) {
            DBusMessageIter props;
            dbus_message_iter_recurse(&stru, &props);
            while (dbus_message_iter_get_arg_type(&props) == DBUS_TYPE_DICT_ENTRY) {
                DBusMessageIter prop;
                dbus_message_iter_recurse(&props, &prop);
                const char *pk = nullptr;
                if (dbus_message_iter_get_arg_type(&prop) == DBUS_TYPE_STRING)
                    dbus_message_iter_get_basic(&prop, &pk);
                if (dbus_message_iter_next(&prop) && pk && std::strcmp(pk, "size") == 0
                    && dbus_message_iter_get_arg_type(&prop) == DBUS_TYPE_VARIANT) {
                    DBusMessageIter sizeVar;
                    dbus_message_iter_recurse(&prop, &sizeVar);
                    if (dbus_message_iter_get_arg_type(&sizeVar) == DBUS_TYPE_STRUCT) {
                        DBusMessageIter sizeSt;
                        dbus_message_iter_recurse(&sizeVar, &sizeSt);
                        dbus_int32_t ww = 0;
                        dbus_int32_t hh = 0;
                        if (dbus_message_iter_get_arg_type(&sizeSt) == DBUS_TYPE_INT32)
                            dbus_message_iter_get_basic(&sizeSt, &ww);
                        if (dbus_message_iter_next(&sizeSt)
                            && dbus_message_iter_get_arg_type(&sizeSt) == DBUS_TYPE_INT32)
                            dbus_message_iter_get_basic(&sizeSt, &hh);
                        *width = ww;
                        *height = hh;
                    }
                }
                dbus_message_iter_next(&props);
            }
        }
        dbus_message_iter_next(&dict);
    }
    return true;
}

bool becomeMonitor(DBusConnection *connection, QString *errorOut)
{
    DBusError err;
    dbus_error_init(&err);
    DBusMessage *msg = dbus_message_new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                                    "org.freedesktop.DBus.Monitoring",
                                                    "BecomeMonitor");
    if (!msg)
        return false;
    DBusMessageIter iter;
    DBusMessageIter arr;
    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);
    const char *rule =
        "type='signal',interface='org.freedesktop.portal.Request',member='Response'";
    dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &rule);
    dbus_message_iter_close_container(&iter, &arr);
    dbus_uint32_t flags = 0;
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &flags);
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(connection, msg, 5000, &err);
    dbus_message_unref(msg);
    if (!reply) {
        if (errorOut)
            *errorOut = QString::fromUtf8(err.message ? err.message : "BecomeMonitor failed");
        dbus_error_free(&err);
        return false;
    }
    dbus_message_unref(reply);
    return true;
}

} // namespace

TreelandCapture::TreelandCapture(QObject *parent)
    : QObject(parent)
{
    m_startTimer.setSingleShot(true);
    connect(&m_startTimer, &QTimer::timeout, this, &TreelandCapture::onStartTimeout);
}

TreelandCapture::~TreelandCapture()
{
    stopResponseWatcher();
}

QString TreelandCapture::name() const
{
    return QStringLiteral("TreelandCapture");
}

int TreelandCapture::pipewireFd() const
{
    return m_pipewireFd;
}

uint TreelandCapture::pipewireNode() const
{
    return m_pipewireNode;
}

int TreelandCapture::streamWidth() const
{
    return m_streamWidth;
}

int TreelandCapture::streamHeight() const
{
    return m_streamHeight;
}

QString TreelandCapture::lastError() const
{
    return m_lastError;
}

void TreelandCapture::stop()
{
    m_startTimer.stop();
    stopResponseWatcher();
    m_starting = false;
    closeSession();
}

void TreelandCapture::closeSession()
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

uint TreelandCapture::readPortalUintProperty(const QString &name) const
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

uint TreelandCapture::availableSourceTypes() const
{
    return readPortalUintProperty(QStringLiteral("AvailableSourceTypes"));
}

uint TreelandCapture::availableCursorModes() const
{
    return readPortalUintProperty(QStringLiteral("AvailableCursorModes"));
}

bool TreelandCapture::start(const DisplaySource &source)
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
                         "xdg-desktop-portal ScreenCast has no sources "
                         "(needs xdg-desktop-portal-dde on Treeland).");
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

bool TreelandCapture::callRequest(const QString &method, const QVariantList &args, int timeoutMs,
                                QVariantMap *results, const QVariantMap &extraOptions)
{
    auto bus = QDBusConnection::sessionBus();
    const QString token = makeToken();
    const QString predicted = ScreenCastPortal::objectPath(
        QStringLiteral("request"), bus.baseService(), token);

    TreelandRequestWaiter waiter;
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
    QObject::connect(&waiter, &TreelandRequestWaiter::finished, &loop, &QEventLoop::quit);
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

bool TreelandCapture::createSession()
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

bool TreelandCapture::selectSources(uint cursorModes)
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

bool TreelandCapture::startResponseWatcher(const QString &token)
{
    stopResponseWatcher();
    m_startToken = token;

    DBusError err;
    dbus_error_init(&err);
    m_monitor = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (!m_monitor) {
        m_lastError = tr("Could not listen for the ScreenCast portal reply.");
        qWarning() << "ScreenCast monitor bus" << (err.message ? err.message : "");
        dbus_error_free(&err);
        return false;
    }
    dbus_connection_set_exit_on_disconnect(m_monitor, false);

    QString monitorError;
    if (!becomeMonitor(m_monitor, &monitorError)) {
        // Unicast Request.Response is destined for Qt's unique name. A second
        // connection only sees it as a bus monitor.
        m_lastError = tr("Could not listen for the ScreenCast portal reply.");
        qWarning() << "ScreenCast BecomeMonitor" << monitorError;
        stopResponseWatcher();
        return false;
    }

    int fd = -1;
    if (!dbus_connection_get_socket(m_monitor, &fd) || fd < 0) {
        m_lastError = tr("Could not listen for the ScreenCast portal reply.");
        stopResponseWatcher();
        return false;
    }
    m_monitorNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(m_monitorNotifier, &QSocketNotifier::activated, this,
            [this](QSocketDescriptor) { onMonitorSocket(); });
    qInfo() << "ScreenCast watching Response via libdbus monitor token" << token;
    return true;
}

void TreelandCapture::stopResponseWatcher()
{
    m_startToken.clear();
    delete m_monitorNotifier;
    m_monitorNotifier = nullptr;
    if (!m_monitor)
        return;
    dbus_connection_close(m_monitor);
    dbus_connection_unref(m_monitor);
    m_monitor = nullptr;
}

void TreelandCapture::onMonitorSocket()
{
    if (!m_monitor || !m_starting)
        return;
    dbus_connection_read_write(m_monitor, 0);
    DBusMessage *msg = nullptr;
    while ((msg = dbus_connection_pop_message(m_monitor))) {
        handleMonitorMessage(msg);
        dbus_message_unref(msg);
    }
}

void TreelandCapture::handleMonitorMessage(void *dbusMessage)
{
    auto *msg = static_cast<DBusMessage *>(dbusMessage);
    if (!m_starting || !msg)
        return;
    if (!dbus_message_is_signal(msg, "org.freedesktop.portal.Request", "Response"))
        return;
    const char *path = dbus_message_get_path(msg);
    if (!path || !QString::fromUtf8(path).contains(m_startToken))
        return;
    uint code = 2;
    uint node = 0;
    int width = 0;
    int height = 0;
    if (!parseStartResponse(msg, &code, &node, &width, &height)) {
        qWarning() << "ScreenCast monitor could not parse Response" << path;
        return;
    }
    qInfo() << "ScreenCast monitor Response" << path << "code" << code << "node" << node;
    const uint c = code;
    const uint n = node;
    const int w = width;
    const int h = height;
    QMetaObject::invokeMethod(
        this, [this, c, n, w, h]() { finishStart(c, n, w, h); }, Qt::QueuedConnection);
}

bool TreelandCapture::beginStartSession()
{
    auto bus = QDBusConnection::sessionBus();
    const QString token = makeToken();
    const QString predicted = ScreenCastPortal::objectPath(
        QStringLiteral("request"), bus.baseService(), token);
    if (!startResponseWatcher(token))
        return false;

    QVariantMap options;
    options.insert(QStringLiteral("handle_token"), token);
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(ScreenCastPortal::desktopService),
        QString::fromLatin1(ScreenCastPortal::desktopPath),
        QString::fromLatin1(ScreenCastPortal::screenCastInterface), QStringLiteral("Start"));
    msg << QVariant::fromValue(m_session) << QString() << QVariant::fromValue(options);
    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &TreelandCapture::onStartInvoked);

    m_starting = true;
    m_startTimer.start(180000);
    qInfo() << "ScreenCast Start sent, waiting for the monitor picker" << predicted;
    std::fprintf(stderr, "ot-cast: ScreenCast Start async %s\n", qPrintable(predicted));
    std::fflush(stderr);
    return true;
}

void TreelandCapture::onStartInvoked(QDBusPendingCallWatcher *watcher)
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
}

void TreelandCapture::failStart(const QString &message)
{
    m_startTimer.stop();
    stopResponseWatcher();
    m_starting = false;
    m_lastError = message;
    closeSession();
    Q_EMIT failed(m_lastError);
}

void TreelandCapture::onStartTimeout()
{
    if (!m_starting)
        return;
    qWarning() << "ScreenCast Start timed out waiting for the picker";
    failStart(tr("Timed out waiting to choose a screen."));
}

void TreelandCapture::finishStart(uint response, uint node, int width, int height)
{
    if (!m_starting)
        return;
    m_startTimer.stop();
    stopResponseWatcher();
    m_starting = false;

    qInfo() << "ScreenCast Start response" << response << "node" << node << "size" << width << "x"
            << height;
    if (response == 1) {
        failStart(tr("Screen share was cancelled."));
        return;
    }
    if (response != 0) {
        failStart(tr("The ScreenCast portal could not start the stream."));
        return;
    }

    m_pipewireNode = node;
    m_streamWidth = width;
    m_streamHeight = height;
    QTimer::singleShot(0, this, &TreelandCapture::completeStart);
}

void TreelandCapture::completeStart()
{
    if (m_session.path().isEmpty() || m_pipewireFd >= 0)
        return;
    if (!openPipeWireRemote()) {
        failStart(m_lastError.isEmpty() ? tr("Could not open the PipeWire remote.")
                                        : m_lastError);
        return;
    }
}

void TreelandCapture::onPipeWireRemote(QDBusPendingCallWatcher *watcher)
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

bool TreelandCapture::openPipeWireRemote()
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
            &TreelandCapture::onPipeWireRemote);
    return true;
}

#include "treelandcapture.moc"
