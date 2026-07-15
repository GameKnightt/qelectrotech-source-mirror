[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$trackedPaths = @(git -c core.quotepath=false ls-files)
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to enumerate tracked files.'
}

$caseCollisions = @(
    $trackedPaths |
        Group-Object { $_.ToUpperInvariant() } |
        Where-Object Count -gt 1
)

if ($caseCollisions.Count -gt 0) {
    foreach ($collision in $caseCollisions) {
        Write-Error (
            'Case-insensitive path collision: ' +
            ($collision.Group -join ', ')
        )
    }
    throw 'The repository cannot be checked out cleanly on Windows.'
}

$lfsPointerSignature =
    'version https://git-lfs.github.com/' + 'spec/v1'
$lfsPointers = @(
    git grep -l -F $lfsPointerSignature -- . 2>$null
)
$grepExitCode = $LASTEXITCODE
$global:LASTEXITCODE = 0
if ($grepExitCode -gt 1) {
    throw 'Unable to inspect the Git index for LFS pointers.'
}

if ($lfsPointers.Count -gt 0) {
    foreach ($pointer in $lfsPointers) {
        Write-Error "Tracked Git LFS pointer: $pointer"
    }
    throw 'The current checkout must not depend on Git LFS objects.'
}

Write-Host (
    "Repository contracts satisfied: $($trackedPaths.Count) tracked paths, " +
    'no case-insensitive collision, no Git LFS pointer.'
)
