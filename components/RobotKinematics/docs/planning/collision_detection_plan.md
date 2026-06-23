# Primitive Collision Detection Plan

## Decision

Collision detection is approved as a post-base-library extension.

The implementation must prioritize fast and lightweight self-collision checks. Runtime collision
checking uses primitive shapes, not STL triangle meshes. STL files may be used only by helper tools
to derive or preview primitive collision volumes.

Update: primitive collision has landed and remains the fast approximate/debug backend. Accurate
STL-part coverage is now planned separately in `docs/planning/mesh_collision_backend_plan.md`.

## Goals

- Detect self-collision for a serial robot at a single joint state.
- Keep the core runtime dependency set unchanged: Eigen, Qt, and Qt Test only.
- Reuse existing FK output to place collision geometry in the base frame.
- Provide conservative, configurable primitive collision volumes per link.
- Support disabled collision pairs for adjacent links or known allowed contacts.
- Return structured pair-level diagnostics so UI/example apps can highlight the colliding links.
- Add an STL-to-primitive helper path that assists authors but does not become the runtime checker.

## Non-Goals

- No physical safety certification or claim that the model fully matches a real robot.
- No runtime triangle-mesh or STL self-collision in the MVP.
- No VTK dependency in the core library.
- No path planning, trajectory checking, swept-volume checking, dynamics, torque, or velocity logic.
- No environment/world-object collision in the first collision milestone.
- No automatic generation of perfect collision geometry from CAD.

## Architecture

Collision detection is a separate module under the core library. It consumes:

- `SerialRobotConfig`
- `JointVector`
- `ForwardKinematics::computeChain(...)`
- a collision profile containing primitive geometry and pair-filter rules

The checker must not modify FK, IK, presets, or solver behavior. Higher-level code may combine
joint-limit validation and collision validation, but the collision checker remains a separate API.

## Runtime Geometry Model

Use SI units only.

```cpp
enum class CollisionShapeType {
    Sphere,
    Capsule
};

struct CollisionSphere {
    double radius_m = 0.0;
};

struct CollisionCapsule {
    double radius_m = 0.0;
    double length_m = 0.0;
    // Capsule local frame convention:
    // center is at geometry frame origin, segment axis is local +Z,
    // endpoints are at z = +/- length_m / 2.
};

struct CollisionShape {
    CollisionShapeType type;
    CollisionSphere sphere;
    CollisionCapsule capsule;
};

struct CollisionGeometry {
    std::string id;
    std::string linkId;
    CollisionShape shape;
    Pose geometryToLink;
    double margin_m = 0.0;
    bool enabled = true;
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

Sphere and capsule are the MVP shapes because they are cheap, stable, and enough to approximate most
industrial-arm links conservatively. Box, cylinder, convex mesh, or triangle mesh support requires a
separate scope decision after the MVP is measured.

## Check Request And Result

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
    bool colliding = false;
    double distance_m = 0.0;
};

struct CollisionCheckResult {
    KinematicsStatus status = KinematicsStatus::Ok;
    bool hasCollision = false;
    std::vector<CollisionPairResult> pairs;
    std::string message;

    bool ok() const;
};
```

`status == Ok` means the collision check executed successfully. A found collision is represented by
`hasCollision == true`, not by a failure status. Existing statuses cover invalid model/config/request
cases; do not add `CollisionDetected` to `KinematicsStatus` for the MVP.

## Pair Filtering

The checker must skip:

- disabled geometry ids in the profile;
- pairs listed in `disabledPairs`;
- exact same geometry;
- optionally, pairs on the same link unless explicitly enabled later.

Adjacent links commonly touch at joints, so collision profiles should explicitly disable those pairs
with a reason such as `adjacent_joint_contact`.

## Algorithms

MVP algorithms:

- broad phase: bounding sphere per geometry in base frame;
- narrow phase:
  - sphere-sphere distance;
  - sphere-capsule distance;
  - capsule-capsule distance using closest distance between line segments.

Collision condition:

```text
distance_m <= (shape_margin_a + shape_margin_b + request.safetyMargin_m)
```

For primitive pairs, `distance_m` should represent signed or clamped clearance consistently. MVP
recommendation: return clearance where negative means penetration depth and positive means gap.

## Collision Profile JSON

Do not add collision data as a required field in `robot-kinematics-preset/v1`.

Use a separate profile artifact:

```json
{
  "schema": "robot-kinematics-collision/v1",
  "profile": {
    "id": "nachi_mz04d_conservative_primitives",
    "robotModel": "NachiMZ04D",
    "units": { "length": "m", "angle": "rad" }
  },
  "geometries": [
    {
      "id": "link_2_upper_arm_capsule",
      "link": "link_2",
      "shape": "capsule",
      "geometryToLink": {
        "xyz_m": [0.0, 0.0, 0.0],
        "rpy_rad": [0.0, 0.0, 0.0]
      },
      "capsule": {
        "radius_m": 0.04,
        "length_m": 0.32
      },
      "margin_m": 0.005
    }
  ],
  "disabledPairs": [
    {
      "a": "base_body",
      "b": "link_1_body",
      "reason": "adjacent_joint_contact"
    }
  ],
  "sources": [
    {
      "type": "project_estimate",
      "title": "Conservative primitive approximation from visual CAD",
      "reference": "presets/Nachi/MZ04",
      "appliesTo": ["collision_geometry"]
    }
  ],
  "metadata": {}
}
```

Profiles may be linked from preset `metadata`, but the preset schema remains stable. Runtime code
should load collision profiles explicitly.

## STL-To-Primitive Helper

The helper is authoring support, not runtime collision logic.

Allowed scope:

- read STL vertices from a file;
- compute basic mesh statistics: bounding box, centroid, axis lengths;
- propose a sphere or capsule primitive in mesh-local coordinates;
- emit a draft `robot-kinematics-collision/v1` snippet;
- optionally save debug numbers for manual review.

Out of scope:

- no automatic final approval of generated collision volumes;
- no runtime VTK dependency;
- no triangle-triangle collision;
- no exact CAD semantic reconstruction.

Implementation note: prefer a lightweight STL reader implemented for binary/ascii STL or a small
internal utility. Do not add a third-party geometry dependency without user approval.

## Example Integration

The Qt/VTK example can consume the core collision API after the core module exists.

Rules:

- Keep persistent collision UI controls in `mainwindow.ui`.
- Keep styles in QSS loaded through the existing QRC.
- Use the existing `.pro` file.
- The example may highlight colliding visual actors, but collision truth comes from the primitive
  core API, not from VTK actor geometry.

## Testing Expectations

Add Qt Test coverage for:

- collision profile validation;
- invalid link ids and duplicate geometry ids;
- disabled-pair behavior;
- sphere-sphere, sphere-capsule, and capsule-capsule distances;
- FK-based geometry placement;
- self-collision positive and negative fixtures;
- safety margin behavior;
- JSON profile loading and C++ fallback equivalence for any built-in profile;
- STL helper output on tiny synthetic STL fixtures.

## Implementation Phases

### Phase 9.1: Collision Spec And Public API

- Add public collision headers under `include/RobotKinematics/Collision`.
- Define request/result/profile/shape structs.
- Document status semantics: collision is not an error status.
- Add compile/API tests.

### Phase 9.2: Collision Profile Validation

- Validate profile ids, shape dimensions, link references, duplicate ids, and disabled pairs.
- Reject non-positive radius/length values.
- Add focused validator tests.

### Phase 9.3: FK Placement And Pair Generation

- Transform each enabled geometry into base frame via FK.
- Generate candidate pairs after disabled-pair filtering.
- Add tests using a tiny 2-link/3-link robot fixture.

### Phase 9.4: Primitive Self-Collision Checker

- Implement broad phase with bounding spheres.
- Implement sphere-sphere, sphere-capsule, and capsule-capsule narrow phase.
- Return colliding pair diagnostics and clearances.
- Add positive/negative/self-collision tests.

### Phase 9.5: Built-In Conservative Profiles

- Add a project-owned primitive profile for `Virtual6DofTestArm`.
- Add a conservative primitive profile for `NachiMZ04D` after visual review.
- Preserve source notes and mark real profiles as conservative approximations, not safety-rated
  physical geometry.

### Phase 9.6: Collision Profile JSON Loader

- Add `robot-kinematics-collision/v1` loader.
- Keep preset JSON loader unchanged except optional metadata references.
- Add JSON/C++ profile equivalence tests.

### Phase 9.7: STL-To-Primitive Authoring Helper

- Add a helper that reads STL and proposes sphere/capsule primitives.
- The helper output must be manually reviewed before becoming a profile.
- Add synthetic STL fixture tests.

### Phase 9.8: Example Visualization Integration

- Add collision status and optional safety-margin control to the Qt/VTK example.
- Highlight links involved in colliding pairs.
- Keep VTK and all UI/styling changes example-only.

## Handoff Checklist

When handing off collision work, include:

- phase/task number from this document;
- files changed;
- tests added and run;
- whether any collision volumes are conservative estimates;
- disabled pair rationale;
- performance notes if measured;
- limitations and remaining shape support.
