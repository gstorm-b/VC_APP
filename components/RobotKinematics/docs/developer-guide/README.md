# RobotKinematics Developer Guide

This guide is for **developers who want to use RobotKinematics as a library** in their own
C++ application. (If you are working *on* the library itself, start with
[../developer_onboarding.md](../developer_onboarding.md) and
[../planning/robot_kinematics_implementation_plan.md](../planning/robot_kinematics_implementation_plan.md).)

RobotKinematics is a C++17 / Eigen backend for serial industrial-robot kinematics. It provides:

- a canonical robot model (links, joints, frames, tools, limits, posture/solver metadata);
- forward kinematics (flange, tool/TCP, and user-frame relative poses);
- inverse kinematics — a hybrid solver that uses a closed-form analytic plugin for supported
  morphologies (spherical-wrist articulated 6R) and falls back to an adaptive damped
  least-squares numerical solver otherwise;
- joint-vector and joint-limit validation;
- frame and tool registries;
- robot presets (built-in C++ and `robot-kinematics-preset/v1` JSON);
- adapters for standard DH input and URDF import/export;
- primitive self-collision checks as a fast approximate/debug path;
- optional Coal-backed STL mesh collision through backend-neutral RobotKinematics APIs.

There is **no core UI, no path planning, and no dynamics**. UI code lives only under examples.

## Contents

| Document | What it covers |
|---|---|
| [building-and-linking.md](building-and-linking.md) | Building the static library and linking it into your own app (Qt 6 Core + Eigen). |
| [usage-guide.md](usage-guide.md) | Task-by-task examples: load a robot, FK, IK, frames, tools, posture, custom models, DH/URDF. |
| [api-reference.md](api-reference.md) | Concise reference of the public headers, types, and functions. |
| [conventions-and-gotchas.md](conventions-and-gotchas.md) | Units, the RPY/Euler convention, `solveAll` semantics, IK frame limitations, and other traps. |
| [decisions/](decisions/) | Architecture Decision Records (the *why* behind the design). |

Runnable examples:

- [../../examples/NachiMZ04Cli](../../examples/NachiMZ04Cli/README.md): built-in Nachi preset, FK, IK, and primitive collision.
- [../../examples/CustomPresetCli](../../examples/CustomPresetCli/README.md): programmatic custom preset creation with FK/IK validation.
- [../../examples/Robot3DVizualize](../../examples/Robot3DVizualize/README.md): Qt/VTK visualizer for Nachi MZ04D, including optional mesh collision modes.

## Quick start

```cpp
#include <RobotKinematics/Kinematics/SerialRobotKinematics.h>
#include <RobotKinematics/Kinematics/ForwardKinematics.h>
#include <RobotKinematics/Presets/NachiMZ04D.h>

using namespace RobotKinematics;

int main()
{
    // 1. Get a robot model (a preset, a JSON file, or a builder — see usage-guide.md).
    const SerialRobotConfig config = Presets::nachiMZ04D();

    // 2. Forward kinematics: joint angles -> flange pose (base frame, meters/radians).
    const JointVector q = JointVector::fromDegrees({28.16, -18.81, 163.84, -0.71, 35.89, 152.73});
    const Pose flange = ForwardKinematics::flangePose(config, q);

    // 3. Inverse kinematics: pose -> joints.
    const SerialRobotKinematics robot(config);
    IKRequest request;
    request.targetPose = flange;   // target flange pose (default tool)
    request.seedJoint = q;         // optional starting/closest-to seed
    const IKResult result = robot.solve(request);

    if (result.ok()) {
        const JointVector solved = result.best().joints;  // a 6-element JointVector (radians)
    }
    return 0;
}
```

## Source layout

```text
include/RobotKinematics/   Public headers (this is your include root)
  Core/                    Pose, JointVector, Units, Ids, Result/KinematicsStatus
  Model/                   SerialRobotConfig + value types, validator, builder, registries
  Kinematics/              ForwardKinematics, SerialRobotKinematics, IK request/result types
  Solvers/                 Numerical + analytic IK solvers (used via SerialRobotKinematics)
  Collision/               Primitive and optional mesh collision APIs
  Posture/                 ArmPosture, PostureResolver
  Presets/                 Virtual6DofTestArm, NachiMZ04D, PresetJsonLoader
  Adapters/                DhAdapter, UrdfAdapter
src/                       Implementations (mirror of include/)
presets/                   JSON presets (robot-kinematics-preset/v1)
third_party/eigen/         Bundled Eigen (header-only)
tests/                     Qt Test unit + integration suites
```

All public symbols live in the `RobotKinematics` namespace (presets in
`RobotKinematics::Presets`, unit helpers in `RobotKinematics::units`).

## Two-minute mental model

- A robot is a `SerialRobotConfig` (plain data: links + joints + frames + tools + metadata).
- `ForwardKinematics` is a set of free functions that operate on a config + a `JointVector`.
- `SerialRobotKinematics` wraps a config and exposes `solve` / `solveAll` for IK, handling
  frame and tool resolution and choosing the solver.
- Everything you get back is a `Pose` (an `Eigen::Isometry3d` wrapper) or a `JointVector`,
  in **meters and radians**. Conversion helpers (`units::mm`, `units::deg`, `Pose::*_mm_deg`,
  `JointVector::fromDegrees`) are explicit at the boundary.
- Operations that can fail return a `Result<T>` or an `IKResult` carrying a
  `KinematicsStatus` — there are no exceptions to catch.
