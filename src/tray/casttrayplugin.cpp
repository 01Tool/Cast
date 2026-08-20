#include "tray/casttrayplugin.h"

#include "tray/castclient.h"
#include "tray/detailwidget.h"
#include "tray/quickpanel.h"

#include <DIconTheme>
#include <DLabel>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QStandardPaths>
#include <QVariant>

#include "constants.h"

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

namespace {
constexpr auto kItemKey = "ot-cast";
}

CastTrayPlugin::CastTrayPlugin(QObject *parent)
    : QObject(parent)
{
}

const QString CastTrayPlugin::pluginName() const
{
    return QStringLiteral("ot-cast");
}

const QString CastTrayPlugin::pluginDisplayName() const
{
    return tr("Cast");
}

void CastTrayPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;
    loadTranslator();

    m_client = new CastClient(this);
    m_quick = new QuickPanel;
    m_detail = new DetailWidget(m_client);
    m_tips = new DLabel;
    m_tips->setText(tr("Cast"));
    m_trayIcon = new DLabel;
    m_trayIcon->setFixedSize(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE);
    m_trayIcon->setPixmap(DIconTheme::findQIcon(QStringLiteral("ot-cast"),
                                                DIconTheme::findQIcon(QStringLiteral("video-display")))
                              .pixmap(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE));

    connect(m_quick, &QuickPanel::requestShowDetail, this, &CastTrayPlugin::showDetail);
    connect(m_client, &CastClient::stateChanged, this, [this]() {
        const QString state = m_client->state();
        QString label = m_client->statusMessage();
        if (label.isEmpty()) {
            if (state == QLatin1String("Streaming"))
                label = tr("Casting");
            else if (state == QLatin1String("Scanning"))
                label = tr("Scanning");
            else
                label = tr("Idle");
        }
        m_quick->setStatus(label);
        m_tips->setText(label);
        notifyActive(state == QLatin1String("Streaming"));
    });

    if (!pluginIsDisable())
        m_proxyInter->itemAdded(this, itemKey());
}

QWidget *CastTrayPlugin::itemWidget(const QString &itemKey)
{
    if (itemKey == Dock::QUICK_ITEM_KEY)
        return m_quick;
    if (itemKey == this->itemKey())
        return m_trayIcon;
    return nullptr;
}

QWidget *CastTrayPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return m_tips;
}

QWidget *CastTrayPlugin::itemPopupApplet(const QString &itemKey)
{
    if (itemKey == Dock::QUICK_ITEM_KEY || itemKey == this->itemKey())
        return m_detail;
    return nullptr;
}

const QString CastTrayPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    QList<QVariant> items;
    QMap<QString, QVariant> scan;
    scan.insert(QStringLiteral("itemId"), QStringLiteral("scan"));
    scan.insert(QStringLiteral("itemText"), tr("Scan"));
    scan.insert(QStringLiteral("isCheckable"), false);
    scan.insert(QStringLiteral("isActive"), true);
    items << scan;
    QMap<QString, QVariant> open;
    open.insert(QStringLiteral("itemId"), QStringLiteral("open"));
    open.insert(QStringLiteral("itemText"), tr("Open Cast"));
    open.insert(QStringLiteral("isCheckable"), false);
    open.insert(QStringLiteral("isActive"), true);
    items << open;
    QMap<QString, QVariant> menu;
    menu.insert(QStringLiteral("items"), items);
    menu.insert(QStringLiteral("checkableMenu"), false);
    menu.insert(QStringLiteral("singleCheck"), false);
    return QString::fromUtf8(QJsonDocument::fromVariant(menu).toJson());
}

void CastTrayPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey)
    Q_UNUSED(checked)
    if (menuId == QLatin1String("scan")) {
        showDetail();
        m_client->startScan();
    } else if (menuId == QLatin1String("open")) {
        m_client->raiseWindow();
    }
}

Dock::PluginFlags CastTrayPlugin::flags() const
{
    return Dock::Type_Quick | Dock::Quick_Panel_Single | Dock::Attribute_CanDrag
        | Dock::Attribute_CanInsert;
}

QIcon CastTrayPlugin::icon(Dock::IconType, Dock::ThemeType) const
{
    return DIconTheme::findQIcon(QStringLiteral("ot-cast"),
                                 DIconTheme::findQIcon(QStringLiteral("video-display")));
}

void CastTrayPlugin::setMessageCallback(MessageCallbackFunc cb)
{
    m_messageCallback = cb;
}

QString CastTrayPlugin::message(const QString &msg)
{
    const QJsonObject obj = QJsonDocument::fromJson(msg.toUtf8()).object();
    const QString type = obj.value(Dock::MSG_TYPE).toString();
    if (type == Dock::MSG_GET_SUPPORT_FLAG) {
        QJsonObject data;
        data.insert(Dock::MSG_SUPPORT_FLAG, true);
        QJsonObject reply;
        reply.insert(Dock::MSG_TYPE, type);
        reply.insert(Dock::MSG_DATA, data);
        return QString::fromUtf8(QJsonDocument(reply).toJson(QJsonDocument::Compact));
    }
    if (type == Dock::MSG_WHETHER_WANT_TO_BE_LOADED) {
        QJsonObject data;
        data.insert(Dock::MSG_WHETHER_WANT_TO_BE_LOADED, true);
        QJsonObject reply;
        reply.insert(Dock::MSG_TYPE, type);
        reply.insert(Dock::MSG_DATA, data);
        return QString::fromUtf8(QJsonDocument(reply).toJson(QJsonDocument::Compact));
    }
    return QStringLiteral("{}");
}

bool CastTrayPlugin::pluginIsAllowDisable()
{
    return true;
}

bool CastTrayPlugin::pluginIsDisable()
{
    if (!m_proxyInter)
        return false;
    return m_proxyInter->getValue(this, QStringLiteral("disabled"), false).toBool();
}

void CastTrayPlugin::pluginStateSwitched()
{
    const bool next = !pluginIsDisable();
    m_proxyInter->saveValue(this, QStringLiteral("disabled"), next);
    if (next)
        m_proxyInter->itemRemoved(this, itemKey());
    else
        m_proxyInter->itemAdded(this, itemKey());
}

void CastTrayPlugin::loadTranslator()
{
    const QString name = QStringLiteral("ot-cast-tray_%1.qm").arg(QLocale::system().name());
    const QStringList dirs{
        QCoreApplication::applicationDirPath() + QStringLiteral("/translations"),
        QStringLiteral("/usr/share/ot-cast/translations"),
    };
    for (const QString &dir : dirs) {
        if (m_translator.load(name, dir) || m_translator.load(QLocale(), QStringLiteral("ot-cast-tray"),
                                                             QStringLiteral("_"), dir)) {
            QCoreApplication::installTranslator(&m_translator);
            return;
        }
    }
}

void CastTrayPlugin::showDetail()
{
    m_client->ensureService();
    m_client->startScan();
    if (m_proxyInter)
        m_proxyInter->requestSetAppletVisible(this, itemKey(), true);
}

void CastTrayPlugin::notifyActive(bool active)
{
    if (!m_messageCallback)
        return;
    QJsonObject data;
    data.insert(QStringLiteral("itemActiveState"), active);
    QJsonObject msg;
    msg.insert(Dock::MSG_TYPE, Dock::MSG_ITEM_ACTIVE_STATE);
    msg.insert(Dock::MSG_DATA, data);
    m_messageCallback(this, QString::fromUtf8(QJsonDocument(msg).toJson(QJsonDocument::Compact)));
}

QString CastTrayPlugin::itemKey() const
{
    return QString::fromLatin1(kItemKey);
}