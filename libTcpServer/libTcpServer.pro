#-------------------------------------------------
#
# Project created by QtCreator 2015-07-10T21:36:15
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = libTcpServer
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

include(tcpserver/tcpserver.pri)

SOURCES += main.cpp
