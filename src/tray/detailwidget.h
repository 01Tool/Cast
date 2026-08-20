#pragma once

#include "tray/castclient.h"

#include <DLabel>
#include <DListView>
#include <DSuggestButton>

#include <QSize>
#include <QWidget>

class QStandardItemModel;
class QPushButton;

class DetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DetailWidget(CastClient *client, QWidget *parent = nullptr);
    QSize sizeHint() const override;

private:
    void refresh();
    void onScan();
    void onConnect();
    void onDisconnect();
    QString selectedSinkId() const;

    CastClient *m_client = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_status = nullptr;
    DTK_WIDGET_NAMESPACE::DListView *m_list = nullptr;
    QStandardItemModel *m_model = nullptr;
    QPushButton *m_scan = nullptr;
    DTK_WIDGET_NAMESPACE::DSuggestButton *m_connect = nullptr;
    QPushButton *m_disconnect = nullptr;
    QPushButton *m_open = nullptr;
};
