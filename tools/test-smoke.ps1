param(
    [Parameter(Mandatory=$true)][string]$Bios,
    [ValidateSet('rooms','demo')][string]$Route = 'rooms'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath $Bios -PathType Leaf)) { throw 'BIOS file missing.' }
$inputRoute = if ($Route -eq 'demo') { 'title' } else { 'rooms' }
$audioStart = if ($Route -eq 'demo') { 1000 } else { 6200 }
& python "$PSScriptRoot\smoke_gameplay.py" --bios $Bios `
    --route "$root\tests\routes\$inputRoute.csv" --expected "$root\tests\${Route}_smoke.json" `
    --out "$root\logs\${Route}-smoke" --audio-start $audioStart --no-wav
if ($LASTEXITCODE -ne 0) { throw "Smoke failed; see logs/${Route}-smoke/result.json and native.log." }
