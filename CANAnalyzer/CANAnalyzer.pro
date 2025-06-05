QT += core gui widgets

CONFIG += c++11
TARGET = CANAnalyzer
TEMPLATE = app

SOURCES += main.cpp \
           libs/qcustomplot/qcustomplot.cpp \
           libs/pcanbasic/PCANBasic.cpp \
           can_handler.cpp \
           motor_controller.cpp \
           mainwindow.cpp

HEADERS += libs/pcanbasic/PCANBasic.h \
           libs/qcustomplot/qcustomplot.h \
           can_handler.h \
           motor_controller.h \
           mainwindow.h

# QCustomPlot requires its own source file to be compiled if not used as a library.
# If qcustomplot.cpp was added to SOURCES when it's also part of a precompiled library,
# it could lead to duplicate symbols. But here, we are compiling it directly.

# PCANBasic SDK Integration
INCLUDEPATH += $$PWD/libs/pcanbasic

# QCustomPlot Integration
INCLUDEPATH += $$PWD/libs/qcustomplot

# Ensure our project's root is in include path for our own headers
INCLUDEPATH += $$PWD

# Executable will be created in the build directory by default
# DESTDIR = bin
