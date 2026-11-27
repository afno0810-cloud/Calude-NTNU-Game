**GODKJENT** (men ikke prøvd i editorens Play – se «Må testes i PIE» nederst)

# Kontroll – Spillbarhet runde 5 (dc25c4f): ny hovedmeny, kampvalg i adressen, Test kartet, pause offline

Kontrollert ved å lese commiten (`git show dc25c4f`, hele `GSMenu.cpp`/`GSGameMode.cpp` fra commiten), ikke arbeidsmappen.
`Build.bat` er ikke kjørt (byggeren: Result: Succeeded; `UnrealEditor-Gloshaugen.dll` 10:23, commit 10:24). Motorpåstander er
sjekket mot UE 5.8-kilden (`HUD.cpp` 151–240: `OnHUDPostRender` sendes alltid, også med `bShowHUD = false` og under pause).

## Hodeløs test (UnrealEditor-Cmd `-game -nullrhi`, ingen editor kjørte, hver prosess stoppet etter 50–100 s)
Logger: `Saved/Logs/Kontroll5_Explore.log`, `Kontroll5_Bots.log`, `Kontroll5_Listen.log`. Ingen krasj, ingen Fatal/Ensure.

| Adresse | Hva loggen viser |
|---|---|
| `?Mode=Explore?Caps=999?Allies=-4` | «spill test kartet, 15 min, **10** erobringer … startutstyr av» (klemt), «Test kartet: startet ved rød base», «GSGameMode: Test kartet - ingen kamp, ingen skade». Kartet lastet (2,8 s). |
| `?Mode=Bots?Team=2?Min=1?Caps=0?Enemies=40?Skill=9?Kit=1` | «spill mot bots, **3** min, **1** erobringer … bots 4 + **10** (nivå **3**) - botene finnes ikke ennå, du spiller alene», «du er på Blått lag», og ca. 6 s etter lastet kart «startutstyr = Garand, M1911, 2 x Bandage» (nedtelling 5 s → kamp startet → utstyr). |
| `?listen?Mode=Explore?Min=5?Caps=1` | `Warning: Mode=Explore ignoreres: bots og test kartet bare når du spiller alene (nettmodus 2)`, «spill online, 5 min, 1 erobringer». |

Begrensning: kjøringene stopper på «Waiting on static mesh … being ready before playing» etter 1–305 bilder (meshbygging uten
skjermkort). Oppstart, faser, klemming og nedtelling er bevist; flyging, pause, menyklikk og sult er bare lest i koden. Kjøringene
brukte gamle campus (Helgasetr er ikke i bygget ennå).

## Andreas' faste regel
- **Forbedret:** hovedmenyens venstreliste (tre klare valg i stedet for «Start spill / Bli med / Øv alene»), vert og bli med på
  én side, faste 15 min / 3 erobringer blir valg (vert og bot-kamp), meny i spillet tilpasset det du gjør, hjelpetekstene leser
  `CaptureGoal`/`MatchMinutesSet`.
- **Nytt:** «Test kartet» (fri flyging, Gå til, Lys og vær, ingen skade/sult), bot-siden med lagret oppsett, bot-kamp uten lobby
  med startutstyr, pause offline, «Til hovedmenyen».
- Oppfylt.

## Det som ble sjekket særskilt

**Input og pause – kommer man alltid ut? Ja (i koden).**
- Pausen settes og slås av bare i `SyncModal`, og bare når `bPausedByUs` (en pause noen andre satte røres ikke).
- Alle veier som lukker noe kaller `SyncModal`: `CloseMain`, `Back` (Esc/F10/P), dialog Ja/Nei (OnKey kaller `SyncModal` til
  slutt), «ingen musepeker»-reserven, «ingen tastekrok»-reserven. Ny verden: `ResetWorld` nuller `bPausedByUs`, og en ny verden
  starter upauset.
- Menyen tegnes og tar taster under pause: tegningen henger på `AHUD::OnHUDPostRender` (kalles hvert bilde uansett pause og
  uansett `bShowHUD`, sjekket i motoren), så `IsModalState` (≤ 30 bilder siden tegning) holder seg sann; tastene kommer gjennom
  viewport-krokene, ikke Enhanced Input (som ikke trigger under pause). F10/P åpner og lukker som før i PIE.
- Nav.Fly og Gå til lukker menyen (pausen av) før de flytter figuren. `AGSExplore` tikker ikke under pause.
- Klokka står under pause: `GetServerWorldTimeSeconds` = verdenstid i standalone, og den går ikke på pause.

**Pause aldri online: OK.** `InLocalGame` krever `NM_Standalone` og `GameKind != Online`; en klient får alltid `Online` fra
`LocalKind` uansett hva som replikeres.

**Valgene i adressen kan ikke misbrukes: OK.** `InitGame` leser bare serverens egen kartadresse (klientenes innloggingsvalg går
ikke hit). `Mode=Bots/Explore` godtas bare i `NM_Standalone`, med andre kontroll i `BeginPlay` for PIE (setter tilbake til
Online, Lobby, ingen bots/utstyr). Alle tall klemmes (bevist i loggen). `Team`/`Allies`/`Enemies` brukes bare i bot-kamp.

**Fri flyging bare i test: OK.** Flyging krever `AGSExplore`, som bare lages av GameMode når `GameKind == Explore` (bare
standalone). N fanges bare når `LocalKind == Explore`; `SetClock`/`SetWeather` sjekker `GameKind == Explore`; Gå til/Lys
vises bare i test-menyen. I vanlig kamp og online går N videre til spillet som før.

**Nedsetting fra flyging: OK, med én liten rest.** `SafeSpotNear` prøver der du er, så 8 retninger × 0,5–3 m, så 0,5–3 m opp;
hvert punkt må ha ledig kapsel (`ECC_Pawn`, så dører og møbler teller) både i lufta og på gulvet under (≤ 30 m). Ellers siste
trygge stå-sted (oppdateres hvert 0,5 s på bakken, aldri under flyging). Teleport bruker `TeleportTo` (sjekker plass) og
faller tilbake. Rest: finnes verken ledig plass eller `LastSafe` (bare hvis du aldri har stått på bakken), blir du stående der du
er med kollisjon på (se F3).

**«Spill mot bots» er ærlig: OK.** Bot-radene er grå, siden sier «Bots kommer i neste oppdatering. Nå spiller du alene på
kartet», lobbypanelet sier «Botene kommer i neste oppdatering. Nå er du alene på kartet.», loggen sier det samme.
`bBotsAvailable` er én konstant (i `AGSGameState`, ikke `GSMenu` som planen sa – bedre, HUD-en leser den også).

**Helgasetr / faste koordinater: OK for denne runden.** Gå til, spawn og Explore-start leser `GSCampus::Bases()` (flagg i meter,
spawn 2D med flaggets Z). Med Helgasetr-tallene (flagg Z 13,9 m, dekket over 17,85 m): Gå til sporer fra 15,4 m og ned 6,5 m →
treffer gulvet på Plan 4, under taket. Også riktig med Grafikks midlertidige Z 13,4. Ingen nye faste tall fra gamle kartet.
(Gamle faste tall i `SpawnPickups` – mytisk kasse i Lysgården – er fra før og står i Grafikks åpne bestilling.)

**Andre «Ferdig når»-punkter lest i koden:** bot-kamp: lag fra menyen før kroppen lages (`PostLogin` før `Super`), klar med
en gang, 5 s nedtelling, ingen lagjevning, `bSoloMatch` av, etter resultatet 15 s lobby eller Klar. `StartMatch` bruker
`MatchMinutesSet`, slutt på `CaptureGoal`. Startutstyr bare i Countdown/Playing, slått opp i katalogen én gang. Explore: fasen
er ikke `Playing` → ingen skade/blødning, sult fylles hvert 5. s. `bOpenMainAfterTravel` er per vindu, nulles bare når den
brukes, og åpner startmenyen også i PIE. Fly-tasten er egen rad, kan byttes (bytter plass med en tast i bruk), P og F10 er nå
reservert, H/P/Esc/F10 avvises også ved innlesing. Logglinjene i 3.12 og 4.6 finnes.

## Funn (ingen stopper runden)
- **F1 – Ned i fri flyging virker bare med venstre Ctrl, ikke C** (`GSExplore.cpp`, `Tick`: `KeyFor(PC, "IA_Crouch")`).
  `ActionKey` gir første tast på handlingen, og `IA_Crouch` har LeftControl først og C etter (`GSPlayerController.cpp` l. 92–93).
  Tekstene viser riktig nok «Venstre Ctrl ned» (samme oppslag), men planen, `IMPROVEMENTS.md` 607 og Andreas' forventning sier
  «C ned», og på en laptop er C det naturlige. **Rett:** sjekk alle tastene til handlingen (som `IsActionKey`, f.eks. en
  `GSMenu::IsActionDown(PC, Action)`), for hopp og løp også; vis «C / Ctrl ned».
- **F2 – `IMPROVEMENTS.md` 598–613 er ikke i commiten** (ligger uendret i arbeidsmappen). Dirigenten må ta den med. Linje 607
  sier «C ned» – riktig først etter F1.
- **F3 – Reserve når ingen ledig plass finnes** (`StopFlying`): uten `LastSafe` blir figuren stående med kollisjon på, mulig i en
  vegg. Svært sjelden. **Rett:** siste reserve = `PickSpawnTransform` (egen base).
- **F4 (kosmetisk, se i PIE):** C/Ctrl og Mellomrom går også til Kamps huk/hopp/klatring under flyging. Klatring/glid settes
  tilbake til flyging innen 0,2 s (logges én gang); huk kan stå på når du lander. Bestill `bNoClip`-sjekk hos Kamp bare om det
  synes.

## Må testes i PIE (Andreas eller dirigenten – ingen har trykket Play på denne runden)
1. Play → F10 → «Test kartet» → «Gå inn i kartet»: panelet TEST KARTET, ingen lobby-boks.
2. N: flyging på, gjennom en vegg, Mellomrom opp, **Ctrl** ned (C virker ikke før F1). N igjen midt inne i en vegg: du står på et
   gulv ved siden av.
3. F10 → «Spillet står på pause» synes; F10/P lukker og alt går igjen. F10 → «Gå til» alle fire knapper, «Lys og vær» kl. 22 og
   Regn. «Til hovedmenyen» → Ja: kartet lastes, startmenyen står åpen, F10 lukker den.
4. «Spill mot bots» → «Start kampen»: grå bot-rader og teksten om at botene kommer; 5 s, klokke 10:00, gevær + pistol + 2 bandasjer.
   F10: klokka står. «Start kampen på nytt» → Ja.
5. Number of Players 2, Play As Listen Server: F10 i spillet skal aldri gi pause.

## Forslag til neste runde
1. F1 (alle taster for en handling) og F3 – små, kan tas først i plan-06.
2. Grafikks bestilling om `PickSpawnTransform` (spor fra +200 cm, ikke +400) bør tas nå som Helgasetr kommer: ellers spawner
   hele laget – og Test kartet – oppå flagget.
3. Loot/terminaler fra `GSHelgasetr` (Grafikks bestilling 1–2), ellers er bot-kampen på Helgasetr bare startutstyr.

## Lærdom
- En handling kan ha flere taster (huk = Ctrl og C). Kode som spør «er tasten for X nede» må sjekke alle tastene, ikke den første.
- Hodeløs `-game -nullrhi` stopper etter få bilder på «Waiting on static mesh»: den beviser oppstart, faser og klemming, ikke
  oppførsel over tid (tick, pause, flyging).
