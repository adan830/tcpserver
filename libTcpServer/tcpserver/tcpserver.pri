include(../log4qt/log4qt.pri)

HEADERS += \
    $$PWD/epolltcpserver.h \
    $$PWD/abstracttcphandler.h \
    $$PWD/tcpserverconfigs.h \
    $$PWD/tcphandlerthread.h \
    $$PWD/epollinterface.h \
    $$PWD/clientsocketinfo.h

SOURCES += \
    $$PWD/epolltcpserver.cpp \
    $$PWD/tcphandlerthread.cpp
