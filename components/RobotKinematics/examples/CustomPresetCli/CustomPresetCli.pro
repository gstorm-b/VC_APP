QT -= gui
QT += core

CONFIG += console c++17
CONFIG -= app_bundle

TEMPLATE = app
TARGET = CustomPresetCli

SOURCES += main.cpp

INCLUDEPATH += $$PWD/../../include
INCLUDEPATH += $$PWD/../../third_party/eigen

isEmpty(ROBOTKINEMATICS_LIB_DIR) {
    ROBOTKINEMATICS_LIB_DIR = $$PWD/../../build/msvc/lib
}

LIBS += -L$$ROBOTKINEMATICS_LIB_DIR -lRobotKinematics

win32-msvc {
    PRE_TARGETDEPS += $$ROBOTKINEMATICS_LIB_DIR/RobotKinematics.lib
}
