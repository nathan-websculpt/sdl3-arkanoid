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
    Require-Command -Name "ctest" -CommandType Application | Out-Null
    Assert-RequiredRepoFiles -RepoRoot $repoRoot -RelativePaths @(
        "CMakeLists.txt",
        "CMakePresets.json",
        "vcpkg.json",
        "tests/CMakeLists.txt"
    )
    Assert-ArkanoidPresetContract -RepoRoot $repoRoot -VcpkgRoot $resolvedVcpkgRoot | Out-Null

    if ($Clean -and $SkipConfigure) {
        Fail "-Clean cannot be combined with -SkipConfigure because the analyze build directory is removed before configure."
    }

    Set-Location -LiteralPath $repoRoot

    $analyzeBuildDir = Get-ConfiguredBuildDirectory -RepoRoot $repoRoot -ConfigurePresetName "windows-vcpkg-analyze"
    if ($Clean) {
        Remove-KnownDirectory -Path $analyzeBuildDir -AllowedRoot $repoRoot -Label "analyze build"
    } else {
        Assert-CachedBuildDirectoryMatches -BuildDir $analyzeBuildDir -ToolchainFile $toolchainFile
    }

    $diagnostics = @()
    if ($SkipConfigure) {
        Assert-BuildDirectoryConfigured -BuildDir $analyzeBuildDir
        Write-Host "Skipping configure because -SkipConfigure was specified."
    } else {
        $configureResult = Invoke-CapturedNativeCommand -Command "cmake" -Arguments @("--preset", "windows-vcpkg-analyze")
        if ($configureResult.ExitCode -ne 0) {
            Write-AnalyzeSummary -Message "Configure failed." -Diagnostics @()
            Fail "Configure failed with exit code $($configureResult.ExitCode)."
        }
    }

    foreach ($target in @("arkanoid", "arkanoid_tests")) {
        $buildResult = Invoke-CapturedNativeCommand -Command "cmake" -Arguments @(
            "--build", "--preset", "windows-vcpkg-debug-analyze",
            "--target", $target
        )
        $targetDiagnostics = @(Get-DiagnosticLines -Lines $buildResult.Lines)
        $diagnostics += $targetDiagnostics
        if ($buildResult.ExitCode -ne 0) {
            Write-AnalyzeSummary -Message "Build failed for target '$target'." -Diagnostics $diagnostics
            Fail "Analyze build failed with exit code $($buildResult.ExitCode)."
        }
    }

    Invoke-NativeCommand -Command "ctest" -Arguments @(
        "--preset", "windows-vcpkg-debug-analyze",
        "--output-on-failure",
        "--no-tests=error"
    )

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
