# Gløshaugen Strike – arbeidsinstruks for Claude

Online førstepersons skytespill på NTNU Gløshaugen, laget i Unreal Engine 5.8 (C++).
Eier: Andreas. Han skriver norsk og har ingen kodeerfaring.

## Hvordan du snakker med Andreas
- Svar alltid på norsk, i enkle, hele setninger. Ingen fagsjargong uten forklaring.
- Si hva du har gjort og hva han eventuelt må gjøre (for eksempel «trykk Play», «lukk Unreal»).
- Han tester ved å åpne spillet i Unreal og trykke Play. Si konkret hva han skal se etter.

## Mappestruktur
- `Source/Gloshaugen/` – all spillkode (C++). `Public/` = .h, `Private/` = .cpp. Klassene heter `GS*`.
  - `GSCampusBuilder` bygger hele campus i kode når spillet starter (kartet er tomt).
  - `GSItemCatalog.gen.inl` er **generert** – ikke rediger den for hånd (se under).
- `Tools/ItemCatalog/` – alle våpen og gjenstander. Rediger `catalog.py`, kjør så
  `python Tools/ItemCatalog/gen_cpp.py`.
- `Tools/Blender/` – Python-skript som lager 3D-modeller (`MeshKit`). Én modell = `SM_<Type>_<Navn>`,
  meter, origo på gulvet i midten, front mot +X.
- `Tools/Textures/`, `Tools/Audio/` – lager teksturer og lyd.
- `SourceArt/` – råfiler (FBX, teksturer). `Content/Python/init_unreal.py` importerer dem automatisk
  hver gang editoren åpnes.
- `Content/` – Unreal-filer (.uasset/.umap). Binære: ikke rediger, og ikke commit dem uten grunn.
- `IMPROVEMENTS.md` – loggen over forbedringer (Andreas' mål: minst 1000). Nyeste runde øverst,
  én konkret forbedring per nummerert linje. Fortsett nummereringen.

## Bygge og sjekke
Unreal er installert i `C:\Program Files\Epic Games\UE_5.8`.

Kompilere C++ (bare når editoren er **lukket**, ellers nekter Live Coding):
```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" GloshaugenEditor Win64 Development -Project="%CD%\Gloshaugen.uproject" -WaitMutex
```
- Sjekk først om editoren kjører: `Get-Process UnrealEditor`. Hvis den kjører: ikke lukk den selv.
  Be Andreas trykke **Ctrl+Alt+F11** (Live Coding) i Unreal, eller lukke editoren slik at du kan bygge.
- Åpne editoren: `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%CD%\Gloshaugen.uproject"`
  (dobbeltklikk på .uproject åpner Visual Studio på denne PC-en, ikke Unreal).
- Logger: `Saved/Logs/Gloshaugen.log`. Byggefeil: les loggen og fiks årsaken, ikke symptomet.

## Regler for endringer
- Små, avgrensede endringer. Én forbedring om gangen, så bygg og sjekk.
- Følg stilen i filene rundt (Unreal-konvensjoner: `F`/`U`/`A`/`E`-prefiks, `UPROPERTY`, replikering
  for alt som skal virke online – spillet er flerspiller med listen server).
- Commit-meldinger på engelsk, korte, i stilen `Område: hva som er gjort` (se `git log`).
- Jobb på en egen gren, aldri direkte på `main`. Aldri `git push --force`.
- Ikke slett filer eller innhold Andreas har laget uten å spørre.

## Agentene og forbedringsløkken
Prosjektet har tre agenter i `.claude/agents/`:
1. **planlegger** – finner neste forbedring og lager en konkret plan.
2. **bygger** – gjennomfører planen og bygger.
3. **kontrollor** – ser over arbeidet, godkjenner eller sender tilbake.

Det er tre lag med disse tre agentene (ni i alt): **Kamp**, **Grafikk og kart** og **Spillbarhet**.
Hvert lag eier sine egne filer, se `.claude/lag/LAG.md`.

**Du som leser dette i hovedøkten er hovedagenten** som styrer de ni og snakker med Andreas. Les
`.claude/lag/HOVEDAGENT.md` og `.claude/lag/prioriteringer.md`.

`/forbedringsloop` kjører dem i en løkke (plan → bygg → kontroll → neste runde), slik at spillet
forbedres hele tiden. Planer og vurderinger lagres i `.claude/loop/` så neste runde lærer av forrige.
