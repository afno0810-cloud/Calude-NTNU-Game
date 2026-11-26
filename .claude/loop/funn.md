# Funn fra kontrollørens feiljakt

Nyeste øverst. Planleggeren leser denne og tar de viktigste inn i planene.

## Områder som er gjennomgått
- 2026-11-26 (commit c055645): **loot/pickups** (`GSPickup`, `GSItems::RandomLoot`, plukk-/slipp-koden i
  `GSCharacter`), **prosjektiler/granater** (`GSProjectile`), **spawn og kampregler** (`GSGameMode`,
  `GSGameState`).
- 2026-11-26 (commit dc438f0): **våpen og skyting** (`GSCharacter` skyting, rekyl, spredning, lading,
  sikting, `TakeDamage`; skuddlyd i `GSAudio`; våpendata i `catalog.py`). Se «Feiljakt 2».
- Ikke gjennomgått ennå: boosts/effekter, PC-er og kameraer (`GSTerminal`), vær/døgn (`GSEnvironment`),
  HUD, campusbygging, bevegelse (mantle/skli/gripekrok), nærkamp i detalj.

## Funn

Alvorlighet: **Høy** = ødelegger kamper eller kan utnyttes lett. **Middels** = merkes tydelig online.
**Lav** = pirk som bør tas når man likevel er i filen. Linjenumre gjelder commit c055645.
Alt er funnet ved å lese koden, ikke ved å spille.

### Feiljakt 2: våpen og skyting (2026-11-26, commit dc438f0)

Lest: `GSCharacter.cpp`/`.h` (skyting, rekyl, spredning, lading, sikting, `TakeDamage`), `GSAudio.cpp`
(skuddlyd) og våpendata i `Tools/ItemCatalog/catalog.py`. Linjenumre gjelder commit dc438f0.
Del A er ekte feil. Del B er realismehull målt mot målet «superrealistisk» i `andreas.md`.

#### A. Ekte feil (juks, replikering, balanse)

V1. **Sikting (høyre museknapp) sendes aldri til serveren** – Høy (merkes av alle som ikke er vert)
   - Fil: `GSCharacter.h` linje 286 (`bool bAiming`, ikke replikert), `GSCharacter.cpp` `StartAim`/`StopAim`
     (516–527), `Tick` linje 3117, `ServerFire_Implementation` linje 820.
   - Hva: `bAiming` settes bare lokalt. Følger: (1) Serveren regner farten din som vanlig gange mens din
     egen maskin bruker sakte siktefart (linje 3117). De to er uenige, så klienter rykker/blir dratt
     fremover når de går og sikter. Sprint har egen `ServerSetSprinting`, sikting har ingenting.
     (2) Hagler: serveren bruker alltid hoftespredning for klienter (linje 820), mens verten får
     siktespredning. Klienten tegner haglene med siktespredning (linje 713), så det du ser treffer er ikke
     det serveren regner. (3) Andre spillere kan aldri se at du sikter.
   - Forslag: `ServerSetAiming(bool)` + `UPROPERTY(Replicated)` med `COND_SkipOwner`, akkurat som sprint.

V2. **Klienten bestemmer selv retningen på kula, med spredningen ferdig lagt inn** – Høy (juks)
   - Fil: `FireOnce` linje 691–693 og 761, `ServerFire_Implementation` linje 782–854.
   - Hva: spredning (hofte, bevegelse, hopp, rekyl) regnes bare hos klienten. Serveren bruker `Dir` som
     den er. En juksende klient får null spredning og null rekyl på alle våpen ved å sende ren
     sikteretning. Serveren sjekker heller ikke at `Dir` ligner retningen spilleren faktisk ser.
   - Forslag: klienten sender ren sikteretning + `Seed`. Serveren legger på spredningen selv (den kjenner
     fart, hopp, huk og – etter V1 – sikting), og avviser `Dir` som avviker mye fra kontrollrotasjonen.

V3. **Serveren tillater 43 % raskere skuddtakt enn våpenet har** – Høy (juks)
   - Fil: `ServerFire_Implementation` linje 787 (`FireInterval * 0.7f`).
   - Hva: en klient som sender skudd så fort serveren godtar, får MG3 opp i ca. 1700 skudd/min og Kar98k
     ned til 0,77 s mellom skuddene. Boltgrep/pumping håndheves bare på klienten.
   - Forslag: «skuddkonto» på serveren: fyll på 1 skudd per `FireInterval`, maks 1,5 på konto. Tåler
     nettverkshakk, men gir aldri høyere snittakt enn våpenet.

V4. **Spesialammo varer evig og gir gratis fullt magasin** – Høy (balanse, utnyttbart)
   - Fil: `CompleteUse` linje 2345–2361; `FinishReload` linje 987–996.
   - Hva: én eske panserbrytende/brann/sjokk-ammo setter `AmmoType` på våpenet og fyller magasinet
     gratis. `AmmoType` nullstilles aldri, heller ikke ved lading. Etter én eske skyter våpenet
     sjokk-kuler (legendarisk) resten av livet. I tillegg kan esken brukes som gratis omlading midt i
     kamp uten ladetid.
   - Forslag: tell spesialskudd (`SpecialRounds` i `FGSWeaponSlot`), bruk dem opp, og gå tilbake til
     vanlig ammo når de er tomme. Ikke fyll magasinet direkte; la dem gå gjennom vanlig lading.

V5. **Rakettkastere og granatkaster høres ut som lyddempede våpen for alle andre** – Middels
   - Fil: `ServerFire_Implementation` linje 800 (`MulticastShotFX(Start, ..., true)`), `GSAudio.cpp`
     `ShotName` linje 86 og `ShotRange` linje 103.
   - Hva: for prosjektilvåpen sendes `bSilent = true` (trolig for å slippe lysspor). Da spiller andre
     spillere «Shot_Silenced» med 40 m rekkevidde, og skuddet vises ikke på walkie-talkien. Carl Gustaf
     er i virkeligheten noe av det mest høylytte en soldat bærer (rundt 185 dB).
   - Forslag: egen parameter «uten lysspor» i stedet for å gjenbruke `bSilent`.

V6. **Hagle med lyddemper er ikke dempet for andre** – Lav
   - Fil: `ServerFire_Implementation` linje 836–837, `MulticastPelletFX_Implementation` linje 935–940.
   - Hva: pellet-grenen sender ikke `bSilent`, spiller alltid full haglelyd og viser alltid skuddet på
     radioen (`NoteShotForRadio` uten sjekk). Boostene «Iskald»/«Spøkelse» virker heller ikke for hagler.

V7. **`ServerFire` er en pålitelig (Reliable) RPC som sendes opptil 20 ganger i sekundet** – Middels
   - Fil: `GSCharacter.h` linje 248.
   - Hva: med MG3/Vector (0,05 s) og litt pakketap hoper pålitelige meldinger seg opp. Unreal kobler fra
     en klient som får for mange ubekreftede pålitelige meldinger i kø. Gamle skudd som kommer sent blir
     dessuten utført sent i stedet for å forkastes.
   - Forslag: gjør `ServerFire` Unreliable (tapte skudd rettes av ammo-replikeringen).

V8. **Ingen forsinkelseskompensasjon: klienter må sikte foran målet, verten slipper** – Middels
   - Fil: `ServerFire_Implementation` linje 841–853.
   - Hva: serveren sporer mot der spillerne står akkurat nå, ikke der skytteren så dem. Med 80 ms ping har
     en løpende spiller (6 m/s) flyttet seg ca. 0,5 m. Klienten tegner treff (lysspor mot målet) som
     serveren ikke godtar. Verten har ingen forsinkelse og får en klar fordel.
   - Forslag: lagre posisjonene til treffboksene de siste ~300 ms på serveren og spol tilbake til
     skytterens tidspunkt før sporet. (Faller delvis bort hvis kuler får flytid, se R1.)

V9. **Treffboksene er en stående sylinder og en kule som ikke følger kroppen** – Middels
   - Fil: konstruktøren linje 180–192, `TakeDamage` linje 2518.
   - Hva: kroppen er en sylinder 62 cm bred og 156 cm høy, hodet en kule på 30 cm (et ekte hode er ca.
     18 × 23 cm, altså nesten dobbelt så stort treffområde). De henger fast i kapselen og følger ikke
     animasjon, huk, skli eller lening. Når den synlige menneskekroppen (`SetupHumanBody`) vises,
     stemmer ikke det man ser med det man treffer: skudd mellom arm og kropp eller like ved hodet
     treffer, skudd mot en utstrakt arm bommer.
   - Forslag: treffbokser per bein på skjelettet (physics asset: hode, bryst, mage, armer, lår, legger),
     og bruk `Hit.BoneName` i `TakeDamage`. Dette er også grunnlaget for R4 og for sår der kula traff.

V10. **Full siktepresisjon i samme øyeblikk som høyre museknapp trykkes** – Lav/Middels
   - Fil: `GetCurrentSpread` linje 622, `StartAim` linje 519.
   - Hva: spredningen bytter fra hofte til sikte med en gang, mens våpenet bruker tid på å komme opp
     (`AimBlend`). «Quickscope» med L96 gir full presisjon før kikkerten er oppe.
   - Forslag: bland spredningen med samme `AimBlend` som animasjonen.

V11. **Våpenbytte tar ingen tid på serveren** – Lav
   - Fil: `ServerSelectSlot_Implementation` linje 547–556.
   - Hva: man kan skyte i samme øyeblikk som man bytter våpen; trekkeanimasjonen er bare pynt.
   - Forslag: skyv `ServerLastFireTime` frem med en trekketid per våpenklasse (pistol 0,4 s, gevær 0,7 s, MG 1,2 s).

V12. **Lyddemper passer på alt, også revolver, hagle, MG3, muskett og armbrøst** – Lav
   - Fil: `CompleteUse` linje 2298–2314 (`bGun` utelukker bare nærkamp og Launcher).

#### B. Realismehull (mot målet «superrealistisk»)

R1. **Kuler har ingen flytid og intet fall** – Høy
   - Fil: `ServerFire_Implementation` linje 841–843 (ett øyeblikkelig linjespor opptil 1000 m).
   - Virkelighet: 9 mm (ca. 360 m/s) bruker rundt 0,25 s på 80 m og faller ca. 25–30 cm. 7,62 NATO
     (ca. 800 m/s) bruker rundt 0,4 s på 300 m og faller ca. 0,8 m. .50 BMG (ca. 850 m/s) bruker rundt
     1,5 s på 1000 m og faller godt over 10 m. I spillet treffer alt i samme øyeblikk, rett frem.
   - Forslag: gjør kuler til raske, usynlige prosjektiler på serveren (stegvis linjespor per tick med
     munningsfart og tyngdekraft per kaliber, ev. luftmotstand). Armbrøst/rakett bruker allerede prosjektil.

R2. **Kula «forsvinner» ved rekkevidden, og skaden er lik på alle avstander** – Høy
   - Fil: `catalog.py` `range`-kolonnen (pistoler 25–100 m, SMG 50–70 m, gevær 280–400 m), brukt som
     sporlengde i linje 825 og 842. Ingen skadefall noe sted.
   - Virkelighet: en 9 mm-kule er farlig på over 1 km; «effektiv rekkevidde» handler om å treffe, ikke
     om at kula stopper. I spillet er en Makarov helt ufarlig på 61 m og gir full skade på 59 m.
   - Forslag: med R1 kan kula leve til den treffer noe, og skaden følge resthastigheten.

R3. **Ingen gjennomslag: alt stopper alle kuler** – Høy
   - Fil: linje 825/843 (`LineTraceSingleByChannel`, første treff stopper kula). Barrett-teksten sier
     «Gjennom vegger nesten», men en tredør eller en glassrute stopper .50 BMG.
   - Virkelighet: 7,62 NATO går gjennom gipsvegger, tredører, bildører og tykke trebord; 9 mm går
     gjennom gips og tynn finér; bare betong/tegl/stål stopper geværkuler. Kuler går også gjennom kropper.
   - Forslag: `LineTraceMulti` + gjennomslagsevne per kaliber og tykkelse/materiale per vegg (fysisk
     materiale: glass, tre, gips, betong, stål), med redusert skade etter hvert lag.

R4. **Skade per kroppsdel finnes ikke (bare hode eller «resten»)** – Høy
   - Fil: `TakeDamage` linje 2515–2531.
   - Hva: skudd i foten gjør like mye som skudd i hjertet. Vesten beskytter også armer og bein
     (linje 2556: alt som ikke er hode). Barrett i tåa dreper.
   - Virkelighet: bryst/hode er dødelig, armer og bein sjelden med én gang (men blør, og beinskudd gjør
     at man halter). Vest dekker bare overkroppen, hjelm bare skallen (ikke ansiktet).
   - Forslag (etter V9): soner hode / bryst / mage / armer / bein med egne faktorer, blødning, halting ved
     beinskudd og dårligere sikte ved armskudd. Gir også grunnlag for det groteske målet: sår og blod
     der kula traff.

R5. **Skaden følger ikke patronen; samme kaliber gir fra 36 til 95** – Høy (data)
   - Fil: `catalog.py` våpenlinjene og `GUN_CALIBER`.
   - 7,62 × 51 NATO: MG3 36, AG3 38, SCAR 40, Bren 40, FAL 42, M14 44, Garand 50, Rem700 82, L96 95.
     7,62 × 54R: PKM 38, SVD 70, Mosin 74. 9 mm: PPSh/Uzi 18, MP5 21, Glock 22, Luger 26.
   - Virkelighet: samme patron gir omtrent samme anslagsenergi (9 mm ca. 500 J, 5,56 ca. 1800 J,
     7,62 × 39 ca. 2000 J, 7,62 NATO ca. 3300 J, .50 BMG ca. 18 000 J); løpslengde betyr 5–15 %.
   - Følger i spillet (100 HP): et brystskudd fra Kar98k (75) dreper ikke; hodeskudd med Glock uten hjelm
     (22 × 2 = 44) dreper ikke; Barrett (140) mot keramisk vest (60 % sluk) gir 56 – man overlever
     .50 BMG i brystet.
   - Forslag: flytt skade til en kalibertabell (+ liten løpsfaktor per våpen). Hodeskudd uten hjelm skal
     alltid drepe. Geværkaliber i brystet uten vest: 1–2 skudd. Pistol: 2–4.

R6. **Feil kaliber på flere historiske våpen** – Middels (data)
   - Fil: `catalog.py` `GUN_CALIBER` linje 382–390.
   - PPSh-41 er 7,62 × 25 Tokarev (står 9 mm). Makarov er 9 × 18 (deler eske med 9 × 19). Kar98k er
     7,92 × 57 Mauser (står 7,62 × 54R). Krag-Jørgensen er 6,5 × 55 (står 7,62 × 54R). M1 Garand er
     .30-06 (står 7,62 NATO). L96 «AWM» er .300 Win Mag/.338 Lapua (står 7,62 NATO). Desert Eagle og
     .357-revolver deler eske.
   - Forslag: egne kalibre (7,62 × 25, 9 × 18, 7,92 × 57, 6,5 × 55, .30-06, .338), eller bytt til varianter
     som faktisk bruker kalibrene som finnes.

R7. **Rustning stopper en fast prosent uansett kaliber** – Høy
   - Fil: `TakeDamage` linje 2545–2565, `catalog.py` `armor(...)` (absorb 0,3–0,7).
   - Virkelighet: myk kevlarvest (NIJ IIIA) stopper pistolkuler helt (man får et kraftig slag og
     blåmerker) og nesten ingenting av geværkuler. Keramiske plater (nivå IV) stopper 7,62 NATO, men
     sprekker etter få treff, og stopper ikke .50 BMG. I spillet tar «Myk vest» 35 % av en Barrett-kule
     og bare 35 % av en 9 mm.
   - Forslag: beskyttelsesklasse per vest/hjelm og gjennomslagsklasse per kaliber: stoppet (liten
     slagskade, vesten slites) eller gjennomslag (nesten full skade). Hjelm stopper splinter og pistol,
     ikke gevær.

R8. **Lading er «løse skudd i en pott», uten magasiner, kammer eller avbrudd** – Middels
   - Fil: `ServerReload_Implementation`/`FinishReload` linje 969–996.
   - Hva: alle våpen fyller opp til fullt fra en felles reserve etter én fast tid. Virkelighet: man bytter
     hele magasiner (et halvtomt magasin beholder sine skudd); et våpen med skudd i kammeret har
     magasin + 1 etter lading; tomt våpen krever ladegrep (lengre tid); pumpehagle, revolver og Krag
     lades ett og ett skudd og kan avbrytes for å skyte (i spillet 3,2 s låst for 6 hagl); Garand lades
     med 8-skudds ramme; MG3 har belte og lokk. Musketten bruker 6 s (en trent soldat brukte 15–20 s).
   - Forslag: magasinliste per våpen, +1 i kammer, egen tom-lading, og skudd-for-skudd-lading som kan avbrytes.

R9. **Spredning er en usynlig tilfeldig kjegle, ikke våpenets egen presisjon** – Middels
   - Fil: `GetCurrentSpread` linje 617–629, `catalog.py` `hip`/`aim`.
   - Hva: kula går tilfeldig inntil 1,5–3° ut fra siktepunktet fra hofta, mer ved løping, +4° i lufta, og
     mer jo mer rekyl. Siktet AK-47 har 0,35° (21 MOA; en ekte AK ligger på 3–5 MOA = ca. 0,07°), MG3
     1,2°. Virkelighet: kula går dit løpet peker. Unøyaktighet kommer av at våpenet beveger seg i hendene.
   - Forslag: liten mekanisk spredning per våpen (0,02–0,1°), og la resten være synlig våpenbevegelse
     (svai, pust, bevegelse, utmattelse) som faktisk flytter løpet – slik kikkertsvai alt gjør (`TickScopeSway`).
   - I tillegg: hagler får trangere sverm av å sikte (5° → 3,5°, linje 820). Svermen bestemmes av
     trangboringen, ikke av siktingen. Ekte hagl sprer seg ca. 2,5 cm per meter (ca. 1,5°).

R10. **Rekyl: fast «spraymønster», 60 % kommer tilbake av seg selv, og tallene stemmer ikke innbyrdes** – Middels
   - Fil: `FireOnce` linje 722–733, `TickRecoil` linje 1726–1739, `catalog.py` `recoil`.
   - Hva: sidebevegelsen er en fast sinuskurve per våpen (linje 731–732), altså et lærbart mønster som i
     arkadeskytere. Siktet går automatisk 60 % tilbake. M1911 (1,4°) sparker mer enn AG3 (1,1°) og AK
     (1,0°); en AG3 på helautomat er i virkeligheten svært vanskelig å holde. Rekylen bryr seg ikke om
     stilling (stående/huk), tofot (MG3, Bren, Barrett har modellert tofot uten virkning) eller støtte.
   - Forslag: rekyl fra impuls (kulevekt × fart / våpenvekt), tilfeldig sidekast, ingen automatisk
     retur utover skulderens fjæring, og tofot/huk som demper kraftig.

R11. **Skuddlyd: ingen forsinkelse, for kort rekkevidde, ingen kuleknall** – Middels
   - Fil: `GSAudio.cpp` `PlayShotFor` linje 112–133, `ShotRange` linje 101–110; `MulticastShotFX` linje 914–933.
   - Hva: lyden spilles med en gang uansett avstand. Lyd går 343 m/s: et skudd 300 m unna skal høres
     0,9 s etter munningsflammen. Pistol høres bare til 200 m og gevær til 300 m (virkelig: godt over
     1 km i åpent lende). Det finnes ingen overlydsknall/sus når en kule passerer nær hodet. Lyddempet
     våpen høres bare 40 m (et dempet gevær med vanlig ammo er fortsatt rundt 130–140 dB), og demper
     trekker 8 % skade (linje 570), noe en ekte demper ikke gjør.
   - Forslag: forsink lyden med avstand / 343 s, øk rekkevidden med lavpass på avstand, og legg til
     knall + sus for kuler som passerer innen ca. 3 m.

R12. **Alle skudd tegner lysspor** – Lav/Middels
   - Fil: `FireOnce` linje 705 og `MulticastShotFX` linje 924; «Lysspor»-ammo gir bedre presisjon (linje 573).
   - Virkelighet: vanlig ammo er usynlig i lufta. Sporlys er egne kuler (typisk hver femte i MG-belter)
     og treffer ikke bedre. «Lysspor»-esken burde være det eneste som gir synlig spor.

R13. **Prosjektilfarter er for lave, og piler faller halvt så fort som de skal** – Lav (data)
   - Fil: `catalog.py`: Carl Gustaf 90 m/s (virkelig ca. 240–255), M72 110 (virkelig ca. 145), M79 60
     (virkelig 76), armbrøst 80 med tyngdekraft 0,5 (virkelig ca. 100–120 m/s og full tyngdekraft).

R14. **Skuddtakt og magasin: stort sett riktig, noen avvik** – Lav (data)
   - Uzi 705 skudd/min (virkelig ca. 600). M14 500 (virkelig 700–750). P90 1090 (virkelig ca. 900).
     MG3 1200, AK 600, AG3 600, MP40 545, PPSh 1000 og magasinstørrelsene stemmer godt.
   - Ingen våpen har ildvelger (enkeltskudd/automat); AG3, AK, HK416 og FAL burde kunne veksles.

R15. **Døde kropper kan ikke treffes** – Lav/Middels (grotesk-målet)
   - Fil: `OnRep_Dead` linje 2777–2779 (all treffkollisjon skrus av).
   - Hva: kuler går tvers gjennom lik uten blod eller reaksjon, og liket gir ingen dekning.
   - Forslag: la ragdollen blokkere skuddsporet og gi blodsprut/rykk ved treff.

### Feiljakt 1 – loot, prosjektiler, kampregler (2026-11-26)

1. **Vester og hjelmer kan repareres gratis ved å bytte frem og tilbake** – Høy (utnyttbart)
   - Fil: `GSCharacter.cpp` `TakeFromPickup` (ca. linje 1853–1868), `DropEverything` (ca. 2289–2290),
     `GSPickup.cpp` `SpawnDropped` (60–83).
   - Hva: når du tar på en vest/hjelm settes poengene alltid til full verdi (`Points = Def->Armor`), og
     den gamle slippes som en ny pickup uten at slitasjen følger med. Med to vester kan en spiller bytte
     mellom dem og ha full rustning hele tiden. Rustning fra døde spillere er også alltid «ny».
   - Forslag: gi `AGSPickup` et felt `ArmorPoints` (-1 = ny), send det inn i `SpawnDropped`, og bruk
     det i `TakeFromPickup`.

2. RETTET – **Drap med granat, mine, molotov, rakett og pil telles ikke hvis kasteren dør først** – Høy
   - Fil: `GSProjectile.cpp` linje 140 og 200 (`GetInstigatorController()`), `GSCharacter.cpp` `Die`
     (`DetachFromControllerPendingDestroy`, ca. linje 2529).
   - Hva: prosjektilet finner «hvem drepte» via pawnen som kastet. Når kasteren dør, slipper kontrolleren
     kroppen, og `GetInstigatorController()` blir null. Da går drapet til «Verden»: ingen poeng, feil
     tekst i killfeed, og offeret får ikke drapskort. Gjelder også miner etter at eieren har respawnet.
   - Forslag: lagre `TWeakObjectPtr<AController> OwnerController` i `Launch` og bruk den i `TakeDamage`-kallene.

3. RETTET – **Tilfeldig loot følger ikke kategorivektene: nesten ingen medisin, rustning og granater** – Høy (balanse)
   - Fil: `GSItems.cpp` `RandomLoot` linje 211–231.
   - Hva: vekten legges sammen per gjenstand, ikke per kategori. Katalogen har 59 våpen, 49 utstyr og
     32 mat, men bare 5 vester, 4 hjelmer, 5 medisiner og 6 kastevåpen. Resultat (før sjeldenhet): ca.
     45 % våpen, 22 % mat, 22 % utstyr, men bare ca. 2 % medisin, 1,5 % granater, 1 % vester og 0,5 %
     hjelmer. Av ca. 410 vanlige lootplasser blir det omtrent 8 medisiner og 4 vester på hele campus
     til 20 spillere. `CatWeight` (12 for medisin, 6 for vest) sier at meningen var noe helt annet.
   - Forslag: trekk kategori først (etter `CatWeight`), deretter sjeldenhet, deretter en gjenstand i den gruppen.

4. RETTET – **Granater og raketter hakker og faller feil hos klienter** – Middels (bare synlig online, ikke hos verten)
   - Fil: `GSProjectile.cpp` `Launch` linje 63–68 og konstruktøren.
   - Hva: fart, tyngdekraft og sprett settes bare på serveren. Hos klientene starter
     `ProjectileMovementComponent` med fart 0, tyngdekraft 1 og uten sprett, og motorens
     `PostNetReceiveVelocity` setter ikke farten på bevegelseskomponenten. Klienten simulerer derfor et
     fall rett ned mellom hver posisjonsoppdatering fra serveren, og stopper sin egen simulering ved
     første gulvkontakt. Raketter (tyngdekraft 0 på server) «synker» og rykker.
   - Forslag: repliker startfart, tyngdekraft og sprett (eller overstyr `PostNetReceiveVelocity` og sett
     `Movement->Velocity`), og sett dem i `BeginPlay`/OnRep på klienten.

5. RETTET – **Snubletrådminen forsvinner etter 60 sekunder** – Middels
   - Fil: `GSProjectile.cpp` linje 81 (`SetLifeSpan(60.f)` for alle prosjektiler), `OnStop` linje 171–185.
   - Hva: miner (Fuse = 0) får aldri lengre levetid når de har festet seg, så en mine som ingen går på
     blir stille borte etter ett minutt. Det gjør miner nesten ubrukelige som felle.
   - Forslag: sett levetiden på nytt i `OnStop` for `mine` (for eksempel 5–10 minutter), og ha et tak
     på antall miner per spiller.

6. **Loot kan tømmes i lobbyen, og miner/ild fra lobbyen blir med inn i kampen** – Middels
   - Fil: `GSGameMode.cpp` `StartMatch` linje 91–117.
   - Hva: ved kampstart nullstilles spillerne, men ikke lootplassene. Alt som ble plukket under
     oppvarmingen (også den mytiske kassen, som bruker 150–240 s på å komme tilbake) mangler når kampen
     starter. Ting som er sluppet på bakken, og miner/molotov-ild lagt i lobbyen, ligger også igjen og
     gjør skade så snart fasen er `Playing`.
   - Forslag: i `StartMatch`: slett alle `AGSProjectile` og alle slupne pickups (`bRespawns == false`),
     og kall `Roll()` på alle faste pickups.

7. **Én spiller kan hindre kampstart for alltid** – Middels (griefing)
   - Fil: `GSGameMode.cpp` `SetReady` linje 33–43 og `Tick` linje 53–63.
   - Hva: etter 90 s starter nedtellingen automatisk selv om ikke alle er klare. Men hvis en spiller
     trykker klar og så ikke-klar under nedtellingen, går fasen tilbake til lobby, og nedtellingen
     starter på nytt med 10 nye sekunder. Dette kan gjentas uten grense.
   - Forslag: husk om nedtellingen ble startet av tidsgrensen; da skal «ikke klar» ikke avbryte den.

8. **Loot og låste kasser kan tas gjennom vegger** – Middels
   - Fil: `GSCharacter.cpp` `UpdateFocusPickup` (ca. linje 1939–1953) og `ServerInteract_Implementation`
     (ca. linje 1981–2020).
   - Hva: både klienten og serveren sjekker bare avstand (2,8 m / 3,5 m), ikke fri sikt. Man kan plukke
     og dirke opp kasser fra naborommet eller fra utsiden av bygget.
   - Forslag: ett linjespor (`ECC_WorldStatic`) fra øyet til pickupen på serveren før plukking.

9. **Raketter, piler og kastede ting kan starte på andre siden av en vegg** – Middels (juks og vanlig spill)
   - Fil: `GSCharacter.cpp` `ServerFire_Implementation` (ca. linje 782–803) og `ThrowItem` (ca. linje 2256–2263).
   - Hva: prosjektilet lages 80 cm foran `Start` (50 cm foran kameraet ved kast) uten å sjekke om det er
     en vegg imellom. Står man inntil en tynn vegg eller dør, skyter man gjennom den. I tillegg godtar
     serveren at `Start` er opptil 2,5 m fra det ekte kameraet, så en juksende klient kan starte skudd
     over 3 m unna seg selv, bak dekning.
   - Forslag: linjespor fra serverens kameraposisjon til startpunktet; ved treff startes prosjektilet
     rett foran veggen (eller eksploderer der). Stram inn 250 cm til rundt 100 cm.

10. **Røyk og ild vises feil for spillere som kommer til etterpå** – Lav/Middels
    - Fil: `GSProjectile.cpp` `OnRep_Exploded` linje 233–262.
    - Hva: effekten lages én gang når `bExploded` kommer til klienten, alltid med full varighet (røyk 20 s,
      ild 8 s). En spiller som kommer innenfor nettrekkevidde sent, ser røyk eller ild som varer lenger
      enn hos de andre (røyk opptil ca. 20 s). Den ene ser gjennom røyken, den andre ikke, og ilden som
      henger igjen gjør ikke skade.
    - Forslag: repliker tidspunktet for eksplosjonen og kort ned effekten med tiden som har gått.

11. **Ca. 430 pickups ticker hver frame hos alle, og hver har egne materialer** – Middels (ytelse)
    - Fil: `GSPickup.cpp` linje 12 (`bCanEverTick`), `Tick` 103–112, `OnRep_State` 114–155;
      `GSUtils.h` `SetColor`; `GSGameMode.cpp` `SpawnPickups`.
    - Hva: det lages ca. 352 lootplasser inne, 16 kasser og 60 ute. Hver pickup har 4 mesh-komponenter og
      roterer hver frame på alle maskiner, uansett avstand. `OnRep_State` lager 2–3 nye dynamiske
      materialer hver gang den kjøres (og den kan kjøres flere ganger per endring, én gang per replikert
      felt som endret seg), så ingenting kan tegnes samlet. Pickupene har heller ingen nett-dvale, så
      serveren vurderer alle for alle spillere to ganger i sekundet.
    - Forslag: skru av tick når ingen lokal spiller er nærmere enn ca. 40 m (eller roter i materialet),
      lag ett delt materiale per sjeldenhet, sett `NetDormancy = DORM_Initial` og vekk med
      `FlushNetDormancy()` når noe endres, og sett maks tegneavstand på ring og stråle.

12. **Bare 16 spawnpunkter til 20 spillere, og ingen beskyttelse ved spawn** – Lav/Middels
    - Fil: `GSCampus.h` `SpawnPoints` linje 82–91, `GSGameMode.cpp` `PickSpawnTransform` linje 229–257.
    - Hva: ved kampstart med mer enn 16 spillere må flere dele samme punkt (nøyaktig samme koordinat) og
      starter helt inntil hverandre med våpen. Det finnes heller ingen kort udødelighet etter respawn.
    - Forslag: minst 30 punkter, litt tilfeldig forskyvning rundt punktet, og 2 s spawnbeskyttelse.

13. **Ammoeske kan gi både skudd og hel eske** – Lav
    - Fil: `GSCharacter.cpp` `TakeFromPickup`, ammo-delen (ca. linje 1873–1894).
    - Hva: mangler våpenet for eksempel 5 skudd og esken har 30, får spilleren 5 skudd, og hele esken
      legges likevel i sekken (`UsedBoxes` rundes ned til 0). Litt gratis ammo ved hver eske.
    - Forslag: regn en påbegynt eske som brukt (rund opp), eller ikke fyll fra en eske som ikke brukes helt.

14. **Minen utløses gjennom vegger og gulv** – Lav
    - Fil: `GSProjectile.cpp` linje 154–161.
    - Hva: minen sjekker bare avstand (2,5 m) til spillere, uten sikt. En spiller i naborommet utløser den,
      men tar ingen skade fordi selve skaden har veggsjekk. Minen er da brukt opp til ingen nytte.
    - Forslag: samme linjespor som i `DamageAround` før minen går av.

15. **Vinner kåres selv om ingen har drap** – Lav
    - Fil: `GSGameMode.cpp` `EndMatch` linje 119–131.
    - Hva: hvis tiden går ut uten drap, blir den første spilleren i listen «vinner».
    - Forslag: tom vinner (uavgjort) når beste spiller har 0 drap, og vis uavgjort ved lik stilling.
