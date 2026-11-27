# Headless import of every model and material, without opening the editor (Grafikk runde 5a).
#
#   powershell -ExecutionPolicy Bypass -File Tools\Unreal\run_import.ps1
#
# Runs UnrealEditor-Cmd with -run=pythonscript: the start-up runs Content/Python/init_unreal.py (import, collision,
# Nanite fallback, materials, physical materials), then Content/Python/gs_headless_import.py checks every Helgasetr
# model and writes Saved/Logs/gs_import_report.txt. Log: Saved/Logs/headless_import.log.
# Refuses to start while any Unreal process (editor, game window, another team's -Cmd run) or a C++ build runs:
# two processes on the same .uasset files lock them. Runs once more if the report lists locked files.
# Exit code 0 = all Helgasetr models OK.
param([int]$TimeoutMinutes = 30)

$ErrorActionPreference = 'Stop'
$Project = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$UProject = Join-Path $Project 'Gloshaugen.uproject'
$Cmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Script = Join-Path $Project 'Content\Python\gs_headless_import.py'
$Log = Join-Path $Project 'Saved\Logs\headless_import.log'
$Report = Join-Path $Project 'Saved\Logs\gs_import_report.txt'

function Test-Busy {
    $ue = Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue
    if ($ue) { return "Unreal kjører allerede (" + (($ue | ForEach-Object { "$($_.ProcessName) $($_.Id)" }) -join ', ') + ")" }
    $ubt = Get-CimInstance Win32_Process -Filter "Name='dotnet.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.CommandLine -match 'UnrealBuildTool' }
    if ($ubt) { return "en C++-bygging (UnrealBuildTool) kjører" }
    return $null
}

function Invoke-Import {
    $busy = Test-Busy
    if ($busy) {
        Write-Host "STOPP: $busy. Prøv igjen når den er ferdig (ingenting er endret)."
        exit 2
    }
    if (Test-Path $Report) { Remove-Item $Report -Force }
    $argList = @("`"$UProject`"", '-run=pythonscript', "-script=`"$Script`"", '-unattended', '-nosplash', '-nop4',
              '-stdout', '-FullStdOutLogOutput', "-abslog=`"$Log`"")
    Write-Host "Kjører hodeløs import (tidsgrense $TimeoutMinutes min) ..."
    $p = Start-Process -FilePath $Cmd -ArgumentList $argList -PassThru -WindowStyle Hidden
    $null = $p.Handle   # keeps the handle, so ExitCode is there after the wait
    if (-not $p.WaitForExit($TimeoutMinutes * 60 * 1000)) {
        Write-Host "Tidsgrensen gikk ut: stopper importen."
        Stop-Process -Id $p.Id -Force
        exit 3
    }
    return $p.ExitCode
}

$code = Invoke-Import
$locked = $false
if (Test-Path $Report) {
    $locked = (Select-String -Path $Report -Pattern 'ste filer .*: ingen' -Encoding UTF8 -Quiet) -eq $false
}
if ($locked) {
    Write-Host 'Rapporten har låste filer: kjører importen én gang til.'
    $code = Invoke-Import
}
if (Test-Path $Report) {
    Get-Content $Report -TotalCount 5 | ForEach-Object { Write-Host $_ }
} else {
    Write-Host "Ingen rapport ($Report). Se $Log."
}
Select-String -Path $Log -Pattern 'import-rapport:|alle \d+ modelldeler' -ErrorAction SilentlyContinue | Select-Object -Last 2 | ForEach-Object { Write-Host $_.Line }
Write-Host "Prosesskode: $code"
exit $code
