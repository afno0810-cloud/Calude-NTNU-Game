# Plan 09 – runde 15 (lag Kamp) – SKISSE: animasjoner på kroppen

**Status: skisse.** Skrives ferdig av planleggeren når runde 14 (`plan-08`, ekte førsteperson og `UGSBodyAnim`) er
bygget og kontrollert, og etter at det er avklart om Andreas har lagt inn flere animasjoner (se «Ressurser»).

## Mål
Hopp, landing, glid, klatring, slag, skudd og lading synes på kroppen – både for deg når du ser ned og for andre –
med vekt og riktig lengde per våpentype.

## Forbedrer (gammelt)
- Hopp/landing: i dag bare kameradupp og våpenrykk (`UpdateWeaponAnimation`, `UpdateLocalVisuals`). Nå: avsats, svev,
  landing som tar i knærne (hardere fall = dypere), lyd etter høyde.
- Glid (`StartSlide`): i dag huk + lav friksjon + kamerarull. Nå: kroppen legger seg bakover på hofta med ett bein
  fram (stilling i kode: hofte ned, rygg bakover, IK på føttene), friksjonslyd etter underlag, støv, og at glidet
  bremser i motbakke og varer lenger i nedoverbakke (Spillbarhets funn: glidehopp – avklar).
- Klatring (`TryMantle`): i dag `LaunchCharacter` + våpen gjemt. Nå: hendene griper kanten (hånd-IK til kantpunktet
  fra sporingen), kroppen trekkes opp i to faser (opp, så over) med styrt bane i stedet for et kast; vinduer og høye
  murer (bestill klatrepunkter fra Grafikk i Helgasetr-kartet). Andreas liker klatring.
- Slag (`QuickMelee`/`DoMeleeHit`): i dag bare våpenet som svinger. Nå: kolbeslag (to hender, overkroppen roterer,
  steg fram) og knivstikk; andre ser det (`MM_Attack_01..03` for ubevæpnet, kode for kolbe).
- Skudd/rekyl: rekyl går gjennom skulder og overkropp (additiv rygg-/skulderrykk i `UGSBodyAnim` fra
  `AddWeaponKick`), andre ser rykket (`MM_Rifle_Fire`/`MM_Pistol_Fire` som additiv).
- Lading per våpentype: taktisk vs. tom (finnes i kode), nå også på kroppen andre ser (`MM_Rifle_Reload`,
  `MM_Pistol_Reload` som overkroppsdel), hagle skudd-for-skudd, bolt; magasinet faller fra kroppens hånd hos alle.
- Treffrykk: `HitReact`-sekvensene (8 finnes) som additiv på overkroppen i retning skuddet (Andreas del 5, punkt 1).
- Bytte våpen: `MM_Rifle_Equip`/`MM_Pistol_Equip`; våpenbytte tar tid (V11).

## Nytt
- Overkropp/underkropp-deling i `UGSBodyAnim` (bland fra `spine_01` og opp) og en enkel «montasje»-kø i C++
  (spill én sekvens på overkroppen med inn/ut-blanding), uten Blueprint.
- Fot-rotasjon etter gulvets helning; start/stopp-steg.
- Replikering bare der det trengs: et lite «handlingsnummer» (uint8 + tid) for slag/lading/klatring som andre spiller av.

## Ressurser – ærlig om hva som mangler
I prosjektet i dag (`Content/Characters/Mannequins/Anims`): gange/jogg 8 retn. (ubevæpnet, gevær, pistol), hopp
(start, topp, fall, landing), skudd, lading og bytte for gevær og pistol, 8 treffrykk, 6 dødsfall, 3 slag, dash.
**Mangler:** huk-gange, sprint, glid, klatring/vault, lene, ligge, hagle-/bolt-lading, kolbeslag, bruk av ting.
Det som mangler lages som stilling i kode (godt nok for glid, huk, kolbeslag; svakt for klatring og ligge).

**Anbefalt (gratis) – Andreas må gjøre dette selv i Epic Games Launcher/Fab, ca. 30–60 min:**
1. **Game Animation Sample Project (GASP)** fra Epic (gratis, Fab / «Samples» i launcheren): hundrevis av
   bevegelser på samme UE5-skjelett – huk-gange, start/stopp, sving, og klatring over hindre (vault, mantle, hurdle).
   Andreas: last ned og lag et eget GASP-prosjekt for 5.8, åpne det, høyreklikk animasjonsmappen(e) i Content
   Browser → *Asset Actions → Migrate* → velg `Calude-NTNU-Game/Content`. Si fra til hovedagenten når det er gjort;
   byggeren finner da stiene selv (søk i `Content/`). Uten dette: klatring blir kodet stilling.
2. **Lyra Starter Game** (gratis, Fab): skudd, lading og bytte for gevær/pistol/hagle, huk, død, slag – samme
   UE5-skjelett. Migrer på samme måte (bare animasjonsmappen).
   Viktig for begge: migrerte animasjoner kommer med sitt eget skjelett-objekt. Enten bruker vi deres kropp
   (`SKM_Manny`) også, eller Andreas høyreklikker deres `SK_Mannequin` → *Add Compatible Skeleton* → vårt. Planen
   velger når filene finnes.
3. (Senere, utseende) En ekte menneskemodell i stedet for grå mannekeng: MetaHuman (gratis, innebygd i 5.8) eller en
   gratis soldat/sivil-modell fra Fab med UE5-skjelett. Tungt på RTX 3050 – må måles. Egen avgjørelse.
4. Mixamo (gratis, Adobe-konto) bare som siste utvei: krever omdirigering av skjelett (IK Retargeter) i editoren.

C++ kan *bruke* animasjonsfiler, men ikke lage dem. Alt over er valgfritt: plan-09 bygges uansett, med kode der filer
mangler, og bytter til ekte animasjoner når de finnes (samme mønster som våpenmodellene fra Fab).

## Filer (forventet)
`GSBodyAnim.*`, `GSCharacter.*` (`StartSlide`, `TryMantle`, `DoMeleeHit`, `MulticastMeleeFX`, `FireOnce`/`MulticastShotFX`,
`Reload`, `UpdateLocalVisuals`), `GSAudio` (glid- og landingslyd etter underlag). Ikke kart eller katalog.

## Mulig deling
15a: hopp/landing, glid, slag, treffrykk (alt i kode + sekvenser som finnes). 15b: klatring med hender på kanten,
lading/skudd på kroppen, våpenbytte som tar tid (+ GASP/Lyra hvis de finnes).

## Risiko
Slag og klatring endrer spillet (tid, sårbarhet) – samordne med Spillbarhet. Klatrebanen må valideres på serveren
(samme grense som `ServerMantle` i dag). Animasjon må aldri flytte kameraets komponentplass (skuddstart).
