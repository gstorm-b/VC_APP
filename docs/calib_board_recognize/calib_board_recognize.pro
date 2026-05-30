QT = core

CONFIG += c++17 cmdline

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        calibration/calibration_board.cpp \
        calibration/fanuc_irvision_board.cpp \
        calibration/calibration_board_factory.cpp \
        calibration/calibrator.cpp

HEADERS += \
        calibration/calibration_board.h \
        calibration/fanuc_irvision_board.h \
        calibration/calibration_board_factory.h \
        calibration/calibrator.h

INCLUDEPATH += $$PWD $$PWD/calibration

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -LC:/opencv/build/x64/vc16/lib/ -lopencv_world4110
else:win32:CONFIG(debug, debug|release): LIBS += -LC:/opencv/build/x64/vc16/lib/ -lopencv_world4110d
else:unix: LIBS += -LC:/opencv/build/x64/vc16/lib/ -lopencv_world4110

INCLUDEPATH += C:/opencv/build/include
DEPENDPATH += C:/opencv/build/include
