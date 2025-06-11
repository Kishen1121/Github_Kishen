QT += core gui widgets

CONFIG += c++11
TARGET = CANAnalyzer
TEMPLATE = app

SOURCES = main.cpp \
          libs/qcustomplot/qcustomplot.cpp \
          can_handler.cpp \
          motor_controller.cpp \
          mainwindow.cpp

# Conditionally compile the dummy PCANBasic.cpp for non-Windows platforms
# On Windows, we expect to link against a pre-compiled PCANBasic.lib
!win32 {
    SOURCES += libs/pcanbasic/PCANBasic.cpp
    message("Including dummy PCANBasic.cpp for non-Windows build")
} else {
    message("Excluding dummy PCANBasic.cpp for Windows build, expecting linked library")
}

HEADERS += libs/pcanbasic/PCANBasic.h \
           libs/qcustomplot/qcustomplot.h \
           can_handler.h \
           motor_controller.h \
           mainwindow.h

# QCustomPlot requires its own source file to be compiled if not used as a library.
# If qcustomplot.cpp was added to SOURCES when it's also part of a precompiled library,
# it could lead to duplicate symbols. But here, we are compiling it directly.

# PCANBasic SDK Integration
# The following INCLUDEPATH is for the dummy PCANBasic.h.
# For a real SDK, ensure PCANBasic.h is found, either by replacing the dummy
# or by adjusting paths in the platform-specific blocks below.
INCLUDEPATH += $$PWD/libs/pcanbasic
# When using a real PCANBasic SDK (linked library), remove libs/pcanbasic/PCANBasic.cpp from SOURCES.
# Currently, it's compiled for the dummy version.

win32 {
    message("Configuring for Windows build (linking real PCANBasic.lib)")
    # Example for linking PCANBasic.lib on Windows. User must place the .lib file here.
    # Adjust x64/x86 path and lib name if necessary.
    LIBS += -L$$PWD/libs/pcanbasic/lib/win_x64/ -lPCANBasic -lgdi32
    # Ensure INCLUDEPATH points to the directory containing the real PCANBasic.h
    # This might be the same as the general one if the dummy header is replaced.
    # INCLUDEPATH += $$PWD/libs/pcanbasic
    DEPENDPATH += $$PWD/libs/pcanbasic # For header dependencies
}
# Example for Linux (if you were to link a .so):
# unix:!macx {
#     message("Configuring for Linux build (linking real libpcanbasic.so)")
#     # LIBS += -L$$PWD/libs/pcanbasic/lib/linux_x64/ -lpcanbasic
#     # INCLUDEPATH += $$PWD/libs/pcanbasic
#     # DEPENDPATH += $$PWD/libs/pcanbasic
# }


# QCustomPlot Integration
INCLUDEPATH += $$PWD/libs/qcustomplot

# Ensure our project's root is in include path for our own headers
INCLUDEPATH += $$PWD

# Executable will be created in the build directory by default
# DESTDIR = bin
