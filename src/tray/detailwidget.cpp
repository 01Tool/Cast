#include "tray/detailwidget.h"

#include <DFontSizeManager>
#include <DLabel>
#include <DListView>
#include <DPalette>
#include <DStandardItem>
#include <DStyledItemDelegate>
#include <DSuggestButton>

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QJsonObject>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "constants.h"

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

DetailWidget::DetailWidget(CastClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setFixedWidth(Dock::QUICK_ITEM_FULL_WIDTH);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);

    m_status = new DLabel(this);
    m_status->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_status, DFontSizeManager::T8);

    m_list = new DListView(this);
    m_list->setItemDelegate(new DStyledItemDelegate(m_list));
    m_list->setBackgroundType(DStyledItemDelegate::RoundedBackground);
    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_model = new QStandardItemModel(m_list);
    m_list->setModel(m_model);
    m_list->setMinimumHeight(160);

    auto *buttons = new QWidget(this);
    auto *row = new QHBoxLayout(buttons);
    row->setContentsMargins(0, 0, 0, 0);
    m_scan = new QPushButton(tr("Scan"), buttons);
    m_disconnect = new QPushButton(tr("Disconnect"), buttons);
    m_connect = new DSuggestButton(tr("Connect"), buttons);
    row->addWidget(m_scan);
    row->addStretch();
    row->addWidget(m_disconnect);
    row->addWidget(m_connect);

    m_open = new QPushButton(tr("Open Cast"), this);

    layout->addWidget(m_status);
    layout->addWidget(m_list, 1);
    layout->addWidget(buttons);
    layout->addWidget(m_open);

    connect(m_scan, &QPushButton::clicked, this, &DetailWidget::onScan);
    connect(m_connect, &DSuggestButton::clicked, this, &DetailWidget::onConnect);
    connect(m_disconnect, &QPushButton::clicked, this, &DetailWidget::onDisconnect);
    connect(m_open, &QPushButton::clicked, m_client, &CastClient::raiseWindow);
    connect(m_client, &CastClient::stateChanged, this, &DetailWidget::refresh);
    connect(m_client, &CastClient::sinksChanged, this, &DetailWidget::refresh);
    connect(m_client, &CastClient::availableChanged, this, &DetailWidget::refresh);
    connect(m_list->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection &, const QItemSelection &) {
                const QString state = m_client->state();
                const bool busy = state == QLatin1String("Scanning")
                    || state == QLatin1String("Connecting")
                    || state == QLatin1String("Streaming");
                m_connect->setEnabled(!busy && m_list->currentIndex().isValid());
            });
    refresh();
}

QSize DetailWidget::sizeHint() const
{
    return QSize(Dock::QUICK_ITEM_FULL_WIDTH, 360);
}

void DetailWidget::refresh()
{
    const QString status = m_client->statusMessage();
    if (!m_client->available())
        m_status->setText(tr("Starting Cast…"));
    else if (status.isEmpty())
        m_status->setText(tr("Idle. Scan for Miracast and DLNA."));
    else
        m_status->setText(status);

    const QString selected = selectedSinkId();
    m_model->clear();
    const auto sinks = m_client->sinks();
    if (sinks.isEmpty()) {
        auto *empty = new DStandardItem(tr("No displays found"));
        empty->setEnabled(false);
        m_model->appendRow(empty);
    } else {
        int current = -1;
        for (int i = 0; i < sinks.size(); ++i) {
            const QJsonObject row = sinks.at(i).toObject();
            const QString name = row.value(QStringLiteral("name")).toString();
            const QString protocol = row.value(QStringLiteral("protocol")).toString();
            auto *item = new DStandardItem(protocol.isEmpty() ? name
                                                              : (name + QStringLiteral(" · ") + protocol));
            item->setData(row.value(QStringLiteral("id")).toString(), Qt::UserRole);
            m_model->appendRow(item);
            if (item->data(Qt::UserRole).toString() == selected)
                current = i;
        }
        if (current >= 0)
            m_list->setCurrentIndex(m_model->index(current, 0));
    }

    const QString state = m_client->state();
    const bool busy = state == QLatin1String("Scanning") || state == QLatin1String("Connecting")
        || state == QLatin1String("Streaming");
    m_scan->setEnabled(state != QLatin1String("Scanning") && state != QLatin1String("Connecting")
                       && state != QLatin1String("Streaming"));
    m_connect->setEnabled(!busy && m_list->currentIndex().isValid());
    m_disconnect->setEnabled(state == QLatin1String("Streaming")
                             || state == QLatin1String("Connecting"));
}

void DetailWidget::onScan()
{
    m_client->startScan();
}

void DetailWidget::onConnect()
{
    const QString id = selectedSinkId();
    if (!id.isEmpty())
        m_client->connectToSink(id);
}

void DetailWidget::onDisconnect()
{
    m_client->disconnectFromSink();
}

QString DetailWidget::selectedSinkId() const
{
    const QModelIndex index = m_list->currentIndex();
    if (!index.isValid())
        return {};
    return index.data(Qt::UserRole).toString();
}