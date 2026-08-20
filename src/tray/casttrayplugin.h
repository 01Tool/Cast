#pragma once

#include <pluginsiteminterface_v2.h>

#include <QObject>
#include <QPointer>
#include <QTranslator>

class CastClient;
class DetailWidget;
class QuickPanel;
class QLabel;

class CastTrayPlugin : public QObject, public PluginsItemInterfaceV2
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterfaceV2)
    Q_PLUGIN_METADATA(IID ModuleInterface_iid_V2 FILE "ot-cast-tray.json")

public:
    explicit CastTrayPlugin(QObject *parent = nullptr);

    const QString pluginName() const override;
    const QString pluginDisplayName() const override;
    void init(PluginProxyInterface *proxyInter) override;
    QWidget *itemWidget(const QString &itemKey) override;
    QWidget *itemTipsWidget(const QString &itemKey) override;
    QWidget *itemPopupApplet(const QString &itemKey) override;
    const QString itemContextMenu(const QString &itemKey) override;
    void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked) override;
    Dock::PluginFlags flags() const override;
    QIcon icon(Dock::IconType, Dock::ThemeType themeType) const override;
    void setMessageCallback(MessageCallbackFunc cb) override;
    QString message(const QString &msg) override;
    bool pluginIsAllowDisable() override;
    bool pluginIsDisable() override;
    void pluginStateSwitched() override;

private:
    void loadTranslator();
    void showDetail();
    void notifyActive(bool active);
    QString itemKey() const;

    CastClient *m_client = nullptr;
    QuickPanel *m_quick = nullptr;
    DetailWidget *m_detail = nullptr;
    QLabel *m_tips = nullptr;
    QLabel *m_trayIcon = nullptr;
    QTranslator m_translator;
    MessageCallbackFunc m_messageCallback = nullptr;
};