param(
    [Parameter(Mandatory=$true)][string]$Bios
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath $Bios -PathType Leaf)) { throw 'BIOS file missing.' }
$savedRun = Join-Path $root 'logs\save-station-smoke'
$reloadRun = Join-Path $root 'logs\save-reload-smoke'

# The driver always boots from reset and exits cleanly after each phase.
& python "$PSScriptRoot\smoke_gameplay.py" --bios $Bios `
    --route "$root\tests\routes\save-station.csv" `
    --expected "$root\tests\save_station_smoke.json" `
    --out $savedRun --audio-start 6200 --no-wav
if ($LASTEXITCODE -ne 0) { throw "Save-station smoke failed; see $savedRun." }

# Only the flushed SRAM crosses this process boundary. No savestate is used.
& python "$PSScriptRoot\smoke_gameplay.py" --bios $Bios `
    --save "$savedRun\native-test.sav" `
    --route "$root\tests\routes\save-reload.csv" `
    --expected "$root\tests\save_reload_smoke.json" `
    --out $reloadRun --audio-start 1700 --no-wav
if ($LASTEXITCODE -ne 0) { throw "SRAM reload smoke failed; see $reloadRun." }
Write-Host 'Save station -> clean exit -> fresh process -> saved room/equipment: PASS'
