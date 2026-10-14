param(
    [string]$Version = "0.1.0-preview"
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

$packageName = "MetroidZeroMissionRecomp-windows-x64-v$Version"
$stageDir = Join-Path $root "release-stage\$packageName"
$zipPath = Join-Path $root "release-stage\$packageName.zip"

Write-Host "Staging release to: $stageDir"
if (Test-Path $stageDir) { Remove-Item -Recurse -Force $stageDir }
if (Test-Path $zipPath) { Remove-Item -Force $zipPath }

New-Item -ItemType Directory -Force $stageDir | Out-Null
New-Item -ItemType Directory -Force (Join-Path $stageDir "saves") | Out-Null

# 1. Binary and DLLs
Copy-Item "$root\build\host\MetroidZeroMissionRecomp.exe" $stageDir -Force
Copy-Item "$root\build\host\SDL2.dll" $stageDir -Force
Copy-Item "$root\build\host\libgcc_s_seh-1.dll" $stageDir -Force
Copy-Item "$root\build\host\libstdc++-6.dll" $stageDir -Force
Copy-Item "$root\build\host\libwinpthread-1.dll" $stageDir -Force

# 2. recomp-ui assets
Copy-Item -Recurse "$root\build\host\assets" $stageDir -Force

# 3. Config template
Copy-Item "$root\game.toml" $stageDir -Force

# 4. Documentation and Notices
Copy-Item "$root\README.md" $stageDir -Force
Copy-Item "$root\README.zh-CN.md" $stageDir -Force
Copy-Item "$root\LICENSE" $stageDir -Force
Copy-Item "$root\THIRD_PARTY_NOTICES.md" $stageDir -Force
Copy-Item -Recurse "$root\licenses" $stageDir -Force

# 5. Strict Security Check: Verify prohibited files are ABSENT
$prohibited = Get-ChildItem -Path $stageDir -Recurse -Include *.gba, *.bin, *.sav, *.log, *.pdb, *.cpp, *.h
if ($prohibited) {
    throw "Security violation! Prohibited files detected in release package: $($prohibited.FullName -join ', ')"
}

# 6. Generate SHA256SUMS and manifest.json
$files = Get-ChildItem -Path $stageDir -Recurse -File
$checksumLines = @()
$manifestFiles = @()

foreach ($f in $files) {
    $relative = $f.FullName.Substring($stageDir.Length + 1).Replace('\', '/')
    $hash = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $checksumLines += "$hash  $relative"
    $manifestFiles += @{
        path = $relative
        size = $f.Length
        sha256 = $hash
    }
}

$checksumLines | Set-Content -LiteralPath (Join-Path $stageDir "SHA256SUMS.txt") -Encoding ASCII

$manifest = @{
    name = "MetroidZeroMissionRecomp"
    version = $Version
    platform = "windows-x64"
    package = "$packageName.zip"
    build_time = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
    game = @{
        title = "Metroid: Zero Mission"
        code = "BMXE"
        region = "USA"
        rom_sha1 = "5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8"
        bios_sha1 = "300c20df6731a33952ded8c436f7f186d25d3492"
    }
    files = $manifestFiles
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $stageDir "manifest.json") -Encoding UTF8

Write-Host "Creating zip archive: $zipPath"
Compress-Archive -Path "$stageDir\*" -DestinationPath $zipPath -CompressionLevel Optimal

$zipItem = Get-Item -LiteralPath $zipPath
$zipHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
Write-Host "Release ZIP created successfully:"
Write-Host "  Path:   $zipPath"
Write-Host "  Size:   $($zipItem.Length) bytes ($([math]::Round($zipItem.Length / 1MB, 2)) MB)"
Write-Host "  SHA256: $zipHash"

# 7. Isolated extraction & smoke verification
$testDir = Join-Path $root "build\release-test"
if (Test-Path $testDir) { Remove-Item -Recurse -Force $testDir }
New-Item -ItemType Directory -Force $testDir | Out-Null
Expand-Archive -Path $zipPath -DestinationPath $testDir -Force

$testExe = Join-Path $testDir "MetroidZeroMissionRecomp.exe"
$out = & $testExe --help | Out-String
if ($LASTEXITCODE -ne 0 -or $out -notmatch "MetroidZeroMissionRecomp") {
    throw "Standalone extracted binary failed smoke test!"
}
Write-Host "Standalone extracted binary verified successfully in isolated folder!"
