QT += core
QT -= gui

QMAKE_CXX       = g++
QMAKE_CXXFLAGS += -levent

CONFIG += c++11

TARGET = Libevent_Server
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    server.cpp

HEADERS += \
    server.h \
    queue.h

