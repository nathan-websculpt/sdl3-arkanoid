# Arkanoid-Style Tiny Game

![Gameplay Demo](doc/assets/arkanoid-gif.gif)

Small C++20 / SDL3 Arkanoid-style game with a Windows-first CMake/vcpkg workflow. The runtime stays intentionally direct: SDL input is sampled, fixed-step gameplay mutates `GameState`, and rendering reads from that state.

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

## Quick Start

Requirements:

- Windows PowerShell
- Visual Studio with MSVC; presets use `Visual Studio 18 2026`
- CMake 4.2 or newer
- vcpkg, either through `VCPKG_ROOT` or each script's `-VcpkgRoot`

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

.\tools\windows\build.ps1
.\tools\windows\test.ps1
.\tools\windows\analyze.ps1
.\tools\windows\release.ps1
```

For full local safety validation, run the fail-closed analyzer lane before the packaged release gate.

PowerShell may require a one-session execution-policy bypass:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

## Common Commands

```powershell
# Debug build; launches the game unless -NoRun is supplied.
.\tools\windows\build.ps1
.\tools\windows\build.ps1 -Config Debug -NoRun

# Release build; never auto-launches.
.\tools\windows\build.ps1 -Config Release

# Tests.
.\tools\windows\test.ps1
.\tools\windows\test.ps1 -Config Release
.\tools\windows\test.ps1 -TestFilter GameState

# Fail-closed static analysis diagnostics build lane.
.\tools\windows\analyze.ps1

# Local portable Windows release gate.
.\tools\windows\release.ps1
.\tools\windows\release.ps1 -Clean
.\tools\windows\release.ps1 -BuildIdentity "ci-20260512.1"
```

Generated output lives under `out/`, primarily `out/build-win-vcpkg`, `out/build-win-vcpkg-analyze`, and `out/dist`.

## Controls

- Left Arrow: move paddle left
- Right Arrow: move paddle right
- Space: serve on press in `BallReady`

## Docs

- [Development](doc/development.md): authoritative Windows helper-script and preset reference
- [Release Gate](doc/release_gate.md): packaged Windows release validation details
- [Architecture](doc/architecture.md): ownership boundaries and state model
- [Game Loop](doc/game_loop.md): fixed-step timing and frame/update flow

## Project Shape

- `src/main.cpp` is the entrypoint
- `src/app/application.cpp` owns SDL setup, the event loop, and fixed-step timing
- `src/render/render_frame.cpp` renders read-only from `GameState`
- Gameplay mutation is performed through `arkanoid::Game::update()`
- `GameState` is authoritative runtime state

## Open in Visual Studio

1. Install Visual Studio with the **Desktop development with C++** workload.
2. Ensure `VCPKG_ROOT` is set to your vcpkg installation path.
3. Open the repository root folder in Visual Studio (`File > Open > Folder`).
4. Select configure preset `windows-vcpkg`.
5. Build in `Debug` or `Release` (corresponds to build presets `windows-debug` and `windows-release`).
6. If prompted about a toolchain/cache mismatch, click **Delete and regenerate cache**.
7. Build and run target `arkanoid`.

## License

This project is licensed under the MIT License. Third-party libraries are licensed separately; see [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES.txt).
