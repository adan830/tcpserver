#ifndef TCPHANDLERTHREAD_H
#define TCPHANDLERTHREAD_H

#include <pthread.h>
#include "abstracttcphandler.h"
#include "epollinterface.h"
#include <queue>
using namespace std;

void *tcpHandlerThreadImp(void *handlerThread);

class TcpHandlerThread: public EpollInterface
{
public:
    TcpHandlerThread();
    virtual ~TcpHandlerThread();

    int init(int serverFd, pthread_mutex_t *acceptLocker, AbstractTcpHandler *tcpHandler);
    int start();
    int stop();
    int stopSocketToClose(const ClientSocketInfo &csi);
protected:
    virtual int acceptClientSocket();
    virtual int createEpoll();
    virtual int addClientSocketToEpoll(int fd);
    virtual int removeClientSocketFromEpoll(int fd);
    int setSocketNoBlock(int fd, bool noblock);
private:
    friend void *tcpHandlerThreadImp(void *handlerThread);
private:
    pthread_mutex_t *macceptLocker;
    int mserverFd;
    AbstractTcpHandler *mtcpHandler;
    int mepollFd;

    pthread_mutex_t mepollSocketLocker;
    queue<ClientSocketInfo> msocketsToStop;

    int mthreadStateFlags;
    pthread_t mthreadId;
};

#endif // TCPHANDLERTHREAD_H
