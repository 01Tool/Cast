#pragma once

#include <QString>

struct SinkDevice
{
    QString id;
    QString name;
    QString address;
    bool wfdCapable = false;
};
