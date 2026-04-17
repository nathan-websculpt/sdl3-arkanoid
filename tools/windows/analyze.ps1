param(
    [switch]$Clean,
    [string]$VcpkgRoot
)

$ErrorActionPreference = "Stop"

function Fail([string]$Message) {
    throw $Message
}

if ($VcpkgRoot) {
    $env:VCPKG_ROOT = $VcpkgRoot
}

if (-not $env:VCPKG_ROOT) {
    Fail "VCPKG_ROOT is not set. Pass -VcpkgRoot."
}

$toolchainFile = Join-Path $env:VCPKG_ROOT "scripts/buildsystems/vcpkg.cmake"
if (-not (Test-Path $toolchainFile)) {
    Fail "Invalid VCPKG_ROOT: $toolchainFile not found."
}

$buildDir = "build-win-vcpkg-analyze"

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory..."
    Remove-Item -Recurse -Force $buildDir
}

Write-Host "Configuring analyze preset..."
& cmake --preset windows-vcpkg-analyze
if ($LASTEXITCODE -ne 0) { Fail "Configure failed." }

Write-Host "Building analyze preset..."
& cmake --build --preset windows-vcpkg-debug-analyze
if ($LASTEXITCODE -ne 0) { Fail "Build failed." }

Write-Host "Running analyze tests..."
& ctest --preset windows-vcpkg-debug-analyze
if ($LASTEXITCODE -ne 0) { Fail "Tests failed." }
