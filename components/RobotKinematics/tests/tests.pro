TEMPLATE = app
TARGET = RobotKinematicsTests

QT += core testlib
QT -= gui

CONFIG += console c++17 warn_on
CONFIG -= app_bundle

INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../third_party/eigen

include(../mesh_collision_backend.pri)

LIBS += -L$$OUT_PWD/../lib -lRobotKinematics

DESTDIR = $$OUT_PWD
OBJECTS_DIR = $$OUT_PWD/.obj
MOC_DIR = $$OUT_PWD/.moc
RCC_DIR = $$OUT_PWD/.rcc
UI_DIR = $$OUT_PWD/.ui

SOURCES += \
    TestMain.cpp \
    unit/DhAdapterTests.cpp \
    unit/PoseTests.cpp \
    unit/RobotModelConfigTests.cpp \
    unit/RobotModelValidatorTests.cpp \
    unit/SmokeTests.cpp \
    unit/UnitsTests.cpp \
    unit/JointLimitValidatorTests.cpp \
    unit/FrameToolTests.cpp \
    unit/ForwardKinematicsTests.cpp \
    unit/Robot3DVisualizerLogicTests.cpp \
    unit/IKApiTests.cpp \
    unit/CollisionApiTests.cpp \
    unit/CollisionBackendTests.cpp \
    unit/CollisionProfileValidatorTests.cpp \
    unit/CollisionCheckerTests.cpp \
    unit/CollisionPrimitiveDistanceTests.cpp \
    unit/StlMeshLoaderTests.cpp \
    unit/StlPrimitiveAuthoringHelperTests.cpp \
    unit/IKSolutionRankerTests.cpp \
    unit/NumericalIKSolverTests.cpp \
    unit/PostureResolverTests.cpp \
    unit/AnalyticIKSolverTests.cpp \
    unit/UrdfAdapterTests.cpp \
    integration/CustomPresetTests.cpp \
    integration/CollisionProfileJsonTests.cpp \
    integration/MeshCollisionProfileJsonTests.cpp \
    integration/NachiMeshCollisionTests.cpp \
    integration/FrameToolFkTests.cpp \
    integration/IKIntegrationTests.cpp \
    integration/Virtual6DofTestArmTests.cpp \
    integration/NachiMZ04DTests.cpp

HEADERS += \
    unit/DhAdapterTests.h \
    unit/PoseTests.h \
    unit/RobotModelConfigTests.h \
    unit/RobotModelValidatorTests.h \
    unit/SmokeTests.h \
    unit/UnitsTests.h \
    unit/JointLimitValidatorTests.h \
    unit/FrameToolTests.h \
    unit/ForwardKinematicsTests.h \
    unit/Robot3DVisualizerLogicTests.h \
    unit/IKApiTests.h \
    unit/CollisionApiTests.h \
    unit/CollisionBackendTests.h \
    unit/CollisionProfileValidatorTests.h \
    unit/CollisionCheckerTests.h \
    unit/CollisionPrimitiveDistanceTests.h \
    unit/StlMeshLoaderTests.h \
    unit/StlPrimitiveAuthoringHelperTests.h \
    unit/IKSolutionRankerTests.h \
    unit/NumericalIKSolverTests.h \
    unit/PostureResolverTests.h \
    unit/AnalyticIKSolverTests.h \
    unit/UrdfAdapterTests.h \
    integration/CustomPresetTests.h \
    integration/CollisionProfileJsonTests.h \
    integration/MeshCollisionProfileJsonTests.h \
    integration/NachiMeshCollisionTests.h \
    integration/FrameToolFkTests.h \
    integration/IKIntegrationTests.h \
    integration/Virtual6DofTestArmTests.h \
    integration/NachiMZ04DTests.h
