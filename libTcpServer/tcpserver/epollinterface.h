#ifndef EPOLLINTERFACE_H
#define EPOLLINTERFACE_H

#include "clientsocketinfo.h"

class EpollInterface
{
public:
    virtual ~EpollInterface(){}

    virtual int stopSocketToClose(const ClientSocketInfo &csi) = 0;
};

#endif // EPOLLINTERFACE_H
