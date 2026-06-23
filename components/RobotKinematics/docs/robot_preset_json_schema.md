# Robot Preset JSON Schema

## Decision

Robot preset files use schema id `robot-kinematics-preset/v1`.

The schema is intentionally close to the internal canonical model:

- links
- ordered joints
- joint origin transforms
- joint axes
- frames
- tools
- posture metadata
- solver metadata
- source references

All solver-facing numeric values are stored in SI units only:

- length: meter
- angle: radian

No JSON preset field performs implicit unit conversion. Human-readable notes can mention vendor units, but the values consumed by the library must already be normalized.

## Required Top-Level Fields

```text
schema
identity
units
topology
links
joints
frames
tools
defaultTool
posture
solver
sources
metadata
```

## Field Contract

### schema

Must equal:

```json
"robot-kinematics-preset/v1"
```

### identity

```json
{
  "vendor": "RobotKinematics",
  "model": "Virtual6DofTestArm",
  "name": "Virtual 6DOF Test Arm",
  "revision": "1.0.0"
}
```

### units

Must be:

```json
{
  "length": "m",
  "angle": "rad"
}
```

### topology

Phase 1 supports:

```json
{
  "type": "serial",
  "dof": 6
}
```

### links

Links are stable IDs. They may be referenced by visual or collision metadata, but phase 1 only requires IDs.

```json
[
  { "id": "base_link" },
  { "id": "link_1" },
  { "id": "flange" }
]
```

### joints

Joints are ordered from base to flange for serial robots.

```json
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
```

For revolute joints, `limits` and `home` are radians. For prismatic joints, they are meters.

### frames

```json
{
  "base": "base_link",
  "flange": "flange",
  "userFrames": [
    {
      "id": "vision_frame",
      "parent": "base_link",
      "transform": {
        "xyz_m": [0.0, 0.0, 0.0],
        "rpy_rad": [0.0, 0.0, 0.0]
      }
    }
  ]
}
```

### tools

```json
[
  {
    "id": "default",
    "name": "Default Tool",
    "flangeToTcp": {
      "xyz_m": [0.0, 0.0, 0.0],
      "rpy_rad": [0.0, 0.0, 0.0]
    }
  }
]
```

### posture

```json
{
  "resolver": "serial_6dof_shoulder_elbow_wrist",
  "labels": {
    "shoulder": { "negative": "lefty", "positive": "righty" },
    "elbow": { "negative": "below", "positive": "above" },
    "wrist": { "negative": "non-flip", "positive": "flip" }
  }
}
```

Preset-specific posture rules may be added under `posture.parameters`.

### solver

```json
{
  "default": "adaptive_damped_least_squares",
  "parameters": {
    "maxIterations": 200,
    "maxSeeds": 32,
    "positionTolerance_m": 0.000001,
    "orientationTolerance_rad": 0.000017453292519943296
  }
}
```

Missing parameters use library defaults.

### sources

```json
[
  {
    "type": "project_fixture",
    "title": "RobotKinematics virtual preset definition",
    "reference": "docs/robot_kinematics_spec.md",
    "appliesTo": ["dimensions", "joint_limits", "posture"]
  }
]
```

Preset implementation should preserve these references so future maintainers know where each robot parameter came from.

The loader stores source references in the canonical `SerialRobotConfig` so generated or built-in presets can be compared without losing provenance.

### metadata

Free-form object for non-solver metadata.

Unknown top-level fields are invalid. Put extension data inside `metadata`.

### collision profile references

Collision geometry is not a required top-level field in `robot-kinematics-preset/v1`.

Primitive collision profiles use a separate artifact with schema id
`robot-kinematics-collision/v1`. Accurate mesh collision profiles use
`robot-kinematics-collision-mesh/v1`. A preset may reference profiles from `metadata`:

```json
{
  "metadata": {
    "collisionProfile": "presets/Nachi/MZ04/nachi_mz04d_collision.json",
    "meshCollisionProfile": "presets/Nachi/MZ04/nachi_mz04d_mesh_collision.json"
  }
}
```

The preset loader is not required to load this profile automatically. Runtime code should load the
collision profile explicitly so kinematics presets remain usable without collision data.

## Collision Profile Schema Direction

Collision profile files use SI units only:

- length: meter
- angle: radian

MVP runtime shapes are `sphere` and `capsule`. STL triangle meshes are not runtime collision
geometry.

Minimum shape:

```json
{
  "schema": "robot-kinematics-collision/v1",
  "profile": {
    "id": "virtual_6dof_test_arm_conservative_primitives",
    "robotModel": "Virtual6DofTestArm",
    "units": { "length": "m", "angle": "rad" }
  },
  "geometries": [
    {
      "id": "link_2_capsule",
      "link": "link_2",
      "shape": "capsule",
      "geometryToLink": {
        "xyz_m": [0.0, 0.0, 0.0],
        "rpy_rad": [0.0, 0.0, 0.0]
      },
      "capsule": {
        "radius_m": 0.04,
        "length_m": 0.3
      },
      "margin_m": 0.005,
      "enabled": true
    }
  ],
  "disabledPairs": [
    {
      "a": "base_body",
      "b": "link_1_body",
      "reason": "adjacent_joint_contact"
    }
  ],
  "sources": [],
  "metadata": {}
}
```

For a sphere, use:

```json
{
  "shape": "sphere",
  "sphere": {
    "radius_m": 0.05
  }
}
```

Collision profile loaders must reject unknown top-level fields unless they are under `metadata`.

## Mesh Collision Profile Schema Direction

Mesh collision profile files use schema id `robot-kinematics-collision-mesh/v1`.

The mesh profile is separate from both the robot preset and the primitive collision profile. Mesh
profiles must explicitly declare source units, scale to meters, and the transform from mesh-local
coordinates into the canonical link frame.

Minimum shape:

```json
{
  "schema": "robot-kinematics-collision-mesh/v1",
  "profile": {
    "id": "nachi_mz04d_stl_mesh_collision",
    "robotModel": "MZ04D",
    "units": { "length": "m", "angle": "rad" },
    "backendPreference": ["coal", "fcl", "vtk_debug"]
  },
  "meshes": [
    {
      "id": "j2_mesh",
      "link": "link_2",
      "path": "MZ04-01_j2.stl",
      "format": "stl",
      "sourceUnits": "mm",
      "scaleToMeters": 0.001,
      "meshToLink": {
        "xyz_m": [0.0, 0.0, 0.0],
        "rpy_rad": [0.0, 0.0, 0.0]
      },
      "margin_m": 0.0,
      "enabled": true,
      "quality": {
        "mode": "original",
        "triangleCount": null,
        "simplifiedFrom": null,
        "maxSimplificationError_m": null
      }
    }
  ],
  "disabledPairs": [],
  "sources": [],
  "metadata": {}
}
```

Runtime mesh collision backends are optional and must stay behind RobotKinematics-owned adapter
interfaces.

When a mesh profile is loaded with `MeshCollisionProfileJsonLoader::loadFile(path)`, relative mesh
paths are resolved relative to the directory containing that mesh-profile JSON file. Profiles loaded
from an in-memory JSON string with `loadJson(json)` preserve paths as authored, so programmatic users
must provide absolute paths or set their own working-directory convention before passing the profile
to a backend.

Numeric mesh fields are strict JSON numbers. `meshToLink.xyz_m`, `meshToLink.rpy_rad`,
`scaleToMeters`, `margin_m`, `quality.triangleCount`, and `quality.maxSimplificationError_m` must not
be strings or non-finite values.
