# Customer Installer Packaging

**Status:** Open release risk  
**Last updated:** 2026-06-24  
**Scope:** customer-facing installer or deployment package, not developer build folders

See also [phase4_product_release_plan.md](phase4_product_release_plan.md) for
the first-release product shape and release gate.

## Why This Exists

The Debug build folder now deploys the RobotKinematics runtime DLLs and
`robot_assets/` next to `ncr_picking.exe`, but that only proves local developer
execution. Customer packaging is a separate release concern: a clean target
machine must run without relying on source-tree paths, developer PATH entries,
Qt Creator kits, or previously installed third-party DLLs.

## Installer Payload Checklist

Ship these beside the installed executable or in a layout that the executable
can resolve deterministically:

- The application binary and any project-owned helper executables.
- Qt runtime DLLs and plugins, including at minimum `platforms/`, `imageformats/`,
  and any SQL/style/plugin folders used by the app. Prefer `windeployqt` as an
  initial collector, then audit the result manually.
- OpenCV runtime DLLs matching the build configuration and ABI.
- Basler Pylon runtime DLLs or a documented prerequisite installer.
- Advanced Docking System runtime DLLs if linked dynamically.
- RobotKinematics runtime DLLs currently required by build-folder deploy:
  - `coal.dll`
  - `assimp-vc143-mt.dll`
  - `boost_serialization-vc143-mt-x64-1_87.dll`
  - `boost_filesystem-vc143-mt-x64-1_87.dll`
- RobotKinematics mesh assets under `robot_assets/Nachi/MZ04`, including the
  `simplified/` mesh profile.
- Third-party license notices for Qt, OpenCV, Basler, Coal, Assimp, Boost, and
  any transitive libraries shipped in the installer.

## Clean-Machine Verification

Run this before a customer release:

- Install on a machine or VM that has no Qt Creator kit, no project source tree,
  and no developer PATH additions.
- Launch the app directly from the installed shortcut and from the installed
  executable path.
- Open or create a Localization project.
- Add/configure the first-release device set: Basler GigE camera, Mitsubishi MC
  PLC, VisionOutput TCP/IP, and RobotKinematics advisory checking.
- Confirm `robot_assets/Nachi/MZ04` resolves from the install directory and the
  mesh-collision profile loads without falling back to source-tree discovery.
- Run a simulated or real Localization cycle and confirm PLC outputs,
  VisionOutput send, dashboard lamps/faults, result table, and task log.
- Capture missing-DLL failures with a dependency scanner before changing PATH.

## Current Boundary

`components/RobotKinematics/robotkinematics.pri` is allowed to copy DLLs/assets
for build-folder runs. The customer installer must not depend on
`QMAKE_POST_LINK` as the only deployment mechanism; packaging needs its own
explicit payload manifest and clean-machine smoke result.
