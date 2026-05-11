param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",

    [switch]$Clean,

    [switch]$SkipConfigure,

    [switch]$NoRun,

    [string]$VcpkgRoot
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
. "$PSScriptRoot/common.ps1"

$repoRoot = Resolve-RepoRoot -WindowsToolsRoot $PSScriptRoot
$callerState = Save-CallerState
$exitCode = 0

function Fail([string]$Message) {
    throw $Message
}

function Find-ArkanoidExecutable([string]$BuildDir, [string]$Configuration) {
    $expectedPath = Join-Path (Join-Path $BuildDir $Configuration) "arkanoid.exe"
    if (Test-Path -LiteralPath $expectedPath -PathType Leaf) {
        return (Resolve-Path -LiteralPath $expectedPath).Path
    }

    $candidate = Get-ChildItem -LiteralPath $BuildDir -Recurse -File -Filter "arkanoid.exe" |
        Where-Object { $_.FullName -like "*\$Configuration\*" } |
        Sort-Object -Property LastWriteTime -Descending |
        Select-Object -First 1
    if ($candidate) {
        return $candidate.FullName
    }

    Fail "Could not locate $Configuration build output for arkanoid.exe under '$BuildDir'."
}

try {
    Normalize-WindowsProcessEnvironment

    $resolvedVcpkgRoot = Resolve-VcpkgRoot -RequestedRoot $VcpkgRoot -WriteStatus -PassThru
    $toolchainFile = Get-VcpkgToolchainFile -VcpkgRoot $resolvedVcpkgRoot

    Require-Command -Name "cmake" -CommandType Application | Out-Null
    Assert-RequiredRepoFiles -RepoRoot $repoRoot -RelativePaths @(
        "CMakeLists.txt",
        "CMakePresets.json",
        "vcpkg.json",
        "src/main.cpp"
    )
    Assert-ArkanoidPresetContract -RepoRoot $repoRoot -VcpkgRoot $resolvedVcpkgRoot | Out-Null

    if ($Clean -and $SkipConfigure) {
        Fail "-Clean cannot be combined with -SkipConfigure because the build directory is removed before configure."
    }

    Set-Location -LiteralPath $repoRoot

    $buildDir = Get-ConfiguredBuildDirectory -RepoRoot $repoRoot -ConfigurePresetName "windows-vcpkg"
    if ($Clean) {
        Remove-KnownDirectory -Path $buildDir -AllowedRoot $repoRoot -Label "build"
    } else {
        Assert-CachedBuildDirectoryMatches -BuildDir $buildDir -ToolchainFile $toolchainFile
    }

    if ($SkipConfigure) {
        Assert-SkipConfigureCacheExists -BuildDir $buildDir -PresetName "windows-vcpkg" -ScriptName "build.ps1"
        Write-Host "Skipping configure because -SkipConfigure was specified."
    } else {
        Invoke-NativeCommand -Command "cmake" -Arguments @("--preset", "windows-vcpkg")
    }

    $buildPreset = if ($Config -eq "Release") { "windows-release" } else { "windows-debug" }
    Invoke-NativeCommand -Command "cmake" -Arguments @(
        "--build", "--preset", $buildPreset,
        "--target", "arkanoid"
    )

    $exePath = Find-ArkanoidExecutable -BuildDir $buildDir -Configuration $Config
    if ($NoRun) {
        Write-Host "Run disabled; skipping game launch."
    } elseif ($Config -eq "Debug") {
        Write-Host "Launching game: $exePath"
        Start-Process -FilePath $exePath
    } else {
        Write-Host "Release build completed; skipping auto-launch."
    }
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    $exitCode = 1
} finally {
    Restore-CallerState -State $callerState
}

exit $exitCode
