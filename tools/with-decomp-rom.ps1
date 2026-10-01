param([Parameter(Mandatory=$true)][scriptblock]$Action)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$canonical = Join-Path $root 'Metroid - Zero Mission (USA).gba'
$decomp = Join-Path $root 'reference\mzm'
$temporary = Join-Path $decomp 'mzm_us_baserom.gba'
$identity = Get-Content -LiteralPath "$root\docs\ROM_IDENTITY.json" -Raw | ConvertFrom-Json
if (-not (Test-Path -LiteralPath $decomp -PathType Container)) {
    throw 'Obtain the optional pinned checkout with setup-deps.ps1 -IncludeDecomp first.'
}
if ((Get-FileHash -LiteralPath $canonical -Algorithm SHA1).Hash -ne $identity.rom.sha1 -or
    (Get-FileHash -LiteralPath $canonical -Algorithm SHA256).Hash -ne $identity.rom.sha256) {
    throw 'Canonical ROM identity mismatch.'
}
if (Get-Item -LiteralPath $temporary -Force -ErrorAction SilentlyContinue) {
    throw 'Existing decomp ROM will not be overwritten or removed. Resolve it explicitly first.'
}
try {
    Copy-Item -LiteralPath $canonical -Destination $temporary
    # Wait for the requested command to complete; do not launch detached work.
    & $Action $decomp
}
finally {
    if (Test-Path -LiteralPath $temporary -PathType Leaf) {
        Remove-Item -LiteralPath $temporary -Force
    }
}
