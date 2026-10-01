param([Parameter(Mandatory=$true)][string]$Bios)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath $Bios -PathType Leaf)) { throw 'BIOS file missing.' }
& python "$PSScriptRoot\smoke_gameplay.py" --bios $Bios `
    --route "$root\tests\routes\rooms.csv" --expected "$root\tests\rooms_smoke.json" `
    --out "$root\logs\rooms-smoke" --audio-start 6200 --no-wav
if ($LASTEXITCODE -ne 0) { throw 'Smoke failed; see logs/rooms-smoke/result.json and native.log.' }
