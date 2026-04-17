# Arkanoid-Style Tiny Game

## Why This Project Exists

Many small game projects over-engineer simple problems using:

- deep inheritance trees
- generic entity systems
- unnecessary polymorphism

This project takes the opposite approach:

- match architecture to problem size
- avoid abstraction without justification
- prioritize clarity, safety, and determinism

---

## Constraints (Intentional)

- no textures/sprites
- no fonts (SDL_ttf)
- no menus or UI systems
- no score HUD
- no powerups or special bricks

---

## Running

One-line dev loop
```powershell
# set once per shell (or pass -VcpkgRoot directly to the script)
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

# debug build (auto-launches game after successful debug build under windows-vcpkg-debug)
powershell -ExecutionPolicy Bypass -File .\tools\windows\dev_build.ps1

# debug build without auto-launch
powershell -ExecutionPolicy Bypass -File .\tools\windows\dev_build.ps1 -NoRun

# run tests (debug preset only)
powershell -ExecutionPolicy Bypass -File .\tools\windows\dev_build.ps1 -RunTests

# release build (build preset windows-vcpkg-release; target is arkanoid)
powershell -ExecutionPolicy Bypass -File .\tools\windows\dev_build.ps1 -Preset windows-vcpkg-release
```

## Static analysis

One-line
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\windows\analyze.ps1
```

Or
```powershell
cmake --preset windows-vcpkg-analyze
cmake --build --preset windows-vcpkg-debug-analyze
ctest --preset windows-vcpkg-debug-analyze

# analyze.ps1 runs configure + build + ctest on analyze presets
powershell -ExecutionPolicy Bypass -File .\tools\windows\analyze.ps1
powershell -ExecutionPolicy Bypass -File .\tools\windows\analyze.ps1 -Clean
```

Set-ExecutionPolicy

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

---

## Open in Visual Studio

1. Install Visual Studio with the **Desktop development with C++** workload.
2. Ensure `VCPKG_ROOT` is set to your vcpkg installation path.
3. Open the repository root folder in Visual Studio (`File > Open > Folder`).
4. Select a build preset from the toolbar:
   - `windows-debug` (for development)
   - `windows-release` (for optimized builds)
5. If prompted about a toolchain/cache mismatch, click **Delete and regenerate cache**.
6. Build and run the executable target from the toolbar.
