# Release Gate

Run the packaged Windows release gate:

```powershell
.\tools\windows\release.ps1
```

If PowerShell blocks script execution, use the one-off fallback noted in the README:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

Optional inputs:

- `-Clean` clears the Release CMake output and release-controlled package outputs before starting a fresh run
- `-BuildIdentity <text>` records caller-provided metadata in release status/latest JSON; the default is `local`
- `-VcpkgRoot <path>` sets `VCPKG_ROOT`

Stages:

- validate Windows, vcpkg, `cmake`, `ctest`, required repo paths, CMake presets, cached toolchain when reusing a build directory, and x64 platform
- configure `windows-vcpkg`
- build Release `arkanoid`
- build Release `arkanoid_tests` with the `windows-release` build preset
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

`-Clean` removes the configured Release CMake build directory `out\build-win-vcpkg`, then removes release-controlled outputs under `out\dist`: `_runs`, `install`, `arkanoid-win64.zip`, release status/latest JSON, and temporary promote paths. It does not delete all of `out\dist`, and it does not clean `out\build-win-vcpkg-analyze` or unrelated `out` contents.

`-BuildIdentity` defaults to `local`. It is written as `buildIdentity` in the per-run manifest, `release-status.json`, and `release-latest.json`. It does not change the generated run ID, staging paths, zip name, or promoted artifact paths.

The release gate is fail-closed: any failed command, missing artifact, GUI-subsystem mismatch, smoke timeout, nonzero smoke exit, zip validation failure, or extraction-path failure stops before new artifacts are promoted.
