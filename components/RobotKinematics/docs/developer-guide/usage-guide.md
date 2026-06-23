# Usage Guide

Task-by-task examples. All examples assume `using namespace RobotKinematics;` and that the
relevant headers are included. Everything is **meters and radians** unless a name says
otherwise (see [conventions-and-gotchas.md](conventions-and-gotchas.md)).

- [1. Get a robot model](#1-get-a-robot-model)
- [2. Forward kinematics](#2-forward-kinematics)
- [3. Inverse kinematics](#3-inverse-kinematics)
- [4. Tools (TCP)](#4-tools-tcp)
- [5. User frames](#5-user-frames)
- [6. Posture classification](#6-posture-classification)
- [7. Validate a model](#7-validate-a-model)
- [8. Adapters: DH and URDF](#8-adapters-dh-and-urdf)
- [9. Collision checks](#9-collision-checks)

---

## 1. Get a robot model

Every operation needs a `SerialRobotConfig`. There are four ways to obtain one.

### 1a. A built-in C++ preset

```cpp
#include <RobotKinematics/Presets/NachiMZ04D.h>
#include <RobotKinematics/Presets/Virtual6DofTestArm.h>

const SerialRobotConfig nachi   = Presets::nachiMZ04D();
const SerialRobotConfig virtual6 = Presets::virtual6DofTestArm();
```

`Virtual6DofTestArm` is a synthetic robot for testing; `NachiMZ04D` is a real arm whose
kinematics/limits/posture were derived from teach-pendant data (see
[../preset_references/nachi-mz04d.md](../preset_references/nachi-mz04d.md)).

### 1b. A JSON preset (`robot-kinematics-preset/v1`)

```cpp
#include <RobotKinematics/Presets/PresetJsonLoader.h>

const Result<SerialRobotConfig> loaded =
    PresetJsonLoader::loadFile("presets/Nachi/MZ04/nachi_mz04d.json");
if (!loaded.ok()) {
    // loaded.status is a KinematicsStatus; loaded.message explains the problem.
    return;
}
const SerialRobotConfig config = loaded.value;
```

JSON values are interpreted as **SI units only** (`units: { length: "m", angle: "rad" }`).
The schema is documented in [../robot_preset_json_schema.md](../robot_preset_json_schema.md).
`PresetJsonLoader::loadJson(const std::string&)` parses an in-memory string instead of a file.

### 1c. Build one programmatically

```cpp
#include <RobotKinematics/Model/SerialRobotConfigBuilder.h>

Joint j1;
j1.id = "J1";
j1.type = JointType::Revolute;
j1.parentLinkId = "base_link";
j1.childLinkId = "link_1";
j1.origin = Pose::fromXYZRPY_m_rad(0.0, 0.0, 0.34, 0.0, 0.0, 0.0);
j1.axis = Eigen::Vector3d::UnitZ();
j1.limits = JointLimits{-3.14, 3.14, std::nullopt, std::nullopt};
// ... define J2..J6 the same way ...

const Result<SerialRobotConfig> built = SerialRobotConfigBuilder()
    .identity(RobotIdentity{"Acme", "Arm6", "Acme Arm 6", "1.0"})
    .baseAndFlange("base_link", "flange")
    .addLink("base_link").addLink("link_1") /* ... */ .addLink("flange")
    .addJoint(j1) /* ... addJoint(j2..j6) ... */
    .addTool(Tool{"default", "Default Tool", Pose::identity()})
    .defaultTool("default")
    .build();   // build() validates the model and returns a Result
```

`build()` runs model validation; a malformed chain comes back as a failed `Result`.

### 1d. From a standard DH table

See [section 8](#8-adapters-dh-and-urdf).

---

## 2. Forward kinematics

`ForwardKinematics` is a set of static functions. A `JointVector` must have exactly
`ForwardKinematics::movableJointCount(config)` entries (6 for a 6DOF arm).

```cpp
#include <RobotKinematics/Kinematics/ForwardKinematics.h>

const JointVector q = JointVector::fromDegrees({25, -20, 165, 10, 35, 150}); // deg -> rad
// or: JointVector::fromRadians({0.43, -0.35, 2.88, 0.17, 0.61, 2.62});

// Base -> flange pose:
const Pose flange = ForwardKinematics::flangePose(config, q);

Eigen::Vector3d p = flange.translation_m();             // position (m)
Eigen::Quaterniond r = flange.rotationQuaternion();     // orientation
const Eigen::Isometry3d& T = flange.isometry();         // raw transform if you need it
```

To get everything at once (per-joint axes/origins in base, all link poses, and the flange):

```cpp
const FkChain chain = ForwardKinematics::computeChain(config, q);
const Pose link3 = chain.linkPosesInBase.at("link_3");
const Eigen::Vector3d joint4Axis = chain.joints[3].axisInBase;   // unit axis in base frame
```

---

## 3. Inverse kinematics

Wrap the config in a `SerialRobotKinematics` and call `solve` (best single solution) or
`solveAll` (all solutions the solver found).

```cpp
#include <RobotKinematics/Kinematics/SerialRobotKinematics.h>

const SerialRobotKinematics robot(config);

IKRequest request;
request.targetPose = flange;     // desired TCP pose, in the reference frame, for the chosen tool
request.seedJoint  = q;          // optional: preferred / closest-to starting joints
// request.previousJoint = ...;  // optional: last commanded joints (motion continuity)
// request.referenceFrame = ...; // optional FrameId (empty => base frame)
// request.tool = "pointer";     // optional ToolId (empty => default tool)

const IKResult result = robot.solve(request);
if (result.ok()) {
    const IKSolution& best = result.best();
    const JointVector joints = best.joints;
    double posErr = best.positionError_m;        // residual vs target
    double oriErr = best.orientationError_rad;
}
```

`solveAll` returns every branch the solver found (up to 8 for a spherical-wrist arm):

```cpp
const IKResult all = robot.solveAll(request);
for (const IKSolution& s : all.solutions) {
    // s.joints, s.posture (shoulder/elbow/wrist signs), s.positionError_m, ...
}
```

**Which solver runs?** `SerialRobotKinematics` is hybrid. If the model is a supported
spherical-wrist articulated arm (e.g. `NachiMZ04D`), a **closed-form analytic** solver runs and
returns exact discrete branches. Otherwise it falls back to the **adaptive damped least-squares
numerical** solver. This is automatic — you call the same `solve`/`solveAll` either way. See
[ADR-0004](decisions/adr-0004-hybrid-analytic-numerical-ik.md). Note the numerical `solveAll` is
*found solutions*, not a mathematically exhaustive set (see
[conventions-and-gotchas.md](conventions-and-gotchas.md)).

### Tuning IK (`IKOptions`)

```cpp
request.options.maxPositionError_m    = 1e-6;
request.options.maxOrientationError_rad = 1.7453292519943296e-5; // ~0.001 deg
request.options.requirePosture = false;   // if true, reject solutions that don't match request.posture
request.options.maxSolutions   = 16;
```

### Failure handling

`IKResult::ok()` is true only when `status == Ok` and at least one solution exists. On failure,
`status` is a structured `KinematicsStatus` (e.g. `TargetUnreachable`, `Singularity`,
`ToolNotFound`, `FrameNotFound`, `JointLimitViolation`, `NoConvergedSolution`) and `message`
explains it. Always check `ok()` before calling `best()`.

---

## 4. Tools (TCP)

A tool defines the flange→TCP transform. `IKRequest::targetPose` is the pose of the **selected
tool's TCP**; an empty `request.tool` uses the config's default tool.

```cpp
request.tool = "pointer";                 // a ToolId; must exist in config.tools
request.targetPose = desiredTcpPose;      // pose of the pointer TCP
```

For FK with a tool, pass the flange→TCP transform explicitly:

```cpp
#include <RobotKinematics/Model/ToolRegistry.h>

const ToolRegistry tools = ToolRegistry::fromConfig(config);
const Result<Tool> tool = tools.get(ToolId{"pointer"});
if (tool.ok()) {
    const Pose tcp = ForwardKinematics::toolPose(config, q, tool.value.flangeToTcp);
}
```

You can also apply an arbitrary one-off tool without registering it:

```cpp
const Pose flangeToTcp = Pose::fromXYZRPY_mm_deg(44.2, 0.0, 139.0, 0.0, 0.0, 0.0);
const Pose tcp = ForwardKinematics::toolPose(config, q, flangeToTcp);
```

---

## 5. User frames

User frames are named transforms relative to a parent link (typically the base). For FK you
can resolve them against an `FkChain`:

```cpp
#include <RobotKinematics/Model/FrameRegistry.h>

const FrameRegistry frames = FrameRegistry::fromConfig(config);
const Result<UserFrame> frame = frames.get(FrameId{"table_frame"});

const FkChain chain = ForwardKinematics::computeChain(config, q);
const Result<Pose> frameInBase = ForwardKinematics::userFrameInBase(chain, frame.value);
```

For **IK**, `request.referenceFrame` may name a user frame, but only one **fixed to the base
link** — a frame attached to a moving link returns `InvalidRequest` (its base-relative transform
depends on the unknown joints). Empty / `"base"` means the base frame.

---

## 6. Posture classification

The serial-6DOF resolver classifies a joint vector into shoulder/elbow/wrist branches by sign,
and the preset's `posture.labels` map those signs to vendor names.

```cpp
#include <RobotKinematics/Posture/PostureResolver.h>

const std::unique_ptr<PostureResolver> resolver = PostureResolverFactory::create(config);
if (resolver) {
    const Result<ArmPosture> posture = resolver->classify(config, q);
    if (posture.ok()) {
        int shoulder = *posture.value.shoulder;   // -1 or +1
        int elbow    = *posture.value.elbow;
        int wrist    = *posture.value.wrist;
    }
}
```

Map a branch sign to its configured name (e.g. for `NachiMZ04D`, `shoulder == -1` is `"righty"`):

```cpp
const auto& axis = config.posture.labels.at("shoulder");
const std::string name = (shoulder == -1) ? axis.negative : axis.positive;
```

`fromLabels(config.posture, {{"shoulder","righty"}, ...})` converts names back to branch signs.

---

## 7. Validate a model

```cpp
#include <RobotKinematics/Model/RobotModelValidator.h>

const ModelValidationResult v = RobotModelValidator::validateSerialRobotConfig(config);
if (!v.ok()) {
    const ModelValidationIssue& first = v.issues.front();  // .status, .field, .message
}
```

Presets and the builder validate for you; you mainly need this if you assemble a config by hand.

---

## 8. Adapters: DH and URDF

### Standard Denavit–Hartenberg input

`DhAdapter` converts a standard-DH table into a canonical config. The per-joint transform is
`Rz(thetaOffset + q) * Tz(d) * Tx(a) * Rx(alpha)` (classic / distal DH).

```cpp
#include <RobotKinematics/Adapters/DhAdapter.h>

std::vector<StandardDhParameter> rows(6);
rows[0].jointId = "J1"; rows[0].d_m = 0.340; rows[0].alpha_rad = 1.5707963267948966; rows[0].a_m = 0.0;
rows[1].jointId = "J2"; rows[1].a_m = 0.260; rows[1].alpha_rad = 0.0;
// ... thetaOffset_rad, type (Revolute/Prismatic), limits, home per row ...

const Result<SerialRobotConfig> config =
    DhAdapter::fromStandardDh(RobotIdentity{"Acme","Arm6","Acme Arm 6","1.0"}, rows);
```

The adapter emits a revolute joint plus a fixed joint per DH row; `dof` counts only the movable
joints. Modified (Craig) DH is not implemented.

### URDF import / export (best-effort)

```cpp
#include <RobotKinematics/Adapters/UrdfAdapter.h>

const Result<std::string> urdf = UrdfAdapter::exportSerialRobot(config);

const Result<SerialRobotConfig> imported =
    UrdfAdapter::importSerialRobot(urdfText, /*base*/ "base_link", /*flange*/ "flange");
```

URDF cannot carry all canonical metadata (posture, solver hints, source references); those must
be supplied separately. Import expects a serial chain and returns a clear error for unsupported
structures.

---

## 9. Collision checks

Primitive collision is available in the default build. Mesh collision uses the same
backend-neutral API, but requires a mesh-backend-enabled build such as the Coal flow documented in
[building-and-linking.md](building-and-linking.md).

```cpp
#include <RobotKinematics/Collision/CollisionBackend.h>
#include <RobotKinematics/Collision/CollisionProfileJsonLoader.h>

const Result<CollisionProfile> profile =
    CollisionProfileJsonLoader::loadFile("presets/Nachi/MZ04/nachi_mz04d_collision.json");
if (!profile.ok()) {
    return;
}

CollisionCheckRequest request;
request.joints = q;
request.safetyMargin_m = 0.0;
request.returnAllPairs = true;

const CollisionCheckResult result =
    CollisionBackends::checkPrimitive(config, profile.value, request);
if (result.ok() && result.hasCollision) {
    for (const CollisionPairResult& pair : result.pairs) {
        if (pair.colliding) {
            // pair.linkA, pair.linkB, pair.distance_m, pair.contactCount
        }
    }
}
```

Mesh collision profiles are loaded separately:

```cpp
#include <RobotKinematics/Collision/MeshCollisionProfileJsonLoader.h>

const CollisionBackendInfo mesh = CollisionBackends::meshInfo();
if (mesh.available) {
    const Result<MeshCollisionProfile> meshProfile =
        MeshCollisionProfileJsonLoader::loadFile("presets/Nachi/MZ04/nachi_mz04d_mesh_collision.json");

    MeshCollisionCheckRequest meshRequest;
    meshRequest.joints = q;
    meshRequest.returnAllPairs = true;

    const CollisionCheckResult meshResult =
        CollisionBackends::checkMesh(config, meshProfile.value, meshRequest);
}
```

Collision profiles are not safety certification data. Primitive profiles are approximate/debug
geometry. Mesh profiles cover STL assets more accurately but still depend on authored
`meshToLink` transforms, source units, backend availability, and real fixture validation.
