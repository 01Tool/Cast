#include "ui/mainwindow.h"

#include <DComboBox>
#include <DDialog>
#include <DFontSizeManager>
#include <DIconTheme>
#include <DLabel>
#include <DLineEdit>
#include <DListView>
#include <DMessageManager>
#include <DPalette>
#include <DStandardItem>
#include <DStyledItemDelegate>
#include <DSuggestButton>
#include <DSwitchButton>
#include <DTitlebar>

#include <QAbstractItemView>
#include <QLineEdit>
#include <QRegularExpression>
#include <QPushButton>
#include <QHBoxLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QWidget>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

MainWindow::MainWindow(CastEngine *engine, QWidget *parent)
    : DMainWindow(parent)
    , m_engine(engine)
{
    setMinimumSize(640, 480);
    titlebar()->setTitle(tr("Cast"));
    titlebar()->setIcon(DIconTheme::findQIcon(QStringLiteral("ot-cast"),
                                             DIconTheme::findQIcon(QStringLiteral("video-display"))));
    setupUi();
    bindEngine();
    updateActions();
}

void MainWindow::showAndRaise()
{
    if (isMinimized())
        showNormal();
    else
        show();
    raise();
    activateWindow();
}

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    m_sessionLabel = new DLabel(central);
    m_sessionLabel->setForegroundRole(DPalette::TextTips);
    DFontSizeManager::instance()->bind(m_sessionLabel, DFontSizeManager::T8);

    m_statusLabel = new DLabel(central);
    m_statusLabel->setForegroundRole(DPalette::TextTitle);
    m_statusLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_statusLabel, DFontSizeManager::T6);

    m_sinkView = new DListView(central);
    m_sinkView->setItemDelegate(new DStyledItemDelegate(m_sinkView));
    m_sinkView->setBackgroundType(DStyledItemDelegate::RoundedBackground);
    m_sinkView->setItemSpacing(6);
    m_sinkView->setItemSize(QSize(0, 48));
    m_sinkView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sinkView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_sinkModel = new QStandardItemModel(m_sinkView);
    m_sinkView->setModel(m_sinkModel);

    auto *monitorRow = new QWidget(central);
    auto *monitorLayout = new QHBoxLayout(monitorRow);
    monitorLayout->setContentsMargins(0, 0, 0, 0);
    monitorLayout->setSpacing(8);
    auto *monitorLabel = new DLabel(tr("Monitor"), monitorRow);
    monitorLabel->setForegroundRole(DPalette::TextTitle);
    DFontSizeManager::instance()->bind(monitorLabel, DFontSizeManager::T6);
    m_displayCombo = new DComboBox(monitorRow);
    m_displayCombo->setMinimumWidth(240);
    monitorLayout->addWidget(monitorLabel);
    monitorLayout->addWidget(m_displayCombo, 1);

    auto *audioRow = new QWidget(central);
    auto *audioLayout = new QHBoxLayout(audioRow);
    audioLayout->setContentsMargins(0, 0, 0, 0);
    audioLayout->setSpacing(8);
    auto *audioLabel = new DLabel(tr("Include system audio"), audioRow);
    audioLabel->setForegroundRole(DPalette::TextTitle);
    DFontSizeManager::instance()->bind(audioLabel, DFontSizeManager::T6);
    m_audioSwitch = new DSwitchButton(audioRow);
    m_audioSwitch->setChecked(m_engine->audioEnabled());
    audioLayout->addWidget(audioLabel);
    audioLayout->addStretch();
    audioLayout->addWidget(m_audioSwitch);

    auto *buttons = new QWidget(central);
    auto *buttonLayout = new QHBoxLayout(buttons);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8);

    m_scanButton = new QPushButton(tr("Scan"), buttons);
    m_connectButton = new DSuggestButton(tr("Connect"), buttons);
    m_disconnectButton = new QPushButton(tr("Disconnect"), buttons);

    buttonLayout->addWidget(m_scanButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_disconnectButton);
    buttonLayout->addWidget(m_connectButton);

    layout->addWidget(m_sessionLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_sinkView, 1);
    layout->addWidget(monitorRow);
    layout->addWidget(audioRow);
    layout->addWidget(buttons);

    setCentralWidget(central);
}

void MainWindow::bindEngine()
{
    QString session = tr("Display server: unknown");
    switch (m_engine->displayServer()) {
    case CastEngine::DisplayServer::X11:
        session = tr("Display server: X11");
        break;
    case CastEngine::DisplayServer::Wayland:
        session = tr("Display server: Wayland");
        break;
    case CastEngine::DisplayServer::Unknown:
        break;
    }
    m_sessionLabel->setText(session);
    m_statusLabel->setText(m_engine->statusMessage());

    connect(m_engine, &CastEngine::statusMessageChanged, m_statusLabel, &DLabel::setText);
    connect(m_engine, &CastEngine::sinksChanged, this, &MainWindow::refreshSinkList);
    connect(m_engine, &CastEngine::stateChanged, this, [this](CastEngine::SessionState) {
        updateActions();
    });
    connect(m_engine, &CastEngine::errorOccurred, this, &MainWindow::onError);
    connect(m_engine, &CastEngine::pairingRequested, this, &MainWindow::onPairingRequested);
    connect(m_engine, &CastEngine::pairingFinished, this, &MainWindow::closePairingDialog);

    connect(m_scanButton, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(m_connectButton, &DSuggestButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_audioSwitch, &DSwitchButton::checkedChanged, m_engine, &CastEngine::setAudioEnabled);
    connect(m_engine, &CastEngine::audioEnabledChanged, m_audioSwitch, &DSwitchButton::setChecked);
    connect(m_displayCombo, QOverload<int>::of(&DComboBox::currentIndexChanged), this,
            [this](int index) {
                if (index < 0)
                    return;
                m_engine->setSelectedDisplayId(m_displayCombo->itemData(index).toString());
            });
    connect(m_engine, &CastEngine::displaysChanged, this, &MainWindow::refreshDisplayList);
    connect(m_engine, &CastEngine::selectedDisplayChanged, this, &MainWindow::refreshDisplayList);
    refreshDisplayList();
    connect(m_sinkView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection &, const QItemSelection &) {
                updateActions();
            });
}

void MainWindow::refreshSinkList()
{
    m_sinkModel->clear();

    const auto sinks = m_engine->sinks();
    if (sinks.isEmpty()) {
        auto *empty = new DStandardItem(tr("No displays found"));
        empty->setEnabled(false);
        empty->setTextColorRole(DPalette::TextTips);
        empty->setFontSize(DFontSizeManager::T8);
        empty->setFlags(Qt::NoItemFlags);
        m_sinkModel->appendRow(empty);
        updateActions();
        return;
    }

    const QIcon icon = DIconTheme::findQIcon(QStringLiteral("video-display"));
    for (const auto &sink : sinks) {
        auto *item = new DStandardItem(sink.name);
        item->setIcon(icon);
        item->setData(sink.id, Qt::UserRole);
        item->setFontSize(DFontSizeManager::T6);
        const QString protocol = (sink.protocol == CastProtocol::Dlna)
            ? tr("DLNA")
            : tr("Miracast");
        QString detail = protocol;
        if (!sink.address.isEmpty())
            detail += QStringLiteral(" · ") + sink.address;
        if (sink.protocol == CastProtocol::Miracast && !sink.wfdCapable)
            detail += tr(" · no WFD IEs");
        if (sink.protocol == CastProtocol::Dlna) {
            switch (sink.dlnaMedia) {
            case DlnaMediaKind::LiveTsLikely:
                detail += tr(" · live TS likely");
                break;
            case DlnaMediaKind::FileOnlyLikely:
                detail += tr(" · files only");
                break;
            case DlnaMediaKind::NoVideo:
                detail += tr(" · no video");
                break;
            case DlnaMediaKind::Unknown:
                break;
            }
        }
        auto *sub = new DViewItemAction(Qt::AlignLeft, QSize(), QSize(), false);
        sub->setText(detail);
        sub->setTextColorRole(DPalette::TextTips);
        sub->setFontSize(DFontSizeManager::T8);
        item->setTextActionList({sub});
        auto *badge = new DViewItemAction(Qt::AlignRight, QSize(), QSize(), false);
        badge->setText(protocol);
        badge->setTextColorRole(DPalette::TextTips);
        badge->setFontSize(DFontSizeManager::T8);
        item->setActionList(Qt::RightEdge, {badge});
        m_sinkModel->appendRow(item);
    }
    updateActions();
}

void MainWindow::refreshDisplayList()
{
    const QString selected = m_engine->selectedDisplayId();
    m_displayCombo->blockSignals(true);
    m_displayCombo->clear();
    int current = -1;
    const auto displays = m_engine->displays();
    for (int i = 0; i < displays.size(); ++i) {
        m_displayCombo->addItem(displays.at(i).name, displays.at(i).id);
        if (displays.at(i).id == selected)
            current = i;
    }
    if (current < 0 && !displays.isEmpty())
        current = 0;
    if (current >= 0)
        m_displayCombo->setCurrentIndex(current);
    m_displayCombo->blockSignals(false);
    updateActions();
}

void MainWindow::updateActions()
{
    const auto state = m_engine->state();
    const bool scanning = state == CastEngine::SessionState::Scanning;
    const bool connecting = state == CastEngine::SessionState::Connecting;
    const bool streaming = state == CastEngine::SessionState::Streaming;
    const bool hasSelection = m_sinkView->selectionModel()
        && !m_sinkView->selectionModel()->selectedIndexes().isEmpty()
        && m_sinkModel->itemFromIndex(m_sinkView->currentIndex())
        && m_sinkModel->itemFromIndex(m_sinkView->currentIndex())->isEnabled();

    m_scanButton->setEnabled(!scanning && !connecting && !streaming);
    m_connectButton->setEnabled(!connecting && !streaming && hasSelection);
    m_disconnectButton->setEnabled(streaming || connecting);
    m_audioSwitch->setEnabled(!connecting && !streaming);
    m_displayCombo->setEnabled(!connecting && !streaming && m_displayCombo->count() > 0);
}

void MainWindow::onScanClicked()
{
    m_engine->startScan();
}

void MainWindow::onConnectClicked()
{
    const QModelIndex index = m_sinkView->currentIndex();
    if (!index.isValid())
        return;
    m_engine->connectToSink(index.data(Qt::UserRole).toString());
}

void MainWindow::onDisconnectClicked()
{
    m_engine->disconnectFromSink();
}

void MainWindow::onError(const QString &message)
{
    DMessageManager::instance()->sendMessage(this,
                                             DIconTheme::findQIcon(QStringLiteral("dialog-warning")),
                                             message);
}

void MainWindow::onPairingRequested(CastEngine::PairingKind kind, const QString &sinkName)
{
    closePairingDialog();

    const QString peer = sinkName.isEmpty() ? tr("the display") : sinkName;
    auto *dialog = new DDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setIcon(DIconTheme::findQIcon(QStringLiteral("network-wireless")));
    dialog->setOnButtonClickedClose(false);

    if (kind == CastEngine::PairingKind::Pin) {
        dialog->setTitle(tr("Enter pairing PIN"));
        dialog->setMessage(tr("Enter the PIN shown on %1.").arg(peer));
        auto *edit = new DLineEdit(dialog);
        edit->setPlaceholderText(tr("4 or 8 digit PIN"));
        edit->setClearButtonEnabled(true);
        dialog->addContent(edit);
        const int connectIndex = dialog->addButton(tr("Connect"), true, DDialog::ButtonRecommend);
        dialog->addButton(tr("Cancel"));
        connect(edit->lineEdit(), &QLineEdit::textChanged, edit, [edit]() {
            edit->setAlert(false);
        });
        connect(dialog, &DDialog::buttonClicked, this,
                [this, dialog, edit, connectIndex](int index, const QString &) {
                    if (index != connectIndex) {
                        m_engine->cancelPairing();
                        dialog->close();
                        return;
                    }
                    const QString pin = edit->text().trimmed();
                    static const QRegularExpression pinRe(QStringLiteral("^[0-9]{4}$|^[0-9]{8}$"));
                    if (!pinRe.match(pin).hasMatch()) {
                        edit->setAlert(true);
                        edit->showAlertMessage(tr("Enter a 4- or 8-digit PIN"));
                        return;
                    }
                    m_engine->submitPairingPin(pin);
                    dialog->close();
                });
    } else {
        dialog->setTitle(tr("Confirm pairing"));
        dialog->setMessage(tr("Confirm the pairing request on %1.").arg(peer));
        dialog->addButton(tr("Cancel"));
        connect(dialog, &DDialog::buttonClicked, this, [this, dialog](int, const QString &) {
            m_engine->cancelPairing();
            dialog->close();
        });
    }

    connect(dialog, &QObject::destroyed, this, [this, dialog]() {
        if (m_pairingDialog == dialog)
            m_pairingDialog = nullptr;
    });
    m_pairingDialog = dialog;
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::closePairingDialog()
{
    if (!m_pairingDialog)
        return;
    DDialog *dialog = m_pairingDialog;
    m_pairingDialog = nullptr;
    dialog->close();
}
