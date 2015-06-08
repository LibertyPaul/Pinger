#-------------------------------------------------
#
# Project created by QtCreator 2015-06-06T07:24:01
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PingerInterface
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    pinger.cpp

HEADERS  += mainwindow.hpp \
    pinger.hpp

FORMS    += mainwindow.ui

CONFIG += c++11
QMAKE_CXXFLAGS += -pthread
LIBS += -lpthread
