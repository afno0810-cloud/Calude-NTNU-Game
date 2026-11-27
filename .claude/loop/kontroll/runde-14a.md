**GODKJENT** – med fire små rettinger som tas i 14b (én av dem, IMPROVEMENTS 575, rettes før commit).

Kontrollert: `c616f84` (Combat round 14a), mot `ko/plan-08.md`, `kontroll/forhand-plan-08.md` og motorkoden i
`UE_5.8/Engine/Source/Runtime`. Ikke bygget selv (byggeren: Succeeded). Byggerens hodeløse kjøringer ligger i
scratchpad (`smoke2.log`, `smoke3.log`): `GSBodyAnim` lastet alle sett, ingen tilbakefall-melding, og hode-mot-hodeboks
ble logget for den lokale figuren.

## Andreas' regel
- **Forbedret:** beina når du ser ned → kropp uten hode/armer (566); andre spilleres hengende armer og ubevæpnede gange
  → hender på våpenet og gevær-/pistolgange (569, 570); huk som sto rett opp → senket hofte og bøyde knær (573);
  våpenet på andre følger skulderen (576); reservemenyen, inventar/klar under menyen, sår langt unna (579–581).
- **Nytt:** `UGSBodyAnim` i ren C++ (8 retninger, rotlås, fall), ryggbøy, skytterstilling, hel skygge (WSR).
- Begge deler finnes. ✔

## Forhåndssjekken, punkt for punkt (byggerlisten 1–14)
1. Stier med undermapper – ✔ (`GSBodyAnimImpl::Sets`, `MF_Pistol_Idle_ADS` fra `Pistol/`).
2. Rotlås – ✔ `RootMotionMode = IgnoreRootMotion` i konstruktøren, `FAnimExtractContext(t, true, {}, true)` i `Sample`.
3. Søppeltømming – ✔ `UPROPERTY(Transient) TArray<TObjectPtr<UAnimSequence>> Seqs` på instansen (som komponentens
   `AnimScriptInstance` holder); proxyen får rå pekere i `PreUpdate` (spilltråd). Lasting bare i
   `NativeInitializeAnimation` med `check(IsInGameThread())`.
4. Mønster fra `FAnimSingleNodeInstanceProxy`, `#include "AnimationRuntime.h"` – ✔.
5. Lokalt rom (blanding, hofte, rygg) → komponentrom (IK via `SafeSetCSBoneTransforms`/`LocalBlendCSBoneTransforms`)
   → `ConvertComponentPosesToLocalPoses`; NaN-vakt – ✔. Blandevektene sjekket mot motoren:
   `BlendTwoPosesTogetherInPlace(A, B, w)` gir A·w + B·(1−w), og alle seks kall bruker det riktig.
6. Ryggbøy – ✔ 10° (avvik, se under), 0 for `FPBody`. Treffboks: bare målt stående, sikte 0 (se funn 4).
7. Knebøy-IK alltid ved huk/glid – ✔. Gulvsporing: utsatt til 14b (riktig etter delingen).
8. `AlwaysTickPoseAndRefreshBones` + `AddTickPrerequisiteActor(this)` – ✔. Sjekket i `ExcludeHiddenBones`
   (SkeletalMeshComponent.cpp ~2300): med `AlwaysTickPoseAndRefreshBones` blir skjulte bein (`neck_01`, `clavicle_*`)
   **ikke** fjernet fra de nødvendige beina, så `FPBody` finner `hand_*`/`upperarm_*` og faller ikke tilbake. Bekreftet
   av hodeløs kjøring (ingen «mangler»-linje). Viktig at dette aldri endres til `AlwaysTickPose` på `FPBody`.
9. `FPBody` `None`, eierens `GetMesh()` `WorldSpaceRepresentation`, `OnRep_Dead` → `None` – ✔.
10. Hånd-IK på egen kropp – 14b. ✔ (ikke i denne runden)
11. `SetupLocalBody()` fra `SetupHumanBody` og `PawnClientRestart` – ✔, tåler flere kall (`AddPrerequisite` er unik).
12. Ingen hodebevegelse i kameraet – ✔.
13. Tilbakefall ved oppstart (`HasSequences`, `gs.TrueFP 0`) og underveis (`bMissing` → `PostEvaluate` på spilltråden →
    `Tick` → `FallBackToOldBody`) – ✔.
14. PIE med 2 spillere – **ikke gjort** (bare hodeløs `-nullrhi`). Se «Andreas tester».

Steg 0: `MenuClick` Host/Join (lukk, `OpenMain`, gjenåpne Kamp-menyen hvis ikke modal) – riktig: `SyncModal` setter
musepekeren og `LastDrawFrame` er fersk når menyen allerede tegnes, så `IsModal` blir sann samme bilde når kroken
lever. `ToggleInventory`/`ToggleReady` under meny – ✔. `AddBodyWound` 80 m – ✔.

## Byggerens sju avvik
1. **Ryggbøy ±10° (plan ±15°)** – godtatt. Regnet: 2° på hvert av `spine_01..05` flytter hodebeinet ca. 6,6 cm ved fullt
   utslag (spak 60+50+38+25+15 cm). 15° hadde gitt ~10 cm, utenfor ±9,5 cm-boksen.
2. **Huk 45 cm + 12° (plan 38 cm)** – godtatt for hodet: hodebeinet havner ca. 113 cm over gulvet, hodemidten ca. 123
   mot boksens 120 ±12. Men se funn 3 (knærne).
3. **Sprintfaktor fjernet** – godtatt. Målte farter (logg): gevær gange 304 cm/s, jogg 623 cm/s. Spillets gange 420
   og sprint 680 gir jogg-takt 0,67–1,09, godt innenfor klemmen 0,6–1,5, så en ekstra ×1,15 ville gitt glidende føtter.
   (Tallene er rundt dobbelt av planens gjetning 165/380; om de stemmer ser man bare i PIE – føttene skal ikke gli.)
4. **`CamForwardOffset` = hode.X − kamera.X + 4** – godtatt. Det legger hodebeinet 4 cm *bak* kameraet, som var
   meningen («like bak kameraet»); planens «minus 4» var feil skrevet. `GetMesh()` (det andre ser og treffboksene) flyttes
   ikke, bare `FPBody`.
5. **Håndrotasjon** (målets rotasjon som tillegg til klippets grep, regnet om fra verden til komponentrom med
   `CompQ⁻¹·Δ·CompQ`) – godtatt, matematisk riktig, og fingrene beholder klippets grep.
6. **Venstre hånd / hals** (pistol: venstre hånd under, nakken vris −15° tilbake i skytterstilling) – godtatt.
7. **Tick-forutsetning for alle** – godtatt. Ingen sykel: aktøren venter ikke på meshen. Kostnad null.

## Spesielle sjekker
- **Trådsikkerhet:** ✔. `Update`/`Evaluate` (arbeidstråd) bruker bare kopiert `In`, egne felt og rå `UAnimSequence*`
  (`GetPlayLength`, `GetAnimationPose` – samme som motorens egne noder). Ingen `UObject`-skriving fra arbeidstråd:
  `bMissing` overføres i `PostEvaluate` (spilltråd). `CastChecked` bare i `PreUpdate`/`PostEvaluate`.
- **GC:** ✔ (se punkt 3). Ved tilbakefall byttes instansen med `SetAnimInstanceClass`; proxyen slettes med den.
- **Tilbakefall:** ✔ ved oppstart og underveis. Varig (bytter aldri tilbake) – greit.
- **Treffbokser:** stående sikte 0 målt: hodebein mot boksmidte fram −3, side 4,4, opp −10,3 cm → innenfor. Ikke målt
  ved ±80° eller hukende (funn 4). Knær hukende stikker ut av beinboksen (funn 3).
- **Ytelse, 20 kropper:** ✔. Høyst 4 uttrekk + 3 blandinger + ~13 bein-rotasjoner + 4 to-bens-IK per kropp, på
  arbeidstråder; andres `GetMesh()` har `OnlyTickPoseWhenRendered` + URO. Eieren: 2 evalueringer. Små ting på
  spilltråden (bein-oppslag på navn og to `FString`-kopier av CVar per figur per bilde) er ubetydelige, men kan caches.
- **Nettverk:** ✔. Ingen ny replikering; alt fra `bIsCrouched`, `bSliding`, `bSprinting`, fart, `IsFalling`,
  `GetBaseAimRotation`. Kameraets plass uendret, så serverens skuddsjekk står. Dedikert server: `Tick`-blokken hoppes
  over; meshen der evaluerer aldri (`OnlyTickPoseWhenRendered`).
- **Dødskamera/ragdoll/sår:** ✔ type → `None`, `FPBody` skjules, ragdoll starter fra siste animerte stilling. Liten ting:
  `Evaluate` returnerer `true` uten å skrive, og motoren har allerede satt referansestilling
  (`ParallelEvaluateAnimation` → `ResetToRefPose`). Bein uten fysikklegeme (fingre, vridebein) går derfor til
  A-stilling på liket – rette fingre. Kosmetisk (forslag 3).
- **Bots:** se funn 2.

## Funn
1. **Egen skygge holder ikke våpenet** (`GSCharacter.cpp`, `Tick`, gren `(!IsLocallyControlled() || bTrueFP)`).
   `TPWeaponMesh` plasseres nå også for eieren, men den er `SetOwnerNoSee(true)` uten `bCastHiddenShadow`, og
   `FPrimitiveSceneProxy::IsShadowCast` (PrimitiveSceneProxy.cpp ~1660) dropper skyggen for eierens visning. Skyggen
   viser altså hender rundt ingenting. Planens «Ferdig når: egen skygge … holder våpenet» og IMPROVEMENTS **575**
   («med våpenet i hendene») stemmer ikke. **Rett:** i `SetupLocalBody` når `bTrueFP`: `TPWeaponMesh` og dens barn
   (fester, sikter) `SetFirstPersonPrimitiveType(WorldSpaceRepresentation)` (samme skyggevei som kroppen), eller
   `SetCastHiddenShadow(true)`; tilbake i `OnRep_Dead`/`FallBackToOldBody`. Samme for armbind/vest/hjelm/sekk hvis de
   skal med i skyggen. **Før commit:** skriv 575 om til «Du har en hel skygge av deg selv på gulvet» (uten våpenet), eller
   rett koden først. (IMPROVEMENTS.md er ikke i commiten ennå – den ligger endret i arbeidsmappen.)
2. **Bots vil få førstepersonskropp hos verten** (`SetupLocalBody`: `!IsLocallyControlled()`). En AI-styrt
   `AGSCharacter` er `IsLocallyControlled() == true` på serveren/frittstående. Kommer boten inn med kontroller før
   `BeginPlay` (f.eks. `AutoPossessAI`), eller kalles `SetupLocalBody` på nytt, får botens `GetMesh()`
   `WorldSpaceRepresentation` (motoren skyver den da over i førstepersons-skyggeveien i VSM og egne masker i
   Lumen/strålesporing – feil skygge for alle botene, og WSR-grensene i `FirstPersonSceneExtension` blir slått sammen
   over hele kartet), `AlwaysTickPoseAndRefreshBones` uten URO, og en ekstra synlig `FPBody`-evaluering per bot. Med
   19 bots er det 38 fulle evalueringer hvert bilde hos verten. Dette dekkes av åpen bestilling **K1** (Spillbarhet,
   plan-06: `IsLocalHuman()`), som nevner `SetupLocalBody` spesielt. **Rett i 14b eller før botene bygges:**
   `if (!IsLocallyControlled() || !IsPlayerControlled()) return;` i `SetupLocalBody`. Det er ikke feil i dag (ingen
   bots i commiten), derfor ikke MÅ FIKSES nå.
3. **Hukende knær stikker 10–15 cm ut av beinboksen** (`GSBodyAnim.cpp`, `Evaluate`, `CrouchDrop = 45`). Hofta går
   rett ned mens føttene står, så med lår ~43 og legg ~42 cm og 35 cm hofte–ankel havner kneet ca. 39 cm foran hofta og
   26 cm over gulvet. Hukende beinboks går bare til X = +25. Den som skyter et synlig kne bommer. **Forslag 14b:** flytt
   hofta 10–15 cm bakover (langs −`CompFwd`) mens den senkes, som en ekte huk, eller øk beinboksens `Crouch.X`/`Ext.X`
   (8/17 → 14/22). Sjekk hodet etterpå (tyngdepunktet bakover flytter også hodet).
4. **Treffboks ved ±80° og hukende er ikke målt.** Regnet: stående sikte +80 (ryggen bøyes bakover) gir hodebeinet ca.
   −3 − 6,6 = −9,6 cm fra boksmidten, akkurat utenfor ±9,5. Bakhodet stikker da ut. **Forslag:** flytt
   `GSZoneBoxes[0].Stand.X` fra 3 til 0 (hodebeinet står i snitt på 0, se logg), og mål med `gs.FP.Debug 1` i PIE
   (meldingen logges også hvert 2. s, så det kan leses fra en hodeløs kjøring med en bot/andre figur).
5. **«Avslutt» i reservemenyen gjør ingenting i editorens Play** (`GSPlayerController.cpp`, `MenuClick`, `Quit`).
   `ConsoleCommand("quit")` håndteres bare av `UGameEngine` (GameEngine.cpp 1530), ikke i PIE – og PIE er nettopp der
   kroken kan mangle. Bruk `UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false)` som `GSMenu` gjør
   (avslutter Play i editoren og spillet ellers).

## Andreas tester (PIE, 2 spillere, lytteserver)
Se ned stående/gående/hukende/glidende (bryst, mage, bein; ingen nakke), se den andre gå, huke og sikte opp/ned, sjekk at
føttene ikke glir, egen skygge, dø (kroppen synes i dødskameraet), og `gs.TrueFP 0` + ny runde (gammel kropp).
`gs.ShowHitZones 1` + `gs.FP.Debug 1` viser hode mot hodeboks og rekkevidde for hendene.

## Forslag til neste runde (14b)
1. Funn 1 og 2 (skygge med våpen; `IsPlayerControlled()`-vakt) – små, og 2 må være inne før botene.
2. Funn 3 og 4: hofta bakover i huk, hodeboks X = 0, og mål ±80°/huk med debugmeldingen; skriv tallene.
3. Behold siste stilling ved død (kopier forrige `Output` i proxyen og returner den når `bDead`), så fingrene på liket
   ikke retter seg ut; cache `FName`-ene og CVar-tekstene i `FillBodyAnimInput`/`Tick`.

## Lærdom
- Komponenter med `OwnerNoSee` kaster ikke skygge for eieren uten `bCastHiddenShadow` eller
  `WorldSpaceRepresentation`; en «hel skygge» må gjelde alt som henger på kroppen (våpen, utstyr). (runde 14a)
- Tall en plan krever i rapporten (treffboks, rekkevidde) skal også skrives med `UE_LOG`, så de kan leses fra en hodeløs
  kjøring når PIE ikke er mulig. (runde 14a)
- Alt nytt som er «bare for spilleren bak skjermen» skal kreve `IsPlayerControlled()` i tillegg til
  `IsLocallyControlled()` (AI på serveren er også lokal). (runde 14a)
