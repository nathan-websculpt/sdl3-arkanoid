param(
    [switch]$Clean,
    [string]$VcpkgRoot
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$script:kSmokeTimeoutMilliseconds = 10000

function Fail([string]$Message) {
    throw $Message
}

function FullPath([string]$Path) {
    return [System.IO.Path]::GetFullPath($Path)
}

function FullPathForComparison([string]$Path, [string]$BaseDirectory) {
    try {
        if ([System.IO.Path]::IsPathRooted($Path)) {
            return FullPath $Path
        }

        return FullPath (Join-Path $BaseDirectory $Path)
    } catch {
        return $Path
    }
}

function Assert-PathUnderDirectory([string]$Path, [string]$Directory) {
    $fullPath = FullPath $Path
    $fullDirectory = FullPath $Directory

    $directoryPrefix = $fullDirectory
    $separator = [System.IO.Path]::DirectorySeparatorChar.ToString()
    if (-not $directoryPrefix.EndsWith($separator, [System.StringComparison]::Ordinal)) {
        $directoryPrefix = "$directoryPrefix$separator"
    }

    if (-not $fullPath.StartsWith($directoryPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Fail "Refusing to operate outside expected directory: $fullPath"
    }
}

function Remove-DirectoryIfExists([string]$Path, [string]$AllowedRoot) {
    Assert-PathUnderDirectory $Path $AllowedRoot
    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force
    }
}

function Remove-FileIfExists([string]$Path, [string]$AllowedRoot) {
    Assert-PathUnderDirectory $Path $AllowedRoot
    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Force
    }
}

function New-CleanDirectory([string]$Path, [string]$AllowedRoot) {
    Remove-DirectoryIfExists $Path $AllowedRoot
    New-Item -ItemType Directory -Path $Path -Force | Out-Null
}

function Move-DirectoryUnderRoot([string]$Source, [string]$Destination, [string]$AllowedRoot) {
    Assert-PathUnderDirectory $Source $AllowedRoot
    Assert-PathUnderDirectory $Destination $AllowedRoot
    Move-Item -LiteralPath $Source -Destination $Destination
}

function Move-FileUnderRoot([string]$Source, [string]$Destination, [string]$AllowedRoot) {
    Assert-PathUnderDirectory $Source $AllowedRoot
    Assert-PathUnderDirectory $Destination $AllowedRoot
    Move-Item -LiteralPath $Source -Destination $Destination
}

function Invoke-External([string]$FilePath, [string[]]$Arguments) {
    Write-Host "> $FilePath $($Arguments -join ' ')"
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        Fail "$FilePath failed with exit code $LASTEXITCODE."
    }
}

function Get-CachedToolchainFile([string]$CachePath) {
    $entry = Get-Content -LiteralPath $CachePath | Where-Object { $_ -match "^CMAKE_TOOLCHAIN_FILE(:[^=]*)?=" } | Select-Object -First 1
    if ($null -eq $entry) {
        return $null
    }

    $separatorIndex = $entry.IndexOf("=")
    if ($separatorIndex -lt 0) {
        return $null
    }

    return $entry.Substring($separatorIndex + 1)
}

function Assert-CachedToolchainFileMatches([string]$BuildDir, [string]$ToolchainFile) {
    $cachePath = Join-Path $BuildDir "CMakeCache.txt"
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        return
    }

    $cachedToolchainFile = Get-CachedToolchainFile $cachePath
    if ($null -eq $cachedToolchainFile) {
        Fail "Existing build directory is not a valid vcpkg-configured release build directory: $BuildDir. CMakeCache.txt does not contain CMAKE_TOOLCHAIN_FILE; clean/reconfigure the build directory."
    }

    if ($cachedToolchainFile.Length -eq 0) {
        Fail "Existing build directory is not a valid vcpkg-configured release build directory: $BuildDir. CMakeCache.txt contains an empty CMAKE_TOOLCHAIN_FILE; clean/reconfigure the build directory."
    }

    $cacheDir = Split-Path -Parent $cachePath
    $cachedFullPath = FullPathForComparison $cachedToolchainFile $cacheDir
    $requestedFullPath = FullPathForComparison $ToolchainFile (Get-Location).Path

    if (-not $cachedFullPath.Equals($requestedFullPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        Fail "Existing build directory was configured with a different toolchain and should be cleaned/reconfigured: $BuildDir. Cached CMAKE_TOOLCHAIN_FILE: $cachedFullPath. Requested CMAKE_TOOLCHAIN_FILE: $requestedFullPath."
    }
}

function Validate-InstallTree([string]$InstallRoot) {
    $binDir = Join-Path $InstallRoot "bin"
    $gameExePath = Join-Path $binDir "arkanoid.exe"
    $sdlDllPath = Join-Path $binDir "SDL3.dll"

    if (-not (Test-Path -LiteralPath $gameExePath -PathType Leaf)) {
        Fail "Missing required artifact: $gameExePath"
    }

    if (-not (Test-Path -LiteralPath $sdlDllPath -PathType Leaf)) {
        Fail "Missing required artifact: $sdlDllPath"
    }

    return @{
        BinDir = $binDir
        GameExePath = $gameExePath
    }
}

function Invoke-StartupSmoke([string]$GameExePath, [string]$WorkingDirectory) {
    Write-Host "> $GameExePath --release-startup-smoke"

    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $GameExePath
    $startInfo.Arguments = "--release-startup-smoke"
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false

    $process = [System.Diagnostics.Process]::Start($startInfo)
    if ($null -eq $process) {
        Fail "Failed to start release startup smoke test."
    }

    try {
        if (-not $process.WaitForExit($script:kSmokeTimeoutMilliseconds)) {
            $process.Kill()
            Fail "Release startup smoke test timed out."
        }

        if ($process.ExitCode -ne 0) {
            Fail "Release startup smoke test failed with exit code $($process.ExitCode)."
        }
    } finally {
        $process.Dispose()
    }
}

if ([System.Environment]::OSVersion.Platform -ne [System.PlatformID]::Win32NT) {
    Fail "This release gate is Windows-only."
}

if ($VcpkgRoot) {
    $env:VCPKG_ROOT = $VcpkgRoot
}

if (-not $env:VCPKG_ROOT) {
    Fail "VCPKG_ROOT is not set. Pass -VcpkgRoot."
}

$toolchainFile = Join-Path $env:VCPKG_ROOT "scripts/buildsystems/vcpkg.cmake"
if (-not (Test-Path -LiteralPath $toolchainFile -PathType Leaf)) {
    Fail "Invalid VCPKG_ROOT: $toolchainFile not found."
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Fail "cmake was not found on PATH."
}

if (-not (Get-Command ctest -ErrorAction SilentlyContinue)) {
    Fail "ctest was not found on PATH."
}

$repoRoot = FullPath (Join-Path $PSScriptRoot "../..")
$requiredRepoPaths = @(
    "CMakeLists.txt",
    "CMakePresets.json",
    "vcpkg.json",
    "src/main.cpp"
)

foreach ($requiredPath in $requiredRepoPaths) {
    $fullRequiredPath = Join-Path $repoRoot $requiredPath
    if (-not (Test-Path -LiteralPath $fullRequiredPath)) {
        Fail "Required repo path not found: $fullRequiredPath"
    }
}

$buildDir = Join-Path $repoRoot "build-win-vcpkg"
$distDir = Join-Path $repoRoot "dist"
$runsDir = Join-Path $distDir "_runs"
$runId = "{0}-{1}" -f (Get-Date -Format "yyyyMMdd-HHmmss"), $PID
$runRoot = Join-Path $runsDir $runId
$stageDir = Join-Path $runRoot "stage"
$extractDir = Join-Path $runRoot "extract"
$zipPath = Join-Path $runRoot "arkanoid-win64.zip"
$promotedInstallDir = Join-Path $distDir "install"
$promotedZipPath = Join-Path $distDir "arkanoid-win64.zip"
$tempPromotedInstallDir = Join-Path $distDir "_install-promote-$runId"
$tempPromotedZipPath = Join-Path $distDir "_arkanoid-win64-$runId.zip"

New-Item -ItemType Directory -Path $distDir -Force | Out-Null
New-Item -ItemType Directory -Path $runsDir -Force | Out-Null

if ($Clean) {
    Remove-DirectoryIfExists $runsDir $distDir
    New-Item -ItemType Directory -Path $runsDir -Force | Out-Null
}

Assert-CachedToolchainFileMatches $buildDir $toolchainFile

New-CleanDirectory $runRoot $runsDir
New-Item -ItemType Directory -Path $stageDir -Force | Out-Null
New-Item -ItemType Directory -Path $extractDir -Force | Out-Null

Push-Location $repoRoot
try {
    Invoke-External "cmake" @("--preset", "windows-vcpkg")
    Invoke-External "cmake" @("--build", "--preset", "windows-vcpkg-release")
    Invoke-External "cmake" @("--build", "--preset", "windows-vcpkg-release-tests")
    Invoke-External "ctest" @("--preset", "windows-vcpkg-release")

    Invoke-External "cmake" @("--install", $buildDir, "--config", "Release", "--prefix", $stageDir)

    $stageValidation = Validate-InstallTree $stageDir
    Invoke-StartupSmoke $stageValidation.GameExePath $stageValidation.BinDir

    $stageItems = @(Get-ChildItem -LiteralPath $stageDir -Force)
    if ($stageItems.Count -eq 0) {
        Fail "Install stage is empty: $stageDir"
    }
    Compress-Archive -LiteralPath $stageItems.FullName -DestinationPath $zipPath -Force

    Expand-Archive -LiteralPath $zipPath -DestinationPath $extractDir -Force

    $extractValidation = Validate-InstallTree $extractDir
    Invoke-StartupSmoke $extractValidation.GameExePath $extractValidation.BinDir

    New-CleanDirectory $tempPromotedInstallDir $distDir
    foreach ($stageItem in $stageItems) {
        Copy-Item -LiteralPath $stageItem.FullName -Destination $tempPromotedInstallDir -Recurse -Force
    }
    Copy-Item -LiteralPath $zipPath -Destination $tempPromotedZipPath -Force

    Remove-DirectoryIfExists $promotedInstallDir $distDir
    Move-DirectoryUnderRoot $tempPromotedInstallDir $promotedInstallDir $distDir

    Remove-FileIfExists $promotedZipPath $distDir
    Move-FileUnderRoot $tempPromotedZipPath $promotedZipPath $distDir

    Write-Host "Release artifacts promoted:"
    Write-Host "  $promotedInstallDir"
    Write-Host "  $promotedZipPath"
} finally {
    Pop-Location
}
