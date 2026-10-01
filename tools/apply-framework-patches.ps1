$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$repo = "$root\reference\gbarecomp"
$safe = "safe.directory=$($repo.Replace('\','/'))"
$pin = Get-Content "$root\docs\FRAMEWORK_PIN.json" -Raw | ConvertFrom-Json
$actual = & git -c $safe -C $repo rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $actual -ne $pin.framework.commit) { throw 'Framework revision mismatch.' }
foreach ($entry in $pin.framework.patches) {
    $patch = "$root\$($entry.path)"
    if ((Get-FileHash -LiteralPath $patch -Algorithm SHA256).Hash -ne $entry.sha256) { throw "Patch hash mismatch: $patch" }
    # Reverse-check makes repeated builds idempotent; never overwrite an
    # incompatible checkout or discard local work.
    $ErrorActionPreference = 'Continue'
    & git -c $safe -C $repo apply --reverse --check $patch 2>$null
    if ($LASTEXITCODE -eq 0) { $ErrorActionPreference = 'Stop'; continue }
    & git -c $safe -C $repo apply --check $patch 2>$null
    if ($LASTEXITCODE -ne 0) { throw "Cannot apply framework patch cleanly: $patch" }
    & git -c $safe -C $repo apply $patch
    if ($LASTEXITCODE -ne 0) { throw "Framework patch failed: $patch" }
    $ErrorActionPreference = 'Stop'
}
