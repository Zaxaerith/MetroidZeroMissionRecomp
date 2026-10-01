param(
    [Parameter(Mandatory=$true)][string]$Toolchain,
    [int]$Jobs = 4,
    [string]$Bios,
    [switch]$Regenerate
)
$ErrorActionPreference = 'Stop'
if ($Jobs -lt 1) { throw 'Jobs must be positive.' }
if ($Regenerate -and ([string]::IsNullOrWhiteSpace($Bios) -or -not (Test-Path -LiteralPath $Bios -PathType Leaf))) { throw 'Regeneration requires -Bios with your real BIOS path.' }
$root = Split-Path -Parent $PSScriptRoot
& "$PSScriptRoot\apply-framework-patches.ps1"
$pin = Get-Content -LiteralPath "$root\docs\FRAMEWORK_PIN.json" -Raw | ConvertFrom-Json
foreach ($entry in @(
    @{Path="$root\reference\gbarecomp"; Commit=$pin.framework.commit},
    @{Path="$root\reference\gbarecomp\external\arm-recomp-core"; Commit=$pin.arm_core.commit},
    @{Path="$root\reference\SDL2"; Commit=$pin.sdl2.commit},
    @{Path="$root\build\framework\_deps\tomlplusplus-src"; Commit=$pin.tomlplusplus.commit}
)) {
    $safe = "safe.directory=$($entry.Path.Replace('\','/'))"
    $actual = & git -c $safe -C $entry.Path rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actual -ne $entry.Commit) { throw "Dependency pin mismatch: $($entry.Path)" }
}
function Run-Logged([string]$Name, [string[]]$Arguments) {
    # Windows PowerShell wraps native stderr (including successful compiler
    # warnings) in ErrorRecords. Use the process exit code as the result.
    $ErrorActionPreference = 'Continue'
    & cmake @Arguments *> "$root\logs\$Name.log"
    if ($LASTEXITCODE -ne 0) { throw "CMake failed; see logs/$Name.log." }
}
$gcc = "$Toolchain\bin\gcc.exe".Replace('\','/')
$gxx = "$Toolchain\bin\g++.exe".Replace('\','/')
$common = @('-G','Ninja','-DCMAKE_BUILD_TYPE=Release',"-DCMAKE_C_COMPILER=$gcc","-DCMAKE_CXX_COMPILER=$gxx")
New-Item -ItemType Directory -Force -Path "$root\logs" | Out-Null
Run-Logged 'sdl-configure' (@('-S',"$root\reference\SDL2",'-B',"$root\build\sdl2") + $common + @('-DSDL_SHARED=ON','-DSDL_STATIC=OFF','-DSDL_TEST=OFF','-DSDL_TESTS=OFF',"-DCMAKE_INSTALL_PREFIX=$root\build\deps"))
Run-Logged 'sdl-build' @('--build',"$root\build\sdl2",'--parallel',"$Jobs")
Run-Logged 'sdl-install' @('--install',"$root\build\sdl2")
Run-Logged 'framework-configure' (@('-S',"$root\reference\gbarecomp",'-B',"$root\build\framework") + $common + @("-DGBARECOMP_TOMLPP_INCLUDE_DIR=$root\build\framework\_deps\tomlplusplus-src"))
Run-Logged 'framework-tool-build' @('--build',"$root\build\framework",'--target','gba_recompile','--parallel',"$Jobs")
if ($Regenerate) { & "$PSScriptRoot\regen.ps1" -Bios $Bios }
Run-Logged 'host-configure' (@('-S',$root,'-B',"$root\build\host") + $common + @("-DSDL2_INCLUDE_DIR=$root\build\deps\include\SDL2","-DSDL2_LIBRARY=$root\build\deps\lib\libSDL2.dll.a","-DGBARECOMP_MINGW_RUNTIME_BIN=$Toolchain\bin"))
Run-Logged 'host-build' @('--build',"$root\build\host",'--target','MetroidZeroMissionRecomp','--parallel',"$Jobs")
Get-Item -LiteralPath "$root\build\host\MetroidZeroMissionRecomp.exe" | Select-Object FullName,Length
