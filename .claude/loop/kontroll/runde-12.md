**GODKJENT** – runde 12 (lag Kamp): ballistikk del 2, commit `4d74e7c`

Lest fra commiten (`git show 4d74e7c`, hele `GSBallistics.cpp`, `GSCharacter.cpp`, `GSEffects.cpp/.h` og `GSHUD.cpp`
slik de står i commiten), mot `ko/plan-06.md`, F1–F3 i `kontroll/runde-05a.md`, `catalog.py` og motorkoden i UE 5.8
(`NetDriver.cpp`). IMPROVEMENTS 484–499 og bestillingen er lest i arbeidsmappen. Ikke bygget selv (byggeren:
`Result: Succeeded`). Ett nytt funn (F1, MIDDELS) bør rettes først i neste Kamp-runde, men hindrer ikke godkjenning.

## 1. «Ferdig når», punkt for punkt
| Krav | Oppfylt? |
|---|---|
| Bygger | Ja (byggerens bygg). |
| `ServerFireFast` (Unreliable) bare for `Hitscan` + `bAutomatic` + `FireInterval < 0.15`, ellers `ServerFire` (Reliable), begge i `HandleFire` | Ja. Begge `_Implementation` er én linje til `HandleFire`. Rask vei (fra `catalog.py`, 20 våpen): MP40, Uzi, Suomi, MP5, Thompson, PPSh, UMP, P90, Vector, AG3, AK47, HK416, FAL, M14, G36, SCAR, MG3, Bren, PKM, Minimi. Hagler (også Saiga, som er automatisk, men `Pellets`), boltrifler, muskett, pistoler, SVD/Barrett og alle rakett- og granatkastere går den pålitelige veien. |
| Lysspor: ingen `SpawnActor`/`UMaterialInstanceDynamic::Create` per bilde, én strek per kule, flyttet i `Tick`, høyst 40, alle veier ut gjennom `EndBullet`, bare `!bReal` | Ja. Aktør og materiale lages én gang per kule. `EndBullet` kalles i `Tick` (kula ferdig) og i `AddNow` (taket på 600). Det finnes ingen andre steder kuler fjernes, og `Pending`-kuler har aldri strek. Telleren følger «har fått en strek» (`IsExplicitlyNull`), ikke «aktøren lever», så den kan ikke drive. Streken er `NoCollision`, uten skygge og uten tick (konstruktøren i `GSEffects.cpp`), så verken kuler eller spillere kan treffe den. |
| Smell/sus bare `!bReal`, én gang per kule, aldri egne, høyst ett per 0,05 s | Ja. Se punkt 3. |
| `GetStats` gir ikke lysspor-ammo bedre spredning; `TakeDamage` urørt | Ja, begge. |
| `GSDrawShotStreak` borte, `bFromMuzzle` borte | Ja. Ingen rester i `Source/`. |

## 2. Byggerens avvik: `SpawnTracer` er beholdt
Riktig valg. Planen sa at `grep SpawnTracer` i `GSCharacter.cpp` skulle bli tom, men gripekroken bruker den
(`ServerGrapple_Implementation`, `GSCharacter.cpp:2390`). Å slette den ville brutt bygget eller fjernet streken til
gripekroken. Merk (gammelt, ikke fra denne runden): kallet skjer i en server-RPC, så streken vises bare hos verten.
Klienter ser aldri sin egen gripekrok-line (LAV, legges i funn).

## 3. Sjekket og i orden
**Upålitelig ild og skuddkontoen.**
- To veier gir **ikke** dobbel konto: begge ender i `HandleFire` med samme `LastServerShotIndex` og samme `ShotCredit`.
  Sender en juksemaker samme skudd begge veier, faller det andre på `Jump <= 0`. Bruker han den raske veien for hagla
  eller den pålitelige for MG3, går skuddet gjennom de samme kontrollene. Han får ingenting han ikke kunne fra før.
- Forsvinner skudd merkbart? Bare ved pakketap. Motoren kaster også upålitelige server-RPC-er når forbindelsen er full
  (`NetDriver.cpp:3101`, `!Connection->IsNetReady()`). Men klientens grense er 100 kB/s (`DefaultEngine.ini`), og MG3
  bruker ca. 0,6 kB/s på skudd. Med 1–2 % tap mister du 1–2 % av skuddene. Du ser dem lande hos deg selv, men de gjør
  ingen skade. Det merkes ikke, og det er langt bedre enn å bli koblet fra.
- Ammo: serveren trekker bare for skudd som kom fram. `Weapons` replikeres, så klientens tall rettes opp etter et
  tapt skudd (han får det skuddet tilbake). Ingen gratis kuler.
- Opptil tre tapte på rad (`Jump <= 3`) bruker fortsatt samme frø, så kula du ser og serverens kule går likt.

**Lysspor.** Fargen og gløden er som den gamle streken. Den tennes 10 m ute. Lengden er `min(v·0,02 s; 15 m)` og
aldri lengre enn veien etter 10 m. På 30 bilder/s er første strek 13–28 m ut for 7,62. Flagg 8 regnes på serveren
fra klientens `ShotIndex`, så skytteren og alle andre ser lysspor på de samme kulene.

**Smell og sus.** Øret er kameraet til `GetFirstPlayerController()`, hentet én gang per `Tick` (riktig for vert,
klient og dødskamera). Verten hører ikke dobbelt: ekte kuler hopper over hele blokken, og `MulticastShotFX` lager
ikke en vist kule for egne skudd (`IsLocallyControlled`). Egne kuler hører du aldri (`EarPawn != Shooter`). Avstanden
måles mot hele strekket kula flyttet seg i bildet (`PointDistToSegment`), også det siste strekket mot veggen
(`B.Pos = End` i `Step`), så en kule på 28 m per bilde ikke hopper forbi øret. Grensen for over/under lydfarten:
.45 (260 m/s) suser hele veien, 9 mm smeller de første ca. 32 m og suser etter det, hagl smeller ut til ca. 24 m.
Det stemmer med virkeligheten. En haglskur gir ett smell. Lydfilene finnes (44,1 kHz mono, 0,65 og 0,30 s), og
`GSAudio::Get` husker manglende lyder, så de er stille uten hakk til editoren har importert dem.

**Holdemerker, regnet etter** (8x = FOV 11°, L96 = FOV 15°, 1920 px bredt, `MaintainXFOV` er standard):
| Kaliber, 8x | 150 m | 200 m | 250 m | 300 m | Avvik mot kula ved 300 m |
|---|---|---|---|---|---|
| 7,62 NATO | 4,2 px | 8,6 | 13,4 | 18,6 | kula 5 cm høyere |
| 7,62x54 | 4,3 | 8,9 | 13,8 | 19,1 | 5 cm |
| 5,56 | 3,5 (vises ikke, < 4 px) | 7,2 | 11,3 | 15,8 | 5 cm |
| 7,62x39 | 6,3 | 13,5 | 21,6 | 30,7 | 13 cm |
| .50 BMG | 3,4 (vises ikke) | 6,8 | 10,4 | 14,1 | 2 cm |
| L96 i egen sikt (7,62 NATO) | 3,0 (vises ikke) | 6,3 | 9,8 | 13,6 | 5 cm |

Planens kontrolltall (4 / 9 / 13 / 19 px) stemmer. Jeg har også flydd kula med samme integrator som `Step` (1/60 s og
1/30 s). For geværkalibrene treffer kula 1–5 cm over merket på 300 m. Det er godt innenfor et hode. Formelen
(`0,5·g·t²`) overser at lufta også bremser fallet, så merkene ligger litt for lavt. Det betyr bare noe for
pistolkaliber i 8x, se F3.

**F1 fra 5a (uint16).** `PackSpread`/`UnpackSpread` er `uint16` i 0,01°-trinn i alle tre bruksstedene. L96 siktet i
huk: 0,035 blir 0,04. Den minste kombinasjonen (L96 + 8x + huk + Grønn) blir 0,0059, som pakkes til 0,01 og
aldri 0. Ett byte mer per skudd. Rettet.

**F2 (objektspor).** Treffboksene på figurene er `ECC_Pawn` (`GSCharacter.cpp:161`), ragdoll er `PhysicsBody`.
Sporet `WorldStatic + WorldDynamic` stoppes altså ikke av en lagkamerat. Rakettprosjektiler (`BlockAllDynamic`) kan
gjøre det, men følgen er bare at skuddet starter fra serverens øye. Det er ufarlig. Rettet.

**F3 (lag i kula).** `Team` lagres i `FireReal`. Sjekken i `Step` brukes bare når `Instigator` er borte, ellers
avgjør `IsSparedFrom` som før. Den følger `bFriendlyFire` og `GSTeam::None` likt med `IsSparedFrom`. En lagkamerat
stopper kula og tar ingen skade. Rettet.

**Ytelse, 20 spillere med MG3.** Vist kule: ett ekstra punkt-mot-strekk-regnestykke per bilde (ubetydelig).
Lysspor: hver femte MG3-kule gir ca. 4 streker/s per skytter, 80/s for 20. Taket på 40 avgjør. Hvis alle bruker
lysspor-ammo og kulene treffer vegg etter 0,2 s, blir det i verste fall ca. 200 nye aktører og materialer per sekund.
Det koster under 1 ms per bilde. Godt nok, men en pool er bedre (forslag 2).

**Andreas' faste regel:** oppfylt. Forbedret: lysspor (484–488), skuddmeldingene (493–494), kikkerten (496),
spredningen (497), startpunktet (498) og venneild (499). Nytt: smell og sus (489–491), holdemerker (495) og flygende
lysspor med felles tak (492). `IMPROVEMENTS.md` 484–499 er ført i en egen seksjon, uten doble numre. Bestillingen til
Spillbarhet (teksten for «Lysspor») står øverst i `bestillinger.md`.

## 4. Funn
**F1. MIDDELS: et raskt skudd kan komme fram før våpenbyttet og bli skutt med det forrige våpenet –
`GSCharacter.cpp` `FireOnce`/`HandleFire` og `SelectSlot`.**
`ServerSelectSlot` er pålitelig og `ServerFireFast` er upålitelig. Går pakken med byttet tapt, sender klienten den
på nytt omtrent én tur-retur senere. Kommer et MG3-skudd fram i mellomtiden, bruker `HandleFire` serverens
`CurrentSlot`, altså det gamle våpenet. Det er ingen byttetid som stopper deg (`FireOnce` sjekker bare `FireInterval`
for det nye våpenet). Eksempler:
- L96 → MG3: serveren skyter én L96-kule (95 skade) i stedet for en MG3-kule.
- M72 → automatvåpen: serveren skyter raketten og kaster røret. Spilleren mister rakettkasteren uten å ha skutt med den.

Det skjer bare med pakketap og et skudd innen ca. 0,1–0,2 s etter byttet. Det er sjeldent, men det er nytt i denne
runden: før var begge meldingene pålitelige og kom i rekkefølge. Juks får ingenting nytt av det.
**Rettelse:** send våpenplassen med i begge skudd-RPC-ene (`uint8 Slot`, eller en kort hash av `W->Id`). I `HandleFire`:
`if (Slot != CurrentSlot) return;` (før `Jump`, så nummeret ikke flyttes). Det er ett byte og én linje.

**F2. LAV: en juksemaker kan slippe unna MG-lyssporene.** `GSIsTracer` bruker klientens `ShotIndex % 5`. Hopp på 2–3
godtas med samme frø, så en klient kan hoppe over alle nummer delelig med 5 og skyte MG3 uten lysspor, gratis.
**Rettelse:** på serveren blir skuddet et lysspor hvis et av numrene i `(LastServerShotIndex, ShotIndex]` er delelig med
5. Ærlige spillere ser det samme som før. Et tapt lysspor flyttes bare til neste skudd.

**F3. LAV: holdemerkene for pistolkaliber i 8x ligger for lavt.** Se tabellen i punkt 3. MP5/Thompson/UMP med 8x:
holder du 2-merket på 200 m, treffer kula 18–20 cm over målet, og på 300 m 70–80 cm over. Geværkalibrene er innenfor
5 cm (13 cm for 7,62x39). Rettelse når det passer: regn `HoldUnder` med samme steg som `Step` (en liten tabell per
kaliber), eller vis ikke merker for kaliber under 600 m/s.

**F4. LAV: smell fra en lagkamerat som står inntil deg.** Kula starter ved øyet hans, så står du under 3 m fra ham,
smeller hvert skudd hans hos deg (MG3: 20 i sekundet, som er taket). Smellet skilles i virkeligheten ikke fra
munningssmellet så nær løpet. **Rettelse:** krev `B.Travelled > ~500 cm`, eller at øret ligger foran munningen.

**F5. LAV (synlig): lyssporet slukker et bilde før kula treffer.** Når `Step` gir `false`, flyttes ikke streken det
siste bildet, så på 30 bilder/s forsvinner streken opptil 28 m før veggen. Rettelse: ved treff, sett streken én gang
til `End`, og la den leve ett bilde til (en kort levetid i stedet for `Destroy`, telleren trekkes ned som nå).

Andre små ting (ikke rett nå): Barrett (FOV 18°), Rem700 (22°) og SVD (25°) får ikke holdemerker i sin egen sikt,
bare L96 (15°) og 8x. Kommentaren sier «sniper rifles' own». `#include "GSPlayerState.h"` i `GSBallistics.cpp`
brukes ikke.

## 5. Hva Andreas skal teste (Number of Players: 2, Play As Listen Server, etter kampstart)
- Vanlige kuler synes ikke i lufta. Skyt MG3: omtrent hver femte kule er en lysende strek som flyr bortover i en bue
  og starter et stykke foran løpet. Bytt til lysspor-ammo: alle kulene lyser.
- La den andre skyte forbi hodet ditt (litt til siden): et skarpt smell rett ved deg. Med Thompson eller UMP hører du
  et sus i stedet. Dine egne kuler smeller aldri. (Lydene kommer først etter at Unreal er åpnet én gang etter bygget.)
- 8x-kikkert på et gevær, eller L96: små streker under midten, de brede er merket 2 og 3. Skriv `gs.ShowBullets 1`,
  sikt med 2-streken på en vegg ca. 200 m unna, og se at den gule buen ender der.
- Hold inne avtrekkeren på MG3 i klientvinduet i 10 sekunder: du blir ikke koblet fra, og ammoen går ned som den skal.
- Skyt L96, hagle og M72 i klientvinduet: hvert skudd går.

## Forslag til neste runde
1. F1 (våpenplass i skudd-RPC-ene) og F2 (lysspor regnet på serveren), sammen med V8 (tilbakespoling), som uansett
   endrer `HandleFire`. Begge er små.
2. Pool for lyssporene (40 aktører med materiale laget én gang og gjenbrukt), og la streken nå treffpunktet (F5).
3. Smell på større avstand med fall i styrke (ekte overlydssmell høres godt 10–20 m fra kulebanen), med F4 rettet.

## Lærdom
- Når en melding gjøres upålitelig, må den selv bære det den forutsetter fra pålitelige meldinger (våpen, modus).
  Ellers kan den komme før dem.
- Tall på skjermen som skal stemme med en simulering (holdemerker, siktepunkter) sjekkes mot selve integratoren,
  ikke bare mot formelen.
- Planer som sletter en funksjon må først søke etter alle som bruker den, i hele `Source/`.
