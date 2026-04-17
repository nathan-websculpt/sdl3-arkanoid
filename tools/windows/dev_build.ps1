param(
    [ValidateSet("windows-vcpkg-debug", "windows-vcpkg-release")]
    [string]$Preset = "windows-vcpkg-debug",
    [switch]$NoRun,
    [switch]$RunTests,
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

$buildDir = "build-win-vcpkg"

function Needs-Configure {
    $cacheFile = Join-Path $buildDir "CMakeCache.txt"

    if (!(Test-Path $buildDir)) { return $true }
    if (!(Test-Path $cacheFile)) { return $true }

    $cacheTime = (Get-Item $cacheFile).LastWriteTime

    $toolchainLine = Get-Content -Path $cacheFile | Select-String -Pattern '^CMAKE_TOOLCHAIN_FILE:[^=]*=' | Select-Object -First 1
    if (-not $toolchainLine) { return $true }

    try {
        $cachedToolchainFile = ($toolchainLine.Line -replace '^[^=]*=', '').Trim('"')
        $normalizedCachedToolchainFile = [System.IO.Path]::GetFullPath($cachedToolchainFile.Replace('/', '\'))
        $normalizedExpectedToolchainFile = [System.IO.Path]::GetFullPath($toolchainFile.Replace('/', '\'))
    } catch {
        return $true
    }

    if ($normalizedCachedToolchainFile -ne $normalizedExpectedToolchainFile) {
        return $true
    }

    $watchFiles = @(
        "CMakeLists.txt",
        "CMakePresets.json",
        "vcpkg.json",
        "tests/CMakeLists.txt"
    )

    foreach ($file in $watchFiles) {
        if (Test-Path $file) {
            if ((Get-Item $file).LastWriteTime -gt $cacheTime) {
                return $true
            }
        }
    }

    return $false
}

Write-Host "Using preset: $Preset"

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory..."
    Remove-Item -Recurse -Force $buildDir
}

if (Needs-Configure) {
    Write-Host "Configuring..."
    & cmake --preset windows-vcpkg
    if ($LASTEXITCODE -ne 0) { Fail "Configure failed." }
} else {
    Write-Host "Configure step up-to-date."
}

Write-Host "Building..."
& cmake --build --preset $Preset
if ($LASTEXITCODE -ne 0) { Fail "Build failed." }

if (-not $NoRun -and $Preset -eq "windows-vcpkg-debug") {
    $debugDir = Join-Path $buildDir "Debug"
    $gameExePath = Join-Path $debugDir "arkanoid.exe"

    if (-not (Test-Path $gameExePath)) {
        $gameExe = Get-ChildItem -Path $buildDir -Recurse -File -Filter "arkanoid.exe" |
            Where-Object { $_.FullName -match "\\Debug\\" } |
            Sort-Object -Property LastWriteTime -Descending |
            Select-Object -First 1

        if (-not $gameExe) {
            Fail "Could not locate Debug build output for arkanoid.exe under '$buildDir'."
        }

        $gameExePath = $gameExe.FullName
    }

    Write-Host "Launching game: $gameExePath"
    Start-Process -FilePath $gameExePath
} elseif ($NoRun) {
    Write-Host "Run disabled; skipping game launch."
}

if ($RunTests) {
    if ($Preset -ne "windows-vcpkg-debug") {
        Fail "Tests only run under debug preset."
    }

    Write-Host "Running tests..."
    & ctest --preset $Preset
    if ($LASTEXITCODE -ne 0) { Fail "Tests failed." }
}
