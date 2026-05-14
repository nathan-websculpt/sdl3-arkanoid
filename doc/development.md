# Development And Validation

This repo's canonical Windows workflow is preset-first through vcpkg manifest mode. Use the PowerShell helpers in `tools/windows/` for day-to-day work; they validate the Windows preset contract, run from the repo root, and restore the caller's working directory and previous `VCPKG_ROOT` when they exit.

## Requirements

- Windows PowerShell
- Visual Studio with MSVC; presets use generator `Visual Studio 18 2026`
- CMake 4.2 or newer
- vcpkg, set with `VCPKG_ROOT` or passed as `-VcpkgRoot "C:\path\to\vcpkg"`

If PowerShell blocks scripts in a local shell:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

## Presets And Output

The main configure preset is `windows-vcpkg`. It writes to `out/build-win-vcpkg`, uses x64 architecture, `VCPKG_TARGET_TRIPLET=x64-windows`, and the preset-level toolchain file `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`.

Build presets describe the configuration lane:

- `windows-debug`: Debug build lane
- `windows-release`: Release build lane
- `windows-debug-analyze`: Debug analysis lane under `out/build-win-vcpkg-analyze`

CTest presets run the matching normal build tree and fail on zero discovered tests:

- `windows-debug-tests`
- `windows-release-tests`

There is no Release-specific test build preset. Scripts build `arkanoid_tests` with `windows-release` and then run the `windows-release-tests` CTest preset.

Generated Windows outputs:

- `out/build-win-vcpkg`: normal Debug/Release CMake tree
- `out/build-win-vcpkg-analyze`: analysis CMake tree
- `out/dist`: release output root
- `out/dist/_runs/<run-id>`: isolated release staging, extraction, manifest, and run zip
- `out/dist/install`: promoted install tree
- `out/dist/arkanoid-win64.zip`: promoted portable zip
- `out/dist/release-status.json` and `out/dist/release-latest.json`: release metadata

## Build

```powershell
.\tools\windows\build.ps1
.\tools\windows\build.ps1 -Config Debug -NoRun
.\tools\windows\build.ps1 -Config Release
.\tools\windows\build.ps1 -Clean -Config Debug -NoRun
.\tools\windows\build.ps1 -SkipConfigure -Config Debug -NoRun
.\tools\windows\build.ps1 -VcpkgRoot "C:\path\to\vcpkg" -Config Release
```

`build.ps1` defaults to `-Config Debug`. Debug builds auto-launch `arkanoid.exe` after a successful build unless `-NoRun` is supplied. The Debug launch invokes the executable directly and propagates a nonzero game exit code. Release builds never auto-launch. `-Clean` removes `out/build-win-vcpkg` before configure/build. `-SkipConfigure` reuses an existing `out/build-win-vcpkg/CMakeCache.txt`. `-Clean` and `-SkipConfigure` cannot be combined.

Manual equivalent:

```powershell
cmake --preset windows-vcpkg
cmake --build --preset windows-debug --target arkanoid
.\out\build-win-vcpkg\Debug\arkanoid.exe
```

## Tests

```powershell
.\tools\windows\test.ps1
.\tools\windows\test.ps1 -Config Release
.\tools\windows\test.ps1 -TestFilter Brick
.\tools\windows\test.ps1 -Clean -Config Debug
.\tools\windows\test.ps1 -SkipConfigure -Config Debug
```

`test.ps1` defaults to Debug and supports `-Config Debug|Release`, `-Clean`, `-SkipConfigure`, `-TestFilter <regex>`, and `-VcpkgRoot <path>`. It configures unless skipped, builds `arkanoid_tests` with the matching build preset, then runs CTest with `--output-on-failure` and `--no-tests=error`. `-TestFilter` maps to CTest `-R`.

Manual equivalent:

```powershell
cmake --preset windows-vcpkg
cmake --build --preset windows-debug --target arkanoid_tests
ctest --preset windows-debug-tests --output-on-failure --no-tests=error
```

## Static Analysis

```powershell
.\tools\windows\analyze.ps1
.\tools\windows\analyze.ps1 -Clean
.\tools\windows\analyze.ps1 -SkipConfigure
.\tools\windows\analyze.ps1 -VcpkgRoot "C:\path\to\vcpkg"
```

`analyze.ps1` uses `windows-vcpkg-analyze`, writes to `out/build-win-vcpkg-analyze`, configures `/analyze` for selected targets, and builds the production `arkanoid` target. It exits nonzero on configure/build failure and on first-party compiler warnings or MSVC `/analyze` diagnostics reported under `src/` or `include/`. It is a diagnostics build lane, not a test runner. Use `test.ps1` for unit tests. Full local safety validation runs `analyze.ps1` first, then the packaged `release.ps1` gate. `analyze.ps1` supports `-Clean`, `-SkipConfigure`, and `-VcpkgRoot`; `-Clean` and `-SkipConfigure` cannot be combined.

## Release Gate

```powershell
.\tools\windows\release.ps1
.\tools\windows\release.ps1 -Clean
.\tools\windows\release.ps1 -BuildIdentity "ci-20260512.1"
.\tools\windows\release.ps1 -VcpkgRoot "C:\path\to\vcpkg"
```

`release.ps1` validates the Windows host and preset contract, configures `windows-vcpkg`, builds Release `arkanoid`, builds Release `arkanoid_tests` with `windows-release`, runs `windows-release-tests`, installs to `out/dist/_runs/<run-id>/stage`, validates and smokes the package, creates `arkanoid-win64.zip`, extracts and validates it from ASCII and non-ASCII paths, then promotes canonical outputs under `out/dist`. It does not run the analyzer lane; run `analyze.ps1` first for full local safety validation.

`-BuildIdentity` defaults to `local`. It is metadata only, recorded as `buildIdentity` in the run manifest and release JSON; it does not affect run IDs, staging paths, zip names, or promoted paths.

`-Clean` removes the shared normal Windows CMake build tree `out/build-win-vcpkg` and only release-controlled outputs under `out/dist`: `_runs`, `install`, `arkanoid-win64.zip`, `release-status.json`, `release-latest.json`, and temporary promotion files/directories. It does not delete unrelated `out` contents or the analyze build tree.

See [Release Gate](release_gate.md) for the validation details.

## Formatting

```powershell
.\tools\windows\format.ps1
```

`format.ps1` is an optional local helper, not part of the core build/test/analyze/release contract. It runs `clang-format -i` on project-owned C/C++ source and header-like files while excluding generated output, third-party, and external directories.

## Visual Studio

1. Install Visual Studio with the Desktop development with C++ workload.
2. Ensure `VCPKG_ROOT` is set.
3. Open the repository root folder.
4. Select configure preset `windows-vcpkg`.
5. Build `arkanoid` or `arkanoid_tests` with `windows-debug` or `windows-release`.
6. If prompted about a toolchain/cache/platform mismatch, delete/regenerate cache or rerun the relevant script with `-Clean`.
