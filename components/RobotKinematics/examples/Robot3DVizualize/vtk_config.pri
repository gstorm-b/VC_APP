# External VTK configuration for the Robot3DVizualize example.
#
# VTK is intentionally not vendored and not required by the core library build.
# Provide VTK_ROOT and, if needed, VTK_VERSION from the environment or qmake command line:
#
#   set VTK_ROOT=C:\path\to\vtk-install
#   set VTK_VERSION=9.6
#
# Optional overrides:
#   VTK_INCLUDEPATH, VTK_LIBPATH, VTK_BINPATH, VTK_LIB_SUFFIX

isEmpty(VTK_ROOT): VTK_ROOT = $$(VTK_ROOT)
isEmpty(VTK_VERSION): VTK_VERSION = $$(VTK_VERSION)
isEmpty(VTK_INCLUDEPATH): VTK_INCLUDEPATH = $$(VTK_INCLUDEPATH)
isEmpty(VTK_LIBPATH): VTK_LIBPATH = $$(VTK_LIBPATH)
isEmpty(VTK_BINPATH): VTK_BINPATH = $$(VTK_BINPATH)
isEmpty(VTK_LIB_SUFFIX): VTK_LIB_SUFFIX = $$(VTK_LIB_SUFFIX)

isEmpty(VTK_ROOT) {
    error("VTK_ROOT is not set. Install VTK externally and set VTK_ROOT, for example C:\\VTK\\install. The core library does not require VTK.")
}

isEmpty(VTK_VERSION): VTK_VERSION = 9.6
isEmpty(VTK_INCLUDEPATH): VTK_INCLUDEPATH = $${VTK_ROOT}/include/vtk-$${VTK_VERSION}
isEmpty(VTK_LIBPATH): VTK_LIBPATH = $${VTK_ROOT}/lib
isEmpty(VTK_BINPATH): VTK_BINPATH = $${VTK_ROOT}/bin
isEmpty(VTK_LIB_SUFFIX): VTK_LIB_SUFFIX = -$$VTK_VERSION

!exists($$VTK_INCLUDEPATH/QVTKOpenGLNativeWidget.h) {
    error(QVTKOpenGLNativeWidget.h was not found in $${VTK_INCLUDEPATH}. Rebuild/install VTK with Qt support enabled so vtkGUISupportQt is available.)
}

INCLUDEPATH += $$VTK_INCLUDEPATH
LIBS += -L$$VTK_LIBPATH

VTK_LIBS += \
    vtksys \
    vtkCommonColor \
    vtkCommonCore \
    vtkCommonDataModel \
    vtkCommonExecutionModel \
    vtkCommonMath \
    vtkFiltersSources \
    vtkGUISupportQt \
    vtkInteractionStyle \
    vtkInteractionWidgets \
    vtkIOGeometry \
    vtkRenderingAnnotation \
    vtkRenderingCore \
    vtkRenderingOpenGL2 \
    vtkRenderingUI

for(vtkLib, VTK_LIBS) {
    LIBS += -l$${vtkLib}$${VTK_LIB_SUFFIX}
}

DEFINES += ROBOT3D_VISUALIZER_HAS_VTK
