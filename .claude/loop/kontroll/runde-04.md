**MÅ FIKSES**

# Kontroll – runde 4 (lag Kamp): kroppssoner
Kontrollert: commit `60a516c` (GSCharacter.h/.cpp, GSHUD.cpp, rettinger.md) + IMPROVEMENTS 360–375 i `0f788c6`.
Bygg: ikke kjørt av meg (byggeren: Result: Succeeded). Koden er lest fra commiten, motorpåstandene er sjekket
mot `UE_5.8/Engine/Source/Runtime/Engine/Private/Character.cpp` og `CharacterMovementComponent.cpp`.

Én ting må rettes (skuddkontoen, én linje). Resten av runden er godkjent slik den står.

## Andreas' faste regel
- **Forbedret (gammelt):** treffboksene, skademodellen (vest/hjelm dekker bare det de dekker), medisin behandler
  lemmer, sikting sendes til serveren, rakettkaster-lyden, kroppen andre ser står på bakken, HUD uten arkadetekst,
  musehjul/styreflate. OK.
- **Nytt:** sju treffsoner, beinskade og armskade med følger, «du kjenner hvor du ble truffet», andre ser at du
  sikter, `gs.ShowHitZones`. OK.

## Funn

### 1. MÅ FIKSES – skuddkontoen (V3) avviser ekte skudd ved ujevn levering
`GSCharacter.cpp`, `ServerFire_Implementation`, linjen
`const float MaxCredit = FMath::Clamp(0.15f / FMath::Max(S.FireInterval, 0.01f), 1.1f, 3.f);`

Serveren ser skuddene først når bildet dens starter, så ankomsttidene hakkes opp i serverens bildetid
(33 ms når verten går i 30 bilder/s) og forskyves av vanlig nettjitter. Med tak 1,1 tåler kontoen bare at et
skudd kommer ca. 10–15 % av `FireInterval` «for tidlig» (Luger 0,16 s: 17–23 ms; M1911 0,18 s: 19–26 ms;
Makarov: 20 ms). Jeg regnet det ut med en simulering (klient → 50 ms ping → serverbilde), ny regel / gammel regel:

| Våpen, hvordan det skytes | Vert 30 b/s, 0 jitter | Vert 30 b/s, 30 ms jitter | Vert 60 b/s, 30 ms jitter |
|---|---|---|---|
| Luger, klikker så fort våpenet tillater | 26 % / 0 % | 30 % / 0 % | 12 % / 0 % |
| Makarov, samme | 0 % / 0 % | 9 % / 0 % | 17 % / 0 % |
| M1911 (startvåpenet), samme | 1 % / 0 % | 7 % / 0 % | 9 % / 0 % |
| Luger, vanlig rask klikking 6–8 per sekund | 7 % / 0 % | 9 % / 0 % | 3 % / 0 % |
| Makarov, vanlig rask klikking | 0 % / 0 % | 3 % / 0 % | 5 % / 0 % |
| Automatvåpen (AK, MP40, Bren, Uzi) | 0 % | 0–0,5 % | 0–1 % |

I tillegg: hver gang verten hakker (ett bilde på 150 ms) mister et automatvåpen ett skudd (AK, MP40, Bren, Uzi),
fordi to skudd i samme serverbilde trenger 2,0 på kontoen og taket er 1,5–1,9.
Et avvist skudd ser ekte ut hos skytteren (lyd, lysspor, rekyl, ammo trukket lokalt), men gjør ingen skade, og
klientens ammo står én for lavt til neste godkjente skudd. Andreas tester som vert på laptop, så dette treffer.
(Til runden sin fordel: den gamle 0,7-regelen kastet halvparten av MG3-skuddene på en vert i 30 b/s; det er borte nå.)

**Rettelse:** la kontoen alltid romme minst to skudd, f.eks.
`const float MaxCredit = 1.f + FMath::Clamp(0.15f / FMath::Max(S.FireInterval, 0.01f), 0.5f, 2.f);`
Samme simulering gir da 0 % avviste skudd i alle radene over, og 0 tapte skudd per hakk for AK/MP40/Bren.
Jukseverdien er ett ekstra skudd én gang; snittakten er fortsatt låst til våpenets (5 % slakk), så
IMPROVEMENTS 372 står seg. `ServerSelectSlot` (`Min(ShotCredit, 1)`) kan stå.

### 2. Små ting (ikke grunn til retur, tas når det passer)
- `ClientTookHit_Implementation` (ca. linje 1815): hvert haglkorn i beinet gir sitt eget knekk
  (`CamDipVel -= 90…230`), uten tak. Ni korn fra pumpehagla gir rundt 40 cm kameradykk; ni korn i armen gir ni
  `AddWeaponKick`. Bruk samme «én gang per øyeblikk»-vakt som lyden rett under (f.eks. 0,06 s).
- `Tick` (ca. linje 3470): beinskaden senker `MaxWalkSpeed`, men ikke `MaxWalkSpeedCrouched` (fast 200). Med hardt
  skadd bein går man 231 stående, 143 stående med sikte, men 200 på huk (også med sikte). Gang ned hukefarten med
  samme faktor.
- `GSHUD.cpp` linje 245: teksten «du kan ikke løpe» bruker `LegInjury`, mens sperren bruker `GetLegPain()`. Med
  smertestillende (skade 0,5–0,99) kan man løpe mens teksten sier nei. Bruk `GetLegPain() >= 0.5f` for den delen.
- `GSCharacter.h`: kommentaren på `MulticastShotFX` nevner bit 1 og 2, men koden bruker også bit 4 (rakettkaster).
- `LegInjury`/`ArmInjury` replikeres til alle. Bare eieren og serveren bruker dem; `COND_OwnerOnly` holder
  (og en juksemaker ser ikke hvem som er skadd).
- `MulticastShotFX` for armbrøst: `PlayShotFor(Bow, false)` prøver også `Shot_Distant` over 60 m. Den har rekkevidde
  60 m og høres ikke, men lyden startes unødig. Ufarlig.

## Det som er sjekket og funnet i orden

**De sju boksene.** Regnet ut fra tabellen: stående bein −92…−2, mage −2…33, bryst 33…65, hode 65…89; huk
−58…−8, −8…20, 20…50, 50…74. Hode 19 × 16 × 24 cm, bryst 24 × 36 × 32, skulderbredde med armer 60 cm, bein 18 × 17 × 90.
Det er ekte kroppsmål (hodet før: kule med 30 cm tverrmål). Glippene (2 cm mellom beina, 1 cm arm/bryst) er
naturlige. Alt ligger innenfor kapselens radius på 40, så ingenting stikker gjennom vegger sideveis. På huk
stikker hodeboksen 16 cm over kapselen (kameraet står der også, som før). `SetupHittable` ignorerer alt unntatt
`ECC_Visibility`, så boksene stopper ikke bevegelse, granater eller navigasjon. Sylinder og kule har ikke lenger kollisjon.
- **Huk/glid:** `OnStartCrouch`/`OnEndCrouch` kalles av motoren på server, eier og andre (via `OnRep_IsCrouched`,
  også for sene klienter). Glid bruker `Crouch()`. All treffregistrering skjer på serveren (`ServerFire`,
  `DoMeleeHit`, pil i `GSProjectile` – alle sporer `ECC_Visibility` og får boksen som komponent), så server og
  klienter kan ikke bli uenige om skade; klientens eget spor er bare lysspor.
- **Kostnad per skudd:** sju enkle bokser per spiller (140 ved 20) i stedet for to trekantmesher med kompleks
  kollisjon. Billigere enn før. Feilsøkingstegningen koster ingenting når `gs.ShowHitZones` er 0.
- **Nærkamp:** kulesveipet (radius 22) treffer brystets forside (X=12) før armene (X=6) når man slår rett forfra,
  så kniven blir ikke et «armvåpen».
- **Eksplosjoner/ild:** kommer uten punkttreff → sone `None` → ingen lemfaktor, vesten tar sin del som før.

**Skade per sone (100 i helse, uten rustning).** M1911 28: bryst 4 skudd, mage 4 (25,2), bein 6 (18,2), arm 7 (15,4),
hode 2. AK 34: bryst 3, mage 4, bein 5 (22,1), arm 6 (18,7). Kar98k 75: bryst 2, bein 3 (48,8), arm 3 (41,3).
Barrett 140: bein 91 – man overlever så vidt ett skudd i beinet. Forholdstallene (0,55 / 0,65 / 0,9) er rimelige;
at brystet tåler tre–fire skudd er våpentallene til lag Spillbarhet, ikke denne runden. Blødning (neste steg) er
det som skal gjøre arm- og beinskudd farlige over tid.
- **Vest/hjelm:** vest bare bryst, mage og eksplosjoner; hjelm ikke den nedre delen av hodet forfra
  (`LocalZ < −2,5` og skudd mot ansiktet). Myk vest (0,35): M1911 i brystet 18,2 – likt et beinskudd. Henger sammen.
  `bArmored` for hulspiss følger samme dekning. Runde 3-koden (`WoundPoint`, `MulticastWoundFX`) står urørt og før `Die`.

**Bein- og armskade.**
- Verdier: M1911 i beinet 0,40 per treff (to treff = ingen sprint), AK 0,49 (akkurat under grensen), AG3 0,55,
  Kar98k 1,0. Ved 1,0: fart 55 %, hopp 60 %. Arm: M1911 0,39, AK 0,47; ved 1,0: lading +60 %, rekyl +50 %,
  hoftespredning +1,5°, siktet vandrer 0,5° og skjelver 0,07°.
- Heling: plaster 0,25, bandasje 0,5, smertestillende 0,08 (og halv virkning i 25 s), skrin og kirurgsett helt
  bra. Medisin kan brukes også med full helse (ingen sperre i `ServerUseItem`), så man blir ikke stående skadd
  uten mulighet til å behandle.
- Nullstilling: alle veier til nytt liv ødelegger kroppen og lager en ny (`RestartPlayer`), så skadene er 0.
  Skade settes ikke ved dødelig treff og ikke i lobby (ligger etter de tidlige returene – riktig for venneild senere).
- Juks: settes bare på serveren (`TakeDamage`, `CompleteUse`). Serveren regner selv fart, hopp, sprintsperre og
  ladetid, så en klient som later som den er frisk blir rettet tilbake. Rekyl og sving er klientstyrt (kjent, V2).

**Fart/hopp/sprint mot bevegelsesforutsigelsen.** Samme mønster som effekter og utstyr alt bruker: server og eier
regner `MaxWalkSpeed`/`JumpZVelocity` fra replikerte verdier hver tick. Når man blir truffet, vet serveren det én
ping før eieren, så det kan komme ett lite rykk akkurat da (og når smertestillende går ut). Ikke vedvarende.
Sprint: `Tick` slår av på både server og eier, `StartSprint` og `ServerSetSprinting` har samme grense (< 0,5).

**V1 `ServerSetAiming`.** `bAiming` settes bare i `StartAim`, `StopAim`, `ServerSetAiming_Implementation` og `Die`.
Verten sender ikke RPC til seg selv; klienten sender pålitelig RPC ved trykk og slipp (ikke hver tick).
`COND_SkipOwner` gjør at eierens egen forutsigelse ikke overskrives. Serveren bruker nå siktefart og
siktespredning for hagl. Å lyve om sikting gir tettere hagl, men siktefart – samme bytte som en ærlig spiller.

**V5 og den ekstra biten.** Alle tre kall til `MulticastShotFX` sender `uint8`. Bit 4 (rakettkaster) er riktig tenkt:
engangsrøret fjernes fra våpenlisten i samme `ServerFire`, så mottakeren kan ikke lese klassen fra våpenet.
Carl Gustaf og M79 gir `Shot_Launcher` og vises på radioen; armbrøst gir strengelyd innen 40 m og ingen radio.
`if (bNoTracer) return;` hindrer falskt nedslag. `NoteShotForRadio` kjøres før eier-returen, som før.

**`CacheInitialMeshOffset`.** Stemmer med motoren: `PostInitializeComponents` lagrer (0,0,0) før `BeginPlay`,
`SmoothClientPosition_UpdateVisuals` setter skjelettet til `BaseTranslationOffset` + utjevning hver tick hos
klienter, og `ACharacter::OnStartCrouch/OnEndCrouch` setter Z fra klassens standard (0). `FixBodyMeshOffset`
kjøres etter `Super` og setter −92 (+34 på huk, fordi kapselsenteret synker 34) og samme −90° som før. Sene
klienter som kommer inn mens noen huker dekkes av `bIsCrouched`-grenen i `SetupHumanBody`. Ragdoll røres ikke.

**Scroll.** Samler brøkdeler, nullstiller ved retningsbytte eller 0,3 s pause, maks tre trinn per hendelse, resten
klemmes til ±1. Et vanlig hjulhakk (1,0) gir fortsatt ett trinn. I terminalen: «Q» fram; tilbake er «1» på printeren
og «F» på PC-en – sjekket mot `TerminalInput`: «F» gjør bare noe i kameramodus, så den kan ikke starte noe annet.

**HUD.** «LAVT LIV», det hvite blinket og «HODESKUDD» under siktet er borte. Skadeteksten er liten, dempet og
gjelder bare deg selv. «HODESKUDD» finnes fortsatt på dødskortet (etter planen).

**«Ferdig når».** Ingen sammenligning med `HeadMesh` igjen; `TakeDamage` bruker `ZoneOf`; ingen `* 0.7f` i
`ServerFire`; IMPROVEMENTS 360–375 er inne (361 er byttet til musehjulet, hodet er slått sammen med 360 – greit).
Ingen filer utenfor laget er rørt.

## Forslag til neste runde
1. **Blødning** (sammen med lag Spillbarhet): mageskudd blør mest, arm/bein blør til det bandasjeres. Da blir
   arm- og beinskudd farlige av riktig grunn, og bandasjen får hele jobben sin. Ta rettinger punkt 9 samtidig.
2. **Fallskade som beinskade:** høye fall bør gi `LegInjury` (og litt helsetap). Alt som trengs finnes nå.
3. **Kameraet står 11 cm under midten av hodeboksen** (øye på +66, hode 65…89): 23 cm hode stikker over
   øyehøyden, så man kan bli truffet i hodet bak dekning man ikke ser over. Flytt kameraet til ca. +77 når kroppen
   får ordentlige animasjoner og bein-festede bokser.

## Lærdom
- Server-sjekker på tid må regnes mot serverens bildetid og vanlig jitter (30 b/s vert, 30 ms), ikke mot perfekt
  levering: en konto/teller må alltid romme minst to hendelser i samme serverbilde.
- Følger som ganges inn i fart må ganges inn i alle fartsverdiene (`MaxWalkSpeed` og `MaxWalkSpeedCrouched`).
- Rykk og effekter som utløses per treff trenger samme «én gang per øyeblikk»-vakt som lydene (hagle = ni treff).
