# API Reference

A concise map of the public API. All symbols are in namespace `RobotKinematics` unless noted.
Types are described by their role; see the headers for exact field lists.

## Core (`RobotKinematics/Core/...`)

### `Pose` — `Core/Pose.h`
Wrapper around `Eigen::Isometry3d`. The canonical transform type.

| Member | Description |
|---|---|
| `static Pose identity()` | Identity transform. |
| `static Pose fromIsometry(const Eigen::Isometry3d&)` | Wrap an existing transform. |
| `static Pose fromXYZRPY_m_rad(x, y, z, roll, pitch, yaw)` | Build from position (m) + RPY (rad). Rotation = `Rz(yaw)·Ry(pitch)·Rx(roll)`. |
| `static Pose fromXYZRPY_mm_deg(x, y, z, roll, pitch, yaw)` | Same convention, inputs in mm/deg. |
| `const Eigen::Isometry3d& isometry() const` | Underlying transform. |
| `Eigen::Vector3d translation_m() const` | Position in meters. |
| `Eigen::Quaterniond rotationQuaternion() const` | Orientation. |
| `Pose inverse() const` / `Pose operator*(const Pose&) const` | Compose/invert. |

### `JointVector` — `Core/JointVector.h`
Ordered joint values, SI units (rad for revolute, m for prismatic), one per movable joint.

`fromRadians({...})`, `fromDegrees({...})`, `operator[](int)`, `size()`, `isEmpty()`,
`values()` (`Eigen::VectorXd`), `toDegrees()`.

### `units` — `Core/Units.h` (namespace `RobotKinematics::units`)
`mm(value_mm)`, `deg(value_deg)` → SI; `toMm(value_m)`, `toDeg(value_rad)` → display units.

### `Result<T>` and `KinematicsStatus` — `Core/Result.h`
`Result<T>{ status, value, message }` with `ok()`, `Result::success(v)`, `Result::failure(s,msg)`.
`KinematicsStatus`: `Ok, InvalidRobotConfig, InvalidRequest, FrameNotFound, ToolNotFound,
JointDimensionMismatch, JointLimitViolation, TargetUnreachable, Singularity,
MaxIterationsReached, NoConvergedSolution, PostureConstraintUnsatisfied, UnsupportedSolver,
NumericalError`.

### `FrameId`, `ToolId` — `Core/Ids.h`
Strongly-typed string ids (implicitly constructible from `const char*`); `empty()`, `==`.

## Model (`RobotKinematics/Model/...`)

### Value types — `Model/RobotModelConfig.h`
`SerialRobotConfig` (the whole robot) plus its parts: `RobotIdentity`, `Link`, `Joint`
(`type`, `parentLinkId`, `childLinkId`, `origin`, `axis`, `limits`, `home`, `aliases`),
`JointType` (`Revolute`/`Prismatic`/`Fixed`), `JointLimits`, `UserFrame`, `FrameConfig`,
`Tool`, `PostureMetadata` + `PostureLabelAxis`, `SolverMetadata`, `SourceReference`,
`RobotTopologyType`. `SerialRobotConfig::dof` counts **movable** joints only.

### `SerialRobotConfigBuilder` — `Model/SerialRobotConfigBuilder.h`
Fluent builder: `.identity().baseAndFlange().addLink().addJoint().addUserFrame().addTool()
.defaultTool().posture().solver().addSource()` then `build() -> Result<SerialRobotConfig>`
(validates).

### `RobotModelValidator` — `Model/RobotModelValidator.h`
`static ModelValidationResult validateSerialRobotConfig(config)`. Result has `ok()`, `status()`,
and `issues` (`{status, field, message}`).

### `FrameRegistry`, `ToolRegistry` — `Model/FrameRegistry.h`, `Model/ToolRegistry.h`
`fromConfig(config)`; `get(id) -> Result<...>`, `contains(id)`, `tools()/frames()`,
`ToolRegistry::getDefault()`.

## Kinematics (`RobotKinematics/Kinematics/...`)

### `ForwardKinematics` — `Kinematics/ForwardKinematics.h`
Static functions:

| Function | Returns |
|---|---|
| `movableJointCount(config)` | `int` — required `JointVector` size. |
| `flangePose(config, joints)` | `Pose` base→flange. |
| `toolPose(config, joints, flangeToTcp)` | `Pose` base→TCP. |
| `computeChain(config, joints)` | `FkChain{ joints[], linkPosesInBase, flangeInBase }`. |
| `userFrameInBase(chain, frame)` | `Result<Pose>`. |

`FkChain::joints[i]` is `JointFrameData{ axisInBase, originInBase, type }` per movable joint.

### `SerialRobotKinematics` — `Kinematics/SerialRobotKinematics.h`
Constructed from a `SerialRobotConfig`. `config()`, `solve(IKRequest) -> IKResult`,
`solveAll(IKRequest) -> IKResult`. Resolves frame/tool and selects the solver (analytic if
supported, else numerical). `Kinematics/RobotKinematics.h` is a convenience header that includes
this one.

### IK types — `Kinematics/InverseKinematics.h`
- `IKRequest{ targetPose, seedJoint?, previousJoint?, posture?, referenceFrame, tool, options }`
- `IKOptions{ returnClosestToSeed, requirePosture, maxPositionError_m, maxOrientationError_rad, maxSolutions }`
- `IKSolution{ joints, posture, positionError_m, orientationError_rad, score }`
- `IKResult{ status, solutions, message }` with `ok()` and `best()`.
- `IKStatus` is an alias for `KinematicsStatus`.

## Posture (`RobotKinematics/Posture/...`)

`ArmPosture{ shoulder?, elbow?, wrist?, vendorSpecific }` (branch values −1/+1) —
`Posture/ArmPosture.h`.
`PostureResolver` interface + `Serial6DofSignPostureResolver` + `PostureResolverFactory::create(config)`
— `Posture/PostureResolver.h`. `classify(config, joints)` and `fromLabels(metadata, labels)` both
return `Result<ArmPosture>`.

## Presets (`RobotKinematics/Presets/...`)

- `Presets::virtual6DofTestArm()` — `Presets/Virtual6DofTestArm.h`
- `Presets::nachiMZ04D()` — `Presets/NachiMZ04D.h`
- `PresetJsonLoader::loadFile(path)` / `loadJson(json)` → `Result<SerialRobotConfig>` —
  `Presets/PresetJsonLoader.h`

## Collision (`RobotKinematics/Collision/...`)

### Primitive profiles and checks

- `CollisionProfile` / `CollisionGeometry` — `Collision/CollisionProfile.h`
- `CollisionProfileJsonLoader::loadFile(path)` / `loadJson(json)` →
  `Result<CollisionProfile>` — `Collision/CollisionProfileJsonLoader.h`
- `CollisionCheckRequest{ joints, safetyMargin_m, returnAllPairs }` —
  `Collision/CollisionChecker.h`
- `CollisionCheckResult{ status, hasCollision, pairs, message }` with `ok()`
- `CollisionPairResult{ geometryA, geometryB, linkA, linkB, colliding, distance_m, contactCount }`

Use `CollisionBackends::checkPrimitive(config, profile, request)` for the default lightweight
sphere/capsule path.

### Mesh profiles and optional backend

- `MeshCollisionProfile` / `MeshCollisionGeometry` —
  `Collision/MeshCollisionProfile.h`
- `MeshCollisionProfileJsonLoader::loadFile(path)` / `loadJson(json)` →
  `Result<MeshCollisionProfile>` — `Collision/MeshCollisionProfileJsonLoader.h`
- `MeshCollisionCheckRequest{ joints, safetyMargin_m, returnAllPairs, preferredBackend }` —
  `Collision/CollisionBackend.h`
- `CollisionBackends::meshInfo()` / `availableBackends()` report backend availability.
- `CollisionBackends::checkMesh(config, profile, request)` runs the optional mesh backend and
  returns `UnsupportedSolver` when no mesh backend is compiled.

Mesh backend types are intentionally RobotKinematics-owned. Coal/FCL/VTK/Open3D/CGAL/libigl types
must not appear in public headers.

## Adapters (`RobotKinematics/Adapters/...`)

- `DhAdapter::fromStandardDh(identity, rows)` with `StandardDhParameter{ jointId, a_m, alpha_rad,
  d_m, thetaOffset_rad, type, limits, home }` and `DhJointType{ Revolute, Prismatic }` —
  `Adapters/DhAdapter.h`.
- `UrdfAdapter::exportSerialRobot(config) -> Result<std::string>` and
  `importSerialRobot(urdf, baseLinkId, flangeLinkId) -> Result<SerialRobotConfig>` —
  `Adapters/UrdfAdapter.h`.

## Solvers (`RobotKinematics/Solvers/...`) — usually indirect

You normally reach solvers through `SerialRobotKinematics`. If you need them directly:
`IKSolver` interface (`solve`/`solveAll` on `IKSolveContext`), `NumericalIKSolver` (+ tunable
`NumericalIKDefaults`), `AnalyticIKSolver` (adds `supportsModel`), and
`Analytic6DofSphericalWristSolver`.
