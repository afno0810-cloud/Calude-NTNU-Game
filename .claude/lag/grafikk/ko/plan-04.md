# Plan 04 (Grafikk og kart): Helgasetr del 1 (lagets runde 4) – spillbart råbygg i riktig skala

Les først `.claude/lag/grafikk/helgasetr.md` (fasiten: akser, etasjer, rom, baser, ruter, interaktivitet). Koordinater
i meter: X = øst, Y = sør, Z = opp, origo = akse 5 × akse H på gulvet i Plan 1. Tabellene i helgasetr.md kap. 1 gir
aksene som tall.

## Mål
Andreas trykker Play og står i Helgasetr: et riktig skalert råbygg av Plan 1–4 rundt det store atriet med glasstak,
trappene, glassboksene og de to basene på Plan 4 i hver sin ende – han kan gå hele veien fra base til base på flere måter.

## Forbedrer (gammelt)
- Kartdataene i `GSCampus.h` (`Bases()`, `SpawnPoints()`, `PlayArea()`, `Buildings()`, `Roads()`) som GameMode, HUD,
  hjelpen og ballistikken leser: nå fylt med Helgasetr, så CTF, spawn, flagg, «Flagget sist sett» og minikartet virker
  uten endringer hos de andre lagene.
- Etasjenavnene i «Flagget sist sett» (`HelpPlaceName`) blir riktige i et bygg med ulike etasjehøyder (sonene får 4,3 m).
- Regn og snø stopper under glasstaket (dagens innendørssjekk i `GSEnvironment` treffer taket).

## Nytt
- Helgasetr som nytt kart (Blender-struktur + C++-bygger + felles datakilde), baser, spawn, ruter, sjekkskript.
- Alt som senere skal knuses eller åpnes (glassfelt, lettvegg-paneler, dører) legges allerede nå ut som egne instanser
  med ID, så runde 5 (dører og ødeleggelse) bare bytter oppførsel.

## Hvorfor nå
Andreas: «skrap det gamle mappet», «bruk god tid og lag det nye mappet veldig bra». Alt annet på kartet bygger på et
riktig råbygg. Spillbarhet og Kamp trenger dataene (baser, spawn, plasser, romklang) før de kan tilpasse seg.

## Filer
Nye: `Tools/Blender/helgasetr_plan.py`, `Tools/Blender/gen_helgasetr.py`, `Tools/Blender/building_helgasetr.py`,
`Tools/Blender/check_helgasetr.py`, `Tools/Blender/check_helgasetr_walk.py`, `Source/Gloshaugen/Public/GSHelgasetr.h`,
`Source/Gloshaugen/Public/GSHelgasetr.gen.inl` (generert), `Source/Gloshaugen/Public/GSHelgasetrBuilder.h`,
`Source/Gloshaugen/Private/GSHelgasetrBuilder.cpp`.
Endres: `GSCampus.h` (bryter + Helgasetr-data ut), `GSCampusBuilder.h` (`Build` og `SetStreetLightsOn` virtuelle,
`ISMs`/helpere `protected` – er det allerede), `GSWorldSubsystem.cpp` (lager riktig bygger), `Content/Python/init_unreal.py`
(kollisjon for `SM_Glass_Helgasetr*` og `SM_Mod_Helg_*`), `Content/Python/gs_materials.py` (3 nye instanser),
`IMPROVEMENTS.md` (549–564). Ingen filer slettes. Ingen andre lags filer.

## Steg

### 1. Datakilden `Tools/Blender/helgasetr_plan.py` (ren Python, ingen bpy)
Alt i meter, i spillets koordinater. Bruk akser som navn, så det kan sjekkes mot tegningen:
```python
AX = {"1": -25.75, "2": -15.40, "3": -13.54, "4": -6.77, "5": 0.0, "6": 6.77, "7": 13.54, "8": 15.40, "9": 19.20, "10": 25.75}
AY = {"N": -32.11, "M": -26.45, "L": -20.30, "K": -18.03, "J": -12.02, "I": -6.01, "H": 0.0,
      "G": 6.01, "F": 12.02, "E": 18.03, "D": 20.30, "C": 23.85, "B": 29.80, "A": 35.66}
LEVELS = {"P1": 0.0, "P2": 5.6, "P3": 9.9, "P4": 13.9, "P5": 18.2, "P6": 22.2}
ROOF_Z, RIDGE_Z, SLAB, WALL_OUT, WALL_IN, CORE_WALL = 26.2, 29.0, 0.35, 0.40, 0.15, 0.25
RING_IN = (13.25, 17.75)     # atriekanten |X|, |Y| (glassrekkverk), hjørneradius 1.5
RING_OUT = (15.55, 20.05)    # korridorveggen |X|, |Y| -> fri bredde 2,3 m
```
Lister (tupler eller små dataklasser – velg selv, men hold dem lesbare):
- `FOOTPRINT[level]`: ytre rektangler som utgjør etasjen, minus utsparingene i helgasetr.md kap. 2 (NV-hjørnet
  X < −13,54 og Y < −12,02 fra P3; vestterrassen X < −15,4, Y +6…+18 på P3; sørterrassen X −6,77…+13,54, Y > +29,8 fra P3;
  forplassen X < −15,4, Y < +1,0 på P1–P2).
- `VOIDS[level]`: hull i dekket. P1: ingen. P2: X −9,9…+9,0, Y −17,75…+5,6 (+ nordstripa Y −20,05…−17,75 over hele ringen,
  P2 har ingen nordkorridor) + hullet for trappeløp 1 og sosialtrappa. P3–P6: ringens innside |X| < 13,25, |Y| < 17,75
  (avrundet), pluss «åpent ned» på P3 over studentarbeidsplassene (X −6,77…+13,54, Y +20,3…+29,8), pluss trappehull.
- `ROOMS[level]`: (navn, x0, x1, y0, y1, type, dører). Type: `rom` (vegger, dør(er)), `lukket` (vegger, ingen dør –
  rommet trenger ikke innhold), `korridor`, `lobby`, `trapp`, `apen` (fellessone uten vegger). Dør: (x, y, bredde, art)
  med art `tre`, `glass_kode` (grupperom), `dobbel`, `brann` (trapperom). Dørhøyde 2,1 m. Rom og dører for P4 står i
  tabellen under (steg 1a); P1–P3 i steg 1b. Rom som ikke er nevnt er `lukket` med vegger der tegningen har vegger
  (åpne `plantegninger_2018.pdf` s. 3–6, se helgasetr.md kap. 0, og tegn grovt – nok til at korridorene har vegger).
- `STAIRS`: atriumtrappa løp 1–4 og trapperommene (tabell i steg 1c).
- `BOXES[level]`: glassboksene fra helgasetr.md kap. 4 (x0, x1, y0, y1, type `rom`/`apen`, dør på korridorsiden).
- `BASES`, `SPAWNS`, `ZONES`, `LOOT`, `TERMINALS`, `CAMERAS`, `SOUND_ZONES`, `COVER`, `LIGHTS` (steg 1d–1f).
- `BREACH_WALLS`: lettvegg-paneler som skal kunne sprenges i runde 5 (steg 1g).

#### 1a. Plan 4 (Z 13,9) – rom og dører
| Navn | X | Y | Type | Dører (x, y, bredde, art) |
|---|---|---|---|---|
| Nordbasen | −13,54…+6,77 | −32,11…−20,3 | rom | (−10,0, −20,3, 1,0, tre), (+3,0, −20,3, 1,0, tre), (+6,77, −22,0, 1,0, tre) |
| Nord-lobby | +6,77…+13,54 | −23,85…−20,3 | lobby | (+10,2, −20,3, 1,8, dobbel) |
| Grupperom 4.022 | +6,77…+13,54 | −32,11…−23,85 | lukket | – |
| Nord-spawngang | +13,54…+15,4 | −32,11…−20,3 | korridor | åpen mot Nord-lobby (X +13,54, Y −23,85…−20,3) |
| N1 grupperom 4.035 | +15,4…+25,75 | −26,0…−20,3 | rom | (+15,4, −23,0, 1,0, tre), (+24,0, −20,3, 1,0, tre) |
| N2 grupperom 4.021 | +15,4…+25,75 | −32,11…−26,0 | rom | (+15,4, −29,0, 1,0, tre) |
| NØ-gang | +15,4…+25,75 | −20,3…−12,5 | korridor | åpen mot østkorridoren (X +15,55, Y −20,05…−14,0) |
| Trapp øst + lobby | +15,4…+25,75 | −12,5…−7,5 | trapp | lobby X +15,4…+16,9 åpen mot ringen; (+16,9, −10,0, 1,2, brann) |
| Østfløy: garderober | +15,4…+25,75 | −7,5…−3,0 | lukket | – |
| Treningsleiligheten | +15,4…+25,75 | −3,0…+4,0 | rom | (+15,55, 0,0, 1,0, tre), (+20,5, +4,0, 1,0, tre) |
| Ferdighetsrom vernepleier | +15,4…+25,75 | +4,0…+12,0 | rom | (+15,55, +8,0, 1,0, tre), (+20,5, +12,0, 1,0, tre) |
| Ferdighetsrom radiografi | +15,4…+25,75 | +12,0…+20,3 | rom | (+15,55, +16,0, 1,0, tre) |
| Undervisningsrom SØ | +15,4…+25,75 | +20,3…+35,66 | lukket | – |
| Sørbasen | −6,77…+13,54 | +20,3…+29,8 | rom | (+10,0, +20,3, 1,0, tre), (−3,0, +20,3, 1,0, tre), (−6,77, +22,0, 1,0, tre) |
| Sør-lobby | −13,54…−6,77 | +20,3…+23,85 | lobby | (−10,2, +20,3, 1,8, dobbel); heis sør (lukket boks); (−10,2, +23,85, 1,2, brann) til Trapp sør |
| Trapp sør | −13,54…−6,77 | +23,85…+35,66 | trapp | – |
| Sør-spawngang | −15,4…−13,54 | +20,3…+32,11 | korridor | åpen mot Sør-lobby (X −13,54, Y +20,3…+23,85); vegg mot ringen ved Y +20,3 |
| S1 grupperom 4.041 | −25,75…−15,4 | +20,3…+26,0 | rom | (−15,4, +23,0, 1,0, tre), (−24,0, +20,3, 1,0, tre) |
| S2 grupperom 4.037 | −25,75…−15,4 | +26,0…+32,11 | rom | (−15,4, +29,0, 1,0, tre) |
| S3 garderobe | −25,75…−13,54 | +32,11…+35,66 | lukket | – |
| Venterom/mottak | −25,75…−15,4 | +14,0…+20,3 | korridor | åpen mot vestkorridoren (X −15,55, Y +14,0…+20,05) |
| Audiologi 4.056 | −25,75…−15,4 | +4,0…+14,0 | rom | (−20,5, +14,0, 1,0, tre), (−15,55, +9,0, 1,0, tre), (−20,5, +4,0, 1,0, tre) |
| Psykoakustisk lab | −25,75…−15,4 | −3,0…+4,0 | rom | (−20,5, −3,0, 1,0, tre) |
| Audiologi 4.057 | −25,75…−15,4 | −12,02…−3,0 | rom | (−15,55, −8,0, 1,0, tre) |
| Sørkorridoren øst for basen | +13,54…+15,4 | +20,3…+35,66 | lukket | – (grupperom 5 pl, lukket) |

Ringkorridoren (`korridor`) går rundt mellom RING_IN og RING_OUT. Atriekanten har glassrekkverk 1,1 m, avbrutt der
glassboksene og trappeløp 3/4 møter kanten. Rommene utenfor ringen har yttervegg med vindusbånd (brystning 0,9 m,
vindu til 2,9 m over gulv; fast glass).

#### 1b. Plan 1–3 (det som trengs for rutene)
- **P1 (Z 0):** åpent atriegulv X −15,4…+15,4, Y −20,3…+5,6 (terrazzo). Glassvegg (fast, fasadeglass-typen) mot klatrehallen
  langs Y −20,3 fra X −13,54 til +6,77, hallen bak er `lukket` (en mørk boks med en grå buldreblokk, ses gjennom glasset).
  Servicesenter-disk X +6,8…+13,2, Y −19,8…−18,6, høyde 1,05 (dekning). Vindfang vest X −17,4…−15,4, Y −10…−6 (glassdører
  låst, fast glass). Kjøkken/servering `lukket` X −13,54…+6,77, Y +5,6…+20,3 med disk 1,0 m høy langs Y +5,6 (X −6…+6,77).
  Treningssenteret øst `lukket` X +13,54…+25,75, Y −7,5…+35,66 og NØ X +6,77…+25,75, Y −32,11…−20,3; glassvegg mot atriet
  langs X +15,4 (Y −7,5…+5,6). Trapp øst-lobby som på P4. SV-korridor X −15,4…−12,9, Y +5,6…+20,3, Sør-lobby og Trapp sør
  som på P4. Studentlokalet og servicerommene i vest/sørvest `lukket`.
- **P2 (Z 5,6):** se helgasetr.md kap. 3. Vestkorridor, sørkorridor (Y +18,03…+20,3 mellom X −13,54 og +13,54 er åpen
  mot mesaninen), østkorridor, ingen nordkorridor. Mesaninen (oransje gulv) med tre grupperom (glass, `rom` med
  `glass_kode`-dør mot nord) og sittetrappa-blokken (trinn 0,45 m, topp 1,2 m – dekning). Studentarbeidsplassene X −6,77…+13,54,
  Y +20,3…+29,8 med to åpninger 4 m ved X −3…+1 og +6…+10 i veggen Y +20,3, takhøyde helt til P4-dekket. Undervisningsrom 120
  pl vest (`rom`, dører (−15,55, −14,0) og (−15,55, −6,0)). Møterommene øst X +9…+13,25, Y −12…−1 (`rom`, glass mot atriet).
- **P3 (Z 9,9):** hel ring; «åpent ned» sør med glassrekkverk langs Y +20,3; vest: undervisningsrom 50 pl (Y −12,02…−1,0,
  dør (−15,55, −6,5)) og grupperom 30 pl (Y −1,0…+6,01, dør (−15,55, +2,5)), vinduer mot vestterrassen Y +6…+18; nord:
  undervisningsrom 100 pl X −13,54…−0,5 (dør (−7,0, −20,05)) og X +6,77…+25,75, Y −32,11…−22,0 (dør (+10,0, −20,05)),
  resten `lukket`; øst: ergoterapi X +15,4…+25,75, Y −6,0…+4,0 (dør (+15,55, −1,0)) og Y +6,0…+12,0 (dør (+15,55, +9,0)).
- **P5–P6:** kulisse. Ringdekke, glassrekkverk, glassbokser (kopier P4-boksene), yttervegger med vindu, rommene `lukket`
  med vegg mot ringen. P6: bare ringen og nord/øst-blokkene; resten tak/terrasse ute.

#### 1c. Trapper (ekte trinn; Kamps bestilling)
| Trapp | Plassering | Detaljer |
|---|---|---|
| Atriumtrappa løp 1 (P1→P2) | X −12,9…−9,9; bunn Y −6,5 (Z 0) → topp Y +5,6 (Z 5,6) | 2 × 15–16 opptrinn, 0,18/0,30, repos 2,5 m midt; gule vanger 0,4 m høye over trinnkanten |
| Sosialtrappa (P1→P2) | X −9,6…−6,0; bunn Y −5,0 → topp Y +5,6 | sittetrinn 0,45/0,90 i vest-halvdelen, vanlig trapp 0,18/0,30 i øst-halvdelen (1,2 m) |
| Løp 2 (P2→P3) | Y +6,2…+8,6; bunn X −1,0 (Z 5,6) → topp X +7,5 (Z 9,9) | 24 opptrinn, repos 1,2 m; henger fritt (bunnplate synlig fra kantina) |
| Løp 3 (P3→P4) | X +10,0…+13,0; bunn Y +6,0 (Z 9,9) → topp Y −1,8 (Z 13,9) | 22 opptrinn, repos 1,2 m |
| Løp 4 (P4→P5) | X +10,0…+13,0; Y −1,8 → −10,0 | bygges, men glassport (fast, `Mat_Glass`) ved bunnen: «Kun ansatte» |
| Trapp sør (P1→P4) | X −13,54…−6,77, Y +23,85…+35,66 | to løp per etasje langs Y, repos i begge ender, betong, stålrekkverk; dør i lobbyen hver etasje; sperret over P4 og under P1 med fast branndør |
| Trapp øst (P1→P4) | X +16,9…+25,75, Y −12,5…−7,5 | to løp per etasje langs X; ellers som Trapp sør |
Trinn: høyeste opptrinn ≤ 0,20 m (Kamp: `MaxStepHeight` 50 cm, men føttene skal stå på trinnene). Fri høyde over alle
trinn ≥ 2,2 m.

#### 1d. Baser, spawn, soner
Fra helgasetr.md kap. 5 – nøyaktig disse tallene:
- `BASES = [("Sor", "Sørbasen", (3.4, 26.0, 13.9), -90.0, SPAWN_S), ("Nord", "Nordbasen", (-3.4, -26.0, 13.9), 90.0, SPAWN_N)]`
  (indeks 0 = rødt lag, 1 = blått, som `AGSGameMode::BasePointM` leser).
- `SPAWN_S = [(-23,22.2),(-18,22.2),(-23,24.4),(-18,24.4),(-23,27.8),(-18,27.8),(-23,30.4),(-18,30.4)]`,
  `SPAWN_N` = samme med (−x, −y). Z = 13,9. Byggeren kan flytte punkter innen eget spawnrom hvis sjekk c) krever det –
  da flyttes motpunktet likt.
- `ZONES` (for `GSCampus::Buildings()`; høyde 26,2, etasjehøyde 4,3, `bEnterable = false`):
  «Helgasetr sørfløy» X −25,75…+25,75, Y +20,3…+35,66; «Helgasetr nordfløy» Y −32,11…−20,3; «Helgasetr vestfløy»
  X −25,75…−15,4, Y −20,3…+20,3; «Helgasetr østfløy» X +15,4…+25,75, Y −20,3…+20,3; «Atriet» X −15,4…+15,4, Y −20,3…+20,3.
- `PLAY_AREA = (-26.5, -33.0, 26.5, 36.5)`.
- `CORRIDORS` (minikart via `Roads()`, bredde 2,3, P4): fire sider av ringen langs |X| = 14,4 og |Y| = 18,9.

#### 1e. Plasser til Spillbarhet (data nå, de tar dem i bruk etter bestilling)
- `LOOT`: ca. 60 punkter (x, y, z, nivå 0 vanlig / 1 bedre / 2 kasse). Fordeling: P1 16 (kantina, loungen, salatbaren,
  Ryddekroken, servicesenteret), P2 14 (mesaninen, grupperommene, studentarbeidsplassene, undervisningsrom 120), P3 14
  (glassboksene, rommene vest/nord/øst), P4 16 (laber, østfløyens rom, glassboksene – **ingen** i baserommene, spawnrommene
  eller lobbyene). Samme antall og nivå på vest-/øst- og nord-/sørsiden (sjekkes). To kasser (nivå 2): kantina (−8,0, −16,5, 0)
  og mesaninen (+4,0, +16,0, 5,6). Hvert punkt minst 0,6 m fra vegg og 2 m fra flagg, over gulv på riktig plan.
- `TERMINALS`: PC – servicesenteret (+10,0, −19,0, 0), studentarbeidsplassene (+10,0, +27,0, 5,6), undervisningsrom 100 pl
  nord P3 (−11,0, −30,0, 9,9), treningsleiligheten P4 (+24,5, 0,0, 13,9); printer – kopirom P3 vest (−17,0, +2,0, 9,9) og
  P2 øst møterom (+12,5, −11,0, 5,6). Navn på norsk, f.eks. «Servicesenteret, 1. etasje».
- `CAMERAS`: 8 stk. med navn og retning: kantina ×2, mesaninen, Trapp sør (P4 lobby), Trapp øst (P4 lobby), vestkorridoren
  P4, østkorridoren P4, klatrehallen.
- `SOUND_ZONES`: bokser med navn og RT60 fra helgasetr.md kap. 10.

#### 1f. Dekning og lys (råvolumer)
- `COVER`: enkle bokser som står der de ekte møblene skal stå, med riktig høyde: plantekasserad P1 (X −2…+6, Y −9,5…−8,8,
  h 0,9), Ryddekroken-tårnet (massivt, gult, med kiosknisje), salatbaren (X −10…−5, Y −10…−8, h 1,0), tre stålbur-båser (X
  +10…+13, Y −8…+3, h 1,8, gjennomsiktige sider = bare stolper), servicesenter-disken, skaprekker i korridorene P2–P4 (0,6
  dype, 2,0 høye, langs ytterveggen, 4–6 m lange, ikke foran dører), sittetrappa. Dette er spill-dekning; møblene kommer i
  runde 3–4 på samme plasser.
- `LIGHTS`: 8 store runde hengelamper over atriet (Ø 2,4, Z 7,5–12,0) og ett punktlys per ca. 10 m korridor på P1–P4 –
  til sammen ≤ 48, ingen skygge, `MaxDrawDistance` 4000 cm.

#### 1g. Glassmoduler, dørplasser og lettvegger (forberedt for runde 5)
- `GLASS_PANELS` (genereres av koden i `helgasetr_plan.py`, ikke skrives for hånd): glassrekkverk langs alle atriekanter og
  «åpent ned»-kanter delt i felt på maks 1,5 m (mellom søyler/stolper), boksenes glassvegger delt i felt på maks 1,5 m,
  grupperommenes glassvegger og glassdører. Hvert felt: (id, senter, lengde, høyde, yaw, type `rekkverk`/`vegg`/`dor`).
- `DOOR_SLOTS`: alle dørene fra ROOMS, med id. Runde 4 = åpen døråpning med karm; runde 5 setter dørblad.
- `BREACH_WALLS`: 4 per side, punktsymmetrisk: P4 vest – veggen mellom vestkorridoren og audiologi 4.056 (X −15,55,
  Y +5…+8) og 4.057 (Y −11…−9); P4 øst – motstykkene (X +15,55, Y −8…−5 og +9…+11, flytt så de treffer rom, ikke
  trapperommet); P3 nord og sør – mellom korridoren og undervisningsrommene (2 × 2). Hvert panel 2,4 × 2,8 m. Ikke i
  baserommene, ikke i trapperom, ikke i yttervegger.

### 2. Generator `Tools/Blender/gen_helgasetr.py`
Leser `helgasetr_plan.py` og skriver `Source/Gloshaugen/Public/GSHelgasetr.gen.inl` med en tydelig topplinje «GENERERT av
Tools/Blender/gen_helgasetr.py – ikke rediger». Innhold: `LevelZ[6]`, `PlayArea`, `Zones`, `Bases` (flagg, yaw, spawn),
`FloorRects` per nivå (gulvflater = FOOTPRINT minus VOIDS, delt i rektangler), `Loot`, `Terminals`, `Cameras`,
`SoundZones`, `Lights`, `GlassPanels`, `DoorSlots`, `BreachWalls`, `ModelParts` (navn på alle modellbitene og kollisjonsart).
Kjør: `python Tools/Blender/gen_helgasetr.py`. Med `--check` skriver den ingenting, men feiler hvis fila ikke er oppdatert.

### 3. `Source/Gloshaugen/Public/GSHelgasetr.h`
`namespace GSHelgasetr` med små structer (`FRectM`, `FSpotM {X,Y,Z; uint8 Tier; const TCHAR* Label;}`, `FBaseM`,
`FPanelM`, `FDoorM`, `FModelPartM {const TCHAR* Name; uint8 Collision;}`), `#include "GSHelgasetr.gen.inl"` og
hjelpefunksjoner: `int32 LevelAt(float ZMeters)` (nærmeste plan under, 0,3 m slakk), `FString LevelName(int32)`
(«1. etasje» …), `const TCHAR* ZoneAt(FVector2D)`. Ingen avhengighet til `GSCampus.h` (unngå sirkel).

### 4. `GSCampus.h` – bryter
Øverst i namespace: `inline constexpr bool bHelgasetr = true;` og `#include "GSHelgasetr.h"`. I `Buildings()`, `Roads()`,
`PlayArea()`, `Bases()`, `SpawnPoints()`: hvis `bHelgasetr`, returner en `static` liste bygd én gang fra `GSHelgasetr`
(lambda som konverterer). La de gamle Gløshaugen-listene stå urørt i samme funksjoner (`check_campus.py` leser dem
fortsatt). `RigFences()`/`Mirror()` urørt. Skriv i toppkommentaren at kartet nå er Helgasetr og hvor dataene kommer fra.
Kontroller: ingen andre filer bruker `Buildings()[i].bEnterable` til noe som krever at Helgasetr-sonene er «enterable»
(GameMode-loot og terminaler hopper da over dem – riktig til Spillbarhet leser de nye listene).

### 5. Blender: `Tools/Blender/building_helgasetr.py`
`blender --background --python Tools/Blender/building_helgasetr.py -- [--render]`. Bruker `MeshKit` (Unreal-rom, finish()
speiler Y) og `helgasetr_plan.py`. Lager og eksporterer til `SourceArt/Models/` (FBX + .blend):
- `SM_Bygg_Helgasetr_<Plan>_<Kvadrant>` for P1–P6, kvadrant NV/NØ/SV/SØ (delt ved X = 0, Y = 0; ingen bit over ca. 26 m –
  Lumen-avstandsfelt). Innhold: dekke (med hull, `MeshKit.slab_with_holes`), undersider/kanter (hvit gips, mørke trelameller
  på dekkekanten mot atriet), yttervegger med vindushull, innervegger (0,15 m) med døråpninger og karmer, søyler Ø 0,5 med
  to svarte ringer, trapperom (vegger 0,25), skaprekker og dekningsvolumer (`COVER`), boksenes gulv, tak (hvit underside
  0,45 m) og svarte stolper/rammer, rekkverkenes treoverkant og stolper (ikke glasset), heissjakter (lukket).
  Materialspor som i helgasetr.md kap. 8 (alt inne slutter på `_In`).
- `SM_Bygg_Helgasetr_Trapper`: atriumtrappa løp 1–4 og sosialtrappa (gul, ekte trinn), Trapp sør og Trapp øst (betong).
- `SM_Bygg_Helgasetr_Tak`: sagtakets rammer, hvitt Warren-fagverk (3 hovedfagverk øst–vest + takåser), hengestenger til
  boksene (tynne, ingen kollisjon trengs), flatt tak over fløyene.
- `SM_Glass_Helgasetr_Fasade`: vindusglass i yttervegger, glasstaket, klatrehallens glassvegg, vindfanget, glassporten på
  løp 4 og branndørene øverst/nederst i trapperommene (fast glass/dører som stenger kartet).
- `SM_Mod_Helg_GlassPanel` (1 × 1 m glassfelt, origo midt nede, front +X), `SM_Mod_Helg_GlassDoor` (glassdør 1,0 × 2,1 m
  med svart ramme, prikkebånd og kodelås-boks, åpen stilling ikke nødvendig ennå), `SM_Mod_Helg_Lettvegg` (gipspanel
  1 × 1 m, 0,10 m tykt), `SM_Mod_Helg_Karm` (dørkarm 1,0 m). Disse settes ut og skaleres av C++.
- `SM_Prop_Helg_Hengelampe` (ring Ø 2,4 m, 0,15 m høy, hvit underside som lyser – materialspor `LightPanel`).
- `--render`: ovenfra-snitt per plan P1–P4 (taket over fjernet), og tre perspektiver: fra P4 nordkorridor mot sør over
  atriet, fra mesaninen (P2) ned i kantina, fra P4 vestkorridoren. Lagre til `.claude/lag/grafikk/kontroll/r04-*.png` og
  sammenlign med bilde 01, 06, 22, 26 (kontrolløren ser på dem).

### 6. `init_unreal.py` og `gs_materials.py`
- `init_unreal.py`: legg `"SM_Glass_Helgasetr"` og `"SM_Mod_Helg_"` til `COMPLEX_COLLISION_PREFIXES` (ikke Nanite – de har
  gjennomsiktig materiale; `SM_Mod_Helg_Lettvegg`/`Karm` kan være Nanite, men hold det enkelt: ingen Nanite for `SM_Mod_`).
  Hvis Python-API-et tillater det, sett `distance_field_resolution_scale` = 2 for `SM_Bygg_Helgasetr_*`; ellers dropp det
  (oppdelingen i steg 5 er hovedgrepet) og si fra i rapporten.
- `gs_materials.py` `INSTANCES`: `"HelgYellow": ("PaintedMetal", (1.15, 0.62, 0.06), 2, 0.8, 0, "Surface")`,
  `"HelgOrange": ("Plastic", (1.1, 0.42, 0.08), 2, 1.0, 0, "Surface")`, `"PaintWhite": ("PaintedMetal", (1.1, 1.1, 1.08),
  2, 0.9, 0, "Surface")`. `_In`-variantene lages av seg selv.

### 7. C++: `AGSHelgasetrBuilder` (nye `GSHelgasetrBuilder.h/.cpp`)
- `GSCampusBuilder.h`: gjør `Build()` og `SetStreetLightsOn()` `virtual`. Ingen andre endringer der.
- `class AGSHelgasetrBuilder : public AGSCampusBuilder` (`UCLASS(NotPlaceable)`), overstyrer `Build()`:
  1. For hver `ModelParts`-oppføring: `LoadObject` fra `/Game/Models/<Name>`, lag `UStaticMeshComponent` (som
     `BuildBuilding` gjør for Hovedbygningen), kollisjon etter art: `Full` (BlockAll), `GlassFull` (BlockAll, `Mat_Glass`,
     ingen skygge), `None` (lamper, hengestenger).
  2. **Glassmoduler** (`GlassPanels`): én ISM per type (`rekkverk`, `vegg`, `dor`) med `SM_Mod_Helg_GlassPanel`/`GlassDoor`,
     én instans per panel i id-rekkefølge (instansindeks = panel-id, så runde 5 kan finne dem). Kollisjon: blokker `Pawn`
     og `PhysicsBody`, **ignorer `Visibility` og `Camera`** (kuler går gjennom til Kamp kobler knusing i runde 5), ingen
     skygge, `ComponentTags` `Mat_Glass`. Mangler modellen: tynn boks med `MI_GS_Glass`.
  3. **Lettvegg-paneler** (`BreachWalls`): ISM `SM_Mod_Helg_Lettvegg` (eller boks med `PlasterWhite_In`), BlockAll,
     instansindeks = id, tag `Mat_Plaster`. Strukturmodellen har hull der.
  4. **Dørplasser**: ingenting å spawne i denne runden (åpning + karm i strukturen). Logg antallet.
  5. **Lys**: hengelampene (prop + `URectLightComponent` ned, ca. 3 000 lm, ingen skygge) og korridorlysene
     (`UPointLightComponent`, ingen skygge, attenuation 900 cm), alle `MaxDrawDistance` 4000. Lys bare når ikke dedikert server.
  6. **Lagfarger** ved basene: rød/blå stripe (boks 1,2 × 0,04 × 0,25 m) over hver dør inn til egen base og egen lobby, og
     et skilt «SØR»/«NORD» på korridorveggen ved basedørene – så man ser hvilken base det er (Andreas: lett å kjenne igjen).
  7. **Utenfor**: bakke (boks 220 × 220 m, toppen Z −0,05, `W_Asphalt`) og fire enkle kulisseblokker for nabobyggene (vest
     MTFS/St. Olavs, sør, øst over Elgeseter gate, Høgskoleparken i NØ som en lav grønn flate med noen trær fra `ModelMesh`).
  8. **Reserve** hvis `SM_Bygg_Helgasetr_P4_NV` mangler: legg ut `FloorRects` for alle nivåer som bokser (`AddBoxMat`,
     Terrazzo/WoodLight) og skriv på skjermen i 20 s: «Helgasetr-modellen er ikke importert. Lukk og åpne Unreal på nytt.»
     (`GEngine->AddOnScreenDebugMessage`), og i loggen. Spillet skal aldri stå uten gulv under basene.
  9. `FlushPending()` og logg «Helgasetr: N biter, M glassfelt, K lys (x s)».
- `SetStreetLightsOn` overstyres til å gjøre ingenting (innelyset står på).
- `GSWorldSubsystem.cpp`: `GSCampus::bHelgasetr ? AGSHelgasetrBuilder::StaticClass() : AGSCampusBuilder::StaticClass()`;
  `Env->Campus` settes som før (typen er basisklassen).

### 8. `Tools/Blender/check_helgasetr.py` (ren Python) – med motprøver
Leser `helgasetr_plan.py` (import) og kjører `gen_helgasetr.py --check`. Én linje OK/FEIL per sjekk, exit 1 ved feil.
a) Aksesummer 51,50 og 67,77 (±0,01), etasjehøyder 5,6/4,3/4,0/4,3/4,0/4,0. *Motprøve:* P2 = 5,5 → må feile.
b) Basene på samme Z, i hver sin ende (nordrommet går til Y ≤ −30, sørrommet til Y ≥ +29), |ΔY| flagg ≥ 45 m; flagg ≥ 2 m fra
   vegger; tre dører hver; flagg og dører punktsymmetriske om origo (±0,1). *Motprøve:* flytt nordflagget 1 m → må feile.
c) Spawn: inne i eget spawnrom, ≥ 0,6 m fra vegg, ≥ 1,5 m mellom; **ingen spawn synlig** (2D, vegger blokkerer, dører og glass
   gjør ikke) fra noen punkt hver 0,5 m på ringkorridorens midtlinje, lobbyenes åpninger mot korridoren, trapperommenes dører
   og R2/R4-romrutene. *Motprøve:* et spawnpunkt midt i døra (−15,4, +23,0) → må feile.
d) Ruter: graf av rom/korridorer/trapper (noder) og dører/åpninger/trappeløp (kanter, lengde = avstand mellom
   midtpunktene). Fra hver base finnes minst 4 veier til den andre som ikke deler samme korridorside på P4 (R1, R2, R3, R4) og
   minst én som går via P1. Korteste vei fra eget spawn til motstanderens flagg: forskjell mellom lagene ≤ 20 %.
   *Motprøve:* fjern alle dører i vestfløyen og vestkorridoren på P4 → må feile.
e) Siktlinjer: langs hver rute-midtlinje, hver 0,5 m, fri 2D-sikt i gangretningen og 1,0 m til hver side ≤ 42 m (vegger og
   `COVER` blokkerer, glass gjør ikke). *Motprøve:* fjern veggen mellom Sør-spawngangen og ringen (Y +20,3) → må feile (50 m).
f) Alle LOOT/TERMINALS/CAMERAS/spawn/flagg ligger på et gulv (`FloorRects`) på sitt nivå, ikke i hull, trapper eller lukkede
   rom; loot ikke i base-/spawnrom; lik fordeling nord/sør og vest/øst (antall og nivå). *Motprøve:* et lootpunkt midt i
   atriet på P4 → må feile.
g) `BREACH_WALLS` ligger ikke i bærende vegger, trapperom eller baserom, og er punktsymmetriske. *Motprøve:* et panel i
   Sørbasens korridorvegg → må feile.
h) Hele `FOOTPRINT` ligger inne i `PLAY_AREA`.

### 9. `Tools/Blender/check_helgasetr_walk.py` (Blender, leser .blend-filene) – med motprøver
Som `check_walkable.py` (stråler i Unreal-koordinater; husk speilingen av Y i Blender):
- Alle dører i `DOOR_SLOTS`: vannrett stråle gjennom senteret i 1,0 og 1,9 m høyde går fri; *motprøve* 0,7 m til siden av en
  dør med vegg → må treffe.
- Trinn: for hvert trappeløp, loddrette stråler hver 0,1 m langs midtlinja: ingen sprang > 0,20 m, fri høyde ≥ 2,2 m.
  *Motprøve:* en stråle over trapperommets vegg → treff.
- Under hvert flagg, spawnpunkt og lootpunkt: stråle ned fra Z + 2,0 treffer gulvet innen 0,05 m fra nivåets Z, og en stråle
  **opp** fra Z + 0,2 til Z + 4,0 treffer **ingenting med kollisjon** (GameMode-sporet `SpawnFlags` går fra +3 m og
  `PickSpawnTransform` fra +4 m ned – en himling med kollisjon ville gjort at flagg/spillere havner oppå den).
  Himlinger og lamper har derfor aldri kollisjon. *Motprøve:* punkt under et dekke 3 m over → må treffe.
- Rekkverk: vannrette stråler 1,0 m over gulvet, utover atriekanten hver 0,5 m rundt hele kanten på P2–P4 (og «åpent ned»):
  hver stråle skal treffe en glassmodul (fra `GLASS_PANELS`, regnes i Python) eller struktur innen 0,3 m. *Motprøve:* ett
  felt tatt bort → hull.

### 10. Bygg og prøv
1. `python Tools/Blender/gen_helgasetr.py` → `python Tools/Blender/check_helgasetr.py` (alt OK).
2. `blender --background --python Tools/Blender/building_helgasetr.py -- --render` →
   `blender --background --python Tools/Blender/check_helgasetr_walk.py` (alt OK). Se på bildene selv.
3. Bygg C++ med `Build.bat … -WaitMutex` (editoren lukket; se CLAUDE.md).
4. Be Andreas åpne Unreal (importen skjer da), trykke Play og: gå fra rød spawn (sørvest, 4. etasje) til blått flagg langs
   vestkorridoren, gjennom laberadene, ned Trapp sør til kantina, opp atriumtrappa (tre løp) og tilbake; se opp mot glasstaket.

## Ferdig når
- `check_helgasetr.py` og `check_helgasetr_walk.py` gir bare OK, og alle motprøvene feiler som de skal.
- `gen_helgasetr.py --check` er grønn (den genererte fila er oppdatert).
- `Build.bat` uten feil (eller «Target is up to date» etter byggerens eget bygg).
- Rendringene viser: atriet gjennom seks plan med glasstak og fagverk, gule trappeløp som krysser atriet, glassbokser stablet
  på P3/P4, ringkorridor med søyler, Plan 1-gulvet i lys terrazzo, mesaninen i oransje.
- I spillet (Andreas): man spawner i riktig rom på Plan 4, flaggene står i baserommene, man kan gå alle rutene uten å sette
  seg fast, og regnet stopper under glasstaket. Loggen sier antall biter og lys, og ingen «mangler»-melding.

## Risiko
- **Importen** skjer bare når editoren åpnes; første gang tar det tid (Nanite + kollisjon på mange biter). Reserven (steg 7.8)
  gjør at spillet aldri er uten gulv. Byggeren må teste at `_needs_import` plukker opp nye FBX.
- **Kollisjon «complex as simple»** på store biter: trinn og karmer kan gi hakking i bevegelsen; hold trinnene rene (én
  boks per trinn) og fjern små kanter i gangsonen.
- **Lumen-lekkasjer** gjennom tynne vegger eller for store biter: vegger ≥ 0,15 m, dekker 0,35 m, bitene delt i kvadranter.
- **Glass som ignorerer `Visibility`**: kuler går gjennom alt glass (riktig nå), men også gjennom glasstaket? Nei – taket er i
  `SM_Glass_Helgasetr_Fasade` med `GlassFull` (BlockAll), så innendørssjekken og regnet virker. Ikke bland de to.
- **Andre lag:** loot, PC-er og kameraer forsvinner til Spillbarhet leser `GSHelgasetr::LootSpots()`/`Terminals()`
  (bestilt). Den mytiske kassa (fast punkt (0, −7,5, 0,85) i GameMode) havner i kantina, 0,85 m over gulvet – bestilt flyttet.
  HUD-kartet på PC-ene bruker `PlayArea()` + 40 m (for mye luft rundt et lite bygg) – bestilt hos Kamp. Ingenting stopper
  spillet.
- **Skjevheten med Trapp sør** (helgasetr.md kap. 5): sjekk d) måler den; justeres med dører, ikke med nye vegger.
- **Mye i én runde:** hvis tiden ikke strekker til, kutt i denne rekkefølgen (og si det i rapporten): P5–P6-kulissen
  (bare dekke + rekkverk), kulisseblokkene ute, lampene (bare punktlys). Ikke kutt sjekkene, basene, trappene eller glasset.

## IMPROVEMENTS-linjer (549–564, egen seksjon «Runde 4 (Grafikk og kart): Helgasetr, råbygget»)
549. Nytt kart: Capture the Flag spilles nå inne i Helgasetr (NTNU, Elgeseter gate 10); campuskartet bygges ikke lenger
550. Helgasetr har målene fra arkitektens tegninger: 51,5 × 67,8 m, søyleaksene 1–10 og A–N, og etasjehøyder på 5,6, 4,3 og 4,0 m
551. Atriet går gjennom alle seks etasjene opp til et sagtak av glass med hvitt stålfagverk, og dagslyset kommer inn ovenfra
552. Ringkorridor rundt atriet i 3. og 4. etasje (og som U i 2. etasje) med betongsøyler og glassrekkverk med treoverkant
553. Kantina i 1. etasje: lyst terrazzogulv, Ryddekroken-tårnet, salatbaren, plantekassene og båsene står som dekning der de står i virkeligheten
554. Mesaninen i 2. etasje med oransje gulv, sittetrapp og tre grupperom i glass, og studentarbeidsplassene med dobbel takhøyde
555. Atriumtrappa i tre gule løp med ekte trinn: langs vestsiden, tvers over atriet i lufta og langs østsiden
556. Trapp sør og Trapp øst er betongtrapperom fra 1. til 4. etasje, med dører i hver etasje
557. Glassboksene – grupperommene som henger ut i atriet – i 3. og 4. etasje, stablet oppå hverandre med hvitt undertak
558. Sørbasen (rødt lag) i undervisningsrommet i sørenden av 4. etasje og Nordbasen (blått lag) i de to undervisningsrommene i nordenden, med tre innganger hver
559. Spawn i egne grupperom bak hver base, med to utganger og ute av syne fra korridorene og trappene
560. Seks veier mellom basene: vest- og østkorridoren, laberadene og østfløyens rom, gjennom atriet via trappene, og ned gjennom kantina
561. Røde og blå striper over dørene og skilt «SØR»/«NORD» viser hvilken base du er ved
562. 5. og 6. etasje står ferdige rundt atriet, men trappene opp er sperret med glassdør og kortleser
563. Glass i rekkverk og grupperom er egne felt man ikke går gjennom men skyter gjennom, klare til å bli knust i neste runde
564. To nye sjekkskript for Helgasetr: mål mot tegningene, baser, spawn som ikke synes, ruter, siktlinjer, åpne dører, trinn og rekkverk – med motprøver
