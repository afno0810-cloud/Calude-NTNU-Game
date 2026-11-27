# Forhåndssjekk av plan-08 (runde 14, lag Kamp): ekte førsteperson

**BØR ENDRES** – og deles i to runder (14a og 14b).

Grunnideen holder: en animasjonsklasse i ren C++ uten AnimGraph går an i UE 5.8 (se under). Men planen har
fem feil som vil synes eller ødelegge spill (rotbevegelse, hode-treffboks, bein gjennom gulvet ved glid,
hender som ikke når våpenet, og at de lastede sekvensene kan bli kastet av søppeltømmingen), og to motor-
antakelser som er feil (`AlwaysTickPose`, rekkefølgen i `OnRep_Dead`). Med 25 steg i to nye filer og en
stor del av `GSCharacter.cpp` er runden også for stor til å bli ferdig og testet i én gang.

Kontrollert mot HEAD `9b30848` og motorkoden i `UE_5.8/Engine/Source/Runtime`.

## Er det gjennomførbart? (motorkoden)
- **Ja.** `FAnimInstanceProxy::EvaluateAnimation_WithRoot` (AnimInstanceProxy.cpp ca. linje 1430) kaller
  `Evaluate_WithRoot` → `Evaluate(Output)` først, og går bare til grafen hvis den gir `false`. Grafen er tom
  (`RootNode == nullptr`), og `EvaluateAnimationNode_WithRoot` gjør da ingenting. `Update(float)` kalles fra
  `UpdateAnimation_WithRoot` (linje ca. 1360). `UAnimSingleNodeInstance` gjør det samme (`Evaluate` → `true`),
  så den er det beste mønsteret å kopiere. `CreateAnimInstanceProxy`/`DestroyAnimInstanceProxy` er virtuelle i
  `AnimInstance.h` (linje 1716/1719).
- `UAnimSequence::GetAnimationPose(FAnimationPoseData&, const FAnimExtractContext&)` finnes (AnimSequence.h 427).
  `FAnimExtractContext` (i `Animation/AnimationAsset.h`) har `CurrentTime`, `bExtractRootMotion`, `bLooping`.
- `BlendTwoPosesTogether(FAnimationPoseData, FAnimationPoseData, float, FAnimationPoseData)` og
  `BlendTwoPosesTogetherInPlace` ligger i **`Engine/Public/AnimationRuntime.h`** (ikke `Animation/AnimationRuntime.h`
  som planen skriver). Bruk `InPlace`-varianten (én pose mindre per blanding).
- `AnimationCore::SolveTwoBoneIK(FTransform& Root, FTransform& Joint, FTransform& End, JointTarget, Effector,
  bAllowStretching, StartStretchRatio, MaxStretchScale)` finnes (TwoBoneIK.h 69). Modulen `AnimationCore` må inn i
  Build.cs, som planen sier.
- `FCSPose::SafeSetCSBoneTransforms` / `LocalBlendCSBoneTransforms` / `ConvertComponentPosesToLocalPoses` finnes
  (BonePose.h). **Viktig:** skriv IK-resultatet med `SafeSetCSBoneTransforms` (eller `LocalBlendCSBoneTransforms`
  med vekten), ikke `SetComponentSpaceTransform` bein for bein – ellers flytter ikke fingrene og `ik_hand_*` seg
  med hånda. Hofte, huk og rygg gjøres i **lokalt** rom før posen gjøres om til komponentrom, ellers er barna utdatert.
- Førstepersonstegning finnes i 5.8 under disse navnene: `EFirstPersonPrimitiveType { None, FirstPerson,
  WorldSpaceRepresentation }`, `UPrimitiveComponent::SetFirstPersonPrimitiveType`, `UCameraComponent::
  bEnableFirstPersonScale` / `SetFirstPersonScale` / `bEnableFirstPersonFieldOfView`. Det virker fordi spillets
  `CalcCamera` kaller `Super::CalcCamera` (henter kameraets `GetCameraView`, CameraComponent.cpp 476).
  Ingen innstilling i `DefaultEngine.ini` trengs: skyggen av `WorldSpaceRepresentation` går via VSM sin egen
  førstepersonsklipp-kart, som krever `HasFirstPersonGBufferBit` (ikke-mobil, ikke forward, ingen statisk lys) –
  prosjektet har `r.AllowStaticLighting=False` og `r.Shadow.Virtual.Enable=1`, så det er oppfylt.
- `FirstPerson`-primitiver kaster aldri skygge (PrimitiveSceneProxy.cpp 632–648), og `WorldSpaceRepresentation`
  tvinger `bOwnerNoSee = true` og `bCastHiddenShadow = false` **i proxyen**, uansett hva komponenten sier.

## Ressursene (Content/Characters/Mannequins)
Alt planen nevner finnes, og alle sekvensene peker på `/Game/Characters/Mannequins/Meshes/SK_Mannequin`.
**Men stiene har undermapper**, så `LoadObject` må bruke de nøyaktige stiene:
- `Anims/Unarmed/MM_Idle`, `Anims/Unarmed/Walk/MF_Unarmed_Walk_{Fwd,Fwd_Left,Fwd_Right,Left,Right,Bwd,Bwd_Left,Bwd_Right}`,
  `Anims/Unarmed/Jog/MF_Unarmed_Jog_*`, `Anims/Unarmed/Jump/MM_Fall_Loop`
- `Anims/Rifle/MF_Rifle_Idle_ADS`, `Anims/Rifle/Walk/MF_Rifle_Walk_*`, `Anims/Rifle/Jog/MF_Rifle_Jog_*`,
  `Anims/Rifle/Jump/MM_Rifle_Jump_Fall_Loop` (sikte-mappa heter `Rifle/AIM`, store bokstaver)
- `Anims/Pistol/MF_Pistol_Idle_ADS` (ikke `Pistol/Aim/MF_Pistol_Idle_ADS1`), `Anims/Pistol/Walk|Jog/MF_Pistol_*`,
  `Anims/Pistol/Jump/MM_Pistol_Jump_Fall_Loop` (pistol har ingen `Apex`/`Fall_Land`)
- Skjelett `Meshes/SK_Mannequin`, kropper `SKM_Manny_Simple`, `SKM_Quinn_Simple`.

## Funn som må inn i planen

1. **Rotbevegelse – kroppen glir fram og hopper tilbake.** `MF_*_Walk/Jog`-filene har `bEnableRootMotion`
   lagret (ikke standardverdi, altså på). `GetAnimationPose` låser roten bare når `bExtractRootMotion && bEnableRootMotion`
   eller `bForceRootLock` (AnimSequence.cpp 1862). Planens `bExtractRootMotion = false` gir altså en rot som
   vandrer en hel skrittlengde og hopper tilbake hver syklus. **Endring:** sett `RootMotionMode =
   ERootMotionMode::IgnoreRootMotion` i `UGSBodyAnim`, og `bExtractRootMotion = true` i uttrekket (roten låses,
   bevegelsen kastes). Fartene 165/380 kan da måles med `ExtractRootMotionFromRange` slik planen foreslår.

2. **Sekvensene kan bli kastet av søppeltømmingen (krasj).** En statisk tabell med rå `UAnimSequence*` holder
   dem ikke i live; ingen andre refererer dem. **Endring:** hold dem i en `UPROPERTY() TArray<TObjectPtr<UAnimSequence>>`
   på `UGSBodyAnim` (eller en `UPROPERTY` på karakteren), og gi proxyen rå pekere i `PreUpdate`. Last dem bare på
   spilltråden, aldri i `Evaluate` (den kjøres på arbeidstråder).

3. **Ryggbøy etter sikte flytter hodet ut av hode-treffboksen.** Vektene 0,1+0,15+0,2+0,25+0,3 = 1,0, så 60° sikte
   ned gir 60° ryggbøy. Med ca. 60 cm fra `spine_01` til hodet flyttes hodet ca. 40 cm fram/ned; hodeboksen er
   ±9,5 cm (`GSZoneBoxes`, stående X=3). Andre sikter på hodet de ser, og bommer. **Endring:** samlet ryggbøy
   klemmes til ±15° (resten tas av armene/IK og våpenet, som allerede følger siktet), og byggeren sjekker med den
   eksisterende `gs.ShowHitZones 1` at hodet ligger i hodeboksen ved sikte −80, 0 og +80, stående og hukende.
   Samme sjekk for huk-stillingen (hodeboks hukende: 62 cm over kapselsenteret = 120 cm over gulvet).
   Eierens egen kropp (`bFirstPerson`): **ingen** ryggbøy (structens kommentar «ryggen bøyes mer» må bort) –
   ellers ser du nakke og kragehull når du ser ned.

4. **Huk/glid: beina gjennom gulvet.** Huk senker hoften 38 cm og regner med at bein-IK bøyer knærne, men steg 13
   slår bein-IK av over 200 cm/s og i fall. Glid er huk i høy fart. **Endring:** skill to ting: (a) knebøy-IK som
   holder føttene der animasjonen hadde dem når hofta senkes, er alltid på når `CrouchAlpha > 0`; (b) bare
   gulv-sporingen (`FootOffsetL/R`) er begrenset til < 200 cm/s og ikke fall.

5. **Hendene når ikke våpenet; magasinet svever.** Anslag (Manny: overarm + underarm ca. 55 cm; kroppen flyttes
   bakover så hode-beinet er 4 cm bak kameraet; venstre skulder ca. (−6, −18, −15) fra kameraet):
   gevær i hofta (`FPHip (26,12,−18)`, `MuzzleX` 50 → venstre hånd ved X≈42) er ca. 58 cm unna = **utenfor rekkevidde**;
   lading tar venstre hånd til lomma (`Pouch (−18,14,−40)` i våpenrom) ca. 63 cm unna = utenfor. Med
   `bAllowStretching = false` stopper hånda kort, mens `FPMag` henger på den usynlige `FPArmL` og svever videre.
   **Endring:** (a) regn ut rekkevidden på spilltråden: klem `FPArmL`/`FPArmR`-målet til 97 % av armlengden fra
   skulderen (fra referansestillingen) og **flytt selve `FPArmL` dit**, så magasinet følger hånda; (b) gi
   gevær/hagle en skytterstilling: brystet vris 15–20° (venstre skulder fram) i `spine_03..05` når settet er 1;
   (c) logg rekkevidde i prosent per våpenklasse (hofte, sikte, lading) med `gs.FP.Debug 1` og skriv tallene i
   rapporten. Er noen over 100 % etter (b), flyttes våpenplasseringen i `UpdateWeaponAnimation` (Kamp), ikke i katalogen.

6. **Hendene henger én frame etter når du snur deg fort.** Kameraet (`bUsePawnControlRotation`) får rotasjonen
   først i `GetCameraView` under `UpdateCameraManager`, etter alle tick (LevelTick.cpp 1847). I `Tick` er
   `FPArmR->GetComponentTransform()` altså fra forrige frame; ved rask snuing havner hånda flere cm fra grepet.
   **Endring:** regn ut hånd-målene uten verdensrom: våpenets plass relativt til kameraet × kamera relativt til
   kapselen bygd av `GetControlRotation()` (relativ til aktørens rotasjon) og `Camera`s relative plass × invers av
   `FPBody`s relative plass. Alternativt `Camera->SetWorldRotation(GetControlRotation())` rett før målene regnes ut.
   Tick-rekkefølgen: `FPBody->AddTickPrerequisiteActor(this)` og `GetMesh()->AddTickPrerequisiteActor(this)` –
   `ACharacter` legger bare til bevegelseskomponenten for `Mesh` (Character.cpp 155), ikke aktøren.

7. **`AlwaysTickPose` holder ikke.** Den oppdaterer beinene bare når meshen tegnes (SkinnedMeshComponent.h 100), og
   eierens `GetMesh()` tegnes aldri for eieren (`bOwnerNoSee`). Da fryser skyggen, og `head`-/`pelvis`-bein som
   leses av koden er gamle. **Endring:** `AlwaysTickPoseAndRefreshBones` for eierens `GetMesh()` (og `FPBody`).

8. **`FirstPersonScale` på hele kroppen gir røntgen-bein.** Skalering mot kameraet gir samme bilde, men halv dybde.
   Står du bak et rekkverk, et bord eller en lav mur, tegnes beina dine *gjennom* den, og trappekanter foran foten
   dekker ikke foten. **Endring:** `FPBody` får `FirstPersonPrimitiveType::None` i denne runden, `gs.FP.Scale`
   har standard 1,0 (av), og våpenet beholder dagens tegning + `WallBlend`. (Senere: del i armer/overkropp som
   `FirstPerson` med `thigh_l/r` skjult, og bein som `None` med `SetLeaderPoseComponent` – egen runde.)

9. **`OnRep_Dead`-begrunnelsen er feil (men steget er riktig).** Rekkefølgen spiller ingen rolle: proxyen tvinger
   `bOwnerNoSee` så lenge typen er `WorldSpaceRepresentation`. Det viktige er at typen settes til `None` (da tegnes
   kroppen i dødskameraet). Sjekket: `FPBody` er `bOnlyOwnerSee`, så løkka i `OnRep_Dead` skjuler den. Ragdoll tar
   over som før (fysikk-vekt 1); proxyen kjører fortsatt `Evaluate`, så la `Evaluate` returnere tidlig når `bDead`.

10. **Eierens kropp settes bare opp i `BeginPlay`.** `SetupHumanBody` kalles bare der, og `IsLocallyControlled()`
    kan være usann i `BeginPlay` hos en klient hvis kontrolleren ikke er løst ennå. Faller da bare tilbake til gammel
    løsning (ikke farlig), men er ustabilt. **Endring:** del ut eier-delen i `SetupLocalBody()` som kan kalles flere
    ganger, og kall den også fra `PawnClientRestart` (eller `NotifyControllerChanged`).

11. **Hodebevegelse i kameraet (steg 17, siste del): ta den ut.** Den leser `head` fra `GetMesh()`, som har ryggbøy
    etter sikte, så klemmen på ±4 cm vil stå i taket hele tiden når du ser opp/ned. Spillet har allerede kamera-dupp
    og våpen-gynging; dobbel gynging gir lett bevegelsessyke. Flytt til plan-09, og da fra `FPBody`s hode.

12. **Mindre ting:**
    - `CamForwardOffset`: regn den ut fra referanseskjelettet (sum av ref-transformer opp til `head`), ikke «etter
      første evaluering» – deterministisk og uten tidsproblem.
    - Skjuler `head` ikke nok (nakken synes når du ser ned), skjul `neck_01` i stedet.
    - Akser: meshen er rotert −90° yaw, så i komponentrommet er **fram = +Y** og høyre = −X. Hent aksene ved å regne
      om aktørens vektorer, ikke skriv dem fast (gjelder knehint «fram 30 cm», ryggens sideakse, albuehint).
    - Fot-sporing: planen sier både `ECC_Visibility` og objektspor. Bruk `LineTraceSingleByObjectType` med
      `WorldStatic` + `WorldDynamic` (lærdom), ignorer seg selv. Hopp over når `!GetMesh()->WasRecentlyRendered(0.2f)`
      (andre spillere bak deg), i tillegg til 25 m.
    - Høyst 4 sampler: i overgang til fall trengs 5 (2+2 bevegelse + fall). Slå da bevegelsen sammen til 2 først.
    - NaN-vakt: sjekk `ContainsNaN()` på IK-resultat og hopp over (NaN i bein gjør meshen usynlig og kan få fysikken
      til å klage når ragdollen starter).
    - Eierens skygge: `TPWeaponMesh` plasseres i dag bare for andre (`Tick`, fjern-spiller-delen). Med hånd-IK på
      eierens `GetMesh()` mot et våpen som ikke flyttes, får skyggen rare armer. Plasser `TPWeaponMesh` også for
      eieren når `bTrueFP`, eller slå av hånd-IK på eierens `GetMesh()`.
    - Armbindene og sårene henger på `GetMesh()` (eier ser dem ikke). Greit nå; nevn i rapporten.
    - Steg 0: `GSMenu::IsModal` krever `LastDrawFrame` innen 30 bilder og `bHookLive`, så den blir neppe sann i
      samme bilde som `OpenMain` – planens reserveløsning trengs sannsynligvis. Greit som skrevet.
    - `LoadObject` av ~30 sekvenser ved første spawn gir et lite hakk. Greit; gjør det én gang.

## Ytelse (RTX 3050, 20 spillere)
Akseptabelt slik det er beskrevet: i dag har alle 20 allerede en skjelettkropp med `ABP_Unarmed` (blandingsrom +
tilstandsmaskin). Fire sampler à ~90 bein + to blandinger + rygg + 4 to-bens-IK er omtrent samme kostnad, og kjøres
på arbeidstråder. Eieren får to evalueringer (`GetMesh` + `FPBody`). Foreslått tillegg: `bEnableUpdateRateOptimizations`
på andres `GetMesh()` (halv takt langt unna). Mål `stat anim` / `stat game` som planen sier.

## Nettverk
Ingen ny replikering trengs: `bIsCrouched`, `bSprinting` (`COND_SkipOwner`), `bSliding`, `bAiming`, `bReloading`
og sikte-pitch (`GetBaseAimRotation`) replikeres allerede. Kameraets komponentplass endres ikke (når hodebevegelsen
tas ut, se 11), så serverens startsjekk for skudd står. Treffboksene henger fortsatt på kapselen; derfor funn 3.

## Det som virker i dag
Lading, sikting, slag, glid, klatring, inspeksjon: beholdes fordi `FPArmR/L` fortsatt flyttes (bare usynlige).
Dødskamera, ragdoll, sår og blod festet til `GetMesh()`-bein: virker, gitt funn 7 og 9. Treffbokser: uendret, gitt
funn 3. Tilbakefall: godt ved oppstart (mangler sekvenser → gammel løsning), men legg til et tilbakefall underveis:
finner proxyen ikke beina den trenger (`pelvis`, `hand_r`, `foot_l` …) eller er sekvensene borte, setter den et
flagg som `AGSCharacter` leser neste frame og bytter til gammel løsning (`ABP_Unarmed`, `FPLegs`-oppførsel, synlige
`FPArmR/L`) for den figuren. `gs.TrueFP 0` må testes som eget punkt.

## Forslag til deling
**Runde 14a – kroppen og animasjonen (lavere risiko, gir Andreas punkt 1 delvis):**
steg 0; `UGSBodyAnim` med bevegelse (8 retninger, gange/jogg, fall), rotlås (funn 1), sekvenser i `UPROPERTY`
(funn 2), klemt ryggbøy + treffboks-sjekk (funn 3), huk med knebøy (funn 4a), hånd-IK mot `TPWeaponMesh` for
andre spillere; `GetMesh()` som `WorldSpaceRepresentation` for eieren (hel skygge, funn 7); `FPBody` erstatter
`FPLegs` som helkropp **uten hode og uten armer** (`HideBoneByName` på `neck_01`, `clavicle_l`, `clavicle_r`), og
dagens førstepersonsarmer beholdes. Da ser du bryst, mage, bein og føtter når du ser ned.
**Runde 14b – hendene og føttene:** armene på `FPBody` med hånd-IK (funn 5, 6), skjul armer i kikkert (steg 22),
fot-IK i trapper (steg 13/19 med funn 4b), eventuelt `FirstPersonScale` med delt kropp (funn 8).

## Liste til byggeren (gjelder uansett deling)
1. Stier med undermapper som listet over; `MF_Pistol_Idle_ADS` fra `Anims/Pistol/`.
2. `RootMotionMode = IgnoreRootMotion`, `bExtractRootMotion = true` ved uttrekk.
3. Sekvenser i `UPROPERTY`-array, lastes på spilltråden; proxyen får rå pekere i `PreUpdate`.
4. Kopier mønsteret fra `FAnimSingleNodeInstanceProxy`; `#include "AnimationRuntime.h"` (ikke `Animation/…`).
5. Lokalt rom: blanding, hofte/huk, rygg. Så komponentrom: IK med `SafeSetCSBoneTransforms`/`LocalBlendCSBoneTransforms`.
   Så tilbake til lokalt. NaN-vakt.
6. Ryggbøy klemt til ±15° totalt for andre, 0 for eierens `FPBody`; sjekk hodet mot `gs.ShowHitZones 1` ved −80/0/+80,
   stående og hukende, og juster ev. `GSZoneBoxes` (Kamp) – skriv tallene i rapporten.
7. Knebøy-IK alltid på ved huk/glid; gulv-sporing bare < 200 cm/s og ikke fall, og bare når meshen er tegnet nylig.
8. `AlwaysTickPoseAndRefreshBones` for eierens `GetMesh()` og `FPBody`; `AddTickPrerequisiteActor(this)` på begge.
9. `FPBody` og våpen: `FirstPersonPrimitiveType::None`, `gs.FP.Scale` standard 1,0. `GetMesh()` for eier:
   `WorldSpaceRepresentation`; i `OnRep_Dead` tilbake til `None`.
10. Hånd-mål regnet i kapselrom fra `GetControlRotation()`, klemt til 97 % rekkevidde, og `FPArmL` flyttes til det
    klemte målet så `FPMag` følger. Rekkevidde-prosent per våpenklasse i rapporten.
11. `SetupLocalBody()` kalt fra `BeginPlay` og `PawnClientRestart`.
12. Ingen hodebevegelse på kameraet i denne runden.
13. Tilbakefall både ved oppstart og underveis (flagg fra proxyen). Test `gs.TrueFP 0` + ny spawn.
14. Test i editorens Play med 2 spillere (lytteserver): se ned stående/gående/hukende/glidende, se den andre fra
    siden og forfra mens den sikter opp/ned, dø (dødskameraet ser kroppen), og sjekk skyggen.
