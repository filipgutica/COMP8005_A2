QT += core
QT -= gui

QMAKE_CXX       = g++

CONFIG += c++11

TARGET = Libevent_Server
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    server.cpp


QMAKE_CXXFLAGS += -levent

LIBS += -levent

HEADERS += \
    server.h \
    queue.h

