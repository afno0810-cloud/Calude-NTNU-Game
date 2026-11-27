# Plan 08 – runde 14 (lag Kamp): ekte førsteperson – én kropp for deg og for andre

Bygges når det nye Helgasetr-kartet (lag Grafikk) er spillbart; planen trenger ikke kartet, men testen gjør
(trapper, korridorer, vegger tett på). Runde 13 (`b1eb305`) er godkjent. Rett først de tre små funnene fra
`kontroll/runde-13.md` (steg 0). V8 (tilbakespoling) er flyttet til runde 17, se `retning.md`.

Bruk funksjonsnavn, ikke linjenumre. Les koden slik den står; avviker den fra det som står her, følg koden og skriv
det i rapporten.

## Mål
Ser du ned, ser du din egen kropp – bryst, mage, bein og føtter som går, huker og står i trapper – og hendene som
holder våpenet er kroppens egne hender. Andre ser den samme kroppen med våpenet i hendene og overkroppen som følger
hvor du sikter. Skyggen din på gulvet er hel. Våpen og armer går aldri gjennom veggen.

## Forbedrer (gammelt)
- **Beina du ser når du ser ned** (`FPLegs`, bare beina med overkroppen skjult) blir en hel kropp uten hode.
- **De løse førstepersonsarmene** (`SM_FPArm_R/L`, to statiske modeller) erstattes av kroppens egne armer; alle
  dagens prosedyrebevegelser (lading med magasin i hånda, sikte, sprint, glid, klatring, slag, inspeksjon, veggstilling)
  beholdes fordi hendene følger de samme punktene.
- **Andre spillere** går i dag med armene hengende (ABP_Unarmed) mens våpenet svever ved skulderen. Nå holder de det,
  bøyer overkroppen etter siktet og bruker gange/jogg-animasjonene for gevær og pistol som allerede ligger i prosjektet.
- **Huking**: kroppen står i dag rett opp når kapselen er lav (hodet stikker ut, kameraet havner inni brystet). Nå
  bøyes knærne og hoftene senkes.
- **Klipping**: veggsjekken (`WallBlend`) beholdes, og i tillegg tegnes våpen og egen kropp med motorens
  førstepersonstegning så de aldri stikker gjennom en vegg.

## Nytt
- `UGSBodyAnim` (ny, C++): egen animasjonsklasse uten Blueprint som blander gange/jogg i åtte retninger, fall, siktebøy
  i ryggen og full-body IK (hender til våpenet, føtter til gulvet, hofte ned ved huk).
- `FPBody`: din egen helkropp (uten hode) som bare du ser, plassert så kameraet sitter der øynene er.
- Hel skygge av deg selv for deg selv (kroppen andre ser kaster skyggen, `WorldSpaceRepresentation`).
- Fotplassering i trapper og på ujevnt gulv (føttene treffer trinnene).
- Konsollvalg `gs.TrueFP 0/1` (av = gammel løsning) og `gs.FP.Debug 1` (tegner IK-mål) til testing.

## Hvorfor nå
Andreas' liste etter det nye kartet, punkt 1: «hele kroppen synlig når man ser ned (ikke bare beina)». Alt i punkt 1
og 3 (animasjoner for hopp, slag, glid, klatring, lading, bruk av ting) trenger en kropp med hender som kan styres
fra kode. Denne runden lager grunnmuren; runde 15 (`plan-09`) og 16 (`plan-10`) legger animasjonene på den.

## Hva som finnes og hva som ikke finnes (ærlig)
**Finnes lokalt i `Content/Characters/Mannequins/` (fra Third Person-malen, 128 filer, ikke i git):**
- Kropper: `SKM_Manny_Simple`, `SKM_Quinn_Simple` (skjelett `SK_Mannequin`). **`SKM_Manny`/`SKM_Quinn` (uten
  «_Simple») finnes ikke** – `SetupHumanBody` faller i dag til `_Simple`. Det er greit.
- Animasjonsblueprint: bare `ABP_Unarmed`. Ingen for gevær/pistol.
- Sekvenser: `Rifle/` og `Pistol/`: gange og jogg i 8 retninger (`MF_Rifle_Walk_Fwd`, `_Fwd_Left`, …), sikte-stilling
  (`MF_Rifle_Idle_ADS`, `AO_Rifle` med opp/ned), hopp (`Jump_Start`, `_Apex`, `_Fall_Loop`, `_Fall_Land`,
  `_RecoveryAdditive`), `Fire`, `Reload`, `Equip`, `DryFire`, 8 `HitReact`. `Unarmed/`: `MM_Idle`, gange/jogg 8 retn.,
  `BS_Idle_Walk_Run`, `MM_Jump`, `MM_Fall_Loop`, `MM_Land`, `MM_Dash`, `MM_Attack_01..03`. `Death/`: 6 dødsfall.
- Rigger: `CR_Mannequin_Body`, `CR_Mannequin_FootIK` (Control Rig – krever Blueprint-graf, brukes **ikke**).
- `SourceArt/Models` har ingen skjelettmodeller; `SM_FPArm_R/L` er statiske.
**Finnes ikke:** huk-, glid-, klatre-/vault-, sprint-, lene- og ligge-animasjoner; animasjoner for å bruke ting
(spise, sprøyte, bandasje); førstepersonsarmer med skjelett. Lyra og Game Animation Sample er **ikke** i prosjektet.

**Det som kan gjøres i C++ uten editoren (denne runden):** en egen `UAnimInstance` med egen `FAnimInstanceProxy`
som overstyrer `Evaluate` og selv sampler sekvensene over (`UAnimSequence::GetAnimationPose`), blander dem
(`FAnimationRuntime::BlendTwoPosesTogether`), roterer ryggvirvler og løser to-bens-IK (`AnimationCore::SolveTwoBoneIK`)
for armer og bein. Ingen `.uasset` lages. Alt som mangler (huk, glid) lages som stilling i kode (hofte ned + IK).
**Andreas trenger ikke gjøre noe for denne runden.**

## Avgrensning (ikke i denne runden)
Hopp-start/landing, skudd- og ladeanimasjon på kroppen, slag, glid- og klatrestilling, treffrykk, bruk av ting med
kroppen (→ plan-09/plan-10). Ny, mer ekte karaktermodell (mannekengen er grå plast; se «Ressurser» i plan-09).
Treffboksene står som i dag (bokser på kapselen); å feste dem til beinene er backlog.

## Filer
| Fil | Endring |
|---|---|
| `Public/GSBodyAnim.h`, `Private/GSBodyAnim.cpp` (**nye**, Kamp eier) | `UGSBodyAnim`, `FGSBodyAnimProxy`, `FGSBodyAnimInput` |
| `Public/GSCharacter.h`, `Private/GSCharacter.cpp` | `FPBody` (erstatter `FPLegs`), `SetupHumanBody`, `UpdateArms`, `UpdateItemUse`, `UpdateWeaponAnimation` (slutten), `OnRep_Dead`, `Tick` (fjern-spiller-delen), `UpdateLocalVisuals` (kameraforskyvning), ny `FillBodyAnimInput` |
| `Private/GSPlayerController.cpp`, `Private/GSHUD.cpp` | bare rettingene i steg 0 |
| `Gloshaugen.Build.cs` (felles, lite tillegg) | legg `"AnimationCore"` til `PrivateDependencyModuleNames` |
| `IMPROVEMENTS.md` (566–581) | |

Ikke rør `GSTypes.h`, `GSItems.*`, `catalog.py`, `GSMenu.*`, `GSHelp.*`, `GSGameMode.*`, `GSFlag.*`, `GSBallistics.*`,
`GSCampus*`. Stage filene med navn. Gjør headerne ferdige i ett.

## Steg

### 0. Forberedelse og små rettinger fra runde 13
1. `git log --oneline -8`, `git status`. `Get-Process UnrealEditor`: kjører editoren, ikke bygg (be dirigenten).
2. **Funn 1 (`GSPlayerController::MenuClick`):** etter `GSMenu::AskQuit(this)`: hvis `!GSMenu::IsModal(this)`,
   `ConsoleCommand(TEXT("quit"))`. For «Hovedmeny»: kall `GSMenu::OpenMain(this)` først, og `CloseMenu()` bare hvis
   `GSMenu::IsModal(this)` ble sann – ellers står Kamp-menyen åpen. (Les `IsModal`: den krever at menyen er tegnet de
   siste 30 bildene; sjekk om den blir sann i samme bilde. Gjør den ikke det, bruk i stedet en enkel sjekk på om
   tastekroken finnes – den funksjonen `IsModalState` bruker – og skriv valget i rapporten.)
3. **Styring del 2, funn 1:** `ToggleInventory` og `ToggleReady` gjør ingenting når `bMenuOpen` (samme vakt som de
   andre handlingene).
4. **Funn 2 (`AddBodyWound`):** hopp over hvis avstanden fra lokalt kamera (første lokale `PlayerController`) til
   punktet er over 80 m.

### A. `UGSBodyAnim` – animasjon i C++ (nye filer)
5. `Gloshaugen.Build.cs`: legg `"AnimationCore"` i `PrivateDependencyModuleNames` (ett ord, ingenting annet).
6. Les først motorkoden (stiene under `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\`):
   `Engine/Public/Animation/AnimInstanceProxy.h` (`PreUpdate`, `Update`, `Evaluate`), `Engine/Private/Animation/
   AnimInstanceProxy.cpp` (rundt `EvaluateAnimation_WithRoot`: `Evaluate_WithRoot` → `Evaluate` kalles før grafen; bekreft
   at en klasse uten Blueprint-graf når hit), `Engine/Classes/Animation/AnimSequence.h` (`GetAnimationPose(FAnimationPoseData&,
   const FAnimExtractContext&)`), `Engine/Public/Animation/AnimationRuntime.h` (`BlendTwoPosesTogether`), `Engine/Public/
   BonePose.h` (`FCSPose<FCompactPose>`, `ConvertComponentPosesToLocalPoses`), `AnimationCore/Public/TwoBoneIK.h`.
   Overstyr `UAnimInstance::CreateAnimInstanceProxy`/`DestroyAnimInstanceProxy` slik motorens egne klasser gjør
   (søk etter `CreateAnimInstanceProxy` i `Engine/Classes/Animation/AnimSingleNodeInstance.h`).
7. `GSBodyAnim.h`:
   ```cpp
   struct FGSBodyAnimInput   // kopieres fra spillet hver frame (spilltråd) til proxyen (arbeidstråd)
   {
       FVector LocalVel = FVector::ZeroVector;   // fart i kroppens rom (cm/s), X fram, Y høyre
       float AimPitch = 0.f, AimYaw = 0.f;        // grader, normalisert (-90..90)
       float CrouchAlpha = 0.f;                   // 0 stå, 1 huk (myk overgang)
       float SprintAlpha = 0.f;
       bool bFalling = false;
       uint8 Set = 0;                             // 0 ubevæpnet, 1 gevær, 2 pistol
       bool bHandIK_R = false, bHandIK_L = false;
       FTransform HandR, HandL;                   // mål for hand_r/hand_l i KOMPONENTROM
       FVector ElbowHintR, ElbowHintL;            // komponentrom
       bool bFootIK = false;
       float FootOffsetL = 0.f, FootOffsetR = 0.f, PelvisOffset = 0.f;   // cm, fra fot-sporing (steg 13)
       bool bFirstPerson = false;                 // eierens egen kropp: hodet skjult, ryggen bøyes mer
   };
   ```
   `UCLASS(Transient) class UGSBodyAnim : public UAnimInstance` med `FGSBodyAnimInput Input;` (public, settes av
   `AGSCharacter`), `CreateAnimInstanceProxy`, `DestroyAnimInstanceProxy`. `FGSBodyAnimProxy : FAnimInstanceProxy` med
   `PreUpdate` (kopier `Input`), `Update(float Dt)` (fram tidene), `Evaluate(FPoseContext&)` (returner `true`).
8. **Sekvensene** lastes én gang (statisk tabell, `LoadObject<UAnimSequence>` med `LOAD_NoWarn | LOAD_Quiet`) fra
   `/Game/Characters/Mannequins/Anims/...`: per sett (Unarmed/Rifle/Pistol): stå (`MM_Idle` for ubevæpnet,
   `MF_Rifle_Idle_ADS`/`MF_Pistol_Idle_ADS` for våpen), gange 8 retninger, jogg 8 retninger, fall-løkke
   (`MM_Fall_Loop`, `MM_Rifle_Jump_Fall_Loop`, `MM_Pistol_Jump_Fall_Loop`). Mangler en sekvens: bruk ubevæpnet-settets;
   mangler alt: `Evaluate` returnerer `false` (motoren gir referansestilling) og `AGSCharacter` faller tilbake (steg 12).
   Våpenklasse → sett: `Pistol` → 2; `Melee`, ingen våpen, kastevåpen i hånda → 0; resten → 1.
9. **Bevegelse i `Evaluate`:** fart `Spd = |LocalVel|`, retning `Dir = atan2(Y, X)` (grader). Velg de to nærmeste av
   8 retninger (45° trinn) og bland dem lineært. Bland stå → gange (0–165 cm/s) → jogg (165–420) med vekter. Spill
   av fart: gange `Spd / 165`, jogg `Spd / 380`, klemt 0,6–1,5 (sprint over 420 = jogg × 1,15 og ryggen litt fram
   via `SprintAlpha`). Fasen deles mellom gange og jogg (normalisert 0..1 × lengde), så føttene ikke hopper i
   overgangen. `bFalling` → bland til fall-løkka på 0,15 s. Bruk `BlendTwoPosesTogether` parvis (aldri flere enn 4
   samples per frame). Rotbevegelse: av (`FAnimExtractContext` med `bExtractRootMotion = false`), CMC flytter figuren.
   Tallene 165/380 er startverdier: mål dem (rotbeinets forflytning over én syklus i `MF_*_Walk_Fwd`/`Jog_Fwd`, f.eks.
   via `ExtractRootMotionFromRange` eller `pelvis`-banen) og bytt til målte tall; skriv dem i rapporten.
10. **Rygg og sikte (lokalt rom, etter blandingen):** fordel `AimPitch` over `spine_01..spine_05` (vekter 0,1 / 0,15 /
    0,2 / 0,25 / 0,3, rundt beinets egen sideakse – bruk komponentrommets høyre-akse omregnet til beinets rom, ikke en
    fast lokal akse), og `AimYaw` (avvik mellom sikte og kropp, klemt ±30°) likt. `bFirstPerson`: hode og nakke
    roteres ikke (de er skjult). Med våpen: lite fram-bøy (5°) i `spine_03` i siktestilling.
11. **Huk (kode, ingen animasjon finnes):** `CrouchAlpha` senker `pelvis` 38 cm og vipper den 12° fram, ryggen 10° fram;
    føttene holdes der de var (steg 13 løser beina). Sjekk at hodet da havner rundt kameraets huk-høyde
    (`CrouchedHalfHeight` 58 → kameraet `66` over kapselsenteret = ca. 124 cm over gulvet); juster 38 cm hvis ikke.
12. **IK i komponentrom** (`FCSPose`): 
    - Hender (når `bHandIK_*`): `SolveTwoBoneIK(upperarm, lowerarm, hand, ElbowHint, mål)` uten strekking
      (`bAllowStretching = false`); sett håndas rotasjon lik målets. Bland inn med en vekt (0..1) som proxyen glatter
      over 0,1 s, så hendene ikke smetter når IK slås av/på.
    - Bein: se steg 13. Til slutt `ConvertComponentPosesToLocalPoses`.
    - Bein som ikke finnes (annen modell) → hopp over den delen. Finn bein-indekser én gang via
      `GetRequiredBones().GetCompactPoseIndexFromSkeletonIndex(RefSkeleton.FindBoneIndex(...))`.
13. **Føtter (full-body IK, enkel):** `AGSCharacter` sporer hver fot (se steg 19) og gir `FootOffsetL/R` (gulvets
    høyde under foten minus kapselbunnen, klemt −45..+45 cm) og `PelvisOffset = min(0, min(FootOffsetL, FootOffsetR))`.
    Proxyen senker `pelvis` med `PelvisOffset` (+ huk), og løser `thigh → calf → foot` slik at hver fot står på sin
    høyde (kneet peker fram: hint = kneets nåværende posisjon + fram 30 cm). Fot-rotasjon etter gulvets normal er
    ikke med (plan-09). Bare når figuren står eller går sakte (< 200 cm/s) og ikke faller; ellers bland ut på 0,1 s.

### B. Kroppen i `AGSCharacter`
14. `GSCharacter.h`: bytt `FPLegs` med `UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> FPBody;`
    (søk etter alle bruk av `FPLegs` og flytt dem), legg til `bool bTrueFP = false;`, `float CamForwardOffset = 0.f;`,
    `void FillBodyAnimInput(UGSBodyAnim* Anim, bool bFirstPerson, float Dt);`, og glattede verdier
    `BodyCrouchAlpha`, `FootOffsetL`, `FootOffsetR`.
15. Konstruktøren: `FPBody` festes til kapselen, `SetOnlyOwnerSee(true)`, `SetCastShadow(false)`, ingen kollisjon,
    `SetVisibility(false)`, `VisibilityBasedAnimTickOption = AlwaysTickPoseAndRefreshBones`.
16. `SetupHumanBody`:
    - Bruk `UGSBodyAnim::StaticClass()` som animasjonsklasse for `GetMesh()` når konsollvalget `gs.TrueFP` er 1
      (standard 1, `TAutoConsoleVariable<int32>` i `GSCharacter.cpp`) **og** minst gange-fram-sekvensen for gevær
      lastet; ellers den gamle søkelista (`ABP_Unarmed`) og den gamle `FPLegs`-oppførselen (`bTrueFP = false`).
    - `GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation)` for den lokalt
      styrte figuren (gir eieren hel skygge; motoren setter da selv `bOwnerNoSee`). For lokalt styrt:
      `VisibilityBasedAnimTickOption = AlwaysTickPose` (ellers fryser skyggen og ragdollen starter fra gammel stilling
      hos verten); andre beholder `OnlyTickPoseWhenRendered`.
    - Lokalt styrt og `bTrueFP`: `FPBody` får samme modell og `UGSBodyAnim`, `HideBoneByName(TEXT("head"))`
      (skjuler hode med barn), `SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson)`, synlig.
      Plasser den som `GetMesh()` (samme høyde og −90° yaw), men flyttet bakover langs X med `CamForwardOffset`:
      regn ut én gang etter første evaluering = (kamerapunktets X i kapselrom) − (`head`-beinets X i kapselrom) + 4 cm,
      klemt 0..25 cm, slik at kameraet sitter rett foran der ansiktet ville vært. `FixBodyMeshOffset` må flytte
      `FPBody` likt (huk).
    - `PrimaryComponentTick.AddPrerequisite` fra `GetMesh()` og `FPBody` til aktørens egen tick, så hendene leser
      våpenets plass fra *denne* framen (ellers henger hendene én frame etter). Sjekk i motorkoden om `ACharacter`
      allerede gjør dette for `Mesh` (`Character.cpp`, `PostInitializeComponents`); gjør det i så fall bare for `FPBody`.
17. **Kameraet og førstepersonstegningen:** sett `Camera->bEnableFirstPersonScale = true` og
    `Camera->SetFirstPersonScale(…)` fra konsollvalget `gs.FP.Scale` (standard 0,5) når `bTrueFP`; `bEnableFirstPersonFieldOfView`
    forblir av (samme synsvinkel, ellers står føttene et annet sted enn gulvet). Sett `FirstPersonPrimitiveType =
    FirstPerson` på `FPWeaponMesh`, alle `FPAtt*`, `FPMag`, `FPItemMesh`. Skalering mot kameraet endrer ikke hvordan
    tingene ser ut, bare dybden, så de tegnes foran veggen. Les `Renderer/Private/FirstPersonSceneExtension.cpp` og
    sjekk om noe må slås på i prosjektet (`DefaultEngine.ini`) for at det skal virke; skriv det i rapporten. Virker det
    ikke uten innstilling, bruk 1,0 og la `WallBlend` gjøre jobben alene.
    **Kameraets plass endres ikke for serveren**: `Camera` blir der den er (skudd, slag og kast starter derfra, serveren
    sjekker startpunktet). Hode-følging er bare synlig og legges i `UpdateLocalVisuals` som additiv forskyvning:
    nikk og sidebevegelse fra `head`-beinet i `GetMesh()` relativt til en rolig stående stilling, klemt ±4 cm, glattet.
18. **Hendene følger våpenet** (`UpdateArms`): når `bTrueFP`, skjul `FPArmR`/`FPArmL`/`FPItemHand` (`SetVisibility(false)`, ikke
    `SetHiddenInGame` på `FPArmL` – da forsvinner også `FPMag` som henger på den), men behold dem som punkter: all eksisterende bevegelse (lading, magasinet i
    `FPMag` som henger på `FPArmL`, pumpe, bolt, sikte) flytter fortsatt komponentene. Etter `UpdateWeaponAnimation` og
    `UpdateItemUse`: regn ut `HandR = GripR * FPArmR.World`, `HandL = GripL * FPArmL.World` (`GripR/GripL` = faste
    forskyvninger fra grepspunktet til `hand_r`/`hand_l`-beinet, siden modellenes origo er «toppen av pistolgrepet» og
    «undersiden av håndvernet», se `Tools/Blender/fp_arms.py`), omregnet til `FPBody`s komponentrom. Startverdier:
    `GripR` = (−8, 3, −4) cm og rotasjon slik at håndflata vender inn mot grepet; `GripL` = (−6, 0, −5). Gjør dem
    justerbare med konsollvalg `gs.FP.GripR`/`gs.FP.GripL` («x y z pitch yaw roll») og tegn dem med `gs.FP.Debug 1`
    (`DrawDebugCoordinateSystem` på mål og bein). `bHandIK_L = FPArmL er «synlig» etter dagens regler` (kniv og
    enhånds nærkamp: venstre arm henger). Bruker man en ting (`UseItemId`): høyre hånd til `FPItemMesh`
    (`FPItemHand`s plass), venstre hånd fri. Uten våpen og ting: ingen hånd-IK (armene følger animasjonen).
    Albue-hint: skulder + (ned 30, ut 25, bak 10) cm i kameraets rom.
19. **`FillBodyAnimInput(Anim, bFirstPerson, Dt)`** (kalles fra `Tick` for `GetMesh()` på alle maskiner som tegner, og for
    `FPBody` hos eieren): `LocalVel` = farten i aktørens rom; `AimPitch` = `GetBaseAimRotation().Pitch` normalisert
    (replikert for andre, kontrollerens for eieren); `CrouchAlpha` glattet mot `bIsCrouched` (8/s); `SprintAlpha`
    mot `bSprinting`; `Set` fra våpenklassen; fot-sporing: to korte linjespor (`ECC_Visibility`, objekt-spor mot
    `ECC_WorldStatic`/`WorldDynamic` som lærdommen sier) fra hoftehøyde ned 90 cm under `foot_l`/`foot_r` fra forrige
    frame, bare når hastigheten < 200 og ikke faller, bare innen 25 m fra lokalt kamera (andre spillere lenger unna: av).
    Hånd-mål for andre spillere: `TPWeaponMesh` får to faste grepspunkt (høyre: 0,0,0; venstre: `MuzzleX × 0,45`
    fram, som `LeftHandHome`) – samme `GripR/GripL`-regel. Når de lader (`TPReload > 0,5`) slipper venstre hånd (IK av).
20. **Fjern-spillere i `Tick`:** behold `TPWeaponMesh`-plasseringen (skulder, sikte, sprint, lading, slag), men flytt
    skulderpunktet så det følger `GetMesh()`s `clavicle_r` (når `bTrueFP`), så våpenet sitter der kroppen er når den
    huker eller går i trapper. Spøkelse-blinkingen og lagbåndene virker som før.
21. **`OnRep_Dead`:** sett `GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None)` **før**
    `SetOwnerNoSee(false)` (ellers tvinger motoren `bOwnerNoSee` tilbake og dødskameraet ser ikke kroppen); `FPBody`
    skjules (den er `OnlyOwnerSee`, så løkka der gjør det allerede – sjekk). Ragdollen tar over fra animasjonen som før.
    `OnRep_Dead` og `UpdateLocalVisuals` nullstiller den additive kameraforskyvningen (lærdom runde 2).
22. **Sikte gjennom kikkert** (`IsScopedIn`): `FPBody`s armer skjules sammen med våpenet (`HideBoneByName` på
    `clavicle_l/r` mens kikkerten er oppe, `UnHideBoneByName` etterpå) – ellers ser man underarmene inne i kikkertbildet.

### C. Ytelse
23. `UGSBodyAnim` sampler høyst 4 sekvenser + 2 rygg-/IK-runder per kropp. Andre spillere utenfor synet tikker ikke
    (`OnlyTickPoseWhenRendered`). Med 20 spillere: mål `stat anim` og `stat game` i PIE med 2 klienter før/etter, skriv
    tallene i rapporten. Fot-sporene (2 per kropp per frame) bare innen 25 m.

### D. Testing (PIE, editorens Play med 2 spillere, «Play as Listen Server»)
24. Prøv i editorens Play før levering (lærdom: kompilering beviser ingenting): se ned stående, gående, hukende, i en
    trapp; sikt med gevær, pistol og hagle; lad (magasinet skal være i kroppens venstre hånd); spis/drikk (høyre hånd
    til munnen); gå inntil en vegg; dø (dødskameraet skal se kroppen); se den andre spilleren fra siden og forfra.
    `gs.FP.Debug 1` mens grepene justeres. Ta skjermbilder hvis det går, ellers beskriv.
25. `gs.TrueFP 0` + ny spawn gir nøyaktig dagens oppførsel.

## Ferdig når
- Bygger uten feil (`Build.bat … -WaitMutex`).
- Ser du ned (stående, gående, hukende) ser du bryst, mage, bein og føtter fra *samme* kropp; ingen hodeinnside eller
  nakke i bildet; føttene går i takt med farten uten å gli tydelig.
- Hendene på våpenet er kroppens hender (ingen løse armer); ved lading går venstre hånd med magasinet; ved sikting
  sitter siktet der det satt før (siktepunktet `FPAim` er urørt).
- Huk: kameraet er ikke inni kroppen, knærne bøyes.
- I trapp står føttene på trinnene (begge eller den ene ned), ikke inni dem.
- Andre spillere holder våpenet med begge hender, bøyer overkroppen opp/ned etter siktet og går med gevær-/pistol-
  animasjonene; ingen hengende armer.
- Egen skygge på gulvet er hel (med hode).
- Inntil en vegg stikker verken løp eller hender gjennom veggen.
- Dødskameraet viser kroppen; `gs.TrueFP 0` gir gammel løsning.
- Rettingene i steg 2–4 virker (Avslutt i reservemenyen avslutter).

## Risiko
- **Ny API i motoren** (`FAnimInstanceProxy::Evaluate`, `GetAnimationPose`-signaturer endres mellom versjoner): les
  5.8-headerne før du skriver; byggeren skal ikke gjette.
- **Online:** animasjonen er bare kosmetisk og kjøres lokalt; ingen ny replikering. `AimPitch` for andre kommer fra
  `GetBaseAimRotation` (replikert pitch). Kameraets komponentplass er uendret, så serverens startsjekk for skudd står.
  Verten (listen server) tegner både sin egen `FPBody` og alle andre: `AlwaysTickPose` bare for egen figur.
- **Førstepersonstegning** (`FirstPersonScale`) kan ha følger for lys/skygge på egne armer og for gjennomsiktige
  ting (kikkertglass, røyk foran armene). Test; standard 0,5, fall tilbake til 1,0.
- **Klipping:** `FirstPersonScale` skjuler at armene går gjennom veggen, men kroppen kan fortsatt *se* gjennom en
  dør i hjørner hvis kameraet er nærmere veggen enn kapselradien – kameraet står i kapselsenteret, så det skjer ikke.
- **Hit-bokser** følger kapselen, ikke kroppen: når andre ser hukende kropp må `ApplyHitZoneLayout(true)` passe. Sjekk
  med `gs.FP.Debug 1` (tegn boksene) at hode-boksen i huk sitter på det nye hodet; juster `GSZoneBoxes`-huktallene om
  ikke (de er Kamps).
- **Ytelse** på RTX 3050: to kropper for egen spiller + IK. Mål (steg 23).
- **Manglende innhold** på en annen PC (Content er ikke i git): alt faller tilbake til dagens løsning.

## IMPROVEMENTS-linjer
Seksjon: `## Runde 14 (Kamp): ekte førsteperson – én kropp for deg og for andre (dato)`, med linja «Gammelt som er
forbedret: …» som i runde 13.

566. Ser du ned, ser du hele din egen kropp – bryst, mage, bein og føtter – ikke bare beina
567. Hendene som holder våpenet er nå kroppens egne hender, ikke løse armer foran kameraet
568. Ved lading tar kroppens venstre hånd magasinet ut og setter det nye inn
569. Når du spiser, drikker eller bruker noe, er det kroppens høyre hånd som holder tingen
570. Andre spillere holder våpenet med begge hender i stedet for å la armene henge
571. Andre spillere bøyer overkroppen opp og ned etter hvor de sikter
572. Andre spillere går og jogger med egne gange-animasjoner for gevær og pistol, i åtte retninger
573. Hukende figurer bøyer knærne og senker hoftene i stedet for å stå rett opp
574. Føttene står på trappetrinn og ujevnt gulv i stedet for å henge i lufta eller gå inni
575. Du har en hel skygge av deg selv på gulvet
576. Våpen og armer tegnes slik at de aldri stikker gjennom en vegg du står inntil
577. Kameraet sitter der øynene er og følger hodets små bevegelser, uten å endre hvor skuddet starter
578. Armene skjules når du ser gjennom en kikkert, så de ikke synes inne i siktebildet
579. «Avslutt» og «Hovedmeny» i reservemenyen virker også når hovedmenyen ikke kan vises
580. Inventar og «klar» kan ikke slås av og på mens menyen er åpen
581. Sår på kropper langt unna (over 80 m) lages ikke, så store kamper går lettere
