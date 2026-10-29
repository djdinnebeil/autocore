
#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$DistRoot = Join-Path $RepoRoot 'dist'
$InfoRoot = Join-Path $DistRoot 'info'

$GuideSource = Join-Path $RepoRoot 'docs\Auto Core Guide.md'
$GuideDestination = Join-Path $DistRoot 'Auto Core Guide.md'

$LegalSource = @(
    (Join-Path $RepoRoot 'notice.md'),
    (Join-Path $RepoRoot 'license')
)

$ManualSource = Join-Path $RepoRoot 'docs\manual'
$TutorialSource = Join-Path $RepoRoot 'docs\tutorial'

$InfoDocs = @(
    'Auto Core Manual.md',
    'Auto Core Tutorial.md'
)

try {
    $LegalDestination = Join-Path $InfoRoot 'legal'
    $ManualDestination = Join-Path $InfoRoot 'manual'
    $TutorialDestination = Join-Path $InfoRoot 'tutorial'

    $Sources = @(
        $GuideSource
        $ManualSource
        $TutorialSource
    ) + $LegalSource

    foreach ($name in $InfoDocs) {
        $Sources += Join-Path $RepoRoot "docs\$name"
    }

    foreach ($path in $Sources) {
        if (-not (Test-Path -LiteralPath $path)) {
            throw "Missing documentation source: $path"
        }
    }

    New-Item -ItemType Directory -Force -Path $InfoRoot | Out-Null

    Remove-Item -LiteralPath $LegalDestination -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $ManualDestination -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $TutorialDestination -Recurse -Force -ErrorAction SilentlyContinue

    foreach ($name in $InfoDocs) {
        Remove-Item -LiteralPath (Join-Path $InfoRoot $name) -Force -ErrorAction SilentlyContinue
    }

    # Remove the Guide from its previous location.
    Remove-Item -LiteralPath (Join-Path $InfoRoot 'Auto Core Guide.md') -Force -ErrorAction SilentlyContinue

    New-Item -ItemType Directory -Force -Path $LegalDestination | Out-Null

    foreach ($path in $LegalSource) {
        Copy-Item -LiteralPath $path -Destination $LegalDestination
    }

    Copy-Item -LiteralPath $ManualSource `
        -Destination $ManualDestination `
        -Recurse

    Copy-Item -LiteralPath $TutorialSource `
        -Destination $TutorialDestination `
        -Recurse

    foreach ($name in $InfoDocs) {
        Copy-Item -LiteralPath (Join-Path $RepoRoot "docs\$name") `
            -Destination (Join-Path $InfoRoot $name)
    }

    Copy-Item -LiteralPath $GuideSource `
        -Destination $GuideDestination `
        -Force

    Write-Host 'Published documentation to dist.'
}
catch {
    Write-Host ''
    Write-Host 'Documentation publish failed.'
    Write-Host $_.Exception.Message
    exit 1
}
