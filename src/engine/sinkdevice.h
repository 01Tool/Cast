#pragma once

#include <QString>

struct SinkDevice
{
    QString id;
    QString name;
    QString address;
    QString p2pDevicePath;
    bool wfdCapable = false;
};
