#pragma once

#include <QWidget>

class QLabel;

class QuickPanel : public QWidget
{
    Q_OBJECT

public:
    explicit QuickPanel(QWidget *parent = nullptr);
    void setStatus(const QString &text);
    void setActive(bool active);

Q_SIGNALS:
    void requestShowDetail();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QLabel *m_icon = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_status = nullptr;
};
