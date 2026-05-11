$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
. "$PSScriptRoot/common.ps1"

$repoRoot = Resolve-RepoRoot -WindowsToolsRoot $PSScriptRoot
$callerState = Save-CallerState
$exitCode = 0

function Get-RepoRelativePath([string]$Path, [string]$Root) {
    $rootFullPath = [System.IO.Path]::GetFullPath($Root).TrimEnd([char[]]@('\', '/'))
    $pathFullPath = [System.IO.Path]::GetFullPath($Path)
    $rootPrefix = "$rootFullPath$([System.IO.Path]::DirectorySeparatorChar)"

    if (-not $pathFullPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Stop-ToolScript "Refusing to format file outside repo root: $Path"
    }

    return $pathFullPath.Substring($rootPrefix.Length)
}

function Test-IsExcludedRepoFile([string]$Path, [string]$Root) {
    $relativePath = Get-RepoRelativePath -Path $Path -Root $Root
    $segments = @($relativePath -split '[\\/]+')
    if ($segments.Count -le 1) {
        return $false
    }

    $excludedSegments = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($segment in @(".git", ".vs", "generated", "out", "third_party", "external")) {
        [void]$excludedSegments.Add($segment)
    }

    foreach ($segment in $segments[0..($segments.Count - 2)]) {
        if ($excludedSegments.Contains($segment)) {
            return $true
        }

        if ($segment.StartsWith("build", [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }

        if ($segment.StartsWith("cmake-build-", [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    return $false
}

try {
    Normalize-WindowsProcessEnvironment

    $clangFormat = Get-Command clang-format -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $clangFormat) {
        Stop-ToolScript "clang-format was not found on PATH."
    }

    Set-Location -LiteralPath $repoRoot

    $sourceExtensions = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($extension in @(".cpp", ".hpp", ".h", ".cc", ".cxx", ".hh", ".hxx", ".ixx")) {
        [void]$sourceExtensions.Add($extension)
    }

    $files = @(Get-ChildItem -LiteralPath $repoRoot -Recurse -File |
        Where-Object {
            $sourceExtensions.Contains([System.IO.Path]::GetExtension($_.Name)) -and
            -not (Test-IsExcludedRepoFile -Path $_.FullName -Root $repoRoot)
        } |
        Sort-Object -Property FullName)

    foreach ($file in $files) {
        Write-Host "Formatting $($file.FullName)"
        & $clangFormat.Source -i $file.FullName
        $formatterExitCode = $LASTEXITCODE
        if ($formatterExitCode -ne 0) {
            Stop-ToolScript "clang-format failed for $($file.FullName) with exit code $formatterExitCode."
        }
    }

    Write-Host "Formatting complete."
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    $exitCode = 1
} finally {
    Restore-CallerState -State $callerState
}

exit $exitCode
