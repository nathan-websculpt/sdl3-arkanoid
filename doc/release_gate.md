# Release Gate

Run the packaged Windows release gate:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\windows\release.ps1
```

Optional inputs:

- `-VcpkgRoot <path>` sets `VCPKG_ROOT`
- `-Clean` clears prior per-run release staging

Stages:

- validate Windows, vcpkg, `cmake`, `ctest`, required repo paths, and cached toolchain
- configure `windows-vcpkg`
- build release app and release tests
- run release CTest
- install to a staging directory
- validate `bin\arkanoid.exe` and `bin\SDL3.dll`
- run hidden `--release-startup-smoke`
- zip, extract, validate, and smoke the extracted package
- promote artifacts

Artifacts:

- `dist\install`
- `dist\arkanoid-win64.zip`
- per-run staging under `dist\_runs\<timestamp-pid>`

The release gate is fail-closed: any failed command, missing artifact, smoke timeout, or nonzero smoke exit throws before new artifacts are promoted.
