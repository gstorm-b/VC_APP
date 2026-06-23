QT += core
QT -= gui

CONFIG += console c++17 warn_on
CONFIG -= app_bundle

TEMPLATE = app
TARGET = mesh_simplification

INCLUDEPATH += \
    ../../include \
    ../../third_party/eigen

isEmpty(ROBOTKINEMATICS_LIB_DIR): ROBOTKINEMATICS_LIB_DIR = $$(ROBOTKINEMATICS_LIB_DIR)
isEmpty(ROBOTKINEMATICS_LIB_DIR) {
    error("ROBOTKINEMATICS_LIB_DIR must point to the built RobotKinematics library directory")
}

win32 {
    LIBS += -L$$ROBOTKINEMATICS_LIB_DIR -lRobotKinematics
    PRE_TARGETDEPS += $$ROBOTKINEMATICS_LIB_DIR/RobotKinematics.lib
}

SOURCES += \
    main.cpp
