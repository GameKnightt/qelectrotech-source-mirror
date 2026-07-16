[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ArchivePath
)

$ErrorActionPreference = 'Stop'

function Get-FullPath([string]$Path) {
    return [System.IO.Path]::GetFullPath($Path)
}

$archive = Get-FullPath $ArchivePath
if (-not (Test-Path -LiteralPath $archive -PathType Leaf)) {
    throw "Portable archive not found: $archive"
}
if ([System.IO.Path]::GetExtension($archive) -ne '.zip') {
    throw "Portable archive must be a ZIP file: $archive"
}

$tempParent = Get-FullPath ([System.IO.Path]::GetTempPath())
$tempPrefix = $tempParent.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar) +
    [System.IO.Path]::DirectorySeparatorChar
$extractPath = Get-FullPath (Join-Path $tempParent (
    'qet-portable-archive-' + [Guid]::NewGuid().ToString('N')))
if (-not $extractPath.StartsWith(
        $tempPrefix,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Unsafe temporary extraction path: $extractPath"
}
if (Test-Path -LiteralPath $extractPath) {
    throw "Temporary extraction path already exists: $extractPath"
}

try {
    Expand-Archive -LiteralPath $archive -DestinationPath $extractPath

    $manifestPath = Join-Path $extractPath 'manifest-sha256.txt'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw 'Extracted portable archive has no manifest-sha256.txt.'
    }

    $rootPrefix = $extractPath.TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    $manifestLines = @(Get-Content -LiteralPath $manifestPath -Encoding utf8)
    if ($manifestLines.Count -eq 0) {
        throw 'Extracted portable manifest is empty.'
    }

    $manifestFiles = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($line in $manifestLines) {
        $match = [regex]::Match($line, '^([0-9A-Fa-f]{64}) \*(.+)$')
        if (-not $match.Success) {
            throw "Invalid portable manifest entry: $line"
        }

        $relativePath = $match.Groups[2].Value
        $pathParts = @($relativePath -split '[\\/]')
        if ([System.IO.Path]::IsPathRooted($relativePath) -or
            $pathParts -contains '..') {
            throw "Unsafe portable manifest path: $relativePath"
        }

        $filePath = Get-FullPath (Join-Path $extractPath $relativePath)
        if (-not $filePath.StartsWith(
                $rootPrefix,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Portable manifest path escapes the archive root: $relativePath"
        }
        if (-not (Test-Path -LiteralPath $filePath -PathType Leaf)) {
            throw "Portable manifest entry was not extracted: $relativePath"
        }

        $normalizedRelativePath = $relativePath.Replace(
            [System.IO.Path]::AltDirectorySeparatorChar,
            [System.IO.Path]::DirectorySeparatorChar)
        if (-not $manifestFiles.Add($normalizedRelativePath)) {
            throw "Duplicate portable manifest entry: $relativePath"
        }

        $expectedHash = $match.Groups[1].Value.ToLowerInvariant()
        $actualHash = Get-FileHash -LiteralPath $filePath -Algorithm SHA256
        $actualHash = $actualHash.Hash.ToLowerInvariant()
        if ($actualHash -ne $expectedHash) {
            throw "Portable manifest hash mismatch: $relativePath"
        }
    }

    $extractedFiles = @(Get-ChildItem -LiteralPath $extractPath -Recurse -File |
        Where-Object {
            -not $_.FullName.Equals(
                $manifestPath,
                [System.StringComparison]::OrdinalIgnoreCase)
        })
    foreach ($file in $extractedFiles) {
        $relativePath = $file.FullName.Substring($rootPrefix.Length)
        if (-not $manifestFiles.Contains($relativePath)) {
            throw "Extracted portable file is absent from manifest: $relativePath"
        }
    }
    if ($extractedFiles.Count -ne $manifestFiles.Count) {
        throw (
            "Portable manifest/file count mismatch: {0} entries, {1} files." -f
            $manifestFiles.Count,
            $extractedFiles.Count)
    }

    $requiredFiles = @(
        'qelectrotech.exe',
        'Launch-QElectroTech-Preview.bat',
        'platforms\qoffscreen.dll',
        'share\qt5\qml\QtQuick.2\qmldir',
        'share\qt5\qml\QtQml\qmldir',
        'share\qt5\qml\QtQml\Models.2\qmldir',
        'share\qt5\qml\QtQml\WorkerScript.2\qmldir',
        'qt.conf',
        'examples\ArduinoLCD.qet',
        'examples\grafcet.qet',
        'examples\Habitat-Unifilaire.qet',
        'examples\industrial.qet'
    )
    foreach ($relativePath in $requiredFiles) {
        if (-not (Test-Path -LiteralPath (
                    Join-Path $extractPath $relativePath) -PathType Leaf)) {
            throw "Required portable file was not extracted: $relativePath"
        }
    }

    Write-Output (
        "Portable ZIP round-trip validated: {0} manifest entries." -f
        $manifestLines.Count)
}
finally {
    if (Test-Path -LiteralPath $extractPath) {
        $resolvedCleanup = Get-FullPath $extractPath
        if (-not $resolvedCleanup.StartsWith(
                $tempPrefix,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing unsafe temporary cleanup: $resolvedCleanup"
        }
        Remove-Item -LiteralPath $resolvedCleanup -Recurse -Force
    }
}
