#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <RobotKinematics/Collision/CollisionBackend.h>
#include <RobotKinematics/Collision/CollisionChecker.h>
#include <RobotKinematics/Collision/CollisionProfile.h>
#include <RobotKinematics/Collision/MeshCollisionProfile.h>
#include <RobotKinematics/Kinematics/ForwardKinematics.h>
#include <RobotKinematics/Kinematics/SerialRobotKinematics.h>
#include <RobotKinematics/Model/FrameRegistry.h>
#include <RobotKinematics/Model/ToolRegistry.h>
#include <RobotKinematics/Posture/PostureResolver.h>

#include <vtkSmartPointer.h>

#include <array>
#include <memory>
#include <set>
#include <vector>

class QVTKOpenGLNativeWidget;
class QCheckBox;
class QDoubleSpinBox;
class QTableWidget;
class vtkGenericOpenGLRenderWindow;
class vtkActor;
class vtkAxesActor;
class vtkLineSource;
class vtkOrientationMarkerWidget;
class vtkRenderer;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    struct VisualPartState {
        QString key;
        QString displayName;
        std::string linkId;
        RobotKinematics::Pose homeLinkInBase = RobotKinematics::Pose::identity();
        RobotKinematics::Pose homeVisualCorrection = RobotKinematics::Pose::identity();
        std::array<double, 3> baseColorRgb = {0.75, 0.75, 0.75};
        int jointAxisIndex = -1;
        bool isLoaded = false;
        vtkSmartPointer<vtkActor> actor;
        vtkSmartPointer<vtkAxesActor> originActor;
        vtkSmartPointer<vtkLineSource> axisSource;
        vtkSmartPointer<vtkActor> axisActor;
    };

    struct PrimitiveDebugState {
        std::string geometryId;
        std::string linkId;
        RobotKinematics::CollisionShapeType shapeType = RobotKinematics::CollisionShapeType::Sphere;
        RobotKinematics::Pose geometryToLink = RobotKinematics::Pose::identity();
        double radius_m = 0.0;
        double length_m = 0.0;
        std::array<double, 3> baseColorRgb = {0.45, 0.85, 0.65};
        vtkSmartPointer<vtkActor> bodyActor;
        vtkSmartPointer<vtkActor> capStartActor;
        vtkSmartPointer<vtkActor> capEndActor;
    };

    struct MeshDebugState {
        std::string meshId;
        std::string linkId;
        RobotKinematics::Pose meshToLink = RobotKinematics::Pose::identity();
        double stlScaleToMm = 1.0;
        std::array<double, 3> baseColorRgb = {0.2, 0.85, 0.85};
        vtkSmartPointer<vtkActor> actor;
    };

    enum class BackendSelection {
        Primitive,
        MeshOriginal,
        MeshSimplified,
    };

    struct MeshProfileState {
        RobotKinematics::MeshCollisionProfile profile;
        bool loaded = false;
        bool valid = false;
        QString source;
        QString note;
    };

    void setupVtkViewport();
    void setupModelState();
    void setupUiState();
    void connectSignals();
    void loadRobotVisuals();
    void loadCollisionProfile();
    void loadMeshCollisionProfiles();
    void loadCollisionDebugVisuals();
    void loadMeshCollisionDebugVisuals();
    void applyJointStateToSceneAndReadouts();
    void updateSceneFromChain(const RobotKinematics::FkChain& chain);
    void updateCollisionDebugVisuals(const RobotKinematics::FkChain& chain);
    void updateMeshDebugVisuals(const RobotKinematics::FkChain& chain);
    void updatePoseReadouts(const RobotKinematics::FkChain& chain);
    void updateJointStatus(const RobotKinematics::JointVector& joints);
    void updateCurrentPosture(const RobotKinematics::JointVector& joints);
    void updateCollisionState(const RobotKinematics::JointVector& joints);
    void updateIkStatus(const QString& message);
    void updateActionState();
    void populateCombos();
    void populateJointControls();
    void populatePostureControls();
    void populateSampleButtons();
    void populateCollisionControls();
    void populateBackendControls();
    void populateDebugControls();
    void applyDebugVisualState();
    void resetTargetToCurrentTcp();
    void solveInverseKinematics(bool solveAll);
    void populateIkResults(const RobotKinematics::IKResult& result);
    void populateCollisionPairs(const RobotKinematics::CollisionCheckResult& result);
    void applySelectedIkSolution();
    void setJointDegrees(const std::array<double, 6>& degrees);
    RobotKinematics::JointVector currentJointVector() const;
    std::array<QDoubleSpinBox*, 6> jointSpinBoxes() const;
    std::array<QDoubleSpinBox*, 6> targetSpinBoxes() const;
    std::array<QCheckBox*, 8> partVisibleCheckBoxes() const;
    std::array<QCheckBox*, 8> partOriginCheckBoxes() const;
    std::array<QCheckBox*, 8> partAxisCheckBoxes() const;
    RobotKinematics::Result<RobotKinematics::Tool> selectedTool() const;
    RobotKinematics::Result<RobotKinematics::Pose> selectedReferenceInBase(
        const RobotKinematics::FkChain& chain) const;
    RobotKinematics::Result<std::optional<RobotKinematics::ArmPosture>> requestedPosture() const;

    Ui::MainWindow *ui;
    QVTKOpenGLNativeWidget* vtkWidget_ = nullptr;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow_;
    vtkSmartPointer<vtkOrientationMarkerWidget> orientationMarker_;
    vtkSmartPointer<vtkRenderer> renderer_;
    RobotKinematics::SerialRobotConfig config_;
    RobotKinematics::SerialRobotKinematics robot_;
    RobotKinematics::FrameRegistry frameRegistry_;
    RobotKinematics::ToolRegistry toolRegistry_;
    std::unique_ptr<RobotKinematics::PostureResolver> postureResolver_;
    RobotKinematics::CollisionProfile collisionProfile_;
    bool collisionProfileAvailable_ = false;
    MeshProfileState meshOriginalProfile_;
    MeshProfileState meshSimplifiedProfile_;
    RobotKinematics::CollisionBackendInfo meshBackendInfo_;
    BackendSelection backendSelection_ = BackendSelection::Primitive;
    std::vector<VisualPartState> visualParts_;
    std::vector<PrimitiveDebugState> primitiveDebugStates_;
    std::vector<MeshDebugState> meshOriginalDebugStates_;
    std::vector<MeshDebugState> meshSimplifiedDebugStates_;
    std::vector<RobotKinematics::IKSolution> lastIkSolutions_;
    std::vector<RobotKinematics::CollisionPairResult> lastCollisionPairs_;
    std::set<std::string> collidingGeometryIds_;
    std::set<std::string> collidingLinkIds_;
    QString assetsDirectory_;
    QString collisionProfileSource_;
    QString collisionProfileNote_;
};
#endif // MAINWINDOW_H
