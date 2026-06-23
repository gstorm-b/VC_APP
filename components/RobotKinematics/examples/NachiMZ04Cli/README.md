# Nachi MZ04 CLI Example

This example is a small console application that uses the built-in `NachiMZ04D`
preset to run:

- forward kinematics for a known joint vector;
- inverse kinematics back to the same flange pose;
- primitive self-collision checking with `presets/Nachi/MZ04/nachi_mz04d_collision.json`;
- mesh-backend availability reporting.

It has no UI and does not require VTK. Primitive collision is used so the example
works with the default core library build.

## Build

From the repository root:

```powershell
scripts\build_example_nachi_mz04_cli_msvc.bat
```

The script builds the core library first and then writes the example output under:

```text
examples/NachiMZ04Cli/build/msvc
```

Manual qmake flow:

```powershell
scripts\build_msvc.bat
mkdir examples\NachiMZ04Cli\build\msvc
cd examples\NachiMZ04Cli\build\msvc
qmake ..\..\NachiMZ04Cli.pro
nmake
```

## Run

```powershell
examples\NachiMZ04Cli\build\msvc\release\NachiMZ04Cli.exe
```

Run the executable from anywhere under the repository. The example searches upward
from the executable directory and current directory to find the preset collision
profile.

## Notes

- All core calculations are meter and radian.
- `JointVector::fromDegrees` is used only as an explicit input helper for this
  revolute 6-axis robot.
- The primitive collision profile is a fast approximate/debug profile. Use the
  mesh collision API with a Coal-enabled build when exact STL coverage is needed.
