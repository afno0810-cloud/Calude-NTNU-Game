**BØR ENDRES** – forhåndssjekk av `ko/plan-05.md` (Ballistikk del 1), lag Kamp

Lest mot koden i `HEAD` (f6682cf), `ko/plan-04.md`, `catalog.py`, `GSEffects.cpp`, `funn.md` og `laerdom.md`.
Retningen er riktig og fysikken stemmer (regnet etter: 9 mm 19,5 cm lavt på 80 m, 7,62 NATO 56 cm på 300 m,
skadefaktor 7,62 på 300 m = 0,71). Men planen har ett jukshull som gjør hovedpoenget (V2) virkningsløst, tre
ting som vil merkes dårlig i spill, og den er for stor til ett steg. Ingenting er bygget ennå, så dette er billig å rette nå.

## A. Må endres i planen (viktigst først)

1. **HØY (juks): klienten velger selv `ShotIndex`, og dermed spredningen.** Serveren krever bare
   `ShotIndex > LastServerShotIndex`. Klienten kjenner `SpreadKey` og kan regne ut `ShotSeed(Key, i)` for
   tusen i-er fram og sende den i-en som gir en kule midt i siktet. Da er spredningen borte igjen – V2 er ikke
   lukket, og linje 397 i IMPROVEMENTS blir usann. Rettelse: godta bare små hopp
   (`ShotIndex - LastServerShotIndex <= 3`, nok til pakketap). Ved større hopp: godta skuddet, men trekk
   spredningen fra serverens egen tilfeldighet (`FMath::Rand()`), ikke fra indeksen. Da kan en juksemaker høyst
   velge beste av 3. Skriv dette som eget «Ferdig når»-punkt.

2. **HØY (skudd forsvinner): upålitelig `ServerFire` for ALLE våpen er feil.** Et tapt skudd fra MG3 merkes
   ikke. Et tapt skudd fra L96, Kar98k, Musket (ett skudd, 6 s lading), hagle, Carl Gustaf eller M72
   (engangsrør!) merkes veldig: klienten har hørt smellet, sett rekylen og brukt patronen, og ingenting skjer.
   For M72 blir røret liggende på serveren mens klienten tror det er skutt. Rettelse: to RPC-er som kaller samme
   funksjon – `ServerFire` (Unreliable) for automatvåpen med `FireInterval < 0.15`, og `ServerFireReliable`
   for alt annet (halvautomat, boltrifle, hagle, prosjektilvåpen). `ShotIndex` tåler allerede at de kommer i
   blandet rekkefølge. V7 (frakobling) gjelder bare de raske våpnene uansett.

3. **HØY (balanse): skadefallet starter ved munningen og bryter drap-tersklene i katalogen.**
   `DamageScale` er under 1 fra første meter (9 mm på 10 m = 0,976). Våpen som i dag dreper på et helt antall
   treff mister det på all avstand: Garand 50 × 2 = 100 → 3 skudd, MP40/Makarov 20 × 5 = 100 → 6 skudd.
   Det er en skjult svekkelse av mange våpen, ikke realisme. Rettelse: full skade ut til innskytingsavstanden,
   fall etter den: `R = Speed / (Muzzle * exp(-Drag * ZeroM))`, klemt til 1 (krever `ZeroM` eller farten ved
   null i `FGSBullet`). Da gjelder katalogtallene på 25 m (pistol) og 100 m (rifle), som i dag.
   Eksempelet i planen «9 mm på 100 m ≈ 0,76» er også feil regnet: formelen i planen gir 0,79.

4. **HØY (ytelse): lysspor lager én ny aktør + ett nytt materiale per kule per steg.**
   `AGSShortFX::SpawnTracer` gjør `SpawnActor` og `UMaterialInstanceDynamic::Create` hver gang, med 0,05 s
   levetid. Planen kaller den i hvert `Step` (60 ganger i sekundet per kule, flere ganger per bilde ved lav
   bildefrekvens). Sporlys-ammo i en Vector (20 skudd/s) gir ca. 250 aktører i sekundet per skytter; fem slike
   gir over 1000 – det hakker på RTX 3050-laptopen, og bryter lærdommen «tak per kilde og felles tak».
   Rettelse: én lysende strek per kule som eies av `UGSBallistics` og flyttes hver `Tick` (ikke i `Step`),
   fjernes når kula er ferdig; felles tak (f.eks. 40 synlige spor per maskin). Minstekrav hvis det blir for
   mye: kall `SpawnTracer` én gang per kule per `Tick` og med felles tak per bilde. `GSEffects.*` trenger ikke
   røres hvis streken lages i `GSBallistics.cpp`.

5. **MIDDELS (ytelse, server): kuler skutt opp i lufta lever i 3 s og går 2 km.** Verste tilfelle 20 × MG3:
   400 kuler/s × 3 s = 1200 ekte kuler × 60 spor/s = 72 000 komplekse linjespor i sekundet hos verten, som
   også tegner spillet (og har de viste kulene i tillegg – dobbelt). Kartet er 330 × 265 m. Rettelse:
   fjern kula når den er utenfor spillområdet + 50 m, over ca. 150 m høyde, eller har gått 600 m; `MaxLife` 1,5 s.
   Viste kuler: ett spor per `Tick` (ikke delsteg) – de gjør ingen skade. Skriv tallene inn i «Risiko»
   (planens «noen hundre spor per sekund» gjelder bare når alle treffer en vegg).

6. **MIDDELS (det klienten ser stemmer ikke med serveren): spredningens STØRRELSE regnes to steder.**
   Klienten bruker sin `GetCurrentSpread()` (sin rekyl, fart, huk i sitt øyeblikk), serveren sin egen ved
   ankomst (rekylen har falt annerledes pga. ping og ujevn ankomst; `bAiming` kan komme etter skuddet). Samme
   frø, ulik kjegle → støvet klienten ser havner 10–30 cm fra der serverens kule gikk på 50 m. «Ferdig når»-
   punktet «nedslagene du ser stemmer med der serveren sier du traff» vil ikke holde ved automatild.
   Rettelse: send klientens spredning med i RPC-en (uint8, 0,05°-trinn), og la serveren bruke den klemt til
   0,85–1,15 × sin egen. Ærlige klienter treffer da nøyaktig der de ser; juks gir høyst 15 %.

7. **MIDDELS: kikkertens holdemerker gir lite på dette kartet, og testen er umulig.** Kartet er høyst 330 m;
   «sikt med 4-streken på et mål 400 m unna» kan ikke gjøres. Med 4x-kikkert (FOV 25°, 1920 px) ligger
   200 m-merket 3,7 px under midten (hoppes over), 300 m 8 px, 400 m 13 px – merkene blir en liten klump. Flytt
   kikkerten til del 1b, eller tegn merker for 100/150/200/250/300 m og bare på 8x. Endre testen til en avstand som finnes.

8. **LAV: startpunktet.** 250 cm slingringsmonn lar en juksende klient flytte munningen 2,5 m opp over en
   mur eller rundt et hjørne (veggsjekken stopper bare tykke vegger mellom øye og start). Stram til 120 cm
   (sprint 6 m/s × 0,2 s). `DrawPellets` bør få `FGSBallistic` fra våpenet (i dag er alle hagler 12g, men
   serveren bruker `ForWeapon(S)` og klienten `ForCaliber("12g")` – to kilder for samme tall).

## B. Flytid + ping (blir det ustyrlig?)
Nei, men det må sies ærlig. På 20–80 m: rifle 0,03–0,1 s, 9 mm 0,24 s på 80 m, .45 0,2 s på 50 m. Oppå det
kommer ping/2 før serveren i det hele tatt lager kula, og klienten ser målet ca. 0,1 s + ping/2 forsinket.
Med plan-04s hode på 19 × 16 cm blir hodeskudd på mål i bevegelse nesten flaks for klienter, mens verten har
null forsinkelse – vertens fordel (V8) vokser. Billig del-rettelse som bør inn i denne runden: når serveren
lager en ekte kule fra en fjern klient, flytt den straks fram `min(ping/2, 0,1 s)` i vanlige delsteg
(`PlayerState->ExactPing`). Da flyr serverens kule i takt med den klienten ser. Full V8 (tilbakespoling av
treffbokser) bør være neste Kamp-runde etter denne, ikke «backlog».

## C. Samspill med plan-04 og runde 3 (sjekket, i orden – men pass på)
- Treffboksene (`HitZones`, `ECC_Visibility`, på kapselen) finnes hos klienter også, så viste kuler stopper i
  kropper. `Hit.GetComponent()` fra `Step` gir boksen, så `ZoneOf` virker. Ikke bytt kanal eller bruk sveip.
- Blodet: `MulticastWoundFX` og taket på 3 sår per bilde virker som før (alle hagl kommer i samme `Tick`).
  `ApplyDeathImpulse` (0,8 s-vinduet) påvirkes ikke.
- `TakeDamage`: plan-04 endrer hodeskudd-blokka (`Zone == GSZone::Head`). Steg 23 må legges inn i plan-04s
  versjon, ikke i den som står i HEAD. Lagsjekken (CTF) blir fortsatt ett sted.
- Skytter som koblet fra mens kula fløy: `Shooter` og `Instigator` er null → `DamageCauser` null → ikke blod,
  ikke drap til noen. Lagre `FVector Origin` i kula og bruk den til `ClientTookHit`-retning; godta resten.

## D. For stor for ett steg – del den
Runden rører ny undermodul, hele `ServerFire`, nettverksformatet, `TakeDamage`, HUD, lyd og lydgenerator, oppå
plan-04 som ikke er committet (planen er skrevet mot kode som ikke finnes ennå). 16 loggelinjer. Forslag:
- **5a (kjernen):** `GSBallistics`, ekte + viste kuler, fall, luftmotstand, skadefall (punkt 3), innskyting,
  hagl som kuler, veggsjekk ved start, `IncomingWeaponId`, `gs.ShowBullets`, serverens spredning med
  `ShotIndex`/`SpreadKey` (punkt 1 og 6), levetidsgrenser (punkt 5), framspoling med ping (B). `ServerFire`
  forblir Reliable. Lysspor som før eller etter punkt 4. Forbedrer gammelt (ServerFire, hagler) og legger til nytt.
- **5b:** upålitelig ild for automatvåpen (punkt 2), flygende lysspor (punkt 4), smell/sus + lydene,
  holdemerker (punkt 7), sporlys-teksten.

## E. Byggeren må passe på (uansett deling)
- Lysspor og smell bare for `!bReal`-kuler. Verten har både ekte og vist kule for samme skudd; ellers dobbelt.
- `Step` holder `FGSBullet&` inn i `Bullets` mens `TakeDamage` kjøres. Legges en kule til underveis, flyttes
  tabellen og referansen blir ugyldig. Kopier det som trengs før `TakeDamage`, eller samle nye kuler i en
  egen liste som flettes inn etter løkka.
- `SpreadKey = FMath::Rand()` gir bare 15 biter; bruk `FMath::RandHelper(MAX_int32)` eller to kall.
- `LastServerShotIndex` skal bare oppdateres når skuddet er godtatt (etter retningskrav og skuddkonto) – slik
  planen har det. Ikke flytt den opp.
- `GetBaseAimRotation()` for fjerne klienter kommer fra kameraoppdateringer som også er upålitelige og kan
  ligge noen bilder etter. Logg avviste skudd (`UE_LOG` med vinkel) i testbygg, så 35° kan vurderes med tall.
- Serverens `CurrentRecoil` for fjerne klienter økes bare i `ServerFire` og faller i `Tick`. Sjekk at verten
  ikke får den lagt til to ganger (`FireOnce` + `ServerFire`) – planen har `!IsLocallyControlled()`, behold den.
- Regn ut og skriv i rapporten: skade per treff på 25/50/100 m for Garand, MP40, Pump og L96, før og etter.
- `FCollisionQueryParams` lages én gang per `Tick`, ikke per steg (bytt ignorert aktør per kule).
- Ikke rør `GSEffects.*`/`GSAudio.*`/`GSTypes.h`. Stage filene med navn.

## Lærdom (forslag)
- Tall klienten sender som brukes til tilfeldighet (indeks, frø) er klientens valg: begrens hvor fritt de kan velges.
- Upålitelig sending passer for ting som gjentas ofte; ett enkelt, dyrt skudd må være pålitelig.
- Nye skadeformler regnes mot drap-tersklene i katalogen (hele treff til 100 liv) før de godkjennes.
