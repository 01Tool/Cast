#include "tray/quickpanel.h"

#include <DFontSizeManager>
#include <DIconTheme>
#include <DLabel>

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QVBoxLayout>

#include "constants.h"

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

QuickPanel::QuickPanel(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(Dock::QUICK_ITEM_HEIGHT);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(0);

    m_icon = new DLabel(this);
    m_icon->setFixedSize(Dock::QUICK_PANEL_ICON_SIZE);
    m_icon->setAlignment(Qt::AlignCenter);
    m_icon->setPixmap(DIconTheme::findQIcon(QStringLiteral("ot-cast"),
                                            DIconTheme::findQIcon(QStringLiteral("video-display")))
                          .pixmap(Dock::QUICK_PANEL_ICON_SIZE));

    m_title = new DLabel(tr("Cast"), this);
    m_title->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_title, DFontSizeManager::T8);

    m_status = new DLabel(tr("Idle"), this);
    m_status->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_status, DFontSizeManager::T9);

    auto *iconRow = new QWidget(this);
    auto *iconLayout = new QHBoxLayout(iconRow);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    iconLayout->addStretch();
    iconLayout->addWidget(m_icon);
    iconLayout->addStretch();

    layout->addWidget(iconRow);
    layout->addWidget(m_title);
    layout->addWidget(m_status);
}

void QuickPanel::setStatus(const QString &text)
{
    m_status->setText(text);
}

void QuickPanel::setActive(bool active)
{
    Q_UNUSED(active);
}

void QuickPanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        Q_EMIT requestShowDetail();
    QWidget::mouseReleaseEvent(event);
}