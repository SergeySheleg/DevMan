#-------------------------------------------------
#
# Project created by QtCreator 2015-10-17T14:18:57
#
#-------------------------------------------------

QT       += core gui svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = devman
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

LIBS     += -lsetupapi

RESOURCES += \
    res-icons.qrc
