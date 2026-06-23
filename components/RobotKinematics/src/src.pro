TEMPLATE = lib
TARGET = RobotKinematics

QT += core
QT -= gui

CONFIG += staticlib c++17 warn_on

INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../third_party/eigen

include(../mesh_collision_backend.pri)

DESTDIR = $$OUT_PWD/../lib
OBJECTS_DIR = $$OUT_PWD/.obj
MOC_DIR = $$OUT_PWD/.moc
RCC_DIR = $$OUT_PWD/.rcc
UI_DIR = $$OUT_PWD/.ui

SOURCES += \
    Adapters/DhAdapter.cpp \
    Adapters/UrdfAdapter.cpp \
    Collision/CollisionBackend.cpp \
    Collision/BuiltInCollisionProfiles.cpp \
    Collision/CollisionChecker.cpp \
    Collision/CollisionProfileJsonLoader.cpp \
    Collision/CollisionProfileValidator.cpp \
    Collision/MeshCollisionProfile.cpp \
    Collision/MeshCollisionProfileJsonLoader.cpp \
    Collision/MeshCollisionProfileValidator.cpp \
    Collision/StlMeshLoader.cpp \
    Collision/StlPrimitiveAuthoringHelper.cpp \
    Core/LibraryAnchor.cpp \
    Core/Pose.cpp \
    Core/Units.cpp \
    Kinematics/ForwardKinematics.cpp \
    Kinematics/JointLimitValidator.cpp \
    Kinematics/SerialRobotKinematics.cpp \
    Model/SerialRobotConfigBuilder.cpp \
    Model/RobotModelValidator.cpp \
    Posture/PostureResolver.cpp \
    Presets/PresetJsonLoader.cpp \
    Presets/Virtual6DofTestArm.cpp \
    Presets/NachiMZ04D.cpp \
    Solvers/IKSolutionRanker.cpp \
    Solvers/NumericalIKSolver.cpp \
    Solvers/Analytic6DofSphericalWristSolver.cpp

contains(CONFIG, robotkinematics_mesh_collision):equals(MESH_COLLISION_BACKEND, coal) {
    SOURCES += Collision/CoalMeshCollisionBackend.cpp
}

HEADERS += \
    ../include/RobotKinematics/Adapters/DhAdapter.h \
    ../include/RobotKinematics/Adapters/UrdfAdapter.h \
    ../include/RobotKinematics/Core/Ids.h \
    ../include/RobotKinematics/Core/JointVector.h \
    ../include/RobotKinematics/Core/Pose.h \
    ../include/RobotKinematics/Core/Result.h \
    ../include/RobotKinematics/Core/Units.h \
    ../include/RobotKinematics/Collision/CollisionBackend.h \
    ../include/RobotKinematics/Collision/CollisionChecker.h \
    ../include/RobotKinematics/Collision/CollisionGeometry.h \
    ../include/RobotKinematics/Collision/CollisionProfile.h \
    ../include/RobotKinematics/Collision/BuiltInCollisionProfiles.h \
    ../include/RobotKinematics/Collision/CollisionProfileJsonLoader.h \
    ../include/RobotKinematics/Collision/CollisionProfileValidator.h \
    ../include/RobotKinematics/Collision/MeshCollisionProfile.h \
    ../include/RobotKinematics/Collision/MeshCollisionProfileJsonLoader.h \
    ../include/RobotKinematics/Collision/MeshCollisionProfileValidator.h \
    ../include/RobotKinematics/Collision/StlMeshLoader.h \
    ../include/RobotKinematics/Collision/StlPrimitiveAuthoringHelper.h \
    ../include/RobotKinematics/Collision/TriangleMesh.h \
    ../include/RobotKinematics/Kinematics/ForwardKinematics.h \
    ../include/RobotKinematics/Kinematics/InverseKinematics.h \
    ../include/RobotKinematics/Kinematics/JointLimitValidator.h \
    ../include/RobotKinematics/Kinematics/RobotKinematics.h \
    ../include/RobotKinematics/Kinematics/SerialRobotKinematics.h \
    ../include/RobotKinematics/Model/FrameRegistry.h \
    ../include/RobotKinematics/Model/RobotModelConfig.h \
    ../include/RobotKinematics/Model/RobotModelValidator.h \
    ../include/RobotKinematics/Model/SerialRobotConfigBuilder.h \
    ../include/RobotKinematics/Model/ToolRegistry.h \
    ../include/RobotKinematics/Posture/ArmPosture.h \
    ../include/RobotKinematics/Posture/PostureResolver.h \
    ../include/RobotKinematics/Presets/PresetJsonLoader.h \
    ../include/RobotKinematics/Presets/Virtual6DofTestArm.h \
    ../include/RobotKinematics/Presets/NachiMZ04D.h \
    ../include/RobotKinematics/Solvers/IKSolutionRanker.h \
    ../include/RobotKinematics/Solvers/IKSolver.h \
    ../include/RobotKinematics/Solvers/NumericalIKSolver.h \
    ../include/RobotKinematics/Solvers/AnalyticIKSolver.h \
    ../include/RobotKinematics/Solvers/Analytic6DofSphericalWristSolver.h
