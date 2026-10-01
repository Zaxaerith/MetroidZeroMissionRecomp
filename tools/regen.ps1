param([Parameter(Mandatory=$true)][string]$Bios)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
& "$PSScriptRoot\apply-framework-patches.ps1"
$roms = @(Get-ChildItem -LiteralPath $root -Filter '*.gba' -File)
if ($roms.Count -ne 1) { throw 'Expected exactly one ROM in the project root.' }
$identity = Get-Content -LiteralPath "$root\docs\ROM_IDENTITY.json" -Raw | ConvertFrom-Json
if ((Get-FileHash -LiteralPath $roms[0].FullName -Algorithm SHA1).Hash -ne $identity.rom.sha1) { throw 'Wrong ROM SHA-1.' }
if ((Get-FileHash -LiteralPath $roms[0].FullName -Algorithm SHA256).Hash -ne $identity.rom.sha256) { throw 'Wrong ROM SHA-256.' }
if ((Get-FileHash -LiteralPath $Bios -Algorithm SHA1).Hash -ne $identity.bios.sha1) { throw 'Wrong BIOS SHA-1.' }
$pin = Get-Content -LiteralPath "$root\docs\FRAMEWORK_PIN.json" -Raw | ConvertFrom-Json
$framework = "$root\reference\gbarecomp"
$safe = "safe.directory=$($framework.Replace('\','/'))"
$actual = & git -c $safe -C $framework rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $actual -ne $pin.framework.commit) { throw 'Framework revision mismatch.' }
$tool = "$root\build\framework\gba_recompile.exe"
if (-not (Test-Path -LiteralPath $tool)) { throw 'Build the pinned framework generator first.' }
New-Item -ItemType Directory -Force -Path "$root\logs" | Out-Null
& $tool --rom $roms[0].FullName --config "$root\game.toml" --out "$root\generated\cart" --max-functions 65536 *> "$root\logs\host-cart-generation.log"
if ($LASTEXITCODE -ne 0) { throw 'Cartridge generation failed; see logs/host-cart-generation.log.' }
& $tool --bios $Bios --config "$framework\bios\gba_bios.toml" --out "$root\generated\bios" *> "$root\logs\host-bios-generation.log"
if ($LASTEXITCODE -ne 0) { throw 'BIOS generation failed; see logs/host-bios-generation.log.' }
