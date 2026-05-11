# Arkanoid-Style Tiny Game

![Gameplay Demo](doc/assets/arkanoid-gif.gif)

## Why This Project Exists

Many small game projects add abstraction before the gameplay needs it.
This project keeps the runtime small and explicit:

- match architecture to problem size
- keep gameplay mutation centralized
- keep rendering read-only

## Current Scope

- solid-color primitives (no texture/sprite pipeline)
- no menus or UI systems
- no score HUD
- no powerups or special brick types
- no lives counter
- no game-over or win screen
- board clear auto-restarts a new game

## Requirements

- Windows PowerShell workflow
- Visual Studio with MSVC (presets use generator `Visual Studio 18 2026`)
- CMake 4.2 or newer is required for the preset workflow.
- `VCPKG_ROOT` set to a valid vcpkg install (or pass `-VcpkgRoot` to scripts)

## How It Runs

`SDL events -> input sample -> fixed-step simulation -> read-only render`

- `src/main.cpp` is the entrypoint
- `src/app/application.cpp` owns SDL setup, the event loop, and fixed-step timing
- `src/render/render_frame.cpp` renders read-only from `GameState`
- In runtime code, gameplay mutation is performed through `arkanoid::Game::update()`
- `GameState` is authoritative runtime state

## Controls

- Left Arrow: move paddle left
- Right Arrow: move paddle right
- Space: serve on press in `BallReady`
- Close window: quit

## Docs

- [Architecture](doc/architecture.md) - ownership boundaries and state model
- [Game Loop](doc/game_loop.md) - fixed-step timing and frame/update flow
- [Release Gate](doc/release_gate.md) - packaged Windows release checks

## Running

PowerShell usage: from PowerShell, use direct script invocation (`.\tools\windows\*.ps1` or `./tools/windows/*.ps1`) after execution policy is handled. For a one-off blocked session, run:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

then use the direct commands below. When launching from a non-PowerShell shell or needing a one-off fallback:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\windows\build.ps1
```

One-line build loop
```powershell
# set once per shell (or pass -VcpkgRoot directly to the script)
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

# debug build (auto-launches game)
.\tools\windows\build.ps1

# debug build without auto-launch
.\tools\windows\build.ps1 -Config Debug -NoRun

# release build (does not auto-launch)
.\tools\windows\build.ps1 -Config Release

# clean debug build without auto-launch
.\tools\windows\build.ps1 -Clean -Config Debug -NoRun

# reuse an existing configure
.\tools\windows\build.ps1 -SkipConfigure -Config Debug -NoRun

# build with an explicit vcpkg root
.\tools\windows\build.ps1 -VcpkgRoot "C:\path\to\vcpkg" -Config Release
```

`build.ps1` supports `-Config Debug|Release`, `-Clean`, `-SkipConfigure`, `-NoRun`, and `-VcpkgRoot <path>`. Debug auto-launches by default after a successful build, Release is build-only by default, and `-NoRun` suppresses Debug launch. `-Clean` removes the generated CMake build output under `out/build-win-vcpkg` before configure/build. `-SkipConfigure` builds from the existing CMake cache; it cannot be combined with `-Clean`.

Run tests through the dedicated test entry point:

```powershell
# debug tests
.\tools\windows\test.ps1

# release tests
.\tools\windows\test.ps1 -Config Release

# reuse an existing configure and filter tests
.\tools\windows\test.ps1 -SkipConfigure -TestFilter GameState
```

`test.ps1` supports `-Config Debug|Release`, `-Clean`, `-SkipConfigure`, `-TestFilter <regex>`, and `-VcpkgRoot <path>`. It builds `arkanoid_tests` before running CTest and fails if no tests are discovered or matched. `-SkipConfigure` requires an existing `out/build-win-vcpkg/CMakeCache.txt`; it cannot be combined with `-Clean`.

Format C/C++ source and header-like files:

```powershell
.\tools\windows\format.ps1
```

`format.ps1` runs `clang-format -i` only on `.cpp`, `.hpp`, `.h`, `.cc`, `.cxx`, `.hh`, `.hxx`, and `.ixx` files. It excludes non-source files, `.git`, generated/output directories, `third_party`, and `external`.

## Static Analysis

One-line
```powershell
.\tools\windows\analyze.ps1
```

Or
```powershell
cmake --preset windows-vcpkg-analyze
cmake --build --preset windows-debug-analyze
ctest --preset windows-debug-analyze
```

`analyze.ps1` runs configure + build + ctest on the analyze presets.

```powershell
.\tools\windows\analyze.ps1
.\tools\windows\analyze.ps1 -Clean
.\tools\windows\analyze.ps1 -SkipConfigure
```

`analyze.ps1` supports `-Clean`, `-SkipConfigure`, and `-VcpkgRoot <path>`. It fails if the analyze CTest lane discovers or runs zero tests. `-SkipConfigure` requires an existing `out/build-win-vcpkg-analyze/CMakeCache.txt`; it cannot be combined with `-Clean`.

## Release

Run the Windows release gate:
```powershell
.\tools\windows\release.ps1

# clean release build and package outputs first
.\tools\windows\release.ps1 -Clean

# record a caller-provided identity in release JSON metadata
.\tools\windows\release.ps1 -BuildIdentity "ci-20260511.1"

# release with an explicit vcpkg root
.\tools\windows\release.ps1 -VcpkgRoot "C:\path\to\vcpkg"
```

`release.ps1` supports `-Clean`, `-BuildIdentity <text>`, and `-VcpkgRoot <path>`. It validates, tests, smokes, packages, and writes release status/latest JSON. `-Clean` removes the Release CMake output under `out/build-win-vcpkg` plus release-controlled outputs under `out/dist` before starting a fresh run. `-BuildIdentity` is recorded as `buildIdentity` in release metadata without changing the generated run ID or artifact paths. See [Release Gate](doc/release_gate.md) for the full gate breakdown.

Generated outputs are kept under `out/`:

- `out/build-win-vcpkg`
- `out/build-win-vcpkg-analyze`
- `out/dist/install`
- `out/dist/arkanoid-win64.zip`
- `out/dist/release-status.json`
- `out/dist/release-latest.json`
- `out/dist/_runs/<run-id>`

## Open in Visual Studio

1. Install Visual Studio with the **Desktop development with C++** workload.
2. Ensure `VCPKG_ROOT` is set to your vcpkg installation path.
3. Open the repository root folder in Visual Studio (`File > Open > Folder`).
4. Select configure preset `windows-vcpkg` for x64 vcpkg builds.
5. Build in `Debug` or `Release` using build presets `windows-debug` and `windows-release`; tests use `windows-debug-tests` and `windows-release-tests`.
6. Build directories are `out/build-win-vcpkg` and `out/build-win-vcpkg-analyze`.
7. If prompted about a toolchain/cache/platform mismatch, click **Delete and regenerate cache** or rerun the relevant script with `-Clean`.
8. Build and run target `arkanoid`.

## License

This project is licensed under the MIT License.

Third-party libraries are licensed separately.
See [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES.txt) for details.
