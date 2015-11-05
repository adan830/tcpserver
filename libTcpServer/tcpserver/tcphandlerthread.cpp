#include "tcphandlerthread.h"
#include "tcpserverconfigs.h"
#include <sys/epoll.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>

#define THREAD_STATE_INIT   (1)
#define THREAD_STATE_RUNNING    (2)
#define THREAD_STATE_STOP   (3)

void *tcpHandlerThreadImp(void *handlerThread)
{
    DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;
    TcpHandlerThread *param = (TcpHandlerThread *)handlerThread;
    struct epoll_event events[MAX_EPOLL_EVENT_COUNT];
    int client_sockfd;

    while (param->mthreadStateFlags == THREAD_STATE_RUNNING)
    {
        // try accept new client
        ret = pthread_mutex_trylock(param->macceptLocker);
        if(ret == 0)
        {
            ret = param->acceptClientSocket();
            if(ret < 0)
            {
                LOGGER_WARN("TcpHandlerThread::tcpHandlerThreadImp: failed to accept client socket.\n");
            }

            pthread_mutex_unlock(param->macceptLocker);
        }

        // to handle client socket events
        ret = epoll_wait(param->mepollFd, events, MAX_EPOLL_EVENT_COUNT, EPOLL_TIMEOUT_MS);
        if(ret < 0)
        {
            LOGGER_WARN("TcpHandlerThread::tcpHandlerThreadImp: start epoll_wait failed: %1.\n", strerror(errno));
        }
        else
        {
            for(int i = 0; i < ret; ++i)
            {
                client_sockfd = events[i].data.fd;

                if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (events[i].events & EPOLLRDHUP))
                {
                    param->removeClientSocketFromEpoll(client_sockfd);
                    ret = param->mtcpHandler->onsocketClose(client_sockfd);
                    if(ret < 0)
                    {
                        LOGGER_WARN("TcpHandlerThread::tcpHandlerThreadImp: failed to close socket: %d.\n", client_sockfd);
                    }
                }
                else if(events[i].events & EPOLLIN)
                {
                    ret = param->mtcpHandler->ondataIncome(client_sockfd);
                    if(ret < 0)
                    {
                        param->removeClientSocketFromEpoll(client_sockfd);
                        ret = param->mtcpHandler->onsocketClose(client_sockfd);
                        if(ret < 0)
                        {
                            LOGGER_WARN("TcpHandlerThread::tcpHandlerThreadImp: failed to close socket: %d.\n", client_sockfd);
                        }
                    }
                }
            }
        }

        // to remove the sockets to be closed from epoll
        pthread_mutex_lock(&(param->mepollSocketLocker));
        while(! param->msocketsToStop.empty())
        {
            ClientSocketInfo csi = param->msocketsToStop.front();
            if(csi.epoll() == param->mepollFd)
            {
                param->removeClientSocketFromEpoll(csi.socketfd());
                ret = param->mtcpHandler->onsocketClose(csi.socketfd());
                if(ret < 0)
                {
                    LOGGER_WARN("TcpHandlerThread::tcpHandlerThreadImp: failed to close socket: %d.\n", client_sockfd);
                }
            }

            param->msocketsToStop.pop();
        }
        pthread_mutex_unlock(&(param->mepollSocketLocker));
    }

    return NULL;
}

TcpHandlerThread::TcpHandlerThread()
{
    mserverFd = -1;
    mtcpHandler = NULL;
    macceptLocker = NULL;
    mthreadStateFlags = THREAD_STATE_INIT;
    mepollFd = -1;
}

TcpHandlerThread::~TcpHandlerThread()
{
    this->stop();
}

int TcpHandlerThread::init(int serverFd, pthread_mutex_t *acceptLocker, AbstractTcpHandler *tcpHandler)
{
    this->mserverFd = serverFd;
    this->macceptLocker = acceptLocker;
    this->mtcpHandler = tcpHandler;

    tcpHandler->registerEpollInterface(this);

    return this->createEpoll();
}

int TcpHandlerThread::start()
{
    DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;

    this->mthreadStateFlags = THREAD_STATE_RUNNING;
    ret = pthread_create(&mthreadId, NULL, tcpHandlerThreadImp, (void *)this);
    if(ret < 0)
    {
        LOGGER_WARN("TcpHandlerThread::start: failed to create thread.\n");
        ret = -1;
    }

    return ret;
}

int TcpHandlerThread::stop()
{
//    DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;

    if(this->mthreadStateFlags == THREAD_STATE_RUNNING)
    {
        this->mthreadStateFlags = THREAD_STATE_STOP;

        pthread_join(this->mthreadId, NULL);
    }

    return ret;
}

int TcpHandlerThread::stopSocketToClose(const ClientSocketInfo &csi)
{
    pthread_mutex_lock(&mepollSocketLocker);

    this->msocketsToStop.push(csi);

    pthread_mutex_unlock(&mepollSocketLocker);

    return 0;
}

int TcpHandlerThread::acceptClientSocket()
{
    DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;
    int clientSocket;
    struct sockaddr_in clientAddr;
    socklen_t len;

    len = sizeof(clientAddr);
    clientSocket = accept(this->mserverFd, (struct sockaddr *)&clientAddr, &len);
    if(clientSocket < 0)
    {
        if(errno != EAGAIN)
        {
            LOGGER_WARN("TcpHandlerThread::acceptClientSocket: try to accept client failed: %1.\n", strerror(errno));
            ret = -1;
        }
    }
    else
    {
        char ip[17];
        bzero(ip, sizeof(ip));
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, 16);

        ret = this->mtcpHandler->onsocketIncome(ClientSocketInfo(this->mepollFd, clientSocket), ip);
        if(ret < 0)
        {
            LOGGER_WARN("TcpHandlerThread::acceptClientSocket: failed to handler income socket.\n");
            ret = -2;
        }
        else
        {
            ret = this->setSocketNoBlock(clientSocket, true);
            if(ret < 0)
            {
                LOGGER_WARN("TcpHandlerThread::acceptClientSocket: failed to set client socket(%1) to NONBLOCK.\n", clientSocket);
                ret = -3;
            }
            else
            {
                ret = this->addClientSocketToEpoll(clientSocket);
                if(ret < 0)
                {
                    LOGGER_WARN("TcpHandlerThread::acceptClientSocket: failed to add socket to epoll.\n");
                    ret = -4;
                }
                else
                {
                    ret = 0;
                }
            }
        }
    }

    return ret;
}

int TcpHandlerThread::createEpoll()
{
    DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;

    this->mepollFd = epoll_create(MAX_EPOLL_EVENT_COUNT);
    if(this->mepollFd == -1)
    {
        LOGGER_ERROR("TcpHandlerThread::createEpoll: epoll_create failed: %1.\n", strerror(errno));
        ret = -1;
    }

    return ret;
}

int TcpHandlerThread::addClientSocketToEpoll(int fd)
{
    DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;
    struct epoll_event ev;

    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    ev.data.fd = fd;
    if( epoll_ctl(this->mepollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        LOGGER_WARN("TcpHandlerThread::addClientSocketToEpoll: register client socket to epoll failed: %1.\n", strerror(errno));
        ret = -1;
    }

    return ret;
}

int TcpHandlerThread::removeClientSocketFromEpoll(int fd)
{
    DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;
    struct epoll_event ev;

    if( epoll_ctl(this->mepollFd, EPOLL_CTL_DEL, fd, &ev) == -1)
    {
        LOGGER_WARN("TcpHandlerThread::removeClientSocketFromEpoll: failed to delete socket from epoll: %1.\n", strerror(errno));
        ret = -1;
    }

    return ret;
}

int TcpHandlerThread::setSocketNoBlock(int fd, bool noblock)
{
    DECLEAR_TCPSERVER_LOGGER;
    int flags = fcntl(fd, F_GETFL, 0);

    if(noblock)
    {
        flags |= O_NONBLOCK;
    }
    else
    {
        flags &= (~ O_NONBLOCK);
    }

    if(fcntl(fd, F_SETFL, flags) == -1)
    {
        LOGGER_WARN("EpollTcpServer::createSocket: failed to set socket to %1.\n", noblock ? "NO block":"block");
        return -1;
    }

    return 0;
}
