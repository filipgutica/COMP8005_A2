QT += core
QT -= gui

CONFIG += c++11

TARGET = QtServer_Console
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    server.cpp

HEADERS += \
    server.h
