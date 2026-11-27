**Del 1 (ec133e1, M1–M8 + 3A): GODKJENT · Del 2 (a4e315f, hastefiks): GODKJENT, men ikke prøvd i PIE · Del 3 (abb7780, runde 4): GODKJENT**

# Kontroll – Spillbarhet: menyrettinger + runde 3A (ec133e1), hastefiks for menyen (a4e315f), runde 4 / «3B» (abb7780)

Kontrollert ved å lese commitene (`git show <commit>` og `git show abb7780:<fil>`), ikke arbeidsmappen. Bygget er
ikke kjørt (byggeren: Result: Succeeded for alle tre). Motorpåstanden om editorens tastekrok er sjekket mot UE 5.8-kilden
(`SLevelViewport.cpp` 4940–4975, `GameViewportClient.cpp` 725–792). Input er også lest slik det står etter
`c10df95` (F10/P, WASD lukker lagpanelet, Kamps `IsGameInputBlocked`), fordi det er den tilstanden Andreas spiller med.
Andreas' logger fra i morges er lest (`Saved/Logs/Gloshaugen.log` og to backup-logger 07.38). Ingenting er spilt.

## Andreas' faste regel
- **3A – forbedret:** lobby-panelet (H-linja), Kontroller-siden (H-rad, «Vis hintene på nytt», tre kolonner i lite
  vindu), sen innkobling (egen linje + «ble med på» i lista). **Nytt:** H-oversikten «Taster og regler», 11 hint.
- **Runde 4 – forbedret:** kampslutt på tid, frakobling av bæreren, loot/kameraer/lys (ytelse). **Nytt:** overtid og
  «Flagget sist sett».
- Menyrettingene og hastefiksen er rettinger, ikke runder; regelen gjelder ikke dem. Oppfylt der den gjelder.

## Del 1 – ec133e1: M1–M8 og 3A

| Funn fra runde-02 | Status |
|---|---|
| M1 Grafikk | **Rettet.** Raden leser og stepper fra `GetShadowQuality()` (settes av samlenivået, rører ikke oppløsningen). Kamp har gjort det samme i `GetGraphicsLevel`. Rundtur: nivå N → skygge N → viser N, for alle fem. |
| M2 Bildefrekvens | **Rettet.** Menyen kaller `PC->Bildefrekvens()` og viser `GetFrameLimit()`, trinn 30–240 uten «Ubegrenset». Én eier (Kamp). Rest: `Bildefrekvens` skriver en chatlinje; `SetFrameLimit` er `public` og stille – bruk den. |
| M3 «i kamp» mens man kobler til | **Rettet.** `InMatch(W, S)` gir `bConnectFromMatch` mens tilkoblingen venter, og krever nettdriver for `NM_Client`. `bConnectFromMatch` settes før `bConnecting` (riktig rekkefølge). |
| M4 vertsfeil / port | **Rettet.** `NetDriverListenFailure` hoppes aldri over; faktisk port leses fra nettdriveren og vises når den ikke er 7777. |
| M5 tidsvindu | **Rettet.** `StartJoin` nuller `LeavingUntil`. |
| M6 sperre i lagvalget | **Rettet.** «Vent litt» i 2 s etter eget klikk. |
| M7 Klar-tast | **Rettet.** Spillerens egen `IA_Ready`-tast virker i lagpanelet, Enter også. |
| M8 lite vindu | **Rettet.** Reglene droppes først, så tre kolonner. Regnet for 620 px Play-rute: med reglene ute og to kolonner blir radhøyden ca. 18 px (under 22) → tre kolonner hvis bredden tåler 210 px per kolonne; ellers klemmes til 16 px og skala 0,62. Får plass. |
| M10 småting | Navnesperre 2 s, bare bokstaver/tall/mellomrom/`-_.` på både klient og server, ett navnefelt, konsollen kan skrives i, glidebryter slippes ved sidebytte. OK. |

**3A, «Ferdig når» lest i koden:**
- Flagghint (`OwnTaken`, `Carry`, `EnemyFlag`) har `bLobbyToo = false` og hoppes over utenfor `Playing`. OK.
- Telles etter 2 s (`HelpCountAfter`); avbrutt før 2 s gir verken telling eller 60 s-sperre (`HelpStopHint`). OK.
- Samme hint kommer ikke igjen før vilkåret har vært usant **og** 60 s er gått (`SeenFalse` + `LastShown`). OK.
- H er reservert i `BuildRows`, eldre lagring av H hoppes over i `ApplySavedKeys` og i `TeamKey()`. H-raden står i
  `GSMenu::Controls()`. OK.
- Alle taster i hint og PC-/printerlinjer kommer fra `GSMenu::KeyName` (med standardtast før konteksten finnes). OK.
- Oversikten åpnes ikke med sekken, Kamps meny, terminal eller modal meny. Sen innkobling: avgjøres ved første tegning
  med `GS`, 10 s starter først når laget er kjent. `ResetHints()` tømmer hele seksjonen (også `OverviewOpened`). OK.
- `RestartPlayer`: «ble med på» bare når laget var `None` og fasen `Playing`, 2 s timer med svak peker. OK.

## Del 2 – a4e315f: menyen i editorvinduet

- **Påstanden om motoren stemmer.** `SLevelViewport::StartPlayInEditorSession` binder sin egen `OnOverrideInputKey`
  før spillet tegner første bilde. Den gamle sjekken `if (!IsBound())` gjorde at vår krok aldri kom på i «Selected
  Viewport» – akkurat Andreas' feil. `InstallHooks` legger vår krok foran og kaller den gamle etterpå; håndtaket sjekkes
  hvert bilde, så binder editoren på nytt, kjedes vi på nytt. `Depth`-vakten hindrer at `OnKey` kjøres to ganger hvis
  noen pakker oss inn igjen. Riktig.
- **Sikringene er gode:** modal krever (a) tegning siste 30 bilder, (b) at vår krok er den vinduet kaller, (c) at
  musepekeren er på. Mangler (b), lukkes alt i `Draw`. Mangler (c) ved åpning, lukkes menyen straks. `SyncModal` slår
  pekeren på igjen hvis noe annet har slått den av mens menyen er åpen.
- **Esc/F10/P lukker alt i ett trykk** (dialog, felt, side, meny, lagpanel); bare «venter på ny tast» bruker tasten som
  «avbryt». Ingen automatisk hovedmeny i PIE. Loggingen er nyttig og billig (bare ved hendelser).
- **Andreas' test var i eget spillvindu (`-game`), ikke i editorens Play.** Loggene 07.38–07.40 viser kommandolinja
  `-game` og «tastekrok koblet til vinduet» *uten* «(foran en krok …)», altså var det ingen editorkrok å kjede på.
  Klikk (Nav.Host, Nav.Solo, Nav.Quit, Dlg.Yes), Esc åpne/lukke og «input sluppet til spillet» virket der. Selve
  kjedingen – det hastefiksen handler om – er riktig ifølge motorkoden, men **ikke prøvd**. Lærdommen sier at input
  må prøves i PIE. Det er ikke en kodefeil, men Andreas bør teste Play i editorvinduet: F10 eller P åpner menyen,
  klikk virker, F10/P lukker, og loggen skal si «(foran en krok som var der fra før, f.eks. editorens)».

**Kan menyen fortsatt sluke input i noen tilstand?** Lest gjennom alle veier i `OnKey`/`OnAxis`/`SyncModal` (etter
c10df95) – ingen funnet der man står fast:
- Modal er bare sann når vi tegner, eier kroken og pekeren er på; ellers slippes alt (også musebevegelse og hjul).
- Hver modal tilstand har tastaturvei ut: F10/P (og Esc i ferdig spill) lukker alt; lagpanelet lukkes også med
  gåtastene (tasten går videre til spillet etter `FlushPressedKeys`, så gangen starter med en gang), lagtasten og Enter.
- P er sperret for tastebytte (originaltastene til `IA_Menu` legges i `Reserved`), og P i et tekstfelt skrives som tegn.
- Kamps Esc-meny (`bMenuOpen`): vår krok slipper F10/P videre, så `ToggleMenu` lukker den. På terminal går tasten også
  videre og lukker terminalen. Ny figur mens Kamps meny er åpen: gange/blikk sperres igjen (`AcknowledgePossession`).
- Kamps `GSCharInputBlocked` bruker samme `IsModal` som kroken, så spillhandlingene og kroken er enige; slipp
  (StopFire o.l.) går alltid gjennom.

## Del 3 – abb7780: runde 4

**Overtid, alle kanter (regnet):**
| Tilfelle | Resultat |
|---|---|
| Rødt 2–1, rødt bærer blått flagg når tiden går | D(blått) = 1−2 = −1, rødt flagg hjemme → ingen overtid, rødt vinner. OK |
| 1–1, et flagg ute | D = 0 → overtid. OK |
| Laget som bærer ligger 1–2 | D = 1 → overtid (kan utligne). OK |
| Laget som bærer ligger 0–2 | D = 2 → ingen overtid. OK |
| Erobring i overtid | `Capture` → flagget hjemme → neste tick: ikke avgjørende lenger → `EndMatch`. Poenggrensen sjekkes først. OK |
| Det avgjørende flagget hentes hjem, det andre er fortsatt ute | `AnyDecisiveFlagAway()` brukt begge steder → slutter straks. OK |
| Flagg tas fra hjemme mellom `PhaseEndsAt` og GameMode-tick | `bHomeLocked` sjekker selve tidspunktet (E2). Liggende flagg kan fortsatt plukkes. OK |
| Begge flagg ute | Ingen kan erobre før eget flagg er hjemme; taket på 120 s avgjør. OK |
| Uavgjort etter overtid | Tom vinner = uavgjort, som før. OK |
| Venn kommer inn i en solokamp under overtid | Faller til `BackToLobby`, som nuller `bOvertime`. OK |
| `bOvertime` henger igjen | Nullet i `StartMatch`, `EndMatch`, `BackToLobby`. OK |
| Samme klokke | GameMode og flagget bruker begge `GetServerWorldTimeSeconds()`; flaggets `ServerStep` kjører bare i `Playing`. OK |

**«Sist sett» lekker ikke til feil lag.** Linja tegnes bare når `Own = FindFlag(PS->Team)` er båret og avslørt, altså
for laget som eier flagget (bærerens motstandere). Bærerens eget lag ser sitt eget flagg, og en spiller uten lag finner
intet flagg. `bRevealed` nulles i `Take`, `Drop`, `ReturnHome`, `Capture`, `ResetHome`; `TakenAt` settes i `Take`, så
klokka starter på nytt for hvert løp. Avsløringen oppdateres hvert 5. s fra føttene (riktig etasje). Selve punktet
replikeres til alle (flagget er alltid relevant) – kjent og godtatt i planen (N1 hos Kamp).
`PlaceName` regnet for planens fire punkter: (−125, 10, 0) → «inne i Gamle fysikk, 1. etasje»; (−125, 10, 4,5) →
«2. etasje»; (0, −30, 0) → «ute, nord for Hovedbygningen»; (125, 30, 0) → «ute, sør for Gamle kjemi». Riktig.
Rotasjonen `RotateAngleAxis(−Yaw)` er motsatt av FRotator-yaw, riktig.

**Logout + Kamps `PawnLeavingGame`: ingen dobbel nedlegging.** Rekkefølgen er `PawnLeavingGame` → `DieLeavingGame` →
`Die` (som ikke rører flagget) → `Logout` → `DropIfCarriedBy`: flagget er fortsatt `Carried` med samme `Carrier`, så
`Drop()` kjøres én gang. Kjøres flaggets egen sjekk først (bæreren er død), er tilstanden `Dropped` og `Logout` gjør
ingenting. Det hele skjer i samme kall, så ingen `ServerStep` kommer imellom. `Drop` tåler at pawnen er borte
(bruker `LastCarrierLocation`). Lista får «X [mistet] flagget» og «X [forlot] kampen» (+ Kamps drapslinje) – tre ulike
ting, ikke dobbelt.

**Ytelse:**
- Loot våkner: tick blir aldri slått av (bortsett fra dedikert server); avstand sjekkes 2 ganger i sekundet, også for
  ting som er tatt eller låst, så en ting som kommer tilbake eller låses opp snurrer igjen innen 0,5 s. Grensa skaleres
  med FOV (4× kikkert → 120 m). Vinkelen fra klokka gir et lite hopp når en ting våkner, men det skjer på 30 m+ og
  synes ikke. Ingen andre leser `Mesh`-rotasjonen. OK.
- Kameraer: serveren bruker ikke kamerarotasjonen til noe (bare `ServerViewCamera` for posisjon), så at verten lar
  fjerne kameraer gå på 2 Hz er ufarlig. Det røde lyset blinker uregelmessig bare utenfor 40 m, der det uansett er
  skjult (`MaxDrawDistance` 30 m). Lysene slukker altså ikke feil. Liten rest: bytter man til et kamera på PC-en, kan
  bildet stå stille i inntil 0,5 s før kameraet våkner.
- Skjermlyset på PC/printer: av over 40 m (fade 32–40 m). Uten skygge fra før. OK.

## Funn (ingen hindrer godkjenning)

**F1. Hintet blir stående når vilkåret forsvinner (Lav–middels, synlig).** `GSHelp.cpp` `HelpUpdateHints`: et hint som
vises sjekker bare `bAllowed`, ikke sitt eget vilkår. «Tomt magasin. R lader.» står i 7 s også etter at man har
ladet; «Gå helt bort til flagget» står etter at man har tatt det; «E tar våpenet» etter at det er tatt. Rettelse: avslutt
hintet (med telling hvis 2 s er nådd) når vilkåret har vært usant i ca. 1 s.

**F2. Retning fra byggets midte gir rare stedsnavn ved lange bygg (Lav).** `HelpPlaceName`: retningen regnes fra
midten, ikke fra fotavtrykket. Bærer ved (50, −20) – rett nord for fasaden til Hovedbygningen (110 m lang) – blir
«ute, øst for Hovedbygningen». Bruk de lokale avstandene `DX`/`DY` til fotavtrykket (som allerede regnes) for
himmelretningen.

**F3. PIE er ikke testet (prosess, middels).** Se del 2. Andreas' eneste test etter hastefiksen var med `-game`.

**F4. Småting.**
- `Set.Fps` bruker `Bildefrekvens()` som skriver en chatlinje; bruk `SetFrameLimit()` (public, stille).
- Navnesperren (2 s) kan sluke et navn skrevet rett etter at menyen sendte navnet ved innlogging: feltet viser det nye,
  serveren beholder det gamle. Les navnet tilbake fra `PlayerState` når feltet mister fokus.
- `IMPROVEMENTS.md` 461 sier «seks korte linjer»; oversikten har nå åtte regler. Runde 4-seksjonen ligger under Kamps
  runde 13, ikke øverst (rekkefølge ved commit, ufarlig).
- «Tiden er ute [overtid] til flagget er hjemme» i hendelseslista leses litt klosset; «Overtid: kampen spilles til
  flagget er erobret eller hjemme» som linje uten «drapsform» ville vært bedre.

## Forslag til neste runde
1. F1 og F2 (små, i `GSHelp.cpp`), og test i editorens Play: F10/P, klikk, H, hint – med logg.
2. Plan-05 slik planlagt: lag som blir tomt midt i kampen, og at av/på-kobling fyller hendelseslista (slå sammen
   «forlot»/«ble med på» for samme navn innen 30 s).
3. «Flagget sist sett» også som punkt på PC-kartet (bestilt hos Kamp) – og at Grafikk sier fra hvis navn eller
   fotavtrykk i `GSCampus::Buildings()` endres (stedsnavnene følger dem).

## Lærdom
- En rettelse av input er ikke prøvd før den er prøvd i den modusen feilen kom i: les kommandolinja i loggen
  (`-game` eller PIE) før en test regnes som bevis.
- Et hint eller en melding som vises en stund må også forsvinne når grunnen til den forsvinner, ikke bare når tiden går.
