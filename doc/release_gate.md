# Release Gate

Run the packaged Windows release gate:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\windows\release.ps1
```

Optional inputs:

- `-VcpkgRoot <path>` sets `VCPKG_ROOT`
- `-Clean` clears release-controlled outputs before starting a fresh run

Stages:

- validate Windows, vcpkg, `cmake`, `ctest`, required repo paths, CMake presets, cached toolchain, and x64 platform
- configure `windows-vcpkg`
- build Release `arkanoid`
- build Release `arkanoid_tests`
- run Release CTest with zero-test failure enabled
- install to an isolated per-run staging directory
- validate `bin\arkanoid.exe`, `bin\SDL3.dll`, `LICENSE`, and `THIRD_PARTY_LICENSES.txt`
- verify `bin\arkanoid.exe` uses the Windows GUI subsystem
- run hidden `--release-startup-smoke` from the installed package
- create `arkanoid-win64.zip` and compute SHA-256
- extract, validate, and smoke the zip from isolated ASCII and non-ASCII paths
- promote artifacts only after every gate passes

Artifacts:

- release root: `out\dist`
- `out\dist\install`
- `out\dist\arkanoid-win64.zip`
- `out\dist\release-status.json`
- `out\dist\release-latest.json`
- per-run staging and manifest under `out\dist\_runs\<run-id>`

`-Clean` removes only release-controlled outputs under `out\dist`: `_runs`, `install`, `arkanoid-win64.zip`, release status/latest JSON, and temporary promote paths. It does not clean the CMake build directory or unrelated `out` contents.

The release gate is fail-closed: any failed command, missing artifact, GUI-subsystem mismatch, smoke timeout, nonzero smoke exit, zip validation failure, or extraction-path failure stops before new artifacts are promoted.
