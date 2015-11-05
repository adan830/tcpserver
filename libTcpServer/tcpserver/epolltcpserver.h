#ifndef EPOLLTCPSERVER_H
#define EPOLLTCPSERVER_H

#include "abstracttcphandler.h"
#include "tcphandlerthread.h"
#include <vector>
using namespace std;

class EpollTcpServer
{
public:
    EpollTcpServer();
    ~EpollTcpServer();

    int init(int port, AbstractTcpHandler *handler, int threadCount = 1);
    int startServer();
    int stopServer();
private:
    int createSocket(int port);
    int createHandlerThread(int threadCount);
    int setSocketNoBlock(int fd, bool noblock);
private:
    int mserverPort;
    int mserverFd;
    AbstractTcpHandler *mtcpHandler;
    vector<TcpHandlerThread *> mthreads;
    int mthreadCount;
    pthread_mutex_t mserverSocketMutex;
};

#endif // EPOLLTCPSERVER_H
