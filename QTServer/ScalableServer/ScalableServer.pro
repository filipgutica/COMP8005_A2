#-------------------------------------------------
#
# Project created by QtCreator 2016-01-20T21:35:49
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ScalableServer
CONFIG += console
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    serverthread.cpp

HEADERS  += mainwindow.h \
    serverthread.h

FORMS    += mainwindow.ui
