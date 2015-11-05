#ifndef CLIENTSOCKETINFO_H
#define CLIENTSOCKETINFO_H

class ClientSocketInfo
{
public:
    ClientSocketInfo(int epol = -1, int sock = -1)
    {
        this->mepoll = epol;
        this->msocketFd = sock;
    }

    int epoll() const
    {
        return this->mepoll;
    }

    int socketfd() const
    {
        return this->msocketFd;
    }
private:
    int mepoll;
    int msocketFd;
};

#endif // CLIENTSOCKETINFO_H
