# Plan 02 (Grafikk og kart): stengt bane, åpne ruter, flaggrom i basene

Bygger på commit 48f5cbf. Alle mål i meter, X = øst, Y = sør (som `GSCampus.h`). «Lokalt» = byggets egne
koordinater (origo midt i bygget). Blender-rommet har speilvendt Y (Unreal y → Blender −y).

## Mål
Banen er stengt hele veien rundt med ting som finnes på en ekte campus under ombygging, de tre rutene er
åpne og treffer dørene, tårnet og robotene er borte, og hver base har et ordentlig flaggrom i første etasje.

## Forbedrer (gammelt)
- Hovedbygningen: klokketårnet bak fjernes (R5), bakveggen får vinduer; Lysgården ryddes så man kan gå
  rett gjennom (sceneteppe og stolrader sperrer i dag).
- Basebyggene: like store og speilvendt om midten, stien treffer endedøra, første etasje blir rom i stedet
  for én hall (R1, delvis).
- Materialene: snø og regn legger seg ikke lenger inne (F3). Store ting forsvinner ikke på 90 m (F9).
  Sola flyttes i trinn (F8, ytelse). Robotene som pynt fjernes (R11).

## Nytt
- Kanten: byggegjerder med og uten presenning, sperrebukker, betongsperrer, varselskilt, containere.
- Fem nye modeller (`SiteFence`, `SiteFenceTarp`, `SiteSign`, `RoadBlock`, `Container`/`ContainerGreen`).
- Flaggrom med skjermvegger, siderom og dekning; lagfargede faner på basene.
- Møblert bakgård (der tårnet sto) som dekning i sørruta.

## Hvorfor nå
Andreas har bestemt at tårn og roboter skal bort. Byggeren meldte tre hull etter runde 1 (åpen kant,
sperret Lysgård, stier som bommer på dørene). CTF spilles rundt flaggene, og der er det i dag en tom hall.
Lysrunden (novemberkveld) venter: den kan ikke sjekkes uten editor, og den rører bare `GSEnvironment`,
så den kan tas alene i runde 3.

## Filer
- `Source/Gloshaugen/Public/GSCampus.h`, `Public/GSCampusBuilder.h`, `Private/GSCampusBuilder.cpp`
- `Source/Gloshaugen/Private/GSEnvironment.cpp`, `Public/GSEnvironment.h` (bare soltrinnet, del G)
- `Tools/Blender/building_hovedbygningen.py`, `buildings.py`, `props.py`, `check_walkable.py`,
  `check_campus.py`, `contact_sheet.py` (én linje)
- `Content/Python/gs_materials.py`
- Genererte: `SourceArt/Models/SM_Hovedbygningen.*`, `SM_Glass_Hovedbygningen.*`, `SM_Bygg_*` (de fem
  byggene man kan gå inn i), `SM_Glass_*`, fem nye `SM_Prop_*`
- `IMPROVEMENTS.md` (416–431), bilder i `.claude/lag/grafikk/kontroll/r02-*.png`

Rekkefølge hvis tiden blir knapp: A → B → C → D → E er kjernen. F, G, H er små. Faner (D5) og
bakgårdsmøbler (C4) kan kuttes sist, men si da fra i rapporten og ta ut IMPROVEMENTS-linja.

---

## Steg

### A. Kartdata (`GSCampus.h`) – gjør headeren ferdig i ett
1. Fjern linja `Hovedbygningen taarn` fra `Buildings()`. Rett kommentarene som nevner tårnet.
2. `Gamle kjemi`: `SizeX` 56 → **46**, `Yaw` 0 → **180** (X og Y uendret: 125, 10). Da er de to basene
   like store og banen er lik fra begge sider (snudd 180° om punktet (0, 10)). Bygget dekker X 102…148.
3. Veier: endedørene på basene ligger lokalt y = +1,643 (formelen i `BuildWallSide`). Bytt de to stubbene:
   `{ (-100, 11.6), (-78, 11.6), 3, true }` og `{ (78, 8.4), (100, 8.4), 3, true }`.
4. Spawn: bytt `{ 159, 10 }` med `{ 157, 10 }` både i `Bases()` og i `SpawnPoints()`.
5. `Yaw` har aldri vært brukt før (alle var 0). Les gjennom `BuildBuilding`, hekkene og
   `BuildCampusProps` og sjekk at de bruker `BT` (det gjør de i dag) – ikke legg til noe som antar Yaw 0.

### B. Hovedbygningen (`building_hovedbygningen.py`)
1. **Tårnet bort:** slett hele seksjonen «the tower» (linje ca. 373–463: vegger, hjørnestøtter, bånd, buer,
   urskiver, spir, spiralrampe, klokke, rekkverk) og konstantene `TOWER`, `TOWER_H`. Rett docstringen.
2. Sørveggen: fjern `skip=[(0.0, 7.0)]` så vindusrekka går hele veien. Behold alle tre dørene i `doors_s`;
   den midterste (x 0, y 14) blir bakdør fra Lysgården ut i bakgården.
3. **Scenen:** slett sceneteppet (`Curtain`), scenegulvet (−6…6) og trinnet. Lag i stedet en sokkel
   `(-1.5, -8.5, 0) … (1.5, -6.5, 0.5)` med en 6 cm gesims rundt toppen. Toppen må være z = 0,5:
   Spillbarhets mytiske kasse står på (0, −7,5, 0,85).
4. **Innvendige materialnavn (til F):** lag `PLASTER`, `FLOOR`, `MARBLE`, `WOOD`, `Bulb` med navnene
   `Plaster_In`, `Floor_In`, `Marble_In`, `Wood_In`, `Bulb_In`. Lag i tillegg `TRIM_IN = K.mat("Trim_In", …)`
   og `METAL_IN = K.mat("Metal_In", …)` og bruk dem på alt som står inne (trappetrinn, søyleføtter og
   kapiteler, balustre, sokkelen, ramperekkverk, lysekroner). Ute beholdes `Trim` og `Metal`.
   Øverste dekke (`STONE_D`, f == FLOORS) er tak og beholdes.
5. Kjør `blender --background --python Tools/Blender/building_hovedbygningen.py`.

### C. `GSCampusBuilder`: tårn, roboter, Lysgården, bakgården
1. `BuildBuilding` og `FurnishBuilding`: fjern grenene for `Hovedbygningen taarn`.
2. **Roboter bort:** slett `RobotGuard` og `RobotDog` i `FurnishMainBuilding`, hele blokka «a delivery robot
   somewhere on every path» og `Kind == 7`-grenen (RobotDog) i plen-løkka i `BuildCampusProps` (la den bli
   `ParkBench`). Funksjonene `p_Robot*` i `props.py` og modellfilene blir liggende ubrukt.
3. **Lysgården (`FurnishMainBuilding`):** slett stolradene, `Lectern` og de to `Plant` på z 0,85.
   Flytt kafébordene (med stoler) fra y {−6, −2,5, 1} til y {−7, −4,2, 5}; sofaene til (11, −6,8) og
   (11, −4,2), bordet til (9,6, −5,5). Sett `Plant` på (±6,5, −8,3, 0). Frie ganger etterpå: nord–sør
   x 1,6…3,5 på begge sider av sokkelen, øst–vest y −3,3…0.
4. **Bakgården** (sist i `BuildCampusProps`, faste plasser, verdenskoordinater): varebil
   `AddModel("SM_Car_Van", (−14, 40), yaw 20, "CarPaint_White", kollisjon)`, stasjonsvogn
   `SM_Car_Estate` (16, 33) yaw 195 `CarPaint_Grey`; `BikeRack` (−30, 20), (−30, 24), (30, 20), (30, 24) yaw 0;
   `Dumpster` (36, 50) og (38,5, 50) yaw 90; `Pallet` (−36, 48), `Crates` (−34, 49,5);
   `PicnicTable` (−8, 28) og (9, 46); `Planter` (±6, 18); `ParkBench` (0, 55) yaw −90.
   Hopp over en ting hvis modellen mangler (det gjør `AddProp` selv).
5. **F9:** i `AddProp`, sett cull-avstand etter navn: 40 000 cm for `StreetLamp`, `BusStop`, `Dumpster`,
   `NTNUSign`, `Barrier`, `SiteFence`, `SiteFenceTarp`, `SiteSign`, `RoadBlock`, `Container`,
   `ContainerGreen`; 9000 for resten (liten hjelpefunksjon i det anonyme navnerommet).

### D. Basene
1. **Vegger i modellen (`buildings.py`):** ny metode `base_interior()` som kalles fra `build()` når
   `b["name"]` er `Gamle fysikk` eller `Gamle kjemi`. Første etasje, vegger 0,15 m tykke, z 0 … FH − 0,3,
   materiale `PlasterWhite_In`. Døråpninger er 1,6 m brede og 2,2 m høye (veggen fortsetter over), med
   6 cm karm i `Wood_In` på begge sider og over. Lokale mål (bygget er 46 × 22, innside ±22,6 × ±10,6):
   - Tverrvegg ved x = −7 og x = +7, y −10,6 … 10,6, åpninger med midt i y = −3,5 og y = +3,5.
   - Langvegg ved y = −2: fra x −22,6 til −7 med åpning midt i x = −15, og fra x 7 til 22,6 med åpning x = 15.
   - To skjermvegger i hallen: x −3 … 3 ved y = −6 og ved y = +6 (ingen åpning).
   Resultat: flagget (0, 0) står i en indre sone på 14 × 12 m med seks veier inn (fire hjørner rundt
   skjermveggene, to sideåpninger). Ingen ytterdør har fri sikt til flagget: nord- og sørdøra (x 0)
   ser rett i en skjermvegg, endedørene (y 1,64) ser i tverrveggen.
   Bruk en liten hjelpefunksjon «vegg med hull» (samme idé som `wall()` i `building_hovedbygningen.py`).
2. **Innvendige materialnavn (til F):** `inner` → `PlasterWhite_In`, `floor` → `Terrazzo_In`, ny
   `slab_in` = `ConcreteSmooth_In` for etasjeskillene (taket beholder `slab`), `ramp` → `Concrete_In`,
   ny `metal_in` = `Metal_In` for rekkverk inne, taklysene → `LightPanel_In`.
3. **Møbler (`GSCampusBuilder`):** ny `FurnishBase(B)`; `FurnishBuilding` kaller den for etasje 0 når
   `B.Name` er et basebygg i `GSCampus::Bases()`, og kjører den vanlige rom-løkka fra etasje 1.
   Lokale plasser, z = 0. Hold 2 m fritt rundt (0, 0) og 1,6 m rundt PC-en (−22, −4,4).
   - Indre sone: `LabBench` (−3,5, −2,8), (3,5, −2,8), (−3,5, 2,8), (3,5, 2,8) med langsiden langs X
     (benken er 2,36 m langs sin lokale Y: yaw 90); `Locker` mot skjermveggene innsiden: (−1,5, −5,6) yaw 90
     og (1,5, 5,6) yaw −90; `Crates` (−5,6, 0,4) og (5,6, −0,4); `FilingCabinet` (−6,5, −5,3) og (6,5, 5,3);
     `FireExtinguisher` (6,9, 0, yaw 180, uten kollisjon).
   - Vindfangene (|y| > 6): `Plant` (±5,5, ∓9,6), `TrashBin` (2, −9,8) og (−2, 9,8), `Whiteboard` på
     skjermveggenes ytterside (0, −6,15) yaw −90 og (0, 6,15) yaw 90.
   - Vestre lobby (x < −7, y > −2): `Sofa` (−12, 9,6) yaw −90, `VendingMachine` (−8, 9,8) yaw −90,
     `Locker` (−18, −1,5) yaw 90 og (−11, −1,5) yaw 90.
   - Østre lobby (rampa ligger i x 14…22,6, y 8…10,6): `Sofa` (10, 9,6) yaw −90, `CoffeeMachine`
     (7,6, 0,5) yaw 0, `Locker` (11, −1,5) yaw 90.
   - De to nordrommene: `FurnishRoom(BT, −21,8, −7,8, −9,8, −2,8, 0, Rand, Keep)` og
     `FurnishRoom(BT, 7,8, 21,8, −9,8, −2,8, 0, Rand, Keep)`.
   Sjekk hver yaw mot modellen i `props.py` (forsiden er +X) før du godtar plasseringen.
4. Kjør `blender --background --python Tools/Blender/buildings.py` (alle fem byggene, pga. materialnavnene).
5. **Faner (`buildings.py`, `extras()`):** for basebyggene: skiltet over hoveddørene i lagfarge i stedet for
   NTNU-blått, og hengende faner 0,9 × 5,0 × 0,03 m, midt i z = 6,5: på nord- og sørfasaden ved x = ±2,4,
   og på begge endevegger ved y = dørens y ± 1,9. Gamle fysikk (lag rød, base 0) `PaintRed`, Gamle kjemi
   (lag blå, base 1) `PaintBlue`.

### E. Kanten (`props.py` + ny `BuildPerimeter()`)
1. **Modeller i `props.py`** (forside +X, lengderetning langs Y, under ca. 400 trekanter hver):
   - `SiteFence`: byggegjerde 3,5 × 2,0 m. Rørramme Ø 4 cm (`Metal`), 12 loddrette og 5 vannrette tråder
     (1 cm), betongfot 0,6 × 0,22 × 0,14 i hver ende (`Concrete`), klemme øverst.
   - `SiteFenceTarp`: samme ramme og føtter, med duk 3,4 × 1,8 × 0,01 m i ny materialplass `Tarp`.
   - `SiteSign`: gul varseltrekant (0,6 m) med svart kant og hvit underplate 0,5 × 0,3, midt i z = 1,45,
     2 cm tykk (henges på gjerdet).
   - `RoadBlock`: sperrebukk. Rød/hvit-stripet bord 2,0 × 0,2 m i 1,0 m høyde på to A-bein, rundt
     innkjøring forbudt-skilt (rød skive Ø 0,6 med hvit bjelke) på stolpe til 1,9 m, to oransje lamper.
   - `Container` (`PaintBlue`) og `ContainerGreen` (`PaintGreen`): 6,06 lang (Y) × 2,44 × 2,59 høy,
     korrugerte sider (ribber hver 0,28 m), dørende med fire låsestenger, hjørnekasser i `MetalDark`.
   Kjør `python`/`blender` for de seks navnene, og legg `"Tarp": ("Fabric", (0.10, 0.28, 0.16), 3, 0.9, 0, "Surface")`
   i `INSTANCES` i `gs_materials.py`.
2. **`BuildPerimeter()`** i `GSCampusBuilder` (ny, kalles i `Build()` rett etter `BuildPathEdges()`):
   - Gjerdelinja er `PlayArea()` krympet 0,5 m. For hver av de fire kantene: `N = ceil(lengde / 3,5)`
     felt med jevn avstand. Feltet blir `SiteFence` hvis en vei krysser der
     (`DistToSegment(midt, vei) < bredde/2 + (bPath ? 0,5 : 2,8)`) eller `I % 4 == 2`, ellers
     `SiteFenceTarp`. Yaw slik at forsiden vender inn: nordkant 90, sørkant −90, vestkant 0, østkant 180.
     `SiteSign` på hvert felt med `I % 8 == 5`, 8 cm innenfor gjerdet.
   - Der en vei krysser gjerdelinja (regn ut skjæringspunktet C; veiene er rette langs X eller Y):
     bilvei (Høgskoleringen, to steder): rekke `Barrier` hver 2,1 m tvers over vei og fortau
     (±(bredde/2 + 2,4)) 2 m innenfor C, én `RoadBlock` 3,5 m innenfor, og en varebil
     (`SM_Car_Van`, `CarPaint_White`) 4 m utenfor, 1,5 m til siden, vridd 8°.
     Gangsti (promenaden to steder, stien mot Sentralbygg 1): én `RoadBlock` 1,5 m innenfor C.
   - Containere like innenfor gjerdet (langsiden langs kanten), like for begge lag: `Container`
     (−159, −70), `ContainerGreen` (−159, 75), `Container` (159, 90), `ContainerGreen` (159, −55),
     `Container` (−70, 135,5), `ContainerGreen` (70, 135,5). Legg hver container i `Footprints`
     (så trær og småting holder seg unna).
   - **Usynlig vegg bak gjerdet** (gjerdet er 2,0 m, og man kan klatre 2,2 m): ny
     `UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> PerimeterBlock` (kube, skjult, `BlockAll`,
     ingen skygge – som `TrunkColliders`), fire bokser nøyaktig på `PlayArea()`-kantene, 0,3 m tykke,
     z 0 … 14 m. Det står alltid et gjerde foran.
   - Logg én linje: antall gjerdefelt, sperringer og containere.
3. Hold andre ting unna gjerdet: hjelpefunksjon `NearPerimeter(P, Margin)` (avstand til gjerdelinja).
   Bruk den i `IsFree` (trær, steiner: 1,5 m), i bil-løkka (hopp over biler nærmere enn 4 m fra linja) og i
   veikant-løkka i `BuildCampusProps` (1,5 m).

### F. Snø og regn inne (`gs_materials.py`, F3)
1. `_surface_master`: ny skalar-parameter `Weather` (standard 1,0). Gang `Wetness` og `SnowCover` med den
   før de brukes (mørkning, blankhet, snømaske). Sett `VERSION = 9`.
2. `build_instances`: for hver instans med master `Surface`, lag også `MI_GS_<Navn>_In` med
   `MI_GS_<Navn>` som forelder og `Weather = 0` (lages når den mangler eller når `force`). Den arver
   teksturer og farge, også de skannede.
3. `pick_material(slot, scanned, indoor=False)`: slutter navnet på `_In`, fjern endelsen og bruk
   `_In`-instansen. Er `indoor` sann, bruk `_In`-instansen for alle `Surface`-plasser.
4. `apply_to_models`: `INDOOR_MODELS` = `SM_Prop_` + Desk, Chair, OfficeChair, PC, Printer3D, Locker,
   Bookshelf, Whiteboard, Plant, VendingMachine, CoffeeMachine, Sofa, TrashBin, FireExtinguisher,
   FilingCabinet, ServerRack, LabBench, Stool, CafeTable, LectureSeats, Lectern, Projector → `indoor=True`.
5. `contact_sheet.py` (`_textured`): slå opp materialnavn uten `_In`, så bildene fortsatt får tekstur.
6. Snø på løv og bar tas i tre-runden (backlog), ikke nå.

### G. Sola i trinn (`GSEnvironment`, F8)
I `Tick`: `const float SunHours = FMath::GridSnap(Hours, 1.f / 60.f);` og bruk den til `Elevation`/`Azimuth`
i de to `SetWorldRotation`-kallene, og kall dem bare når `SunHours != LastSunHours` (ny `float LastSunHours
= -1.f` i headeren). Styrke og farge regnes fortsatt hver frame fra `Hours`. Sola flytter seg da 0,25° hvert
2. sekund i stedet for hver frame, så skyggekartene kan gjenbrukes mellom trinnene.

### H. Sjekker og bilder
1. `check_walkable.py`:
   - Fjern «taarndor mot sor», døp «sordor midt» til «bakdor til bakgaarden».
   - Tårnet er borte: stråle (0, 20, 60) → (0, 20, 0,5) skal ikke treffe.
   - Lysgården: (2,5, −13, z) → (2,5, −3, z) og (−16, −0,4, z) → (16, −0,4, z) for z 0,3 / 0,9 / 1,7 skal
     være frie. Treffer de en søyle eller trappestolpe: flytt strålen maks 0,5 m og skriv hvorfor; treffer
     de fortsatt, rett modellen.
   - Basene (begge, lokale mål): de seks åpningene frie i z 0,3 / 0,9 / 1,7 (stråle 1,5 m på hver side);
     «ser veggen» (`want_hit=True`, z 1,5): (0, −10, ) → (0, 0), (0, 10) → (0, 0), (−22, 1,64) → (0, 0),
     (22, 1,64) → (0, 0).
2. `check_campus.py`: d) de to basebyggene har samme størrelse, og flagg og bygg ligger speilvendt om
   (0, 10); e) for hver base finnes en sti som ender høyst 2,5 m fra en endedør og høyst 0,5 m til siden
   for den (dørens y regnes med formelen fra `buildings.py`, snudd når Yaw er 180).
3. Bilder til `.claude/lag/grafikk/kontroll/` (se på dem selv før du leverer):
   `r02-hovedbygning-sor.png` (fra bakgården, uten tårn), `r02-lysgaarden.png` (inne, fra hoveddøra mot
   trappa), `r02-flaggrom.png` (inne i Gamle fysikk fra en sideåpning mot flaggplassen),
   `r02-base-fasade.png` (Gamle fysikk fra nord, fanene), `r02-kant.png`
   (`TEXTURED=1 contact_sheet.py` med de seks nye modellene).
4. `python -m py_compile Content/Python/gs_materials.py` (syntaks; `unreal` importeres ikke ved kompilering).
5. `Build.bat … -WaitMutex` til slutt.

---

## Ferdig når
- `Build.bat` går gjennom. `check_campus.py` og `check_walkable.py` sier alt OK, med de nye sjekkene.
- `GSCampus.h` har ikke tårnet, Gamle kjemi er 46 m med Yaw 180, stubbene ligger på y 11,6 og 8,4.
- Søk etter `Robot` i `GSCampusBuilder.cpp` gir ingen treff.
- De fem bildene finnes og viser: ingen tårn og vinduer i bakveggen; fri gang gjennom Lysgården;
  vegger, åpninger og skjermvegger i flaggrommet; røde faner; seks nye modeller som ser ekte ut.
- `gs_materials.py` har `Weather`, `VERSION = 9`, `_In`-instansene og `INDOOR_MODELS`; kompilerer.
- `BuildPerimeter` setter gjerde rundt hele `PlayArea()`, sperrer de fem veikryssingene og legger fire
  usynlige vegger på kantene.
- Kontrolløren kan lese i koden at ingen ytterdør i basene har fri linje til flagget.

## Risiko
- **Mange modeller lages på nytt.** Andreas må åpne editoren én gang etterpå: da importeres modellene og
  materialene bygges på nytt (`VERSION` 9). Det tar noen minutter. Før det ser han gamle modeller med nye
  møbler (flaggrom-møbler uten vegger). Si det i rapporten.
- **Yaw 180** er første bygg som ikke står rett. Kollisjon og møbler bruker `BT`, men se etter steder som
  regner i verdenskoordinater uten å snu. Spillbarhets loot og PC bruker `BT` (sjekket).
- **Spillbarhets fil (`GSGameMode.cpp`)**: to kameraer blir hengende i lufta der tårnet sto, og loot i
  basene kan havne i en av de nye veggene (15 cm). Bestilt i `bestillinger.md`; ingenting stopper.
- **Usynlig vegg** stopper også kuler og granater 0…14 m over gjerdet. Greit ved kanten, men den må ligge
  nøyaktig på kanten, ikke innenfor.
- **Kollisjon på gjerdet** blir en hel plate (automatisk hylster), så kuler stopper i et nettinggjerde.
  Godtas ved kanten. Lag Kamp får materialmerking senere.
- **Ytelse:** ca. 340 gjerdefelt, men som instanser (HISM) med få trekanter. Presenningsfeltene er
  billigst, derfor flest av dem.
- **F3 kan ikke ses uten editor.** Kontrolløren leser grafen i koden. Instans med instans som forelder er
  vanlig i Unreal, men feiler kallet: fall tilbake til egen instans med samme teksturer og `Weather = 0`.
- **Soltrinnet:** skyggene flytter seg et lite hakk hvert 2. sekund. Ved lav sol kan det synes på lange
  skygger; da halveres trinnet (1/120).
- Tilfeldig-rekkene endrer seg (roboter ut), så benker og trær flytter litt på seg. Alle maskiner bygger
  likt, så det er trygt online.

## IMPROVEMENTS-linjer
Seksjon øverst: `## Runde 2 (Grafikk og kart): stengt bane, åpne ruter og flaggrom (26. nov.)`

416. Klokketårnet bak Hovedbygningen er borte (det finnes ikke i virkeligheten); bakveggen har vinduer hele veien og en bakdør fra Lysgården ut i bakgården
417. Robotene som sto som pynt i Lysgården, på stiene og på plenene er fjernet
418. Banen er stengt hele veien rundt med byggegjerder, noen med presenning og noen du ser gjennom; ingen kan gå ut av kartet, og det står alltid et gjerde der du blir stoppet
419. Høgskoleringen er sperret i begge ender med betongsperrer, sperrebukk og en parkert varebil; promenaden og stien mot Sentralbygget er stengt med sperrebukk og skilt
420. Nye modeller: byggegjerde, gjerde med presenning, varselskilt, sperrebukk og to containere; containerne står langs kanten som dekning
421. Midtruta gjennom Lysgården er åpen: sceneteppet, scenen og stolradene er borte, og kafébord og sofaer står ikke lenger i gangveien
422. Stiene til endedørene på basene går nå rett på døra (lå 1,6 m til siden)
423. De to basene er like store og speilvendt om midten av banen, så ingen av lagene har fordel
424. Flaggrom i første etasje i begge basene: en hall med skjermvegger og to siderom, seks veier inn, og ingen ytterdør med fri sikt til flagget
425. Flaggrommene er innredet som gamle laboratorier: labbenker, skap og kasser som dekning rundt flagget, sofa og automater i gangene
426. Basene er lette å kjenne igjen: røde faner og skilt på Gamle fysikk, blå på Gamle kjemi
427. Snø og regn legger seg ikke lenger inne: gulv, vegger og møbler holder seg tørre i all slags vær
428. Lyktestolper, busskur, containere og gjerder synes på lang avstand (før forsvant de på 90 meter)
429. Sola flyttes i små trinn i stedet for hver eneste frame, så skyggene ikke må tegnes helt på nytt hele tiden
430. Bakgården der tårnet sto er møblert: varebil, sykkelstativ, avfallscontainere, paller og bord gir dekning i sørruta
431. Sjekkskriptene finner nå også sperrede ganger i Lysgården og flaggrommene, fri sikt til flagget og stier som bommer på dørene
