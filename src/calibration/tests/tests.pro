QT       += core testlib
QT       -= gui

CONFIG   += c++17 console testcase
CONFIG   -= app_bundle

TARGET    = calib_tests

INCLUDEPATH += $$PWD/.. $$PWD/../calibration

SOURCES += \
    tst_main.cpp \
    ../calibration/calibration_board.cpp \
    ../calibration/fanuc_irvision_board.cpp \
    ../calibration/calibration_board_factory.cpp \
    ../calibration/calibrator.cpp

HEADERS += \
    ../calibration/calibration_board.h \
    ../calibration/fanuc_irvision_board.h \
    ../calibration/calibration_board_factory.h \
    ../calibration/calibrator.h

win32:CONFIG(release, debug|release): LIBS += -LC:/opencv/build/x64/vc16/lib/ -lopencv_world4110
else:win32:CONFIG(debug, debug|release): LIBS += -LC:/opencv/build/x64/vc16/lib/ -lopencv_world4110d
else:unix: LIBS += -LC:/opencv/build/x64/vc16/lib/ -lopencv_world4110

INCLUDEPATH += C:/opencv/build/include
DEPENDPATH  += C:/opencv/build/include
