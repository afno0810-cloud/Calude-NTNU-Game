**GODKJENT** (med merknader som bør inn i neste plan – se særlig funn 1, som er en gammel og alvorlig feil)

Feilrettingsrunde 1 (commit `57493cb`, «Fixes from bug hunt»). Kontrollert mot commiten, ikke arbeidsmappen.
Alt er vurdert ved å lese koden (og motorens kildekode i `UE_5.8/Engine/Source`); ingenting er sett i spillet.
Lootfordelingen er regnet ut nøyaktig fra `GSItemCatalog.gen.inl` i commiten (180 gjenstander).

## Bygg og omfang
- Ikke bygget av kontrolløren (samtidig drift). Lest for kompilerbarhet: ingen feil funnet (`PostNetReceiveVelocity`
  er riktig overstyrt, `HasActorBegunPlay`, tabellstørrelsene 9 grupper × 6 sjeldenheter stemmer med enumene).
- Commiten rører bare `GSProjectile.h/.cpp`, `GSItems.cpp` og `IMPROVEMENTS.md` (313–317, riktig nummerert).
- `GSItems.cpp` eies nå av lag Spillbarhet. Commiten ble laget før lagdelingen (`55ca90f`), så det er ikke et
  regelbrudd, men oppfølgingen av lootfunnene under må gå som bestilling til Spillbarhet.
- Fast regel «én gammel + én ny» gjelder ikke (ren feilretting).

## De fire rettelsene
| Funn | Vurdering |
|---|---|
| 2. Drap krediteres etter at kasteren er død | Virker. `OwnerController` lagres i `Launch` og brukes i begge `TakeDamage`-kallene. Faller tilbake til `GetInstigatorController()` hvis spilleren har forlatt spillet. Selvdrap håndteres riktig i `OnPlayerKilled` og `Die` (ingen poeng, tomt drapsnavn). Se funn 4 for det som fortsatt er skjevt. |
| 3. Loot per kategori | Virker for vanlige lootplasser: ca. 53 medisin, 29 vester, 20 hjelmer, 29 kastevåpen, 82 våpen, 82 mat, 49 ammo av 410. Aldri under `MinRarity`. Se funn 3 for bivirkninger. |
| 4. Prosjektilbevegelse hos klienter | Virker i hovedsak. Se (a) under og funn 1 og 2. |
| 5. Minen blir liggende | Virker. `SetLifeSpan(0)` i `OnStop` fjerner 60 s-timeren; `Explode` setter 0,5 s etterpå. |

## (a) Klientsimuleringen
- **Dobbel bevegelse:** nei. Klienten simulerer selv mellom oppdateringene, og motoren setter posisjon etter farten
  (`PostNetReceiveVelocity` kalles før `PostNetReceiveLocationAndRotation`). Dette er standardmønsteret.
- **Sprett som avviker:** `Bounciness`/`Friction` er flyttet til konstruktøren og er like hos alle. Spretter klienten
  litt for tidlig eller sent, rettes det av neste oppdatering. Stopper klienten for tidlig (traff noe først hos seg),
  vekkes simuleringen igjen med `SetUpdatedComponent(Collision)`. OK.
- **Fortsetter etter eksplosjon:** nei. `OnRep_Exploded` stopper bevegelsen og slår av tyngdekraft, og
  `PostNetReceiveVelocity` gjør ingenting når `bExploded` er satt. `ReplicatedMovement` ligger før `bExploded` i
  rekkefølgen, så eksplosjonen tegnes på serverens posisjon. Røyk og molotovild blir stående stille. OK.
- Hvile på serveren: `StopSimulating` nullstiller komponentfarten, klienten får fart 0 og slår av tyngdekraft. OK.
- Ett hull: se funn 2.

## Funn

### 1. GAMMEL FEIL, HØY: prosjektiler flyr i feil retning (ikke innført av denne commiten)
- Fil: `GSProjectile.cpp` konstruktøren (linje 36–42) og `Launch` (linje 68).
- `UProjectileMovementComponent::bInitialVelocityInLocalSpace` er `true` som standard. `Launch` setter
  `Movement->Velocity` i **verdenskoordinater** før `FinishSpawning`. `InitializeComponent` kjøres i
  `FinishSpawning` og tolker da farten som **lokal** og dreier den med aktørens rotasjon (som er
  `Velocity.Rotation()`). Resultat: retningen dreies én gang til. Kast rett mot +X går riktig (men med dobbel
  helning opp), kast mot +Y (90°) går mot −X (180°), og så videre. Gjelder granater, miner, C4, raketter, 40 mm og piler.
- Bekreftet i motorkoden (`ProjectileMovementComponent.cpp` linje 22, 87–98, 450–455; `Actor.cpp` 4374–4449).
  Bør likevel prøves i spillet: kast en granat mens du ser nord, øst, sør og vest.
- Rettelsen av funn 4 gjør at klientene nå følger serverens (feil) bane jevnt, så «riktig bue hos alle» i
  IMPROVEMENTS 316 stemmer først når dette er rettet.
- Må gjøres: `Movement->bInitialVelocityInLocalSpace = false;` i konstruktøren. Én linje.

### 2. MIDDELS: liggende mine/C4 siger ned hos klienter som får den inn sent
- Fil: `GSProjectile.cpp` `BeginPlay` linje 86–89 og `OnRep_Flight` linje 99–105.
- Når en klient får et prosjektil som allerede ligger stille (går inn i nettverksrekkevidde, eller kobler til),
  kjøres `OnRep`-ene først (fart 0 → tyngdekraft 0), og deretter `BeginPlay`, som kaller `OnRep_Flight` på nytt.
  Der er `UpdatedComponent` fortsatt satt hos klienten, så `bResting` blir usann og tyngdekraften slås **på** igjen.
  Serveren sender ingen flere oppdateringer for noe som ligger stille, så en mine som sitter på en vegg eller i en
  skråning faller til gulvet hos den klienten og vises på feil sted for godt. Miner på flatt gulv merker ingenting.
- Dette blir vanligere nå som miner varer evig (rekkevidden for replikering er 150 m).
- Må gjøres: la `bResting` hos klienter også være sann når `GetReplicatedMovement().LinearVelocity` er nær null.

### 3. MIDDELS: bivirkninger av lootfordelingen (Spillbarhets fil – bestilling)
Tall = forventet antall på 410 vanlige lootplasser.
- **Sjelden ammo er tre ganger vanligere enn vanlig ammo.** 13 vanlige kalibre deler vekten 40, tre «Uncommon»
  (rakett, 40 mm, .50 BMG) deler 28. Det gir 6,8 esker rakettammo, men bare 2,2 esker 9 mm (som passer ti våpen).
  Før rettelsen var 9 mm 3,1 og rakett 2,2. Ammo totalt gikk bare fra 46,8 til 49,2, så IMPROVEMENTS 315 er
  riktig om andelen (12 %), men vanlig ammo ble faktisk litt sjeldnere. Strider mot realismemålet.
- **Sjeldenhet snus i små grupper** fordi vekten til en sjeldenhet deles på antall gjenstander i gruppen:
  Barrett og Carl Gustaf (Mythic) 0,62 hver mot 0,41 per Legendary-våpen; bandasje (Uncommon) 16,7 mot plaster
  (Common) 11,9; snubletrådmine (Epic) 4,4 mot håndgranat (Rare) 4,2.
- **Låste kister er blitt mye dårligere på våpen.** Med `MinRarity = Epic`: våpen 29 % (før ca. 72 %), kirurgisett
  19 % alene, platebærer 9 %, hjelm 7 %, mine 7 %. Med `Legendary` (høye bygg): kirurgisett 19 %, C4 10 %, og
  Exo-drakten (Mythic) 10 %, mens hvert Legendary-våpen har 2,5 %. Gruppevektene er stilt inn for vanlige plasser
  og passer ikke når en gruppe bare har én gjenstand igjen. `MinRarity` virker teknisk (aldri under grensen).
- Forslag: (1) i ammogruppen: trekk kaliber med egne vekter (vanlig kaliber høyest), ikke via sjeldenhet;
  (2) vekt sjeldenhet med `RarityWeight × antall gjenstander` innad i gruppen, så en Mythic aldri er vanligere per
  stykk enn en Legendary; (3) egne gruppevekter når `MinRarity >= Epic` (våpen rundt 55–60 %).

### 4. LAV/MIDDELS: (d) dødskort og treffmerke ved granat-/minedrap etter at kasteren har fått ny kropp
- `Die` (`GSCharacter.cpp` 2660–2682): navnet blir nå riktig. Men `Killer->GetPawn()` er den **nye** kroppen, så
  dødskortet viser avstand, liv og rustning til den nye kroppen, og dødskameraet snur seg mot den – kanskje 300 m
  unna minen som drepte deg. Er kasteren død og ikke respawnet ennå, er `KillerPawn` null, og kortet viser bare navn
  og våpen (det er riktig oppførsel).
- `TakeDamage` linje 2512: `Shooter = Proj->GetOwner()` er fortsatt den **gamle** kroppen. Den har ingen kontroller,
  så kasteren får ikke treffmerke/drapsmerke for minen sin etter respawn, retningspilen hos offeret peker mot liket,
  og Blodtørst heler et lik. Etter 15 s er liket borte og `Shooter` null.
- Forslag: i `TakeDamage`, bruk `Proj->OwnerController->GetPawn()` som `Shooter` for treffmerket, og bruk
  prosjektilets posisjon som «hvor kom det fra». I `Die`: hvis skaden kom fra et prosjektil, send prosjektilets
  posisjon som `KillerLocation` og dropp liv/rustning/avstand når kasterens kropp ikke er den som kastet.

### 5. LAV: (c) byggerens egne merknader
- **`*It == GetOwner()`** (linje 198): eieren er kroppen som kastet. Etter respawn (eller etter `StartMatch`, som
  sletter alle kropper) er eieren ikke lenger unntatt, og utløser sin egen mine. Skaden regnes som selvdrap, så
  ingenting krasjer og ingen får poeng. Mens eieren lever i samme kropp utløser han den ikke, men tar full skade om
  en annen gjør det. Inkonsekvent: velg én regel. Enklest og mest realistisk: alle utløser den (også eieren) etter
  armeringstiden på 2 s; ellers sammenlign `It->GetController()` med `OwnerController`.
- **Ingen grense på antall miner:** ikke et ytelsesproblem nå (hver mine går gjennom maks 20 spillere per frame), men
  lootplasser fylles på igjen, så antallet kan vokse hele kampen. Sett et tak per spiller (for eksempel 5, eldste fjernes).
- **Miner overlever kampslutt:** `StartMatch`/`BackToLobby` rydder ikke prosjektiler. I lobby og nedtelling smeller
  de uten skade når noen går forbi (bortkastet), eller de ligger til neste kamp og dreper noen der. Bestilling til
  Spillbarhet: slett alle `AGSProjectile` i `StartMatch`.
- **C4 har 60 s levetid:** uten betydning. C4 har `Fuse = 4` og smeller alltid etter 4 s; det finnes ingen fjernutløser.
  Katalogteksten sier også «Smeller etter 4 sek». En ekte C4 med fjernutløser er en mulig ny ting senere.

## Forslag til neste runde (viktigst først)
1. Rett funn 1 (`bInitialVelocityInLocalSpace = false`) og funn 2 sammen, og prøv i spillet med to spillere.
2. Rett funn 4 (treffmerke, retningspil og dødskort for prosjektildrap) og velg regel for eierens egen mine.
3. Bestilling til Spillbarhet: ammovekter per kaliber, kistevekter, og rydding av prosjektiler i `StartMatch`.

## Lærdom
- Verdier som settes på en komponent før `FinishSpawning` kan bli tolket om av komponentens `InitializeComponent`
  (for eksempel fart i lokale koordinater). Les motorens standardverdier, ikke bare egne linjer.
- Hos klienter kjøres `OnRep` for startverdiene før `BeginPlay`. Kode i `BeginPlay` må ikke overskrive det `OnRep` satte.
- Når en tilfeldig trekning deles i trinn (gruppe → sjeldenhet → gjenstand), regn ut fordelingen per gjenstand og for
  alle `MinRarity`-nivåer før den godkjennes; små grupper gir lett omvendt sjeldenhet.
