# Plan 08 – runde 14a (lag Kamp): ekte førsteperson, del 1 – kroppen og animasjonen

Revidert etter forhåndssjekken (`kontroll/forhand-plan-08.md`, BØR ENDRES, delt i 14a/14b etter hovedagentens
avgjørelse). Funn 1–12 og byggerlisten (14 punkter) er tatt inn; numrene står i hakeparentes, f.eks. [F3], [B7].
Del 2 (hånd-IK på egen kropp, fot-IK i trapper, armer i kikkert, ev. førstepersonsskalering) er `plan-08b.md`.

Bygges når Helgasetr-kartet (lag Grafikk) er spillbart. Runde 13 (`b1eb305`) er godkjent. Bruk funksjonsnavn, ikke
linjenumre. Les koden slik den står; avviker den fra det som står her, følg koden og skriv det i rapporten.

## Mål
Ser du ned, ser du din egen kropp – bryst, mage, bein og føtter som går, huker og glir – sammen med dagens
førstepersonsarmer på våpenet. Andre ser den samme kroppen gå med gevær-/pistolanimasjoner, holde våpenet med begge
hender og bøye overkroppen litt etter siktet. Skyggen din på gulvet er hel.

## Forbedrer (gammelt)
- **Beina du ser når du ser ned** (`FPLegs`: bare beina) blir en kropp uten hode og armer: bryst, mage, bein, føtter.
- **Andre spillere** går i dag med armene hengende (`ABP_Unarmed`) mens våpenet svever ved skulderen. Nå holder de
  det med hendene, går med gevær-/pistolanimasjonene og bøyer ryggen litt etter siktet.
- **Huking**: kroppen står i dag rett opp når kapselen er lav (hodet stikker ut over treffboksen, kameraet havner i
  brystet). Nå senkes hofta og knærne bøyes – også under glid.
- **Rettinger fra runde 13-kontrollen** (steg 0).

## Nytt
- `UGSBodyAnim` (ny, C++ uten Blueprint): blander gange/jogg i åtte retninger og fall fra sekvensene som allerede
  ligger i prosjektet, ryggbøy etter sikte, huk og knebøy i kode, to-bens-IK for hender (andre spillere) og knær.
- `FPBody`: din egen kropp uten hode og armer, som bare du ser.
- Hel skygge av deg selv for deg selv (`WorldSpaceRepresentation`).
- Konsollvalg `gs.TrueFP 0/1` (av = gammel løsning) og `gs.FP.Debug 1` (tegner IK-mål og rekkevidde).

## Hvorfor nå
Andreas' liste etter det nye kartet, punkt 1: «hele kroppen synlig når man ser ned (ikke bare beina)». Alt i punkt 1
og 3 trenger en kropp som kan styres fra kode. 14a er den trygge halvdelen: kroppen og animasjonen. Hendene på din
egen kropp kommer i 14b.

## Hva som finnes (ærlig)
**Lokalt i `Content/Characters/Mannequins/` (Third Person-malen, 128 filer, ikke i git; alle peker på skjelettet
`/Game/Characters/Mannequins/Meshes/SK_Mannequin`)** [B1 – nøyaktige stier, med undermapper]:
- Kropper: `Meshes/SKM_Manny_Simple`, `Meshes/SKM_Quinn_Simple` (`SKM_Manny`/`SKM_Quinn` uten «_Simple» finnes ikke).
- Ubevæpnet: `Anims/Unarmed/MM_Idle`, `Anims/Unarmed/Walk/MF_Unarmed_Walk_{Fwd,Fwd_Left,Fwd_Right,Left,Right,Bwd,Bwd_Left,Bwd_Right}`,
  `Anims/Unarmed/Jog/MF_Unarmed_Jog_*` (samme 8), `Anims/Unarmed/Jump/MM_Fall_Loop`.
- Gevær: `Anims/Rifle/MF_Rifle_Idle_ADS`, `Anims/Rifle/Walk/MF_Rifle_Walk_*`, `Anims/Rifle/Jog/MF_Rifle_Jog_*`,
  `Anims/Rifle/Jump/MM_Rifle_Jump_Fall_Loop` (siktemappa heter `Rifle/AIM`, store bokstaver – brukes ikke nå).
- Pistol: `Anims/Pistol/MF_Pistol_Idle_ADS` (ikke `Pistol/Aim/MF_Pistol_Idle_ADS1`), `Anims/Pistol/Walk|Jog/MF_Pistol_*`,
  `Anims/Pistol/Jump/MM_Pistol_Jump_Fall_Loop` (pistol har ingen `Apex`/`Fall_Land`).
- Bare én animasjonsblueprint (`ABP_Unarmed`). Ingen huk-, glid-, klatre-, sprint-, lene- eller ligge-animasjoner.
**I C++ uten editoren:** egen `UAnimInstance` + `FAnimInstanceProxy` som overstyrer `Evaluate` (motoren kaller den
før grafen, og grafen er tom), sampler sekvensene, blander, bøyer rygg og løser IK. Ingen `.uasset` lages.
**Andreas trenger ikke gjøre noe.** Animasjonspakker (GASP/Lyra) hentes ikke foreløpig.

## Avgrensning (ikke i 14a)
Hånd-IK på `FPBody` og egne armer i kroppen, fot-IK på trappetrinn, armer skjult i kikkert, førstepersonsskalering
(alt i 14b). Hodebevegelse i kameraet [F11/B12] (plan-09, fra `FPBody`s hode). Hopp/landing, skudd og lading på
kroppen, slag, glid- og klatrestilling utover huk (plan-09). Treffboksene står som i dag (bokser på kapselen).

## Filer
| Fil | Endring |
|---|---|
| `Public/GSBodyAnim.h`, `Private/GSBodyAnim.cpp` (**nye**, Kamp eier) | `UGSBodyAnim`, `FGSBodyAnimProxy`, `FGSBodyAnimInput` |
| `Public/GSCharacter.h`, `Private/GSCharacter.cpp` | `FPBody` (erstatter `FPLegs`), `SetupHumanBody`, ny `SetupLocalBody`, ny `FallBackToOldBody`, ny `FillBodyAnimInput`, `Tick` (fjern-spiller-delen + eierens `TPWeaponMesh`), `OnRep_Dead`, `FixBodyMeshOffset`, `PawnClientRestart` (ny overstyring) |
| `Private/GSPlayerController.cpp` | steg 0 |
| `Gloshaugen.Build.cs` (felles, lite tillegg) | `"AnimationCore"` i `PrivateDependencyModuleNames` |
| `IMPROVEMENTS.md` (566–581) | |

Ikke rør `GSTypes.h`, `GSItems.*`, `catalog.py`, `GSMenu.*`, `GSHelp.*`, `GSGameMode.*`, `GSFlag.*`, `GSBallistics.*`,
`GSCampus*`. Stage filene med navn. Gjør headerne ferdige i ett.

## Steg

### 0. Forberedelse og små rettinger fra runde 13
1. `git log --oneline -8`, `git status`. `Get-Process UnrealEditor`: kjører editoren, ikke bygg.
2. **Funn 1 (`AGSPlayerController::MenuClick`):** etter `GSMenu::AskQuit(this)`: hvis `!GSMenu::IsModal(this)`,
   `ConsoleCommand(TEXT("quit"))`. «Hovedmeny»: `IsModal` blir neppe sann i samme bilde som `OpenMain` (den krever
   `LastDrawFrame` innen 30 bilder og `bHookLive`) [F12], så bruk reserveløsningen: kall `GSMenu::OpenMain(this)` og
   `CloseMenu()` bare hvis tastekroken er på (det `IsModalState` sjekker); ellers står Kamp-menyen åpen. Skriv valget.
3. **Styring del 2, funn 1:** `ToggleInventory` og `ToggleReady` gjør ingenting når `bMenuOpen`.
4. **Funn 2 (`AddBodyWound`):** hopp over hvis punktet er over 80 m fra lokalt kamera.

### A. `UGSBodyAnim` (nye filer)
5. `Gloshaugen.Build.cs`: legg `"AnimationCore"` i `PrivateDependencyModuleNames` (ett ord).
6. Les motorkoden først (under `UE_5.8\Engine\Source\Runtime\`): `Engine/Classes/Animation/AnimSingleNodeInstance.h`
   og `Engine/Public/Animation/AnimSingleNodeInstanceProxy.h` – **kopier mønsteret fra `FAnimSingleNodeInstanceProxy`**
   (`CreateAnimInstanceProxy`/`DestroyAnimInstanceProxy`, `Evaluate` → `true`) [B4]; `Engine/Private/Animation/
   AnimInstanceProxy.cpp` (`EvaluateAnimation_WithRoot`, `UpdateAnimation_WithRoot`); `Engine/Classes/Animation/
   AnimSequence.h` (`GetAnimationPose(FAnimationPoseData&, const FAnimExtractContext&)`); `Engine/Classes/Animation/
   AnimationAsset.h` (`FAnimExtractContext`: `CurrentTime`, `bExtractRootMotion`, `bLooping`);
   **`Engine/Public/AnimationRuntime.h`** (`#include "AnimationRuntime.h"`, ikke `Animation/…`) [B4];
   `Engine/Public/BonePose.h` (`FCSPose`, `SafeSetCSBoneTransforms`, `LocalBlendCSBoneTransforms`,
   `ConvertComponentPosesToLocalPoses`); `AnimationCore/Public/TwoBoneIK.h` (transform-varianten, linje ca. 69).
7. `GSBodyAnim.h`:
   ```cpp
   struct FGSBodyAnimInput   // kopieres fra spillet (spilltråd) til proxyen hver frame
   {
       FVector LocalVel = FVector::ZeroVector;   // fart i aktørens rom (cm/s), X fram, Y høyre
       float AimPitch = 0.f;                      // grader, normalisert
       float CrouchAlpha = 0.f;                   // 0 stå, 1 huk/glid (myk)
       float SprintAlpha = 0.f;
       bool bFalling = false, bDead = false;
       uint8 Set = 0;                             // 0 ubevæpnet, 1 gevær, 2 pistol
       bool bHandIK_R = false, bHandIK_L = false;
       FTransform HandR, HandL;                   // mål for hand_r/hand_l i KOMPONENTROM
       FVector ElbowHintR, ElbowHintL;            // komponentrom
       bool bFirstPerson = false;                 // eierens FPBody: ingen ryggbøy, ingen hånd-IK i 14a
       FVector CompFwd, CompRight, CompUp;        // aktørens akser omregnet til komponentrom [F12]
   };
   ```
   `UCLASS(Transient) class UGSBodyAnim : public UAnimInstance`: `FGSBodyAnimInput Input;` (public);
   `UPROPERTY() TArray<TObjectPtr<UAnimSequence>> Seqs;` [F2/B3]; `bool bMissingBones = false;` (settes av proxyen,
   leses av karakteren – tilbakefall underveis [B13]); konstruktøren setter `RootMotionMode =
   ERootMotionMode::IgnoreRootMotion` [F1/B2]. `FGSBodyAnimProxy : FAnimInstanceProxy` med `PreUpdate` (kopierer
   `Input` og **rå pekere** fra `Seqs`), `Update(float Dt)` (fram tidene), `Evaluate(FPoseContext&)` (returnerer `true`).
8. **Lasting** [F2/B3]: én funksjon `UGSBodyAnim::LoadSequences()` på spilltråden (kalles fra `NativeInitializeAnimation`)
   med `LoadObject<UAnimSequence>(…, LOAD_NoWarn | LOAD_Quiet)` og stiene over i en fast rekkefølge (3 sett × [stå,
   8 gange, 8 jogg, fall] = 54 plasser). Tomme plasser fylles med ubevæpnet-settets. Aldri lasting i `Evaluate`
   (arbeidstråd). Mangler gevær-gange-fram eller ubevæpnet stå: `bMissingBones = true` (karakteren faller tilbake).
   Lasting av ~50 sekvenser ved første spawn gir et lite hakk – greit, én gang.
9. **Bevegelse i `Evaluate`:** `if (bDead) return true;` med posen urørt (ragdollen tar over) [F9]. `Spd = |LocalVel|`,
   `Dir = atan2(Y, X)`. Velg de to nærmeste av 8 retninger (45° trinn), bland lineært. Bland stå → gange (0–165 cm/s)
   → jogg (165–420). Avspillingsfart: gange `Spd/165`, jogg `Spd/380`, klemt 0,6–1,5; sprint (> 420) = jogg × 1,15.
   Gange og jogg deler fase (0..1 × lengde). Uttrekk med `FAnimExtractContext` der `bExtractRootMotion = true` og
   `bLooping = true` (roten låses, bevegelsen kastes – `MF_*`-filene har rotbevegelse på) [F1/B2]. Bruk
   `BlendTwoPosesTogetherInPlace`. **Høyst 4 sampler:** i overgang til fall, slå bevegelsen sammen til én pose (2
   sampler) før fall-løkka blandes inn (0,15 s) [F12]. Mål ekte fart med `ExtractRootMotionFromRange` over én syklus av
   `MF_*_Walk_Fwd`/`Jog_Fwd` (spilltråd, ved lasting) og bruk de målte tallene i stedet for 165/380; skriv dem i rapporten.
10. **Lokalt rom, etter blandingen** [B5]: (a) huk: `CrouchAlpha` senker `pelvis` 38 cm og vipper den 12° fram, ryggen
    10° fram; (b) ryggbøy etter sikte, **bare når `!bFirstPerson`**, samlet klemt til ±15° [F3/B6], fordelt
    0,2/0,2/0,2/0,2/0,2 på `spine_01..05` rundt `CompRight` omregnet til hvert beins rom (ikke fast lokal akse);
    (c) gevær/hagle (`Set == 1`): skytterstilling – brystet vris 15° (venstre skulder fram) over `spine_03..05` [F5b].
    Eierens `FPBody` får huk og skytterstilling, men aldri ryggbøy (ellers nakke/kragehull når du ser ned) [F3].
11. **Komponentrom** (`FCSPose<FCompactPose>`) [B5]:
    - **Knebøy-IK alltid når `CrouchAlpha > 0`** (også i fart og glid) [F4a/B7]: hver fot holdes der den var i
      animasjonen før hofta ble senket; løs `thigh → calf → foot` med `AnimationCore::SolveTwoBoneIK`
      (transform-varianten, `bAllowStretching = false`), knehint = kneet + `CompFwd` × 30 cm.
    - **Hender** (bare `!bFirstPerson` i 14a): når `bHandIK_*`, samme løser for `upperarm → lowerarm → hand`; håndas
      rotasjon = målets. Skriv med `LocalBlendCSBoneTransforms` med en vekt som glattes over 0,1 s (eller
      `SafeSetCSBoneTransforms` ved vekt 1), **aldri** `SetComponentSpaceTransform` bein for bein – ellers følger ikke
      fingrene og `ik_hand_*` med [B5].
    - NaN-vakt: hopp over et IK-resultat som `ContainsNaN()` [F12/B5].
    - `ConvertComponentPosesToLocalPoses` til slutt.
    - Bein-indekser finnes én gang (`GetRequiredBones()` + referanseskjelettet). Mangler `pelvis`, `spine_03`,
      `thigh_l/r`, `calf_l/r`, `foot_l/r`, `hand_l/r`: sett `bMissingBones = true` og returner posen uendret [B13].

### B. Kroppen i `AGSCharacter`
12. `GSCharacter.h`: bytt `FPLegs` med `UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> FPBody;`
    (søk etter alle bruk av `FPLegs` og flytt dem); `bool bTrueFP = false;`, `float CamForwardOffset = 0.f;`,
    `void SetupLocalBody();`, `void FallBackToOldBody();`, `void FillBodyAnimInput(UGSBodyAnim* Anim, bool bFirstPerson,
    float Dt);`, `virtual void PawnClientRestart() override;`, glattede `BodyCrouchAlpha`, `BodySprintAlpha`.
13. Konstruktøren: `FPBody` festes til kapselen, `SetOnlyOwnerSee(true)`, `SetCastShadow(false)`, ingen kollisjon,
    `SetVisibility(false)`, `FirstPersonPrimitiveType::None` [F8/B9].
14. `SetupHumanBody` (alle figurer): når `gs.TrueFP` (standard 1, `TAutoConsoleVariable<int32>`) og sekvensene finnes:
    `GetMesh()->SetAnimInstanceClass(UGSBodyAnim::StaticClass())`, `bTrueFP = true`; ellers dagens søkeliste med
    `ABP_Unarmed`. Andres `GetMesh()`: `OnlyTickPoseWhenRendered` som i dag og `bEnableUpdateRateOptimizations = true`
    (halv takt langt unna). Til slutt `SetupLocalBody()`.
15. **`SetupLocalBody()`** (kan kalles flere ganger; kalles fra `SetupHumanBody` og fra `PawnClientRestart` [F10/B11];
    gjør ingenting hvis `!IsLocallyControlled()`):
    - `bTrueFP`: `GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation)` (hel
      skygge; motoren tvinger `bOwnerNoSee` i proxyen) og `VisibilityBasedAnimTickOption =
      AlwaysTickPoseAndRefreshBones` [F7/B8] (`AlwaysTickPose` oppdaterer ikke beina når meshen ikke tegnes for eieren).
    - `FPBody`: samme modell, `SetAnimInstanceClass(UGSBodyAnim::StaticClass())`, `Input.bFirstPerson = true`,
      `HideBoneByName` på `neck_01` (hode og nakke; `head` alene lar nakken synes) [F12], `clavicle_l` og `clavicle_r`
      (armene – dagens førstepersonsarmer brukes), `AlwaysTickPoseAndRefreshBones`, synlig. `FPLegs`-koden for
      `spine_03` og −14 cm går ut.
    - `CamForwardOffset` regnes ut **fra referanseskjelettet** (sum av referansetransformer fra roten til `head`,
      omregnet med meshens −90° yaw), ikke etter første evaluering [F12]: avstand fram fra kapselsenteret til hodet,
      minus 4 cm, klemt 0..25 cm. `FPBody` plasseres som `GetMesh()` men flyttet bakover langs aktørens X med
      `CamForwardOffset`. `FixBodyMeshOffset` flytter `FPBody` likt (huk).
    - `GetMesh()->AddTickPrerequisiteActor(this)` og `FPBody->AddTickPrerequisiteActor(this)` [F6/B8]
      (`ACharacter` legger bare til bevegelseskomponenten for `Mesh`).
    - `!bTrueFP`: gammel løsning (som `SetupHumanBody` gjør i dag med `FPLegs`-oppførsel).
16. **`FallBackToOldBody()`** [B13]: `ABP_Unarmed` (søkelista) på `GetMesh()`, `FirstPersonPrimitiveType::None`,
    `OnlyTickPoseWhenRendered`; `FPBody` som gamle `FPLegs` (skjul `spine_03`, −14 cm); `bTrueFP = false`. Kalles fra
    `Tick` når `UGSBodyAnim::bMissingBones` er satt på `GetMesh()` eller `FPBody` (underveis), og når `gs.TrueFP` er 0
    ved oppsett.
17. **`FillBodyAnimInput`** (fra `Tick`, for `GetMesh()` på alle maskiner som ikke er dedikert server, og for `FPBody`
    hos eieren): `LocalVel` i aktørens rom; `AimPitch` = `GetBaseAimRotation().Pitch` normalisert; `CrouchAlpha`
    glattet (8/s) mot `bIsCrouched || bSliding`; `SprintAlpha` mot `bSprinting`; `bFalling`; `bDead`; `Set` fra
    våpenklassen (`Pistol` → 2; `Melee`, ingen våpen, ting i hånda → 0; resten → 1); `CompFwd/Right/Up` = aktørens
    vektorer omregnet med `GetMesh()->GetComponentTransform().InverseTransformVectorNoScale` (meshen er −90° yaw, så
    fram er +Y i komponentrommet – aldri faste akser) [F12].
18. **Hender for andre spillere og for eierens skygge** [F12]: `TPWeaponMesh` får to grepspunkt (høyre: våpenets origo,
    venstre: `MuzzleX × 0,45` fram, som `LeftHandHome`; pistol: venstre hånd under høyre) og faste forskyvninger
    `GripR/GripL` (startverdier høyre (−8, 3, −4) cm, venstre (−6, 0, −5), justerbare med `gs.FP.GripR`/`gs.FP.GripL`
    «x y z pitch yaw roll») til `hand_r`/`hand_l`. Målene regnes om til `GetMesh()`s komponentrom.
    `bHandIK_L = false` når de lader (`TPReload > 0,5`), har kniv/enhånds nærkamp, eller bruker en ting.
    Albuehint: skulder + `CompUp` × −30 + utover × 25 cm. **`TPWeaponMesh` plasseres også for eieren når `bTrueFP`**
    (samme kode som for andre; den er `bOwnerNoSee`, men skyggen av kroppen holder da våpenet riktig) – ellers slå av
    hånd-IK på eierens `GetMesh()`. Skulderpunktet for `TPWeaponMesh` følger `clavicle_r` på `GetMesh()` når `bTrueFP`
    (så våpenet sitter på kroppen i huk).
19. `gs.FP.Debug 1`: `DrawDebugCoordinateSystem` på hånd-mål og hånd-bein, og skriv rekkevidden (avstand skulder→mål /
    armlengde fra referansestillingen) i prosent for hver figur på skjermen [F5c].
20. **`OnRep_Dead`** [F9/B9]: sett `GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None)` (det er
    *typen* som teller – så lenge den er `WorldSpaceRepresentation` tvinger proxyen `bOwnerNoSee`; rekkefølgen mot
    `SetOwnerNoSee(false)` spiller ingen rolle). `FPBody` skjules av løkka over `bOnlyOwnerSee` (sjekk). `Input.bDead`
    settes, så `Evaluate` slipper posen til ragdollen. Ved gjenoppliving settes typen på nytt i `SetupLocalBody`.
21. Ingen hodebevegelse i kameraet denne runden [F11/B12]. Kameraets komponentplass er uendret (skuddstart).

### C. Treffbokser, ytelse og test
22. **Hode mot hodeboks** [F3/B6]: med `gs.ShowHitZones 1` (finnes) og en annen spiller som sikter −80, 0 og +80 grader,
    stående og hukende: hodet skal ligge i hodeboksen (stående X=3, ±9,5 cm; hukende 62 cm over kapselsenteret).
    Juster huk-senkingen (38 cm) eller `GSZoneBoxes` (Kamp) til det stemmer; skriv tallene i rapporten.
23. **Ytelse:** `stat anim` og `stat game` i PIE med 2 spillere før/etter; skriv tallene.
24. **Test i editorens Play, 2 spillere, lytteserver** [B14] (kompilering beviser ingenting): se ned stående, gående,
    hukende og glidende (bryst, mage, bein, føtter; ingen nakke; dagens armer på våpenet); se den andre fra siden og
    forfra mens han går, huker og sikter opp/ned; sjekk egen skygge (hel, med hode og våpen); dø (dødskameraet ser
    kroppen); bytt `gs.TrueFP 0` + ny spawn (gammel løsning) [B13]. Skjermbilder hvis det går, ellers beskriv.

## Ferdig når
- Bygger uten feil (`Build.bat … -WaitMutex`, kjøres av byggeren).
- Ser du ned (stående, gående, hukende, glidende): bryst, mage, bein og føtter, ingen nakke eller hodeinnside;
  bena går i takt med farten, kroppen glir ikke fram og hopper tilbake (rotlås).
- Huk og glid: knærne bøyes, føttene går ikke gjennom gulvet, kameraet er ikke inni kroppen.
- Andre holder våpenet med begge hender (pistol: to hender), bøyer ryggen høyst 15° etter siktet og går med gevær-/
  pistolanimasjonene; hodet ligger i hodeboksen ved alle siktevinkler (tall i rapporten).
- Egen skygge er hel og holder våpenet.
- Dødskameraet viser kroppen; `gs.TrueFP 0` gir gammel løsning; manglende sekvenser gir gammel løsning (også underveis).
- Steg 2–4 virker.

## Risiko
- **Motor-API:** les 5.8-headerne (steg 6) før du skriver; ikke gjett signaturer.
- **Søppeltømming:** sekvensene ligger i `UPROPERTY` [F2]; proxyen får bare rå pekere i `PreUpdate`.
- **Online:** ingen ny replikering; alt kommer fra `bIsCrouched`, `bSprinting`, `bSliding`, `bAiming`, `bReloading` og
  `GetBaseAimRotation`. Kameraets komponentplass er uendret.
- **Hode/treffboks:** ryggbøy klemt [F3]; sjekket i steg 22.
- **Ytelse** på RTX 3050: eieren får to evalueringer (`GetMesh` + `FPBody`); andre som før (ABP_Unarmed var like dyr).
- **Armbind og sår** henger på `GetMesh()`; eieren ser dem ikke på `FPBody` – nevn i rapporten (14b/plan-09).
- **Manglende innhold** på en annen PC: tilbakefall til dagens løsning.

## IMPROVEMENTS-linjer (566–581)
Seksjon `## Runde 14a (Kamp): ekte førsteperson, del 1 – kroppen (dato)` med «Gammelt som er forbedret: …».

566. Ser du ned, ser du din egen kropp – bryst, mage, bein og føtter – ikke bare beina
567. Din egen kropp går, jogger og stopper i takt med farten din, i åtte retninger
568. Kroppen glir ikke fram og hopper tilbake mellom skrittene (rotbevegelsen er låst)
569. Andre spillere holder våpenet med begge hender i stedet for å la armene henge
570. Andre spillere går og jogger med egne animasjoner for gevær og pistol, i åtte retninger
571. Andre spillere bøyer overkroppen litt etter hvor de sikter, uten at hodet flytter seg ut av treffet
572. Gevær og hagle holdes i skytterstilling med venstre skulder fram
573. Hukende figurer bøyer knærne og senker hoftene i stedet for å stå rett opp
574. Under glid bøyes knærne, og beina går ikke gjennom gulvet
575. Du har en hel skygge av deg selv på gulvet, med våpenet i hendene
576. Andre spilleres våpen følger skulderen når de huker
577. Hodet på andre spillere ligger der hode-treffet er, også når de huker (kontrollert med treffboksene)
578. Fallanimasjon når du hopper eller faller, for deg og for andre
579. «Avslutt» og «Hovedmeny» i reservemenyen virker også når hovedmenyen ikke kan vises
580. Inventar og «klar» kan ikke slås av og på mens menyen er åpen
581. Sår på kropper langt unna (over 80 m) lages ikke, så store kamper går lettere
