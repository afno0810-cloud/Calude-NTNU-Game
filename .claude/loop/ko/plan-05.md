# Plan 5a – runde 5 (lag Kamp): ballistikk del 1a. Kula har fart, faller og mister kraft

Skrevet om etter kontrollørens forhåndssjekk (`kontroll/forhand-plan-05.md`, BØR ENDRES). Den gamle planen er
delt i to: denne (**5a, kjernen**) og `ko/plan-06.md` (**5b**: upålitelig ild for automatvåpen, flygende lysspor,
overlydssmell/sus, holdemerker). Planen er skrevet mot `HEAD` (`a1e924d`): runde 4 (kroppssoner, `60a516c`) og
venneild/bærerfart (`2f6f46b`) er inne. **Byggeren legger samtidig inn lagfarger og CTF-HUD i `GSCharacter`,
`GSHUD` og `GSPlayerController` – denne planen rører ikke de delene** (ingen endring i `GSHUD.*` eller
`GSPlayerController.*`, og i `BeginPlay` bare én ny linje). Linjenumre flytter seg: bruk funksjonsnavn og sitert tekst.

## Mål
Kuler flyr med ekte fart og faller med tyngdekraften: støvet i veggen kommer litt etter smellet, på lang avstand
må du holde over målet, og skaden faller etter innskytingsavstanden. Serveren bestemmer spredningen.

## Forbedrer (gammelt)
- **`ServerFire`** (funn V2, R1, R2, funn 9): serveren trekker spredningen selv fra en nøkkel og skuddnummeret,
  avviser skudd i en retning spilleren ikke ser, og starter aldri et skudd bak en vegg. Forblir **pålitelig**.
- **Hagler**: hvert hagl er en egen liten kule som bremses fort.
- **Drapsloggen og hodeskudd-faktoren** følger våpenet kula ble skutt med, ikke det skytteren holder når den treffer.
- **Retningskilen når du blir truffet** peker dit skuddet kom fra, også om skytteren har flyttet seg eller koblet fra.
- **Rakett/granat/pil** starter ikke lenger på andre siden av en vegg (funn 9, delen i `ServerFire`).
- **Fast «rekkevidde»** ved skyting er borte: kula flyr til den treffer noe.

## Nytt
- **Kuler som ekte prosjektiler** (ny fil `GSBallistics.*`): munningsfart, luftmotstand og tyngdekraft per kaliber.
- **Skadefall over avstand**, med full skade ut til innskytingsavstanden (25/50/100 m) så drap-tersklene i katalogen står.
- **Innskyting**: siktene møter kula på 25 m (pistolkaliber, hagle), 50 m (5,7 mm, muskett) eller 100 m (rifler).
- **Framspoling med ping**: serverens kule flyttes straks fram `min(ping/2, 0,1 s)`, så den flyr i takt med den klienten ser.
- Feilsøkingskommando **`gs.ShowBullets 1`** (tegner banene til serverens kuler).

## Hvorfor nå
Punkt 3 i hovedagentens prioriteringer (ballistikk + V2). V1 (sikting til serveren) kom i runde 4, så serveren kan
regne spredning selv. Grunnmur for 5b, gjennomslag, lyd på avstand og vind. Alt ligger i Kamps egne filer.

## Avgrensning (bevisst – resten er 5b eller senere)
- `ServerFire` er fortsatt `Reliable` for alle våpen (V7 løses i 5b, bare for automatvåpen).
- **Lysstreken ved skudd er som i dag** (en kort strek som er der med en gang), men den går høyst 50 m og viser
  ikke lenger nedslaget – støvet kommer når kula kommer fram. Flygende lysspor og «vanlige kuler usynlige» er 5b.
  Linja i `GetStats` om lysspor-ammo (× 0,85) står til 5b.
- Ingen smell/sus, ingen holdemerker, ingen nye lyder, ingen endring i `GSHUD.*` (5b).
- Ingen gjennomslag og ingen tilbakespoling av treffbokser (V8 – neste Kamp-runde etter 5b).
- Rakettkaster, granatkaster, armbrøst og bue bruker fortsatt `AGSProjectile`.
- Ingen nye våpendata: reservetabell per kaliber i `GSBallistics.cpp` til Spillbarhet leverer `MuzzleVelocity`.

## Hensyn til lag (CTF)
All skade går fortsatt gjennom `AGSCharacter::TakeDamage` med `FPointDamageEvent` og kulas `Instigator`, så
`IsSparedFrom` (venneild) virker som før. En kule stopper i en lagkamerat uten å skade, som i dag.

## Filer
- **Nye:** `Source/Gloshaugen/Public/GSBallistics.h`, `Source/Gloshaugen/Private/GSBallistics.cpp`.
- `Source/Gloshaugen/Public/GSCharacter.h`, `Private/GSCharacter.cpp` – bare skytefunksjonene, tre små innskudd i
  `TakeDamage`, én linje i `GetLifetimeReplicatedProps` og én i `BeginPlay`.
- `IMPROVEMENTS.md` (linje 386–401), `.claude/lag/bestillinger.md` (én linje, steg 27).

Ikke rør `GSHUD.*`, `GSPlayerController.*`, `GSTypes.h`, `GSItems.*`, `catalog.py`, `GSProjectile.*`, `GSEffects.*`,
`GSAudio.*`, `GSCampus.h` (bare les `PlayArea()`), `Gloshaugen.Build.cs`. Stage filene med navn.

## Steg

### 0. Forberedelse
1. Lagfarge/CTF-HUD-arbeidet skal være bygget og committet først (`git status --short Source` tom). Les på nytt:
   `FireOnce`, `DrawPellets`, `ServerFire_Implementation`, `MulticastShotFX_Implementation`,
   `MulticastPelletFX_Implementation`, `GetCurrentSpread`, `TakeDamage`. Skuddkontoen (`ShotCredit`) og
   `* (1.f + 0.5f * GetArmPain())` i rekylen skal stå.
2. `Get-Process UnrealEditor` – kjører editoren: ikke bygg.
3. Gjør `GSCharacter.h` ferdig i ett. Lag `GSBallistics.h` før `.cpp`.

### A. `Public/GSBallistics.h`
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
   	float ZeroM = 25.f;            // the sights meet the bullet at this distance; full damage out to here
   };

   namespace GSBallistics
   {
   	GLOSHAUGEN_API FGSBallistic ForCaliber(FName Caliber);
   	GLOSHAUGEN_API FGSBallistic ForWeapon(const FGSWeaponStats& S);
   	GLOSHAUGEN_API float TimeTo(const FGSBallistic& B, float DistM);
   	GLOSHAUGEN_API float ZeroAngle(const FGSBallistic& B);                  // radians the barrel points above the sight line
   	GLOSHAUGEN_API float HoldUnder(const FGSBallistic& B, float DistM);     // radians below the sight line at DistM (5b: scope marks)
   	GLOSHAUGEN_API int32 ShotSeed(int32 Key, int32 ShotIndex);
   	GLOSHAUGEN_API FVector Lift(const FVector& Dir, float ZeroRad);         // tilt Dir up by the zero angle
   	/** One bullet: AimDir, a random point in a cone of SpreadDeg (full angle), lifted by the zero angle. */
   	GLOSHAUGEN_API FVector ShotDirection(const FVector& AimDir, float SpreadDeg, int32 Seed, const FGSBallistic& B);
   	/** A shotgun blast. The one place pellet directions are drawn, for server and every client alike. */
   	GLOSHAUGEN_API void PelletDirections(const FVector& AimDir, float ConeDeg, int32 Seed, int32 Pellets, const FGSBallistic& B, TArray<FVector>& Out);
   	/** Damage left after DistM metres: 1 out to the zero distance, then with the energy left, never below 0.2. */
   	GLOSHAUGEN_API float DamageScale(const FGSBallistic& B, float DistM);
   	/** Spread in 0.1 degree steps for the fire RPC. Owner and server both use the unpacked value. */
   	GLOSHAUGEN_API uint8 PackSpread(float Deg);
   	GLOSHAUGEN_API float UnpackSpread(uint8 Q);
   }

   struct FGSBullet
   {
   	FVector Pos = FVector::ZeroVector;
   	FVector Vel = FVector::ZeroVector;   // cm/s
   	FVector Origin = FVector::ZeroVector; // where it was fired from (hit direction, also if the shooter is gone)
   	FGSBallistic Ball;
   	float Age = 0.f;
   	float Travelled = 0.f;               // cm
   	float Damage = 0.f;                  // at the muzzle
   	FName WeaponId;
   	uint8 AmmoType = 0;
   	TWeakObjectPtr<AGSCharacter> Shooter;
   	TWeakObjectPtr<AController> Instigator;
   	bool bReal = false;                  // server bullet that does damage; otherwise it only shows
   };

   /** Every bullet in flight in this world. Server: real bullets. Each player's machine: bullets that only show. */
   UCLASS()
   class GLOSHAUGEN_API UGSBallistics : public UTickableWorldSubsystem
   {
   	GENERATED_BODY()
   public:
   	/** Server. AdvanceSeconds: fly this far at once (half the shooter's ping), so it keeps pace with what he sees. */
   	void FireReal(AGSCharacter* Shooter, const FVector& Start, const FVector& Dir, const FGSBallistic& B, float Damage, FName WeaponId, uint8 AmmoType, float AdvanceSeconds);
   	void FireShown(const AGSCharacter* Shooter, const FVector& Start, const FVector& Dir, const FGSBallistic& B);
   	virtual void Tick(float DeltaTime) override;
   	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGSBallistics, STATGROUP_Tickables); }
   protected:
   	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
   	{
   		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
   	}
   private:
   	TArray<FGSBullet> Bullets;
   	TArray<FGSBullet> Pending;   // bullets fired while Tick walks Bullets
   	bool bStepping = false;
   	void Add(const FGSBullet& B);
   	/** Moves one bullet Dt seconds. False when it is done (hit something, spent, too old, left the map). */
   	bool Step(FGSBullet& B, float Dt, FCollisionQueryParams& Params);
   };
   ```

### B. `Private/GSBallistics.cpp`
5. Includes: `GSBallistics.h`, `GSCharacter.h`, `GSItems.h`, `GSEffects.h`, `GSCampus.h`, `Engine/World.h`,
   `Engine/DamageEvents.h`, `GameFramework/Controller.h`, `DrawDebugHelpers.h`, `HAL/IConsoleManager.h`.
6. Tabell og grenser:
   ```cpp
   namespace
   {
   	struct FCalRow { const TCHAR* Caliber; float V0; float Drag; float ZeroM; };
   	const FCalRow CalTable[] = {
   		{ TEXT("9mm"),     360.f, 0.0015f,  25.f },
   		{ TEXT("45acp"),   260.f, 0.0009f,  25.f },   // subsonic
   		{ TEXT("357"),     440.f, 0.0022f,  25.f },
   		{ TEXT("57"),      715.f, 0.0018f,  50.f },
   		{ TEXT("556"),     940.f, 0.0009f, 100.f },
   		{ TEXT("762x39"),  730.f, 0.0013f, 100.f },
   		{ TEXT("762nato"), 840.f, 0.00074f, 100.f },
   		{ TEXT("762x54"),  830.f, 0.00075f, 100.f },
   		{ TEXT("50bmg"),   890.f, 0.00034f, 100.f },
   		{ TEXT("12g"),     400.f, 0.0064f,  25.f },   // one 00 buckshot pellet: slows fast
   		{ TEXT("ball"),    450.f, 0.0035f,  50.f },   // musket ball
   	};
   	constexpr float GravityCm = 980.f;
   	constexpr float MaxLife = 1.5f;           // s
   	constexpr float MaxTravelCm = 60000.f;    // 600 m: the map is 330 x 265 m
   	constexpr float MaxHeightCm = 15000.f;    // 150 m up: gone
   	constexpr float AreaMarginM = 50.f;       // outside the play area by this much: gone
   	constexpr float MinSpeedCm = 8000.f;      // 80 m/s: spent
   	constexpr float StepSeconds = 1.f / 60.f;
   	constexpr int32 MaxBullets = 600;         // oldest shown bullets go first
   	TAutoConsoleVariable<int32> CVarShowBullets(TEXT("gs.ShowBullets"), 0, TEXT("1 = draw the path of every real bullet (server)"));
   }
   ```
   Sjekk i `catalog.py` (kalibertabellen ved linje ca. 383) at alle kaliber-navn i bruk står i tabellen; mangler
   ett, legg det til med fornuftige tall og skriv det i rapporten.
7. Funksjonene (meter, sekunder, radianer):
   - `ForCaliber`: slå opp i tabellen, ellers `{ 400.f, 0.002f, 25.f }`. `ForWeapon(S)`: `ForCaliber(S.Caliber)` med
     kommentaren `// When the catalog carries it: if (S.MuzzleVelocity > 0.f) B.MuzzleVelocity = S.MuzzleVelocity;`
   - `TimeTo`: `Drag > 0 ? (Exp(Drag * DistM) - 1) / (Drag * MuzzleVelocity) : DistM / MuzzleVelocity`.
   - `ZeroAngle`: `T = TimeTo(B, B.ZeroM); return Atan(0.5f * 9.81f * T * T / B.ZeroM);`
   - `HoldUnder`: `T = TimeTo(B, DistM); Drop = 0.5f * 9.81f * T * T - Tan(ZeroAngle(B)) * DistM; return Atan(Drop / DistM);`
   - `ShotSeed`: `static_cast<int32>(HashCombine(GetTypeHash(Key), GetTypeHash(ShotIndex)))`.
   - `Lift(Dir, ZeroRad)`: `Up = (FVector::UpVector - Dir * Dot(Dir, UpVector)).GetSafeNormal(); return Up.IsNearlyZero() ? Dir : (Dir + Up * Tan(ZeroRad)).GetSafeNormal();`
   - `ShotDirection`: `FRandomStream Rng(Seed); return Lift(Rng.VRandCone(AimDir, DegreesToRadians(Max(SpreadDeg, 0) * 0.5f)), ZeroAngle(B));`
   - `PelletDirections`: `Out.Reset(); FRandomStream Rng(Seed); const FVector Axis = Lift(AimDir, ZeroAngle(B));` så
     `Pellets` ganger `Out.Add(Rng.VRandCone(Axis, DegreesToRadians(ConeDeg * 0.5f)))`.
   - `DamageScale(B, DistM)`: `R = Exp(-B.Drag * Max(0, DistM - B.ZeroM)); return Clamp(0.2f + 0.8f * R * R, 0.2f, 1.f);`
     (Regnes fra **avstanden**, ikke fra kulas utregnede fart: da er faktoren nøyaktig 1,0 ut til innskytingsavstanden,
     og små regnefeil i banen kan ikke gjøre 100 til 99,5. Fysikken er den samme: farten er `v0 · exp(−Drag · x)`.)
   - `PackSpread`: `static_cast<uint8>(Clamp(RoundToInt(Deg * 10.f), 0, 255))`; `UnpackSpread`: `Q * 0.1f`.
   Kontrolltall: 9 mm treffer ca. 19 cm lavt på 80 m; 7,62 NATO ca. 56 cm lavt på 300 m. Skadefaktor: 9 mm 1,00 på
   25 m, 0,94 på 50 m, 0,84 på 100 m; 7,62 NATO 1,00 på 100 m, 0,80 på 300 m; hagl 1,00 på 25 m, 0,86 på 40 m.
8. `Add(B)`: er `bStepping`, legg i `Pending`, ellers i `Bullets`. Er `Bullets.Num() >= MaxBullets`: fjern først den
   eldste **viste** kula (`!bReal`); finnes ingen, legg likevel til (ekte kuler forsvinner aldri).
9. `FireShown`: returner på `NM_DedicatedServer`. Lag kule (`Pos = Origin = Start`, `Vel = Dir * B.MuzzleVelocity * 100.f`,
   `Ball = B`, `Shooter`), `Add`.
   `FireReal`: samme + `bReal = true`, `Damage`, `WeaponId`, `AmmoType`, `Instigator = Shooter ? Shooter->GetController() : nullptr`.
   **Framspoling:** før `Add`, flytt kula `Clamp(AdvanceSeconds, 0, 0.1)` sekunder i delsteg på høyst `StepSeconds`
   med `Step` (egne `FCollisionQueryParams` med skytteren ignorert). Returnerer `Step` `false`, er kula ferdig: ikke `Add`.
   Kula er en lokal variabel her, så `TakeDamage` kan ikke gjøre referansen ugyldig.
10. `Tick(DeltaTime)`: lag `FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBullet), true);` **én gang**. Sett
    `bStepping = true`. Gå baklengs gjennom `Bullets`: **kopier** kula (`FGSBullet B = Bullets[I];`), bytt ignorert aktør
    (`Params.ClearIgnoredSourceObjects(); if (B.Shooter.IsValid()) Params.AddIgnoredActor(B.Shooter.Get());` – sjekk
    navnet i `CollisionQueryParams.h`; `ClearIgnoredActors` er avviklet i 5.5+). `Dt = Min(DeltaTime, 0.1f)`.
    - Ekte kule: delsteg på høyst `StepSeconds` til `Dt` er brukt eller `Step` gir `false`.
    - Vist kule: **ett** `Step(B, Dt, Params)` per `Tick` (den gjør ingen skade; ett spor per bilde er nok).
    Lever den: `Bullets[I] = B;`, ellers `Bullets.RemoveAtSwap(I)`. Etter løkka: `bStepping = false;
    Bullets.Append(Pending); Pending.Reset();`.
11. `Step(B, Dt, Params)`:
    ```cpp
    UWorld* World = GetWorld();
    const float Speed = B.Vel.Size();
    const float DragCm = B.Ball.Drag / 100.f;
    // Air drag (with the square of the speed) and gravity.
    const FVector NewVel = B.Vel + (FVector(0.f, 0.f, -GravityCm) - B.Vel * (DragCm * Speed)) * Dt;
    const FVector NewPos = B.Pos + (B.Vel + NewVel) * 0.5f * Dt;
    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, B.Pos, NewPos, ECC_Visibility, Params);
    const FVector End = bHit ? FVector(Hit.ImpactPoint) : NewPos;
    ```
    Kanalen skal være `ECC_Visibility` og et linjespor (treffboksene fra runde 4 ligger der; ikke sveip). Deretter:
    - Feilsøking (bare `bReal`, `#if !UE_BUILD_SHIPPING`): `if (CVarShowBullets.GetValueOnGameThread()) DrawDebugLine(World, B.Pos, End, bHit ? FColor::Red : FColor::Yellow, false, 4.f, 0, 1.5f);`
    - Treff (`bHit`):
      - `bReal` og `AGSCharacter* Victim = Cast<AGSCharacter>(Hit.GetActor())`:
        ```cpp
        const float DistM = (B.Travelled + FVector::Dist(B.Pos, End)) / 100.f;
        const float Dmg = B.Damage * GSBallistics::DamageScale(B.Ball, DistM);
        FPointDamageEvent Event(Dmg, Hit, B.Vel.GetSafeNormal(), nullptr);
        Victim->IncomingAmmoType = B.AmmoType;
        Victim->IncomingWeaponId = B.WeaponId;
        Victim->IncomingOrigin = B.Origin;
        Victim->TakeDamage(Dmg, Event, B.Instigator.Get(), B.Shooter.Get());
        Victim->IncomingAmmoType = GSAmmoType::Normal;
        Victim->IncomingWeaponId = NAME_None;
        ```
        (`Hit.GetComponent()` er treffboksen, så `ZoneOf` virker som før.)
      - `!bReal` og ikke `AGSCharacter`: `AGSShortFX::SpawnImpact(World, Hit.ImpactPoint, Hit.ImpactNormal);`
        (blod i kropp kommer fra `MulticastWoundFX` som før).
      - `return false;`
    - Ellers: `B.Travelled += FVector::Dist(B.Pos, NewPos); B.Pos = NewPos; B.Vel = NewVel; B.Age += Dt;` og returner
      `false` hvis noe av dette slår til: `B.Age >= MaxLife`, `B.Travelled >= MaxTravelCm`,
      `NewVel.SizeSquared() < Square(MinSpeedCm)`, `NewPos.Z > MaxHeightCm`, `NewPos.Z < -2000.f`, eller kula er utenfor
      `GSCampus::PlayArea().ExpandBy(AreaMarginM)` (området er i meter: test `FVector2D(NewPos.X, NewPos.Y) / 100.f`;
      lag boksen én gang som `static const`). Ellers `true`.

### C. `GSCharacter.h` (i ett)
12. Øverst blant forward-deklarasjonene: `struct FGSBallistic;`. Under `uint8 IncomingAmmoType = 0;`:
    ```cpp
    /** Server: the weapon and the firing point of the bullet hitting us right now (the shooter may have switched, moved or left). */
    FName IncomingWeaponId;
    FVector IncomingOrigin = FVector::ZeroVector;
    ```
13. `UFUNCTION(Server, Reliable) void ServerFire(FVector_NetQuantize Start, FVector_NetQuantizeNormal AimDir, int32 ShotIndex, uint8 SpreadQ);`
    med kommentar `/** AimDir has no spread: the server draws it. SpreadQ: the spread the owner used (GSBallistics::PackSpread). */`
14. `UFUNCTION(NetMulticast, Unreliable) void MulticastShotFX(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, uint8 Flags);`
    Kommentar: `// Flags: 1 = suppressed, 2 = no bullet (rocket/grenade/bolt is its own actor), 4 = launcher`. Bit 8 er ledig til 5b.
15. `void DrawPellets(const FVector& Start, const FVector& Dir, int32 Seed, int32 Pellets, float ConeDeg, bool bFromMuzzle, const FGSBallistic& B);`
16. Ved `ServerLastFireTime`:
    ```cpp
    // Spread: the server draws it from SpreadKey and the shot number; the owner knows the key and predicts the same.
    UPROPERTY(Replicated) int32 SpreadKey = 0;
    int32 LocalShotIndex = 0;        // owner: shots sent
    int32 LastServerShotIndex = 0;   // server: newest shot accepted
    ```

### D. Skyting (`GSCharacter.cpp`)
17. `#include "GSBallistics.h"`. `GetLifetimeReplicatedProps`: `DOREPLIFETIME_CONDITION(AGSCharacter, SpreadKey, COND_OwnerOnly);`
    `BeginPlay`, **én ny linje rett etter `Super::BeginPlay();`** (ikke rør fargelinjene under – de er i arbeid):
    `if (HasAuthority()) SpreadKey = FMath::RandHelper(MAX_int32);   // FMath::Rand() has only 15 bits`
18. Liten filstatisk hjelper over `FireOnce` (lysstreken «som i dag», men kort og uten nedslag):
    ```cpp
    // The muzzle streak: straight, at most 50 m. Where the bullet lands is shown by the bullet itself (GSBallistics).
    static void GSDrawShotStreak(UWorld* World, const AActor* Ignore, const FVector& From, const FVector& Start, const FVector& Dir)
    {
    	FHitResult Hit;
    	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSStreak), true, Ignore);
    	const FVector End = Start + Dir * 5000.f;
    	AGSShortFX::SpawnTracer(World, From, World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) ? FVector(Hit.ImpactPoint) : End);
    }
    ```
19. `FireOnce` – bytt alt fra `const FVector Start = Camera->GetComponentLocation();` til og med
    `else if (S.Mode == EGSFireMode::Pellets) { DrawPellets(…); }` med:
    ```cpp
    const FVector Start = Camera->GetComponentLocation();
    const FVector Aim = Camera->GetForwardVector();
    const int32 ShotIndex = ++LocalShotIndex;
    const int32 Seed = GSBallistics::ShotSeed(SpreadKey, ShotIndex);
    const FGSBallistic Ball = GSBallistics::ForWeapon(S);
    // The spread we use is sent along (0.1 degree steps); we use the packed value ourselves so both sides agree exactly.
    const uint8 SpreadQ = GSBallistics::PackSpread(S.Mode == EGSFireMode::Pellets ? (bAiming ? S.AimSpread : S.HipSpread) : GetCurrentSpread());
    const float Spread = GSBallistics::UnpackSpread(SpreadQ);
    const float MuzzleDist = GSItems::GetMesh(W->Id) ? S.MuzzleX : 50.f;
    const FVector MuzzleStart = FPWeaponMesh->GetComponentLocation() + Aim * MuzzleDist;

    // Local, instant feedback: our own copy of the bullet flies and shows where it lands (the server's decides the hit).
    if (S.Mode == EGSFireMode::Hitscan)
    {
    	const FVector D = GSBallistics::ShotDirection(Aim, Spread, Seed, Ball);
    	if (UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>()) Ballistics->FireShown(this, Start, D, Ball);
    	GSDrawShotStreak(GetWorld(), this, MuzzleStart, Start, D);
    }
    else if (S.Mode == EGSFireMode::Pellets)
    {
    	DrawPellets(Start, Aim, Seed, S.Pellets, Spread, true, Ball);
    }
    ```
    Bruk `Aim` der resten av funksjonen har `Camera->GetForwardVector()`. Nederst: `ServerFire(Start, Aim, ShotIndex, SpreadQ);`.
    `SpreadQ` må regnes **før** `CurrentRecoil += Recoil` (som i dag). `Dir` og `S.Range` skal ikke stå igjen i `FireOnce`.
20. `DrawPellets` – ny kropp:
    ```cpp
    TArray<FVector> Dirs;
    GSBallistics::PelletDirections(Dir, ConeDeg, Seed, Pellets, B, Dirs);
    UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>();
    const FVector From = bFromMuzzle ? FPWeaponMesh->GetComponentLocation() + Dir * 60.f : TPWeaponMesh->GetComponentLocation();
    for (const FVector& D : Dirs)
    {
    	if (Ballistics) Ballistics->FireShown(this, Start, D, B);
    	GSDrawShotStreak(GetWorld(), this, From, Start, D);
    }
    ```
21. `ServerFire_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal AimDir, int32 ShotIndex, uint8 SpreadQ)`.
    Toppen (til og med `W.Ammo <= 0`-linja) står. Bytt startpunkt-sjekken og legg til, **før** skuddkontoen:
    ```cpp
    // The owner numbers his shots. Older or repeated numbers are dropped.
    const int64 Jump = int64(ShotIndex) - int64(LastServerShotIndex);
    if (Jump <= 0) return;
    const FVector Aim = FVector(AimDir).GetSafeNormal();
    if (Aim.IsNearlyZero()) return;
    // The shot must go roughly where we look (35 degrees leaves room for fast turns and network delay).
    if (!IsLocallyControlled() && FVector::DotProduct(Aim, GetBaseAimRotation().Vector()) < 0.82f)
    {
    #if !UE_BUILD_SHIPPING
    	UE_LOG(LogTemp, Warning, TEXT("ServerFire: shot %d from %s rejected, %.0f degrees off"), ShotIndex, *GetName(),
    		FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Aim, GetBaseAimRotation().Vector()))));
    #endif
    	return;
    }
    ```
    Skuddkontoen (`MaxCredit` … `ShotCredit -= 1.f;`) uendret. Deretter (bytter `ServerLastFireTime = Now;` … `const FVector D = …;`):
    ```cpp
    LastServerShotIndex = ShotIndex;   // only shots that passed every check move the number
    ServerLastFireTime = Now;
    W.Ammo--;
    CancelUse();
    const FGSEffectMods Mods = GetMods();
    // Where it starts: the owner's eye as he saw it, if that is close to where we have him (120 cm = a sprint for
    // 0.2 s) and no wall lies between. Otherwise our own eye. The shot is never thrown away for this.
    const FVector Eye = Camera->GetComponentLocation();
    FCollisionQueryParams WallParams(SCENE_QUERY_STAT(GSFireStart), false, this);
    FHitResult Wall;
    const bool bStartOk = FVector::DistSquared(Start, Eye) <= FMath::Square(120.f)
    	&& !GetWorld()->LineTraceSingleByChannel(Wall, Eye, Start, ECC_WorldStatic, WallParams);
    const FVector From = bStartOk ? FVector(Start) : Eye;
    // Spread. The size: what the owner used, held within 15 % of what we get ourselves (so an honest player hits
    // exactly where he saw it, and a cheat gains at most 15 %). The host's own FireOnce already added this shot's recoil.
    const bool bPellets = S.Mode == EGSFireMode::Pellets;
    const float Own = bPellets ? (bAiming ? S.AimSpread : S.HipSpread) : GetCurrentSpread();
    const float Claimed = GSBallistics::UnpackSpread(SpreadQ);
    const float Spread = IsLocallyControlled() ? Claimed : FMath::Clamp(Claimed, Own * 0.85f, Own * 1.15f);
    if (!IsLocallyControlled()) CurrentRecoil += S.RecoilPitch * Mods.Recoil * (1.f + 0.5f * GetArmPain());   // same as FireOnce
    // The pattern: from the key and the shot number, so the owner sees the same. He picks the number himself, so
    // only small jumps (lost packets) are trusted; after a bigger jump we draw our own.
    const int32 Seed = Jump <= 3 ? GSBallistics::ShotSeed(SpreadKey, ShotIndex) : FMath::RandHelper(MAX_int32);
    const FGSBallistic Ball = GSBallistics::ForWeapon(S);
    const APlayerState* PS = GetPlayerState();
    const float Advance = IsLocallyControlled() || !PS ? 0.f : FMath::Min(PS->GetPingInMilliseconds() * 0.0005f, 0.1f);   // half the round trip
    UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>();
    ```
    (Sjekk i motorens `PlayerState.h`/`.cpp` at `GetPingInMilliseconds()` er tur-retur-tiden; skriv i rapporten hva du fant.)
    - **Prosjektilgrenen:** `D` → `Aim`; startpunkt
      `FVector Spawn = From + Aim * 80.f; if (GetWorld()->LineTraceSingleByChannel(Wall, From, Spawn, ECC_WorldStatic, WallParams)) Spawn = FVector(Wall.ImpactPoint) - Aim * 15.f;`
      brukes i `AGSProjectile::Launch`. `MulticastShotFX(From, Aim, <samme flagg-uttrykk som i dag>);`. Resten (engangsrør) uendret.
    - **Hagl** (bytter `FCollisionQueryParams Params…` og hele løkka):
      ```cpp
      TArray<FVector> Dirs;
      GSBallistics::PelletDirections(Aim, Spread, Seed, S.Pellets, Ball, Dirs);
      if (Ballistics) { for (const FVector& PD : Dirs) Ballistics->FireReal(this, From, PD, Ball, S.Damage * Mods.Damage, W.Id, W.AmmoType, Advance); }
      MulticastPelletFX(From, Aim, Seed, S.Pellets, Spread);
      NoteShotForRadio(From);
      return;
      ```
    - **Vanlig skudd** (alt fra `FHitResult Hit;` og ut):
      ```cpp
      const FVector D = GSBallistics::ShotDirection(Aim, Spread, Seed, Ball);
      if (Ballistics) Ballistics->FireReal(this, From, D, Ball, S.Damage * Mods.Damage, W.Id, W.AmmoType, Advance);
      MulticastShotFX(From, D, static_cast<uint8>(S.bSilent || Mods.bSilent ? 1 : 0));
      ```
    Merk: `W` er en referanse inn i `Weapons`. Kopier `W.Id` og `W.AmmoType` til lokale variabler før `FireReal`
    (framspolingen kan kalle `TakeDamage`), og bruk kopiene etterpå.
    `grep -n "S.Range"` skal etterpå bare vise `DoMeleeHit`.
22. `MulticastShotFX_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, uint8 Flags)`: alt til og med
    `GSAudio::PlayShotFor(…)`-blokka står som i dag. Bytt resten (`const FVector From = …` og ut) med:
    ```cpp
    if (bNoTracer) return;   // rockets, grenades and bolts fly as their own actors
    const FGSWeaponSlot* W = GetCurrentWeapon();
    const FGSWeaponStats S = W ? GetStats(*W) : GSItems::Weapon(NAME_None);
    if (UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>()) Ballistics->FireShown(this, Start, FVector(Dir), GSBallistics::ForWeapon(S));
    if (!bSilent) GSDrawShotStreak(GetWorld(), this, TPWeaponMesh->GetComponentLocation(), Start, FVector(Dir));
    ```
    `MulticastPelletFX_Implementation`: `DrawPellets(Start, Dir, Seed, Pellets, ConeDeg, false, B)` der `B` er
    `GSBallistics::ForWeapon(GetStats(*W))` hvis nåværende våpen finnes og har `Mode == Pellets`, ellers
    `GSBallistics::ForCaliber(TEXT("12g"))`. `grep -n "MulticastShotFX(\|DrawPellets(\|ServerFire("` – alle kall skal ha ny form.

### E. `TakeDamage` – tre små innskudd (ikke rør lagsjekken øverst eller noe annet)
23. Rett etter `if (Shooter) WeaponName = Shooter->CurrentWeaponName();`:
    ```cpp
    const bool bBullet = !IncomingWeaponId.IsNone();   // a bullet from GSBallistics: it knows its own gun and where it was fired
    if (bBullet) WeaponName = GSItems::Name(IncomingWeaponId);
    ```
    I hodeskudd-blokka: `if (const AGSCharacter* S = Cast<AGSCharacter>(DamageCauser))` →
    `if (bBullet) Damage *= GSItems::Weapon(IncomingWeaponId).HeadshotMultiplier;` `else if (const AGSCharacter* S = …)` (resten uendret).
24. I `ClientTookHit`-blokka: `bHasFrom = Proj ? !bOwnProjectile : (bBullet || (Shooter && Shooter != this));` og i `From`-uttrykket
    siste ledd `: bBullet ? IncomingOrigin : Shooter->GetActorLocation();`. Blod-blokka: `if (DamageCauser && !bBurn)` →
    `if ((DamageCauser || bBullet) && !bBurn)` (linja `if (!bPointHit) WoundDir = … DamageCauser->…` er trygg: en kule er alltid et punkttreff).
25. «Blodtorst»-blokka: `if (Shooter && Shooter != this && bShooterThrewIt)` → legg til `&& !Shooter->bDead`.

### F. Bygg, logg, bestilling
26. `grep -n "LineTraceSingleByChannel" Source/Gloshaugen/Private/GSCharacter.cpp`: i skytefunksjonene skal det bare stå
    `GSDrawShotStreak` og de to vegg-sjekkene i `ServerFire`. Bygg (`-WaitMutex`, editoren lukket; feil i andres fil:
    vent og prøv igjen, inntil 4 ganger). `IMPROVEMENTS.md`: ny overskrift øverst med linjene under.
    Commit: `Combat: ballistics part 1a - bullets with velocity, drop and damage falloff, server-side spread, start and aim checks`.
27. Én linje øverst i `.claude/lag/bestillinger.md`:
    `- <dato> · Kamp (bygger) → Spillbarhet · catalog.py · (1) Kolonnen range brukes ikke lenger til skyting (kula flyr til den treffer noe; bare nærkamp bruker den) – den kan bli «effektiv rekkevidde» i beskrivelsen. (2) Når MuzzleVelocity kommer i FGSWeaponStats, si fra: Kamp bytter én linje i GSBallistics::ForWeapon. · ballistikk 5a · ÅPEN`
28. **I rapporten** (kontrollørens krav): tabell med skade per treff i brystet på 25/50/100 m for Garand, MP40, Pump
    (per hagl) og L96, før og etter, og antall treff for å drepe (100 liv). Forventet: uendret på 25 m for alle, og på
    100 m for Garand og L96; MP40 20 → 18,8 på 50 m og 16,8 på 100 m; Pump 13 → 10,2 på 50 m.

## Ferdig når
- `Build.bat` går gjennom uten feil.
- **Juks (V2):** `FireOnce` sender `Aim` uten spredning. `ServerFire` bruker frøet fra `ShotIndex` bare når
  `ShotIndex − LastServerShotIndex` er 1–3; ved større hopp trekker serveren sitt eget (`FMath::RandHelper`).
  `LastServerShotIndex` settes først etter retningskravet og skuddkontoen.
- **Spredningens størrelse:** klientens verdi brukes, klemt til 0,85–1,15 × serverens egen (ikke klemt for verten).
- **Skadefall:** `DamageScale` gir nøyaktig 1,0 ut til `ZeroM`. Rapporten har tabellen fra steg 28, og ingen våpen
  trenger flere treff enn før innenfor innskytingsavstanden.
- **Start:** startpunktet brukes bare innen 120 cm fra serverens øye og uten vegg imellom; ellers øyet. Ingen skudd kastes for dette.
- **Levetid:** kuler fjernes etter 1,5 s, 600 m, over 150 m høyde eller 50 m utenfor `PlayArea()`. Viste kuler: ett spor per `Tick`.
- **Framspoling:** `FireReal` flytter kula `min(ping/2, 0,1 s)` for fjerne klienter, 0 for verten.
- `ServerFire` er fortsatt `Reliable`. `UGSBallistics` kaller `TakeDamage` bare for `bReal`, bare med `FPointDamageEvent`,
  og holder ingen referanse inn i `Bullets` mens `TakeDamage` kjører. `FCollisionQueryParams` lages én gang per `Tick`.
- `TakeDamage`: lagsjekken, `ZoneOf`, lemskade og `MulticastWoundFX` står urørt. `GSHUD.*`, `GSPlayerController.*`,
  `GSEffects.*`, `GSAudio.*`, `GSTypes.h` er ikke endret.
- I spillet (Andreas: «Number of Players: 2», «Play As Listen Server», etter kampstart):
  - `gs.ShowBullets 1` i konsollen, skyt et gevær bortover plassen: en gul bue tegnes og faller mot slutten. Med pistol faller den mye før.
  - Skyt mot en vegg 150–250 m unna (fra den ene basen mot den andre): støvet kommer et øyeblikk etter skuddet.
  - Pistol mot et merke ca. 80 m unna: kula treffer omtrent en håndbredd under der du siktet.
  - Hagle på 5 m dreper som før; på 40 m gjør hvert hagl mindre skade.
  - I klientvinduet: skyt automatild mens du går – støvet du ser i veggen, og blodet på den andre spilleren, kommer der du så kula gå.
  - Skyt den andre og bytt våpen rett etter: drapsloggen viser våpenet du skjøt med.
  - Still deg bak et hjørne og bli truffet av en som så løper vekk: retningskilen peker dit skuddet kom fra.

## Risiko
- **Treff på løpende mål blir vanskeligere for klienter** (flytid + at du ser målet ca. 0,1 s + ping/2 forsinket; verten
  har ingen forsinkelse). På 20–80 m: rifle 0,03–0,1 s, 9 mm 0,24 s på 80 m. Framspolingen fjerner serverens del.
  Resten er V8 (tilbakespoling av treffbokser), som er **neste Kamp-runde etter 5b**, ikke backlog.
- **Restjuks:** en klient kan velge beste av tre mønstre (hopp ≤ 3) og 15 % mindre kjegle. Mye mindre enn i dag (null spredning).
- **Retningskravet (35°)** kan avvise et ekte skudd ved lynrask vending med høy ping; da er skuddet tapt for klienten
  (han hørte smellet). Loggen fra steg 21 viser vinkelen. Avvises ekte skudd i test: øk til 45° (`0.82f` → `0.7f`) og skriv det i rapporten.
- **Klemmen 0,85–1,15** treffer når serverens sikte-/farttilstand henger etter klientens (f.eks. `bAiming` kommer etter
  skuddet): da lander serverens kule litt utenfor det klienten så, for det ene skuddet.
- **Ytelse, server:** en ekte kule = ett linjespor per 1/60 s. Verste tilfelle 20 × MG3 (400 kuler/s): rett opp i
  lufta lever kula ca. 0,2 s (150 m høyde), bortover høyst ca. 0,5 s før den er ute av området → 100–200 kuler i
  lufta → 6 000–12 000 spor i sekundet. I vanlig spill (kuler treffer vegger etter 0,05–0,2 s) noen hundre. Viste
  kuler: ett spor per bilde, tak 600. Lysstreken koster ett spor per skudd, som i dag.
- **`UTickableWorldSubsystem` i UE 5.8:** krever motoren flere overstyringer, legg dem til og skriv det i rapporten.
- **Verten har to kuler per skudd** (ekte + vist). Bare den viste lager støv; bare den ekte skader. Ikke bland.
- **Våpenbytte hos andre:** den viste kula hos andre maskiner bruker kaliberet til våpenet skytteren holder når
  meldingen kommer. Byttet han i samme øyeblikk, får den ene viste kula feil bue. Bare utseende.
- **Frakoblet skytter:** kula skader fortsatt, med blod og riktig retning, men drapet går ikke til noen.
- **Ammotelleren** hos klienten kan vise ett skudd for lite etter et avvist skudd, til neste lading (som i dag).

## IMPROVEMENTS-linjer
```
## Runde 10 (Kamp): ballistikk 1a – kuler med fart og fall (26. nov.)

386. Kuler er ekte prosjektiler med flytid og sin egen fart per kaliber (9 mm ca. 360 m/s, 5,56 mm ca. 940 m/s, .50 BMG ca. 890 m/s), i stedet for å treffe i samme øyeblikk
387. Kuler faller med tyngdekraften: på lang avstand må du holde over målet, mest med pistol og hagle
388. Luftmotstand bremser kula, og skaden faller med avstanden – men først etter innskytingsavstanden, så våpnene dreper på like mange treff som før på vanlig hold
389. Kula flyr til den treffer noe, i stedet for å forsvinne ved en fast «rekkevidde» (en Makarov er ikke lenger helt ufarlig på 61 m)
390. Siktene er innskutt som på ekte våpen: pistol, maskinpistol og hagle på 25 m, P90 og muskett på 50 m, rifler på 100 m
391. Hvert hagl er en egen liten kule som bremses fort, så haglen er dødelig på nært hold og svak på avstand
392. Støvet i veggen kommer når kula kommer fram, ikke i det du trykker
393. Serveren trekker spredningen selv, så en juksende klient kan ikke lenger skyte helt uten spredning eller velge seg et heldig skudd
394. Du treffer der du ser kula gå også som klient: spredningen du ser og den serveren bruker er den samme
395. Serveren avviser skudd i en retning spilleren ikke ser
396. Skudd, raketter og granater kan ikke lenger starte på andre siden av en vegg eller flere meter fra der skytteren står
397. Serverens kule spoles fram med halve pingen, så den flyr i takt med kula du ser hos deg selv
398. Drapsloggen og hodeskuddskaden følger våpenet kula ble skutt med, også når skytteren rakk å bytte våpen eller døde før kula traff
399. Retningskilen når du blir truffet peker dit skuddet ble avfyrt, også om skytteren har flyttet seg eller koblet fra
400. Blodtørst gir ikke lenger liv til en skytter som døde før kula hans traff
401. Kuler som forlater banen ryddes bort med en gang, så serveren holder følge når mange skyter maskingevær samtidig
```
