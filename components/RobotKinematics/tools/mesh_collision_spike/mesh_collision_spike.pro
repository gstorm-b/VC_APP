QT += core
QT -= gui

CONFIG += console c++17 warn_on
CONFIG -= app_bundle

TEMPLATE = app
TARGET = mesh_collision_spike

include(../../examples/Robot3DVizualize/vtk_config.pri)

SOURCES += \
    main.cpp

VTK_EXTRA_LIBS += \
    vtkCommonTransforms \
    vtkFiltersCore \
    vtkFiltersModeling

for(vtkLib, VTK_EXTRA_LIBS) {
    LIBS += -l$${vtkLib}$${VTK_LIB_SUFFIX}
}
