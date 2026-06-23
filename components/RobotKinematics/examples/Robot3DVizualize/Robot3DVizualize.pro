QT += widgets openglwidgets

TEMPLATE = app
TARGET = Robot3DVizualize

CONFIG += c++17 warn_on

include(vtk_config.pri)

INCLUDEPATH += \
    $$PWD/../../include \
    $$PWD/../../third_party/eigen

# ROBOTKINEMATICS_LIB_DIR selects which RobotKinematics build to link against. The
# default targets the no-mesh-backend MSVC build. To exercise the mesh backend
# selector in this example, build the Coal-enabled library and pass
#   qmake ... "ROBOTKINEMATICS_LIB_DIR=<repo>/build/msvc_mesh_coal/lib"
isEmpty(ROBOTKINEMATICS_LIB_DIR): ROBOTKINEMATICS_LIB_DIR = $$(ROBOTKINEMATICS_LIB_DIR)
isEmpty(ROBOTKINEMATICS_LIB_DIR): ROBOTKINEMATICS_LIB_DIR = $$PWD/../../build/msvc/lib

LIBS += -L$$ROBOTKINEMATICS_LIB_DIR -lRobotKinematics
PRE_TARGETDEPS += $$ROBOTKINEMATICS_LIB_DIR/RobotKinematics.lib

# Pull in optional mesh backend link flags (Coal/Boost/Assimp) when the example
# is linked against a Coal-enabled RobotKinematics build. Activated via
#   qmake ... "CONFIG+=robotkinematics_mesh_collision" "MESH_COLLISION_BACKEND=coal" ...
include($$PWD/../../mesh_collision_backend.pri)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Robot3DVisualizerLogic.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
