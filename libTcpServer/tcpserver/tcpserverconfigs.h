#ifndef TCPSERVERCONFIGS_H
#define TCPSERVERCONFIGS_H

#define USING_LOG_4_QT      (1)

/**
* marco for using logger easily
*/
#if USING_LOG_4_QT
#include "log4qt/logger.h"
#define DECLEAR_TCPSERVER_LOGGER    Log4Qt::Logger *logger = Log4Qt::Logger::logger("TcpServer")
#define LOGGER_DEBUG    logger->debug
#define LOGGER_WARN     logger->warn
#define LOGGER_ERROR    logger->error
#else
#define DECLEAR_TCPSERVER_LOGGER
#define LOGGER_DEBUG
#define LOGGER_WARN
#define LOGGER_ERROR
#endif

/**
* marco for socket configs
*/
#define MAX_TCP_LISTEN_QUEUE_SIZE   (128)
#define MAX_EPOLL_EVENT_COUNT   (65535)
#define EPOLL_TIMEOUT_MS    (300)

#endif // TCPSERVERCONFIGS_H
