param([switch]$IncludeDecomp)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$pin = Get-Content -LiteralPath "$root\docs\FRAMEWORK_PIN.json" -Raw | ConvertFrom-Json
$identity = Get-Content -LiteralPath "$root\docs\ROM_IDENTITY.json" -Raw | ConvertFrom-Json
function Ensure-Pinned([string]$Relative, [string]$Repository, [string]$Commit) {
    $destination = Join-Path $root $Relative
    $safe = "safe.directory=$($destination.Replace('\','/'))"
    if (-not (Test-Path -LiteralPath (Join-Path $destination '.git'))) {
        if ((Test-Path -LiteralPath $destination) -and
            (Get-ChildItem -LiteralPath $destination -Force | Select-Object -First 1)) {
            throw "Existing non-Git dependency directory: $Relative. Preserve it and resolve explicitly."
        }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
        & git clone $Repository $destination
        if ($LASTEXITCODE -ne 0) { throw "Clone failed: $Relative" }
        & git -c $safe -C $destination checkout --detach $Commit
        if ($LASTEXITCODE -ne 0) { throw "Pin checkout failed: $Relative" }
    }
    $actual = & git -c $safe -C $destination rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actual -ne $Commit) {
        throw "Existing dependency pin mismatch: $Relative. Resolve it without discarding local work."
    }
}
Ensure-Pinned 'reference\gbarecomp' $pin.framework.repository $pin.framework.commit
Ensure-Pinned 'reference\gbarecomp\external\arm-recomp-core' $pin.arm_core.repository $pin.arm_core.commit
Ensure-Pinned 'reference\SDL2' $pin.sdl2.repository $pin.sdl2.commit
Ensure-Pinned 'build\framework\_deps\tomlplusplus-src' $pin.tomlplusplus.repository $pin.tomlplusplus.commit
# Setup never duplicates the user ROM. with-decomp-rom.ps1 supplies a temporary
# filename-compatible copy only for an explicitly requested decomp command.
if ($IncludeDecomp) {
    Ensure-Pinned 'reference\mzm' $identity.decomp.repository $identity.decomp.commit
}
Write-Host 'Pinned dependencies available. Provide your ROM/BIOS and run build-host.ps1 -Regenerate.'
