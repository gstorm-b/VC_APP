# Spec: RobotKinematics

## Objective

Build a reusable C++ backend library for industrial robot kinematics.

The primary users are developer engineers integrating robot guidance, bin picking, remote control, and similar robotics applications. The library must let engineers define or load a robot model, configure frames/tools, compute forward kinematics, solve inverse kinematics, validate joint limits, and choose IK results using seed/posture/options.

Phase 1 focuses on serial 6DOF industrial robots. The first implementation preset is a self-designed virtual serial 6DOF robot used to validate the base architecture, JSON schema, FK, IK, frames/tools, joint limits, and posture behavior before real vendor preset data is available. Nachi MZ04D is now implemented from teach-pendant reference data. Kawasaki RS007N remains a real-preset validation target after its source data is provided. The architecture must remain extensible for SCARA, parallel/delta, 4DOF, 5DOF, and other robot families, but those robot types are not implementation scope for the first phase.

## Users

- Developer engineers integrating robotics logic into backend or desktop applications.
- Robotics/application engineers validating joint limits and reachable poses for CV matching, bin picking, and remote control workflows.
- Future library maintainers adding robot presets, solver implementations, and import/export adapters.

## Core Use Cases

- Compute TCP pose from joint values.
- Compute joint values from target TCP pose.
- Validate whether a joint vector is inside configured joint limits.
- Validate whether a target pose is reachable by the configured robot.
- Validate whether a joint vector causes conservative self-collision, once the collision extension is enabled.
- Configure base/user frames and tools.
- Solve IK using previous joint state, seed joint state, posture constraints, and solver options.
- Return either one best IK solution or multiple candidate solutions.
- Define custom presets for robots with the same general morphology but different link transforms, joint axes, limits, names, and solver metadata.

## Non-Goals For Phase 1

- No UI.
- No physical robot calibration accuracy claim.
- No collision checking in the original phase 1 milestone. Primitive self-collision is a post-base extension documented below.
- No path planning beyond kinematic validation and single-pose IK.
- No dynamic model, torque, velocity planning, or trajectory generation.
- No guaranteed support for SCARA, delta, parallel, 4DOF, 5DOF implementations in phase 1.
- No exhaustive `solveAll` guarantee for numerical IK.
- No dependency of core solvers on URDF parser internals.

## Tech Stack

- Language: C++.
- Math: Eigen.
- Build target: Qt 6 project with qmake.
- Core library type: reusable C++ library, no UI.
- Test framework: Qt Test.
- Primary compiler/toolchain: MSVC.
- Compatibility compiler/toolchain: MinGW.

## Commands

```powershell
# Incremental MSVC build
scripts\build_msvc.bat

# Run tests
scripts\test_msvc.bat

# Clean rebuild + tests
scripts\rebuild_msvc.bat
```

MSVC is the primary supported compiler. MinGW is a compatibility target and must be verified before release.

For MinGW compatibility, use `scripts\build_mingw.bat` and `scripts\test_mingw.bat`.

## Architecture Decisions

### Canonical Robot Model

The library uses an internal URDF-like canonical robot model based on links, joints, joint origin transforms, and joint axes.

DH, Modified DH, and URDF are adapter formats. They can be converted into the canonical model, but the core FK/IK solvers operate only on the canonical model.

Canonical model fields:

- Robot identity: name, vendor, model, version.
- Topology: serial, scara, parallel, custom.
- Links.
- Joints.
- Joint origin transform: parent link to joint frame.
- Joint axis: expressed in joint frame.
- Joint type: revolute, prismatic, fixed.
- Joint limits: position min/max, optional velocity/acceleration.
- Base frame.
- Flange frame.
- Tool registry.
- User frame registry.
- Solver metadata.
- Posture/branch definitions.
- Vendor/controller naming aliases.

### Units

All internal calculations and canonical model values use SI units:

- Length: meter.
- Angle: radian.

The public API provides explicit helper constructors/converters for industrial robot units:

- millimeter to/from meter.
- degree to/from radian.

No implicit unit conversion is performed.

Example helper direction:

```cpp
namespace units {
double mm(double value);
double deg(double value);
double toMm(double meters);
double toDeg(double radians);
}
```

### Pose Representation

The core pose representation is a rigid transform based on `Eigen::Isometry3d`.

Euler/RPY, quaternion, XYZABC, and XYZRPY are helper formats only. They must not become the internal source of truth.

Recommended wrapper:

```cpp
class Pose {
public:
    static Pose identity();

    static Pose fromIsometry(const Eigen::Isometry3d& transform);
    const Eigen::Isometry3d& isometry() const;

    static Pose fromXYZRPY_m_rad(double x, double y, double z,
                                 double roll, double pitch, double yaw);

    static Pose fromXYZRPY_mm_deg(double x, double y, double z,
                                  double roll, double pitch, double yaw);

    Eigen::Vector3d translation_m() const;
    Eigen::Quaterniond rotationQuaternion() const;
};
```

RPY helper convention must be explicitly documented and tested. Default recommendation: roll around X, pitch around Y, yaw around Z.

### FK

Forward kinematics computes transforms by chaining canonical joint/link transforms.

Minimum FK outputs:

- Base to flange.
- Base to active tool TCP.
- Any configured frame to flange/TCP when frame and tool are supplied.

### IK

The library uses a hybrid IK architecture.

Phase 1 provides generic numerical IK for serial 6DOF robots. The first numerical IK method is adaptive damped least squares with joint-limit handling, weighted position/orientation residuals, and multi-seed search for `solveAll`.

Analytic IK is introduced later as solver plugins for supported robot morphologies.

`solveAll` returns all solutions found by the selected solver. Exhaustive solution guarantees only apply to analytic solvers that explicitly declare support for the robot model.

Recommended API shape:

```cpp
struct IKRequest {
    Pose targetPose;
    std::optional<JointVector> seedJoint;
    std::optional<JointVector> previousJoint;
    std::optional<ArmPosture> posture;
    FrameId referenceFrame;
    ToolId tool;
    IKOptions options;
};

struct IKOptions {
    bool returnClosestToSeed = true;
    bool requirePosture = false;
    double maxPositionError_m = 1e-6;
    double maxOrientationError_rad = 1.7453292519943296e-5;
    int maxSolutions = 16;
};

struct IKSolutionScore {
    double totalCost;
    double seedDistanceCost;
    double jointLimitMarginCost;
    double postureMismatchCost;
    double motionContinuityCost;
};

struct IKSolution {
    JointVector joints;
    ArmPosture posture;
    double positionError_m;
    double orientationError_rad;
    IKSolutionScore score;
};

struct IKResult {
    IKStatus status;
    std::vector<IKSolution> solutions;
    std::string message;
};
```

Required APIs:

- `solve(request)`: returns the best solution according to seed/posture/options.
- `solveAll(request)`: returns all solutions found by the selected solver.

`previousJoint` and `seedJoint` are preferences unless options explicitly require otherwise.

### Posture And Branches

The library must support configurable posture/branch constraints for serial robots, including concepts such as lefty/righty, above/below, flip/non-flip.

These names must not be hard-coded as universal concepts in the base class. Different robot families and vendors may use different naming and branch rules.

Recommended direction:

```cpp
struct ArmPosture {
    std::optional<int> shoulder;
    std::optional<int> elbow;
    std::optional<int> wrist;
    std::map<std::string, int> vendorSpecific;
};
```

Each robot family or preset may provide a `PostureResolver` that maps vendor-specific posture names to internal branch constraints.

### Frames And Tools

The library must support:

- Base frame.
- Flange frame.
- User-defined frames.
- Tool definitions.
- Active tool selection per FK/IK request.

Frame/tool transforms must use the same `Pose` representation and SI units as the rest of the core.

### Collision Detection Extension

Collision detection is a post-base-library extension. Phase 9 provides fast conservative
self-collision for serial robot joint states using primitive geometry. Phase 10 adds an accurate mesh
collision backend for STL-part coverage.

The primitive backend must not use STL triangle meshes. The mesh backend may use STL-derived triangle
meshes, but only through RobotKinematics-owned data types and internal backend adapters. VTK remains
example/debug-only unless the user explicitly approves a core VTK dependency.

MVP collision shapes:

- sphere;
- capsule.

Additional shapes such as box, cylinder, convex mesh, or triangle mesh require a separate scope
decision after the primitive MVP is measured.

Collision geometry is attached to canonical link ids:

```cpp
enum class CollisionShapeType {
    Sphere,
    Capsule
};

struct CollisionSphere {
    double radius_m;
};

struct CollisionCapsule {
    double radius_m;
    double length_m;
};

struct CollisionGeometry {
    std::string id;
    std::string linkId;
    Pose geometryToLink;
    CollisionShapeType shapeType;
    CollisionSphere sphere;
    CollisionCapsule capsule;
    double margin_m;
    bool enabled;
};

struct DisabledCollisionPair {
    std::string geometryA;
    std::string geometryB;
    std::string reason;
};

struct CollisionProfile {
    std::string id;
    std::string robotModel;
    std::vector<CollisionGeometry> geometries;
    std::vector<DisabledCollisionPair> disabledPairs;
};
```

Collision checks consume a `SerialRobotConfig`, `CollisionProfile`, and `JointVector`. FK places
each geometry in the base frame, pair filtering removes disabled or adjacent-contact pairs, and the
checker runs primitive distance tests.

Collision result status semantics:

- `KinematicsStatus::Ok` means the check executed successfully.
- A found collision is represented by `CollisionCheckResult::hasCollision == true`.
- Do not add `CollisionDetected` to `KinematicsStatus` for the MVP.
- Existing error statuses cover invalid robot config, invalid request, joint-dimension mismatch,
  and numerical/internal errors.

Recommended result shape:

```cpp
struct CollisionCheckRequest {
    JointVector joints;
    double safetyMargin_m = 0.0;
    bool returnAllPairs = true;
};

struct CollisionPairResult {
    std::string geometryA;
    std::string geometryB;
    std::string linkA;
    std::string linkB;
    bool colliding;
    double distance_m;
};

struct CollisionCheckResult {
    KinematicsStatus status;
    bool hasCollision;
    std::vector<CollisionPairResult> pairs;
    std::string message;
};
```

Collision profiles use SI units. Runtime code should load collision profiles explicitly from a
separate `robot-kinematics-collision/v1` artifact. Preset JSON may reference collision profiles in
`metadata`, but collision data is not required by `robot-kinematics-preset/v1`.

Accurate mesh collision uses a separate `robot-kinematics-collision-mesh/v1` artifact. Mesh profiles
must declare source units, `scaleToMeters`, and an explicit `meshToLink` transform for each mesh.
Third-party backend types from Coal, FCL, VTK, Open3D, CGAL, or libigl must not appear in public
RobotKinematics headers.

Phase 10 may expose RobotKinematics-owned backend availability queries and mesh-profile/STL loading
APIs before a production mesh backend is compiled. In that default no-backend configuration, mesh
collision requests should return `UnsupportedSolver` while primitive collision remains available.

### Presets

Phase 1 preset strategy:

- `Virtual6DofTestArm`: required first implementation preset. This is a synthetic serial 6DOF robot designed by the project to exercise the canonical model, numerical IK, posture classification, frames/tools, JSON loading, and C++ fallback behavior.
- `Kawasaki RS007N`: real-preset validation target after source data is provided.
- `Nachi MZ04D`: implemented real-preset validation target from teach-pendant reference data.

Each preset must include enough data for FK, IK, joint limit validation, tool/frame behavior, and posture handling.

For virtual presets, the source of truth is the project-owned preset spec and fixture tests. For real robot presets, the source of truth is vendor documentation, verified internal robot configuration, or measured teach-pendant reference data. Each real preset JSON must include source references for dimensions, joint limits, and posture definitions where available.

Presets are stored as JSON files so developers can add or customize robots without recompiling the library. `Virtual6DofTestArm` and `NachiMZ04D` have built-in C++ fallbacks. Kawasaki RS007N should also have a built-in C++ fallback once its real preset data is added.

`Virtual6DofTestArm` requirements:

- Serial 6DOF revolute chain.
- Non-zero link offsets and non-parallel joint axes sufficient to exercise transform chaining.
- Joint limits wide enough for stable numerical IK tests, but finite.
- Home position and joint-limit midpoint are valid.
- Default tool and at least one non-default test tool.
- Base frame and at least one user frame.
- Posture labels for lefty/righty, above/below, and flip/non-flip.
- Deterministic FK fixture poses generated from selected joint vectors.
- Non-singular IK fixture targets generated from FK of known joint vectors.

Preset JSON files use a versioned schema named `robot-kinematics-preset/v1`.

Rules:

- JSON numeric values are stored in canonical SI units only: meter and radian.
- The schema may include source references and notes that mention vendor units, but solver-facing values must already be normalized.
- Unknown top-level fields are invalid unless placed under `metadata`.
- Serial robot joints are ordered in kinematic-chain order from base to flange.
- IDs must be stable because tests, frames, tools, and posture metadata may reference them.

Minimum preset shape:

```json
{
  "schema": "robot-kinematics-preset/v1",
  "identity": {
    "vendor": "RobotKinematics",
    "model": "Virtual6DofTestArm",
    "name": "Virtual 6DOF Test Arm",
    "revision": "1.0.0"
  },
  "units": {
    "length": "m",
    "angle": "rad"
  },
  "topology": {
    "type": "serial",
    "dof": 6
  },
  "links": [
    { "id": "base_link" },
    { "id": "link_1" },
    { "id": "flange" }
  ],
  "joints": [
    {
      "id": "J1",
      "type": "revolute",
      "parent": "base_link",
      "child": "link_1",
      "origin": {
        "xyz_m": [0.0, 0.0, 0.0],
        "rpy_rad": [0.0, 0.0, 0.0]
      },
      "axis": [0.0, 0.0, 1.0],
      "limits": {
        "lower": -3.141592653589793,
        "upper": 3.141592653589793,
        "velocity": null,
        "acceleration": null
      },
      "home": 0.0,
      "aliases": ["JT1"]
    }
  ],
  "frames": {
    "base": "base_link",
    "flange": "flange",
    "userFrames": []
  },
  "tools": [
    {
      "id": "default",
      "name": "Default Tool",
      "flangeToTcp": {
        "xyz_m": [0.0, 0.0, 0.0],
        "rpy_rad": [0.0, 0.0, 0.0]
      }
    }
  ],
  "defaultTool": "default",
  "posture": {
    "resolver": "serial_6dof_shoulder_elbow_wrist",
    "labels": {
      "shoulder": { "negative": "lefty", "positive": "righty" },
      "elbow": { "negative": "below", "positive": "above" },
      "wrist": { "negative": "non-flip", "positive": "flip" }
    }
  },
  "solver": {
    "default": "adaptive_damped_least_squares",
    "parameters": {}
  },
  "sources": [
    {
      "type": "project_fixture",
      "title": "RobotKinematics virtual preset definition",
      "reference": "docs/robot_kinematics_spec.md",
      "appliesTo": ["dimensions", "joint_limits", "posture"]
    }
  ],
  "metadata": {}
}
```

For revolute joints, `limits.lower`, `limits.upper`, `limits.velocity`, `limits.acceleration`, and `home` are angular values in radians. For prismatic joints, the same fields are linear values in meters.

The exact JSON Schema artifact can be generated from this contract during implementation, but code and tests must follow this shape.

### Numerical IK Defaults

Phase 1 numerical IK uses adaptive damped least squares with these initial default parameters:

```cpp
struct NumericalIKDefaults {
    int maxIterations = 200;
    int maxSeeds = 32;

    double positionTolerance_m = 1e-6;
    double orientationTolerance_rad = 1.7453292519943296e-5; // 0.001 deg

    double stepTolerance = 1e-10;
    double costTolerance = 1e-12;

    double initialDamping = 1e-3;
    double minDamping = 1e-8;
    double maxDamping = 1e2;
    double dampingDecreaseFactor = 0.5;
    double dampingIncreaseFactor = 10.0;

    double maxJointStepNorm = 0.2;
    double jointLimitAvoidanceWeight = 0.05;
    double positionResidualWeight = 1.0;
    double orientationResidualWeight = 1.0;

    double duplicateJointTolerance = 1e-6;
};
```

Default seed order:

1. `previousJoint`, when provided.
2. `seedJoint`, when provided and different from `previousJoint`.
3. Posture-derived seeds, when posture is requested.
4. Robot home joint.
5. Joint-limit midpoint.
6. Deterministic low-discrepancy seeds within joint limits until `maxSeeds` is reached.

These values are implementation defaults, not immutable API guarantees. They may be overridden through `IKOptions` and tuned after measurement, but any change to defaults must update tests and documentation.

### Failure Status Enum

IK and validation APIs use structured status values. Phase 1 status enum:

```cpp
enum class KinematicsStatus {
    Ok,
    InvalidRobotConfig,
    InvalidRequest,
    FrameNotFound,
    ToolNotFound,
    JointDimensionMismatch,
    JointLimitViolation,
    TargetUnreachable,
    Singularity,
    MaxIterationsReached,
    NoConvergedSolution,
    PostureConstraintUnsatisfied,
    UnsupportedSolver,
    NumericalError
};
```

`IKResult.status == KinematicsStatus::Ok` means at least one valid solution is available. Other statuses may still include diagnostic candidates if useful, but callers must not treat them as executable robot commands.

Custom presets must allow developers to change:

- Link transforms.
- Joint axes.
- Joint names.
- Link names.
- Joint limits.
- Tool/flange naming.
- Solver metadata.
- Posture definitions.

## Proposed Project Structure

```text
include/
  RobotKinematics/
    Core/
      Pose.h
      Units.h
      Result.h
    Model/
      RobotModelConfig.h
      SerialRobotConfig.h
      Link.h
      Joint.h
      Frame.h
      Tool.h
    Kinematics/
      RobotKinematics.h
      SerialRobotKinematics.h
      ForwardKinematics.h
      InverseKinematics.h
    Solvers/
      IKSolver.h
      NumericalIKSolver.h
      AnalyticIKSolver.h
    Collision/
      CollisionGeometry.h
      CollisionProfile.h
      CollisionChecker.h
      CollisionBackend.h
      MeshCollisionProfile.h
      StlMeshLoader.h
    Posture/
      ArmPosture.h
      PostureResolver.h
    Presets/
      Virtual6DofTestArm.h
      KawasakiRS007N.h
      NachiMZ04D.h
    Adapters/
      UrdfAdapter.h
      DhAdapter.h

src/
  Core/
  Model/
  Kinematics/
  Solvers/
  Posture/
  Presets/
  Adapters/

tests/
  unit/
  integration/
  fixtures/

docs/
  project_description.md
  robot_kinematics_spec.md
  planning/
    robot_kinematics_implementation_plan.md
```

## Code Style

Prefer small explicit value types and request/result objects over long positional argument lists.

Example style:

```cpp
IKRequest request;
request.targetPose = Pose::fromXYZRPY_mm_deg(400.0, 0.0, 300.0, 180.0, 0.0, 90.0);
request.seedJoint = JointVector::fromDegrees({0.0, -30.0, 45.0, 0.0, 60.0, 0.0});
request.tool = ToolId{"default"};
request.referenceFrame = FrameId{"base"};
request.options.requirePosture = true;

IKResult result = robot.solve(request);
if (!result.ok()) {
    return result.status;
}

JointVector joints = result.best().joints;
```

Conventions:

- Public APIs use clear request/result objects.
- Internal units remain meter/radian.
- Helper methods must include unit names when accepting raw numeric values.
- Avoid implicit global active state where possible; if active tool/frame exists, request-level override should still be supported.
- Return structured status codes, not only boolean success/failure.

## Testing Strategy

### Unit Tests

- Unit conversion helpers.
- Pose construction and transform composition.
- Joint limit validation.
- Canonical model validation.
- FK transform chaining.
- Frame/tool transform correctness.
- Numerical IK convergence behavior.
- IK solution ranking.
- Posture resolver mapping.
- Collision profile validation, primitive distance checks, pair filtering, and FK-based geometry
  placement once the collision extension is implemented.

### Integration Tests

- Virtual6DofTestArm preset FK/IK round-trip.
- Kawasaki RS007N preset FK/IK round-trip after source data is provided.
- Nachi MZ04D preset FK/IK round-trip.
- Custom serial 6DOF preset can be built and solved.
- `solve` selects the closest solution to seed/previous joint.
- `solveAll` returns multiple found solutions when configured with multiple seeds.

### Accuracy Criteria

The accuracy target applies to library computation only, not physical robot accuracy.

Required phase 1 criteria:

- FK deterministic expected-pose tests pass within agreed numeric tolerance.
- IK pose residual: `FK(IK(targetPose))` must produce TCP position error <= `0.001 mm` (`1e-6 m`) for normal non-singular test poses.
- Orientation error must be checked separately. Phase 1 target: <= `0.001 degree` (`1.7453292519943296e-5 rad`) for normal non-singular test poses.
- Joint round-trip `joint -> FK -> IK -> joint` is required only when seed or posture constrains the solver to the original branch.
- When a test compares an IK solution against an expected revolute-joint vector, the default per-joint angular tolerance is <= `0.0001 degree` (`1.7453292519943296e-6 rad`).
- A relaxed per-joint angular tolerance of <= `0.001 degree` (`1.7453292519943296e-5 rad`) is allowed only for tests whose expected pose or coordinate source comes from teach-pendant/reference data rounded to 2 decimal places. Such tests must document the source precision and why the relaxed tolerance is used.
- Joint-angle comparison tolerance is separate from TCP pose residual tolerance. Do not loosen pose residual criteria just because a joint-vector comparison uses the teach-pendant exception.
- Tests must exclude singularities, joint-limit boundaries, and unreachable poses unless the test is specifically validating failure behavior.

For Virtual6DofTestArm, phase 1 must support posture classification and posture-constrained solve for lefty/righty, above/below, and flip/non-flip. The same posture behavior is validated for Nachi MZ04D, with documented caveats around inferred elbow above-side behavior. Kawasaki RS007N posture behavior must be validated after its real preset data is provided. Exhaustive branch enumeration is best-effort for numerical IK and becomes guaranteed only when analytic solver support is added.

## Boundaries

### Always

- Keep core calculations in meter/radian.
- Use explicit helper names for mm/degree APIs.
- Keep FK/IK core independent from URDF parser details.
- Keep primitive collision independent from STL/VTK triangle mesh logic.
- Keep mesh collision backend libraries behind internal adapters.
- Return structured statuses for IK failure modes.
- Add tests for every public behavior added.
- Document solver assumptions and limitations.

### Ask First

- Changing unit convention.
- Changing public API names.
- Adding third-party dependencies beyond Eigen/Qt/test framework.
- Adding or changing runtime mesh collision dependencies.
- Claiming physical robot accuracy.
- Expanding implementation scope to SCARA, delta, parallel, 4DOF, or 5DOF.
- Making URDF the canonical model.

### Never

- Do not silently convert units.
- Do not treat RPY/Euler as the internal pose source of truth.
- Do not claim numerical `solveAll` is mathematically exhaustive.
- Do not hard-code vendor posture names as universal base-class behavior.
- Do not mix robot preset data directly into solver logic.
- Do not treat primitive collision profiles as physical safety certification.

## Base Code Milestone Success Criteria

The first engineering milestone is complete when:

- A Qt 6/qmake C++ library builds successfully.
- Core model can represent serial 6DOF robots using URDF-like link/joint transforms.
- FK works for custom serial 6DOF configs.
- Numerical IK works for normal non-singular serial 6DOF test cases.
- `solve` and `solveAll` APIs exist with structured request/result types.
- Tool and user frame transforms are supported in FK and IK requests.
- Joint limit validation exists and is tested.
- Posture/branch metadata exists and is used by IK selection at least at the policy/interface level.
- Virtual6DofTestArm preset exists as JSON and built-in C++ fallback.
- Tests validate FK, IK residual, joint limits, frames/tools, and preset loading.
- Documentation explains units, pose conventions, IK limitations, and preset customization.

## Real Preset Validation Success Criteria

Real preset validation is complete when:

- The preset exists as JSON.
- A built-in C++ fallback exists.
- Source references are recorded for dimensions, joint limits, and posture definitions where available.
- FK/IK fixture tests pass for normal non-singular poses.
- Posture classification and posture-constrained solve are validated for lefty/righty, above/below, and flip/non-flip.

## Open Questions

- Exact source documents/config files for Kawasaki RS007N dimensions, joint limits, and posture definitions still need to be collected and referenced.

## Validation Follow-Ups

- Solver defaults may need tuning after numerical validation, but the initial defaults are now specified above.
- Primitive collision profile dimensions need review and tuning after the collision module lands.
