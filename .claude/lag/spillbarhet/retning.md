# Retning for laget Spillbarhet

Eies av lagets planlegger. Andreas' ønsker i `.claude/loop/andreas.md` er rammen (én modus: Capture the Flag;
superrealistisk, veldig groteskt, svært høye standarder, minst én gammel ting forbedret og én ny per runde).
Hovedagentens prioriteringer i `.claude/lag/prioriteringer.md` styrer rekkefølgen.

## Visjon
Gløshaugen Strike er **Capture the Flag på et lite, detaljert utsnitt av campus**: to lag, to baser, ett flagg
hver. En kamp skal føles som en ekte, farlig kveld på campus, ikke som en arkadehall, og alt fra du starter
spillet til kampen er over skal henge sammen, forklare seg selv og være rettferdig.

1. **Du kommer inn uten hjelp.** Hovedmeny, innstillinger og taster som huskes, og spillet viser selv hva
   knappene gjør og hva CTF går ut på. Ingen skal trenge konsollen for å spille med venner.
2. **CTF med vekt.** Flagget er en ting i verden: det bæres synlig, bæreren er tregere, det blir liggende der
   han faller. Laget vinner på samarbeid (forsvar, eskorte, ruter gjennom bygg), ikke på reflekser alene.
3. **Ren start, lik for alle.** Jevne lag, spawn ved egen base, full loot, ingen feller fra oppvarmingen,
   sidebytte mellom kampene. Serveren bestemmer alt; ingen enkeltspiller kan ødelegge for resten.
4. **Det du finner og bærer er ekte.** Riktig kaliber, skade etter patron, vester som stopper det de faktisk
   stopper, utstyr du velger før runden. Kroppen er sårbar: blødning må stoppes, tung bør gjør deg treg.
5. **Grunn til å spille en gang til.** Tydelig kampbue, nøktern oppsummering, statistikk som bare låser opp
   utseende, og neste kamp er annerledes (side, vær, tid på døgnet).
6. **Tåler 20 spillere og juks.** Alt regnes på serveren, og nett og ytelse holder med fullt hus.

## Hva som finnes i dag (lest i koden 2026-11-26)
- `GSGameMode`: lobby → nedtelling → kamp → resultat → lobby, alle mot alle til 25 drap. 16 spawnpunkter på hele
  campus. Ca. 430 lootplasser og 16 låste kasser.
- `GSGameState` (fase, klokke, vær, drapslogg), `GSPlayerState` (drap, død, klar). Ingen lag.
- `GSItems` + `catalog.py` (ca. 170 gjenstander), `GSPickup` (svever og snurrer), `GSTerminal` (PC, 3D-printer).
- Hos Kamp: Esc-meny tegnet i `GSHUD` (vert, bli med siste adresse, grafikk, følsomhet), hjelpetekst i 30 s.
- Hos Grafikk (kommer): `GSCampus::PlayArea()` og `GSCampus::Bases()` (Vest = Gamle fysikk, Øst = Gamle kjemi).

## Neste ideer (backlog, viktigst øverst)

### Køen
1. **[plan-01, runde 1] CTF-grunnmuren.** Lag (`Team`), `AGSFlag` (ta, bære, miste, hente hjem, erobre),
   lagpoeng og kampslutt ved 3 erobringer, spawn ved egen base, statusstripe og lobby-forklaring i `GSHelp.*`,
   lagbytte med L, ren kampstart (funn 6, 7, 15, S1, S2 + Kamps bestilling). Se `ko/plan-01.md`.
2. **[plan-02, skrevet 26. nov.] Hovedmenyen (0b), lagvalg med mus og taster** i nye `GSMenu.*`, C++ uten .uasset.
   Se `ko/plan-02.md`. Kort: hovedmeny i standalone (Start spill med egne adresser, Bli med med tekstfelt og liming,
   Øv alene, Innstillinger, Kontroller, Avslutt), «Kobler til …» med 20 s-grense, feilmeldinger ved brutt/mislykket
   tilkobling (F1, F6), bekreftelse før man forlater en kamp, eget navn renset på serveren (F2), grafikk/skjermmodus/
   V-sync/bildefrekvens/følsomhet/lydstyrke lagret, taster som kan endres, lagvalg-panel med mus i lobbyen. Innputt via
   `UGameViewportClient`-krokene (svelger alt mens menyen er åpen, funn I3 for våre flater), tekst via egen
   `UGSGameViewportClient::InputChar`. Bare én liten bestilling til Kamp (Esc-menyens knapper og hinttekster); menyen
   fanger klikkene deres til den er levert.
3. **[plan-03 «3A», skrevet om 26. nov.] Knappehjelp (0a).** Se `ko/plan-03.md`. H-oversikt (taster fra
   `GSMenu::Controls`, CTF-reglene), 11 hint som vises høyst to ganger og aldri to ganger på rad (tastene fra
   `GSMenu::KeyName`), H reservert i menyen, «Vis hintene på nytt», beskjed ved sen innkobling (C2). IMPROVEMENTS 460–467.
4. **[plan-04 «3B», skrevet 26. nov.] CTF-regler som mangler + ytelse.** Se `ko/plan-04.md`. Overtid bare når flagget som
   er ute kan avgjøre (ønske 33), hjemmeflagg kan ikke tas når tiden er ute, flaggbæreren meldes etter 20 s som
   **radiomelding med sted** («Flagget sist sett: inne i Gamle fysikk, 2. etasje», hvert 5. s – ingen markør), `Logout`
   legger ned flagget (C1), og Grafikks ytelsesbestilling for `GSPickup`/`GSTerminal`. IMPROVEMENTS 468–475.
5. **[plan-05, skrevet 27. nov.] Ny hovedmeny: Spill mot bots, Spill online, Test kartet.** Se `ko/plan-05.md`.
   `GameKind` (Online/Bots/Explore) og vertsvalg (kamplengde, erobringer, venneild) som valg i adressen til `OpenLevel`,
   lest og klemt i `AGSGameMode::InitGame`; bot-siden (lag, antall bots per lag, vanskelighetsgrad, startutstyr); «Test
   kartet» (`EGSMatchPhase::Explore`, ingen skade/sult, fri flyging på N, «Gå til» basene, klokke og vær) i nye
   `GSExplore.*`; meny etter modus inne i spillet; pause i bot-kamp/test. IMPROVEMENTS 598–613.
6. **[plan-06, skrevet 27. nov.] Bots for CTF.** Se `ko/plan-06.md`. `AGSBotController` (AIController + crowd) i samme
   `AGSCharacter`; navnett laget i kode (`ANavMeshBoundsVolume` med `BrushBodySetup`-boks, `RuntimeGeneration=Dynamic`,
   reserve: invoker); via- og forsvarspunkter regnet fra navnettet (ingen koordinater → tåler Helgasetr); syn/hørsel uten
   veggsyn, reaksjonstid, siktefeil som krymper, egen rekyl, enkel dekning, medisin; `AGSBotTeam` fordeler roller; fire
   grader. Kroker bestilt hos Kamp (K1 `IsLocalHuman` + `friend`, K2) og Grafikk (G1–G4). IMPROVEMENTS 614–629.
7. **[plan-07, skisse] Bots med taktikk:** dører, eskorte, flanke, undertrykkende ild, granater, loot, nærkamp, bots i
   online-kamper. Se `ko/plan-07.md`.
8. **[plan-08] Resten av CTF for Helgasetr** (det gamle 4b): regel utenfor området trengs ikke inne (ytterveggene er ekte);
   sidebytte mellom kampene; respawn i bølger (10 s) aldri i sikte av fiende, kort spawnbeskyttelse; lag som står tomt;
   av/på-kobling i hendelseslista; 3 s ved mistet flagg hvis berøring er for lett; loot, PC-er og kameraer fra Grafikks
   `GSHelgasetr`-lister (`Loot()`, `Terminals()`, `Cameras()`), `AGSDoor` i egne filer og `AGSMapState::ResetForMatch()` i
   `CleanWorldForMatch` (Grafikks bestilling 27. nov., deres runde 5) – **dette kan måtte rykke foran bots** hvis Helgasetr
   lander uten loot (hovedagentens prioritering 27. nov.: Spillbarhet tilpasser spawn, loot, PC-er og kamplengde).
9. **[plan-09] Våpendata som følger virkeligheten (Kamps bestilling, funn R5–R7, R13) + loot for CTF.**
   - Kalibertabell i `catalog.py`: `MuzzleVelocity`, `BulletMassG`, `PenClass`, skade etter patron, riktige
     kalibre med egne ammoesker, `ProtClass` på vester og hjelmer, ekte prosjektilfarter.
   - `RandomLoot`: ammovekt per kaliber, sjeldenhet som ikke snus i små grupper, våpen oftere i låste kasser
     (Kamps kontroll-bestilling). Loot-tetthet for det lille kartet; det beste lootet i midtområdet, ikke i basene.
   - Medisin: «Skinne» og «Tourniquet»/trykkbandasje med `Kind` Kamp kan lese (avtales i `bestillinger.md`).
     Kikkert-teksten «Hold F» rettes (funn G6).
10. **[plan-10] Utstyr/klasse før runden.** Velg startpakke i lobbyen (f.eks. gevær, sanitet, støtte, speider),
   realistisk utstyr og vekt; loot blir tillegg, ikke grunnlag.
11. **[plan-11] Invitere venner, steg 1 (0c), inn på siden «Spill online»:** «Finn spill på nettverket» (LAN-søk) og kode/IP man kan kopiere;
   koden legges opp mot OnlineSubsystem-sesjoner så Steam/EOS kan komme når Andreas har konto.
12. **[plan-12] Lagverktøy:** markere fiender og ping for laget; kart/minikart med flagg, baser og lagkamerater
   (egne filer + ett bestilt tegnekall); nærradio/stemmechat (Unreal VoIP) som egen runde.

### Deretter
- **Loot som ligger på gulvet** (funn 11 + arkade-preg): `GSPickup` uten `Tick`, ting ligger stille på gulv/bord
  (server-linjespor ved utlegging), våpen på siden, skala maks 2×, delte materialer, nett-dvale. Nye API-er til
  Kamp: `CanBeReachedFrom(Eye)` (funn 8) og `ArmorPoints` på pickup (funn 1). Påfylling bare når ingen ser plassen.
- **Overlevelse som passer CTF:** blødning som må stoppes (regler og gjenstander hos oss, skadelogikk hos Kamp),
  vekt i kg på alt, medisin som tar tid. Sult/tørst betyr lite i en kamp på 15 min (funn B2): avklar med
  hovedagenten om det skal tones ned eller gjøres om til utholdenhet.
- **Boosts og printer** (funn B1, B3, R2, R3): tak på stabling, færre overnaturlige boosts, printer som lager
  troverdige ting og nok filament til at den brukes. PC-sporing er veggsyn for alle (B4, R4): begrens den.
- **Oppsummering og statistikk:** nøktern kampslutt (erobringer, returer, drap, treffprosent), statistikk og
  nivåer som bare låser opp utseende, lagret lokalt. Replay etter runden (stort – deles opp).
- **Kampflyt:** spiller som kobler fra/til midt i kamp, verten forsvinner (F6), avstemning om neste kamp.
- **Feiljakt som gjenstår i egne filer:** `GSTerminal` videre, lootplassering per bygg, til-/frakobling.

## Endringer i retningen
- 2026-11-27 (plan-06a/06b): **Loot, PC-er og kameraer i Helgasetr rykker foran alt annet** (Andreas' punkt 2, hovedagentens prioritering 10.10): Helgasetr er spillbart, men har bare startpistolen. Gamle plan-06/07 (bots) er overtatt av lag Bots; nummeret 06 brukes nå til loot (06a: steder etter rom, bedre lenger fra basene, balanse mellom lagene, ammo etter kaliber) og terminaler/stedsnavn/bestillinger (06b). Bots' kroker venter til Bots 01a er godkjent.
- 2026-11-27 (plan-05/06/07): **Andreas: «menysystem med spill mot bots, spill online og test mappet».** Hovedmenyen
  bygges om rundt tre valg (plan-05), bots kommer i to runder (plan-06 grunnmur, plan-07 taktikk). Vertsvalgene fra gamle
  4b er bakt inn i plan-05 (samme kode for bot- og vertskamper). Resten av gamle 4b er nå plan-08 og tilpasses Helgasetr;
  våpendata blir plan-09, resten ett hakk ned. «Test kartet» er også et verktøy for Grafikk (gå/fly rundt, lys og vær,
  «Vis navnett»). Alt i bot-koden er kartuavhengig: flagg, spawn og grenser fra `GSCampus`, ruter og forsvarspunkter
  regnet av navnettet ved hver kampstart. Bots får aldri informasjon en spiller ikke har.
- 2026-11-26 (etter forhåndssjekk av plan-03): **Delt i 3A (plan-03: hjelp) og 3B (plan-04: regler og ytelse)**, fordi
  runden var for stor. **Avslørt bærer vises som radiomelding med stedsnavn, ikke som markør** (hovedagentens valg: en
  firkant med meter gjennom vegger er veggsyn i praksis, siden nesten alle flaggløp varer over 20 s). Overtid spør nå
  «kan flagget som er ute avgjøre?», ikke bare «er et flagg ute?». Hint maser ikke (60 s og vilkåret usant imellom).
  Vertsvalg m.m. er nå plan-05, våpendata plan-06, og resten ett hakk ned.
- 2026-11-26 (plan-03): **Plan-03 er delt i to.** Knappehjelpen (0a) og de små CTF-reglene (avsløring etter 20 s,
  overtid, frakobling, sen innkobling) tas nå; vertsvalg, regel utenfor området, sidebytte og respawn i bølger er ny
  plan-04, fordi vertsvalget bygger på `GSMenu` som ennå ikke er kontrollert. Våpendata/loot flyttes til plan-05 og
  resten ett hakk ned. Sult- og tørst-hint er droppet (systemet virker ikke i praksis, funn B2). Oppdaget i motorkoden:
  pawnen slettes *før* `GameMode::Logout`, så død/drap/utstyr ved frakobling må gjøres i Kamps `PawnLeavingGame` (bestilt).
- 2026-11-26 (plan-02): **Taster lagres i `[Gloshaugen.Keys]` og settes rett inn i Kamps `UInputMappingContext`**
  (som lages i kode og er `public`), i stedet for Enhanced Input sine brukerinnstillinger. Brukerinnstillingene ville
  krevd `PlayerMappableKeySettings` på hver handling i Kamps fil, Config-endring og SaveGame (funn I2) – en stor
  bestilling for samme resultat. Kan flyttes dit senere uten å endre menyen. Menyen tar innputt gjennom
  `UGameViewportClient`-krokene og tegner via `OnHUDPostRender`, så den virker uten Kamps filer. Vertsvalg (venneild,
  kamplengde, erobringer, «Start nå») er flyttet til plan-03 for å holde plan-02 innenfor én runde.
- 2026-11-26 kl. 17.20+: **Andreas bestemte én modus, Capture the Flag.** Retningen er skrevet om rundt CTF.
  Plan-01 er CTF-grunnmuren (ikke hovedmenyen), fordi Kamp, Grafikk, menyens lagvalg og knappehjelpen alle
  trenger lagfeltene, flagget og reglene først, og fordi den kan spilles og testes alene uten andre lags filer.
  Hovedmenyen er plan-02. Knappehjelpen (H-oversikt, hint) er flyttet til plan-03; forklaringen av CTF
  (statusstripe og lobby-panel) kommer allerede i plan-01. Den gamle planen «ren kampstart» er bakt inn i
  plan-01; «loot på gulvet» og «ny spawn på hele campus» er nedprioritert (kartet blir mindre, spawn er nå base).
- 2026-11-26 (tidligere): 0a (knappehjelp), 0b (hovedmeny), 0c (invitere venner) og mindre kart kom inn øverst.
- 2026-11-26 (runde 1): visjon og backlog skrevet. 2026-11-26: laget opprettet.
