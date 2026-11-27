# Plan 13 – lag Kamp: verden som tar imot kuler, granater og slag (Interaktivs K1–K5) + rettinger 10–13

**Rekkefølge:** bygges etter lydrunde 1 (`plan-11.md`) og før plan-09 (animasjoner). Forutsetter Interaktivs runde 1b
(glass, kulehull). Sjekk først i `GSMapState.cpp` at `BulletSegment`/`AddBulletMark`/`ApplyMelee` ikke lenger er tomme
(i 1a returnerer de 1 / ingenting / `BIG_NUMBER`). Er 1b ikke ferdig: kallene legges inn likevel (de er trygge), men
steg 6 (fjerne den mørke flisa) **utsettes**, og det står i rapporten.

## Mål
Kuler knuser vinduer og glassvegger og blir bremset av dem, kulehull blir værende, granater blåser opp dører og knuser
glass, kolben knuser ruta i stedet for å slå gjennom den, og dører/glass høres ut som dører og glass.

## Forbedrer (gammelt)
- Kuletreff (`UGSBallistics::Step`): kula mister fart og skade gjennom glass; skudd i dører/kodelåser virker.
- Nærkamp (`DoMeleeHit`): kniv/kolbe treffer ikke lenger folk bak en hel glassvegg, og knuser ruta.
- Eksplosjoner (`AGSProjectile::Explode`): påvirker kartet, ikke bare figurer.
- Bruk-tasten (`Interact`/`UpdateFocusPickup`): E gjør aldri to ting; det nærmeste/midt i blikket vinner.
- Rettinger 10–13 fra kontroll av 14b (synlighet uten vipping, én regel for oppløsning, rekkevidde ved bratt sikte,
  beinboks som dekker knærne).

## Nytt
- Glass som knuses av kuler (alle maskiner ser samme bane: ekte og viste kuler bremses likt).
- Varige kulehull etter underlag (Interaktivs pool, tak 400) i stedet for den mørke flisa som forsvant etter 40 s.
- Granater, miner og C4 sprenger dører opp og knuser glass.
- Nye lyder: `Door_Open`, `Door_Close` (resten av K5 er laget i lydrunde 1).

## Hvorfor nå
Hovedagentens prioritering: K1 og K4 tidlig, før lyd og animasjoner. Interaktivs dører og glass finnes, men verden
reagerer ikke på våpnene før Kamp kaller funksjonene. Rettingene skal tas først i neste Kamp-runde.

## Filer
`Source/Gloshaugen/Private/GSCharacter.cpp`, `Private/GSPlayerController.cpp`, `Private/GSBallistics.cpp`,
`Private/GSProjectile.cpp`, `Private/GSEffects.cpp`, ev. `Private/GSAudio.cpp`, `Tools/Audio/sources.json`,
`SourceArt/Sounds/S_GS_Door_Open_*.wav`, `S_GS_Door_Close_*.wav`, `SourceArt/Sounds/CREDITS.md`, `IMPROVEMENTS.md`.
Ingen endring i Interaktivs filer (`GSMapState.*`, `GSInteract.*`, `GSDoor*`).

## Steg

### Del A – rettinger 10–13 (egen commit først)
1. **Retting 10 – armer vippes hvert bilde.** I `AGSCharacter::Tick` (l. ~4614–4620) fjernes blokken
   `if (bBodyHands) { FPArmR->SetVisibility(false); FPArmL->SetVisibility(false); FPItemHand->SetVisibility(false); }`.
   I stedet: `UpdateArms` (l. 1727) og `UpdateItemUse` (l. 1810) bruker `const bool bBodyHands = UsesBodyHands();` og
   setter `FPArmR->SetVisibility(bShow && !bBodyHands)` (l. 1734), `FPArmL->SetVisibility(bLeft && !bBodyHands)`
   (l. 1780) og `FPItemHand->SetVisibility(!bBodyHands && FPItemHand->GetStaticMesh() != nullptr)` (l. 1831). Punktenes
   plassering beregnes som før (magasinet henger på `FPArmL`; `FPMag` sin egen synlighet endres ikke). Bruk
   `SetVisibility` bare når verdien faktisk endres (`if (FPArmR->IsVisible() != V)`), så ingenting skrives hvert bilde.
   Sjekk med `gs.FP.Hands 0` at de gamle armene vises, og med 1 at de er skjult også under spising/drikking.
2. **Retting 11 – én regel for oppløsning.** `AGSPlayerController::ApplyGraphicsLevel`: les
   `const int32 Res = GetResolutionPercent();` før `SetOverallScalabilityLevel`, og sett
   `S->SetResolutionScaleValueEx(float(FMath::Clamp(Res, 50, 100)));` etterpå (som `Set.Graphics` i `GSMenu.cpp`).
   Rett kommentaren («keeps the player's own resolution, like the main menu»). Skriv IMPROVEMENTS **597** om til:
   «Skarpere bilde: første oppstart og gamle 72 % blir full oppløsning på Høy/Episk (85 % på Lav/Middels)».
3. **Retting 12 – rekkevidde ved bratt sikte.** `TickReachTest`: utvid stillingene fra 3 til 5 per våpenklasse:
   hofte, sikte, lading, **sikte +70° opp**, **sikte −70° ned** (sett `Controller->SetControlRotation` med pitch ±70 i
   stillingen, og tilbake til 0 etter). `Poses`-tabellen og `% 3`/`/ 3` blir `% NumPoses`/`/ NumPoses`
   (`constexpr int32 NumPoses = UE_ARRAY_COUNT(Poses)`). Logg som i dag (`UE_LOG`, «OVER 100»). Er en verdi over 100 %:
   flytt førstepersonsvåpenet inn mot kroppen med pitch (f.eks. `Hip.X -= 6.f * FMath::Clamp((FMath::Abs(Pitch) - 45.f) / 35.f, 0.f, 1.f)`
   der `Hip` settes, l. ~1421) til målingen er ≤ 100 %. Skriv tallene før/etter i rapporten.
4. **Retting 13 – stående beinboks.** `GSZoneBoxes` (l. ~108–109): beinboksenes `StandExt.X` fra `9.f` til `13.f`
   (dekker knærne 8 cm foran/6 cm bak midt i et steg; bredden Y og høyden uendret). Sjekk med `gs.ShowHitZones 1` at
   boksene ikke stikker ut foran tærne i stillstand, og at beina til to stående figurer side om side ikke overlapper
   magen (magen ligger over, Z 15,5 ± 17,5 – ingen overlapp i X/Z-planet med beinboksene som går til Z −2).

### Del B – Interaktivs bestilling K1–K5
Alle kall er statiske i `#include "GSMapState.h"` / `"GSInteract.h"`, tåler at kartet ikke finnes, og gjør ingenting
med figurer.

5. **K1 – kuler (`UGSBallistics::Step`, `GSBallistics.cpp` l. 308).**
   - Gjør `NewVel` ikke-const. Rett etter linjesporet (etter `End` er satt):
     ```cpp
     const float GlassF = GSMapState::BulletSegment(World, B.Pos, End, B.Vel, B.Damage, B.bReal);
     if (GlassF < 1.f) { NewVel *= GlassF; B.Damage *= GlassF; }
     ```
     `End` er allerede treffpunktet når sporet traff noe (ellers telles glass bak en vegg med). Glasset ignorerer
     `Visibility`, så dagens spor går gjennom det; dette er det eneste som finner det. Ekte og viste kuler kaller det
     likt, så banene blir de samme på alle maskiner. Skaden faller med samme faktor (8 % per rute); regn i rapporten
     om antall treff for å drepe endres for 9 mm og 5,56 i brystet gjennom én rute (lærdom fra plan 5) – en endring
     fra 2 til 3 treff er greit (det er realistisk), mer enn det ikke.
   - I `if (B.bReal)`-grenen, når `!Victim`: `GSMapState::ApplyBulletHit(World, Hit, B.Damage * GSBallistics::DamageScale(B.Ball, DistM));`
     (regn `DistM` som i figurgrenen; flytt utregningen opp så begge bruker den).
   - I `else if (!Victim)` (vist kule): `GSMapState::AddBulletMark(World, Hit);` **før** `SpawnImpact`.
6. **K1 – flisa i `AGSShortFX::SpawnImpact` (`GSEffects.cpp` l. 170)** fjernes (spawnen av den mørke kuben med
   `SetLifeSpan(40.f)`), bare når 1b har gjort `AddBulletMark` ekte (se toppen). Lyd, gnister og støv blir stående, og
   materialparameteren fra lydrunde 1 beholdes. `SpawnImpact` brukes også av nærkamp (l. 1180): der skal det heller
   ikke bli hull – det er riktig.
7. **K2 – eksplosjon (`AGSProjectile::Explode`, `GSProjectile.cpp`, siste `else`-gren).** Før `DamageAround`:
   `if (Radius > 0.f) GSMapState::ApplyBlast(GetWorld(), Loc, Radius, Damage);` (`Explode` kjører bare på serveren).
   Flash, røyk og brann får ikke kallet. Granat, mine, C4 og rakett går alle hit – sjekk at alle tre typer åpner en
   lukket dør i testrommet (`Interaktiv.Test` eller hva 1a kaller konsollkommandoen – se `GSMapState.cpp`).
8. **K3 – nærkamp (`AGSCharacter::DoMeleeHit`, l. 1154).** Kjører på serveren (via `ServerMelee`). Etter at `Start`/`End`
   er satt og **før** sveipet brukes:
   `const float GlassDist = GSMapState::ApplyMelee(this, Start, End, Damage * GetMods().Damage);`
   Når sveipet treffer: `const bool bBehindGlass = FVector::Dist(Start, Hit.ImpactPoint) > GlassDist;`. Er
   `bBehindGlass`: ingen skade på figuren og ingen `SpawnImpact` (slaget stoppet i ruta; Interaktiv lager knus og lyd).
   Ellers som i dag. `MulticastMeleeFX()` kalles uansett.
9. **K4 – bruk-tasten (`GSCharacter.cpp`).** `#include "GSInteract.h"`.
   - `Interact()` (l. 2392): rett etter linja `if (PC && PC->IsOnTerminal()) { ... return; }` (terminalen skal fortsatt
     få E): `if (GSInteract::HasFocus(this)) return;`
   - `UpdateFocusPickup()` (l. 2353): bytt `if (FocusTerminal.IsValid()) return;` med å pakke pickup-løkka i
     `if (!FocusTerminal.IsValid()) { ... }`, og sist i funksjonen:
     `if (GSInteract::HasFocus(this)) { FocusPickup = nullptr; FocusTerminal = nullptr; }`
     (`HasFocus` setter `bCharacterHooked`, så Interaktivs aktør vet at vi viker). Funksjonen kjøres bare for
     `IsLocalHuman()` (l. 4587) – det er riktig.
   - Test i PIE (Selected Viewport): stå ved en dør med et våpen på bakken rett foran; E skal gjøre én ting (det som
     står i hintet), og hintet skal ikke vises to steder.
10. **K5 – lyder.** Lydrunde 1 har laget `Glass_Shatter/Crack/Collapse/Tinkle`, `Door_Kick/Slam/LockBreak`, `Lock_Pick`,
    `Keypad_Beep/Error/Break` (Kenney, med lag). Mangler: `Door_Open` og `Door_Close` (Interaktiv bruker begge).
    - Legg til `Door_Open_1..3` og `Door_Close_1..3` i `Tools/Audio/sources.json`, kategori `World`, fra filene som
      finnes i `C:\Users\afno0\Documents\Lydbibliotek` (list med `fetch_library.py --list`, ikke gjett stier):
      `Door_Open` = håndtak/smekklås (`Kenney_InterfaceSounds/.../switch_00N.ogg`, speed ca. 0,6) + lag med
      `impactWood_light_00N` (delay ca. 0,12 s, −8 dB) – en dør som går opp er stille; `peak_db` ca. −14.
      `Door_Close` = `impactWood_medium_00N` (speed ca. 0,8) + lag med `switch_00N` (delay ca. 0,04 s, speed 0,5,
      −4 dB) for låsfallen; `peak_db` ca. −8. Lytt og juster; ingen syntetiske lyder hvis Kenney holder.
    - Kjør `python -I Tools/Audio/prep_sounds.py` bare for de nye navnene (verktøyet sletter ikke andre filer);
      `CREDITS.md` oppdateres av verktøyet.
    - Sjekk at `GSAudio::Get(TEXT("Door_Open"))` (det `AGSMapState::PlayPartSound` kaller) finner variantene `_1.._n`
      etter lydrunde 1. Gjør den ikke det, rett det i `GSAudio.cpp` (Kamps fil): `Get(Name)` uten `_n` skal velge en
      tilfeldig variant når `S_GS_<Name>` ikke finnes. Ellers spiller Interaktiv reservelydene for evig.
    - Editoren må åpnes én gang for import (`gs_sounds.py`) – står i rapporten.

## Ferdig når
- `Build.bat` uten feil og advarsler i endrede filer.
- Rettinger: ingen `SetVisibility` hvert bilde på `FPArmR/FPArmL/FPItemHand` (logg et teller-`UE_LOG` i 3 s eller sjekk
  med breakpoint); «Grafikk 2» beholder spillerens `Opplosning`; reach-testen logger 5 stillinger per klasse og alle
  ≤ 100 %; beinboksen dekker knærne (`gs.ShowHitZones 1`).
- K1: skudd gjennom en glassrute i testrommet knuser den hos server og klient, kula går videre litt saktere og treffer
  bak; skudd i kodelåsen bryter den; kulehull blir stående (også etter 60 s). Logg `UE_LOG` med `GlassF` og skade før/
  etter for ett skudd.
- K2: granat ved en lukket dør blåser den opp og knuser glass i nærheten; flash/røyk gjør det ikke.
- K3: kniv mot en figur bak hel glassvegg gir ingen skade; ruta knuses; kniv mot figur uten glass virker som før.
- K4: E ved en dør med gjenstand foran gjør én ting; terminalen får fortsatt E.
- K5: `S_GS_Door_Open_1..3` og `S_GS_Door_Close_1..3` finnes, står i `CREDITS.md`, og en dør høres med dem (ikke med
  reservelyden `Reload_In`).
- Bestillingen i `.claude/lag/bestillinger.md` (linja «Interaktiv verden (planlegger) → Kamp») merkes LEVERT K1–K5
  (eller K1–K4 + hva som ble utsatt), og rettinger 10–13 merkes RETTET i `.claude/loop/rettinger.md`.

## Risiko
- **Online:** `BulletSegment` kalles for ekte kuler på serveren og viste kuler hos alle; bruker 1b ulik glasstilstand på
  maskinene (rute knust hos server før klienten vet det), kan en vist kule bremses der den ekte ikke gjør det. Liten
  forskjell (8 %); nevn det for kontrolløren, ikke rett i Interaktivs kode.
- **Ytelse:** ett ekstra kall per kulesteg (60 Hz per kule). Interaktivs funksjon gjør sitt eget spor; mål `stat game`
  med en full salve fra LMG mot en glassvegg og skriv tallet.
- **Kulehull på dører** som svinger: hullene er Interaktivs ansvar (de bør feste dem til dørbladet); meld i rapporten
  om de blir hengende i lufta.
- **Skade gjennom glass** endrer drap-terskler litt – se steg 5.
- **Retting 12** kan flytte våpenet synlig ved bratt sikte; hold endringen under 6 cm og bare over 45°.
- **Retting 13** gjør beina litt lettere å treffe – det er meningen (synlige knær skal telle).
- Steg 6 fjerner flisa: gjør det ikke før `AddBulletMark` faktisk lager hull, ellers blir det ingen hull i det hele tatt.

## IMPROVEMENTS-linjer
760. Førstepersonsarmene skjules ett sted i stedet for å vippes synlig/usynlig hvert bilde (mindre arbeid for grafikken)
761. Grafikknivået i Esc-menyen og «Grafikk N» beholder din egen oppløsning, som i hovedmenyen
762. Hendene holder grepet også når du sikter bratt opp eller ned (målt med `gs.FP.ReachTest` ved ±70°)
763. Skudd på knærne til en som går teller som beinskudd (beinboksen dekker hele steget)
764. Kuler knuser vinduer og glassvegger og blir bremset av dem (litt mindre fart og skade per rute)
765. Kulehull blir værende i vegger, gulv og tak (etter underlag) i stedet for å forsvinne etter 40 s
766. Du kan skyte opp kodelåser og låsen på låste tredører
767. Granater, miner og C4 blåser opp dører, bryter låser og knuser glass i nærheten
768. Kniv og kolbe knuser glassruter, slår løs kodelåser og sparker dører – og treffer ikke lenger noen bak en hel glassvegg
769. Bruk-tasten gjør aldri to ting: det som er nærmest midt i blikket (dør, gjenstand eller terminal) vinner
770. Nye lyder for dører som åpnes og lukkes (håndtak, smekklås), fra Kenney (CC0)
