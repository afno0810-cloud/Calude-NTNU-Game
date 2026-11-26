# Plan 03 (Grafikk og kart): flaggrommene i basene

Utkast, skrevet sammen med plan-02 fordi detaljløftet ble skjøvet hit. Bygger på koden slik den er etter
plan-02 (basene 46 × 22 m, Gamle kjemi med Yaw 180, skjermvegger ved lokalt y = ±6, `_In`-materialer).
Planleggeren går over den mot kontrollen av runde 2 før den bygges. IMPROVEMENTS-numre deles ut av dirigenten.

Lokale mål (origo midt i bygget, innside ±22,6 × ±10,6, etasjehøyde 4 m).

## Mål
Hver base har et ordentlig flaggrom i første etasje: rom, dører og dekning rundt flagget, og basene er
lette å kjenne igjen utenfra.

## Forbedrer (gammelt)
Første etasje i basene er én åpen hall med tilfeldige møbelgrupper (R1). Rampetoppen er trang (K7).
Skiltet over dørene er NTNU-blått på begge baser.

## Nytt
Tverrvegger og siderom, flaggrom innredet som gammelt laboratorium, lagfargede faner, repos i rampetoppen.

## Hvorfor nå
CTF avgjøres rundt flagget, og der er det i dag ingenting å forsvare seg bak. Kan sjekkes uten editor
(stråler og bilder).

## Filer
`Tools/Blender/buildings.py`, `check_walkable.py`, `Source/Gloshaugen/Private/GSCampusBuilder.cpp`,
`Public/GSCampusBuilder.h`, genererte `SM_Bygg_*`, `IMPROVEMENTS.md`.

## Steg
1. **Vegger (`buildings.py`, ny `base_interior()` for `Gamle fysikk` og `Gamle kjemi`):** 0,15 m tykke,
   z 0 … 3,7, `PlasterWhite_In`. Døråpninger 1,6 × 2,2 m med 6 cm karm i `Wood_In`.
   - Tverrvegg ved x = −7 og x = +7, y −10,6 … 10,6, åpninger med midt i y = −3,5 og y = +3,5.
   - Langvegg ved y = −2: x −22,6 … −7 med åpning i x = −15, og x 7 … 22,6 med åpning i x = 15.
   - Skjermveggene fra plan-02 beholdes (x −3 … 3 ved y = ±6).
   Flagget (0, 0) står da i en sone på 14 × 12 m med seks veier inn. Endedørene (y 1,64) ser i tverrveggen.
   Lag en hjelpefunksjon «vegg med hull» (som `wall()` i `building_hovedbygningen.py`).
2. **K7, repos (`floors_and_ramps`):** la rampen ende 1,2 m før ytterveggen i et flatt repos i gulvhøyde
   (rampen blir litt brattere, maks 28°), og åpne rekkverket langs hele reposet. Oppdater rampestrålene i
   `check_walkable.py` til å kreve 1,2 m gangbar topp.
3. **Møbler (`GSCampusBuilder`, ny `FurnishBase(B)`):** `FurnishBuilding` kaller den for etasje 0 i
   basebyggene og kjører vanlig rom-løkke fra etasje 1. Hold 2 m fritt rundt (0, 0) og 1,6 m rundt PC-en
   (−22, −4,4). Sjekk hver yaw mot modellen i `props.py` (forside +X).
   - Indre sone: `LabBench` (±3,5, ±2,8) med langsiden langs X (yaw 90); `Locker` (−1,5, −5,6) yaw 90 og
     (1,5, 5,6) yaw −90; `Crates` (−5,6, 0,4) og (5,6, −0,4); `FilingCabinet` (−6,5, −5,3) og (6,5, 5,3);
     `FireExtinguisher` (6,9, 0) yaw 180 uten kollisjon.
   - Vindfangene (|y| > 6): `Plant` (5,5, −9,6) og (−5,5, 9,6), `TrashBin` (2, −9,8) og (−2, 9,8),
     `Whiteboard` på skjermveggenes ytterside.
   - Vestre lobby (x < −7, y > −2): `Sofa` (−12, 9,6), `VendingMachine` (−8, 9,8), `Locker` (−18, −1,5)
     og (−11, −1,5).
   - Østre lobby (rampa: x 14 … 22,6, y 8 … 10,6): `Sofa` (10, 9,6), `CoffeeMachine` (7,6, 0,5),
     `Locker` (11, −1,5).
   - Nordrommene: `FurnishRoom(BT, −21,8, −7,8, −9,8, −2,8, 0, …)` og `FurnishRoom(BT, 7,8, 21,8, −9,8, −2,8, 0, …)`.
4. **Faner (`extras()`):** skiltet over hoveddørene i lagfarge, og hengende faner 0,9 × 5,0 × 0,03 m med
   midt i z = 6,5: på nord- og sørfasaden ved x = ±2,4 og på endeveggene ved dørens y ± 1,9.
   Gamle fysikk `PaintRed` (lag rød, base 0), Gamle kjemi `PaintBlue` (lag blå, base 1).
5. Kjør `buildings.py` for de to basene (alle fem hvis K7 tas for alle).
6. **Sjekker (`check_walkable.py`):** de seks åpningene frie i z 0,3 / 0,9 / 1,7, med motprøve 1 m til
   siden; «ser veggen» fra endedørene: (−22, 1,64) → (0, 0) og (22, 1,64) → (0, 0) skal treffe.
7. **Bilder:** `r03-flaggrom.png` (fra en sideåpning mot flaggplassen), `r03-base-fasade.png` (fanene).
8. Prøv Nanite for `SM_Tree_*`, `SM_Bush`, `SM_Hedge` i `init_unreal.py` (P3) bare hvis Andreas kan se
   på trærne i editoren samme dag; ellers egen runde.

## Ferdig når
Bygget går gjennom, begge sjekkskript OK, bildene viser vegger, åpninger, møbler og faner, og ingen
ytterdør har fri linje til flagget (stråler).

## Risiko
- Spillbarhets loot i basene kan havne i en vegg (bestilt: hold 0,5 m unna vegglinjene).
- Brattere rampe (K7): sjekk at `WalkableFloorAngle` i `GSCharacter` tåler 28° (standard er 44°).
- Møblene står etter faste mål: flyttes en vegg, må møblene flyttes med.
