# Plan 02 (Grafikk og kart): jevnere bildefrekvens, stengt bane og dekning for CTF

Bygger på commit 48f5cbf. Følger hovedagentens rekkefølge: (1) ytelse P1–P3/P8, (2) dekning og spawn (K4)
sammen med kanten, (3) tårn og roboter bort, K1–K3, K6, F3. Flaggrommene inne i basene, fanene og K7 er
skjøvet til `plan-03.md`.

Alle mål i meter, X = øst, Y = sør (som `GSCampus.h`). «Lokalt» = byggets egne koordinater.
Blender-rommet har speilvendt Y (Unreal y → Blender −y).

## Mål
Spillet går jevnere på RTX 3050-laptopen, banen er stengt hele veien rundt med ting som finnes på en campus
under ombygging, ingen rute er en åpen skytebane, og du starter skjermet bak din egen base.

## Forbedrer (gammelt)
- Ytelse: sola flyttes i trinn, lave klosser kaster ikke skygge, trærne koster mindre, oppstarten er kortere.
- Hovedbygningen: klokketårnet bort (R5), Lysgården ryddet så midtruta er åpen.
- Basene: like store og snudd om midten (K5), stien treffer endedøra (K3), spawn flyttet i skjul (K4).
- Kantstein i kryss (K2), snø og regn inne (F3), store ting som forsvinner på 90 m (F9), robotene (R11),
  README-linja (K1), sjekkskriptene (K6).

## Nytt
- Kanten: byggegjerder med og uten presenning, sperrebukker, betongsperrer, varselskilt, containere.
- Seks nye modeller: `SiteFence`, `SiteFenceTarp`, `SiteSign`, `RoadBlock`, `Container`, `ContainerGreen`.
- Dekning i rutene: inngjerdede gravefelt, parkerte varebiler, riggområde bak hver base, møblert bakgård,
  skjermvegg mellom ytterdør og flagg.

## Hvorfor nå
Andreas har bedt om jevnere bildefrekvens, og kontrolløren fant tre billige, sikre grep (P1, P2, P8).
Kontrollen av runde 1 (K4) viser at CTF ikke er rettferdig før rutene har dekning og spawn er skjermet.
Tårn og roboter har Andreas bestemt skal bort.

## Filer
- `Source/Gloshaugen/Public/GSCampus.h`, `Public/GSCampusBuilder.h`, `Private/GSCampusBuilder.cpp`
- `Source/Gloshaugen/Private/GSEnvironment.cpp`, `Public/GSEnvironment.h` (bare soltrinnet)
- `Tools/Blender/building_hovedbygningen.py`, `buildings.py`, `props.py`, `nature.py`, `backdrop.py` (del I),
  `check_walkable.py`, `check_campus.py`, `contact_sheet.py` (én linje)
- `Content/Python/gs_materials.py`, `README.md` (én linje, K1)
- Genererte modeller i `SourceArt/Models/`, bilder i `.claude/lag/grafikk/kontroll/r02-*.png`
- `IMPROVEMENTS.md` (416–431)

**Rekkefølge og kutt:** A → B → C → D → E → F → G → H. Del I (F17) tas bare hvis resten er ferdig og grønt.
Må noe kuttes: kutt I, så C5 (bakgårdsmøbler), og si fra i rapporten (ta da ut IMPROVEMENTS-linja).

---

## Steg

### A. Ytelse (`GSCampusBuilder`, `GSEnvironment`, `nature.py`)
1. **P1, sola i trinn (`GSEnvironment::Tick`):** `const float SunHours = FMath::GridSnap(Hours, 1.f / 60.f);`
   Regn `Elevation`/`Azimuth` for de to `SetWorldRotation`-kallene fra `SunHours`, og kall dem bare når
   `SunHours != LastSunHours` (ny `float LastSunHours = -1.f;` i headeren). Styrke, farge og `DayFactor`
   regnes som før fra `Hours`. Sola flytter seg da 0,25° hvert 2. sekund i stedet for hver frame.
2. **P1, trær (`AddModel`):** for grupper der navnet starter med `SM_Tree_`, `SM_Bush` eller `SM_Hedge`,
   før `RegisterComponent()`: `SetWorldPositionOffsetDisableDistance(4000)` og
   `ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Rigid`. Sjekk begge navnene mot
   motorens headere i UE 5.8 (`StaticMeshComponent.h`, `PrimitiveComponent.h`) før du bygger.
3. **P2, skygge:** i `AddBox` og `AddBoxMat`: en kloss er «lav» når toppen ligger under 0,3 m
   (`CenterCm.Z / 100 + SizeM.Z / 2 < 0.3`) og den ikke er vippet (pitch og roll 0). Lave klosser går i en
   egen gruppe (nøkkel med `_L` bak) med `SetCastShadow(false)`. Gjelder da bakken, veier, plasser,
   oppmerking, fortau og kantstein. `GetISM` får en ekstra `bool bShadow`.
4. **P8, oppstart:** ikke kall `AddInstance` én og én. Samle transformene per gruppe
   (`TMap<UInstancedStaticMeshComponent*, TArray<FTransform>> Pending`) i `AddMesh`, `AddBoxMat`, `AddProp`,
   `AddModel` og `AddTrunkCollider`, og tøm dem sist i `Build()` med ett
   `AddInstances(Transforms, false, /*bWorldSpace*/ true)` per gruppe, før tellingen i logglinja.
   I `AddProp`: `Plant`, `TrashBin`, `Stool` og `Projector` får aldri kollisjon (overstyr `bCollide`).
5. **P9:** gatelysene får `MaxDrawDistance = 8000` og `MaxDistanceFadeRange = 1500`.
6. **F9:** cull-avstand i `AddProp` etter navn: 40 000 cm for `StreetLamp`, `BusStop`, `Dumpster`,
   `NTNUSign`, `Barrier`, `SiteFence`, `SiteFenceTarp`, `SiteSign`, `RoadBlock`, `Container`,
   `ContainerGreen`; 9000 for resten.
7. **P3, det som er sikkert:** `nature.py`, lønna: bladkort per grentupp 7 → 4 (linje ca. 123) og
   40 → 20 (linje ca. 126). Kjør skriptet, og render lønna før og etter i samme bilde
   (`r02-lonn.png`): krona skal fortsatt se tett ut. Gjør den ikke det: prøv 5 og 28. Nanite for løv
   tas i plan-03 (må ses i editoren).

### B. Kartdata (`GSCampus.h`) – gjør headeren ferdig i ett
1. Fjern linja `Hovedbygningen taarn` fra `Buildings()`. Rett kommentarene som nevner tårnet.
2. **K5:** `Gamle kjemi`: `SizeX` 56 → **46**, `Yaw` 0 → **180** (midt uendret: 125, 10). Bygget dekker
   X 102…148, og banen er lik fra begge sider (snudd 180° om punktet (0, 10)).
3. **K3:** endedørene ligger lokalt y = +1,643. Bytt de to stubbene til
   `{ (-100, 11.6), (-78, 11.6), 3, true }` og `{ (78, 8.4), (100, 8.4), 3, true }`.
4. **K4, spawn bak basen** (mellom bygget og gjerdet, skjult av bygget fra alle tre rutene):
   vest: (−161, 3), (−161, 10), (−161, 17), (−155, 3), (−155, 10), (−155, 17), (−158, 6.5).
   Øst (speilet om (0, 10)): (161, 17), (161, 10), (161, 3), (155, 17), (155, 10), (155, 3), (158, 13.5).
   Bytt de samme 14 i `SpawnPoints()`; de fire midtpunktene der beholdes.
5. `Yaw` har aldri vært brukt før (alle var 0). Les `BuildBuilding`, hekkene og `BuildCampusProps` og
   sjekk at alt går via `BT`.

### C. `GSCampusBuilder`: tårn, roboter, Lysgården, kantstein, bakgården
1. Fjern grenene for `Hovedbygningen taarn` i `BuildBuilding` og `FurnishBuilding`.
2. **Roboter bort:** slett `RobotGuard` og `RobotDog` i `FurnishMainBuilding`, hele blokka «a delivery robot
   somewhere on every path», og la `Kind == 7` i plen-løkka bli `ParkBench`. `p_Robot*` i `props.py` og
   modellfilene blir liggende ubrukt.
3. **Lysgården (`FurnishMainBuilding`):** slett stolradene, `Lectern` og de to `Plant` på z 0,85.
   Kafébordene (med stoler) fra y {−6, −2,5, 1} til y {−7, −4,2, 5}; sofaene til (11, −6,8) og (11, −4,2),
   bordet til (9,6, −5,5); `Plant` på (±6,5, −8,3, 0). Frie ganger: nord–sør x 1,6…3,5 på begge sider av
   sokkelen (del D), øst–vest y −3,3…0.
4. **K2, kantstein (`BuildPathEdges`):** test begge ender og midten av hver bit mot andre veier, plasser og
   bygg; treffer en ende, kort biten inn til krysset (eller hopp over). Ingen bit skal ligge tvers over en sti.
5. **Bakgården** (der tårnet sto; dekning i sørruta; faste plasser sist i `BuildCampusProps`):
   `SM_Car_Van` (−14, 40) yaw 20 `CarPaint_White`; `SM_Car_Estate` (16, 33) yaw 195 `CarPaint_Grey`;
   `BikeRack` (−30, 20), (−30, 24), (30, 20), (30, 24); `Dumpster` (36, 50) og (38,5, 50) yaw 90;
   `Pallet` (−36, 48), `Crates` (−34, 49,5); `PicnicTable` (−8, 28) og (9, 46); `Planter` (±6, 18);
   `ParkBench` (0, 55) yaw −90.

### D. Modellene (`building_hovedbygningen.py`, `buildings.py`)
1. **Tårnet bort:** slett seksjonen «the tower» (linje ca. 373–463) og `TOWER`, `TOWER_H`; rett docstringen.
   Sørveggen: fjern `skip=[(0.0, 7.0)]` så vindusrekka går hele veien. Behold alle tre dørene i `doors_s`;
   den midterste (x 0, y 14) blir bakdør fra Lysgården til bakgården.
2. **Scenen:** slett sceneteppet (`Curtain`), scenegulvet og trinnet. Lag en sokkel
   `(-1.5, -8.5, 0) … (1.5, -6.5, 0.5)` med 6 cm gesims. Toppen må være z = 0,5: Spillbarhets mytiske
   kasse står på (0, −7,5, 0,85).
3. **Skjermvegg foran flagget (K4, `buildings.py`):** for `Gamle fysikk` og `Gamle kjemi`, første etasje:
   to vegger lokalt x −3 … 3 ved y = −6 og y = +6, 0,15 m tykke, z 0 … FH − 0,3, `PlasterWhite_In`.
   Da ser nord- og sørdøra (x 0) rett i en vegg, og flagget kan ikke beskytes fra stien utenfor.
   I `FurnishBuilding`: for basebyggene (navn i `GSCampus::Bases()`), legg `Keep`-punkter i z 0 på
   (−4,5, ±6), (−2,5, ±6), (0, ±6), (2,5, ±6), (4,5, ±6), så ingen møbler står i eller rundt veggene.
4. **Innvendige materialnavn (til del G):**
   - Hovedbygningen: `Plaster_In`, `Floor_In`, `Marble_In`, `Wood_In`, `Bulb_In` i stedet for dagens navn,
     og nye `Trim_In` og `Metal_In` på alt som står inne (trappetrinn, søyleføtter og kapiteler, balustre,
     sokkelen, ramperekkverk, lysekroner). Ute beholdes `Trim` og `Metal`. Øverste dekke er tak.
   - `buildings.py`: `inner` → `PlasterWhite_In`, `floor` → `Terrazzo_In`, ny `slab_in` =
     `ConcreteSmooth_In` for etasjeskillene (taket beholder `slab`), `ramp` → `Concrete_In`, ny
     `metal_in` = `Metal_In` for rekkverk inne, taklysene → `LightPanel_In`.
5. Kjør begge skriptene (Hovedbygningen og alle fem byggene man kan gå inn i).

### E. Nye modeller (`props.py`)
Forside +X, lengderetning langs Y, under ca. 400 trekanter hver:
- `SiteFence`: byggegjerde 3,5 × 2,0 m. Rørramme Ø 4 cm (`Metal`), 12 loddrette og 5 vannrette tråder
  (1 cm), betongfot 0,6 × 0,22 × 0,14 i hver ende (`Concrete`).
- `SiteFenceTarp`: samme ramme og føtter, med duk 3,4 × 1,8 × 0,01 m i ny materialplass `Tarp`
  (`"Tarp": ("Fabric", (0.10, 0.28, 0.16), 3, 0.9, 0, "Surface")` i `INSTANCES`).
- `SiteSign`: gul varseltrekant (0,6 m) med svart kant og hvit underplate 0,5 × 0,3, midt i z = 1,45.
- `RoadBlock`: sperrebukk. Rød/hvit-stripet bord 2,0 × 0,2 m i 1,0 m høyde på to A-bein, innkjøring
  forbudt-skilt (rød skive Ø 0,6 med hvit bjelke) på stolpe til 1,9 m, to oransje lamper.
- `Container` (`PaintBlue`) og `ContainerGreen` (`PaintGreen`): 6,06 lang (Y) × 2,44 × 2,59 høy,
  korrugerte sider (ribber hver 0,28 m), dørende med fire låsestenger, hjørnekasser i `MetalDark`.

### F. Kanten og dekningen (`GSCampusBuilder`, ny `BuildPerimeter()` rett etter `BuildPathEdges()`)
Hjelpere: `FenceRun(A, B, bOpen)` setter gjerdefelt fra A til B (`N = ceil(lengde / 3,5)`, jevn avstand,
forsiden mot venstre for retningen A→B eller som oppgitt), og `NearPerimeter(P, Margin)` (avstand til
gjerdelinja).
1. **Gjerdet:** gjerdelinja er `PlayArea()` krympet 0,5 m, forsiden inn. Feltet er `SiteFence` (åpent)
   der en vei krysser (`DistToSegment(midt, vei) < bredde/2 + (bPath ? 0,5 : 2,8)`) eller når
   `I % 4 == 2`, ellers `SiteFenceTarp`. `SiteSign` på hvert 8. felt, 8 cm innenfor.
2. **Veier som krysser linja** (regn ut skjæringspunktet C): Høgskoleringen (vest og øst): rekke
   `Barrier` hver 2,1 m tvers over vei og fortau 2 m innenfor C, én `RoadBlock` 3,5 m innenfor, og en
   varebil (`SM_Car_Van`, `CarPaint_White`) 4 m utenfor, vridd 8°. Promenaden (vest og øst) og stien mot
   Sentralbygg 1: én `RoadBlock` 1,5 m innenfor C.
3. **Usynlig vegg bak gjerdet** (gjerdet er 2,0 m, man klatrer 2,2 m): ny
   `UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> PerimeterBlock` (kube, skjult, `BlockAll`, ingen
   skygge), fire bokser nøyaktig på `PlayArea()`-kantene, 0,3 m tykke, z 0 … 14 m.
4. **Containere innenfor gjerdet** (langsiden langs kanten): `Container` (−159, −70) og (159, 90),
   `ContainerGreen` (−159, 75) og (159, −55), `Container` (−70, 135,5), `ContainerGreen` (70, 135,5).
   Legg hver i `Footprints`.
5. **Riggområde bak hver base (skjermer spawn, K4):** presenningsgjerde fra gjerdelinja og inn mot
   byggets ytre hjørner, med 4 m åpning inntil bygget. Vest: `FenceRun((−164,5, −3), (−152, −3))` og
   `FenceRun((−164,5, 23), (−152, 23))`. Øst: `FenceRun((164,5, −3), (152, −3))` og
   `FenceRun((164,5, 23), (152, 23))`. Én `Pallet` og én `Crates` i hvert riggområde, inntil gjerdet
   (minst 3 m fra hvert spawnpunkt).
6. **Nordruta (316 m rett sti, K4)**, likt på begge sider av midten:
   - Gravefelt tvers over stien ved x = ±96: rektangel 7 × 5 m (x ±96 ± 3,5, y −33 … −28) av
     `SiteFenceTarp` (fire `FenceRun`), med `Pallet` og `Barrier` inni. Man går rundt på plenen i sør.
   - Varebil parkert på stien: `SM_Car_Van` (−42, −30,7) yaw 4 `CarPaint_White` og
     `SM_Car_Estate` (42, −29,3) yaw 184 `CarPaint_Blue`.
   - Forplassen: `BikeRack` (±11, −31,5), `Planter` (±5, −33).
   Fra (−125, −30) skal det ikke lenger være fri sikt til (125, −30).
7. **Sørpromenaden (330 m rett):** gravefelt 7 × 6 m ved (±100, 120) (y 117 … 123, dekker hele stien),
   `SM_Car_Van` (−30, 120,6) yaw 3 `CarPaint_Grey`, `SM_Car_Hatch` (32, 119,4) yaw 183 `CarPaint_Red`.
8. Hold andre ting unna: `NearPerimeter` i `IsFree` (1,5 m), i bil-løkka (4 m) og i veikant-løkka i
   `BuildCampusProps` (1,5 m); benker og småting i veikant-løkka hoppes også over nærmere enn 5 m fra
   gravefeltene og de parkerte bilene (enkelt: en liste `Reserved` med punkter og radius).
9. Logg én linje: antall gjerdefelt, sperringer og containere.

### G. Snø og regn inne (`gs_materials.py`, F3)
1. `_surface_master`: ny skalar `Weather` (standard 1,0). Gang `Wetness` og `SnowCover` med den før de
   brukes (mørkning, blankhet, snømaske). `VERSION = 9`.
2. `build_instances`: for hver `Surface`-instans, lag også `MI_GS_<Navn>_In` med `MI_GS_<Navn>` som
   forelder og `Weather = 0` (lages når den mangler eller ved `force`). Den arver teksturer og farge.
3. `pick_material(slot, scanned, indoor=False)`: slutter navnet på `_In`, fjern endelsen og gi
   `_In`-instansen. Er `indoor` sann, gi `_In`-instansen for alle `Surface`-plasser.
4. `apply_to_models`: `INDOOR_MODELS` = `SM_Prop_` + Desk, Chair, OfficeChair, PC, Printer3D, Locker,
   Bookshelf, Whiteboard, Plant, VendingMachine, CoffeeMachine, Sofa, TrashBin, FireExtinguisher,
   FilingCabinet, ServerRack, LabBench, Stool, CafeTable, LectureSeats, Lectern, Projector → `indoor=True`.
5. `contact_sheet.py`: slå opp materialnavn uten `_In`, så bildene fortsatt får tekstur.
6. Snø på løv og bar tas i tre-runden.

### H. Sjekker, bilder, README
1. **K1:** `README.md` linje 118: rett Blender-stien til `...\Blender 5.3\blender.exe` (det står et
   backspace-tegn der i dag).
2. `check_walkable.py` (K6 + nytt):
   - Fjern «taarndor mot sor», døp «sordor midt» til «bakdor til bakgaarden».
   - Hver dør: én motprøve-stråle 1 m til siden for karmen som SKAL treffe (`want_hit=True`).
   - Rampene: stråler rett ned på rampen og på gulvet ved siden av; krev trinn ≤ 0,45 m på minst 0,65 m
     lengde i toppen (dagens mål; K7 gjør det romsligere i plan-03).
   - Tårnet er borte: (0, 20, 60) → (0, 20, 0,5) skal ikke treffe.
   - Lysgården: (2,5, −13, z) → (2,5, −3, z) og (−16, −0,4, z) → (16, −0,4, z), z 0,3 / 0,9 / 1,7, frie.
   - Basene (begge, lokalt, z 1,5): (0, −10) → (0, 0) og (0, 10) → (0, 0) skal treffe skjermveggen;
     (−5, −10) → (−5, 0) og (5, 10) → (5, 0) skal være frie.
3. `check_campus.py`:
   - Krev nøyaktig 10 bygg (K6; var 11 før tårnet).
   - d) de to basebyggene er like store, og flagg og bygg ligger speilvendt om (0, 10).
   - e) for hver base ender en sti høyst 2,5 m fra en endedør og høyst 0,5 m til siden for den
     (dørens y med formelen fra `buildings.py`, snudd når Yaw er 180).
   - f) hvert base-spawnpunkt ligger bak eget bygg: mellom byggets ytterste vegg og `PlayArea()`-kanten,
     og innenfor byggets Y-utstrekning.
4. Bilder til `.claude/lag/grafikk/kontroll/` (se på dem selv før du leverer):
   `r02-hovedbygning-sor.png` (fra bakgården, uten tårn), `r02-lysgaarden.png` (inne, fra hoveddøra mot
   trappa), `r02-base-skjermvegg.png` (inne i Gamle fysikk fra norddøra), `r02-kant.png`
   (`TEXTURED=1 contact_sheet.py` med de seks nye modellene), `r02-lonn.png` (A7).
5. `python -m py_compile Content/Python/gs_materials.py`. `Build.bat … -WaitMutex` til slutt.

### I. Bare hvis alt over er ferdig: F17 (`backdrop.py`)
Sett husene på terrenghøyden (samme `noise` som terrenget), start den indre ringen på z = 0 ved
r = 1 150 m, og legg et skjørt ned til −5 m, så himmelen ikke synes som en stripe under åsene.
Kjør skriptet og render ett bilde fra 30 m høyde (`r02-horisont.png`). Ikke med i IMPROVEMENTS-numrene
under; tas den, bytt den inn for linje 431 og slå 431 sammen med 430.

---

## Ferdig når
- `Build.bat` går gjennom. `check_campus.py` og `check_walkable.py` sier alt OK med de nye sjekkene og
  motprøvene.
- I koden: sola roteres bare når `SunHours` endres; tre-gruppene har WPO-avstand og `Rigid`; lave
  klosser ligger i grupper uten skygge; ingen `AddInstance` én og én igjen i de fem `Add*`-funksjonene.
- `GSCampus.h`: ikke noe tårn, Gamle kjemi 46 m med Yaw 180, stubbene på y 11,6 og 8,4, 14 nye spawnpunkter.
- Søk etter `Robot` i `GSCampusBuilder.cpp` gir ingen treff.
- `BuildPerimeter` setter gjerde rundt hele `PlayArea()`, sperrer de fem veikryssingene, legger fire
  usynlige vegger på kantene, to riggområder, fire gravefelt og fire parkerte biler i rutene.
- Kontrolløren kan regne i koden at linja (−125, −30) → (125, −30) og linja (−160, 120) → (160, 120)
  krysser minst ett gravefelt eller en bil, og at ingen spawnpunkter ses fra nordstien.
- `gs_materials.py` har `Weather`, `VERSION = 9`, `_In`-instansene og `INDOOR_MODELS`; kompilerer.
- Bildene finnes og viser det de skal. README-linja er rettet.

## Måling (Andreas, i editoren – kan ikke gjøres uten)
Før og etter, samme sted (foran hovedinngangen, klarvær): `stat unit`, `stat gpu` (ShadowDepths), og
logglinja «campus built … (x s)» (var 2,97 s, mål under 1 s). Skriv tallene i `logg.md` når de finnes.

## Risiko
- **Mange modeller lages på nytt.** Andreas må åpne editoren én gang: da importeres modellene og
  materialene bygges om (`VERSION` 9), det tar noen minutter. Si det i rapporten.
- **Motor-API (A2):** finnes ikke `SetWorldPositionOffsetDisableDistance` eller `Rigid` under de navnene
  i 5.8, bruk det headeren faktisk har, eller dropp den ene og skriv det i rapporten. Ikke gjett.
- **`AddInstances` (A4):** instansene finnes ikke før tømmingen sist i `Build()`. Ingenting i `Build()`
  må lese instanser underveis. `Footprints` og `Placed` er egne lister og påvirkes ikke.
- **Yaw 180** er første bygg som ikke står rett. Spillbarhets loot og PC bruker `BT` (sjekket).
- **Spillbarhets fil (`GSGameMode.cpp`):** to kameraer blir hengende i lufta der tårnet sto. Bestilt.
- **Usynlig vegg** stopper også kuler og granater over gjerdet. Den må ligge nøyaktig på kanten.
- **Gjerdet har hel kollisjon** (automatisk hylster), så kuler stopper i netting. Godtas ved kanten;
  gravefeltene bruker derfor presenning (ser tett ut).
- **Gravefeltene sperrer stien.** Det er meningen (man går rundt på plenen), men sjekk at det er minst
  2 m fri plen på sørsiden (nord) / nordsiden (promenaden) uten hekk, tre eller benk (`Reserved`).
- **Soltrinnet:** skyggene flytter seg et hakk hvert 2. sekund; synes det, halver trinnet (1/120).
- **F3 kan ikke ses uten editor.** Feiler instans-med-instans-som-forelder: lag egen instans med samme
  teksturer og `Weather = 0`.
- Tilfeldig-rekkene endrer seg, så trær og benker flytter på seg. Alle maskiner bygger likt.

## IMPROVEMENTS-linjer
Seksjon øverst: `## Runde 2 (Grafikk og kart): jevnere bilde, stengt bane og dekning (26. nov.)`

416. Jevnere bildefrekvens: sola flyttes i små trinn, og trærne tvinger ikke lenger skyggene til å tegnes på nytt hver eneste frame
417. Bakken, veiene, oppmerkingen, fortauene og kantsteinen kaster ikke lenger skygge (de kunne ikke skygge for noe), så skjermkortet får mindre å gjøre
418. Kortere frys når spillet starter og når noen kobler til: campus settes ut gruppe for gruppe i stedet for én ting om gangen, og lønnetrærne har halvparten så mange bladflater
419. Klokketårnet bak Hovedbygningen er borte (det finnes ikke i virkeligheten); bakveggen har vinduer hele veien og en bakdør fra Lysgården ut i bakgården
420. Robotene som sto som pynt i Lysgården, på stiene og på plenene er fjernet
421. Banen er stengt hele veien rundt med byggegjerder, noen med presenning og noen du ser gjennom; ingen kan gå ut av kartet, og det står alltid et gjerde der du blir stoppet
422. Høgskoleringen er sperret i begge ender med betongsperrer, sperrebukk og en parkert varebil; promenaden og stien mot Sentralbygget er stengt med sperrebukk
423. Nye modeller: byggegjerde, gjerde med presenning, varselskilt, sperrebukk og to containere; containerne står langs kanten som dekning
424. Du starter skjermet i et riggområde bak din egen base, ikke på åpen plen ved motstanderens rute
425. Nordstien og sørpromenaden er ikke lenger åpne skytebaner: inngjerdede gravefelt og parkerte biler bryter sikten og gir dekning
426. Flagget kan ikke lenger beskytes utenfra: en skjermvegg står mellom ytterdørene og flagget i begge basene
427. De to basene er like store og speilvendt om midten av banen, og stiene til endedørene går rett på døra
428. Midtruta gjennom Lysgården er åpen: sceneteppet, scenen og stolradene er borte, og kafébord og sofaer står ikke lenger i gangveien
429. Bakgården der tårnet sto er møblert med varebil, sykkelstativ, avfallscontainere, paller og bord som dekning i sørruta
430. Snø og regn legger seg ikke lenger inne: gulv, vegger og møbler holder seg tørre i all slags vær
431. Småfeil: kantsteinen ligger ikke lenger tvers over stier, lyktestolper og busskur synes på lang avstand (før 90 meter), og sjekkskriptene har motprøver så de ikke sier OK av feil grunn
