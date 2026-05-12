param(
    [switch]$Clean,

    [switch]$SkipConfigure,

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

function Get-DiagnosticLines([object[]]$Lines) {
    $diagnosticPattern = 'warning C\d{4}|error C\d{4}|C264\d{2}|C268\d{2}|C6\d{3}'
    return @($Lines | ForEach-Object { $_.ToString() } | Where-Object { $_ -match $diagnosticPattern })
}

function Write-AnalyzeSummary([string]$Message, [object[]]$Diagnostics) {
    $matchedDiagnosticCount = 0
    if ($null -ne $Diagnostics) {
        $matchedDiagnosticCount = $Diagnostics.Count
    }

    Write-Host ""
    Write-Host "Analyze summary"
    Write-Host "---------------"
    Write-Host $Message

    if ($matchedDiagnosticCount -gt 0) {
        Write-Host "Matched diagnostic lines: $matchedDiagnosticCount"
    }
}

try {
    Normalize-WindowsProcessEnvironment

    $resolvedVcpkgRoot = Resolve-VcpkgRoot -RequestedRoot $VcpkgRoot -WriteStatus -PassThru
    $toolchainFile = Get-VcpkgToolchainFile -VcpkgRoot $resolvedVcpkgRoot

    Require-Command -Name "cmake" -CommandType Application | Out-Null
    Assert-RequiredRepoFiles -RepoRoot $repoRoot -RelativePaths @(
        "CMakeLists.txt",
        "CMakePresets.json",
        "vcpkg.json"
    )
    Assert-ArkanoidPresetContract -RepoRoot $repoRoot -VcpkgRoot $resolvedVcpkgRoot | Out-Null

    Assert-CleanSkipConfigureCompatible -Clean:$Clean -SkipConfigure:$SkipConfigure -Label "analyze build directory"

    Set-Location -LiteralPath $repoRoot

    $analyzeBuildDir = Get-WindowsAnalyzeBuildDir -RepoRoot $repoRoot
    if ($Clean) {
        Clear-KnownBuildDirectory -BuildDir $analyzeBuildDir -RepoRoot $repoRoot -Label "analyze build"
    } else {
        Assert-CachedBuildDirectoryMatches -BuildDir $analyzeBuildDir -ToolchainFile $toolchainFile
    }

    if ($SkipConfigure) {
        Assert-SkipConfigureCacheExists -BuildDir $analyzeBuildDir -PresetName "windows-vcpkg-analyze" -ScriptName "analyze.ps1"
        Invoke-CMakeConfigureUnlessSkipped -SkipConfigure:$SkipConfigure -Preset "windows-vcpkg-analyze" -CaptureOutput | Out-Null
    } else {
        $configureResult = Invoke-CMakeConfigureUnlessSkipped -SkipConfigure:$SkipConfigure -Preset "windows-vcpkg-analyze" -CaptureOutput
        if ($configureResult.ExitCode -ne 0) {
            Write-AnalyzeSummary -Message "Configure failed." -Diagnostics @()
            Fail "Configure failed with exit code $($configureResult.ExitCode)."
        }
    }

    $buildResult = Invoke-CMakeBuildPreset -Preset "windows-debug-analyze" -Target "arkanoid" -CaptureOutput
    $diagnostics = @(Get-DiagnosticLines -Lines $buildResult.Lines)
    if ($buildResult.ExitCode -ne 0) {
        Write-AnalyzeSummary -Message "Build failed." -Diagnostics $diagnostics
        Fail "Analyze build failed with exit code $($buildResult.ExitCode)."
    }

    if ($diagnostics.Count -gt 0) {
        Write-AnalyzeSummary -Message "Analyze completed, but warnings or static-analysis diagnostics were found. Review the output above." -Diagnostics $diagnostics
    } else {
        Write-AnalyzeSummary -Message "Analyze completed successfully with no detected compiler warnings or static-analysis diagnostics." -Diagnostics @()
    }
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    $exitCode = 1
} finally {
    Restore-CallerState -State $callerState
}

exit $exitCode
