# -------------------------------------------------
# Project created by QtCreator 2012-03-18T16:48:28
# -------------------------------------------------
QT += sql
QT -= gui
TARGET = omega_dbs
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    dbs.cpp
CONFIG(debug, debug|release):LIBS += -lqextserialportd
else:LIBS += -lqextserialport
unix:DEFINES = _TTY_POSIX_
win32:DEFINES = _TTY_WIN_
HEADERS += dbs.h
