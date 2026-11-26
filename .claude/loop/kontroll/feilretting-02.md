**GODKJENT** (med fire små merknader; ingen av dem stopper commiten)

Feilrettingsrunde 2, lag Kamp (commit `2f04300`, «Combat fixes»). Kontrollert mot commiten, ikke arbeidsmappen.
Alt er vurdert ved å lese koden og motorens kildekode i `UE_5.8/Engine`. Ingenting er sett i spillet, og
kontrolløren har ikke bygget (samtidig drift; byggeren melder «Bygget: OK»).

## Bygg og omfang
- Lest for kompilerbarhet: ingen feil funnet. `NextWeaponAction` er fjernet og ingen andre filer i commiten
  bruker den (`git grep`). `Die` har fått et fjerde argument med standardverdi, så den andre kalleren
  (`Die(nullptr, false, Cause)`, linje 3093) virker som før. `TArray<AGSProjectile*>::Sort` med referanser i
  lambdaen er riktig for pekertabeller.
- Bare Kamps egne filer er rørt, pluss ett felt lagt til nederst i `FGSDeathInfo` i `GSTypes.h` (lov) og
  `IMPROVEMENTS.md` 376–385 i egen seksjon (ingen kolliderende numre).
- Ren rettingsrunde: regelen «én gammel + én ny» gjelder ikke.

## De seks rettelsene
| Nr. | Vurdering |
|---|---|
| 1. Kasteretning | Virker. `bInitialVelocityInLocalSpace = false` i konstruktøren. `InitializeComponent` dreier da ikke farten, men kaller fortsatt `UpdateComponentVelocity()`, så første nettverksoppdatering har riktig fart. |
| 2. Liggende mine hos sen klient | Virker. `OnRep_Flight` regner prosjektilet som liggende når replikert fart er under 1 cm/s. Sjekket: et prosjektil i flukt har alltid fart i første oppdatering (se nr. 1), så et nykastet prosjektil mister ikke tyngdekraften. Verten berøres ikke (`!HasAuthority()`). Gjelder vert, klient og sen klient. |
| 3. Dødskort/dødskamera/treffmerke/retningspil | Virker. Alle tilfeller gjennomgått: kaster lever i samme kropp (som før, med liv og rustning), kaster død uten ny kropp (avstand fra kastestedet + «han er selv død»), kaster i ny kropp (samme, ingen liv/rustning, kamera mot kastestedet), egen mine i ny kropp (ingen pil, «DU DØDE»), kaster har forlatt spillet (faller tilbake til instigator). Treffmerke går til ny kropp, Blodtørst bare til kroppen som kastet. `ClientDeathCam` tåler `KillerPawn == nullptr` og bruker `KillerLocation`. Høyden på dødskortet stemmer i alle tre varianter. |
| 4. Egen mine + tak | Virker. Sammenligner kontroller, ikke kropp. Regnet ut taket: 4 ute → ingen fjernes; 5 ute → 1 fjernes → 5 med den nye; aldri mer enn 5. Verste tilfelle 20 spillere × 5 miner × 20 spillere = 2000 avstandssjekker per bilde på serveren, ubetydelig. |
| 5. Musehjulet | Virker for vanlig mus. Se egen gjennomgang under og merknad 1 og 2. |
| 6. Kast og slag fra klienter | Virker. Se merknad 3 om hvordan det faktisk virker (kommentaren i koden er upresis). |

## Musehjulet, sjekket ekstra nøye
- **Akseverdi:** `SceneViewport.cpp` linje 968 sender `MouseWheelAxis` med hjulets utslag én gang per hjulhendelse.
  Tasten er merket `UpdateAxisWithoutSamples`, og akkumulatoren nullstilles hvert bilde (`PlayerInput.cpp` 1394),
  så verdien er null igjen bildet etter. `Triggered` uten egne utløsere fyrer bare når verdien ikke er null.
  Altså: **ett hakk = ett kall**, ikke gjentatte kall. Dette er standardmåten å lese hjulet på.
- **Fortegn:** hjul opp gir positiv verdi → `Step = -1` → forrige våpen / oppover i sekkelisten. Hjul ned → neste.
  Stemmer med listen i inventaret (tegnes ovenfra og ned) og med teksten i overskriften.
- **Rundgang:** `((x + Step) % N + N) % N` er riktig for begge retninger, også fra plass 0 og oppover.
- **Inventar åpent:** blar i `Backpack` (lokal `SelectedItem`, samme som `NextItem` gjør), bytter ikke våpen.
  Tom sekk gir 0. `ToggleInventory` endrer ikke inputmodus, så hjulet kommer frem med inventaret åpent.
- **Lukket:** bytter våpen begge veier via `SelectSlot` (lokalt + `ServerSelectSlot`). Ett våpen: ingenting skjer.
- **Meny:** `bMenuOpen` → ingenting. **Død:** `bDead` → ingenting (kroppen har uansett mistet kontrolleren).
- **Terminal:** sender «Q» (neste oppskrift / neste kamera) uansett retning. Se merknad 2.
- Ingen andre bindinger bruker hjulet (`git grep` på MouseWheel/MouseScroll), så ingenting «spiser» det.
- Byggerens forklaring på den gamle feilen («Started kom nesten aldri») er usikker: UE 5.8 har egen kode for
  taster som trykkes og slippes i samme bilde (`EnhancedPlayerInput.cpp` 607–612). Det Andreas meldte var at man
  ikke kunne bla i inventaret, og det fantes ganske enkelt ikke før. Nå finnes det. Bør prøves i spillet.

## Merknader (små, tas i en senere plan)

### 1. LAV/MIDDELS: styreflate og «frihjul» blar flere trinn per bevegelse
- Fil: `GSCharacter.cpp`, `Scroll` (linje 547–565).
- Styreflaten på en laptop (Andreas tester på en) og mus med jevn rulling sender mange små hjulutslag
  (brøkdeler av et hakk) over flere bilder. Koden bruker bare fortegnet, så hvert bilde med utslag blir ett
  trinn: én tofingersbevegelse kan bla gjennom alle våpnene. Med vanlig mus med hakk er det riktig.
- Bør gjøres: samle verdien i en variabel og ta ett trinn først når summen passerer ±1 (nullstill summen når
  retningen snur eller etter ca. 0,3 s uten utslag).

### 2. LAV: hjul opp i terminalen går samme vei som hjul ned
- `Scroll` sender «Q» for begge retninger. På 3D-printeren betyr det at hjul opp blar nedover i oppskriftene.
- Bør gjøres: bruk `Step`. Printer: opp = «1» (forrige), ned = «Q». PC i kameramodus: opp = «F», ned = «Q».

### 3. LAV: sikteretningen på serveren kommer fra kameraoppdateringen, ikke fra «hvert bevegelsessteg»
- `GetBaseAimRotation()` henter retningen fra kameralageret til spillerens kontroller. For klienter fylles det av
  `ServerUpdateCamera`, som klienten sender (upålitelig, opptil ca. 60 ganger i sekundet) når retningen endrer seg
  (`PlayerCameraManager.cpp` 812–866, `PlayerController.cpp` 1836). Retningen har både riktig helning og side.
- Følge: retningen kan være ett nettverkssteg gammel (kast rett etter et raskt rykk med musa kan gå noen grader
  feil), og eventuell kamerarystelse følger med. Godt nok nå. Skal kast bli helt presise, send sikteretningen med
  i selve kast-/slag-RPC-en, slik `ServerFire` allerede gjør for skudd.
- Kommentaren i `DoMeleeHit` («the controller's, sent with every move») bør rettes så neste leser ikke blir lurt.

### 4. LAV: blør man i hjel etter en mine, brukes fortsatt kasterens nye kropp
- `Die(LastDamageInstigator, false, LastDamageWeapon)`-veien (linje 3093 og andre dødsårsaker som krediterer
  siste angriper) sender ikke med prosjektilet, så dødskortet viser da ny kropps avstand og liv som før.
  Sjeldent. Kan løses ved å huske `LaunchLocation` sammen med `LastDamageInstigator`.

## Forslag til neste runde (viktigst først)
1. Prøv i spillet med to spillere: hjulet (inventar åpent/lukket, begge veier, på styreflate), granatkast opp og
   ned fra klienten, og mine på vegg sett av en spiller som kommer inn sent. Alt her er bare lest, ikke sett.
2. Merknad 1 og 2 i samme lille rettelse (samle hjulutslag, og riktig retning i terminalen).
3. Bestillingene til Spillbarhet fra feilretting 1 står fortsatt: ammovekter, kistevekter, og sletting av
   prosjektiler i `StartMatch` (miner overlever kampslutt; med taket på 5 er det nå høyst 100).

## Lærdom
- Inndata fra hjul og styreflate er en akse med brøkverdier: samle opp til et helt trinn i stedet for å bruke
  fortegnet per bilde.
- Når byggeren forklarer hvorfor motoren oppførte seg feil, skal forklaringen sjekkes mot motorkoden før den
  skrives inn som kommentar og i IMPROVEMENTS; en feil forklaring leder neste retting på villspor.
- Server-kode som trenger hvor en klient sikter skal bruke `GetBaseAimRotation()` eller en retning sendt i RPC-en,
  aldri kamerakomponenten (den oppdateres bare for den lokale spilleren).
