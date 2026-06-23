# Custom Preset CLI Example

This example creates a new robot preset in C++ with `SerialRobotConfigBuilder`.
It defines a simple 6DOF Cartesian wrist:

- J1/J2/J3 are prismatic X/Y/Z axes in meters;
- J4/J5/J6 are revolute X/Y/Z axes in radians;
- the default tool is the identity flange-to-TCP transform.

The app validates the generated canonical config, runs forward kinematics, and
then solves inverse kinematics back to the same flange pose.

## Build

From the repository root:

```powershell
scripts\build_example_custom_preset_cli_msvc.bat
```

The script builds the core library first and then writes the example output under:

```text
examples/CustomPresetCli/build/msvc
```

Manual qmake flow:

```powershell
scripts\build_msvc.bat
mkdir examples\CustomPresetCli\build\msvc
cd examples\CustomPresetCli\build\msvc
qmake ..\..\CustomPresetCli.pro
nmake
```

## Run

```powershell
examples\CustomPresetCli\build\msvc\release\CustomPresetCli.exe
```

## Turning This Into A JSON Preset

Use the same canonical model concepts in `docs/robot_preset_json_schema.md`:

- keep numeric lengths in meters and angles in radians;
- list links first, then joints in chain order;
- use explicit joint `type`, `parentLinkId`, `childLinkId`, `axis`, `origin`, and `limits`;
- add a default tool, even if it is the identity transform;
- preserve source references for any real robot dimensions or limits.

For a production preset, add JSON loader tests comparing the JSON preset with a
C++ fallback or with source-reference fixtures.
