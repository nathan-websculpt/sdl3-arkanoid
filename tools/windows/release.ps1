param(
    [switch]$Clean,

    [string]$BuildIdentity = "local",

    [string]$VcpkgRoot
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
. "$PSScriptRoot/common.ps1"

$repoRoot = Resolve-RepoRoot -WindowsToolsRoot $PSScriptRoot
$callerState = Save-CallerState
$exitCode = 0

$distRoot = Get-WindowsDistRoot -RepoRoot $repoRoot
$runsDir = Join-Path $distRoot "_runs"
$canonicalInstallDir = Join-Path $distRoot "install"
$canonicalZipPath = Join-Path $distRoot "arkanoid-win64.zip"
$latestPath = Join-Path $distRoot "release-latest.json"
$statusPath = Join-Path $distRoot "release-status.json"

$commandResults = New-Object System.Collections.Generic.List[object]
$gateResults = New-Object System.Collections.Generic.List[object]
$smokeResults = New-Object System.Collections.Generic.List[object]
$validatedFiles = New-Object System.Collections.Generic.List[object]
$script:Manifest = $null
$script:RunManifestPath = $null

function Fail([string]$Message) {
    throw $Message
}

function Add-GateResult([string]$Name, [string]$Status, [object]$Details) {
    $gateResults.Add([ordered]@{
        name = $Name
        status = $Status
        details = $Details
        completedAtUtc = Get-UtcText (Get-Date)
    })
    Save-Manifest
}

function Save-Manifest {
    if ($script:Manifest -and $script:RunManifestPath) {
        Write-JsonFile -Path $script:RunManifestPath -Value $script:Manifest
    }
}

function Write-ReleaseStatus([string]$GateStatus, [string]$Message) {
    if (-not $script:Manifest) {
        return
    }

    $statusObject = [ordered]@{
        status = $GateStatus
        runId = $script:Manifest.runId
        buildIdentity = $script:Manifest.buildIdentity
        message = $Message
        startedAtUtc = $script:Manifest.startedAtUtc
        completedAtUtc = $script:Manifest.completedAtUtc
        runRoot = $script:Manifest.runRoot
        runManifestPath = $script:RunManifestPath
        canonicalInstallPath = $canonicalInstallDir
        canonicalZipPath = $canonicalZipPath
        zipSha256 = $script:Manifest.zipSha256
    }
    Write-JsonFile -Path $statusPath -Value $statusObject
}

function Get-GitValue([string[]]$Arguments) {
    $git = Get-Command git -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $git) {
        return $null
    }

    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & $git.Source @Arguments 2>$null
        if ($LASTEXITCODE -ne 0) {
            return $null
        }

        return ($output | Select-Object -First 1)
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
}

function Invoke-GateCommand([string]$Name, [string]$Command, [string[]]$Arguments) {
    $started = Get-Date
    Write-Host ""
    Write-Host "[$Name] $Command $($Arguments -join ' ')"
    & $Command @Arguments
    $commandExitCode = $LASTEXITCODE
    if ($null -eq $commandExitCode) {
        $commandExitCode = 0
    }

    $completed = Get-Date
    $result = [ordered]@{
        name = $Name
        command = $Command
        arguments = $Arguments
        exitCode = $commandExitCode
        startedAtUtc = Get-UtcText $started
        completedAtUtc = Get-UtcText $completed
    }
    $commandResults.Add($result)
    Save-Manifest

    if ($commandExitCode -ne 0) {
        Fail "$Name failed with exit code $commandExitCode."
    }

    return $result
}

function Remove-ReleaseControlledOutputs {
    if (-not (Test-Path -LiteralPath $distRoot -PathType Container)) {
        return
    }

    Remove-KnownDirectory -Path $runsDir -AllowedRoot $distRoot -Label "release runs"
    Remove-KnownDirectory -Path $canonicalInstallDir -AllowedRoot $distRoot -Label "canonical install"
    Remove-KnownFile -Path $canonicalZipPath -AllowedRoot $distRoot -Label "canonical zip"
    Remove-KnownFile -Path $latestPath -AllowedRoot $distRoot -Label "latest release JSON"
    Remove-KnownFile -Path $statusPath -AllowedRoot $distRoot -Label "release status JSON"

    $temporaryInstallDirs = @(Get-ChildItem -LiteralPath $distRoot -Directory -Filter "_install-promote-*" -ErrorAction SilentlyContinue)
    foreach ($directory in $temporaryInstallDirs) {
        Remove-KnownDirectory -Path $directory.FullName -AllowedRoot $distRoot -Label "temporary promoted install"
    }

    $temporaryZipFiles = @(Get-ChildItem -LiteralPath $distRoot -File -Filter "_arkanoid-win64-*.zip" -ErrorAction SilentlyContinue)
    foreach ($file in $temporaryZipFiles) {
        Remove-KnownFile -Path $file.FullName -AllowedRoot $distRoot -Label "temporary promoted zip"
    }
}

function Assert-PackageSnapshotMatches([object[]]$Expected, [object[]]$Actual, [string]$Label) {
    $expectedJson = ConvertTo-Json -InputObject $Expected -Depth 5 -Compress
    $actualJson = ConvertTo-Json -InputObject $Actual -Depth 5 -Compress
    if ($expectedJson -ne $actualJson) {
        Fail "$Label package contents do not match the installed package snapshot."
    }

    Add-GateResult -Name "$Label-snapshot-match" -Status "passed" -Details ([ordered]@{
        fileCount = $Expected.Count
    })
}

function Assert-PackageUnchanged([object[]]$Before, [object[]]$After, [string]$Label) {
    $beforeJson = ConvertTo-Json -InputObject $Before -Depth 5 -Compress
    $afterJson = ConvertTo-Json -InputObject $After -Depth 5 -Compress
    if ($beforeJson -ne $afterJson) {
        Fail "$Label package changed during startup smoke."
    }

    Add-GateResult -Name "$Label-smoke-mutation-check" -Status "passed" -Details ([ordered]@{
        fileCount = $Before.Count
    })
}

function Validate-PackageTree([string]$Root, [string]$Label) {
    if (-not (Test-Path -LiteralPath $Root -PathType Container)) {
        Fail "$Label package root does not exist: $Root"
    }

    $requiredFiles = @(
        "bin/arkanoid.exe",
        "bin/SDL3.dll",
        "LICENSE",
        "THIRD_PARTY_LICENSES.txt"
    )

    $validated = New-Object System.Collections.Generic.List[object]
    foreach ($relativeFile in $requiredFiles) {
        $packagePath = Join-Path $Root $relativeFile.Replace("/", "\")
        if (-not (Test-Path -LiteralPath $packagePath -PathType Leaf)) {
            Fail "$Label package is missing required file '$relativeFile'."
        }

        $hash = Get-FileSha256 -Path $packagePath
        if ($relativeFile -eq "LICENSE" -or $relativeFile -eq "THIRD_PARTY_LICENSES.txt") {
            $sourcePath = Join-Path $repoRoot $relativeFile
            $sourceHash = Get-FileSha256 -Path $sourcePath
            if ($hash -ne $sourceHash) {
                Fail "$Label package file '$relativeFile' does not match the source-controlled file."
            }
        }

        $record = [ordered]@{
            package = $Label
            path = $relativeFile
            length = (Get-Item -LiteralPath $packagePath).Length
            sha256 = $hash
        }
        $validated.Add($record)
        $validatedFiles.Add($record)
    }

    $allFiles = @(Get-ChildItem -LiteralPath $Root -Recurse -File)
    $badArtifacts = @($allFiles | Where-Object {
        $baseName = [System.IO.Path]::GetFileNameWithoutExtension($_.Name)
        $_.Name -eq "arkanoid_tests.exe" -or
        $_.Name -like "gtest*.dll" -or
        $_.Name -like "gmock*.dll" -or
        $_.Name -like "*.pdb" -or
        $_.Name -like "*.ilk" -or
        $_.Name -like "*.exp" -or
        $_.Name -like "*.lib" -or
        ($_.Extension -eq ".dll" -and $baseName.EndsWith("d", [System.StringComparison]::OrdinalIgnoreCase))
    })
    if ($badArtifacts.Count -gt 0) {
        Fail "$Label package contains forbidden build/test artifacts: $($badArtifacts.Name -join ', ')"
    }

    $exeDetails = Assert-WindowsGuiExecutable -Path (Join-Path $Root "bin\arkanoid.exe") -Label $Label
    $result = [ordered]@{
        root = $Root
        requiredFiles = $validated.ToArray()
        executable = $exeDetails
        fileCount = $allFiles.Count
    }
    Add-GateResult -Name "$Label-package-validation" -Status "passed" -Details $result

    return $result
}

function Invoke-StartupSmoke([string]$PackageRoot, [string]$Label) {
    $binDir = Join-Path $PackageRoot "bin"
    $gameExePath = Join-Path $binDir "arkanoid.exe"
    if (-not (Test-Path -LiteralPath $gameExePath -PathType Leaf)) {
        Fail "$Label smoke executable does not exist: $gameExePath"
    }

    $started = Get-Date
    Write-Host ""
    Write-Host "[$Label-smoke] $gameExePath --release-startup-smoke"

    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $gameExePath
    $startInfo.Arguments = "--release-startup-smoke"
    $startInfo.WorkingDirectory = $binDir
    $startInfo.UseShellExecute = $false

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    [void]$process.Start()

    try {
        if (-not $process.WaitForExit(10000)) {
            $process.Kill()
            $process.WaitForExit()
            Fail "$Label startup smoke timed out after 10 seconds."
        }

        if ($process.ExitCode -ne 0) {
            Fail "$Label startup smoke failed with exit code $($process.ExitCode)."
        }

        $completed = Get-Date
        $result = [ordered]@{
            name = $Label
            packageRoot = $PackageRoot
            executablePath = $gameExePath
            workingDirectory = $binDir
            exitCode = $process.ExitCode
            startedAtUtc = Get-UtcText $started
            completedAtUtc = Get-UtcText $completed
        }
        $smokeResults.Add($result)
        Add-GateResult -Name "$Label-smoke" -Status "passed" -Details $result
    } finally {
        $process.Dispose()
    }
}

try {
    Normalize-WindowsProcessEnvironment

    if ([System.Environment]::OSVersion.Platform -ne [System.PlatformID]::Win32NT) {
        Fail "The Windows release gate must run on a Windows host."
    }

    Require-Command -Name "cmake" -CommandType Application | Out-Null
    Require-Command -Name "ctest" -CommandType Application | Out-Null
    Require-Command -Name "Compress-Archive" -CommandType Function | Out-Null
    Require-Command -Name "Expand-Archive" -CommandType Function | Out-Null

    $resolvedVcpkgRoot = Resolve-VcpkgRoot -RequestedRoot $VcpkgRoot -WriteStatus -PassThru
    $toolchainFile = Get-VcpkgToolchainFile -VcpkgRoot $resolvedVcpkgRoot
    $normalizedBuildIdentity = $null
    if (-not [string]::IsNullOrWhiteSpace($BuildIdentity)) {
        $normalizedBuildIdentity = $BuildIdentity.Trim()
    }

    Assert-RequiredRepoFiles -RepoRoot $repoRoot -RelativePaths @(
        "CMakeLists.txt",
        "CMakePresets.json",
        "vcpkg.json",
        "LICENSE",
        "THIRD_PARTY_LICENSES.txt",
        "src/main.cpp",
        "src/app/run_mode.cpp",
        "tests/CMakeLists.txt"
    )
    Assert-ArkanoidPresetContract -RepoRoot $repoRoot -VcpkgRoot $resolvedVcpkgRoot | Out-Null

    Set-Location -LiteralPath $repoRoot
    $buildDir = Get-WindowsBuildDir -RepoRoot $repoRoot
    if ($Clean) {
        Clear-KnownBuildDirectory -BuildDir $buildDir -RepoRoot $repoRoot -Label "release build"
    } else {
        Assert-CachedBuildDirectoryMatches -BuildDir $buildDir -ToolchainFile $toolchainFile
    }

    New-Item -ItemType Directory -Force -Path $distRoot | Out-Null
    if ($Clean) {
        Remove-ReleaseControlledOutputs
    }
    New-Item -ItemType Directory -Force -Path $runsDir | Out-Null

    $commitHash = Get-GitValue @("rev-parse", "--short", "HEAD")
    if (-not $commitHash) {
        $commitHash = "unknown"
    }

    $startedAt = Get-Date
    $runId = "{0}-{1}-{2}" -f $startedAt.ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ"), $PID, $commitHash
    $runRoot = Join-Path $runsDir $runId
    $stageDir = Join-Path $runRoot "stage"
    $asciiExtractDir = Join-Path $runRoot "extract-ascii"
    $unicodeToken = New-StringFromCodePoints -CodePoints @(0x30b2, 0x30fc, 0x30e0)
    $unicodeExtractDir = Join-Path $runRoot ("extract-nonascii-{0}" -f $unicodeToken)
    $runZipPath = Join-Path $runRoot "arkanoid-win64.zip"
    $tempPromotedInstallDir = Join-Path $distRoot ("_install-promote-{0}" -f $runId)
    $tempPromotedZipPath = Join-Path $distRoot ("_arkanoid-win64-{0}.zip" -f $runId)
    $script:RunManifestPath = Join-Path $runRoot "run-manifest.json"

    Assert-PathContainsOnlyAscii -Path (Get-FullPath $asciiExtractDir) -Label "ASCII package extraction path"
    Assert-PathContainsNonAscii -Path (Get-FullPath $unicodeExtractDir) -Label "Non-ASCII package extraction path"

    New-CleanDirectory -Path $runRoot -AllowedRoot $runsDir -Label "release run"
    New-Item -ItemType Directory -Force -Path $stageDir | Out-Null

    $script:Manifest = [ordered]@{
        status = "running"
        runId = $runId
        startedAtUtc = Get-UtcText $startedAt
        completedAtUtc = $null
        commitHash = $commitHash
        buildIdentity = $normalizedBuildIdentity
        repoRoot = $repoRoot
        runRoot = $runRoot
        generator = "Visual Studio 18 2026"
        architecture = "x64"
        configuration = "Release"
        triplet = "x64-windows"
        vcpkgRoot = $resolvedVcpkgRoot
        vcpkgToolchainFile = $toolchainFile
        commands = $commandResults
        gates = $gateResults
        smokeResults = $smokeResults
        validatedFiles = $validatedFiles
        installPath = $stageDir
        asciiExtractPath = $asciiExtractDir
        unicodeExtractPath = $unicodeExtractDir
        runZipPath = $runZipPath
        canonicalInstallPath = $canonicalInstallDir
        canonicalZipPath = $canonicalZipPath
        zipSha256 = $null
        error = $null
    }
    Save-Manifest
    Write-ReleaseStatus -GateStatus "running" -Message "release gate started"

    Add-GateResult -Name "preflight" -Status "passed" -Details ([ordered]@{
        vcpkgRoot = $resolvedVcpkgRoot
        toolchainFile = $toolchainFile
        buildDir = $buildDir
    })

    Invoke-GateCommand -Name "configure" -Command "cmake" -Arguments @("--preset", "windows-vcpkg") | Out-Null
    Invoke-GateCommand -Name "release-build" -Command "cmake" -Arguments @(
        "--build", "--preset", "windows-release",
        "--target", "arkanoid",
        "--parallel"
    ) | Out-Null
    Invoke-GateCommand -Name "release-tests-build" -Command "cmake" -Arguments @(
        "--build", "--preset", "windows-release",
        "--target", "arkanoid_tests",
        "--parallel"
    ) | Out-Null
    Invoke-GateCommand -Name "release-tests" -Command "ctest" -Arguments @(
        "--preset", "windows-release-tests",
        "--output-on-failure",
        "--no-tests=error"
    ) | Out-Null
    Invoke-GateCommand -Name "install" -Command "cmake" -Arguments @(
        "--install", $buildDir,
        "--config", "Release",
        "--prefix", $stageDir
    ) | Out-Null

    $installedValidation = Validate-PackageTree -Root $stageDir -Label "installed"
    $installedSnapshot = Get-PackageSnapshot -Root $stageDir
    Invoke-StartupSmoke -PackageRoot $stageDir -Label "installed"
    $installedAfterSmokeSnapshot = Get-PackageSnapshot -Root $stageDir
    Assert-PackageUnchanged -Before $installedSnapshot -After $installedAfterSmokeSnapshot -Label "installed"

    $stageItems = @(Get-ChildItem -LiteralPath $stageDir -Force)
    if ($stageItems.Count -eq 0) {
        Fail "Install stage is empty: $stageDir"
    }
    if (Test-Path -LiteralPath $runZipPath -PathType Leaf) {
        Remove-KnownFile -Path $runZipPath -AllowedRoot $runRoot -Label "run zip"
    }
    Compress-Archive -LiteralPath $stageItems.FullName -DestinationPath $runZipPath -CompressionLevel Optimal -Force
    if (-not (Test-Path -LiteralPath $runZipPath -PathType Leaf)) {
        Fail "Zip creation did not produce '$runZipPath'."
    }

    $script:Manifest.zipSha256 = Get-FileSha256 -Path $runZipPath
    Add-GateResult -Name "zip-create" -Status "passed" -Details ([ordered]@{
        path = $runZipPath
        sha256 = $script:Manifest.zipSha256
        length = (Get-Item -LiteralPath $runZipPath).Length
    })

    New-Item -ItemType Directory -Force -Path $asciiExtractDir | Out-Null
    Expand-Archive -LiteralPath $runZipPath -DestinationPath $asciiExtractDir -Force
    Add-GateResult -Name "ascii-zip-extract" -Status "passed" -Details ([ordered]@{
        path = $asciiExtractDir
    })
    $asciiValidation = Validate-PackageTree -Root $asciiExtractDir -Label "ascii-extracted"
    $asciiSnapshot = Get-PackageSnapshot -Root $asciiExtractDir
    Assert-PackageSnapshotMatches -Expected $installedSnapshot -Actual $asciiSnapshot -Label "ascii-extracted"
    Invoke-StartupSmoke -PackageRoot $asciiExtractDir -Label "ascii-extracted"
    $asciiAfterSmokeSnapshot = Get-PackageSnapshot -Root $asciiExtractDir
    Assert-PackageUnchanged -Before $asciiSnapshot -After $asciiAfterSmokeSnapshot -Label "ascii-extracted"

    New-Item -ItemType Directory -Force -Path $unicodeExtractDir | Out-Null
    Expand-Archive -LiteralPath $runZipPath -DestinationPath $unicodeExtractDir -Force
    Add-GateResult -Name "nonascii-zip-extract" -Status "passed" -Details ([ordered]@{
        path = $unicodeExtractDir
    })
    $unicodeValidation = Validate-PackageTree -Root $unicodeExtractDir -Label "nonascii-extracted"
    $unicodeSnapshot = Get-PackageSnapshot -Root $unicodeExtractDir
    Assert-PackageSnapshotMatches -Expected $installedSnapshot -Actual $unicodeSnapshot -Label "nonascii-extracted"
    Invoke-StartupSmoke -PackageRoot $unicodeExtractDir -Label "nonascii-extracted"
    $unicodeAfterSmokeSnapshot = Get-PackageSnapshot -Root $unicodeExtractDir
    Assert-PackageUnchanged -Before $unicodeSnapshot -After $unicodeAfterSmokeSnapshot -Label "nonascii-extracted"

    New-CleanDirectory -Path $tempPromotedInstallDir -AllowedRoot $distRoot -Label "temporary promoted install"
    foreach ($stageItem in $stageItems) {
        Copy-Item -LiteralPath $stageItem.FullName -Destination $tempPromotedInstallDir -Recurse -Force
    }
    Copy-Item -LiteralPath $runZipPath -Destination $tempPromotedZipPath -Force

    Remove-KnownDirectory -Path $canonicalInstallDir -AllowedRoot $distRoot -Label "canonical install"
    Move-DirectoryUnderRoot -Source $tempPromotedInstallDir -Destination $canonicalInstallDir -AllowedRoot $distRoot

    Remove-KnownFile -Path $canonicalZipPath -AllowedRoot $distRoot -Label "canonical zip"
    Move-FileUnderRoot -Source $tempPromotedZipPath -Destination $canonicalZipPath -AllowedRoot $distRoot

    $latest = [ordered]@{
        status = "success"
        runId = $runId
        completedAtUtc = Get-UtcText (Get-Date)
        commitHash = $commitHash
        buildIdentity = $normalizedBuildIdentity
        installPath = $canonicalInstallDir
        zipPath = $canonicalZipPath
        zipSha256 = Get-FileSha256 -Path $canonicalZipPath
        runManifestPath = $script:RunManifestPath
        installedPackage = $installedValidation
        asciiExtractedPackage = $asciiValidation
        nonAsciiExtractedPackage = $unicodeValidation
    }
    Write-JsonFile -Path $latestPath -Value $latest

    $script:Manifest.status = "success"
    $script:Manifest.completedAtUtc = $latest.completedAtUtc
    $script:Manifest.canonicalInstallPath = $canonicalInstallDir
    $script:Manifest.canonicalZipPath = $canonicalZipPath
    Save-Manifest
    Write-ReleaseStatus -GateStatus "success" -Message "release gate passed"

    Write-Host ""
    Write-Host "Release gate passed."
    Write-Host "Install: $canonicalInstallDir"
    Write-Host "Zip: $canonicalZipPath"
    Write-Host "SHA-256: $($latest.zipSha256)"
} catch {
    $message = $_.Exception.Message
    $positionMessage = $_.InvocationInfo.PositionMessage
    $scriptStackTrace = $_.ScriptStackTrace
    [Console]::Error.WriteLine($message)
    if ($positionMessage) {
        [Console]::Error.WriteLine($positionMessage)
    }
    if ($scriptStackTrace) {
        [Console]::Error.WriteLine($scriptStackTrace)
    }

    if ($script:Manifest) {
        $script:Manifest.status = "failed"
        $script:Manifest.completedAtUtc = Get-UtcText (Get-Date)
        $script:Manifest.error = [ordered]@{
            message = $message
            position = $positionMessage
            stack = $scriptStackTrace
        }
        Save-Manifest
        Write-ReleaseStatus -GateStatus "failed" -Message $message
    }

    $exitCode = 1
} finally {
    Restore-CallerState -State $callerState
}

exit $exitCode
