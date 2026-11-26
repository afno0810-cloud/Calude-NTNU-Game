**MÅ FIKSES**

# Kontroll – Spillbarhet runde 2 (commit ffa99ef): hovedmenyen, taster, lagvalg med mus + rettinger fra runde 1

Kontrollert ved å lese commiten (`git show ffa99ef:<fil>`), ikke arbeidsmappen. Bygget er ikke kjørt på nytt
(byggeren: Result: Succeeded). Motorpåstander er sjekket mot UE 5.8-kilden (`GameViewportClient.cpp`, `HUD.cpp`,
`World.cpp`, `UnrealEngine.cpp`, `GameUserSettings.cpp`, `Scalability.cpp`, `InputMappingContext.h`,
`IpNetDriver.cpp`, `BaseScalability.ini`). Ingenting er spilt; alt er funnet ved lesing.

Menyen er godt bygget i kjernen: innputt-fangeren er tett, tilstanden er per vindu, tastebyttet er riktig koblet til
Kamps mapping-kontekst, bekreftelsene og feilmeldingene henger sammen, og alle fire rettingene fra runde 1 er
riktige. Det som stopper godkjenning er **to innstillinger som ikke virker slik de står på skjermen** (M1 Grafikk,
M2 Maks bildefrekvens). Begge er små rettinger i `GSMenu.cpp`. M3–M5 bør tas i samme retting.

## Andreas' faste regel
- **Forbedret (gammelt):** Bli med (ekte tekstfelt i stedet for konsoll), bekreftelse før man forlater/avslutter
  (også fra Kamps Esc-meny), lagbytte (panel i stedet for blind L), spillernavn (renset og unikt på serveren),
  følsomhet med glidebryter, strålesporing huskes, flagget kan tas selv om en forsvarer står på det (F1),
  lagbytte har tidssperre (F3), flagg som mistes i lufta lander på bakken (F6), riktig tekst ved frakobling (F5).
- **Nytt:** hovedmeny, «Kobler til …» med avbryt og tidsgrense, feilmeldinger, lydstyrke, taster som kan endres,
  Kontroller-oversikt, lagvalg med mus, `UGSGameViewportClient` for ekte tegn (æøå).
- Regelen er oppfylt.

## Rettingene fra runde 1
| Funn | Status |
|---|---|
| **F1** forsvarer på flagget | **Rettet.** `ServerStep` bruker `LineTraceTestByObjectType(…, ECC_WorldStatic)`. Spillerkapsler er objekttype `Pawn` (`GSCharacter.cpp` 160) og treffes ikke lenger. |
| F1 – åpner det for flagg gjennom vegg? | **Nei.** Alt som bygger campus bruker profilen `BlockAll` (objekttype WorldStatic): alle sju stedene i `GSCampusBuilder.cpp` (70, 149, 384, 417, 786, 812, 922) og PC-ene (`GSTerminal.cpp` 44). Det eneste som nå slipper gjennom er `PhysicsActor` (løse magasiner o.l.), `BlockAllDynamic` (prosjektiler) og figurer – ingen av dem er vegg. Blir dører/glass senere laget som `WorldDynamic`, må sporet få den typen i tillegg (meld fra til Grafikk/Kamp). |
| **F3** tidssperre lagbytte | **Rettet.** 2 s per spiller, sjekkes etter regelsjekken og før figuren slettes; kartet ryddes ved > 64. Se M6 om at panelet ikke viser sperren. |
| **F5** tekst ved frakobling | **Rettet.** «Det blå flagget [ligger ute]» i hendelseslista og «… ligger ute» i stripa. |
| **F6** flagg mistet høyt | **Rettet.** 50 m objektspor ned; treffer første gulv under bæreren. |
| F2 (stripe over kompass) | Løst av Kamp (linje 441: kompasset flyttet). |
| F4, F7, F8, `Logout` | Ikke i denne runden (ligger i plan-03). |

## «Ferdig når», punkt for punkt
- Bygg: Succeeded (byggerens rapport). Filer: planens liste + `GSFlag.cpp` (rettinger), `GSItems.cpp` (to linjer,
  navnebytte for unity-bygg) og en uplanlagt blokk i `DefaultEngine.ini` (se M9).
- `OnKey` gir `false` for alt når ingenting er modalt, unntatt lagtasten i lobby/nedtelling og de tre fangede
  klikkene i Kamps Esc-meny; `true` for alt (også slipp, gjentak, gamepad) når noe er modalt. `SyncModal` kaller
  `FlushPressedKeys` begge veier. **OK.**
- Ny kamp / bli med i en annen / forlat / avslutt går via dialog når verden er vert eller klient, også fra Kamps
  Esc-meny (`GetHitBoxAtCoordinates` før `PlayerController::InputKey`, så `MenuClick` aldri kjøres). **OK.**
  (Konsollkommandoene `Host`/`Join` går fortsatt utenom – greit.)
- Lagknappen bruker nøyaktig samme uttrykk som `RequestTeam`; panelet lukkes når fasen blir `Playing`. **OK.**
- Ingen konsollkommando trengs. **OK.**
- «Grafikk Lav → bildet endres» og «maks bildefrekvens … lagret»: **ikke oppfylt, se M1 og M2.**

## Funn

**M1. Grafikk-raden viser nesten alltid «Lav» og kan ikke settes høyere enn Middels – MÅ FIKSES (Høy, synlig)**
- Fil: `GSMenu.cpp` `ChangeSetting` (`Set.Graphics`, linje 821–828) og `DrawSettings` (linje 905).
- Årsak: raden leser `PC->GetGraphicsLevel()` = `GetOverallScalabilityLevel()`. Motoren
  (`Scalability.cpp` 1083–1097) gir −1 når oppløsningsskalaen ikke er nøyaktig nivåets egen verdi, og nivåenes
  verdier er `50 71 87 100 100` (`BaseScalability.ini` 37). Menyen setter nivået og legger så spillerens egen
  oppløsning tilbake (byggerens avvik «behold oppløsningen») – dermed stemmer de nesten aldri, −1 klemmes til 0,
  og raden viser «Lav».
- Regnet ut:
  - Ferdig spill, Kamps standard (Høy + 72 %): viser «Lav». «>» → nivå 1 + 72 % (≠ 71) → viser «Lav» igjen.
    Neste «>» regner fra 0 igjen → nivå 1. Høy, Episk og Film kan aldri velges.
  - Play i editoren, f.eks. Episk + 100 %: viser «Episk». «<» → nivå 2 + 100 % (≠ 87) → viser «Lav» (er egentlig
    Høy). «>» → nivå 1. Andreas ser dette i første test.
  - Oppløsning-raden flytter også Grafikk-raden til «Lav» som bivirkning.
- Rettelse (egen fil): ikke les samlenivået. Les ett delnivå som ikke blandes med oppløsningen, f.eks.
  `Gus->GetShadowQuality()` (eller minste av `GetViewDistanceQuality/GetShadowQuality/GetTextureQuality/…`), både
  for visning og som utgangspunkt for `+Dir`. Behold resten (sett samlenivå, legg oppløsningen tilbake).

**M2. «Maks bildefrekvens» glemmes hver gang et kart lastes, og viser feil verdi – MÅ FIKSES (Middels, to eiere)**
- Filer: `GSMenu.cpp` `Set.Fps` (linje 840–847, 929–933) mot Kamps `GSPlayerController.cpp` `BeginPlay`
  (247–250) og `ApplyFrameLimit` (333–356).
- Årsak: Kamp eier taket i `[Gloshaugen] FrameLimit` (standard 60, klemt 30–240) og setter `t.MaxFPS` i **hver**
  `BeginPlay` for den lokale kontrolleren; i ferdig spill skriver den også verdien inn i `GameUserSettings`.
  Menyen lagrer bare i `GameUserSettings`. Følger:
  - Velg 144 i hovedmenyen → «Start som vert» laster kartet → ny kontroller → `t.MaxFPS = 60`, og i ferdig spill
    overskrives og lagres 60. Valget forsvinner altså akkurat idet kampen starter. Samme ved Bli med og Forlat.
  - Play i editoren: raden viser editorens egen verdi (ofte «Ubegrenset») mens det virkelige taket er 60.
  - «Ubegrenset» (0) strider mot Kamps begrunnelse for taket (laptopen blir varm), og Kamp klemmer til 30–240.
  - IMPROVEMENTS 409 sier «maks bildefrekvens, lagret mellom øktene» – stemmer ikke.
- **Forslag til eier: Kamp.** Én kilde (`[Gloshaugen] FrameLimit` + `t.MaxFPS`, som også virker i editoren).
  Menyen blir bare ansiktet: kall `PC->Bildefrekvens(N)` (er `public`, lagrer og bruker verdien) og vis
  `PC->GetFrameLimit()`. Trinn 30 · 60 · 90 · 120 · 144 · 240, uten «Ubegrenset». Kan gjøres i dag uten å vente.
  Bestill gjerne en stille variant hos Kamp (`SetFrameLimit(int32)` uten `ClientMessage`), og at Kamp fjerner
  skrivingen til `GameUserSettings` hvis de vil – men det haster ikke.

**M3. Venstremenyen bytter til «i kamp»-utgaven mens man kobler til fra hovedmenyen (Middels, synlig)**
- Fil: `GSMenu.cpp` `InMatch()` (linje 599–603), brukt i `DrawMain`, `LeaveText`, `TryJoin`, `HandleClick`.
- Årsak: mens en tilkobling venter, utleder motoren nettmodus fra den ventende adressen
  (`World.cpp` 9668–9678): en verden som er alene rapporterer `NM_Client`. I inntil 20 s viser menyen da
  «Tilbake til kampen / Ny kamp som vert / Bli med i en annen / Forlat kampen» (grået ut), og Avslutt-dialogen sier
  «Du forlater kampen du er i.» Skjer ved hver tilkobling. (Byggeren har alt tatt høyde for dette i
  tidsgrensa med `ConnectWorld` – bra – men ikke i `InMatch`.)
- Rettelse: `InMatch` = verden har nettdriver (`World->GetNetDriver() != nullptr`) i tillegg til modus, eller
  `InMatchFor(S, World)` som gir `S.bConnectFromMatch` så lenge `S.bConnecting`.

**M4. «Kunne ikke starte som vert» blir aldri vist, og porten kan være en annen enn 7777 (Lav–middels)**
- `Host.Go` og dialog `Host` setter `LeavingUntil = nå + 10 s` rett før `PC->Host()`. `NetDriverListenFailure`
  sendes under kartlastingen (`World.cpp` 7967–7969), altså innenfor de 10 s, og `SkipFailure` kaster den.
  Spilleren havner i hovedmenyen igjen uten forklaring. Rettelse: ikke hopp over `NetDriverListenFailure`.
- Dessuten: er 7777 opptatt, binder motoren stille neste ledige port (`IpNetDriver.cpp` 839, `BindNextPort`), så
  «Port 7777 er i bruk» inntreffer nesten aldri, og en ekstra kopi på samme maskin blir vert på 7778 mens siden
  fortsatt sier 7777. Vis den faktiske porten når den ikke er 7777 (`World->URL.Port` / nettdriverens adresse).

**M5. Vinduet «vi forlot med vilje» (10 s) skjuler også ekte feil for neste forsøk (Lav)**
- Forlat kampen (eller start som vert) og prøv Bli med innen 10 s til en adresse som feiler med en gang
  (ugyldig adresse, avvist): `SkipFailure` kaster feilen, `bConnecting` blir stående, motoren laster standardkartet,
  og menyen viser «Kobler til … » til 20 s er gått. Rettelse: `StartJoin` setter `S.LeavingUntil = 0`.

**M6. Lagvalg-panelet kjenner ikke 2 s-sperren (Lav)** – knappen er åpen, klikket gjør ingenting, ingen
forklaring. Skjer når man er alene eller lagene er 2–1 og man bytter fram og tilbake. Husk eget siste klikk i
`FState` og vis «Vent litt» (sperret) i 2 s. Serveren avgjør uansett.

**M7. Enter i lagvalget er fastkodet, hintet er ikke (Lav)** – nederst står «%s klar» med tasten fra `IA_Ready`,
men `OnKey` sjekker `EKeys::Enter`. Har spilleren flyttet Klar til en annen tast, sier panelet én tast og lytter
på en annen (den nye tasten svelges). Sammenlign med mappingens tast i tillegg til Enter.

**M8. Kontroller-siden blir trang i et lite vindu (Lav–middels, «ferdig»-standard)** – 25 rader i to kolonner.
Regnet ut for høyre panel: 1080 px høyde → 28 px rader, god plass. 720 px → 18,6 px rader og tekst i skala 0,62
(ca. 9 px). 620 px (vanlig Play-rute i editoren på laptop) → innholdet er 423 px mot 416 px plass og renner ut av
panelet nederst; 500 px → ca. 100 px utenfor skjermen. Dropp Capture the Flag-boksen når høyden er liten, eller
bruk tre kolonner / blaing. Andreas tester med Play i editoren, så dette er det han ser.

**M9. Uplanlagt blokk i `Config/DefaultEngine.ini` (Lav, rydding)** – seksjonen
`[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]` (14 linjer med et `SecurityToken`) er
skrevet av editoren og kom med i commiten. Den hører ikke til runden og filen er felles («bare små tillegg»).
Dirigenten bør ta den ut i en egen ryddecommit. Selve `GameViewportClientClassName`-linja er riktig; at
`[/Script/Engine.Engine]` er ny seksjon er greit.

**M10. Småting**
- Navnebytte har ingen tidssperre på serveren (`ChangeName`). Ufarlig for ytelse (navnet replikeres med
  `PlayerState` sin vanlige takt, unik-løkka er høyst 100 × 20 sammenligninger), men et navn kan byttes i ett sett
  for å forvirre. Legg 2–3 s sperre per kontroller når `bNameChange` er sann. Rensingen slipper gjennom usynlige
  tegn (U+200B, U+00A0) og like-utseende bokstaver; vurder å bare tillate bokstaver, tall, mellomrom og `-_.`.
- `UGSGameViewportClient::InputChar` tar tegnene før konsollen når menyen er åpen: i editoren kan man åpne
  konsollen (§), men ikke skrive i den, og tegnene havner i navnefeltet. Slipp gjennom når konsollen er aktiv.
- Navnefeltet tegnes to ganger på Innstillinger-siden (venstre panel og egen rad, samme id, to markører).
- Lydstyrken settes først ved første tegnede bilde i hver verden (et kort øyeblikk full lyd), og i editoren blir
  den stående på editorens lydenhet etter Play.
- Et adressefelt uten punktum («andreas-pc») tolkes av motoren som kartnavn og gir «Kartet kunne ikke lastes /
  Verten bruker et kart du ikke har» – misvisende. Si «Sjekk adressen» når adressen mangler punktum/kolon.
- Slipper man museknappen utenfor vinduet, kan en glidebryter bli hengende til neste klikk (`SetPage` nullstiller
  ikke `DragSlider`).
- Kamps HUD sier fortsatt «Trykk Enter når du er klar» og «Start server» (bestilt, åpen bestilling).

## Det dirigenten ba om å sjekke særskilt
- **Kan spillet få tastetrykk mens menyen er åpen?** Nei, ikke funnet noen vei. Motoren kaller
  `OnOverrideInputKey` før `PlayerController::InputKey` (`GameViewportClient.cpp` 786–800) og
  `OnOverrideInputAxis` før akser (871–874). Tastetrykk, slipp, gjentak, museknapper, hjul, museblikk, gamepad og
  HUD-klikk (sendes fra `PlayerController::InputKey`) stoppes. Kamp poller ingen taster direkte
  (`IsInputKeyDown`/`WasInputKeyJustPressed` finnes ikke i `GSHUD`, `GSCharacter`, `GSPlayerController`). Taster
  som holdes når menyen åpnes, slippes med `FlushPressedKeys`. Konsollen og Alt+Enter/F11 går foran oss (greit).
- **Kan menyen bli hengende modal?** Ikke funnet. Modal krever at vi har tegnet de siste 30 bildene. Lagvalget
  lukkes selv når kampen starter. Alle dialoger lukkes med Esc/Enter. Etter respawn settes markør og modus på
  nytt bare hvis noe er åpent. Ny verden starter alltid i spillmodus. Eneste sted uten tastaturvei er roten av
  hovedmenyen alene (krever museklikk på «Øv alene») – som planlagt. Museposisjonen skaleres mot lerretet, så
  DPI-skalering tåles.
- **Esc:** dialog → avbryt; lytter på tast → avbryt; tekstfelt → slipp fokus (navnet lagres); underside → rot;
  «Kobler til» → avbryt; hovedmeny i kamp → lukk; lagvalg → lukk; ellers Kamps Esc-meny. Konsekvent.
- **To vinduer i PIE:** tilstanden ligger per `UGameViewportClient` (delt peker i statisk kart, ryddes ved > 8),
  krokene bindes per vindu, mapping-konteksten lages per kontroller (`NewObject(this)`), så tastene deles ikke
  feil. Delt mellom vinduene er bare det som skal være likt: lagret oppsett, adresselista, lydenheten.
  PIE startet som «Listen Server» kan rapportere feil nettmodus etter «Forlat kampen» – test feilflyten også med
  «Standalone Game» (står i planens risiko).
- **Tilkobling:** avbryt nullstiller `TravelURL` og kaller `CancelPending`; tidsgrensa (20 s) sjekkes etter «vi er
  inne», så en treg kartlasting gir ikke falsk feil. Fra en kamp er den gamle forbindelsen allerede stengt av
  motoren, og avbryt laster kartet alene – riktig håndtert (avvik fra planen, og nødvendig). Feilmeldingene er
  norske, motorens tekst vises bare som liten detalj. Se M3–M5.
- **Navn:** rens på både klient og server (styretegn, `" ? = # \`, maks 20, trim), tomt navn beholder det gamle,
  unikt uten hensyn til store/små («Kari 2»), navn fra URL går samme vei (`InitNewPlayer` → `ChangeName`). OK.
- **Tastebytte:** `GetMapping(i).Key` er samme liste som ombyggingen leser (`InputMappingContext.h` 219–220,
  `EnhancedInputSubsystemInterface.cpp` 988). Lagres som avvik i `[Gloshaugen.Keys]`, brukes én gang per kontekst
  (altså for hver ny kontroller/verden) og overlever respawn og ny kamp fordi konteksten bor på kontrolleren.
  Konflikt løses ved bytte (også mot lagtasten), Esc/F10/mus/hjul er sperret, tilbakestilling tømmer seksjonen
  og legger originalene tilbake. Modifikatorene på WASD følger med. OK. Kjent rest: en lagret tast kan kollidere
  med en ny standardtast Kamp legger til senere (ingen sjekk ved lasting).
- **Lagvalg mot serveren:** samme regel, serveren avgjør, panelet tåler ny figur. Se M6, M7.
- **Ytelse med menyen lukket:** per bilde ett kartoppslag, to `IsBound`, noen sammenligninger og én
  museposisjon; ingenting tegnes. Per tastetrykk ett oppslag i config (lagtasten). Ubetydelig, også med 20.
- **Dedikert server:** `EnsureHooked` kalles ikke, ingen visningsklient lages, `StateFor` gir null uten vindu.
  `ChangeName` og tidssperren kjører på serveren. OK.
- **Nøktern og ferdig?** Fargene, panelene og tekstene er rolige og ensartede (ingen blink, ingen lyd, dempet
  rav som eneste aksent). Men M1, M2, M3 og M8 er nettopp slikt Andreas ser med en gang, så «ferdig» er den ikke
  før de er rettet. Kan ikke bedømmes ved lesing: om tittelen i skala 1,9 er skarp med motorens skrift, og hvor
  mye av Kamps lobbytekst som skinner gjennom lagvalg-panelet (86 % dekkende). Andreas bør se på begge.

## Byggerens avvik fra planen
Byggerens egen liste ligger ikke i repoet; dette er avvikene som kan leses ut av koden.
| Avvik | Vurdering |
|---|---|
| Navngitt navnerom `GSMenuImpl` i stedet for anonymt (+ navnebytte i `GSItems.cpp`) | Riktig, unity-bygg. |
| `SyncModal` som følger tilstanden, i stedet for `SetModal(på/av)` | Bedre; tåler respawn og ny verden. |
| Modal krever tegning siste 30 bilder | Bra sikring. |
| `ConnectWorld`/`bConnectFromMatch`, og kartet lastes alene ved avbrutt forsøk fra en kamp | Nødvendig; planen tok feil om at den gamle kampen levde videre. |
| `bMsgPending`: meldingen vises når det finnes en skjerm | Bra. |
| Skjermmodus sperret i editoren | Bra (ville endret editorvinduet). |
| `ApplyNonResolutionSettings` + «behold oppløsningen» ved nivåbytte | Tanken er god, men gir M1. |
| Bare første lokale spiller per vindu, museposisjon skalert til lerretet, `<`/`>` som piler, `\` og tegn 127 renses, IPv4 filtrert på tekst | Greit. |

## Forslag til neste runde
1. Finn spill på nettverket og kopierbar adresse/kode (0c steg 1), og vis den faktiske porten (M4).
2. Følg opp bestillingen hos Kamp: hinttekstene i HUD leser `GSMenu::KeyName`, og Esc-menyen får én «Hovedmeny»-knapp.
3. Tastaturvalg i menyen (piltaster + Enter) så roten av hovedmenyen ikke krever mus.

## Lærdom
- En innstillingsrad må prøves som rundtur i motoren: sett verdien, les den tilbake med samme kall som raden
  viser, for alle trinn. Samlegettere (f.eks. samlet grafikknivå) gir −1 når én del avviker.
- Før en ny innstilling lages: søk etter hvem som alt skriver samme konsollvariabel eller config-nøkkel. To eiere
  betyr at den ene overskriver den andre ved neste kartlasting.
- Nettmodus er ikke «er jeg i en kamp»: en verden som venter på tilkobling rapporterer klient. Bruk nettdriveren
  eller egen tilstand.
- Et tidsvindu som demper feil («vi forlot med vilje») må nullstilles når spilleren starter noe nytt, og aldri
  dempe feil fra handlingen som åpnet det.
