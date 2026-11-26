**MÅ FIKSES**

# Kontroll – Spillbarhet runde 1 (commit 0f788c6): grunnmuren for Capture the Flag

Kontrollert ved å lese commiten (`git show 0f788c6:<fil>`), ikke arbeidsmappen. Bygget er ikke kjørt på nytt
(byggeren: Result: Succeeded). Motorpåstander er sjekket mot UE 5.8-kilden (`HUD.cpp`, `GameViewportClient.cpp`,
`BaseEngine.ini`). Ingenting er spilt; alt er funnet ved lesing.

Grunnmuren er solid: tilstandsmaskinen, replikeringen, lagfordelingen, ryddingen og lobbyrettingene holder.
Det er **én regelfeil i kjernen** (F1) som må rettes før runden godkjennes. Den er på én linje. F2 og F3 bør tas
i samme retting, resten kan gå i neste plan.

## Andreas' faste regel
- **Forbedret (gammelt):** spawn (ved egen base, ledig plass, på bakken – før tilfeldig punkt, flere på samme
  koordinat), kampstart (rydder miner, ild, røyk, slupne ting og lik, fyller loot og kasser), lobbyen (tvungen
  nedtelling kan ikke stoppes, 90 s fra spiller nr. 2, solokamp avbrytes når en venn kommer), kampregler
  (lag og uavgjort i stedet for «vinner med 0 drap»), utendørs loot følger `PlayArea()`.
- **Nytt:** to lag med automatisk fordeling og bytte med L, `AGSFlag` (ta, bære, miste, hente hjem, erobre),
  lagpoeng og kampslutt ved 3 erobringer, statusstripe og lobby-innføring i `GSHelp`, datafeltene Kamp bestilte.
- Regelen er oppfylt.

## «Ferdig når», punkt for punkt
- Bygg: Succeeded (byggerens rapport). Filene i commiten = planens liste + `GSCampus.h` (Grafikks kartdata,
  forklart i commit-meldingen). OK.
- Lag før pawn: `RestartPlayer` kaller `AssignTeam` før `RestartPlayerAtTransform`. `StartMatch` jevner ut til
  forskjell ≤ 1 (hver flytting minker forskjellen med 2, løkka stopper). OK.
- Flagg bare av motstander, bare under `Playing`, erobring krever eget flagg `Home`: OK (`ServerStep`).
- Død/frakoblet bærer → `Dropped` innen 0,1 s og `CarriedFlagTeam` = 0: OK (se tabellen).
- Ingen ny kropp før `CleanWorldForMatch`: OK (steg 4 → 5 → 6 i `StartMatch`).

## Flaggets tilstandsmaskin – alle kanter
| Kant | Resultat |
|---|---|
| Bærer dør | `Die` → `DetachFromControllerPendingDestroy` → `PS->GetPawn()` er null → `Drop()` innen 0,1 s. Stedet er siste posisjon fra `UpdatePose` (høyst ett bilde gammelt), gulvet finnes med linjespor. Liket har kapsel uten kollisjon (`OnRep_Dead` kjøres også på serveren), så det sperrer ikke flagget. OK |
| Bærer kobler fra (funn C1) | Det finnes fortsatt **ingen `Logout`-overstyring**, men flaggdelen av C1 er løst ved polling: `AGSGameMode` er `AGameModeBase`, så `PlayerState` og pawn ødelegges ved frakobling, `IsValid(Carrier)` blir usann og flagget slippes der bæreren sto. OK. Resten av C1 (ingen drap, ingen loot, ingen melding) står åpent – se F5 |
| Bærer bytter lag | Kan bare skje i `Lobby` (flagg er da hjemme) og i `StartMatch` (etter nullstilling). `PS->Team == Team` → `Drop()` finnes i tillegg. OK |
| To tar flagget i samme bilde | Én løkke på serveren, første levende i rekka tar det, `return`. OK |
| Erobring når eget flagg ikke er hjemme | Ingenting skjer; stripa forklarer hvorfor. Skjer straks eget flagg er hentet hjem. OK |
| Begge flagg bæres samtidig | Låst til en bærer dør. Vanlig CTF. OK |
| Kampslutt mens flagg bæres | `EndMatch` → `ResetHome()` nullstiller `CarriedFlagTeam`. `ServerStep` kjører ikke utenfor `Playing`. OK |
| Ny kamp | `StartMatch` nullstiller poeng, `CarriedFlagTeam`, flagg (`CleanWorldForMatch`) og drapslogg. OK |
| Flagg mistet i fritt fall / utenfor kartet | Ingen gulvtreff innen 6 m → flagget henger i lufta der bæreren var, og går hjem etter 180 s. Lavt, se F6 |
| Forsvarer står på eget flagg | **Feil, se F1** |

## Replikering (vert, klient, sen klient)
- `Team`, `State`, `Carrier`, `DropLocation` er replikert med `OnRep_Look`; flagget er `bAlwaysRelevant`. `BeginPlay`
  hos klient overskriver ikke replikert tilstand (bare `LastCarrierLocation` og et nytt `OnRep_Look`). Sen klient
  får riktig tilstand, farge og stripe. OK.
- Verten: serveren kaller `OnRep_Look()` selv etter hver endring (`Announce`, `ResetHome`), og fargen settes etter
  `Flag->Team = Team` i `SpawnFlags`. OK.
- `PlayerState::GetPawn()` virker hos klienter (settes av pawnens `OnRep_PlayerState`). Er bæreren utenfor
  relevansavstand, står flagget igjen i lufta der han sist ble sett (se F7).
- Poeng, lag og `CarriedFlagTeam` er replikert til alle. `MulticastFlagEvent` er pålitelig og sjelden. OK.
- Dedikert server: `EnsureHooked` kalles ikke, og det finnes uansett ingen HUD der. OK.

## Funn

**F1. En forsvarer som står på sitt eget flagg gjør det umulig å ta – MÅ FIKSES (Høy, regelhull)**
- Fil: `GSFlag.cpp`, `ServerStep`, linja
  `if (World->LineTraceTestByChannel(Loc, FlagLoc + FVector(0,0,60), ECC_WorldStatic, Params)) continue;`
- Årsak: sporet går på *kanalen* WorldStatic, og spillerkapsler (profilen `Pawn`, `BaseEngine.ini` linje 3110)
  blokkerer alle kanaler unntatt Visibility. Bare spilleren selv og flagget er unntatt. Står en levende
  forsvarer på flaggplata, ender sporet inne i kapselen hans og alle angripere hoppes over. Det samme skjer når en
  hvilken som helst levende spiller står mellom angriperen og stanga. Å stå ved flagget er det mest naturlige en
  forsvarer gjør, så dette blir oppdaget i første kamp.
- Rettelse: spor mot verdensgeometri, ikke pawns:
  `LineTraceTestByObjectType(Loc, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params)` (campus er
  `BlockAll` = objekttype WorldStatic). Bruk samme objektspor i `Drop()` (gulvet) og i `SpawnFlags`.

**F2. Statusstripa dekker kompasset – bør rettes sammen med F1 (Middels, synlig)**
- Fil: `GSHelp.cpp` stripe `Y = 8`, høyde ≥ 34, bredde 520; `GSHUD.cpp` `DrawCompass` `Y = 18`, høyde 22, bredde 400
  (Kamp). Stripa tegnes etter HUD-en (via `OnHUDPostRender`) med 78 % dekkende panel, så kompasset nesten
  forsvinner for alle som har kompass i sekken. Byggeren meldte selv avviket.
- Rettelse (egen fil): flytt stripa under kompasset når den lokale figuren har kompass
  (`AGSCharacter::HasItemKind("compass")` er offentlig), eller bestill hos Kamp at kompass og kampklokke flyttes
  ned når `GSHelp::Draw` tas inn i `DrawHUD`. Ikke la det stå til «senere» uten bestilling.

**F3. `ServerRequestTeam` har ingen sperre mot gjentak – bør rettes (Middels, juks/belastning)**
- Fil: `GSGameMode.cpp` `RequestTeam`. Reglene er riktige (bare `Lobby`, bare lag 1/2, aldri skjevere, tak 10,
  bare eget `PlayerState` kan kalle RPC-en). Men ved ulikt antall (2–1, eller alene) er byttet lov *hver* gang:
  2–1 → 1–2 → 2–1 … Hvert bytte ødelegger og lager en ny figur. En klient kan sende RPC-en hundrevis av ganger
  i sekundet og få serveren til å lage figurer i løkke (og teleportere seg mellom basene i lobbyen).
- Rettelse: tidsstempel per spiller på serveren, f.eks. minst 2 s mellom godkjente bytter.

**F4. Sju spawnplasser per base, inntil ti spillere per lag (Lav)**
- `PickSpawnTransform` bruker basens 7 punkter. Ved kampstart med 8–10 på et lag er alle opptatt for de siste,
  som får «litt til siden» (ett tilfeldig forsøk per punkt, 2–3,5 m) og til slutt flaggpunktet inne i bygget
  (flere kan havne der samtidig). Plassene er sjekket mot vegg og andre spillere, så ingen havner i en vegg, og
  punkter som treffer et tre forkastes (stammekolliderne er over 4 m, kapselen overlapper). Be Grafikk om minst
  10 punkter per base, eller prøv flere sideforsøk før reserven.

**F5. Frakobling: teksten blir feil, og resten av C1 står igjen (Lav)**
- `Announce(Dropped, …, Old = nullptr)` skriver «Blått lag [mistet] det blå flagget» i hendelseslista (det var
  motstanderen som bar det), og stripa viser bare «Det blå flagget» uten verb (`GSHelp.cpp`, grenen
  `E.Who.IsEmpty()` dekker bare `Returned`). Skriv «Det blå flagget ligger ute».
- `Logout` mangler fortsatt: ingen drap til siste skytter, ingen loot, ingen «forlot kampen». Ikke i planen,
  men hold funnet åpent.

**F6. Flagg mistet høyt over bakken (Lav)** – `Drop()` sporer bare 6 m ned. Faller bæreren fra et tak og dør i
lufta, blir flagget hengende utilgjengelig i 180 s. Spor lenger (f.eks. 50 m) så det lander på bakken.

**F7. Båret flagg når bæreren er utenfor syne for klienten (Lav)** – flagget tegnes stående i lufta på siste
kjente sted. Skjul `FlagRoot` når `State == Carried` og pawnen ikke er kjent på maskinen.

**F8. Småting**
- Lobby-panelet sier «L bytter lag» også under nedtelling, der L ikke virker.
- `GSHUD` skriver fortsatt «Først til 25 drap eller 15 minutter» rett ved siden av CTF-panelet, og venneild
  virker fortsatt til Kamp leverer. Begge er bestilt (øverst i `bestillinger.md`) og nevnt i planens risiko.
- Stripa tegnes også når `bShowHUD` er av (kroken kjøres uansett). Ufarlig nå.

## Det dirigenten ba om å sjekke særskilt
- **Spawn ved base:** ute rundt eget bygg (9–15 m fra veggen), på bakken via linjespor, kapseltest mot alt som
  blokkerer og 150 cm til levende spillere, vendt mot midten (`SpawnYaw`). Ikke inni vegg. Verten spawner første
  gang før campus er bygget (motoren logger ham inn før `BeginPlay`); da brukes Z = 120 som før. OK.
- **Flagg inne i byggene (byggerens avvik):** flaggpunktet (±125, 10) er midt i bygget, i tverrkorridoren som
  `FurnishBuilding` holder fri (±1,5 m). Sporet starter 3 m opp, under dekket i 2. etasje (3,7 m), og treffer
  gulvet. Campus bygges synkront i `UGSWorldSubsystem::OnWorldBeginPlay`, som motoren kjører før
  `GameMode::BeginPlay`. OK. Usikkert bare hvis en Blender-modell (`SM_Bygg_Gamle_fysikk`) har vegg eller trinn
  akkurat i midten – Andreas bør se at flagget står på gulvet og kan nås.
- **Duk bakover:** `LastCarrierYaw + 180` med duken på +X gir duk bak bæreren, over hodet (stangtopp 2,7 m over
  gulvet). Skjult for bæreren selv, også på verten. OK.
- **`CleanWorldForMatch`:** samler først, sletter etterpå. Sletter alle `AGSProjectile`, pickups med
  `bRespawns == false` (bare `SpawnDropped` lager slike), og alle `AGSCharacter` – på det tidspunktet er alle
  spillerfigurer allerede fjernet, det finnes ingen boter, så bare lik går. Kasser låses og fylles (`Roll`).
  Terminaler, kameraer, campus og flagg blir stående. Sletter ikke noe den ikke skal. OK.
- **Lobbyrettingene:** funn 7 (tvungen nedtelling), S1 (90 s fra spiller 2, også på nytt etter `BackToLobby`) og
  S2 (solokamp avbrytes) er riktig lagt inn. `PhaseEndsAt` i lobbyen leses ikke av `DrawMatch`. OK.
- **Stripa via `OnHUDPostRender`:** registreres én gang (statisk flagg), kalles én gang per lokal HUD per bilde,
  aldri på dedikert server. Når Kamp kaller `GSHelp::Draw` selv, hopper kroken over (bildeteller), og L-tasten
  har egen vakt mot dobbelt kall. `DebugCanvas` har samme størrelse og DPI-skala som HUD-lerretet
  (`GameViewportClient.cpp` linje 1522 og 2166, `DebugCanvas.cpp` linje 256). Ikke dobbelt. OK.
- **Ytelse, 20 spillere:** to flagg × 10 sjekker/s × inntil 32 figurer, spor bare innen 1,3 m. Stripa gjør tre
  små aktørsøk per bilde. Ubetydelig. Kampstart ruller all loot på nytt i ett bilde (ett lite rykk, én gang).

## Forslag til neste runde
1. Lagfarge og venneild er det som mangler mest for at CTF skal kunne spilles med flere enn to: følg opp
   bestillingen hos Kamp, og vis i mellomtiden lagkameraters navn i lagfarge fra `GSHelp` (egen fil).
2. `Logout` i `GSGameMode` (resten av C1): drap til siste skytter, slipp utstyret, «X forlot kampen».
3. Regel for dem som går ut av `PlayArea()` og sidebytte mellom kampene (Grafikks bestilling c og e).

## Lærdom
- Et linjespor på kanalen `ECC_WorldStatic` treffer også spillerkapsler. «Er det vegg imellom?» skal spørres med
  objektspor (`LineTraceTestByObjectType`) eller med alle figurer unntatt.
- Tegner et lag noe på skjermen fra egne filer, må planen liste hva de andre lagene allerede tegner på samme
  sted (kompass, klokke, lister) – ikke bare det som alltid vises.
- Hver ny server-RPC som lager eller flytter en figur trenger en tidssperre per spiller.
