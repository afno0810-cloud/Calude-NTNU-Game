**GODKJENT** (med én tekstretting i `IMPROVEMENTS.md` før den committes, se funn 1)

Kontrollert: commit `8ab2084` (GSMapState, GSInteract, GSDoor, logg). Kontrolløren har lest alle seks filene i commiten,
Kamps `Interact`/`ServerInteract`, `GSMenu::KeyName`/`IsActionDown` og `AGSPlayerController::IsGameInputBlocked` i commiten,
og Grafikks Helgasetr-data i arbeidsmappen (`GSHelgasetr.h`, `GSHelgasetr.gen.inl`, `helgasetr_plan.py`).

**Egen hodeløs kjøring** (ingen UnrealEditor kjørte; `UnrealEditor-Cmd -game -nullrhi -ExecCmds="Interaktiv.Selvtest"`):
`Interaktiv: 54 dører (25 låst, 24 glassdører med kodelås) [server]`, bruk-tasten (E) bundet, testrommet bygd (6 dører),
**17 OK, 0 FEIL**. Ingen nye feil i loggen.

## Fast regel (forbedret + nytt)
- **Forbedret:** dirkesett og adgangskort åpner nå kodelåser på dører (før bare kasser); bruk-tasten E virker på dører,
  med hint i samme stil og plass som Kamps. Begge kan prøves i spillet nå.
- **Nytt:** `AGSMapState` (én replikert liste), `AGSInteractor`, `AGSDoor` med fire dørtyper, låser, spark, rømning,
  branndører, testrom og seks konsollkommandoer. Regelen er oppfylt.

## Forhåndssjekkens 7 feil – alle rettet
1. **E ved slipp:** `bPressMine` avgjøres i `OnInteractStarted` (ny fokus samme bilde); ikke vårt → hele trykket ignoreres
   til tasten slippes. Kamps `Interact` bruker `FocusPickup`/`FocusTerminal`, og vi viker når en av dem er satt, så de to
   bindingene kan aldri begge gjøre noe på samme trykk. Holdt.
2. **Sperrer:** `IsGameInputBlocked() || bShowInventory || IsOnTerminal()`, død/mangler pawn og `UseItemId` – både for fokus,
   trykk, hint og tegning. `IsGameInputBlocked` = `bMenuOpen || GSMenu::IsModal`, så F10/P-menyen og modale menyer i PIE
   stopper alt. Input og tegning bare når `IsLocalController()`, `PopInputComponent` i `EndPlay`. Meny midt i et hold
   sender `HoldEnd`. Holdt.
3. **Ferske effekter:** ikke ferskt ved første replikering (`bFirstRepDone` settes i `PostNetReceive`, etter
   FastArray-tilbakekallene), før registeret finnes, via `ApplyAllLocal`/`PendingKeys`, uten `GameState`, og bare
   `0 ≤ dt < 2 s`. Holdt.
4. **Innestengt:** `EscapeSide()` (baksiden = inne i rommet, forsiden for branndører) – `Tap` og `HoldStart` åpner alltid
   derfra, låsen forblir låst. `GetUsePoint` følger brukerens side. Holdt (se også «innestengt» under).
5. **Spor gjennom glass:** fokus og server bruker samme objektspor (WorldStatic/WorldDynamic/Destructible), døra og brukeren
   unntatt. Holdt.
6. **K1 `To = ImpactPoint`:** står i bestillingen til Kamp. Holdt.
7. **K3 slag gjennom glass:** `ApplyMelee` returnerer avstand (`BIG_NUMBER` i 1a), og bestillingen ber Kamp forkaste treffet.
   Holdt.
De mindre punktene er også tatt: ruta er en `UStaticMeshComponent` (Destructible) på bladet, emissiv lysdiode,
branndøra venter på tom døråpning, `PredictUse` bare når ulåst/innside, bots med `!IsPlayerControlled()`.

## Byggerens seks avvik – vurdering
1. **Alarmdøra åpnes fra forsiden (K9):** riktig. Grafikks regel er at forsiden peker *ut av rommet døra tilhører*; for
   «Trapp sør» er det lobbyen. K9 skal hindre at blått lag kommer *opp* trappa inn i rødt lags lobby. Fra lobbyen (forsiden)
   går man ut i trappa med panikkstanga (rømning), og fra trappa er døra stengt. Det er det koden gjør.
   Merk: Grafikk lager ikke hull for `laast` (`helgasetr_plan.py`: `if kind != "laast"`), så dør 31 står inne i en 25 cm
   betongvegg (boks 1297). Den er usynlig og helt død (fokus, smell og kuler stoppes av veggen). Ufarlig, se funn 4.
2. **Jobbavbrudd 2,5 m** (planen: 1,8 m): godtatt. Målt fra øyet til brukspunktet i håndtakshøyde, så 2,5 m er ca. 2,4 m
   vannrett. Litt rausere enn planen, men jobben krever fortsatt at tasten holdes. Se forslag 3.
3. **`Interaktiv.Selvtest`:** bra, beviser dørlogikken hodeløst. Den går rett på `ServerUse`, så den prøver *ikke*
   inputveien (E, `bPressMine`, sperrer) og ikke klienter. Se forslag 1.
4. **`GSMenu::KeyName`/`IsActionDown`:** godtatt. Spillbarhets offentlige funksjoner, følger tastebytte, samme navn
   (`IA_Interact`, laget av `MakeAction`) som Kamps binding.
5. **`ServerJobStarted` i grensesnittet:** godtatt (flagg og lyd ved start hører hjemme der).
6. **Ingen `using namespace`:** godtatt (lærdom om Unity-bygg).

## Særskilte sjekker
- **Input:** se punkt 1–2 over. Én bitteliten svakhet: slippes E mens en modal meny sluker tastene, kan `bPressActive`
  henge til neste trykk, og da mistes ett trykk. Ingen dobbel handling. Ikke prøvd i PIE (kan ikke gjøres hodeløst).
- **Replikering:** én `bAlwaysRelevant` FastArray med bare endrede deler; dørene er lokale aktører med samme nøkkel overalt.
  Vert: `SetPart` kaller `ApplyLocal(true)` selv. Klient: `PostReplicatedAdd/Change`. Sen klient: alt i første pakke gjelder
  bare sluttutseendet; registeret ventes på (`PendingKeys` + `ApplyAllLocal`); testrommet bygges i `BuildRegistry` hvis
  `bTestRoom` kom først. Tilbakestilling: fjernede elementer → `ResetPartLocal`, og `OnRep_ResetCount` → `ResetLocal` +
  `ApplyAllLocal`, så begge rekkefølger ender riktig. Ingen rekkefølgefeil funnet.
- **Kollisjon/rubberbanding:** bladet ignorerer `Pawn` mens det går og blokkerer når det står, likt på alle maskiner. En
  klient som ser døra lukket når den er åpen på serveren blir bare stoppet (ingen retting). Retting kan bare skje i vinduet
  på ca. RTT/2 når en dør *lukkes* (godtatt i forhåndssjekken), og ved en forutsagt åpning serveren avviser (sjelden). Ok.
- **Juks:** klienten sender bare nøkkel + verb. Serveren sjekker levende, `UseItemId`, avstand ≤ 2,5 m fra *serverens*
  øyepunkt, samme objektspor, tidssperre 0,15 s, kort/dirkesett i sekken, og fullfører jobber på serverklokka (dirking 6 s,
  brekking 4 s, spark 1 s × 3). Fri linje sjekkes ikke på nytt under jobben, og ikke at spilleren ser mot døra (begge små).
- **Innestengt:** gått gjennom alle 54 dører og testrommet. Alle låste glassdører åpnes innenfra; ingen låste tredører i
  dataene; branndører og dobbeldører er ulåste; dør 31 står i en vegg uten hull. Testrommets trapperom har to utganger.
  Ingen kan bli stengt inne. (En vanlig dør som lukkes oppå en spiller i døråpningen skyves ut av bevegelseskoden; se forslag 2.)
- **Ytelse med 54 dører:** dørene tikker bare når de går, har tidtaker eller noen dirker. Fokus 20 Hz bare hos eieren,
  spor bare for kandidater innen 2,2 m. Bots 5 Hz (figurer × 54, billig). Ca. 400 kube-komponenter og 130 kollisjonsbokser
  i alt, alle `Movable` og med skygge, også 2 × 14 × 2 cm håndtak. Godt nok for 1a (1b bytter til modeller), se forslag 4.
- **Står dørene i åpningene?** Regnet ut med et eget skript mot `BoxesM`/`PanelsM` (åpningen fri, vegg på begge sider,
  overligger, gulv, og svingrommet 0–95° på begge sider):
  - **53 av 54 står riktig i en åpning** med fri sving. Dør 6–8 (Grupperom 2.1–2.3) står i glassvegger (`PanelsM` 84/85 osv.),
    riktig.
  - **Dør 31** (Trapp sør, P4, `laast`) står inne i betongveggen 1297 (ingen åpning, med vilje fra Grafikk). Ufarlig.
  - **Dør 23, 25 og 28** (Grupperom 4.V2, 4.V4, 4.O1): hengselen sitter inntil skilleveggen mellom to grupperom (1245, 1251,
    1267). Ved 95° går bladet ca. 7–8 cm inn i den 12 cm tykke veggen. Bare utseende.
  - **Dør 49** (Treningsleiligheten): rett ved siden står en 2,4 m bred åpning uten dør (Y −3,0…−0,6). Grafikks data, ikke vår feil.
  - Retningen stemmer: forsiden peker ut av rommet i alle stikkprøver (3.N1, 3.S1, 2.1, 4.V1), så kodelåsen står i gangen og
    rømningssiden er inne i rommet.

## Funn
1. **`IMPROVEMENTS.md` 653, 655, 658, 660 lover ting som ikke skjer i spillet ennå** (lærdom runde 2). Ingen fil hos Kamp
   kaller `ApplyBulletHit`/`ApplyBlast`/`ApplyMelee` (K1–K3 ikke levert), og det finnes ingen bots ennå. Så «skytes opp ved
   låsen» (653), «eller skytes i stykker» (655), «Granater blåser opp dører» (658) og «Botene åpner ulåste dører» (660) virker
   bare via konsollen eller senere. **Må rettes før IMPROVEMENTS committes** (bare tekst): skriv f.eks. «klart for granater
   (virker når Kamp kobler på K2; prøv nå med Interaktiv.Smell)», eller flytt linjene til runden der kroken leveres.
2. **`GSMapState::ApplyMelee` slår gjennom vegger** (`GSMapState.cpp` l. 765–772): sweepen spør bare etter `WorldDynamic`,
   så et slag mot en vegg kan treffe en kodelås eller et dørblad bak veggen (kodelåsen sitter 14 cm fra karmen). Ikke i bruk
   før Kamp legger inn K3, men rett det før: ta med `WorldStatic` i sweepen og bruk bare treffet hvis det er en `AGSDoor`.
3. **`GSInteract::BotUse(…, HoldStart)` returnerer `true` uten å gjøre noe** på låste dører (`GSInteract.cpp` l. 438–447:
   jobben kastes). Bestillingen til Bots lover at den «sparker inn tredører, bryter opp kodelåser». Returner `false` når
   `R.Job` er satt (eller kjør jobben på serveren), og si det i bestillingen.
4. **Dør 31 inne i betongvegg:** hopp over `laast`-dører som ikke har hull, eller be Grafikk om hull + karm (da virker den som
   en ekte enveis alarmdør, som K9 vil). Liten.

## Forslag til neste runde
1. **Bevis inputveien hodeløst:** en `Interaktiv.Selvtest input` som sender E inn gjennom spillerens input
   (`UEnhancedInputLocalPlayerSubsystem::InjectInputForAction` eller `PlayerInput->InputKey` trykk/slipp) med en pickup foran
   en dør, med inventaret åpent og med `bMenuOpen`, og logger hva som skjedde. Andreas tester ikke selv nå, og lærdommen sier at
   kompilering ikke beviser input. Gjerne også en hodeløs vert + klient (`-server` / `127.0.0.1 -game`) som logger dørtilstand
   hos begge.
2. **Dører som lukkes oppå noen:** la `Tap`/lukk vente (som branndøra) når en `Pawn` står i svingboksen, og begrens åpningen til
   ca. 90° der hengselen sitter inntil en vegg (dør 23/25/28). Send `OnWalkableChanged` også ved tilbakestilling.
3. **Lyd og billig tegning:** Kamps bestilling (linje 6 i `bestillinger.md`) bruker andre navn (`Door_Wood_Open`,
   `Door_Glass_Open`, `Door_Metal_Stair`, `Door_Pushbar`, `Codelock_Beep` …) enn vi ber om (`Door_Open` …). La
   `PlayPartSound` prøve de art-spesifikke navnene først. Slå av skygge og sett tegneavstand (ca. 25 m) på håndtak, kodelås og
   lysdiode. Vurder 2,0 m og fri linje i jobbavbruddet.

## Lærdom
- Selvtester som kaller serverfunksjonen direkte beviser logikken, ikke inputen: en runde med ny tastebinding trenger også en
  hodeløs test som går gjennom selve tasten.
- Kartdata fra et annet lag kontrolleres geometrisk (åpning fri, vegg på begge sider, svingrom) med et lite skript mot de
  genererte boksene, ikke bare ved å lese tallene.
