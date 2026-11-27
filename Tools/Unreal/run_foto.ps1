# Headless photo run through Helgasetr (Grafikk runde 5a).
#
#   powershell -ExecutionPolicy Bypass -File Tools\Unreal\run_foto.ps1 -Label foer
#
# Starts the game in a window (Test kartet, 1920x1080, no sound) with -ExecCmds="Grafikk.Foto": a fixed camera route
# with screenshots and timings; the game closes itself. If it is still running after 6 min it is stopped.
# The pictures are copied to Saved/Screenshots/foto_<Label>/ and the numbers (times, brightness, burnt-out and black
# pixels) go to .claude/lag/grafikk/kontroll/r05-maaling.md under "## <Label>". Log: Saved/Logs/foto_<Label>.log.
# Refuses to start while any Unreal process or a C++ build runs. The screen must be on (not a locked PC).
# -GammeltLys starts the game with -GSGammeltLys: the light as before round 5a (before/after from the same build).
param([string]$Label = 'foto', [int]$TimeoutMinutes = 6, [switch]$GammeltLys)

$ErrorActionPreference = 'Stop'
$Project = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$UProject = Join-Path $Project 'Gloshaugen.uproject'
$Exe = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$Log = Join-Path $Project "Saved\Logs\foto_$Label.log"
$Shots = Join-Path $Project 'Saved\Screenshots\foto'
$Dest = Join-Path $Project "Saved\Screenshots\foto_$Label"
$Md = Join-Path $Project '.claude\lag\grafikk\kontroll\r05-maaling.md'

$ue = Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue
if ($ue) { Write-Host ("STOPP: Unreal kjorer allerede (" + (($ue | ForEach-Object { "$($_.ProcessName) $($_.Id)" }) -join ', ') + "). Prov igjen senere."); exit 2 }
$ubt = Get-CimInstance Win32_Process -Filter "Name='dotnet.exe'" -ErrorAction SilentlyContinue | Where-Object { $_.CommandLine -match 'UnrealBuildTool' }
if ($ubt) { Write-Host 'STOPP: en C++-bygging (UnrealBuildTool) kjorer. Prov igjen senere.'; exit 2 }

if (Test-Path $Shots) { Remove-Item (Join-Path $Shots '*') -Force -ErrorAction SilentlyContinue }
$argList = @("`"$UProject`"", '/Game/Maps/Gloshaugen?Mode=Explore', '-game', '-windowed', '-ResX=1920', '-ResY=1080',
             '-nosound', '-nosplash', "-abslog=`"$Log`"", '-ExecCmds="Grafikk.Foto"')
if ($GammeltLys) { $argList += '-GSGammeltLys' }
Write-Host "Starter fotorunden '$Label' (tidsgrense $TimeoutMinutes min) ..."
$p = Start-Process -FilePath $Exe -ArgumentList $argList -PassThru
$null = $p.Handle
if (-not $p.WaitForExit($TimeoutMinutes * 60 * 1000)) {
    Write-Host 'Tidsgrensen gikk ut: lukker spillet.'
    Stop-Process -Id $p.Id -Force
    Start-Sleep -Seconds 2
}
# never leave a game window behind
Get-Process -Name 'UnrealEditor' -ErrorAction SilentlyContinue | Where-Object { $_.Id -eq $p.Id } | Stop-Process -Force -ErrorAction SilentlyContinue

New-Item -ItemType Directory -Force $Dest | Out-Null
Remove-Item (Join-Path $Dest '*') -Force -ErrorAction SilentlyContinue
$n = 0
if (Test-Path $Shots) { Get-ChildItem $Shots -Filter *.png | ForEach-Object { Copy-Item $_.FullName $Dest; $n++ } }
Write-Host "$n bilder kopiert til $Dest"
if (Test-Path $Log) {
    Select-String -Path $Log -Pattern 'Grafikk\.Foto:|alle \d+ modelldeler|reserve \(klosser\)|Helgasetr: flagg|spawn' -Encoding UTF8 |
        Select-Object -Last 40 | ForEach-Object { Write-Host $_.Line }
    & python (Join-Path $PSScriptRoot 'foto_stats.py') $Log $Dest $Label $Md | Out-Null
}
Write-Host "Prosesskode: $($p.ExitCode)"
