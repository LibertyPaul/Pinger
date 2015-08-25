#-------------------------------------------------
#
# Project created by QtCreator 2015-06-06T07:24:01
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = PingerInterface
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    pinger.cpp \
    pingtimeplot.cpp \
    QCustomPlot/qcustomplot.cpp \
    probabilitydensityplot.cpp \
    colorgenerator.cpp

HEADERS  += mainwindow.hpp \
    pinger.hpp \
    pingtimeplot.hpp \
    QCustomPlot/qcustomplot.h \
    pingresult.hpp \
    probabilitydensityplot.hpp \
    colorgenerator.hpp

FORMS    += mainwindow.ui \
    pingtimeplot.ui \
    probabilitydensityplot.ui

CONFIG += c++11
QMAKE_CXXFLAGS += -pthread
LIBS += -lpthread

win32: LIBS += -lWS2_32

win32: LIBS += -lIPHlpApi

