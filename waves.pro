TEMPLATE = app
TARGET = waves

QT += qml quick widgets opengl openglextensions
CONFIG += c++11

QMAKE_CXXFLAGS += -stdlib=libc++

SOURCES += main.cpp \
    waves.cpp \
    simulator.cpp \
    cpglquads.cpp \
    cpgrid.cpp \
    wavesolver.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    waves.h \
    simulator.h \
    cpglquads.h \
    cpgrid.h \
    wavesolver.h
