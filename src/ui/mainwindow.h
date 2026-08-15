#pragma once

#include "engine/castengine.h"

#include <DMainWindow>

class QPushButton;
class QStandardItemModel;

DWIDGET_BEGIN_NAMESPACE
class DComboBox;
class DLabel;
class DListView;
class DSuggestButton;
class DSwitchButton;
DWIDGET_END_NAMESPACE

class MainWindow : public DTK_WIDGET_NAMESPACE::DMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(CastEngine *engine, QWidget *parent = nullptr);

private:
    void setupUi();
    void bindEngine();
    void refreshSinkList();
    void refreshDisplayList();
    void updateActions();
    void onScanClicked();
    void onConnectClicked();
    void onDisconnectClicked();
    void onError(const QString &message);

    CastEngine *m_engine = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_statusLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_sessionLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DListView *m_sinkView = nullptr;
    QStandardItemModel *m_sinkModel = nullptr;
    QPushButton *m_scanButton = nullptr;
    DTK_WIDGET_NAMESPACE::DSuggestButton *m_connectButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    DTK_WIDGET_NAMESPACE::DSwitchButton *m_audioSwitch = nullptr;
    DTK_WIDGET_NAMESPACE::DComboBox *m_displayCombo = nullptr;
};
