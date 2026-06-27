QT       += core testlib
QT       -= gui

CONFIG   += c++17 console testcase
CONFIG   -= app_bundle

TARGET    = calib_tests

INCLUDEPATH += $$PWD/..

SOURCES += \
    tst_main.cpp \
    ../calibration_board.cpp \
    ../fanuc_irvision_board.cpp \
    ../calibration_board_factory.cpp \
    ../calibrator.cpp

HEADERS += \
    ../calibration_board.h \
    ../fanuc_irvision_board.h \
    ../calibration_board_factory.h \
    ../calibrator.h

include($$PWD/../../../qmake/opencv_dependency.pri)
