# Plan – runde 4 (lag Kamp): kroppssoner. Det betyr noe hvor kula treffer

Bygges oppå runde 3 («Blod», `ko/plan-03.md`). Planen er skrevet mot koden i commit `55ca90f` pluss det
plan-03 legger til. **Linjenumre flytter seg etter runde 3 – bruk funksjonsnavn og tekstene som er sitert.**
Følger `prioriteringer.md` punkt 2 (kroppssoner) og tar med de små juksehullene V1, V3 og V5 fra `funn.md`.

## Mål
Et skudd i beinet gjør at du halter, et skudd i armen at siktet svaier, vesten beskytter bare overkroppen og
hjelmen bare skallen – og treffområdene har størrelsen til en ekte kropp.

## Forbedrer (gammelt)
- **Treffboksene** (funn V9): én sylinder (62 cm bred) og en kule på 30 cm blir sju bokser med ekte mål.
  Hodet blir 19 × 16 × 24 cm (før nesten dobbelt så stort treffområde).
- **Skademodellen** (funn R4): i dag gjør et skudd i foten like mye som et skudd i hjertet, og vesten beskytter
  armer og bein. Blir: egne faktorer for arm, bein og mage; vest bare på overkroppen; hjelm ikke i ansiktet.
- **Plaster, bandasje og førstehjelpsskrin** får en ekte oppgave (behandler skadd arm/bein), og
  **smertestillende** demper følgene så lenge de virker.
- **Sikting online** (V1): sendes til serveren – slutt på rykking når klienter går og sikter, og haglene
  treffer der klienten ser dem.
- **Skuddtakt** (V3): serveren godtar ikke lenger 43 % raskere ild enn våpenet har.
- **Rakettkaster/granatkaster** (V5): høres ikke lenger ut som lyddempede våpen for andre.
- **Kroppen andre ser** står riktig på bakken hos alle spillere og når man huker (feil funnet ved lesing, steg C).
- **HUD**: «LAVT LIV»-teksten, det hvite helskjermsblinket og «HODESKUDD»-teksten fjernes (prioritering 4).
- Kontrollørens funn 3 fra runde 2: hodeskudd som knuser hjelmen mister ikke lenger øresus og rykk.

## Nytt
- **Sju treffsoner**: hode, bryst, mage, to armer, to bein. Følger huking og skliing.
- **Beinskade** (replikert `LegInjury`): saktere gange, lavere hopp, ingen sprint ved stor skade, og kameraet
  halter i takt med stegene.
- **Armskade** (replikert `ArmInjury`): siktet vandrer og skjelver med alle våpen, mer rekyl, tregere lading.
- **Du kjenner hvor du ble truffet**: kneet svikter ved beinskudd, våpenet rykker ved armskudd.
- **Andre ser at du sikter** (våpenet løftes til øyet).
- Feilsøkingskommando `gs.ShowHitZones 1` som tegner treffboksene, så Andreas og kontrolløren kan se at de
  stemmer med kroppen.

## Hvorfor nå
- Punkt 2 i hovedagentens prioriteringer, rett etter blodet. Blodet fra runde 3 viser *at* noen ble truffet;
  denne runden gjør at det betyr noe *hvor*.
- Grunnmur for det som kommer: blødning (mageskudd blør mest), sår på kroppen der kula traff, og ballistikk
  (gjennomslag gjennom en arm er noe annet enn gjennom brystet).
- V1 må inn før ballistikk-runden (serveren må vite om du sikter før den kan regne spredning selv, V2).
- Alt ligger i lagets egne filer. Ingen endring i våpendata; ingen nye .uasset-filer.

## Avgrensning (bevisst)
- Boksene henger på kapselen, ikke på skjelettet. Grunn: den synlige kroppen (`SetupHumanBody`) bruker
  `OnlyTickPoseWhenRendered` og har ingen huke-animasjon, så bein-festede bokser ville stått oppreist når
  spilleren huker, og ikke oppdatert seg på serveren for kropper verten ikke ser. Bein-nøyaktige bokser kommer
  når kroppen får ordentlige animasjoner (lagt i `retning.md`).
- Ingen blødning i denne runden (neste store steg sammen med lag Spillbarhet).
- V2 (klienten bestemmer spredning) tas i ballistikk-runden, der hele `ServerFire` skrives om likevel.

## Filer
- `Source/Gloshaugen/Public/GSCharacter.h`, `Private/GSCharacter.cpp` – nesten alt.
- `Source/Gloshaugen/Private/GSHUD.cpp` – skadetekst ved helsestolpen, tre ting fjernes.
- `IMPROVEMENTS.md` – linje 360–375 under ny overskrift.

Ikke rør `GSTypes.h`, `GSItems.*`, `catalog.py`, `GSGameMode.*`, `GSProjectile.*`, `GSEffects.*`, `GSAudio.*`.
Stage kun filene over, med navn (aldri `git add -A` / `git add .`).

## Steg

### 0. Forberedelse
1. `git log --oneline -3`: runde 3 («Blood: …») skal være committet. Er den ikke det, vent. Les `TakeDamage`,
   `OnRep_Dead`, `Die` og `DoMeleeHit` på nytt – runde 3 har lagt til `bIncomingMelee`, `MulticastWoundFX`,
   `ApplyDeathImpulse` og `SpawnDeathPool`. Ingenting av det skal flyttes eller endres her.
2. `Get-Process UnrealEditor` – kjører editoren: ikke bygg (be om Ctrl+Alt+F11 eller lukking).
3. Gjør alle endringer i `GSCharacter.h` ferdig i ett (andre lag bygger samtidig).

### A. Header (`GSCharacter.h`)
4. Rett under `namespace GSHurtKind { … }`:
   ```cpp
   /** Which part of the body a hit landed on. */
   namespace GSZone
   {
   	constexpr uint8 None = 0;      // no single point: blast, hunger
   	constexpr uint8 Head = 1;
   	constexpr uint8 Chest = 2;
   	constexpr uint8 Stomach = 3;   // belly and pelvis
   	constexpr uint8 Arm = 4;
   	constexpr uint8 Leg = 5;
   }
   ```
5. Forward-deklarasjon øverst: `class UBoxComponent;`
6. Public, ved de andre replikerte feltene (etter `bFlashlightOn`):
   ```cpp
   /** 0 = fine, 1 = badly shot. Leg: slower, lower jumps, no sprint, limp. Arm: unsteady aim, more recoil, slow reload. */
   UPROPERTY(Replicated, BlueprintReadOnly) float LegInjury = 0.f;
   UPROPERTY(Replicated, BlueprintReadOnly) float ArmInjury = 0.f;
   /** The injury as it is felt right now (painkillers halve it). */
   float GetLegPain() const;
   float GetArmPain() const;
   virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
   virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
   ```
7. Protected, etter `HeadMesh`:
   ```cpp
   /** Hit zones with the size of real body parts: head, chest, stomach, left/right arm, left/right leg. */
   UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> HitZones[7];
   void ApplyHitZoneLayout(bool bCrouch);
   uint8 ZoneOf(const UPrimitiveComponent* Comp) const;
   /** Keeps the body others see standing on the ground (also while crouched and for network smoothing). */
   void FixBodyMeshOffset(float HalfHeightAdjust);
   float ReloadSeconds(const FGSWeaponStats& S) const;
   ```
8. Ved de andre `Server`-RPC-ene: `UFUNCTION(Server, Reliable) void ServerSetAiming(bool bNew);`
9. Endre `MulticastShotFX`: siste parameter `bool bSilent` → `uint8 Flags` (bit 1 = lyddempet, bit 2 = uten lysspor):
   `UFUNCTION(NetMulticast, Unreliable) void MulticastShotFX(FVector_NetQuantize Start, FVector_NetQuantize End, uint8 Flags);`
10. `bool bAiming = false;` (private felt) → `UPROPERTY(Replicated) bool bAiming = false;` (samme sted).
11. Nye private felt (ved `ServerLastFireTime`):
    ```cpp
    // Server: shots the weapon is allowed to fire right now (refills at the weapon's own rate)
    float ShotCredit = 1.f;
    float ShotCreditTime = -100.f;
    float TPAim = 0.f;        // others see the gun come up to the eye when you aim
    float LimpPhase = 0.f;    // local: the camera limps in step when a leg is hurt
    ```

### B. Treffsonene (`GSCharacter.cpp`)
12. Nye includes: `#include "Components/BoxComponent.h"`, `#include "DrawDebugHelpers.h"`, `#include "HAL/IConsoleManager.h"`.
13. I det anonyme navnerommet øverst (etter `FxPoison`):
    ```cpp
    const FName FxPainkillers(TEXT("Smertestillende"));

    /** One hit box: where it sits on the capsule (cm from the capsule centre, X forward, Y right) and half its size,
     *  standing and crouched. Standing capsule: feet at -92, crouched: feet at -58. */
    struct FGSZoneBox { uint8 Zone; FVector Stand; FVector StandExt; FVector Crouch; FVector CrouchExt; };
    const FGSZoneBox GSZoneBoxes[7] = {
    	{ GSZone::Head,    FVector(3.f, 0.f, 77.f),     FVector(9.5f, 8.f, 12.f),    FVector(6.f, 0.f, 62.f),     FVector(9.5f, 8.f, 12.f) },
    	{ GSZone::Chest,   FVector(0.f, 0.f, 49.f),     FVector(12.f, 18.f, 16.f),   FVector(3.f, 0.f, 35.f),     FVector(12.f, 18.f, 15.f) },
    	{ GSZone::Stomach, FVector(0.f, 0.f, 15.5f),    FVector(11.f, 16.5f, 17.5f), FVector(0.f, 0.f, 6.f),      FVector(12.f, 16.5f, 14.f) },
    	{ GSZone::Arm,     FVector(0.f, -24.5f, 28.f),  FVector(6.f, 5.5f, 32.f),    FVector(4.f, -24.5f, 22.f),  FVector(7.f, 5.5f, 26.f) },
    	{ GSZone::Arm,     FVector(0.f, 24.5f, 28.f),   FVector(6.f, 5.5f, 32.f),    FVector(4.f, 24.5f, 22.f),   FVector(7.f, 5.5f, 26.f) },
    	{ GSZone::Leg,     FVector(0.f, -9.5f, -47.f),  FVector(9.f, 8.5f, 45.f),    FVector(8.f, -9.5f, -33.f),  FVector(17.f, 8.5f, 25.f) },
    	{ GSZone::Leg,     FVector(0.f, 9.5f, -47.f),   FVector(9.f, 8.5f, 45.f),    FVector(8.f, 9.5f, -33.f),   FVector(17.f, 8.5f, 25.f) },
    };

    TAutoConsoleVariable<int32> CVarShowHitZones(TEXT("gs.ShowHitZones"), 0, TEXT("1 = draw the hit zone boxes of every player"));
    ```
    (Stående: bein −92…−2, mage −2…33, bryst 33…65, hode 65…89. Huk: bein −58…−8, mage −8…20, bryst 20…50,
    hode 50…74. Kameraet står på +66 i begge.)
14. Konstruktøren:
    - Endre lambdaen `SetupHittable` til å ta `UPrimitiveComponent* C` (innholdet er likt).
    - `SetupHittable(BodyMesh);` → `SetupVisual(BodyMesh);` og `SetupHittable(HeadMesh);` → `SetupVisual(HeadMesh);`
      (sylinderen og kula er nå bare reservekropp å se på når mannekengen mangler; kuler treffer boksene).
    - Rett etter `HeadMesh`-blokka:
      ```cpp
      // Hit zones: boxes the size of real body parts. Bullets, blades and arrows (ECC_Visibility) hit these.
      static const TCHAR* ZoneNames[7] = { TEXT("ZoneHead"), TEXT("ZoneChest"), TEXT("ZoneStomach"), TEXT("ZoneArmL"),
      	TEXT("ZoneArmR"), TEXT("ZoneLegL"), TEXT("ZoneLegR") };
      for (int32 I = 0; I < 7; ++I)
      {
      	UBoxComponent* B = CreateDefaultSubobject<UBoxComponent>(ZoneNames[I]);
      	B->SetupAttachment(GetCapsuleComponent());
      	SetupHittable(B);
      	B->SetCanEverAffectNavigation(false);
      	HitZones[I] = B;
      }
      ApplyHitZoneLayout(false);
      ```
    - Rett kommentaren over kapselen til `// Bullets hit the zone boxes, not the capsule.`
15. Nye funksjoner (legg dem rett før `SetupHumanBody`):
    ```cpp
    void AGSCharacter::ApplyHitZoneLayout(bool bCrouch)
    {
    	for (int32 I = 0; I < 7; ++I)
    	{
    		if (!HitZones[I]) continue;
    		const FGSZoneBox& Z = GSZoneBoxes[I];
    		HitZones[I]->SetRelativeLocation(bCrouch ? Z.Crouch : Z.Stand);
    		HitZones[I]->SetBoxExtent(bCrouch ? Z.CrouchExt : Z.StandExt, false);
    	}
    }

    uint8 AGSCharacter::ZoneOf(const UPrimitiveComponent* Comp) const
    {
    	for (int32 I = 0; I < 7; ++I)
    	{
    		if (Comp && HitZones[I] == Comp) return GSZoneBoxes[I].Zone;
    	}
    	return GSZone::None;
    }

    void AGSCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
    {
    	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
    	ApplyHitZoneLayout(true);
    	FixBodyMeshOffset(HalfHeightAdjust);
    }

    void AGSCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
    {
    	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
    	ApplyHitZoneLayout(false);
    	FixBodyMeshOffset(0.f);
    }

    float AGSCharacter::GetLegPain() const
    {
    	const bool bNumb = Effects.ContainsByPredicate([](const FGSActiveEffect& E) { return E.Id == FxPainkillers; });
    	return LegInjury * (bNumb ? 0.5f : 1.f);
    }

    float AGSCharacter::GetArmPain() const
    {
    	const bool bNumb = Effects.ContainsByPredicate([](const FGSActiveEffect& E) { return E.Id == FxPainkillers; });
    	return ArmInjury * (bNumb ? 0.5f : 1.f);
    }

    float AGSCharacter::ReloadSeconds(const FGSWeaponStats& S) const
    {
    	return S.ReloadTime * GetMods().Reload * (1.f + 0.6f * GetArmPain());   // a shot arm fumbles the magazine
    }
    ```
    `OnStartCrouch`/`OnEndCrouch` kalles av motoren på server, eier og hos alle andre (via `OnRep_IsCrouched`), så
    boksene er like overalt. Skli bruker `Crouch()` og får dermed huke-oppsettet.
16. `GetLifetimeReplicatedProps`: legg til
    ```cpp
    DOREPLIFETIME(AGSCharacter, LegInjury);
    DOREPLIFETIME(AGSCharacter, ArmInjury);
    DOREPLIFETIME_CONDITION(AGSCharacter, bAiming, COND_SkipOwner);
    ```
17. `OnRep_Dead`: der `HeadMesh`/`BodyMesh` får `NoCollision`, legg til
    `for (UBoxComponent* Z : HitZones) { if (Z) Z->SetCollisionEnabled(ECollisionEnabled::NoCollision); }`
    (de to gamle linjene kan stå; de er harmløse).
18. `Tick`, helt nederst (utenfor `IsLocallyControlled`-blokka):
    ```cpp
    #if !UE_BUILD_SHIPPING
    	if (CVarShowHitZones.GetValueOnGameThread() != 0 && !bDead && GetNetMode() != NM_DedicatedServer)
    	{
    		static const FColor ZoneColors[6] = { FColor::White, FColor::Red, FColor::Orange, FColor::Yellow, FColor::Cyan, FColor::Green };
    		for (int32 I = 0; I < 7; ++I)
    		{
    			if (const UBoxComponent* Z = HitZones[I])
    			{
    				DrawDebugBox(GetWorld(), Z->GetComponentLocation(), Z->GetScaledBoxExtent(), Z->GetComponentQuat(),
    					ZoneColors[GSZoneBoxes[I].Zone], false, -1.f, 0, 0.6f);
    			}
    		}
    	}
    #endif
    ```

### C. Kroppen andre ser står på bakken (`SetupHumanBody` + ny `FixBodyMeshOffset`)
Funnet ved lesing, ikke sett i spillet: `SetupHumanBody` flytter skjelettet ned til føttene i `BeginPlay`, men
motoren har alt lagret «utgangsposisjonen» (0,0,0) i `PostInitializeComponents`. To følger: (1) hos klienter
flytter nettverksutjevningen andre spilleres kropp tilbake til utgangsposisjonen (midt i kapselen = svever
92 cm over bakken); (2) `ACharacter::OnStartCrouch/OnEndCrouch` setter skjelettets høyde ut fra klassens
standard (0), så kroppen hopper opp når noen huker og blir stående der. Uten rettelsen gir ikke treffsonene mening.
19. Ny funksjon (ved de andre i steg 15):
    ```cpp
    void AGSCharacter::FixBodyMeshOffset(float HalfHeightAdjust)
    {
    	if (!bHumanBody || bDead) return;
    	const float StandHalf = GetDefault<AGSCharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
    	const FVector Loc(0.f, 0.f, -StandHalf + HalfHeightAdjust);   // feet on the floor (the capsule centre drops when crouched)
    	const FRotator Rot(0.f, -90.f, 0.f);
    	GetMesh()->SetRelativeLocationAndRotation(Loc, Rot);
    	CacheInitialMeshOffset(Loc, Rot);   // the engine's network smoothing and crouch code start from this
    }
    ```
20. `SetupHumanBody`: rett etter `bHumanBody = true;`:
    `FixBodyMeshOffset(bIsCrouched ? GetDefault<AGSCharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() : 0.f);`
    (De to eksisterende `SetRelativeLocation`/`SetRelativeRotation`-linjene kan stå.)
    `CacheInitialMeshOffset` er en offentlig `ACharacter`-funksjon. Finnes den ikke under det navnet i UE 5.8: sett
    `BaseTranslationOffset = Loc; BaseRotationOffset = Rot.Quaternion();` direkte (beskyttede felt i `ACharacter`),
    og skriv i rapporten hva du brukte.

### D. Skade etter sone (`TakeDamage`, `CompleteUse`)
21. `TakeDamage`, øverst ved `bool bHeadshot = false;`: `uint8 Zone = GSZone::None; bool bFace = false;`
22. I `if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))`-blokka: bytt **bare** betingelsen
    `if (PE.HitInfo.GetComponent() == HeadMesh)` med `if (Zone == GSZone::Head)`, og legg dette rett over den
    (etter `PE`-linja og etter runde 3 sine `WoundPoint`-linjer):
    ```cpp
    Zone = ZoneOf(PE.HitInfo.GetComponent());
    if (Zone == GSZone::None) Zone = GSZone::Chest;   // a point hit that found no zone box counts as the torso
    if (Zone == GSZone::Head)
    {
    	// The helmet covers the skull, not the face: the lower part of the head, hit from the front, goes past it.
    	const float LocalZ = PE.HitInfo.GetComponent()->GetComponentTransform().InverseTransformPosition(PE.HitInfo.ImpactPoint).Z;
    	bFace = LocalZ < -2.5f && FVector::DotProduct(FVector(PE.ShotDirection), GetActorForwardVector()) < -0.2f;
    }
    ```
23. Rett **før** `Damage *= GetMods().Taken;`:
    ```cpp
    // Arms and legs are seldom deadly on their own; the belly a little less than the chest.
    if (Zone == GSZone::Arm) Damage *= 0.55f;
    else if (Zone == GSZone::Leg) Damage *= 0.65f;
    else if (Zone == GSZone::Stomach) Damage *= 0.9f;
    ```
24. Rustning dekker bare det den dekker. Rett før `const bool bArmored = …`:
    ```cpp
    const bool bHelmetCovers = bHeadshot && !bFace;
    const bool bVestCovers = !bHeadshot && Zone != GSZone::Arm && Zone != GSZone::Leg;   // torso, and blasts
    ```
    - `const bool bArmored = bHeadshot ? !Helmet.IsNone() : !BodyArmor.IsNone();` →
      `const bool bArmored = (bHelmetCovers && !Helmet.IsNone()) || (bVestCovers && !BodyArmor.IsNone());`
    - `if (bHeadshot && !Helmet.IsNone())` → `if (bHelmetCovers && !Helmet.IsNone())`
    - `else if (!bHeadshot && !BodyArmor.IsNone())` → `else if (bVestCovers && !BodyArmor.IsNone())`
25. Rett etter `Health = FMath::Max(0.f, Health - Damage);`:
    ```cpp
    // A hit that draws blood from a limb leaves it hurt until it is treated.
    if (Health > 0.f && HealthBefore > Health)
    {
    	if (Zone == GSZone::Leg) LegInjury = FMath::Min(1.f, LegInjury + (HealthBefore - Health) / 45.f);
    	else if (Zone == GSZone::Arm) ArmInjury = FMath::Min(1.f, ArmInjury + (HealthBefore - Health) / 40.f);
    }
    ```
26. `ClientTookHit`-kallet i `TakeDamage`: siste argument `Kind` → `static_cast<uint8>(Kind | (Zone << 4))`
    (sonen i de fire øverste bitene; signaturen endres ikke).
27. `ClientTookHit_Implementation`, helt øverst:
    ```cpp
    const uint8 Zone = Kind >> 4;   // GSZone in the high bits
    Kind &= 0x0F;
    ```
    - `const bool bHead = Kind == GSHurtKind::Head || Kind == GSHurtKind::HeadArmor;` →
      `const bool bHead = Zone == GSZone::Head || Kind == GSHurtKind::Head || Kind == GSHurtKind::HeadArmor;`
      (hodeskudd som knuser hjelmen får nå rykket – kontrollørens funn 3, runde 2).
    - Rett etter `if (bHead) { … }`-blokka (før lyd-delen):
      ```cpp
      // You feel where it hit: the knee gives way, or the gun is knocked in your hands.
      if (HealthLost > 0.f && Zone == GSZone::Leg) CamDipVel -= 90.f + 140.f * FMath::Clamp(HealthLost / 30.f, 0.f, 1.f);
      if (HealthLost > 0.f && Zone == GSZone::Arm)
      {
      	const FGSWeaponSlot* W = GetCurrentWeapon();
      	AddWeaponKick(2.5f, W ? GetStats(*W).Class : EGSWeaponClass::Rifle);
      }
      ```
    - I lyd-`switch`en, `case GSHurtKind::ArmorBroke:`: legg til
      `if (Zone == GSZone::Head) GSAudio::Play2D(this, TEXT("Ear_Ring"), 0.4f, 1.f);`
28. `CompleteUse`, i `case EGSItemCat::Medicine:`-grenen (felles med Food/Drug), rett etter `Health = …`-linja:
    ```cpp
    if (Def->Cat == EGSItemCat::Medicine && Def->Heal > 0.f)
    {
    	// Dressing the wound: a plaster helps a little, a bandage a lot, a first-aid kit sets the limb right.
    	const float Treat = Def->Heal / 60.f;
    	LegInjury = FMath::Max(0.f, LegInjury - Treat);
    	ArmInjury = FMath::Max(0.f, ArmInjury - Treat);
    }
    ```
    (Plaster 15 → 0,25. Bandasje 30 → 0,5. Førstehjelpsskrin 75 og feltkirurgsett 100 → helt bra.)

### E. Følgene av skadene
29. `Tick`: bytt de tre linjene fra `float Speed = bSprinting ? …` til `Move->JumpZVelocity = …` med:
    ```cpp
    const float LegPain = bDead ? 0.f : GetLegPain();
    // A badly shot leg cannot run. Server and owner both apply it (bSprinting is predicted by the owner).
    if (bSprinting && LegPain >= 0.5f && (HasAuthority() || IsLocallyControlled())) SetSprinting(false);
    float Speed = bSprinting ? SprintSpeed : (bAiming ? AimSpeed : WalkSpeed);
    if (!UseItemId.IsNone()) Speed = FMath::Min(Speed, AimSpeed);
    Move->MaxWalkSpeed = Speed * Mods.Speed * GearSpeed * (1.f - 0.45f * LegPain);
    Move->JumpZVelocity = 430.f * Mods.Jump * (1.f - 0.4f * LegPain);
    ```
30. `StartSprint`: betingelsen får `&& GetLegPain() < 0.5f`. `ServerSetSprinting_Implementation`:
    `bSprinting = bNew && Stamina > 1.f && !bIsCrouched && GetLegPain() < 0.5f;`
31. `UpdateLocalVisuals` – halting. Rett før `Camera->ClearAdditiveOffset();`:
    ```cpp
    // A hurt leg: the view drops and tips on every second step.
    const float LimpAmt = bDead || bFalling ? 0.f : GetLegPain() * FMath::Clamp(Vel.Size2D() / 200.f, 0.f, 1.f);
    LimpPhase = FMath::Frac(LimpPhase + Vel.Size2D() * Dt / 350.f);   // one cycle = two steps
    const float Limp = LimpAmt * FMath::Max(0.f, FMath::Sin(LimpPhase * 2.f * PI));
    ```
    og i `CamRot`/`AddAdditiveOffset`: rull `CamLean + Flinch * FlinchSign * 3.f` → `… + Limp * 2.2f`, og
    `FVector(0.f, 0.f, CamDip)` → `FVector(0.f, 0.f, CamDip - Limp * 7.f)`.
32. `TickScopeSway` – armskade. Etter `if (bScoped) { … }`-blokka og før `ScopeSwayAmp = FMath::FInterpTo(…)`:
    ```cpp
    // A shot arm cannot hold the gun still: the sights wander and tremble with any weapon. Holding your breath does not help.
    const float ArmPain = bDead ? 0.f : GetArmPain();
    const float Shake = bAiming ? 0.07f * ArmPain : 0.f;
    if (bAiming) Target += 0.5f * ArmPain;
    ```
    og bytt `Off`-linja med:
    ```cpp
    const FVector2D Off(FMath::Sin(T * 0.9f) * ScopeSwayAmp + FMath::Sin(T * 6.3f) * Shake,
    	FMath::Sin(T * 1.7f + 0.8f) * ScopeSwayAmp * 0.6f + FMath::Sin(T * 7.9f + 1.3f) * Shake);
    ```
33. `GetCurrentSpread`: rett før `return`: `if (!bAiming) Spread += 1.5f * GetArmPain();`
34. `FireOnce`: `const float Recoil = S.RecoilPitch * Mods.Recoil;` → `… * Mods.Recoil * (1.f + 0.5f * GetArmPain());`
35. Lading: i `ServerReload_Implementation` byttes `S.ReloadTime * GetMods().Reload` med `ReloadSeconds(S)`, og i
    `UpdateWeaponAnimation` byttes `ReloadLength = W ? S.ReloadTime * GetMods().Reload : 2.f;` med
    `ReloadLength = W ? ReloadSeconds(S) : 2.f;` (ellers er animasjonen ferdig før serveren).

### F. Juksehull og feil fra `funn.md`
36. **V1 – sikting til serveren.**
    ```cpp
    void AGSCharacter::StartAim()
    {
    	if (TerminalPC() || bDead) return;
    	bAiming = true;
    	SetSprinting(false);
    	if (!HasAuthority()) ServerSetAiming(true);
    }

    void AGSCharacter::StopAim()
    {
    	bAiming = false;
    	if (!HasAuthority()) ServerSetAiming(false);
    	if (bWantsSprint) StartSprint();
    }

    void AGSCharacter::ServerSetAiming_Implementation(bool bNew)
    {
    	bAiming = bNew && !bDead;
    	if (bAiming) bSprinting = false;
    }
    ```
    `Die`: legg til `bAiming = false;` ved `StopFire();`. Sjekk med `grep -n "bAiming = "` at dette er de eneste
    stedene feltet settes (lærdom runde 2: alle som setter verdien må også sende den).
    Dermed bruker serveren siktefart i `Tick` og siktespredning for hagl i `ServerFire` – ingen endring trengs der.
    **Andre ser at du sikter** – i `Tick`, blokka «Other players' weapons», før `const FVector Shoulder`:
    `TPAim = FMath::FInterpTo(TPAim, bAiming && !bSprinting && !bReloading ? 1.f : 0.f, DeltaSeconds, 9.f);`
    og `const FVector Shoulder(5.f, 18.f, bIsCrouched ? 10.f : 48.f);` →
    `const FVector Shoulder(5.f + 4.f * TPAim, 18.f - 9.f * TPAim, (bIsCrouched ? 10.f : 48.f) + 9.f * TPAim);`
37. **V3 – skuddtakt.** `ServerFire_Implementation`: ta `|| Now - ServerLastFireTime < S.FireInterval * 0.7f` ut av
    den første `if`-en, og legg rett etter avstandssjekken (`DistSquared … return;`), før `ServerLastFireTime = Now;`:
    ```cpp
    // Shot account: one shot is earned per FireInterval (5 % slack, the same the client allows itself). Up to
    // 0.15 s worth may be saved up, so shots that arrive bunched together after a network hiccup still count,
    // but the average rate can never beat the weapon's own.
    const float MaxCredit = FMath::Clamp(0.15f / FMath::Max(S.FireInterval, 0.01f), 1.1f, 3.f);
    ShotCredit = FMath::Min(MaxCredit, ShotCredit + (Now - ShotCreditTime) / FMath::Max(S.FireInterval * 0.95f, 0.01f));
    ShotCreditTime = Now;
    if (ShotCredit < 1.f) return;
    ShotCredit -= 1.f;
    ```
    `ServerSelectSlot_Implementation`: legg til `ShotCredit = FMath::Min(ShotCredit, 1.f);` (oppspart ild fra et
    raskt våpen følger ikke med til et tregt). `ServerMelee` røres ikke.
38. **V5 – rakettkaster er ikke lyddempet.**
    - `ServerFire`, prosjektilgrenen: `if (!S.bSilent) MulticastShotFX(Start, FVector(Start) + D * 200.f, true);` →
      `MulticastShotFX(Start, FVector(Start) + D * 200.f, static_cast<uint8>(2 | (S.bSilent || S.Class == EGSWeaponClass::Bow ? 1 : 0)));`
    - Nederst i `ServerFire` (vanlig skudd): siste argument → `static_cast<uint8>(S.bSilent || Mods.bSilent ? 1 : 0)`.
    - `MulticastShotFX_Implementation(…, uint8 Flags)`: øverst
      `const bool bSilent = (Flags & 1) != 0; const bool bNoTracer = (Flags & 2) != 0;`
      `if (!bSilent && !bNoTracer) AGSShortFX::SpawnTracer(…);` og legg `if (bNoTracer) return;` rett etter lysspor-linja
      (prosjektilet lager sitt eget nedslag). Resten uendret: lyd og radio følger `bSilent`, så Carl Gustaf, M72 og
      M79 gir `Shot_Launcher` på 450 m og vises på walkie-talkien; armbrøst er fortsatt stille.
    - `grep -n "MulticastShotFX("` – alle kall skal ha `uint8`.

### G. HUD (`GSHUD.cpp`)
39. `DrawBars`: rett etter `Box(X - 8.f, Y - 30.f, W + 16.f, 150.f, …);`:
    ```cpp
    // Hurt limbs, small and plain, above the bars
    {
    	float IY = Y - 52.f;
    	const FLinearColor Col(0.85f, 0.45f, 0.4f, 0.85f);
    	if (C->ArmInjury > 0.05f) { Text(C->ArmInjury > 0.5f ? TEXT("Armen er hardt skadd") : TEXT("Armen er skadd"), X, IY, Col, 0.75f); IY -= 18.f; }
    	if (C->LegInjury > 0.05f) { Text(C->LegInjury >= 0.5f ? TEXT("Beinet er hardt skadd - du kan ikke løpe") : TEXT("Beinet er skadd"), X, IY, Col, 0.75f); }
    }
    ```
40. `DrawHurtOverlay`: fjern `if (Low > 0.6f) { Text(TEXT("LAVT LIV - finn dekning og heal"), …); }`, og fjern blokka
    «Headshot you survived: a quick white flash» (`Box(0.f, 0.f, SX, SY, FLinearColor(1.f, 1.f, 1.f, …))`).
    Øresus, hardt rykk og blodkant står igjen.
41. `DrawHitMarker`: fjern `if (Kind == GSHitKind::Kill && C->WasLastKillHeadshot()) { Text(TEXT("HODESKUDD"), …); }`.
    (Dødskortet til offeret viser fortsatt hodeskudd. `WasLastKillHeadshot()` kan stå.)

### H. Bygg og logg
42. Bygg med kommandoen i `CLAUDE.md` (`-WaitMutex`, editoren lukket). Feiler bygget i en fil laget ikke eier:
    vent og prøv igjen (inntil 4 ganger), ikke rett den.
43. `IMPROVEMENTS.md`: ny overskrift øverst med linjene under.
44. Commit på grenen: `Combat: body hit zones with limb injuries, aiming sent to server, server-side fire rate, launcher sound`.

## Ferdig når
- `Build.bat` går gjennom uten feil.
- Kode-sjekk: `HeadMesh`/`BodyMesh` har ikke lenger kollisjon; `TakeDamage` bruker `ZoneOf`; ingen steder
  sammenligner treff med `HeadMesh`.
- Kode-sjekk: `LegInjury`/`ArmInjury` settes bare på serveren (`TakeDamage`, `CompleteUse`) og er replikert.
- Kode-sjekk: `bAiming` settes bare i `StartAim`, `StopAim`, `ServerSetAiming_Implementation` og `Die`.
- Kode-sjekk: `ServerFire` har ingen `* 0.7f` igjen; `MulticastShotFX` har `uint8 Flags` alle steder.
- Runde 3 sin kode i `TakeDamage` (`MulticastWoundFX`, `WoundPoint`) står urørt og kjøres fortsatt før `Die`.
- I spillet (Andreas, «Number of Players: 2», Net Mode «Play As Listen Server», etter kampstart):
  - Skriv `gs.ShowHitZones 1` i konsollen (tasten `|` / `~`): sju fargede bokser på den andre spilleren som
    dekker hode, bryst, mage, armer og bein, og som krymper sammen når han huker.
  - Skyt den andre i beinet: han dør ikke av ett pistolskudd, men går saktere, bildet hans halter, og etter et
    par treff kan han ikke løpe. Teksten «Beinet er skadd» står over helsestolpen. Bandasje/førstehjelp retter det.
  - Skyt ham i armen: siktet hans vandrer når han sikter, og lading tar lengre tid.
  - Med vest: skudd i bryst gir blågrå kant (vesten tar det), skudd i bein gjør full beinskade.
  - I klientvinduet: gå og sikt samtidig – ingen rykking. Verten ser våpenet ditt løftes til øyet.
  - Den andre skyter en rakettkaster: du hører et kraftig smell, ikke et dempet «pfft».
  - Kroppen til den andre spilleren står med føttene på bakken i begge vinduer, også etter at han har huket.

## Risiko
- **`TakeDamage` endres to runder på rad.** Stegene her er innskudd og tre byttede betingelser; ingenting fra
  runde 2/3 flyttes. Har kontrolløren sendt runde 3 tilbake med rettinger der, les funksjonen på nytt først.
- **Boksene følger ikke animasjonen** (løpende bein, svingende armer): avvik på inntil ca. 20 cm for armer og
  bein. Hode og overkropp stemmer godt. `gs.ShowHitZones` viser det ærlig. Mannekengen har ingen huke-animasjon,
  så en hukende spiller ser oppreist ut for andre mens boksene er lave – det er en gammel svakhet som nå blir
  synlig med feilsøkingsboksene; rettes når kroppen får ordentlige animasjoner (se `retning.md`).
- **Steg C bygger på lesing av motorkode, ikke på noe sett i spillet.** Er antakelsen feil, er rettelsen likevel
  ufarlig: den setter bare posisjonen `SetupHumanBody` allerede setter, og forteller motoren om den. Sjekk i
  to-spiller-testen. Ragdollen røres ikke (`bDead`-vakt).
- **Balanse med dagens våpendata:** arm/bein-faktorene (0,55/0,65) gjør at lemmer tåler mer. Skadetallene eies av
  lag Spillbarhet (bestilling R5–R7 ligger inne); faktorene her er forholdstall og virker uansett hva tallene blir.
- **Sprint-forutsigelse:** eieren slår selv av sprint når `LegInjury` kommer over nettet; i de få millisekundene
  før kan serveren rette posisjonen én gang. Akseptabelt.
- **Skuddkonto (V3):** klokkeforskjell/jitter kan gi at et enkelt skudd avvises ved helautomatisk ild over tid;
  5 % slakk og oppsparing på 0,15 s er satt for å unngå det. Avvist skudd rettes av ammo-replikeringen (som i dag).
- **`UPROPERTY` på C-tabell** (`HitZones[7]`): støttes av UHT med tall som størrelse. Klager den, bruk sju
  enkeltfelt eller `TArray<TObjectPtr<UBoxComponent>>` og skriv det i rapporten.
- **Andre spor på `ECC_Visibility`** (blodspor fra runde 3, nærkamp-sveip, piler, fokus på loot): treffer nå
  boksene i stedet for sylinder/kule. Runde 3 sine blodspor hopper over alle `APawn`, så de påvirkes ikke.
- **Ytelse:** 7 enkle bokser per spiller (140 ved 20 spillere) erstatter to trekant-mesher med kompleks kollisjon – billigere enn før.

## IMPROVEMENTS-linjer
```
## Runde 9 (Kamp): kroppssoner (26. nov.)

360. Kroppen har sju treffsoner med mål som en ekte kropp (hode, bryst, mage, to armer, to bein) i stedet for én sylinder og en kule
361. Hodet er like stort som et ekte hode; før var treffområdet nesten dobbelt så stort
362. Skudd i arm eller bein gjør mindre skade enn skudd i brystet, og mageskudd litt mindre
363. Vesten beskytter bare overkroppen, ikke armer og bein
364. Hjelmen beskytter skallen, ikke ansiktet når skuddet kommer forfra
365. Skudd i beinet: du går saktere, hopper lavere, halter i bildet og kan ikke løpe når skaden er stor
366. Skudd i armen: siktet vandrer og skjelver, rekylen blir verre og lading tar lengre tid
367. Plaster, bandasje og førstehjelpsskrin behandler skadde armer og bein, og smertestillende demper følgene mens de virker
368. Du kjenner hvor du ble truffet: kneet svikter ved beinskudd, våpenet rykker i hendene ved armskudd, og en liten tekst over helsa sier hva som er skadd
369. Treffsonene følger med når du huker eller sklir
370. Hodeskudd som knuser hjelmen gir fortsatt øresus og hardt rykk
371. Sikting sendes til serveren: slutt på rykking når du går og sikter online, haglene treffer der du ser dem, og andre ser at du løfter våpenet til øyet
372. Serveren godtar ikke lenger raskere skuddtakt enn våpenet har (før opptil 43 % for fort)
373. Rakettkastere og granatkasteren smeller høyt for alle og vises på walkie-talkien, i stedet for å høres lyddempet ut
374. Kroppen andre ser står med føttene på bakken hos alle spillere, også etter huking
375. Mindre arkade i bildet: «LAVT LIV»-teksten, det hvite blinket ved hodeskudd og «HODESKUDD» under siktet er fjernet
```
