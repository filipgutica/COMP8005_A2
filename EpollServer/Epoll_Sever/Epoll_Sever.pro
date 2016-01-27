#-------------------------------------------------
#
# Project created by QtCreator 2016-01-26T21:52:12
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Epoll_Sever
TEMPLATE = app


SOURCES += main.cpp\
        application.cpp \
    server.cpp

HEADERS  += application.h \
    server.h

FORMS    += application.ui
