# Plan 03 (Grafikk og kart): spillbare ruter, motpart til Kjelhuset, repos og flaggrom

Bygger på koden etter 1ee5e98 og 6f7e35a (100 m vegg med lokk, sluser i riggområdet, `GSCampus::RigFences()`,
`Mirror()`, `check_campus.py` a–i). Prioritet: **spillbart først** (`prioriteringer.md`, 26. nov. kl. 22.55).
Del A er det som avgjør om banen er spillbar (N1, N3, K7, N2). Del B er detaljene (faner, skilt). Møblering
av flaggrommet som laboratorium (FurnishBase fra utkastet) og Nanite for trær (P3) er flyttet til plan-04.

Koordinater i meter, X = øst, Y = sør. Lokale mål i basene: origo midt i bygget, innside ±22,6 × ±10,6,
etasjehøyde 4 m. **Symmetri:** rutene er like om x = 0 (vest ↔ øst, samme y), kalt **X-speiling** under
(`(x, y) → (−x, y)`). Basene er fortsatt snudd 180° om (0, 10) som før.

## Mål
Man kommer seg fra base til base på alle tre rutene uten å sette seg fast eller bli skutt langs hele stien,
begge lag har like ruter, man går rett av rampene, og flagget ses ikke fra stien.

## Forbedrer (gammelt)
- Gravefeltene på nordstien og sørpromenaden (N1: de dekket bare selve stien; linjene 2–4 m ved siden er frie
  over hele banen, 329 m).
- Bilene langs rutene (står ulikt i vest og øst).
- Rampene i alle bygg (K7: trang topp, man går av de siste 0,65 m).
- Basene: endedøra ser rett på flagget (N2), og skiltet over dørene er NTNU-blått på begge.
- `check_campus.py` d) (sjekket bare basene for symmetri).

## Nytt
- Forskjøvet dekning (sikksakk med containere) på begge ruter, med data i ny `GSCampus::RouteCover()`.
- Nytt bygg **Varmeteknisk** (finnes på ekte Gløshaugen) som motpart til Kjelhuset (N3).
- Repos i rampetoppen, tverrvegger og flaggrom i basene, lagfargede faner.
- Tre nye sjekker: fri sikt langs rutene (j), framkommelighet base til base (k), X-speiling (d2).
- Kartbilde fra `check_campus.py --kart`.

## Hvorfor nå
Andreas har bekreftet at spillet kjører (menyen virker, ca. 60 bilder/s i eget vindu). Det neste som stopper en
ekte CTF-kamp er kartet: åpne skytebaner langs stiene, ulike sider, og steder man kan sette seg fast. Alt kan
sjekkes med skript og stråler uten editor.

## Filer
- `Source/Gloshaugen/Public/GSCampus.h` (Varmeteknisk, `FCover`, `RouteCover()`, `MirrorX()`)
- `Source/Gloshaugen/Private/GSCampusBuilder.cpp` (`BuildPerimeter` 6–7, `BuildBuilding` ramper, `FurnishBuilding`)
- `Tools/Blender/buildings.py` (repos, `base_interior()`, faner, skilt)
- `Tools/Blender/check_campus.py` (d, d2, j, k, `--kart`), `Tools/Blender/check_walkable.py` (ramper, basene)
- Genererte `SourceArt/Models/SM_Bygg_{Kjelhuset,Gamle_fysikk,Gamle_kjemi,Hovedbygningen_vest,Hovedbygningen_oest,Varmeteknisk}.*`
- `IMPROVEMENTS.md` (533–548), `.claude/lag/bestillinger.md` (én linje til Spillbarhet)

## Steg

### Del A: spillbarhet på kartet

1. **Data for dekningen (`GSCampus.h`).** Legg til etter `RigFences()`:
   ```cpp
   inline FVector2D MirrorX(const FVector2D& P) { return FVector2D(-P.X, P.Y); }   // the routes mirror about x = 0
   /** Cover on the north path and the south promenade, WEST half (m). The east half is MirrorX()ed, Yaw -> 180 - Yaw.
    *  SizeX/SizeY = footprint after Yaw. Checked by Tools/Blender/check_campus.py d2), j) and k). */
   struct FCover { const TCHAR* Kind; float X, Y, Yaw, SizeX, SizeY; };
   inline const TArray<FCover>& RouteCover() { static const TArray<FCover> List = {
       { TEXT("DigSite"),        -96.f,  -30.f,   0.f, 7.f,   9.f   },   // north path: y -34.5 .. -25.5
       { TEXT("SM_Car_Van"),     -42.f,  -30.7f,  4.f, 5.3f,  2.0f  },
       { TEXT("ContainerGreen"), -28.f,  -31.f,   0.f, 2.44f, 6.06f },   // chicane, north half of the band
       { TEXT("Container"),      -21.f,  -27.5f,  0.f, 2.44f, 6.06f },   // chicane, south half
       { TEXT("DigSite"),       -100.f,  120.f,   0.f, 7.f,   9.f   },   // south promenade: y 115.5 .. 124.5
       { TEXT("SM_Car_Van"),     -22.f,  120.6f,  3.f, 5.3f,  2.0f  },   // moved from x -30 (room for the chicane)
       { TEXT("Container"),      -39.f,  119.f,   0.f, 2.44f, 6.06f },
       { TEXT("ContainerGreen"), -32.5f, 122.5f,  0.f, 2.44f, 6.06f },
   }; return List; }
   ```
   Regnet på forhånd (2D, alt over regnet som tett, bånd = stimidt ±4 m, steg 0,25 m): lengste frie linje blir
   **70 m nord** og **76 m sør**. Uten de fire containerne: 185 og 193 m (motprøven). Sikksakken har 4,6 m (nord)
   og 4,1 m (sør) mellom containerne; man går rundt den ene og inn mellom dem. Containerne står minst 7 m fra
   hovedinngangen (x 0) og sykkelstativene (±11, −31,5).

2. **`BuildPerimeter` punkt 6–7** leser `RouteCover()` i stedet for faste tall, og setter ut vest og X-speilet øst:
   - `DigSite`: lambdaen tar senter og størrelse (`CX ± SizeX/2`, `CY ± SizeY/2`); pall og betongsperre som før;
     `Reserved` med radius 8 (var 7).
   - Containere: `AddProp(Kind, …, Yaw)`, `Footprints.Add` (som i punkt 4) og `Reserved.Add(…, 4.5f)`.
   - Biler: `AddModel(Kind, …)`. Lakk: vest hvit (nord) og grå (sør), øst blå (nord) og rød (sør).
   - Øst: `GSCampus::MirrorX(P)` og `Yaw' = 180 − Yaw`.
   - Fjern den gamle `Parked`-tabellen og de faste `DigSite(Sgn * …)`-kallene. Ikke rør punkt 1–5 (kanten, slusene).

3. **Motpart til Kjelhuset (`GSCampus.h` `Buildings()`).** Rett etter Kjelhuset:
   `{ TEXT("Varmeteknisk"), 62.f, 105.f, 22.f, 18.f, 180.f, 12.f, 4.f, Concrete, true },`
   (X-speilet av Kjelhuset, samme størrelse og etasjer; Yaw 180 som Gamle kjemi). Plassen er fri: passasjen
   x 76…80, østfløyen x ≤ 55, promenaden y ≥ 117,5. `buildings.py` har alt stilen `"Varmeteknisk": "tower"`
   (betong, flatt tak). Den gamle `SM_Bygg_Varmeteknisk` i `SourceArt/Models` er fra det store kartet og blir
   overskrevet. Oppdater kommentaren øverst i `GSCampus.h`.

4. **K7, repos i rampetoppen.** Åpningen i dekket beholdes (lengde `ramp_len = FH / tan 25°`), så møbler, PC-er og
   `FurnishBuilding` står som før. Ny konstant `RAMP_LANDING = 1.2` (Blender) og `RampLanding = 1.2f` (C++).
   - `buildings.py` `floors_and_ramps`: rampen går fra `ox0` (gulvet under) til `ix1 − 1,2` (gulvet over), vinkel
     `atan(FH / (ramp_len − 1,2))`: 28,5° ved 4 m, 28,0° ved 4,5 m (gangbart er 50°, `GSCharacter.cpp:144`).
     Legg et dekke (`slab_in` + `floor`) i `x ix1 − 1,2 … ix1`, `y oy0 … iy1` i hver øvre etasje. Rekkverket på
     gulvsiden av rampen stopper 2,6 m før rampens nye topp; rekkverket langs åpningen i etasjen over slutter ved
     `ix1 − 1,2 − 2,6`, så reposet er åpent mot gulvet. Håndlisten mot veggen følger bare rampen.
   - `GSCampusBuilder.cpp` `BuildBuilding` (klossvarianten uten modell, ca. linje 760–785): samme regning, så de
     to holdes like.
   - Kjør `buildings.py` for Kjelhuset, Varmeteknisk, Hovedbygningen vest og Hovedbygningen oest (basene i steg 5).

5. **N2, vegger i basene (`buildings.py`, ny `base_interior()`, kalles i `build()` for `read_base_buildings()`
   ved siden av `screen_walls()`).** 0,15 m tykke, z 0 … `fh − SLAB_T`, samme materiale som skjermveggene
   (`m.inner`). Døråpninger 1,6 × 2,2 m med 6 cm karm i `Wood_In` (sjekk at den finnes i `gs_materials.py`;
   ellers bruk et `_In`-tremateriale som finnes). Lag `wall_with_holes(axis, fixed, a0, a1, holes)` (som `wall()`
   i `building_hovedbygningen.py`).
   - Tverrvegg x = −7 og x = +7, y −10,6 … 10,6, åpninger med midt i y = −3,5 og y = +3,5.
   - Langvegg y = −2: x −22,6 … −7 med åpning i x = −15, og x 7 … 22,6 med åpning i x = 15.
   - Skjermveggene (y ±6, x −3 … 3) står. Flagget (0, 0) står da i et rom på 14 × 12 m med seks veier inn
     (nord- og sørdøra rundt skjermveggene, fire åpninger i tverrveggene). Linja fra endedøra (±22,6, 1,64)
     til flagget krysser x = ±7 ved y ≈ 0,5, altså i veggen.
   - Rampehjørnet (x 13,6 … 22,2, y 7,6 … 10,2) og PC-en (−22, −4,4) berøres ikke; se etter i bildet.
   - Kjør `buildings.py` for Gamle fysikk og Gamle kjemi.

6. **Møbler unna veggene (`FurnishBuilding`).** I løkka over `Bases()` der skjermveggene får `Keep`: legg
   `Keep`-punkter i etasje 0 hver 1,5 m langs x = ±7 (y −10 … 10) og langs y = −2 (|x| 7,5 … 22). `Free()` holder
   da ca. 2,4 m unna, så ingen møbler står i en vegg eller en åpning. Ingen ny møblering nå (plan-04).

7. **Sjekker i `check_campus.py`** (alle nye med motprøve som skal feile):
   - a)–i) som før; endre «noyaktig 10 bygg» til 11.
   - **d2) X-speiling:** hvert bygg man kan gå inn i, som ikke er base eller Hovedbygningen, har en partner på
     `(−x, y)` med samme størrelse, høyde og etasjehøyde (Kjelhuset ↔ Varmeteknisk). `RouteCover()` speiles av
     koden selv; sjekk i stedet at ingen ting i den ligger innenfor x = ±3 (hovedinngangen). Motprøve: byggelisten
     uten Varmeteknisk blir avvist.
   - **j) fri sikt langs rutene:** linjer y = −34 … −26 (nord) og 116 … 124 (sør), hver 0,25 m, fra gjerde til
     gjerde (x −164,5 … 164,5). Tette: bygg (`footprints`), `RouteCover()` vest + X-speilet, riggjerdene. Krav:
     lengste frie strekk ≤ 80 m; skriv ut det lengste og hvor. Motprøve 1: uten containerne (skal gi > 80 m).
     Motprøve 2: gravefeltene med gammel dybde (5 og 6 m) og uten containere.
   - **k) framkommelighet:** rutenett 0,5 m i båndet y −45 … −15 (nord) og 105 … 135 (sør), hindringer = de samme
     rektanglene utvidet 0,45 m (kapselen) og riggjerdene. Bredde-først-søk fra (−158, −30) til (158, −30) og fra
     (−160, 120) til (160, 120) skal finne vei. Motprøve: en vegg tvers over båndet (x −60, y −45 … −15 og
     y 105 … 135) gjør at søket ikke finner vei.
   - **`--kart <png>`** (matplotlib 3.10 finnes): ovenfra med bygg, veier, gjerder, `RouteCover`, spawn, flagg og
     den lengste frie linja i rødt for hver rute. Lagre som `.claude/lag/grafikk/kontroll/r03-kart.png`.

8. **Sjekker i `check_walkable.py`:**
   - `check_ramps`: legg `"Varmeteknisk"` til i listen, og regn med reposet (`ramp_len − RAMP_LANDING`). Krav: trinn
     ≤ 0,45 m mellom reposet og gulvet ved siden av over minst **1,1 m** (var 0,65 m). Motprøven flyttes til 3 m før
     veggen (der er rampa ca. 1 m under gulvet). Rekkverket midt på rampa skal fortsatt finnes.
   - `check_base` (lokale mål, modellen før Yaw): stråler fra stubben utenfor endedøra mot flagget skal treffe:
     fra `(x, 1,64, 1,6)` for x = 25, 30, 35, 40, 45 mot `(0, 0, 1,0)` og `(0, 0, 2,0)`, og det samme fra `(−x, 1,64, 1,6)`
     (den andre endedøra). Motprøver: `(−20, 1,64, 1,5) → (−7,5, 1,64, 1,5)` er fri (døra er åpen) og
     `(−7,5, 3,5, 1,0) → (−6,5, 3,5, 1,0)` er fri (åpningen er åpen).
   - De seks åpningene er frie i z 0,3 / 0,9 / 1,7 tvers gjennom veggen; motprøve 1 m til siden treffer.

9. **Bilder (`render_building.py`):** `r03-flaggrom.png` (fra åpningen i x −7, y 3,5 mot flaggplassen, z 1,7),
   `r03-rampetopp.png` (reposet i Kjelhuset sett fra gulvet over), `r03-varmeteknisk.png` (utvendig fra sørvest).
   Legg dem i `.claude/lag/grafikk/kontroll/` sammen med kartet fra steg 7.

### Del B: kjenne igjen basene (detaljer)

10. **Faner og skilt (`buildings.py` `extras()`, bare basene).** Skiltet over hoveddørene i lagfarge i stedet for
    `NTNUBlue`: Gamle fysikk `PaintRed` (rødt lag, base 0, se `AGSGameMode::BasePointM`), Gamle kjemi `PaintBlue`.
    Hengende faner 0,9 × 0,03 × 5,0 m med midt i z = 6,5, 0,1 m ut fra veggen, med en stang 1,2 m øverst
    (`m.metal`): på nord- og sørfasaden ved x = ±2,4, og på endeveggene ved dørens y ± 1,9. Del av byggmodellen.
    Bilde: `r03-base-fasade.png`.

11. Oppdater kommentarene (`GSCampus.h` øverst, `BuildPerimeter` 6–7) og skriv IMPROVEMENTS (under).

12. **Bestilling til Spillbarhet** (`bestillinger.md`, én linje): nytt bygg man kan gå inn i (Varmeteknisk,
    (62, 105), Yaw 180, 3 etasjer) får PC og loot av seg selv som de andre; gravefeltene er nå 7 × 9 m med senter
    (±96, −30) og (±100, 120), og det står fire containere på rutene, alt i `GSCampus::RouteCover()`: hopp over
    utendørs loot der. Basene har vegger ved x = ±7 og y = −2 (|x| > 7): hold loot 0,5 m unna (som før bestilt).

## Ferdig når
- `python Tools/Blender/check_campus.py` og `blender --background --python Tools/Blender/check_walkable.py` sier OK
  med kode 0, alle nye motprøver feiler som de skal, og utskriften viser lengste frie linje ≤ 80 m på begge ruter.
- Bildene fra steg 7, 9 og 10 finnes og viser sikksakken, Varmeteknisk, reposet, veggene med åpninger og fanene.
- Ingen faste tall for gravefelt og biler igjen i `BuildPerimeter` (`grep "DigSite(Sgn"` gir ingenting).
- `Build.bat … -WaitMutex` går gjennom (byggeren kjører det når editoren er lukket; ellers merkes runden som
  ikke lenket, som i runde 2).
- `IMPROVEMENTS.md` har 533–548 i egen seksjon.

## Risiko
- **Likt på vert og klient:** alt er faste tall i fast rekkefølge; `RouteCover()` er en `TArray`, ikke `TMap`.
- **Trær og småting** der containerne kommer: `Reserved` og `Footprints` tar det, fordi `BuildPerimeter` kjører
  før `BuildTreesAndProps` (sjekk at det fortsatt er slik).
- **Brattere rampe (28,5°):** godt under 50°, men Andreas bør gå opp og ned én gang. Rampens kollisjon er samme
  `k.box` som før.
- **Spillbarhets loot og PC-er** følger formlene i `SpawnTerminals`; PC-en i etasje 0 (−HX + 1, −HY·0,4) ligger i
  nordrommet i basene, utenfor de nye veggene.
- **Kjelhusets pipe** står utenfor vestveggen; Varmeteknisk har ingen. Liten forskjell i dekning, godtatt.
- **N4** er rettet i 6f7e35a, **N6** gjelder ikke (vi bruker containere, ikke åpent byggegjerde). **N5** (sola i
  trinn) må fortsatt ses av Andreas.

## IMPROVEMENTS-linjer
Seksjon: `## Runde 3 (Grafikk og kart): ruter uten lange skytebaner, Varmeteknisk, repos og flaggrom (27. nov.)`
Gammelt som er forbedret: gravefeltene og bilene (533, 534, 537), rampene (539), basene (540–543, 545), sjekkene (548).
Nytt: sikksakk-dekning (535, 536), Varmeteknisk (538), faner (544), sikt- og framkommelighetssjekk (546, 547).

533. Gravefeltene på nordstien er gjort 9 m dype, så de stenger både stien og plenen 4 m på hver side, ikke bare selve stien
534. Det samme for gravefeltene på sørpromenaden
535. To containere står forskjøvet på nordstien på hver side av hovedinngangen, som en sikksakk du går gjennom: ingen linje langs stien har mer enn 80 m fri sikt
536. Samme sikksakk av containere på sørpromenaden
537. Bilene langs rutene står likt på begge sider, så ingen side får mer dekning (varebilen på sørpromenaden er flyttet inn mot midten)
538. Nytt bygg øst for bakgården: Varmeteknisk, like stort som Kjelhuset, med tre etasjer og ramper, så begge lag har et bygg å gå inn i ved sørruta
539. Rampene i alle byggene ender i et flatt repos på 1,2 m, så du går rett av øverst i stedet for å klemme deg inn mot veggen
540. Basene har vegger i første etasje: et flaggrom på 14 × 12 m midt i bygget med seks veier inn, og egne rom på hver side
541. Flagget kan ikke lenger ses gjennom endedøra fra stien utenfor; du ser en vegg
542. Dørkarmer i tre rundt de nye døråpningene i basene
543. Møblene holder seg unna de nye veggene og åpningene, så ingenting står i en døråpning
544. Røde faner på Gamle fysikk og blå på Gamle kjemi, på alle fire sidene, så du ser på lang avstand hvilken base som er hvilken
545. Skiltet over hoveddørene i basene har lagets farge
546. Sjekkskriptet regner fri sikt langs nordstien og sørpromenaden (også 4 m til hver side) og sier fra over 80 m, med motprøve
547. Sjekkskriptet går gjennom rutene i et rutenett og sjekker at du kommer fra base til base uten å sette deg fast i dekningen, med motprøve
548. Sjekkskriptene krever at byggene og dekningen står likt på begge sider, at rampene har repos og at flagget ikke ses fra døra, med motprøver
