"""Headless import check (runde 5a). Run by Tools/Unreal/run_import.ps1:

    UnrealEditor-Cmd.exe Gloshaugen.uproject -run=pythonscript -script=".../Content/Python/gs_headless_import.py" -unattended ...

The commandlet's start-up already runs init_unreal.py (import, collision, Nanite fallback, materials, physical
materials) and keeps the report in gs_materials.RUN_ALL_REPORT. This script does not import again; it checks every
Helgasetr model once more (asset there, collision, Nanite, materials, PM_GS_*), writes Saved/Logs/gs_import_report.txt
and logs "Gloshaugen: import-rapport: N/58 Helgasetr-modeller OK, M feil". It raises (exit code != 0) when anything
is wrong, so the PowerShell script can see it.
"""
import unreal

import gs_materials

report = getattr(gs_materials, "RUN_ALL_REPORT", None)
import init_unreal  # noqa: E402  (as a real module; runs run_all() only if the start-up did not)

if report is None:
    report = init_unreal.LAST_REPORT
if report is None:
    report = init_unreal.run_all()

# Fresh check after everything the start-up did (the fixes can have saved the meshes after the first check).
check = dict(imported=report.get("imported", []), import_failed=report.get("import_failed", []),
             errors=report.get("errors", []), models=[])
init_unreal.check_helgasetr(check)
init_unreal.write_report(check)
n = len(check["models"])
unreal.log(f"Gloshaugen: hodeløs import ferdig: {check['ok_count']}/{n} OK, {check['bad_count']} feil, "
           f"{len(check['locked'])} låst, import feilet: {len(check['import_failed'])}")
if check["bad_count"] or check["locked"] or check["import_failed"]:
    raise RuntimeError(f"Gloshaugen: import-rapporten har feil ({check['bad_count']} feil, {len(check['locked'])} låst); "
                       f"se Saved/Logs/gs_import_report.txt")
