[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,

    [Parameter(Mandatory = $true)]
    [string]$SourceDir,

    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    [string]$Msys2Root = 'C:\msys64'
)

$ErrorActionPreference = 'Stop'

function Get-FullPath([string]$Path) {
    return [System.IO.Path]::GetFullPath($Path)
}

function Assert-DirectoryContainsFile(
    [string]$Path,
    [string]$Filter,
    [string]$Description
) {
    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "Missing $Description directory: $Path"
    }
    if (-not (Get-ChildItem -LiteralPath $Path -Recurse -File -Filter $Filter |
            Select-Object -First 1)) {
        throw "$Description directory contains no '$Filter' file: $Path"
    }
}

if (-not [Environment]::Is64BitProcess) {
    throw 'Run this deployment script from 64-bit PowerShell.'
}

$buildPath = Get-FullPath $BuildDir
$sourcePath = Get-FullPath $SourceDir
$outputPath = Get-FullPath $OutputDir
$ucrtBin = Join-Path (Get-FullPath $Msys2Root) 'ucrt64\bin'
$system32 = Join-Path $env:WINDIR 'System32'
$sourceExecutable = Join-Path $buildPath 'qelectrotech.exe'
$cmakeCache = Join-Path $buildPath 'CMakeCache.txt'
$launcherSource = Join-Path $sourcePath 'packaging\windows\Launch-QElectroTech-Preview.bat'
$windeployqt = Join-Path $ucrtBin 'windeployqt-qt5.exe'
$qmakeQt5 = Join-Path $ucrtBin 'qmake-qt5.exe'
$qtPluginRoot = Join-Path (Get-FullPath $Msys2Root) 'ucrt64\share\qt5\plugins'
$objdump = Join-Path $ucrtBin 'objdump.exe'

if ($outputPath -eq [System.IO.Path]::GetPathRoot($outputPath)) {
    throw "OutputDir must not be a filesystem root: $outputPath"
}
if (Test-Path -LiteralPath $outputPath) {
    throw "OutputDir must not already exist: $outputPath"
}

$outputParent = Split-Path -Parent $outputPath
if (-not (Test-Path -LiteralPath $outputParent -PathType Container)) {
    throw "OutputDir parent must already exist: $outputParent"
}
$parentItem = Get-Item -LiteralPath $outputParent -Force
if ($parentItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
    throw "OutputDir parent must not be a reparse point: $outputParent"
}

if (-not (Test-Path -LiteralPath $sourceExecutable -PathType Leaf)) {
    throw "Missing QElectroTech executable: $sourceExecutable"
}
if (-not (Test-Path -LiteralPath $cmakeCache -PathType Leaf)) {
    throw "Missing CMake cache: $cmakeCache"
}
if (-not (Test-Path -LiteralPath $launcherSource -PathType Leaf)) {
    throw "Missing portable preview launcher: $launcherSource"
}
$buildType = Select-String -LiteralPath $cmakeCache -Pattern '^CMAKE_BUILD_TYPE:STRING=(.+)$' |
    Select-Object -First 1
if (-not $buildType -or $buildType.Matches[0].Groups[1].Value -ne 'Release') {
    throw 'BuildDir must contain a single-configuration Release build.'
}
if (-not (Test-Path -LiteralPath $windeployqt -PathType Leaf)) {
    throw "Missing Qt 5 deployment tool: $windeployqt"
}
if (-not (Test-Path -LiteralPath $objdump -PathType Leaf)) {
    throw "Missing objdump: $objdump"
}

$requiredSourceFiles = @('LICENSE', 'ELEMENTS.LICENSE')
foreach ($relativePath in $requiredSourceFiles) {
    $requiredPath = Join-Path $sourcePath $relativePath
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "Missing required license: $requiredPath"
    }
}

$requiredSourceDirectories = @('elements', 'titleblocks', 'lang', 'licenses')
foreach ($relativePath in $requiredSourceDirectories) {
    $requiredPath = Join-Path $sourcePath $relativePath
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Container)) {
        throw "Missing required source directory: $requiredPath"
    }
}
Assert-DirectoryContainsFile (Join-Path $sourcePath 'elements') '*.elmt' 'elements'
Assert-DirectoryContainsFile (Join-Path $sourcePath 'titleblocks') '*.titleblock' 'titleblocks'
Assert-DirectoryContainsFile (Join-Path $sourcePath 'lang') 'qet_*.qm' 'translations'

$outputLeaf = Split-Path -Leaf $outputPath
$stagingPath = Join-Path $outputParent (
    ".{0}.staging-{1}" -f $outputLeaf, [Guid]::NewGuid().ToString('N'))
$deployedExecutable = Join-Path $stagingPath 'qelectrotech.exe'

try {
    New-Item -ItemType Directory -Path $stagingPath | Out-Null
    Copy-Item -LiteralPath $sourceExecutable -Destination $deployedExecutable
    Copy-Item -LiteralPath $launcherSource -Destination $stagingPath
    Copy-Item -LiteralPath (Join-Path $sourcePath 'LICENSE') -Destination $stagingPath
    Copy-Item -LiteralPath (Join-Path $sourcePath 'ELEMENTS.LICENSE') -Destination $stagingPath
    Copy-Item -LiteralPath (Join-Path $sourcePath 'licenses') -Destination $stagingPath -Recurse

    $previousPath = $env:PATH
    try {
        $env:PATH = "$ucrtBin;$previousPath"
        if (Test-Path -LiteralPath $qmakeQt5 -PathType Leaf) {
            # The PE closure below copies the compiler runtime and every other
            # non-system dependency deterministically.
            & $windeployqt --release --no-compiler-runtime $deployedExecutable
            if ($LASTEXITCODE -ne 0) {
                throw "windeployqt-qt5 failed with exit code $LASTEXITCODE"
            }
        } else {
            # A partially repaired local Qt 5 installation may be usable by
            # CMake while lacking the qmake-qt5.exe name required by
            # windeployqt-qt5. Copy the explicit runtime set used by
            # QElectroTech; the PE closure below supplies its linked Qt and
            # UCRT64 dependencies.
            $requiredQtPlugins = @(
                'bearer\qgenericbearer.dll',
                'iconengines\qsvgicon.dll',
                'imageformats\qgif.dll',
                'imageformats\qico.dll',
                'imageformats\qjpeg.dll',
                'imageformats\qsvg.dll',
                'platforms\qwindows.dll',
                'printsupport\windowsprintersupport.dll',
                'sqldrivers\qsqlite.dll',
                'styles\qwindowsvistastyle.dll'
            )
            foreach ($relativePlugin in $requiredQtPlugins) {
                $pluginSource = Join-Path $qtPluginRoot $relativePlugin
                if (-not (Test-Path -LiteralPath $pluginSource -PathType Leaf)) {
                    throw "Missing required Qt plugin: $pluginSource"
                }
                $pluginDestination = Join-Path $stagingPath $relativePlugin
                $pluginDestinationDirectory = Split-Path -Parent $pluginDestination
                if (-not (Test-Path -LiteralPath $pluginDestinationDirectory)) {
                    New-Item -ItemType Directory -Path $pluginDestinationDirectory |
                        Out-Null
                }
                Copy-Item -LiteralPath $pluginSource -Destination $pluginDestination
            }

            # Qt loads ANGLE dynamically, so objdump cannot discover these
            # libraries through the PE import table. Match windeployqt's
            # portable output explicitly, including its Direct3D compiler.
            $requiredDynamicLibraries = @(
                (Join-Path $ucrtBin 'libEGL.dll'),
                (Join-Path $ucrtBin 'libGLESv2.dll'),
                (Join-Path $system32 'D3DCompiler_47.dll')
            )
            foreach ($dynamicLibrary in $requiredDynamicLibraries) {
                if (-not (Test-Path -LiteralPath $dynamicLibrary -PathType Leaf)) {
                    throw "Missing required dynamically loaded library: $dynamicLibrary"
                }
                Copy-Item -LiteralPath $dynamicLibrary -Destination $stagingPath
            }
        }
    } finally {
        $env:PATH = $previousPath
    }

    # QElectroTech uses SQLite. Removing unrelated SQL drivers avoids shipping
    # database client dependencies which are not used by the application.
    $optionalSqlDrivers = @(
        'qsqlibase.dll',
        'qsqlmysql.dll',
        'qsqlodbc.dll',
        'qsqlpsql.dll'
    )
    foreach ($driver in $optionalSqlDrivers) {
        $driverPath = Join-Path $stagingPath "sqldrivers\$driver"
        if (Test-Path -LiteralPath $driverPath -PathType Leaf) {
            Remove-Item -LiteralPath $driverPath
        }
    }

    # MSYS2's windeployqt deploys the direct Qt DLLs but can omit their UCRT64
    # dependencies (ICU, HarfBuzz, double-conversion, SQLite, and others).
    # Resolve the complete PE dependency closure from the same toolchain.
    $queue = [System.Collections.Generic.Queue[string]]::new()
    Get-ChildItem -LiteralPath $stagingPath -Recurse -File |
        Where-Object { $_.Extension -in '.exe', '.dll' } |
        ForEach-Object { $queue.Enqueue($_.FullName) }

    $seen = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    $copied = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    $unresolved = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    $stagingPrefix = $stagingPath.TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar

    while ($queue.Count -gt 0) {
        $binary = $queue.Dequeue()
        if (-not $seen.Add($binary)) {
            continue
        }

        $objdumpOutput = @(& $objdump -p $binary 2>&1)
        $objdumpExitCode = $LASTEXITCODE
        if ($objdumpExitCode -ne 0) {
            throw "objdump failed for '$binary' with exit code $objdumpExitCode"
        }
        if (-not ($objdumpOutput -match 'file format pei-x86-64')) {
            throw "Runtime binary is not a 64-bit PE file: $binary"
        }

        $dependencies = $objdumpOutput |
            Select-String 'DLL Name:' |
            ForEach-Object { ($_.Line -split ':', 2)[1].Trim() }

        foreach ($dependency in $dependencies) {
            if ([System.IO.Path]::IsPathRooted($dependency) -or
                $dependency -ne [System.IO.Path]::GetFileName($dependency) -or
                $dependency.Contains([System.IO.Path]::DirectorySeparatorChar) -or
                $dependency.Contains([System.IO.Path]::AltDirectorySeparatorChar)) {
                throw "Unsafe PE dependency name '$dependency' imported by '$binary'"
            }

            $destination = Get-FullPath (Join-Path $stagingPath $dependency)
            if (-not $destination.StartsWith(
                    $stagingPrefix,
                    [System.StringComparison]::OrdinalIgnoreCase)) {
                throw "PE dependency escapes the staging directory: $dependency"
            }
            if (Test-Path -LiteralPath $destination -PathType Leaf) {
                continue
            }

            $toolchainDependency = Join-Path $ucrtBin $dependency
            if (Test-Path -LiteralPath $toolchainDependency -PathType Leaf) {
                Copy-Item -LiteralPath $toolchainDependency -Destination $destination
                [void]$copied.Add($dependency)
                $queue.Enqueue($destination)
                continue
            }

            $isWindowsForwarder = $dependency -match '^(api|ext)-ms-win-'
            $isWindowsSystemDll = Test-Path -LiteralPath (Join-Path $system32 $dependency)
            if (-not $isWindowsForwarder -and -not $isWindowsSystemDll) {
                [void]$unresolved.Add($dependency)
            }
        }
    }

    if ($unresolved.Count -gt 0) {
        throw "Unresolved runtime dependencies: $($unresolved -join ', ')"
    }

    $runtimeData = @(
        @{ Source = 'elements'; Destination = 'elements' },
        @{ Source = 'titleblocks'; Destination = 'titleblocks' }
    )
    foreach ($entry in $runtimeData) {
        $dataSource = Join-Path $sourcePath $entry.Source
        $dataDestination = Join-Path $stagingPath $entry.Destination
        Copy-Item -LiteralPath $dataSource -Destination $dataDestination -Recurse
    }

    $languageSource = Join-Path $sourcePath 'lang'
    $languageDestination = Join-Path $stagingPath 'l10n'
    New-Item -ItemType Directory -Path $languageDestination | Out-Null
    Get-ChildItem -LiteralPath $languageSource -File -Filter '*.qm' |
        Copy-Item -Destination $languageDestination

    $requiredRuntimeFiles = @(
        'qelectrotech.exe',
        'Launch-QElectroTech-Preview.bat',
        'Qt5Core.dll',
        'libEGL.dll',
        'libGLESv2.dll',
        'D3DCompiler_47.dll',
        'platforms\qwindows.dll',
        'sqldrivers\qsqlite.dll',
        'LICENSE',
        'ELEMENTS.LICENSE'
    )
    foreach ($relativePath in $requiredRuntimeFiles) {
        $requiredPath = Join-Path $stagingPath $relativePath
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            throw "Incomplete portable runtime, missing: $relativePath"
        }
    }
    Assert-DirectoryContainsFile (Join-Path $stagingPath 'elements') '*.elmt' 'deployed elements'
    Assert-DirectoryContainsFile (Join-Path $stagingPath 'titleblocks') '*.titleblock' 'deployed titleblocks'
    Assert-DirectoryContainsFile (Join-Path $stagingPath 'l10n') 'qet_*.qm' 'deployed translations'

    # Verify startup without inheriting MSYS2 from PATH. --version exits before
    # opening a window and exercises the native loader and the Qt platform setup.
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $deployedExecutable
    $startInfo.Arguments = '--version'
    $startInfo.WorkingDirectory = $stagingPath
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.EnvironmentVariables['PATH'] = "$system32;$env:WINDIR"

    $smokeProcess = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $smokeProcess.WaitForExit(15000)) {
        $smokeProcess.Kill()
        $smokeProcess.WaitForExit()
        throw 'Portable runtime smoke test timed out after 15 seconds.'
    }
    $versionOutput = $smokeProcess.StandardOutput.ReadToEnd().Trim()
    $versionError = $smokeProcess.StandardError.ReadToEnd().Trim()
    $versionExitCode = $smokeProcess.ExitCode
    $smokeProcess.Dispose()
    if ($versionExitCode -ne 0 -or [string]::IsNullOrWhiteSpace($versionOutput)) {
        throw (
            "Portable runtime smoke test failed with exit code {0}: {1}" -f
            $versionExitCode, $versionError)
    }

    $manifestPath = Join-Path $stagingPath 'manifest-sha256.txt'
    $manifestLines = @(Get-ChildItem -LiteralPath $stagingPath -Recurse -File |
        Where-Object { $_.FullName -ne $manifestPath } |
        Sort-Object FullName |
        ForEach-Object {
            $relativePath = $_.FullName.Substring($stagingPrefix.Length)
            $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
            "$hash *$relativePath"
        })
    $utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllLines($manifestPath, $manifestLines, $utf8WithoutBom)

    Move-Item -LiteralPath $stagingPath -Destination $outputPath
    Write-Output "Portable QElectroTech preview deployed to: $outputPath"
    Write-Output "Additional UCRT64 DLLs copied: $($copied.Count)"
    Write-Output ($versionOutput -join [Environment]::NewLine)
} catch {
    $originalError = $_
    if (Test-Path -LiteralPath $stagingPath) {
        for ($attempt = 1; $attempt -le 5; ++$attempt) {
            try {
                Remove-Item -LiteralPath $stagingPath -Recurse -Force
                break
            } catch {
                if ($attempt -eq 5) {
                    Write-Warning "Could not remove staging directory: $stagingPath"
                } else {
                    Start-Sleep -Milliseconds 200
                }
            }
        }
    }
    throw $originalError
}
