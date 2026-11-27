**GODKJENT**

# Kontroll av runde 4a (Grafikk og kart) – commit bebbf7c, Helgasetr som klossbygg fra data

Kontrollert mot `ko/plan-04.md` (4a), forhåndssjekken `kontroll/forhand-plan-04.md` (K1–K24) og fasiten `helgasetr.md`.
Jeg har lest commiten med `git show`. `Tools/Blender/*` og `GSHelgasetr*` i arbeidsmappen er like commiten, så skriptene
jeg kjørte gjelder commiten. Build.bat er ikke kjørt. Byggeren meldte Succeeded og at den hodeløse prøven ga «flagg 2/2 og
spawn 16/16».

- `python Tools/Blender/gen_helgasetr.py --check` sier OK.
- `python Tools/Blender/check_helgasetr.py` sier «Alt OK».
  - Sjekk a–j går gjennom, også de nye fra forhåndssjekken (c, d2, e2, e3, i, j).
  - Alle motprøvene blir avvist.
- `python Tools/Blender/check_campus.py` sier også «Alt OK». Den gamle kartdataen leses fortsatt riktig fra `else`-grenene.

Runden er solid: hele bygget lages fra én datakilde, sjekkene har motprøver, og interaktive deler har id fra første dag.
Ingenting av det jeg fant ødelegger spillet i dag. Fem ting bør rettes tidlig i 4b (se Funn).

## Andreas' regel: forbedret og nytt
- **Forbedret:**
  - `GSCampus::Bases()/SpawnPoints()/PlayArea()/Buildings()/Roads()` leser nå Helgasetr. GameMode, HUD, minikartet og
    «Flagget sist sett» virker uten endringer.
  - Etasjenavnene er riktige for alle seks plan (sonene har 4,3 m etasjehøyde).
  - Regn og snø stopper under glasstaket. Taket blokkerer `Visibility`, og fagverket over P4 gjør det også.
  - `UGSWorldSubsystem` bygger nå kartet i `OnActorsInitialized`, altså før motoren lager de første spillerne. Det gjelder
    også det gamle kartet.
- **Nytt:**
  - Helgasetr P1–P6 som klosser: 2 484 klosser, 343 glassfelt, 54 døråpninger, 6 lettvegger, 24 dekningsklosser og
    48 lys.
  - Baser, spawn i sluser, sjekkskriptet og id-tabellene for Interaktiv.

Begge deler er oppfylt.

## Sjekket spesielt

**1. Spawn usett fra fiendens ruter.**
- Sjekk c ser på 2 215 punkter: ringen, lobbyåpningene ±1 m, venterommet/NØ-gangen, laberadene, østfløyens rom,
  trappeutgangene og R2/R4.
- Den regner alle dører som åpne og glass som gjennomsiktig. Det er det verste tilfellet, og det er riktig.
- Spawnrommene ligger i ytterfløyen, med betonggulv over og under, så en 2D-sjekk på P4 er nok.
- Interaktivs dører står i tillegg lukket ved start.
- Fra P3, P5 og atriet er det ingen sikt inn.

**2. Spawn- og flaggsporene, med koden slik den er nå.**
- `AGSGameMode` i foreldrecommiten (bfb38ef, Spillbarhet) sporer allerede fra `+2 m` med objektspor (`WorldStatic`).
- Jeg simulerte disse sporene på dataene for begge flaggene og alle 16 spawnpunktene, med `Flag.Z` både 13,7 og 13,9.
  Alle 18 treffer gulvet på 13,9 m.
- Klaringen fra vegger og dekning er 0,95–2,4 m, så kapselen (r 0,42) får plass.
- Spawn virker altså. Men broen og sporprøven er utdatert (funn 1).

**3. Fall og fastsetting.**
- Trinnene er 0,18 m (`MaxStepHeight` er 50 cm).
- Under trappene er det fylt der fri høyde er under 2,0 m, og det er ingen spalte på 0,3–0,9 m mot dekkekanter (sjekk i).
- Rekkverket er 1,16 m med treoverkant, og glassfeltene har 1 cm mellomrom.
- Rekkverket er fjernet bare der et trappeløp lander.
- Dekkene er 0,35 m, og veggene går ned gjennom dekket.
- Alle kollisjoner er bokser (kuben har enkel bokskollisjon, K15).
- Jeg fant ingen steder å falle gjennom. Det eneste stedet man kan bli stående fast er avhengig av gripekroken (funn 3).

**4. Id-tabellene mot Interaktivs kode i arbeidsmappen.**
- `GSMapState.cpp` (`CollectMapDefs`/`CollectMapGlass`) leser disse feltene, og alle finnes i `GSHelgasetr.h`:
  - fra `FDoorM`: `Id, X, Y, Z, Width, Height, Yaw, bHingeLeft, Kind, bLocked, bOpen, Room`
  - fra `FPanelM`: `Id, Cx, Cy, Cz, Len, Height, Yaw, Type`
- `Kind` 0–4 tilsvarer `DoorKindNames`.
- Glassdører ligger ikke i `GlassPanels()`, slik Interaktivs hopp over glassfelt i døråpninger forutsetter.
- Taggen `GS_Int_Glass` finnes og kan brukes av `HideBuilderGlass`.
- Id-ene er stabile mellom maskiner, fordi alle bygger fra samme kompilerte tabell.
  - `FlushPending` legger instansene inn i køens rekkefølge på helt nye komponenter, så indeks = id stemmer.
  - `QueueBoxM` deler aldri glassfelt, lettvegger eller dekning i fliser (`bTile = false`).
  - Ingen glassfelt har lengde under 2 mm. Om noen hadde hatt det, ville resten av indeksene forskjøvet seg.
- Id-ene er **ikke** stabile når dataene endres: én ny dør på P2 nummererer om alle dører på P3–P4. Det står i
  helgasetr.md kap. 13, men ingenting sjekker det (forslag 2). To semantiske avvik er funn 2 og 4.

**5. Ytelse.**
- Det blir ca. 21 strukturgrupper, 2 for glass, 1 for lettvegger, 5 for dekning og 1–2 for lys og ledninger, til sammen
  ca. 30 ISM-komponenter. Det er ca. 5 300 instanser etter 4 m-flisene.
- 48 lys, alle uten skygge og med `MaxDrawDistance` 4000 og fade: 8 flatelys (RectLight) og 40 punktlys med demping
  ≤ 600 cm.
- Glass har ingen skygge, ingen distansefelt og ingen strålesporing (K19).
- Lys, skilt, gløding og ledninger lages ikke på dedikert server.
- Ingenting kjører hvert bilde. Det eneste som går etter starten er én logglinje etter 3 s.
- `stat unit` / `stat gpu` er ikke målt (det står i «Ferdig når»), så det må hovedagenten ta.
- Lyslekkasje er funn 5.

**6. Det gamle kartet.**
- `if constexpr (bHelgasetr) … else` beholder listene uendret (sammenlignet linje for linje i diffen).
- `AGSCampusBuilder` har bare fått `virtual` på `Build`/`SetStreetLightsOn`.
- `check_campus.py` er tilpasset og grønn.

**Avvik byggeren meldte.** Alle er godtatt:
- `Flag.Z` 13,7 (begrunnet: reserveplassen ligger over gulvet).
- 6 lettvegger (sjekk g er grønn, P3-veggene står i vest og øst i stedet for nord og sør).
- Flyttede dører, spawnpunkter og L-skjermer (sjekkene krevde det, og det står i kap. 13).
- Rette atriehjørner (rundes av i 4b).
- Motprøven i c er byttet til et punkt bak den flyttede S1-døra. Det er greit, siden døra er flyttet.

## Funn (rettes tidlig i 4b; ingen ødelegger spillet nå)

**1. Broen `Flag.Z = 13,7` og sporprøven følger ikke GameMode slik den er nå.**
- Filer: `GSHelgasetr.gen.inl` l. 8 (`FlagZBridgeM`), kommentaren i `GSCampus.h` `Bases()`, og
  `GSHelgasetrBuilder.cpp` l. 393–457.
- Bestillingen (K1) ble LEVERT før denne commiten, i bfb38ef: spawn og flagg sporer nå med objektspor fra +2 m. Der står
  det: «Grafikk kan sette Flag.Z tilbake til 13,9».
- Kommentaren påstår fortsatt at sporet starter på +4 m.
- `ProbeGameModeTraces` kjører de *gamle* sporene: kanalspor fra +4 m, og flaggspor fra +3 m. «Flagg 2/2 og spawn 16/16»
  beviser derfor ikke de sporene spillet faktisk bruker. Min simulering viser at de virker likevel.
- Gjør:
  - Sett `FlagZBridgeM` til 13,9 (eller bruk `Bases().Z`).
  - Rett kommentaren i `GSCampus.h` og helgasetr.md.
  - La prøven kopiere `TryPlace`/`SpawnFlags` slik de er nå: `LineTraceSingleByObjectType(WorldStatic)` fra +2 m til
    −3 m, deretter kapselen.

**2. Den låste alarmdøra på P4 (DoorSlot 31, `laast`) står i en hel vegg.**
- I `helgasetr_plan.py` `derive()` får `laast` ikke noe hull i veggen (`if kind != "laast"`).
- Interaktiv lager likevel en dør der (`CollectMapDefs`: Kind 4 → Fire, låst).
- Etter Interaktivs regel 656 kan låste dører og alarmdører alltid åpnes innenfra. Innsiden er Trapp sør, der blått kommer
  opp. Da svinger dørbladet ut av betongveggen og inn i røds lobby, med lyd og E-hint.
- Hvis 4b lager hull for å se riktig ut, åpner det flankeruta som d2 skal stenge.
- Gjør ett av to:
  - (a) Ikke lag en DoorSlot for `laast` (bare vegg).
  - (b) Gi den et felt `bAlarm`/`bSealed` som Interaktiv respekterer: kan ikke åpnes, brytes eller sprenges.
- Skriv valget i `bestillinger.md` til Interaktiv. helgasetr.md kap. 13 sier «54 døråpninger … alle åpne hull i 4a», og det
  stemmer ikke for denne døra.

**3. Toppen av veggene i P4-glassboksene er et gulv man kan stå på, inne i taket som ikke stopper spillere.**
- Filer: `helgasetr_plan.py` (boksvegger på P4: glassfelt type 1 og lettvegg med topp 16,9 m, stolper med topp 16,9) og
  `GSHelgasetrBuilder.cpp` `SetupCollision(Coll_High)`.
- Boksetaket (16,9–17,35) er `Coll_High`: det slipper `Pawn` gjennom, men stopper `Visibility`.
- Glassfeltene og lettveggene under taket stopper `Pawn` og `WorldStatic`-kanalen. Gripekroken (`LineTraceSingleByChannel
  ECC_WorldStatic`) kan derfor feste i dem.
- En spiller kan da bli stående på en vegg- eller stolpetopp 3 m over P4. Kroppen står inne i taket, og der stopper taket
  kuler (`Visibility`), mens hodet stikker opp. Det er et nesten usårbart utkikkspunkt over atriet.
- En spiller som lander ved siden av, faller gjennom taket og ned i en låst boks.
- Det går ikke i dag, fordi det ikke finnes gripekrok i Helgasetr før loot kommer. IMPROVEMENTS 563 («ingen kan stå på
  glassboksene») er likevel ikke helt sant.
- Gjør: la `Coll_High` stoppe `Pawn` med `WalkableSlopeOverride = Unwalkable`, eller legg en blokkering (`Coll_Blocker`)
  i hele taket på P4-boksene. Utvid sjekk j: ingen flate som stopper `Pawn` har ledig høyde over seg ≥ 1,84 m over Z 14,0
  i atriet.

**4. Kodelåsdørene: dataene og Interaktiv mener ulike ting.**
- Dataene har `bLocked = false` for alle 24 `glass_kode`-dører.
- Interaktiv låser alle sammen ved start (`bLocked = D.bLocked || D.Kind == 1 || D.Kind == 4`).
- Det er ikke feil for spillet. Ingen rute i d/e3 går gjennom en boks, og boksene er ment å være kodelåste.
- Men `bLocked` i `gen.inl` er misvisende for alle andre som leser det (Spillbarhet og bots).
- Gjør: sett `locked=True` for `glass_kode` i `derive()`, eller skriv i kap. 13 at `bLocked` bare gjelder alarmdøra og at
  `Kind` styrer resten.

**5. Lys lekker inn i basene og gjennom dekker.**
- Fil: `helgasetr_plan.py` `LIGHTS`.
- Seks korridorlys på P3 står på Z 13,2 med 6 m demping, 1,15–1,7 m fra basenes yttervegg på (±8 / 0, ±18,9). Uten skygge
  lyser de opp beina, L-skjermene og veggene inne i basene fra under gulvet. Planens regel «ikke nærmere basevegger enn
  3 m» er bare oppfylt i 2D.
- De 8 flatelysene har 26 m demping uten skygge og lyser gjennom mesanin og dekker ned i rommene på P1–P2.
- Gjør:
  - Regn avstanden i 3D (kule mot rom på andre plan).
  - Senk korridorlysene til ≤ 4 m, eller bruk spotlys rett ned.
  - Sett flatelysene til ca. 12–14 m demping.

## Forslag til neste runde
1. **4b: rett funn 1–3 først.** Det er små endringer i data og bygger. Ta så målingen fra «Ferdig når» i spillet:
   `stat unit` / `stat gpu` fra P4 nordkorridor og fra kantina, og at alle seks rutene kan gås uten å sette seg fast (en
   bot eller et hodeløst gangskript langs rutene).
2. **Id-manifest i `check_helgasetr.py`.** Lagre (nivå, rom, posisjon) → id for dører, glass, lettvegger og dekning i en
   fil som sjekkes inn. Sjekken skal feile når en eksisterende id flytter seg, med motprøve. Interaktiv (og senere
   Spillbarhet og bots) nøkler tilstand på id-ene. I dag merker ingen det når de nummereres om.
3. **Spawnrommene har bakdør mot en fienderute.** Døra S1→venterom (og N1→NØ-gangen) er skjult for sikt, men fienden kan
   gå rett inn i spawnrommet. Vurder at døra bare kan åpnes fra innsiden (rømningsdør, Interaktiv) eller at slusa er eneste
   vei inn. Lag sjekk d3: ingen fiende-rute når et spawnpunkt uten å gå gjennom egen sluse.

## Lærdom
- Er en bestilling LEVERT før byggingen starter, skal broen fjernes i samme runde, og en prøve som «kjører GameMode-sporene»
  skal kopiere koden slik den er nå, ikke planens beskrivelse av den.
- Kollisjon som slipper `Pawn` gjennom (kulisse) må ikke ligge rett over en flate som stopper `Pawn` og kan nås. Tynne
  vegg- og stolpetopper er gulv for CharacterMovement.
- Data som et annet lag leser (dører, lås, glass), må bety det samme hos begge. En «dør» uten hull, eller et låsefelt som
  tolkes ulikt, blir feil hos mottakeren selv om alle sjekker er grønne.
