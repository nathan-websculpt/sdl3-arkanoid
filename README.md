# Arkanoid-Style Tiny Game

![Gameplay Demo](doc/assets/arkanoid-gif.gif)

Small C++20 / SDL3 Arkanoid-style game with a Windows-first CMake/vcpkg workflow. The runtime stays intentionally direct: SDL input is sampled, fixed-step gameplay mutates `GameState`, and rendering reads from that state.

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

# Static analysis diagnostics build lane.
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
- Close window: quit

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

## License

This project is licensed under the MIT License. Third-party libraries are licensed separately; see [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES.txt).
