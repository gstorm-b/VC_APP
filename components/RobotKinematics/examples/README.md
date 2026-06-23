# Examples

RobotKinematics examples are kept outside the core library. Each example owns its build output under
its own `build/` folder.

## No-UI Console Examples

- [NachiMZ04Cli](NachiMZ04Cli/README.md): built-in Nachi MZ04D preset, forward kinematics,
  inverse kinematics, and primitive collision checking.
- [CustomPresetCli](CustomPresetCli/README.md): create a new canonical robot preset in C++ with
  `SerialRobotConfigBuilder`, then verify it with FK/IK.

## Visual Example

- [Robot3DVizualize](Robot3DVizualize/README.md): Qt 6 + VTK 3D viewer for Nachi MZ04D with
  primitive and optional mesh collision backend modes.
