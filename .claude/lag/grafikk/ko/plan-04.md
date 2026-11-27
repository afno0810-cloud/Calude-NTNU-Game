# Plan 04a (Grafikk og kart): Helgasetr – spillbart klossbygg fra data (lagets runde 4a)

Les først `.claude/lag/grafikk/helgasetr.md` (fasiten) og kontrollørens forhåndssjekk
`.claude/lag/grafikk/kontroll/forhand-plan-04.md` (punktnumrene «K1…K24» under viser dit). Runde 4 er delt:
**4a (denne)** = data, C++-klossbygg av hele bygget og datasjekken – spillbart samme dag uten import i editoren.
**4b (`ko/plan-04b.md`)** = Blender-modellen som erstatter klossene bit for bit, walk-sjekken og rendringene.

Koordinater i meter: X = øst, Y = sør, Z = opp, origo = akse 5 × akse H på gulvet i Plan 1 (helgasetr.md kap. 1).

**Si til Andreas:** runde 4 gir CTF i Helgasetr **med bare startpistolen**. Loot, PC-er og kameraer kommer når
Spillbarhet leser de nye listene (bestilt, bør komme rett etter 4a).

## Mål
Andreas trykker Play og står i Helgasetr: et riktig skalert bygg av Plan 1–4 rundt det store atriet, med trappene,
glassboksene og de to basene på Plan 4 i hver sin ende, og han kan gå fra base til base på flere måter.

## Forbedrer (gammelt)
- Kartdataene i `GSCampus.h` (`Bases()`, `SpawnPoints()`, `PlayArea()`, `Buildings()`, `Roads()`) som GameMode, HUD,
  menyen, hjelpen og ballistikken leser: nå fylt med Helgasetr, så CTF, spawn, flagg, «Flagget sist sett» og minikartet
  virker uten endringer hos de andre lagene.
- Etasjenavnene i «Flagget sist sett» blir riktige i et bygg med ulike etasjehøyder (sonene får 4,3 m).
- Regn og snø stopper under glasstaket (dagens innendørssjekk i `GSEnvironment` treffer taket).

## Nytt
- Helgasetr som kart: felles datakilde, generert C++-data, `AGSHelgasetrBuilder`, baser, spawn i sluser, ruter, sjekkskript.
- Glassfelt, lettvegger og dekningsklosser er egne instanser med id fra første dag, klare for knusing i runde 5.

## Filer
Nye: `Tools/Blender/helgasetr_plan.py`, `Tools/Blender/gen_helgasetr.py`, `Tools/Blender/check_helgasetr.py`,
`Source/Gloshaugen/Public/GSHelgasetr.h`, `Source/Gloshaugen/Public/GSHelgasetr.gen.inl` (generert),
`Source/Gloshaugen/Public/GSHelgasetrBuilder.h`, `Source/Gloshaugen/Private/GSHelgasetrBuilder.cpp`.
Endres: `GSCampus.h` (bryter + Helgasetr ut), `GSCampusBuilder.h` (`Build`, `SetStreetLightsOn` virtuelle),
`GSWorldSubsystem.cpp` (riktig bygger), `IMPROVEMENTS.md` (549–564). Ingen Python i `Content/`, ingen Blender, ingen
andre lags filer, ingenting slettes.

## Steg

### 1. Datakilden `Tools/Blender/helgasetr_plan.py` (ren Python, ingen bpy)
Alt i meter, i spillets koordinater, med akser som navn (sporbart til tegningen):
```python
AX = {"1": -25.75, "2": -15.40, "3": -13.54, "4": -6.77, "5": 0.0, "6": 6.77, "7": 13.54, "8": 15.40, "9": 19.20, "10": 25.75}
AY = {"N": -32.11, "M": -26.45, "L": -20.30, "K": -18.03, "J": -12.02, "I": -6.01, "H": 0.0,
      "G": 6.01, "F": 12.02, "E": 18.03, "D": 20.30, "C": 23.85, "B": 29.80, "A": 35.66}
LEVELS = {"P1": 0.0, "P2": 5.6, "P3": 9.9, "P4": 13.9, "P5": 18.2, "P6": 22.2}
ROOF_Z, RIDGE_Z, SLAB, WALL_OUT, WALL_IN, CORE_WALL = 26.2, 29.0, 0.35, 0.40, 0.15, 0.25
RING_IN = (13.25, 17.75)     # atriekanten |X|, |Y|, hjørneradius 1,5 (i klosser: avfaset hjørne)
RING_OUT = (15.55, 20.05)    # korridorveggen -> fri bredde 2,3 m
```
Lister (små dataklasser eller navngitte tupler):
- `FOOTPRINT[level]`, `VOIDS[level]`, `ROOMS[level]` (navn, x0, x1, y0, y1, type `rom`/`lukket`/`korridor`/`lobby`/`trapp`/
  `apen`, dører (x, y, bredde, art `tre`/`glass_kode`/`dobbel`/`brann`/`laast`)), `STAIRS`, `BOXES[level]`, `COLUMNS`
  (søyler Ø 0,5 på akse 3 og 7 ved hver bokstavakse E–K, og der tegningen ellers har dem), `BASES`, `SPAWNS`, `ZONES`,
  `LOOT`, `TERMINALS`, `CAMERAS`, `SOUND_ZONES`, `COVER`, `SCREENS` (L-skjermer), `LIGHTS`, `BREACH_WALLS`,
  `ATRIUM_BLOCKERS` (K10).
- `FLOOR_RECTS[level]` = FOOTPRINT − VOIDS **+ boksegulv + trapperepos** (K4), delt i rektangler.
- `RAIL_EDGES[level]` = hele grensen mellom `FLOOR_RECTS` og hull (K5) – også rundt åpne fellessoner, repos og «åpent ned» –
  minus der en boksvegg, trappeløp eller vegg står. Glassrekkverk 1,1 m genereres langs disse kantene.
- `GLASS_PANELS` (generert): rekkverk og boksvegger delt i felt ≤ 1,5 m, glassdører; (id, senter, lengde, høyde, tykkelse
  0,06, yaw, type).
- `DOOR_SLOTS` (generert fra ROOMS, med id).

Innholdet står i helgasetr.md kap. 2–5 (**rettet etter forhåndssjekken**: heiser, lobbyer, Trapp sør, baser, spawn).
Det viktigste for P4 i tabell 1a. P1–P3 som i helgasetr.md kap. 3; rom som ikke trengs til rutene er `lukket`.
P5–P6: bare dekke med hull, rekkverk og `ATRIUM_BLOCKERS` (kulisse).

#### 1a. Plan 4 (Z 13,9) – rom og dører
| Navn | X | Y | Type | Dører / åpninger |
|---|---|---|---|---|
| Sørbasen | −6,77…+13,54 | +20,05…+29,8 | rom | (+10,0, +20,05, 1,0, tre), (−3,0, +20,05, 1,0, tre); **ingen sidedør** (K9). L-skjerm 1,6 × 2,1 m 1,5 m innenfor hver dør (oppslagstavle/knaggrekke, ugjennomsiktig) (K6) |
| Nordbasen | −13,54…+6,77 | −32,11…−20,05 | rom | (−10,0, −20,05, 1,0, tre), (+3,0, −20,05, 1,0, tre); L-skjermer som i sør |
| Sør-lobby | −13,54…−10,2 | +20,05…+26,9 | lobby | åpning 3,2 m mot ringen X −13,4…−10,2 ved Y +20,05; heis sør (lukket boks) X −10,2…−6,6, Y +20,4…+23,8 |
| WC-blokk | −10,2…−6,77 | +23,8…+26,9 | lukket | – |
| Trapp sør | −13,54…−6,77 | +26,9…+35,66 | trapp | dør fra lobbyen (−12,0, +26,9, 1,2) **laast på P4** (alarmdør; åpen på P1–P3) (K9) |
| Sør-spawngang | −15,4…−13,54 | +20,05…+32,11 | korridor | vegg mot ringen ved Y +20,05; dør til lobbyen i lobbyens vestvegg (−13,54, +25,9, 1,2, tre) = sluse med L-sving (K8) |
| S1 grupperom 4.041 | −25,75…−15,4 | +20,3…+26,0 | rom | (−15,4, +23,0, 1,0, tre); (−24,6, +20,3, 1,0, tre) til venterommet + L-skjerm 1,5 m innenfor |
| S2 grupperom 4.037 | −25,75…−15,4 | +26,0…+32,11 | rom | (−15,4, +29,0, 1,0, tre) |
| S3 garderobe | −25,75…−13,54 | +32,11…+35,66 | lukket | – |
| Venterom/mottak | −25,75…−15,4 | +14,0…+20,3 | korridor | åpen mot vestkorridoren (X −15,55, Y +14,0…+20,05) |
| Audiologi 4.056 | −25,75…−15,4 | +4,0…+14,0 | rom | (−20,5, +14,0), (−15,55, +9,0), (−20,5, +4,0) |
| Psykoakustisk lab | −25,75…−15,4 | −3,0…+4,0 | rom | (−20,5, −3,0) |
| Audiologi 4.057 | −25,75…−15,4 | −12,02…−3,0 | rom | (−15,55, −8,0) |
| Nord-lobby | +10,2…+13,54 | −26,9…−20,05 | lobby | åpning 3,2 m X +10,2…+13,4 ved Y −20,05 |
| Grupperom 4.022 | +6,77…+10,2 | −32,11…−20,05 | lukket | – |
| Nordrom bak lobbyen | +10,2…+13,54 | −32,11…−26,9 | lukket | – |
| Nord-spawngang | +13,54…+15,4 | −32,11…−20,05 | korridor | vegg mot ringen; dør til lobbyen (+13,54, −25,9, 1,2) = sluse |
| N1 grupperom 4.035 | +15,4…+25,75 | −26,0…−20,3 | rom | (+15,4, −23,0); (+24,6, −20,3) til NØ-gangen + L-skjerm |
| N2 grupperom 4.021 | +15,4…+25,75 | −32,11…−26,0 | rom | (+15,4, −29,0) |
| NØ-gang | +15,4…+25,75 | −20,3…−12,5 | korridor | åpen mot østkorridoren (X +15,55, Y −20,05…−14,0) |
| Heis øst | +15,6…+18,2 | −11,9…−9,9 | lukket | – |
| Trapp øst-lobby | +15,6…+18,5 | −9,9…−7,5 | lobby | åpen mot østkorridoren |
| Trapp øst | +18,5…+25,75 | −12,5…−7,5 | trapp | dør (+18,5, −8,7, 1,2, brann) – åpen på P1–P4 |
| Garderober | +15,4…+25,75 | −7,5…−3,0 | lukket | – |
| Treningsleiligheten | +15,4…+25,75 | −3,0…+4,0 | rom | (+15,55, 0,0), (+20,5, +4,0) |
| Ferdighetsrom vernepleier | +15,4…+25,75 | +4,0…+12,0 | rom | (+15,55, +8,0), (+20,5, +12,0) |
| Ferdighetsrom radiografi | +15,4…+25,75 | +12,0…+20,3 | rom | (+15,55, +16,0) |
| Undervisningsrom SØ | +13,54…+25,75 | +20,3…+35,66 | lukket | – |
Dører 1,0 m brede, 2,1 m høye om ikke annet står. Glassboksene og deres veggtype: se steg 1b.

**Disse tallene er et utgangspunkt.** Sjekk c/d2/e2/e3 bestemmer. Byggeren kan flytte dører, L-skjermer, spawnpunkter og
COVER innen egne rom, og flytte begge sider likt (punktsymmetri), men ikke legge inn vegger tegningen ikke har (bruk
skjermer, skap, sofagrupper, miljøstasjoner) og ikke flytte basene ut av rommene sine. Si i rapporten hva som ble flyttet.

#### 1b. Glassboksene (K6, K7)
Som helgasetr.md kap. 4. **Veggen mot korridoren og skilleveggene mellom boksene er lettvegg** (ugjennomsiktig, hvit, med
whiteboard – slik bilde 21 viser), glassdøra sitter i lettveggen; veggene mot atriet er glass. Boksene har gulv 0,3 m, tak
0,45 m hvitt, svarte stolper. Åpne fellessoner har rekkverk langs alle kanter mot hullet. Miljøstasjonen i P4 NØ-fellessonen
er en tett blokk (skap) 2,0 m høy mot korridoren.

#### 1c. Trapper (ekte trinn, én boks per trinn)
| Trapp | Plassering | Detaljer |
|---|---|---|
| Løp 1 (P1→P2) | X −12,9…−9,9; bunn Y −6,5 (Z 0) → topp Y +5,6 (Z 5,6) | 31 × 0,18/0,30, repos 2,5 m; vangene 1,1 m over trinnkant; ingen sprekk mot dekkekanten (fyll 0,35 m-spalten) (K11) |
| Sosialtrappa (P1→P2) | X −9,6…−6,0; Y −5,0 → +5,6 | sittetrinn 0,45/0,90 (vest) + vanlig trapp 1,2 m (øst) |
| Løp 2 (P2→P3) | Y +6,2…+8,6; X −1,0 (Z 5,6) → +7,5 (Z 9,9) | 24 trinn, repos 1,2 m; sittetrappa på mesaninen flyttes til X +8…+12,5 så den ikke står under løpet |
| Løp 3 (P3→P4) | X +10,0…+13,0; Y +6,0 (Z 9,9) → −1,8 (Z 13,9) | 22 trinn, repos 1,2 m |
| Trapp sør | X −13,54…−6,77, Y +26,9…+35,66 | to løp per etasje langs Y; P1–P3 åpen, døra på P4 låst; over P4/under P1 stengt |
| Trapp øst | X +18,5…+25,75, Y −12,5…−7,5 | to løp per etasje langs X; P1–P4 åpen; over P4/under P1 stengt |
Opptrinn ≤ 0,20 m, fri høyde over trinn ≥ 2,2 m. **Alle soner under trapper med fri høyde < 2,0 m fylles med et
massivt volum** (vange, benk, plantekasse, teknisk rom) (K11). Løp 4 (P4→P5) og de gule løpene videre opp bygges i 4b som
kulisse.

#### 1d. Baser, spawn, soner, plasser
- `BASES = [("Sor", "Sørbasen", (3.4, 26.0, 13.9), -90.0, SPAWN_S), ("Nord", "Nordbasen", (-3.4, -26.0, 13.9), 90.0, SPAWN_N)]`
  (indeks 0 = rødt, 1 = blått). **I `GSCampus::Bases()` settes `Flag.Z = 13.4`** (0,5 m under gulvet) til Spillbarhet har
  flyttet spawnsporet til +2 m (bestilt) (K1): `PickSpawnTransform` sporer fra `Base.Z + 4 m` = 17,4 m (under P5-dekket som
  starter på 17,85), `SpawnFlags` fra +3 m = 16,4 m, og reserven `Base + 1,2 m` blir 14,6 m. Skriv det i kommentaren.
- `SPAWN_S` (Z 13,9): (−23,0, +22,2), (−18,5, +22,2), (−23,0, +24,6), (−18,5, +24,6), (−23,0, +27,6), (−18,5, +27,6),
  (−23,0, +30,6), (−18,5, +30,6). `SPAWN_N` = samme med (−x, −y). Ikke i linje med noen dør (sjekk c).
- `ZONES`, `PLAY_AREA = (-26.5, -33.0, 26.5, 36.5)`, `CORRIDORS` (ringens fire sider på P4, bredde 2,3) – som helgasetr.md kap. 5.
- `LOOT` (ca. 60, `Tier` 0/1/2), `TERMINALS` (4 PC, 2 printere), `CAMERAS` (8), `SOUND_ZONES` – som helgasetr.md kap. 5 og
  10; ingen loot i base-, spawnrom, lobbyer eller sluser; lik fordeling nord/sør og vest/øst.
- `COVER` (egne ISM-klosser med id, **ikke** del av strukturen, K21): plantekasseraden og salatbaren på P1,
  stålbur-båsene (stolper), servicesenter-disken, sittetrappa (struktur), skaprekker og sofagrupper i korridorene og ved
  ringhjørnene på atriesiden, slik at sjekk e3 går gjennom. Ryddekroken-tårnet er struktur (massivt).
- `BREACH_WALLS`: 4 per side, punktsymmetriske: P4 vest (X −15,55; Y +5…+7,4 og −11…−8,6), P4 øst motstykker (X +15,55;
  Y −7,4…−5 → flytt til nærmeste rom som ikke er trapperom/garderobe, f.eks. Y −3…−0,6, og motstykket i vest likt), P3
  nord og sør mellom korridoren og undervisningsrommene. 2,4 × 2,8 m. Aldri i baserom, trapperom, yttervegger.
- `LIGHTS`: 8 store runde hengelamper (Ø 2,4 m, Z 7,5–12) og korridorlys **≤ 6 m demping**, ett per ca. 8 m, ikke nærmere
  basevegger enn 3 m; alle uten skygge, `MaxDrawDistance` 4000 cm; maks 48 (K20).
- `ATRIUM_BLOCKERS` (K10): én usynlig vegg i full høyde (fra P5-gulvet til glasstaket) langs atriekanten på P5 og P6.

### 2. Generator `Tools/Blender/gen_helgasetr.py`
Leser `helgasetr_plan.py` og skriver `Source/Gloshaugen/Public/GSHelgasetr.gen.inl` (topplinje «GENERERT av
Tools/Blender/gen_helgasetr.py – ikke rediger»). **Bare `inline constexpr`-tabeller** (eller funksjoner med `static`
inni), aldri vanlige globale variabler – fila tas med overalt der `GSCampus.h` er med (K16). Innhold: `LevelZ`, `PlayArea`,
`Zones`, `Bases`, `Spawns`, `FloorRects` (per nivå), `Walls` (ferdig oppdelte veggstykker med åpninger, per nivå: senter,
lengde, høyde, tykkelse, yaw, materialnavn), `Stairs` (én post per trinn og repos), `Columns`, `Boxes`, `GlassPanels`,
`BreachWalls`, `Cover`, `Screens`, `DoorSlots`, `Loot`, `Terminals`, `Cameras`, `SoundZones`, `Lights`, `AtriumBlockers`,
`Roof` (glassflater og fagverksbjelker som bokser). Med `--check` skriver den ingenting, men feiler hvis fila er utdatert.
Kjør: `python Tools/Blender/gen_helgasetr.py`.

### 3. `Source/Gloshaugen/Public/GSHelgasetr.h`
`namespace GSHelgasetr`: små `struct`-er (`FRectM`, `FBoxM {Cx,Cy,Cz,Sx,Sy,Sz,Yaw; const TCHAR* Mat;}`, `FSpotM`, `FBaseM`,
`FPanelM`, `FDoorM`), `#include "GSHelgasetr.gen.inl"`, og `inline`-hjelpere `LevelAt(float ZM)` (nærmeste plan under,
0,3 m slakk), `LevelName(int32)` («1. etasje» …), `ZoneAt(FVector2D)`. Ingen include av `GSCampus.h` (unngå sirkel).

### 4. `GSCampus.h` – bryter
`inline constexpr bool bHelgasetr = true;` og `#include "GSHelgasetr.h"`. I `Buildings()`, `Roads()`, `PlayArea()`,
`Bases()` (med `Flag.Z = 13.4`, se 1d), `SpawnPoints()`: hvis `bHelgasetr`, returner en `static` liste bygd én gang fra
`GSHelgasetr`. De gamle Gløshaugen-listene står urørt (`check_campus.py` leser dem). Oppdater toppkommentaren.

### 5. C++: `AGSHelgasetrBuilder`
- `GSCampusBuilder.h`: `Build()` og `SetStreetLightsOn()` blir `virtual`. Ingenting annet.
- `class AGSHelgasetrBuilder : public AGSCampusBuilder`, overstyrer `Build()`:
  1. **Struktur som klosser** (`AddBoxMat`, delte ISM-er er greit her): `FloorRects` som dekker (0,35 m, toppen på nivåets Z;
     terrazzo P1, `PaintOrange_In`-tone på mesaninen – bruk eksisterende `PaintOrange` til 4b lager `HelgOrange`, parkett
     `WoodLight` i korridorene, `Floor` i grupperom), `Walls` (gips `PlasterWhite`, betong `ConcreteSmooth` i trapperom,
     yttervegger `ConcreteSmooth` med vindushull fylt av fast glass), `Columns`, `Stairs` (ett trinn = én boks, gul
     `PaintYellow` på atriumtrappa, betong i trapperommene), boksenes gulv/tak/stolper, Ryddekroken-tårnet, rekkverkets
     treoverkant (`Wood`) og stolper, `Screens`, glasstaket (flate glassbokser på 26,2–29,0 i sagtakform, se helgasetr.md
     kap. 7) og fagverket (hvite bokser `PlasterWhite`), bakke ute (Z −0,05, `W_Asphalt`). Himlinger lages ikke i 4a.
  2. **Glassfelt** (`GlassPanels`): **egne** `UInstancedStaticMeshComponent` per type (rekkverk/vegg/dør), **ikke** via
     `GetISM` (K14), `GSUtils::Cube()` skalert til feltet (tykkelse 0,06 m) med `MI_GS_Glass`. Enkel bokskollisjon (kuben har
     det) (K15). Kollisjon: blokker `Pawn`, `PhysicsBody`, `WorldDynamic`; **ignorer `Visibility` og `Camera`** (kuler går
     gjennom til runde 5). `CastShadow = false`, `bAffectDistanceFieldLighting = false`, `bVisibleInRayTracing = false`
     (K19). `ComponentTags` `Mat_Glass`. Instansindeks = rekkefølge; fyll `TMap<int32, TPair<ISM*, int32>> PanelById`.
     Instanser fjernes aldri (runde 5 skalerer til 0).
  3. **Lettvegger** (`BreachWalls`) og **dekning** (`Cover`): egne ISM-er på samme måte, BlockAll, egne id-tabeller, tag
     `Mat_Plaster` for lettveggene.
  4. **Fast glass** (vinduer, glasstak, klatrehallens glassvegg, vindfang, låste branndører/glassporter): egen ISM, BlockAll
     (også `Visibility`, så regnet stopper ved taket), uten skygge/DF/strålesporing, `Mat_Glass`.
  5. **Over P4 i atriet** (K10): P4-boksenes tak, P5/P6-boksene, fagverk, hengestenger og trappeløp over P4 får kollisjon som
     blokkerer `Visibility` men **ignorerer `Pawn` og `WorldStatic`** (gripekroken finner ikke feste, ingen kan stå der).
     `AtriumBlockers`: usynlige bokser (ingen rendering) som blokkerer `Pawn` og `WorldStatic`. Glasstaket: BlockAll.
  6. **Lys**: `LIGHTS` (hengelamper som en hvit lysende ring av klosser med `LightPanel` + `URectLightComponent` ned, ca.
     3 000 lm; korridorlys `UPointLightComponent` `AttenuationRadius` ≤ 600), ingen skygge, `MaxDrawDistance` 4000, ikke på
     dedikert server.
  7. **Lagfarger**: rød/blå stripe over de to basedørene og ved lobbyåpningene, og skilt «SØR»/«NORD» på korridorveggen.
  8. **Sporprøve** (logg, ingen spillendring): etter `FlushPending()`, for hver base kjør nøyaktig GameMode-sporene –
     `LineTraceSingleByObjectType(WorldStatic)` fra `Flag + 3 m` til `Flag − 3 m`, og for hvert spawnpunkt
     `LineTraceSingleByChannel(ECC_WorldStatic)` fra `(x, y, Flag.Z) + 4 m` til `−3 m` + `OverlapBlockingTestByChannel` med
     kapsel 42/92 på treffet + 1 m – og logg «Helgasetr: flagg 2/2 og spawn 16/16 står riktig» eller hvilke som feiler.
  9. Logg antall klosser, glassfelt, lys og tid.
- `SetStreetLightsOn`: gjør ingenting.
- `GSWorldSubsystem.cpp`: `GSCampus::bHelgasetr ? AGSHelgasetrBuilder::StaticClass() : AGSCampusBuilder::StaticClass()`.
- Klossbygget er også reserven i 4b: hold koden slik at hver struktur-del kan hoppes over når 4b-modellen for den biten finnes
  (en `bool` per bit, nøkkel = bitnavn fra `gen.inl`, K13).

### 6. `Tools/Blender/check_helgasetr.py` (ren Python) – med motprøver
Leser `helgasetr_plan.py` og kjører `gen_helgasetr.py --check`. OK/FEIL per linje, exit 1 ved feil. Hver sjekk har en
motprøve (endret kopi av dataene i minnet) som **må** feile.
- a) Aksesummer 51,50/67,77, etasjehøyder. *Motprøve:* P2 = 5,5.
- b) Basene på samme Z, i hver sin ende (nord til Y ≤ −30, sør til Y ≥ +29), |ΔY| ≥ 45 m, flagg ≥ 2 m fra vegg, to
  korridordører hver, punktsymmetri (±0,1). *Motprøve:* nordflagget 1 m til siden.
- c) Spawn inne i eget rom, ≥ 0,6 m fra vegg, ≥ 1,5 m mellom; **ingen spawn synlig** (2D; vegger, skjermer og lettvegger
  blokkerer, åpne dører og glass gjør ikke) fra noe punkt hver 0,5 m på ringkorridoren, lobbyåpningene, venterommet/NØ-gangen,
  trapperomsdørene og R2/R4. *Motprøve:* spawnpunkt (−18,5, +23,0) (i linje med S1-døra).
- d) Rutegraf (rom/korridorer/trapper = noder, dører/åpninger/trappeløp = kanter): ≥ 4 veier base↔base som ikke deler
  korridorside på P4, ≥ 1 via P1; korteste vei spawn → fiendens flagg lik for begge lag ±20 %. *Motprøve:* alle dører i
  vestfløyen og vestkorridoren på P4 fjernet.
- **d2)** (K9) For hver trapperomsutgang og hvert trappeløp som lander på P4: vei til fiendens flagg for laget som angriper
  derfra – lik for begge lag ±20 %; ingen fiendtlig trapperoms-/trappeutgang med sikt til, eller nærmere enn 12 m fra, egen
  spawnutgang. *Motprøve:* døra fra Sør-lobbyen til Trapp sør åpen på P4.
- e) Fri 2D-sikt langs hver rute (midtlinje og ±1,0 m) ≤ 42 m. *Motprøve:* veggen mellom Sør-spawngangen og ringen fjernet.
- **e2)** (K6) Ingen fri linje > 42 m fra et punkt i den ene basen (flagget ± 2 m og dørene) til et punkt i den andre.
  *Motprøve:* L-skjermene og lettveggene på boksene fjernet → linja gjennom origo (52,6 m) må finnes.
- **e3)** (K7) Langs R1/R3-midtlinja finnes minst hver 8. m en dekningslomme (sirkel r 0,75 m på gulvet) uten sikt til
  motsatt ringside og til P3-ringen (2D for samme plan, og «sett ovenfra» for P3: lomma ligger under P4-dekket, boksetak eller
  trappeløp). *Motprøve:* all COVER og lettvegg fjernet.
- f) Alle LOOT/TERMINALS/CAMERAS/spawn/flagg på et gulv (`FloorRects`) på sitt nivå. *Motprøve:* loot midt i atriet på P4.
- g) `BREACH_WALLS` ikke i bærende vegger, trapperom eller baserom; punktsymmetriske. *Motprøve:* panel i Sørbasens vegg.
- h) Hele FOOTPRINT inne i `PLAY_AREA`.
- **i)** (K11) Kapselprøve i data: for hver gulvcelle (0,25 m rutenett) under et trappeløp eller repos: fri høyde ≥ 2,0 m eller
  cellen er dekket av et massivt volum; ingen spalte 0,3–0,9 m mellom trappeløp og dekkekant. *Motprøve:* fjern fyllet under
  løp 2.
- **j)** (K10) Ingen flate med kollisjon som blokkerer `Pawn` ligger over P4-gulvet (Z > 14,0) innenfor atriet
  (|X| < 15,55, |Y| < 20,05), unntatt glasstaket og `AtriumBlockers`. *Motprøve:* et boksetak på P5 med `Pawn`-blokk.

### 7. Prøv
1. `python Tools/Blender/gen_helgasetr.py` → `python Tools/Blender/check_helgasetr.py` (alt OK, alle motprøver feiler).
2. Bygg C++ med `Build.bat … -WaitMutex` (editoren lukket, se CLAUDE.md).
3. Andreas: åpne Unreal, Play. Les loggen: «Helgasetr: flagg 2/2 og spawn 16/16 står riktig».

## Ferdig når
- `check_helgasetr.py` gir bare OK; alle motprøver feiler. `gen_helgasetr.py --check` er grønn.
- `Build.bat` uten feil (eller «Target is up to date» etter byggerens eget bygg).
- Loggen fra sporprøven: flagg 2/2 og spawn 16/16.
- Andreas (eller hovedagenten) i spillet: spawner i riktig grupperom på P4 og kommer ut gjennom slusa; flaggene står i
  baserommene; alle seks rutene kan gås uten å sette seg fast; regnet stopper under glasstaket; gripekroken fester ikke over
  P4 i atriet. `stat unit` / `stat gpu` måles fra P4 nordkorridor mot sør og fra kantina opp, og tallene skrives i rapporten.

## Risiko
- **Mange klosser:** vegger, trinn og glassfelt blir flere tusen instanser. Hold dem i få ISM-er (per materiale og type);
  `FlushPending` legger dem inn i én omgang. Lumen med klosser er grovt, men tett (vegger ≥ 0,15, dekker 0,35).
- **Kollisjon på trinn:** én boks per trinn gir små kanter; `MaxStepHeight` 50 cm tar 0,18 m-trinn.
- **Andre lag:** ingen loot/PC/kameraer før Spillbarhet leser listene (startpistol); `Flag.Z = 13.4` er en bro til spawnsporet
  flyttes (bestilt) – da settes den til 13,9. Den mytiske kassa havner i kantina (bestilt flyttet). PC-kartet viser for mye
  luft (bestilt hos Kamp).
- **Sjekkene kan kreve endringer i utgangsoppsettet** (dører, skjermer, spawnpunkter) – det er meningen; rapporter dem.
- Hvis tiden ikke strekker til: kutt lysene (bare noen få punktlys) og P5–P6 (bare dekke + blokkere). Ikke kutt sjekkene,
  basene, slusene, trappene eller glasset.

## IMPROVEMENTS-linjer (549–564, seksjon «Runde 4a (Grafikk og kart): Helgasetr, spillbart klossbygg»)
549. Nytt kart: Capture the Flag spilles nå inne i Helgasetr (NTNU, Elgeseter gate 10); campuskartet bygges ikke lenger
550. Helgasetr har målene fra arkitektens tegninger: 51,5 × 67,8 m, søyleaksene 1–10 og A–N, og etasjehøyder på 5,6, 4,3 og 4,0 m
551. Atriet går gjennom alle seks etasjene opp til et sagtak av glass med hvitt fagverk, og regn og snø stopper ved taket
552. Ringkorridor rundt atriet i 3. og 4. etasje (og som U i 2. etasje) med betongsøyler og glassrekkverk med treoverkant
553. Kantina i 1. etasje med lyst gulv, Ryddekroken-tårnet, salatbaren, plantekassene og båsene der de står i virkeligheten
554. Mesaninen i 2. etasje med oransje gulv, sittetrapp og grupperom i glass, og studentarbeidsplassene med dobbel takhøyde
555. Atriumtrappa i tre gule løp med ekte trinn: langs vestsiden, tvers over atriet i lufta og langs østsiden
556. Trapp øst går fra 1. til 4. etasje og Trapp sør fra 1. til 3. etasje (alarmdøra på 4. er låst)
557. Glassboksene henger ut i atriet i 3. og 4. etasje, med lettvegg og whiteboard mot korridoren og glass mot atriet
558. Sørbasen (rødt lag) i undervisningsrommet i sørenden av 4. etasje og Nordbasen (blått lag) i nordenden, med to dører og skjermer innenfor som hindrer sikt tvers over atriet
559. Spawn i grupperom bak hver base, med sluse og L-sving ut, så ingen ser inn fra korridorene eller trappene
560. Seks veier mellom basene: vest- og østkorridoren, laberadene og østfløyens rom, gjennom atriet via trappene, og ned gjennom kantina
561. Dekning minst hver 8. meter i korridorene langs atriet: søyler, skap, sofagrupper og glassboksenes vegger
562. Røde og blå striper og skilt «SØR»/«NORD» viser hvilken base du er ved
563. Gripekroken fester ikke i noe over 4. etasje i atriet, og ingen kan stå på glassboksene eller fagverket
564. Glass, lettvegger og dekning er egne deler med nummer, klare til å bli knust, sprengt og flyttet i neste runder
