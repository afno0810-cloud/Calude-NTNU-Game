# Lærdom fra forbedringsløkken

Generelle regler løkken har lært. Alle tre agentene leser denne før de starter.
Oppdateres etter hver runde av `/forbedringsloop` (fra kontrollørens «Lærdom»).
Hold listen kort: slå sammen like regler, og fjern regler som har vist seg feil.
Hver regel: én linje, med (runde N) bak, og et telletall når den bekreftes igjen.

## Regler
- Bygg aldri mens Unreal-editoren er åpen; be Andreas bruke Ctrl+Alt+F11 eller lukke den. (start)
- `GSItemCatalog.gen.inl` er generert: endre `catalog.py` og kjør `gen_cpp.py`. (start)
- Kamera/view target online må tåle at RPC-er og replikerte verdier kommer i vilkårlig rekkefølge: sett tilstanden på både server og klient, og ha en billig vakt som bare retter når noe faktisk er feil. (runde 1)
- `Build.bat` som sier «Target is up to date» etter byggerens bygg er godt nok bevis på at koden kompilerer. (runde 1)
- Planer for nettverksting må nevne motorens egne kamera-/possession-kall (SetPawn, ClientRestart); byggeren måtte finne dem selv. (runde 1)
