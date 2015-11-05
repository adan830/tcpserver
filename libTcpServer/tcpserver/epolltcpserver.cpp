#include "epolltcpserver.h"
#include "tcpserverconfigs.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

EpollTcpServer::EpollTcpServer()
{
    this->mserverFd = -1;
    pthread_mutex_init(&mserverSocketMutex, NULL);
}

EpollTcpServer::~EpollTcpServer()
{
    for(unsigned int i = 0; i < this->mthreads.size(); ++i)
    {
         delete this->mthreads[i];
    }
    this->mthreads.clear();

    if(this->mserverFd != -1)
    {
        close(this->mserverFd);
    }
}

int EpollTcpServer::init(int port, AbstractTcpHandler *handler, int threadCount)
{
    DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;

    if(threadCount <= 0)
    {
        LOGGER_WARN("EpollTcpServer::init: threadCount < 0.\n");
        return -1;
    }

    this->mserverPort = port;
    this->mtcpHandler = handler;

    ret = this->createSocket(port);
    if(ret < 0)
    {
        LOGGER_WARN("EpollTcpServer::init: failed to create server socket on port: %1.\n", port);
        ret = -1;
    }
    else
    {
        ret = createHandlerThread(threadCount);
        if(ret < 0)
        {
            LOGGER_WARN("EpollTcpServer::init: failed to create handler thread.\n");
            ret = -2;
        }
    }

    return ret;
}

int EpollTcpServer::startServer()
{
    //DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;

    for(unsigned int i = 0;i < this->mthreads.size(); ++i)
    {
        this->mthreads[i]->start();
    }

    return ret;
}

int EpollTcpServer::stopServer()
{
    //DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;

    for(unsigned int i = 0; i < this->mthreads.size(); ++i)
    {
        this->mthreads[i]->stop();
    }

    return ret;
}

int EpollTcpServer::createSocket(int port)
{
    DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;
    struct sockaddr_in servaddr;

    this->mserverFd = socket(AF_INET, SOCK_STREAM, 0);
    if(this->mserverFd < 0)
    {
        LOGGER_WARN("EpollTcpServer::createSocket: failed to create server socket fd.\n");
        ret = -1;
    }
    else
    {
        bzero(&servaddr,sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        int reuseFlag = 1;
        if(setsockopt(this->mserverFd, SOL_SOCKET, SO_REUSEADDR, &reuseFlag, sizeof(reuseFlag)) < 0)
        {
            LOGGER_WARN("EpollTcpServer::createSocket: failed to set socket reuse option.\n");
            ret =  -2;
        }
        else
        {
            if(this->setSocketNoBlock(this->mserverFd, true) < 0)
            {
                LOGGER_WARN("EpollTcpServer::createSocket: failed to set socket to NONBLOCK.\n");
                ret = -3;
            }
            if(bind(this->mserverFd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0)
            {
                LOGGER_WARN("EpollTcpServer::createSocket: bind failed: %1.\n", strerror(errno));
                ret = -4;
            }
            else
            {
                if(listen(this->mserverFd, MAX_TCP_LISTEN_QUEUE_SIZE))
                {
                    LOGGER_WARN("EpollTcpServer::createSocket: failed to listen: %1.\n", strerror(errno));
                    ret = -5;
                }
            }
        }
    }

    return ret;
}

int EpollTcpServer::createHandlerThread(int threadCount)
{
    DECLEAR_TCPSERVER_LOGGER;
    int ret = 0;
    int newIndex = 0;

    for(newIndex = 0;newIndex < threadCount; ++newIndex)
    {

        TcpHandlerThread *tht = new TcpHandlerThread;
        ret = tht->init(this->mserverFd, &mserverSocketMutex, this->mtcpHandler);
        if(ret < 0)
        {
            LOGGER_WARN("EpollTcpServer::createHandlerThread: failed to init tcp handler thread.\n");
            delete tht;
            break;
        }
        else
        {
            this->mthreads.push_back(tht);
        }
    }

    // if could not alloc as many threads as the threadCount says, delete all the threads already alloced.
    if(newIndex < threadCount)
    {
        for(int j = 0; j < newIndex; ++j)
        {
            delete this->mthreads[j];
        }

        this->mthreads.clear();

        ret = -1;
    }

    return ret;
}

int EpollTcpServer::setSocketNoBlock(int fd, bool noblock)
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
