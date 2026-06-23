# Building and Linking

RobotKinematics builds as a **static library** (`RobotKinematics.lib` / `libRobotKinematics.a`).
This page covers building it and linking it into your own application.

## Dependencies

| Dependency | How it is provided | Why |
|---|---|---|
| **Qt 6 Core** | External (you install it) | The library links `QtCore`; `PresetJsonLoader` uses Qt's JSON + file IO. |
| **Eigen** | Bundled in `third_party/eigen` (header-only) | All linear algebra and the `Pose` transform type. |
| **C++17** | Compiler flag | Standard the library targets. |
| **Qt Test** | External | Only needed to build/run the test suite, not to use the library. |

> Even if you never touch JSON presets, the static library still links `QtCore` (it is part of
> the same translation units). Your application must therefore link `Qt6Core`. If a Qt-free
> build is a hard requirement for you, raise it — `PresetJsonLoader` is the only Qt-dependent
> compilation unit and could be made optional.

Primary compiler is **MSVC**; **MinGW** is a compatibility target.

## Build the library and tests

From the repository root:

```powershell
scripts\build_msvc.bat
scripts\test_msvc.bat
```

This produces:

- the static library under the build's `lib/` directory, e.g. `build/msvc/lib`, and
- the test executable `RobotKinematicsTests.exe` under the build's `tests/` directory.

A successful run prints one line per suite ending in `PASS` and exits with code `0`.

> Run the test executable from the **repository root**. Preset-loading tests look for
> `presets/*.json` using relative paths.

For a clean MSVC rebuild plus tests:

```powershell
scripts\rebuild_msvc.bat
```

For MinGW compatibility:

```powershell
scripts\build_mingw.bat
scripts\test_mingw.bat
```

## Build no-UI examples

Two console examples are available for library users:

```powershell
scripts\build_example_nachi_mz04_cli_msvc.bat
scripts\build_example_custom_preset_cli_msvc.bat
```

Outputs stay under each example folder:

- `examples/NachiMZ04Cli/build/msvc/release/NachiMZ04Cli.exe`
- `examples/CustomPresetCli/build/msvc/release/CustomPresetCli.exe`

See each example README for the expected output and the manual qmake flow.

## Third-party mesh backend dependencies

Optional mesh-backend dependencies are kept in the repository under `third_party/`:

- source checkouts: `third_party/<name>`;
- Windows/MSVC build trees: `third_party/build/<name>`;
- local install roots used by qmake scripts: `third_party/install/<name>`.

The `scripts/build_third_party_*_msvc.bat` scripts are the supported path for building these
dependencies on Windows 11 with MSVC/Visual Studio 2022. Set `VCVARS` if Visual Studio is installed
outside the default location. Set `QT_CMAKE` if Qt's bundled CMake is not at
`C:\Qt\Tools\CMake_64\bin\cmake.exe`.

### Shadow (out-of-source) build

The scripts above are shadow builds that use `build/msvc/` and `build/mingw/`.
If you need to run qmake manually, create a build directory, run `qmake` pointing
at the top-level `.pro`, then build:

```powershell
mkdir build_cli; cd build_cli
qmake ..\RobotKinematics.pro
nmake
cd ..
.\build_cli\tests\RobotKinematicsTests.exe
```

## Linking RobotKinematics into your application

You have two practical options.

### Option A — add the sources/headers to your own qmake project

Point your `.pro` at the include roots and link the built static lib:

```pro
INCLUDEPATH += \
    /path/to/RobotKinematics/include \
    /path/to/RobotKinematics/third_party/eigen

LIBS += -L/path/to/RobotKinematics/build/msvc/lib -lRobotKinematics

QT += core
CONFIG += c++17
```

### Option B — CMake (or any build system)

There is no CMake project shipped, but linking is standard once the library is built:

- **Include paths:** `include/` and `third_party/eigen/`.
- **Link:** the built `RobotKinematics` static lib **and** `Qt6::Core`.
- **Standard:** C++17.

Sketch:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(myapp main.cpp)
target_compile_features(myapp PRIVATE cxx_std_17)
target_include_directories(myapp PRIVATE
    /path/to/RobotKinematics/include
    /path/to/RobotKinematics/third_party/eigen)
target_link_libraries(myapp PRIVATE
    /path/to/RobotKinematics/build/msvc/lib/RobotKinematics.lib
    Qt6::Core)
```

At runtime your app needs the Qt 6 Core shared library on its path (e.g. add the Qt `bin`
directory to `PATH` on Windows).

## Including headers

Include via the `RobotKinematics/...` prefix (the include root is `include/`):

```cpp
#include <RobotKinematics/Kinematics/SerialRobotKinematics.h>
#include <RobotKinematics/Kinematics/ForwardKinematics.h>
#include <RobotKinematics/Model/SerialRobotConfigBuilder.h>
#include <RobotKinematics/Presets/PresetJsonLoader.h>
```

There is no single umbrella header; include the specific headers you use. See
[api-reference.md](api-reference.md) for the per-module header list.
