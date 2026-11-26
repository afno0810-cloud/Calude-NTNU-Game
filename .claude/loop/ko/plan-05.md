# Plan – runde 5 (lag Kamp): ballistikk del 1. Kula har fart, faller og mister kraft

Bygges oppå rettingene i `.claude/loop/rettinger.md` og runde 4 («kroppssoner», `ko/plan-04.md`). Planen er
skrevet mot koden i commit `aaaab9d` pluss det rettingene og plan-04 legger til. **Linjenumrene flytter seg –
bruk funksjonsnavn og tekstene som er sitert.** Følger `prioriteringer.md` (Kamp punkt 3: ballistikk, V2) og
Andreas' godkjente forslag «kulefall» (del 1). Lagfeltene for CTF er ikke levert; denne runden trenger dem ikke.

## Mål
Kuler flyr med ekte fart og faller med tyngdekraften: treffet i veggen kommer litt etter smellet, på lang
avstand må du holde over målet, skaden faller med avstanden, og kuler som går tett forbi hodet ditt smeller.

## Forbedrer (gammelt)
- **`ServerFire`** (funn V2, V7, R1, R2, funn 9): skrives om. Serveren legger på spredningen selv (en juksende
  klient kan ikke lenger skyte uten spredning), avviser skudd i en retning spilleren ikke ser, starter ikke
  skudd på andre siden av en vegg, og RPC-en blir upålitelig (MG3 med pakketap kan ikke lenger koble deg fra).
- **Lysspor** (funn R12): vanlige kuler er usynlige; bare lysspor-ammo og hver femte kule fra maskingevær
  lyser, og sporet flyr med kula i stedet for å være en strek som er der med en gang. Lysspor-ammo gir ikke
  lenger bedre treff.
- **Hagler**: hver hagl er en egen liten kule som bremses fort.
- **Kikkertsiktet**: prikkene under midten byttes med holdemerker for 200, 300, 400 m … regnet ut fra patronen.
- **Drapsloggen og hodeskudd-faktoren** bruker våpenet kula ble skutt med, ikke det skytteren holder når den
  treffer (han kan ha byttet våpen eller være død).
- **Rakett/granat/pil** starter ikke lenger på andre siden av en vegg (funn 9, delen i `ServerFire`).

## Nytt
- **Kuler som ekte prosjektiler** på serveren (ny fil `GSBallistics.*`): munningsfart, luftmotstand og
  tyngdekraft per kaliber, stegvis linjespor hvert 1/60 s, lever til de treffer noe (maks 3 s).
- **Skadefall over avstand** etter farten kula har igjen.
- **Innskyting**: siktene møter kula på 25 m (pistolkaliber), 50 m (5,7 mm, muskett) eller 100 m (rifler).
- **Overlydssmell og sus** når en annen spillers kule passerer innen ca. 3 m av hodet ditt (to nye lyder).
- **Holdemerker i kikkerten** (som på et ekte kampsikte).
- Feilsøkingskommando **`gs.ShowBullets 1`**: tegner kulebanene (server) så Andreas og kontrolløren ser buen.
- Reservetabell for kulefart per kaliber, klar til å byttes med lag Spillbarhets våpendata når de kommer.

## Hvorfor nå
- Punkt 3 i hovedagentens prioriteringer, og nest øverst i backlogen. Punkt 1 (lag i kamp/CTF) venter på
  lagfeltene fra Spillbarhet, som ikke er levert.
- V1 (sikting til serveren) kommer i runde 4; da kan serveren regne spredning selv (V2).
- Grunnmur for ballistikk del 2 (gjennomslag), lyd på avstand (smell forsinket med 343 m/s) og vind.
- Alt ligger i lagets egne filer og en ny fil laget eier. Ingen `.uasset`-filer (lydene importeres automatisk).

## Avgrensning (bevisst)
- **Ingen gjennomslag** (del 2): kula stopper i første ting den treffer, som i dag.
- **Ingen forsinkelseskompensasjon (V8)**: med flytid må man uansett sikte foran løpende mål; ping legges oppå.
- **Ikke forsinket skuddlyd** (backlog punkt 8). Smellet når en kule passerer er med, fordi det hører til kula.
- **Ingen nye våpendata.** Kulefart og luftmotstand ligger i en reservetabell i `GSBallistics.cpp`. Når lag
  Spillbarhet leverer `MuzzleVelocity` (bestilling ligger inne), byttes én linje i `GSBallistics::ForWeapon`.
- Rakettkaster, granatkaster, armbrøst og bue bruker fortsatt `AGSProjectile` (de har alt flytid og fall).
- «Batching» av `ServerFire` trengs ikke: ett skudd er én liten upålitelig melding, og serveren tåler at de
  kommer i feil rekkefølge (skuddnummer). V7 er løst med `Unreliable` alene.

## Hensyn til lag (Capture the Flag)
All skade fra kuler går fortsatt gjennom `AGSCharacter::TakeDamage` (med `FPointDamageEvent`), så lagsjekken
som kommer med lagfeltet stopper kuler som før. Ikke lag noen annen vei til skade.

## Filer
- **Nye:** `Source/Gloshaugen/Public/GSBallistics.h`, `Source/Gloshaugen/Private/GSBallistics.cpp` (lag Kamp eier dem).
- `Source/Gloshaugen/Public/GSCharacter.h`, `Private/GSCharacter.cpp` – skyting, `TakeDamage` (to små innskudd), `GetStats`.
- `Source/Gloshaugen/Private/GSHUD.cpp` – `DrawScope` (holdemerker).
- `Tools/Audio/gen_sounds.py` + to nye filer i `SourceArt/Sounds/` (`S_GS_Bullet_Crack.wav`, `S_GS_Bullet_Whiz.wav`).
- `IMPROVEMENTS.md` (linje 386–401), `.claude/lag/bestillinger.md` (én linje, steg 33).

Ikke rør `GSTypes.h`, `GSItems.*`, `catalog.py`, `GSProjectile.*`, `GSEffects.*`, `GSAudio.*`, `Gloshaugen.Build.cs`.
Stage kun filene over, med navn (aldri `git add -A` / `git add .`).

## Steg

### 0. Forberedelse
1. `git log --oneline -5`: rettingene og runde 4 («Combat: body hit zones …») skal være committet. Er runde 4 ikke
   det, vent. Les på nytt: `FireOnce`, `ServerFire_Implementation`, `MulticastShotFX_Implementation` (runde 4 har
   gitt den `uint8 Flags`: bit 1 = lyddempet, bit 2 = uten lysspor), `MulticastPelletFX_Implementation`,
   `DrawPellets`, `GetCurrentSpread`, `TakeDamage` og `GetStats`. Runde 4 har også lagt inn skuddkontoen
   (`ShotCredit`) og `* (1.f + 0.5f * GetArmPain())` i rekylen – begge skal stå.
2. `Get-Process UnrealEditor` – kjører editoren: ikke bygg (be om Ctrl+Alt+F11 eller lukking).
3. Gjør `GSCharacter.h` ferdig i ett (andre lag bygger samtidig). Lag `GSBallistics.h` før `.cpp`-filene.

### A. Ny fil `Public/GSBallistics.h`
4. ```cpp
   #pragma once

   #include "CoreMinimal.h"
   #include "Subsystems/WorldSubsystem.h"
   #include "GSBallistics.generated.h"

   class AGSCharacter;
   class AController;
   struct FGSWeaponStats;

   /** How a cartridge flies. */
   struct FGSBallistic
   {
   	float MuzzleVelocity = 400.f;  // m/s
   	float Drag = 0.0015f;          // 1/m: the speed falls as v0 * exp(-Drag * distance)
   	float ZeroM = 25.f;            // the sights meet the bullet at this distance
   };

   namespace GSBallistics
   {
   	/** Reserve table per caliber until the item catalog carries muzzle velocity (order to team Gameplay). */
   	GLOSHAUGEN_API FGSBallistic ForCaliber(FName Caliber);
   	GLOSHAUGEN_API FGSBallistic ForWeapon(const FGSWeaponStats& S);
   	/** Seconds to fly DistM metres along the bore. */
   	GLOSHAUGEN_API float TimeTo(const FGSBallistic& B, float DistM);
   	/** Radians the barrel points above the line of sight so the two meet at ZeroM. */
   	GLOSHAUGEN_API float ZeroAngle(const FGSBallistic& B);
   	/** Radians below the line of sight the bullet hits at DistM (positive = low). For scope hold marks. */
   	GLOSHAUGEN_API float HoldUnder(const FGSBallistic& B, float DistM);
   	/** Same number on server and owner: the owner predicts exactly the spread the server will use. */
   	GLOSHAUGEN_API int32 ShotSeed(int32 Key, int32 ShotIndex);
   	/** Where the bullet leaves the barrel: AimDir, a random point in a cone of SpreadDeg (full angle), lifted by ZeroRad. */
   	GLOSHAUGEN_API FVector ShotDirection(const FVector& AimDir, float SpreadDeg, int32 Seed, float ZeroRad);
   	/** Damage left in a bullet that has slowed from Muzzle to Speed (1 at the muzzle, never below 0.2). */
   	GLOSHAUGEN_API float DamageScale(float Speed, float Muzzle);
   }

   struct FGSBullet
   {
   	FVector Pos = FVector::ZeroVector;
   	FVector Vel = FVector::ZeroVector;   // cm/s
   	float Muzzle = 0.f;                  // cm/s
   	float DragCm = 0.f;                  // 1/cm
   	float Age = 0.f;
   	float Travelled = 0.f;               // cm
   	float Damage = 0.f;                  // at the muzzle
   	FName WeaponId;
   	uint8 AmmoType = 0;
   	TWeakObjectPtr<AGSCharacter> Shooter;
   	TWeakObjectPtr<AController> Instigator;
   	bool bReal = false;                  // server bullet that does damage; otherwise only shows
   	bool bTracer = false;
   	bool bPassedEar = false;
   };

   /** Every bullet in flight in this world. Server: real bullets. Each player's machine: bullets that only show. */
   UCLASS()
   class GLOSHAUGEN_API UGSBallistics : public UTickableWorldSubsystem
   {
   	GENERATED_BODY()
   public:
   	void FireReal(AGSCharacter* Shooter, const FVector& Start, const FVector& Dir, const FGSBallistic& B, float Damage, FName WeaponId, uint8 AmmoType);
   	void FireShown(const AGSCharacter* Shooter, const FVector& Start, const FVector& Dir, const FGSBallistic& B, bool bTracer);
   	virtual void Tick(float DeltaTime) override;
   	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGSBallistics, STATGROUP_Tickables); }
   protected:
   	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
   	{
   		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
   	}
   private:
   	TArray<FGSBullet> Bullets;
   	float LastCrackTime = -100.f;
   	/** Moves one bullet Dt seconds. Returns false when it is done (hit something, too slow, too old). */
   	bool Step(FGSBullet& B, float Dt);
   };
   ```

### B. Ny fil `Private/GSBallistics.cpp`
5. Includes: `GSBallistics.h`, `GSCharacter.h`, `GSItems.h`, `GSEffects.h`, `GSAudio.h`, `Engine/World.h`,
   `Engine/DamageEvents.h`, `GameFramework/PlayerController.h`, `Camera/PlayerCameraManager.h`,
   `DrawDebugHelpers.h`, `HAL/IConsoleManager.h`.
6. Reservetabellen (kilde: vanlige fabrikkpatroner; luftmotstanden er satt så farten på 100 m stemmer):
   ```cpp
   namespace
   {
   	struct FCalRow { const TCHAR* Caliber; float V0; float Drag; float ZeroM; };
   	const FCalRow CalTable[] = {
   		{ TEXT("9mm"),     360.f, 0.0015f,  25.f },   // 9x19: ca. 315 m/s at 100 m
   		{ TEXT("45acp"),   260.f, 0.0009f,  25.f },   // subsonic
   		{ TEXT("357"),     440.f, 0.0022f,  25.f },
   		{ TEXT("57"),      715.f, 0.0018f,  50.f },   // 5.7x28 (P90)
   		{ TEXT("556"),     940.f, 0.0009f, 100.f },
   		{ TEXT("762x39"),  730.f, 0.0013f, 100.f },
   		{ TEXT("762nato"), 840.f, 0.00074f, 100.f },
   		{ TEXT("762x54"),  830.f, 0.00075f, 100.f },
   		{ TEXT("50bmg"),   890.f, 0.00034f, 100.f },
   		{ TEXT("12g"),     400.f, 0.0064f,  25.f },   // one 00 buckshot pellet: slows fast
   		{ TEXT("ball"),    450.f, 0.0035f,  50.f },   // musket ball
   	};
   	constexpr float GravityCm = 980.f;
   	constexpr float MaxLife = 3.f;          // s (a .50 BMG goes ~2 km in that time)
   	constexpr float MinSpeedCm = 8000.f;    // 80 m/s: spent
   	constexpr float StepSeconds = 1.f / 60.f;
   	constexpr int32 MaxBullets = 600;       // oldest shown bullets go first
   	TAutoConsoleVariable<int32> CVarShowBullets(TEXT("gs.ShowBullets"), 0, TEXT("1 = draw the path of every real bullet (server)"));
   }
   ```
7. Funksjonene i `GSBallistics` (regnet i meter, sekunder og radianer):
   ```cpp
   FGSBallistic GSBallistics::ForCaliber(FName Caliber)
   {
   	for (const FCalRow& R : CalTable)
   	{
   		if (Caliber == FName(R.Caliber)) return FGSBallistic{ R.V0, R.Drag, R.ZeroM };
   	}
   	return FGSBallistic{ 400.f, 0.002f, 25.f };
   }

   FGSBallistic GSBallistics::ForWeapon(const FGSWeaponStats& S)
   {
   	FGSBallistic B = ForCaliber(S.Caliber);
   	// When the catalog carries it (order to team Gameplay): if (S.MuzzleVelocity > 0.f) B.MuzzleVelocity = S.MuzzleVelocity;
   	return B;
   }

   float GSBallistics::TimeTo(const FGSBallistic& B, float DistM)
   {
   	// v = v0 * exp(-k x)  =>  t = (exp(k x) - 1) / (k v0)
   	return B.Drag > 0.f ? (FMath::Exp(B.Drag * DistM) - 1.f) / (B.Drag * B.MuzzleVelocity) : DistM / B.MuzzleVelocity;
   }

   float GSBallistics::ZeroAngle(const FGSBallistic& B)
   {
   	const float T = TimeTo(B, B.ZeroM);
   	return FMath::Atan(0.5f * 9.81f * T * T / B.ZeroM);
   }

   float GSBallistics::HoldUnder(const FGSBallistic& B, float DistM)
   {
   	const float T = TimeTo(B, DistM);
   	const float Drop = 0.5f * 9.81f * T * T - FMath::Tan(ZeroAngle(B)) * DistM;
   	return FMath::Atan(Drop / DistM);
   }

   int32 GSBallistics::ShotSeed(int32 Key, int32 ShotIndex)
   {
   	return static_cast<int32>(HashCombine(GetTypeHash(Key), GetTypeHash(ShotIndex)));
   }

   FVector GSBallistics::ShotDirection(const FVector& AimDir, float SpreadDeg, int32 Seed, float ZeroRad)
   {
   	FRandomStream Rng(Seed);
   	const FVector D = Rng.VRandCone(AimDir, FMath::DegreesToRadians(FMath::Max(SpreadDeg, 0.f) * 0.5f));
   	// The barrel points a little above the line of sight, so bullet and sights meet at the zero distance.
   	const FVector Up = (FVector::UpVector - D * FVector::DotProduct(D, FVector::UpVector)).GetSafeNormal();
   	return Up.IsNearlyZero() ? D : (D + Up * FMath::Tan(ZeroRad)).GetSafeNormal();
   }

   float GSBallistics::DamageScale(float Speed, float Muzzle)
   {
   	const float R = Muzzle > 0.f ? FMath::Clamp(Speed / Muzzle, 0.f, 1.f) : 1.f;
   	return FMath::Clamp(0.2f + 0.8f * R * R, 0.2f, 1.f);   // energy left, but a slow bullet still hurts
   }
   ```
   (Eksempler som kontrolløren kan regne etter: 9 mm faller ca. 19 cm under siktepunktet på 80 m; 7,62 NATO
   ca. 56 cm på 300 m; skadefaktor for 9 mm på 100 m ≈ 0,76, for 7,62 NATO på 300 m ≈ 0,71.)
8. `FireReal` og `FireShown` legger en `FGSBullet` i `Bullets` med `Vel = Dir * B.MuzzleVelocity * 100.f`,
   `Muzzle = B.MuzzleVelocity * 100.f`, `DragCm = B.Drag / 100.f`. `FireReal` setter `bReal = true`, `Damage`,
   `WeaponId`, `AmmoType`, `Shooter` og `Instigator = Shooter ? Shooter->GetController() : nullptr`.
   `FireShown` returnerer med en gang på `NM_DedicatedServer`. Er `Bullets.Num() >= MaxBullets`: fjern den eldste
   *viste* kula (`!bReal`); finnes ingen, legg likevel til (ekte kuler skal aldri forsvinne).
9. `Tick(DeltaTime)`: for hver kule baklengs: del `FMath::Min(DeltaTime, 0.1f)` i steg på høyst `StepSeconds`, kall
   `Step`, og fjern kula (`RemoveAtSwap`) når den returnerer `false`.
10. `Step(FGSBullet& B, float Dt)`:
    ```cpp
    UWorld* World = GetWorld();
    const float Speed = B.Vel.Size();
    // Air drag (slows the bullet with the square of its speed) and gravity.
    const FVector NewVel = B.Vel + (FVector(0.f, 0.f, -GravityCm) - B.Vel * (B.DragCm * Speed)) * Dt;
    const FVector NewPos = B.Pos + (B.Vel + NewVel) * 0.5f * Dt;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBullet), true, B.Shooter.Get());
    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, B.Pos, NewPos, ECC_Visibility, Params);
    const FVector End = bHit ? FVector(Hit.ImpactPoint) : NewPos;
    ```
    Deretter, i denne rekkefølgen:
    - **Feilsøking** (bare `bReal`, ikke i Shipping): `if (CVarShowBullets…) DrawDebugLine(World, B.Pos, End, bHit ? FColor::Red : FColor::Yellow, false, 4.f, 0, 1.5f);`
    - **Vist kule:** lysspor `if (B.bTracer && B.Travelled > 300.f) AGSShortFX::SpawnTracer(World, B.Pos, End);`
      (eksisterende funksjon: tegner siste del av strekket i 0,05 s – med ett steg per bilde blir det en lysende
      strek som flyr). Overlydssmell: finn lokal spiller
      (`World->GetFirstPlayerController()`, `PC->PlayerCameraManager->GetCameraLocation()`). Er `!B.bPassedEar`,
      `PC->GetPawn() != B.Shooter.Get()`, og `FMath::PointDistToSegment(Ear, B.Pos, End) < 300.f`: sett
      `bPassedEar = true`, og hvis `World->GetTimeSeconds() - LastCrackTime > 0.05f` (hagl: ett smell, ikke ni):
      `LastCrackTime = …;` og `GSAudio::PlayAt(World, Speed > 34300.f ? TEXT("Bullet_Crack") : TEXT("Bullet_Whiz"),
      FMath::ClosestPointOnSegment(Ear, B.Pos, End), Speed > 34300.f ? 0.9f : 0.6f, 30.f, FMath::FRandRange(0.92f, 1.08f));`
      Langsomme kuler (under lydfarten) suser bare når de er nærmere enn 150 cm.
    - **Treff:** `if (bHit)`:
      - `bReal` og `AGSCharacter* Victim = Cast<AGSCharacter>(Hit.GetActor())`:
        ```cpp
        const float Dmg = B.Damage * GSBallistics::DamageScale(Speed, B.Muzzle);
        FPointDamageEvent Event(Dmg, Hit, B.Vel.GetSafeNormal(), nullptr);
        Victim->IncomingAmmoType = B.AmmoType;
        Victim->IncomingWeaponId = B.WeaponId;
        Victim->TakeDamage(Dmg, Event, B.Instigator.Get(), B.Shooter.Get());
        Victim->IncomingAmmoType = GSAmmoType::Normal;
        Victim->IncomingWeaponId = NAME_None;
        ```
        (Skytteren sendes som `DamageCauser` også om han er død: kroppen ligger i 90 s, og drapskortet skal vise
        der han sto. Drapet går til `Instigator`, samme mønster som `OwnerController` i `GSProjectile`.)
      - `!bReal` og ikke `AGSCharacter`: `AGSShortFX::SpawnImpact(World, Hit.ImpactPoint, Hit.ImpactNormal);`
        (blod ved treff i kropp kommer fra `MulticastWoundFX` som før).
      - `return false;`
    - Ellers: `B.Travelled += FVector::Dist(B.Pos, NewPos); B.Pos = NewPos; B.Vel = NewVel; B.Age += Dt;`
      `return B.Age < MaxLife && NewVel.SizeSquared() > FMath::Square(MinSpeedCm) && NewPos.Z > -20000.f;`

### C. `GSCharacter.h` (i ett)
11. Public, rett under `uint8 IncomingAmmoType = 0;`:
    `/** Server: the weapon the bullet hitting us right now was fired from (it may be gone from the shooter's hands). */`
    `FName IncomingWeaponId;`
12. `ServerFire`: `UFUNCTION(Server, Unreliable) void ServerFire(FVector_NetQuantize Start, FVector_NetQuantizeNormal AimDir, int32 ShotIndex);`
13. `MulticastShotFX`: `UFUNCTION(NetMulticast, Unreliable) void MulticastShotFX(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, uint8 Flags);`
    med kommentar `/** Flags: 1 = suppressed, 2 = no bullet (rocket/grenade/bolt is its own actor), 4 = tracer round. */`
14. Ved `ServerLastFireTime`:
    ```cpp
    // Spread: the server draws it from SpreadKey and the shot number; the owner knows the key and predicts the same.
    UPROPERTY(Replicated) int32 SpreadKey = 0;
    int32 LocalShotIndex = 0;        // owner: shots sent
    int32 LastServerShotIndex = 0;   // server: newest shot accepted (older ones arrived late and are dropped)
    float LastLocalSpread = 0.f;     // listen-server host: the spread FireOnce used
    ```

### D. Skyting (`GSCharacter.cpp`)
15. `#include "GSBallistics.h"`. `GetLifetimeReplicatedProps`: `DOREPLIFETIME_CONDITION(AGSCharacter, SpreadKey, COND_OwnerOnly);`
    `BeginPlay`: `if (HasAuthority()) SpreadKey = FMath::Rand();` (bare på serveren – lærdom: `BeginPlay` skal ikke
    overskrive replikert tilstand hos klienten).
16. Ny liten hjelper (static, over `FireOnce`): `static bool GSIsTracer(const FGSWeaponStats& S, uint8 AmmoType, int32 ShotIndex)`
    `{ return AmmoType == GSAmmoType::Tracer || (S.Class == EGSWeaponClass::LMG && ShotIndex % 5 == 0); }`
    (maskingeværbelter og -magasiner har vanligvis hver femte kule som sporlys).
17. `FireOnce` – bytt alt fra `const FVector Start = Camera->GetComponentLocation();` til og med
    `else if (S.Mode == EGSFireMode::Pellets) { DrawPellets(…); }` med:
    ```cpp
    const FVector Start = Camera->GetComponentLocation();
    const FVector Aim = Camera->GetForwardVector();
    const int32 ShotIndex = ++LocalShotIndex;
    const int32 Seed = GSBallistics::ShotSeed(SpreadKey, ShotIndex);
    LastLocalSpread = GetCurrentSpread();
    const float MuzzleDist = GSItems::GetMesh(W->Id) ? S.MuzzleX : 50.f;
    const FVector MuzzleStart = FPWeaponMesh->GetComponentLocation() + Aim * MuzzleDist;

    // Local, instant feedback: our own copy of the bullet flies and shows where it lands (the server's decides the hit).
    if (S.Mode == EGSFireMode::Hitscan)
    {
    	if (UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>())
    	{
    		const FGSBallistic B = GSBallistics::ForWeapon(S);
    		Ballistics->FireShown(this, Start, GSBallistics::ShotDirection(Aim, LastLocalSpread, Seed, GSBallistics::ZeroAngle(B)), B,
    			GSIsTracer(S, W->AmmoType, ShotIndex));
    	}
    }
    else if (S.Mode == EGSFireMode::Pellets)
    {
    	DrawPellets(Start, Aim, Seed, S.Pellets, bAiming ? S.AimSpread : S.HipSpread, true);
    }
    ```
    Bytt `ServerFire(Start, S.Mode == EGSFireMode::Hitscan ? Dir : Camera->GetForwardVector(), Seed);` med
    `ServerFire(Start, Aim, ShotIndex);`. Sjekk at `Dir` ikke brukes lenger i `FireOnce`.
18. `DrawPellets` – ny kropp (signaturen beholdes; `bFromMuzzle` brukes ikke lenger):
    ```cpp
    UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>();
    if (!Ballistics) return;
    const FGSBallistic B = GSBallistics::ForCaliber(TEXT("12g"));
    FRandomStream Rng(Seed);
    for (int32 I = 0; I < Pellets; ++I)
    {
    	Ballistics->FireShown(this, Start, Rng.VRandCone(Dir, FMath::DegreesToRadians(ConeDeg * 0.5f)), B, false);
    }
    ```
    (Samme `FRandomStream(Seed).VRandCone`-rekkefølge som serveren bruker i steg 19, så haglene lander likt.)
19. `ServerFire_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal AimDir, int32 ShotIndex)` – bytt
    toppen og de tre grenene, men behold runde 4 sin skuddkonto og `CancelUse()`/`W.Ammo--`. Rekkefølgen:
    ```cpp
    if (bDead || bReloading || !Weapons.IsValidIndex(CurrentSlot)) return;
    FGSWeaponSlot& W = Weapons[CurrentSlot];
    const FGSWeaponStats S = GetStats(W);
    const float Now = GetWorld()->GetTimeSeconds();
    if (S.Mode == EGSFireMode::Melee || W.Ammo <= 0) return;
    // Unreliable: a shot can arrive late or twice. Only newer shots count.
    if (ShotIndex <= LastServerShotIndex) return;
    const FVector Eye = Camera->GetComponentLocation();
    // Basic anti-cheat: the shot must start close to where we really are ...
    if (FVector::DistSquared(Start, Eye) > FMath::Square(250.f)) return;
    // ... and go roughly where we look (35 degrees leaves room for fast turns and network delay).
    const FVector Aim = FVector(AimDir).GetSafeNormal();
    if (FVector::DotProduct(Aim, GetBaseAimRotation().Vector()) < 0.82f) return;
    ```
    Så runde 4 sin skuddkonto (`MaxCredit` … `ShotCredit -= 1.f;`) uendret, deretter:
    ```cpp
    LastServerShotIndex = ShotIndex;
    ServerLastFireTime = Now;
    W.Ammo--;
    CancelUse();
    const FGSEffectMods Mods = GetMods();
    FCollisionQueryParams WallParams(SCENE_QUERY_STAT(GSFireStart), false, this);
    // Never start a shot on the far side of a wall (funn 9): if the claimed start is behind one, start at our eye.
    FHitResult Wall;
    const FVector From = GetWorld()->LineTraceSingleByChannel(Wall, Eye, Start, ECC_WorldStatic, WallParams) ? Eye : FVector(Start);
    const int32 Seed = GSBallistics::ShotSeed(SpreadKey, ShotIndex);
    // The spread is decided here. The host's own FireOnce already added this shot's recoil, so use what it used.
    const float Spread = IsLocallyControlled() ? LastLocalSpread : GetCurrentSpread();
    if (!IsLocallyControlled()) CurrentRecoil += S.RecoilPitch * Mods.Recoil * (1.f + 0.5f * GetArmPain());   // same as FireOnce
    ```
    - **Prosjektilgrenen:** `D` → `Aim`, og startpunktet
      `FVector Spawn = From + Aim * 80.f; if (GetWorld()->LineTraceSingleByChannel(Wall, From, Spawn, ECC_WorldStatic, WallParams)) Spawn = FVector(Wall.ImpactPoint) - Aim * 15.f;`
      brukes i `AGSProjectile::Launch(…)`. `MulticastShotFX(From, Aim, static_cast<uint8>(2 | (S.bSilent || S.Class == EGSWeaponClass::Bow ? 1 : 0)));`
      (runde 4 sitt flagg-uttrykk, nå med retning i stedet for endepunkt). Resten av grenen (engangsrør) uendret.
    - **Hagl:** `FCollisionQueryParams Params…` og hele løkka med `LineTraceSingleByChannel`/`TakeDamage` byttes med
      ```cpp
      if (UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>())
      {
      	const FGSBallistic B = GSBallistics::ForWeapon(S);
      	const float Cone = bAiming ? S.AimSpread : S.HipSpread;
      	FRandomStream Rng(Seed);
      	for (int32 I = 0; I < S.Pellets; ++I)
      	{
      		Ballistics->FireReal(this, From, Rng.VRandCone(Aim, FMath::DegreesToRadians(Cone * 0.5f)), B, S.Damage * Mods.Damage, W.Id, W.AmmoType);
      	}
      	MulticastPelletFX(From, Aim, Seed, S.Pellets, Cone);
      }
      NoteShotForRadio(From);
      return;
      ```
    - **Vanlig skudd** (alt fra `FHitResult Hit;` til slutten av funksjonen) byttes med
      ```cpp
      const FGSBallistic B = GSBallistics::ForWeapon(S);
      const FVector D = GSBallistics::ShotDirection(Aim, Spread, Seed, GSBallistics::ZeroAngle(B));
      if (UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>())
      {
      	Ballistics->FireReal(this, From, D, B, S.Damage * Mods.Damage, W.Id, W.AmmoType);
      }
      MulticastShotFX(From, D, static_cast<uint8>((S.bSilent || Mods.bSilent ? 1 : 0) | (GSIsTracer(S, W.AmmoType, ShotIndex) ? 4 : 0)));
      ```
    `S.Range` brukes ikke lenger ved skyting (bare som nærkamprekkevidde i `DoMeleeHit`). `grep -n "S.Range"` skal
    bare vise `DoMeleeHit`.
20. `MulticastShotFX_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, uint8 Flags)`:
    ```cpp
    const bool bSilent = (Flags & 1) != 0;
    if (!bSilent) NoteShotForRadio(Start);
    if (IsLocallyControlled()) return;   // the shooter already fired his own copy
    LastRemoteShotTime = GetWorld()->GetTimeSeconds();
    const FGSWeaponSlot* W = GetCurrentWeapon();
    const FGSWeaponStats S = W ? GetStats(*W) : GSItems::Weapon(NAME_None);
    GSAudio::PlayShotFor(this, W ? S.Class : EGSWeaponClass::Rifle, bSilent, Start);
    if ((Flags & 2) != 0) return;        // rockets, grenades and bolts fly as their own actors
    if (UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>())
    {
    	Ballistics->FireShown(this, Start, FVector(Dir), GSBallistics::ForWeapon(S), (Flags & 4) != 0);
    }
    ```
    Den gamle lysspor-linja og nedslagssporet (`FxTrace` … `SpawnImpact`) forsvinner – den viste kula lager nedslaget
    når den kommer fram. `MulticastPelletFX_Implementation` er uendret (den kaller `DrawPellets`, som nå skyter viste hagl).
    `grep -n "MulticastShotFX("` – alle kall skal ha retning og `uint8`.
21. `GetStats`: fjern linja `if (Slot.AmmoType == GSAmmoType::Tracer) { S.HipSpread *= 0.85f; S.AimSpread *= 0.85f; }`
    (sporlys gjør ikke kula mer presis; den viser bare hvor den går).

### E. `TakeDamage` – våpenet kula kom fra (to innskudd)
22. Rett etter `if (Shooter) WeaponName = Shooter->CurrentWeaponName();`:
    `if (!IncomingWeaponId.IsNone()) WeaponName = GSItems::Name(IncomingWeaponId);   // the bullet's own gun`
23. I hodeskudd-blokka (etter runde 4: `if (Zone == GSZone::Head)` … `bHeadshot = true;`): bytt
    `if (const AGSCharacter* S = Cast<AGSCharacter>(DamageCauser))` med
    `if (!IncomingWeaponId.IsNone()) Damage *= GSItems::Weapon(IncomingWeaponId).HeadshotMultiplier;`
    `else if (const AGSCharacter* S = Cast<AGSCharacter>(DamageCauser))` (resten av kjeden uendret).
24. «Blodtorst»-blokka: `if (Shooter && Shooter != this)` → `if (Shooter && Shooter != this && !Shooter->bDead)`
    (en kule som treffer etter at skytteren døde, skal ikke gi liv til liket).
    Ikke endre noe annet i `TakeDamage`; runde 3 og 4 sin kode står.

### F. Kikkerten (`GSHUD.cpp`, `DrawScope`)
25. `#include "GSBallistics.h"`. I prikk-løkka: tegn ikke prikkene under midten (`FVector2D(CX, CY + D)` tas ut av
    listen). Rett etter løkka:
    ```cpp
    // Hold marks below the centre, like the drop stadia on a real combat scope: where to hold at 200, 300, 400 ... m
    // with the cartridge in this gun (from the same numbers the bullets fly by).
    if (const FGSWeaponSlot* W = C->GetCurrentWeapon())
    {
    	const FGSBallistic B = GSBallistics::ForWeapon(C->GetStats(*W));
    	const float FOV = PlayerOwner && PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetFOVAngle() : 25.f;
    	const float PxPerRad = (Canvas->SizeX * 0.5f) / FMath::Tan(FMath::DegreesToRadians(FOV * 0.5f));
    	for (int32 M = 200; M <= 1000; M += 100)
    	{
    		const float Dy = FMath::Tan(GSBallistics::HoldUnder(B, float(M))) * PxPerRad;
    		if (Dy > R * 0.85f) break;
    		if (Dy < 6.f) continue;
    		const bool bMajor = M % 200 == 0;
    		const float Half = bMajor ? 14.f : 7.f;
    		Box(CX - Half, CY + Dy - 1.f, Half * 2.f, 2.f, Ink);
    		if (bMajor) Text(FString::FromInt(M / 100), CX + Half + 5.f, CY + Dy - 9.f, Ink, 0.7f);
    	}
    }
    ```

### G. Lydene (`Tools/Audio/gen_sounds.py`)
26. Ny funksjon `bullet_sounds()` (før `loop_save`):
    - `Bullet_Crack`: overlydssmellet (en «N-bølge»): 0,35 s; et skarpt, nesten symmetrisk trykkfall på ca. 1 ms
      (`np.sin`/sagtann over ~45 samples) fulgt av høypassfiltrert støy med 4 ms forfall, så `reverb(x, 0.3, 0.25)`
      for et kort ekko. `save("Bullet_Crack", x, 0.9)`.
    - `Bullet_Whiz`: 0,3 s båndpass-støy (`highpass(lowpass(noise, 3000), 600)`) med `env(n, 0.08, 0.1)` og en
      tonehøyde som synker (dopplereffekt: multipliser med `np.sin(2π f(t) t)` der f går fra 1400 til 700 Hz,
      blandet 30 %). `save("Bullet_Whiz", x, 0.6)`.
27. I `__main__`: `if len(sys.argv) > 1 and sys.argv[1] == "bullet": bullet_sounds(); raise SystemExit` (ved siden av
    `hurt`), og `bullet_sounds()` nederst i fulle kjøringen.
28. Kjør `python Tools/Audio/gen_sounds.py bullet`. Sjekk at `SourceArt/Sounds/S_GS_Bullet_Crack.wav` og
    `S_GS_Bullet_Whiz.wav` finnes (editoren importerer dem selv neste gang den åpnes, via `gs_sounds.py`; til da
    er lydene stille – `GSAudio::Get` gir `nullptr` uten feil).

### H. Bygg, logg og bestilling
29. `grep -n "LineTraceSingleByChannel" Source/Gloshaugen/Private/GSCharacter.cpp` – ingen spor i `FireOnce`,
    `ServerFire_Implementation` (bortsett fra de to vegg-sjekkene), `DrawPellets` eller `MulticastShotFX_Implementation`.
30. Bygg med kommandoen i `CLAUDE.md` (`-WaitMutex`, editoren lukket). Feiler bygget i en fil laget ikke eier:
    vent og prøv igjen (inntil 4 ganger), ikke rett den.
31. `IMPROVEMENTS.md`: ny overskrift øverst med linjene under (386–401).
32. Commit på grenen: `Combat: ballistics part 1 - bullets with velocity, drop and damage falloff, server-side spread, unreliable fire, scope hold marks, bullet crack`.
33. Legg én linje øverst i `.claude/lag/bestillinger.md`:
    `- <dato> · Kamp (bygger) → Spillbarhet · catalog.py · (1) Teksten til «Lysspor» sier «Litt bedre treff»; fra Kamps runde 5 gir sporlys ikke bedre treff, bare synlige kuler (og alle ser deg) – endre teksten. (2) Kolonnen range brukes ikke lenger til skyting (kula flyr til den treffer noe); den kan bli «effektiv rekkevidde» i beskrivelsen eller fjernes. (3) Når MuzzleVelocity kommer i FGSWeaponStats, si fra: Kamp bytter én linje i GSBallistics::ForWeapon. · ballistikk del 1 · ÅPEN`

## Ferdig når
- `Build.bat` går gjennom uten feil.
- Kode-sjekk: `ServerFire` er `Unreliable`, tar `ShotIndex`, og retningen klienten sender har ingen spredning
  (`FireOnce` kaller `ServerFire(Start, Aim, ShotIndex)`). Spredningen regnes i `ServerFire` med `GetCurrentSpread()`.
- Kode-sjekk: ingen `S.Range` ved skyting; `GSItems::Weapon(W.Id).Range` styrer ikke lenger hvor langt kula går.
- Kode-sjekk: `UGSBallistics::Step` kaller `TakeDamage` bare for `bReal`-kuler og bare via `FPointDamageEvent`.
- Kode-sjekk: `IncomingWeaponId` settes og nullstilles rundt `TakeDamage`, som `IncomingAmmoType`.
- Runde 3 og 4 sin kode i `TakeDamage` (`MulticastWoundFX`, `ZoneOf`, lemskade) står urørt.
- I spillet (Andreas, «Number of Players: 2», Net Mode «Play As Listen Server», etter kampstart):
  - Skriv `gs.ShowBullets 1` i konsollen og skyt et gevær langt bortover: en gul bue tegnes og faller tydelig mot
    slutten. Med pistol faller buen mye tidligere.
  - Skyt mot en vegg 150–250 m unna: støvet i veggen kommer et lite øyeblikk etter skuddet, ikke i samme øyeblikk.
  - Pistol mot et merke på en vegg ca. 80 m unna: kula treffer omtrent en håndbredd under der du siktet.
  - Med 4x eller 8x kikkert: små streker under midten merket 2, 4, 6 … (hundre meter). Sikt med 4-streken på et
    mål 400 m unna (f.eks. en vegg tvers over kartet) og treff.
  - Vanlige kuler er usynlige i lufta. Bytt til lysspor-ammo, eller skyt MG3: kula vises som en lysende strek som
    flyr bortover i en bue.
  - La den andre spilleren skyte forbi hodet ditt (litt til siden): du hører et skarpt smell rett ved deg, i tillegg
    til skuddet. Med Thompson/UMP (.45, under lydfarten) hører du et sus i stedet.
  - Hagle på 5 m dreper som før; på 40 m gjør hvert hagl tydelig mindre skade.
  - I klientvinduet: skyt mens du løper – kulene spres som før, og nedslagene du ser stemmer med der serveren
    sier du traff (blodet kommer der støvet ville kommet).
  - Skyt den andre og bytt våpen rett etter: drapsloggen viser våpenet du skjøt med.

## Risiko
- **Treff på løpende mål blir vanskeligere** (flytid + ping, ingen V8). På dette kartet (≤ 330 m) er flytiden for
  gevær under 0,4 s, for pistol under 0,25 s på 80 m. Nært hold merkes nesten ikke. V8 ligger i backlogen.
- **Klienten kjenner `SpreadKey`** og kan i teorien regne ut hvor neste kule går og vente med å skyte. Det er langt
  mindre enn i dag (null spredning ved å sende ren retning). Fullt sikkert krever serverens egen tilfeldighet og
  at klientens nedslag tegnes ut fra serverens svar – vurderes i del 2.
- **Retningskravet (35°)** kan avvise et skudd under en ekstremt rask vending med høy ping. Det rettes av ammo-
  replikeringen ved neste skudd (klienten har trukket ett skudd for mye i noen hundredeler). Avvises legitime skudd
  i testen: øk til 45° (`0.82f` → `0.7f`) og skriv det i rapporten.
- **Upålitelig `ServerFire`:** et tapt skudd bruker ikke ammo på serveren; klientens forutsagte tall rettes neste
  gang serveren endrer ammoen. Siste skudd i et magasin kan da vise 0 mens serveren har 1 – lading retter det.
- **Ytelse:** en kule koster ett linjespor per 1/60 s. 20 spillere med MG3 (20 skudd/s) og kuler som lever ca. 0,5 s
  før de treffer noe på det lille kartet gir noen hundre spor per sekund på serveren – lite. Viste kuler har tak (600).
- **`UTickableWorldSubsystem` i UE 5.8:** finnes som før (`Subsystems/WorldSubsystem.h`). Krever motoren andre
  overstyringer (f.eks. `GetTickableTickType`), legg dem til og skriv det i rapporten.
- **Rettingene og runde 4 endrer de samme funksjonene** (`ServerFire`, `MulticastShotFX`, `TakeDamage`). Stegene her
  er skrevet som bytter av hele grener med tydelig start og slutt; les funksjonene på nytt før du begynner.
- **Kroppen andre ser** skyter fortsatt ut fra øyet (som i dag), ikke fra våpenets munning; avviket er 10–20 cm.

## IMPROVEMENTS-linjer
```
## Runde 10 (Kamp): ballistikk del 1 – kuler med fart og fall (26. nov.)

386. Kuler er ekte prosjektiler med flytid og sin egen fart per kaliber (9 mm ca. 360 m/s, 5,56 mm ca. 940 m/s, .50 BMG ca. 890 m/s), i stedet for å treffe i samme øyeblikk
387. Kuler faller med tyngdekraften: på lang avstand må du holde over målet, mest med pistol og hagle
388. Luftmotstand bremser kula, og skaden faller med farten den har igjen når den treffer
389. Kula flyr til den treffer noe, i stedet for å forsvinne ved en fast «rekkevidde» (en Makarov er ikke lenger helt ufarlig på 61 m)
390. Siktene er innskutt som på ekte våpen: pistol og maskinpistol på 25 m, P90 og muskett på 50 m, rifler på 100 m
391. Hvert hagl er en egen liten kule som bremses fort, så haglen er dødelig på nært hold og svak på avstand
392. Kikkertsiktene har holdemerker for 200, 300, 400 m og videre, regnet ut fra patronen i våpenet
393. Vanlige kuler er usynlige i lufta; bare lysspor-ammo og hver femte kule fra maskingevær lyser
394. Lysspor flyr med kula i dens egen fart og bue, i stedet for å være en strek som er der med en gang
395. Støv og lyd i veggen kommer når kula kommer fram, ikke i det du trykker
396. Kuler som passerer nær hodet ditt gir et skarpt overlydssmell; langsomme kuler (.45) suser
397. Serveren bestemmer spredningen selv, så en juksende klient kan ikke lenger skyte helt uten spredning
398. Serveren avviser skudd i en retning spilleren ikke ser, og skudd eller raketter som starter på andre siden av en vegg
399. Skudd sendes som raske meldinger som kan gå tapt, så maskingevær med pakketap ikke lenger kan koble deg fra serveren, og gamle skudd som kommer for sent forkastes
400. Drapsloggen og hodeskuddskaden følger våpenet kula ble skutt med, også når skytteren rakk å bytte våpen eller døde før kula traff
401. Lysspor-ammo gir ikke lenger bedre treff, bare synlige kuler
```
