**GODKJENT** – runde 5a (lag Kamp): ballistikk, commit `a28c23e`

Lest fra commiten (`git show a28c23e`, hele `GSBallistics.h/.cpp`, `GSCharacter.cpp` og `GSHUD.cpp` slik de står
i commiten), mot `ko/plan-05.md`, `kontroll/forhand-plan-05.md`, `catalog.py` og motorkoden i UE 5.8. Ikke
bygget selv (byggeren: `Result: Succeeded`).

## 1. Forhåndssjekken, punkt for punkt
| Punkt i `forhand-plan-05.md` | Fulgt? |
|---|---|
| A1 juks med `ShotIndex` | Ja. `Jump = ShotIndex − LastServerShotIndex` regnes i `int64`; `Jump <= 0` avvises; frøet fra nøkkel + nummer brukes bare ved `Jump <= 3`, ellers `FMath::RandHelper(MAX_int32)`. `LastServerShotIndex` settes først etter retningskravet og skuddkontoen. Et stort tall (f.eks. `INT_MAX`) stenger bare juksemakeren selv ute. |
| A2 upålitelig `ServerFire` | Riktig utsatt til 5b. `ServerFire` er fortsatt `Reliable`. |
| A3 skadefall fra munningen | Rettet. `DamageScale` regnes fra avstanden, nøyaktig 1,0 ut til `ZeroM` (se tabellen under). |
| A4 lysspor per steg | Utsatt til 5b. `Step` lager ingen lysspor. Streken (`GSDrawShotStreak`) er én per skudd/hagl og høyst 50 m, som før. |
| A5 levetid | Ja: 1,5 s, 600 m, over 150 m, under −20 m, 50 m utenfor `PlayArea()`, under 80 m/s. Viste kuler: ett spor per `Tick`. |
| A6 spredningens størrelse | Ja. `SpreadQ` sendes med, eieren bruker selv den pakkede verdien, serveren klemmer til 0,85–1,15 × sin egen (ikke for verten). Serverens rekyl for fjerne klienter legges til **etter** at `Own` er regnet, som i `FireOnce`. |
| A7 holdemerker | Utsatt til 5b (`HoldUnder` finnes, ubrukt). |
| A8 startpunkt 120 cm, ballistikk fra våpenet i `DrawPellets` | Ja, begge. |
| B framspoling | Ja: `min(ping/2, 0,1 s)` i delsteg, 0 for verten. |
| C samspill med runde 4 | Ja: `ECC_Visibility`, linjespor, `Hit.GetComponent()` er treffboksen, så `ZoneOf` virker. Hodeskudd-innskuddet ligger i runde 4s `Zone == GSZone::Head`-blokk. `Origin` lagres i kula. |
| E1 ekte/vist kule | Ja. Bare den viste lager støv, bare den ekte skader. |
| E2 referanse inn i `Bullets` | Ja. Kula kopieres før `Step`, nye kuler går i `Pending` og flettes inn etter løkka. `FireReal` bruker en lokal kule under framspolingen. `W.Id`/`W.AmmoType` kopieres før `FireReal`. |
| E3 `RandHelper` for `SpreadKey` | Ja. |
| E5 logg av avviste skudd | Ja, med vinkel (og `Acos` klemt). |
| E6 ikke dobbel rekyl hos verten | Ja (`!IsLocallyControlled()`). |
| E8 `FCollisionQueryParams` én gang per `Tick` | Ja (`ClearIgnoredSourceObjects` + `AddIgnoredActor` per kule). |
| E9 ikke rør `GSEffects`/`GSAudio`/`GSTypes` | Ja, ikke endret. |

Planens steg 26 sjekket: `LineTraceSingleByChannel` i skytefunksjonene står bare i `GSDrawShotStreak` og i de to
vegg-sjekkene i `ServerFire`; `S.Range` står bare i nærkampen (`GSCharacter.cpp:1020`). Alle kall til
`ServerFire`, `MulticastShotFX` og `DrawPellets` har ny form. Alle kaliber i bruk for vanlige skudd finnes i
tabellen (`flare` og `""` er bare prosjektilvåpen).

## 2. Byggerens merknader
- **Vegg-sjekkene bruker kanalspor på `ECC_WorldStatic`, som også treffer spillerkapsler.** Følgene er ufarlige:
  står noen mellom øyet og det oppgitte startpunktet, brukes serverens eget øye (skuddet går likevel), og en
  rakett mot en fiende nærmere enn 80 cm starter 15 cm foran kapselen hans i stedet for inni ham. Ingen skudd går
  tapt. Lærdommen sier likevel objektspor (LAV, F2 under).
- **Verten har to kuler per skudd:** riktig skilt (`bReal`). Ingen dobbel skade, ingen dobbelt støv.
- **`t.MaxFPS` med konsoll-prioritet i editoren:** riktig grunn (`EndPlay` setter 0 med konsoll-prioritet, en svakere
  `Set` i neste Play ville ellers blitt stille avvist). Liten bivirkning, se F4.
- **`AddNow` og tak på 600:** riktig. Når taket er nådd fjernes den eldste **viste** kula; ekte kuler fjernes aldri.
  Søket er O(n) per ny kule bare når tabellen er full – ubetydelig.
- **`GetPingInMilliseconds` er tur-retur:** sjekket i motoren. `NetConnection.cpp:3135` sender `PlayerFacingPing`
  (RTT eller RTT uten bildetid) til `APlayerController::UpdatePing` → `APlayerState::UpdatePing` → `ExactPing`
  (gjennomsnitt i ms), og `GetPingInMilliseconds` gir `ExactPing` (eller `CompressedPing * 4`). `× 0,0005` er
  altså halve tur-returen i sekunder. Riktig.

## 3. Regnet etter
**Kulefall** (bare tyngdekraft, analytisk som i koden):
- 9 mm, 80 m: flytid 0,236 s, fall 27,3 cm, innskytingen løfter 7,9 cm → **19,5 cm lavt** (planen: ca. 19).
- 7,62 NATO, 300 m: flytid 0,400 s, fall 78,4 cm, løft 22,5 cm → **56 cm lavt** (planen: ca. 56).
- Hagl på 40 m: skadefaktor 0,86 (planen: 0,86). 7,62 NATO på 300 m: 0,795 (planen: 0,80).

**Skade per treff i brystet (uten vest, 100 liv), før → etter, og antall treff for å drepe.**
«Før» bruker katalogens gamle `range`: MP40 60 m, Pump 40 m, Garand 350 m, L96 800 m – utenfor den var skaden 0.
| Våpen (kaliber) | 25 m | 50 m | 100 m |
|---|---|---|---|
| Garand (7,62 NATO, 50) | 50 → 50 (2 → 2) | 50 → 50 (2 → 2) | 50 → 50 (2 → 2) |
| MP40 (9 mm, 20) | 20 → 20 (5 → 5) | 20 → 18,8 (5 → 6) | **0** → 16,8 (aldri → 6) |
| Pump, per hagl (12g, 13 × 9) | 13 → 13 (8 hagl → 8) | **0** → 10,2 (aldri → 10 hagl, mer enn ett skudd) | **0** → 6,6 (aldri → 16 hagl) |
| L96 (7,62 NATO, 95) | 95 → 95 (2 → 2) | 95 → 95 (2 → 2) | 95 → 95 (2 → 2) |

Innenfor innskytingsavstanden trenger ingen våpen flere treff enn før. Planens forventning stemmer, med én
presisering: MP40 på 100 m og Pump på 50/100 m gjorde **0** før (utenfor rekkevidden), så der er det en økning,
ikke et fall. Eneste svekkelse: MP40 på 50 m går fra 5 til 6 treff (18,84 × 5 = 94). Det er ønsket realisme
(utenfor 25 m-innskytingen), og loggens linje 388 sier «på vanlig hold».

## 4. Det som er sjekket og er i orden
- **Juks, spredning:** `SpreadQ = 0` fra en juksende klient klemmes opp til 0,85 × serverens egen. Restjuksen er
  som planen sa: beste av tre mønstre og 15 % mindre kjegle.
- **Juks, retning:** 35° (`0.82f`) mot `GetBaseAimRotation()` (serverens kontrollrotasjon), ikke for verten.
- **Juks, start:** 120 cm og ingen vegg imellom, ellers øyet. Rundt et hjørne kan en klient fortsatt flytte starten
  inntil 1,2 m langs veggen (linja øye→start krysser ikke veggen) – kjent og godtatt i planen.
- **Skudd forsvinner aldri stille** av ballistikken selv: ekte kuler kastes aldri for taket, `Pending` flettes alltid
  inn, framspolingen stopper bare når kula traff noe eller forlot banen. De eneste stedene et skudd faller bort er
  de planlagte (retningskravet med logg, skuddkontoen og ammo/lading som før).
- **Ytelse, vert med 20 spillere:** ekte kule = ett linjespor per 1/60 s, vist kule = ett per bilde. 20 × MG3
  (400 skudd/s) gir i verste fall (alle skyter bortover åpent terreng, levetid høyst ca. 0,5 s før de er ute av
  området) ca. 200 ekte + 200 viste kuler i lufta, ca. 24 000 korte spor i sekundet; i vanlig spill (kuler treffer
  vegg etter 0,05–0,2 s) noen tusen. Hagl: 9 ekte + 9 viste, framspolingen høyst 6 spor per hagl med én gang.
  Støvet har sitt eget felles tak i `SpawnImpact` (`GSEffects.cpp:426–431`). Ingen ny aktør eller nytt materiale per
  kule. Godt nok for 5a; flygende lysspor i 5b må få eget tak.
- **Kroppssoner, blod, venneild:** `TakeDamage` får `FPointDamageEvent` med treffboksen, `Instigator` og
  `Shooter`. Lagsjekken øverst er urørt og stopper venneild (kula stopper i lagkameraten, ingen blod/merke).
  Blod: `(DamageCauser || bBullet)`, så en kule fra en frakoblet skytter blør fortsatt; taket på 3 sår per bilde
  gjelder alle hagl. `WoundDir`-linja er vernet (`&& DamageCauser`). Drapsloggen og hodeskudd følger
  `IncomingWeaponId`; retningskilen bruker `IncomingOrigin`; blodtørst krever `!Shooter->bDead`.
- **Replikering:** `SpreadKey` bare til eieren (`COND_OwnerOnly`), ny nøkkel per figur. `ServerFire` pålitelig.
  `MulticastShotFX` sender retningen (med spredning og løft) så andre maskiner ikke løfter den på nytt; hagl får
  frøet, så alle tegner samme hagl, også når serveren trakk sitt eget frø.
- **Andreas' faste regel:** oppfylt. Forbedret: `ServerFire` (spredning, retning, start), haglene, drapslogg/hodeskudd,
  retningskilen (391, 393–396, 398–399). Nytt: kuler med flytid og fall, skadefall og innskyting, framspoling,
  `gs.ShowBullets` (386–390, 392, 397). `IMPROVEMENTS.md` er ført (386–401, og 446–449 for rettingene fra runde 11;
  ingen dobbelte numre i filen). Bestillingen til Spillbarhet (steg 27) er skrevet.
- **Utenfor planen:** små endringer i `GSHUD.cpp` og `GSPlayerController.*`. Det er rettingene jeg selv ba om i
  forhåndssjekken av runde 11 (F1, F3, F4, F6), ført som 446–449. Godtatt.

## 5. Funn (ingen hindrer godkjenning)
**F1. LAV–MIDDELS: 0,1°-trinnene i `SpreadQ` er grovere enn de mest presise våpnene – `GSBallistics.cpp:122–130`.**
L96 siktet har 0,05° (Barrett 0,08°). `PackSpread(0.05)` = `RoundToInt(0.5)` = 1 → 0,1°. Verten (ikke klemt)
får dobbelt så stor kjegle som katalogen (8,7 cm i stedet for 4,4 cm på 100 m); L96 siktet i huk (0,035°) blir 0 →
helt uten spredning. For klienter klemmer serveren 0,1 til 0,0575, så kula de ser og serverens går i litt ulik
kjegle (noen cm på 100 m). Rettelse (5b): finere trinn der det trengs, f.eks. `uint16` i 0,01°-trinn, eller
ikke-lineært (0,01° under 1°, grovere over).

**F2. LAV: vegg-sjekkene i `ServerFire` bør være objektspor – `GSCharacter.cpp:931` og `:954`.**
Se punkt 2. Ufarlig nå, men `LineTraceSingleByObjectType` med `ECC_WorldStatic` + `ECC_WorldDynamic` (dører) gjør
det slik lærdommen sier, og hindrer at en lagkamerat ved siden av deg flytter startpunktet ditt til øyet.

**F3. LAV: venneild fra en frakoblet lagkamerat mens kula flyr.** `IsSparedFrom(nullptr)` gir `false`, så kuler som
er i lufta (høyst 1,5 s) når skytteren kobler fra, skader lagkamerater. Sjelden. Rettelse: lagre skytterens lag i
`FGSBullet` ved avfyring og sjekk det i `Step` før `TakeDamage`.

**F4. LAV (bare i editoren): `EndPlay` på én spillerkontroller tar av taket for hele editoren** – `GSPlayerController.cpp:333–343`.
Forlater klientvinduet kampen i PIE mens verten spiller videre, settes `t.MaxFPS` til 0 for begge. Og en `t.MaxFPS`
Andreas har skrevet selv i editoren blir overstyrt ved hver Play. Ufarlig i et pakket spill. Kan rettes med en teller
(sett 0 bare når siste lokale kontroller går).

## 6. Hva Andreas skal teste (Number of Players: 2, Play As Listen Server, etter kampstart)
- Skriv `gs.ShowBullets 1` i konsollen og skyt et gevær bortover plassen: en gul linje som bøyer nedover mot slutten.
  Med pistol bøyer den mye før.
- Pistol mot et merke ca. 80 m unna: kula treffer omtrent en håndbredd (ca. 20 cm) under der du siktet.
- Skyt mot en vegg langt unna: støvet kommer et lite øyeblikk etter smellet.
- Hagle på 5 m dreper som før. På 40–50 m gjør den nå litt skade (før: ingenting).
- Skyt den andre og bytt våpen rett etter: drapsloggen viser våpenet du skjøt med.

## Forslag til neste runde
1. 5b som planlagt (`ko/plan-06.md`): upålitelig ild for automatvåpen, flygende lysspor med felles tak, smell/sus.
   Ta med F1 (finere `SpreadQ`) der, siden RPC-en endres uansett.
2. V8 (tilbakespoling av treffbokser) rett etter 5b: nå som kula har flytid, er det klientenes største ulempe mot verten.
3. Lagre lag i kula (F3) og replikér lagnummeret på figuren (runde 11, F7) i samme lille runde.

## Lærdom
- Tall som pakkes for nettet må ha finere trinn enn den minste verdien de skal bære; regn ut den minste verdien i katalogen.
- Det en kule eller et prosjektil trenger ved treff (lag, våpen, startpunkt) lagres ved avfyring; skytteren kan være borte når den treffer.
