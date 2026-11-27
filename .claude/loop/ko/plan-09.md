# Plan 09 – runde 15 (lag Kamp) – SKISSE: animasjoner på kroppen

**Status: skisse.** Skrives ferdig når runde 14a og 14b (`plan-08.md`, `plan-08b.md`: `UGSBodyAnim`, kroppen, hånd- og
fot-IK) er bygget og kontrollert. **Hovedagenten har bestemt (27. nov.): GASP/Lyra hentes ikke foreløpig. Planen må
virke med det som ligger i prosjektet + kode.** Hodebevegelse i kameraet (fra `FPBody`s hode, forhåndssjekk F11) tas her.

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

## Ressurser – det som finnes, og hva som lages i kode
I prosjektet (`Content/Characters/Mannequins/Anims`, nøyaktige stier i `plan-08.md`): gange/jogg 8 retn. (ubevæpnet,
gevær, pistol), hopp (gevær: start, start-løkke, topp, fall, landing, landing-additiv; pistol: start, fall, landing-
additiv; ubevæpnet: `MM_Jump`, `MM_Fall_Loop`, `MM_Land`), `Fire`, `Reload`, `Equip`, `DryFire` for gevær og pistol,
8 treffrykk (`Rifle/HitReact`), 6 dødsfall, 3 slag (`Unarmed/Attack`), `MM_Dash`, `MM_WallJump`.
**Lages i kode (ingen filer finnes):** glid (hofte ned og bakover, ett bein fram via fot-IK, rygg bakover), klatring
(hånd-IK til kantpunktet fra sporingen i `TryMantle`, to faser opp/over, knær opp), kolbeslag (overkroppsrotasjon +
hånd-IK mot våpenet), knivstikk, hagle skudd-for-skudd og bolt (hånd-IK følger de eksisterende `FPArmL`-nøkkelbanene),
huk-gange (gange-sekvensene + hofte ned + knebøy fra 14a, saktere avspilling), sprint (jogg × fart + rygg fram + armer
inn). Kvaliteten blir god nok for glid, slag og lading; klatring blir enklere enn med ekte animasjoner.
**Hvis Andreas senere vil ha mer:** Game Animation Sample (traversal/vault/mantle, huk-gange) og Lyra (hagle-lading,
huk, slag) er gratis og bruker samme UE5-skjelett; da migrerer han animasjonsmappene inn i `Content` og en senere
runde bytter kode-stillingene ut. Ikke en del av denne planen.

## Filer (forventet)
`GSBodyAnim.*`, `GSCharacter.*` (`StartSlide`, `TryMantle`, `DoMeleeHit`, `MulticastMeleeFX`, `FireOnce`/`MulticastShotFX`,
`Reload`, `UpdateLocalVisuals`), `GSAudio` (glid- og landingslyd etter underlag). Ikke kart eller katalog.

## Mulig deling
15a: hopp/landing, glid, slag, treffrykk, hodebevegelse i kameraet (sekvenser som finnes + kode). 15b: klatring med
hender på kanten, lading/skudd på kroppen andre ser, våpenbytte som tar tid.

## Risiko
Slag og klatring endrer spillet (tid, sårbarhet) – samordne med Spillbarhet. Klatrebanen må valideres på serveren
(samme grense som `ServerMantle` i dag). Animasjon må aldri flytte kameraets komponentplass (skuddstart).
