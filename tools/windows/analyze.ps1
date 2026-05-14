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

function Test-FirstPartyDiagnosticPath([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) {
        return $false
    }

    try {
        $trimmedPath = $Path.Trim().Trim([char[]]@('"'))
        $diagnosticPath = Get-FullPath $trimmedPath
        $firstPartyRoots = @(
            (Join-Path $repoRoot "src")
            (Join-Path $repoRoot "include")
        )

        foreach ($root in $firstPartyRoots) {
            $rootPrefix = (Remove-TrailingPathSeparators -Path (Get-FullPath $root)) + [System.IO.Path]::DirectorySeparatorChar
            if ($diagnosticPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                return $true
            }
        }
    } catch {
        return $false
    }

    return $false
}

function Get-DiagnosticLines([object[]]$Lines) {
    $sourceDiagnosticPattern = '^\s*(?:\d+>)?(?<Path>.+?)\(\d+(?:,\d+)?\):\s+(?:warning|error)\s+C\d{4,5}\b'

    return @($Lines | ForEach-Object {
            $line = $_.ToString()
            if ($line -match $sourceDiagnosticPattern -and (Test-FirstPartyDiagnosticPath -Path $Matches.Path)) {
                $line
            }
        })
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
        $diagnosticFailureMessage = "Analyze completed with $($diagnostics.Count) compiler warning/static-analysis diagnostic line(s)."
        Write-AnalyzeSummary -Message $diagnosticFailureMessage -Diagnostics $diagnostics
        Fail $diagnosticFailureMessage
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
