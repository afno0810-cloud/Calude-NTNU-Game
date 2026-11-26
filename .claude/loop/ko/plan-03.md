# Plan – runde 3: blod. Skudd som treffer mennesker setter spor

Erstatter den gamle planen for runde 3 (drapsserier og medaljer). Den ble strøket fordi Andreas' nye, overordnede
mål er **superrealistisk** og **veldig groteskt**, uten arkade-preg. Bygges oppå commit `94f1370` (runde 2).

## Mål
Når en kule treffer et menneske ser alle i nærheten det: blod i lufta ved såret, sprut på veggen og gulvet bak,
dråper på bakken, og en blodpøl som sakte vokser under kroppen som blir liggende.

## Forbedrer (gammelt)
- **Døden/ragdollen** (`OnRep_Dead`): i dag kastes alle kropper likt bakover og opp i lufta (fast støt
  `-4000 forover + 1500 opp`), uansett hvor skuddet kom fra. Blir: kroppen får et lite støt *i treffpunktet, i
  kulas retning* og segner ellers bare sammen. Eksplosjoner kaster hardere. Ingen treff (sult) = ingen støt.
- **Kropper blir liggende**: 90 sekunder i stedet for 15 (tak på 12 kropper samtidig).
- **Treff på andre spillere**: i dag skjer det *ingenting* synlig eller hørbart for tilskuere og skytter når en
  spiller treffes (bare treffmarkøren). Blir: alle i nærheten ser blodet og hører treffet i kropp eller vest.

## Nytt
- **Blodsky og dråper ved såret** – for alle maskiner (ny `MulticastWoundFX`).
- **Blodsprut på vegg/gulv bak offeret** når kula går gjennom (opptil ca. 3 m bak), avlang når den treffer skrått,
  større ved hodeskudd. Stoppes kula av vest/hjelm blir det støv og fiber i stedet for blod.
- **Bloddråper på gulvet** under den som treffes (lander litt etter, som ekte dråper).
- **Blodpøl som vokser** under døde kropper (under hodet ved hodeskudd), tilpasser seg kanter og trapper.
- **Eget blodmateriale** `M_GS_Blood`: blankt og vått med ujevn kant, tørker (mørkere og mattere) og krymper bort.
  Lages av Python-skriptet når editoren åpnes (som `M_GS_FX`); finnes det ikke, brukes flate mørkerøde skiver.

## Hvorfor nå
- Andreas' to nye hovedmål. Spillet har i dag **null blod i verden** (bare rød skjermkant fra runde 2), og treff på
  folk er usynlige for alle andre. Dette er det største enkeltgrepet mot «groteskt og realistisk».
- Grunnmur for neste runder: blødning/blodspor, sår på kroppen og skade etter kroppsdel bruker samme multicast,
  samme flekk-funksjon og samme materiale.
- Nesten ren C++ i `GSEffects` (ny kode) + noen få linjer i `GSCharacter`. Ingen .uasset i git (materialene i
  `Content/GS/Materials` lages lokalt av skriptet og er ikke sporet).

## Filer
- `Source/Gloshaugen/Public/GSTypes.h` – ny `namespace GSWound`.
- `Source/Gloshaugen/Public/GSEffects.h`, `Private/GSEffects.cpp` – `SpawnBloodStain`, `SpawnBloodHit`,
  `SpawnBloodPool`, flekk-modus i `Tick`.
- `Source/Gloshaugen/Public/GSCharacter.h`, `Private/GSCharacter.cpp` – `MulticastWoundFX`, `ApplyDeathImpulse`,
  `SpawnDeathPool`; små tillegg i `TakeDamage`, `DoMeleeHit`, `OnRep_Dead`, `Die`.
- `Content/Python/gs_materials.py` – `_blood_master()` + kall i `run()`.
- `IMPROVEMENTS.md` – linje 313–326 under ny overskrift.

Ikke rør `GSHUD.*`, `GSGameMode.*`, `GSPlayerController.*`, `Config/`, eller usporede filer i `Content/`.
Stage kun filene over, med navn (aldri `git add -A` / `git add .`).

## Steg

### 0. Forberedelse
1. `git log -1` skal vise `94f1370` (eller en rettings-commit for runde 2 oppå den). Har kontrolløren sendt runde 2
   tilbake med rettinger i `TakeDamage`, vent til de er inne, og les funksjonen på nytt før steg D.
2. `Get-Process UnrealEditor` – kjører editoren: ikke bygg (be om Ctrl+Alt+F11 eller lukking).

### A. Typer (`GSTypes.h`)
3. Legg til rett under `namespace GSHurtKind`:
   ```cpp
   /** What a hit did to the body, for the blood everyone sees (MulticastWoundFX). */
   namespace GSWound
   {
   	constexpr uint8 Flesh = 0;    // bullet stayed in the body: blood at the wound, drops on the floor
   	constexpr uint8 Through = 1;  // went through: also spatter on whatever is behind
   	constexpr uint8 Head = 2;     // head, through
   	constexpr uint8 Armor = 3;    // stopped by vest/helmet: dust and fibre, no blood
   	constexpr uint8 Blast = 4;    // explosion / fragments
   	constexpr uint8 Blade = 5;    // melee
   }
   ```

### B. Blodmaterialet (`Content/Python/gs_materials.py`)
4. Ny funksjon rett etter `_fx_master` (samme stil; `_expr`, `_new_material`, `mel`, `eal` finnes):
   ```python
   def _blood_master():
       """Wet blood lying on a surface (AGSShortFX stains, on a flat plane). Lit and glossy, ragged edge.
       Params: Color (rgb), Roughness, Opacity (1 = whole stain, towards 0 it erodes away), Soft."""
       m = _new_material("M_GS_Blood")
       m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
       m.set_editor_property("opacity_mask_clip_value", 0.3)
       col = _expr(m, unreal.MaterialExpressionVectorParameter, -900, -200, parameter_name="Color",
                   default_value=unreal.LinearColor(0.11, 0.004, 0.004, 1))
       mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
       rough = _expr(m, unreal.MaterialExpressionScalarParameter, -900, -60, parameter_name="Roughness", default_value=0.12)
       mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
       op = _expr(m, unreal.MaterialExpressionScalarParameter, -900, 40, parameter_name="Opacity", default_value=1.0)
       soft = _expr(m, unreal.MaterialExpressionScalarParameter, -900, 120, parameter_name="Soft", default_value=0.6)
       uv = _expr(m, unreal.MaterialExpressionTextureCoordinate, -1300, 250)
       sub = _expr(m, unreal.MaterialExpressionSubtract, -1150, 250, const_b=0.5)
       mel.connect_material_expressions(uv, "", sub, "A")
       dot = _expr(m, unreal.MaterialExpressionDotProduct, -1000, 250)
       mel.connect_material_expressions(sub, "", dot, "A")
       mel.connect_material_expressions(sub, "", dot, "B")
       r2 = _expr(m, unreal.MaterialExpressionMultiply, -850, 250, const_b=4.0)
       mel.connect_material_expressions(dot, "", r2, "A")
       inv = _expr(m, unreal.MaterialExpressionOneMinus, -700, 250)
       mel.connect_material_expressions(r2, "", inv, "")
       sat = _expr(m, unreal.MaterialExpressionSaturate, -560, 250)
       mel.connect_material_expressions(inv, "", sat, "")
       pw = _expr(m, unreal.MaterialExpressionPower, -420, 250)
       mel.connect_material_expressions(sat, "", pw, "Base")
       mel.connect_material_expressions(soft, "", pw, "Exponent")
       mask = pw
       try:
           # world-space noise breaks the round edge up: every stain gets its own ragged outline
           nz = _expr(m, unreal.MaterialExpressionNoise, -700, 420, scale=0.3, levels=3, output_min=0.2, output_max=1.0)
           mm = _expr(m, unreal.MaterialExpressionMultiply, -300, 330)
           mel.connect_material_expressions(pw, "", mm, "A")
           mel.connect_material_expressions(nz, "", mm, "B")
           mask = mm
       except Exception as e:
           log(f"stoey i blod hoppet over: {e}")
       a = _expr(m, unreal.MaterialExpressionMultiply, -200, 150)
       mel.connect_material_expressions(mask, "", a, "A")
       mel.connect_material_expressions(op, "", a, "B")
       mel.connect_material_property(a, "", unreal.MaterialProperty.MP_OPACITY_MASK)
       mel.recompile_material(m)
       eal.save_loaded_asset(m)
   ```
5. I `run()`, rett etter `forced = build_masters()` (ikke øk `VERSION` – da bygges alle materialer på nytt):
   ```python
   try:
       if not eal.does_asset_exist(f"{MAT}/M_GS_Blood"):
           log("bygger blodmateriale")
           _blood_master()
   except Exception as e:  # never block the rest
       log(f"M_GS_Blood feilet: {e}")
   ```
6. Sjekk syntaks: `python -m py_compile Content/Python/gs_materials.py` (`__pycache__` er i `.gitignore`).
   Skriptet kan ikke kjøres uten editoren – derfor reserveløsningen i steg C.

### C. Effektene (`GSEffects.h` / `GSEffects.cpp`)
7. `GSEffects.h` – nye statiske funksjoner (public) og felt (protected):
   ```cpp
   /** Blood stain lying flat on a surface. Grows to Size (cm, across) over GrowSeconds, dries, erodes away at the
    *  end of LifeSeconds. Stretch > 1 makes it long along Along (spatter that hit at an angle). Delay = seconds
    *  before it shows (a falling drop). */
   static AGSShortFX* SpawnBloodStain(UWorld* World, const FVector& Location, const FVector& Normal, float Size,
   	float GrowSeconds, float LifeSeconds, const FVector& Along = FVector::ZeroVector, float Stretch = 1.f, float Delay = 0.f);
   /** A hit on a body (GSWound kind): mist and drops at the wound, spatter behind, drops on the floor. Strength 0..1.
    *  bOwnView: the local player is the victim (no mist in his own face). */
   static void SpawnBloodHit(UWorld* World, const FVector& Point, const FVector& Dir, uint8 Kind, float Strength,
   	const AActor* Victim, bool bOwnView);
   /** Pool that slowly spreads under a body. Shrinks to fit the floor it lies on (ledges, stairs). */
   static void SpawnBloodPool(UWorld* World, const FVector& Location, const FVector& Normal, float Size, const AActor* Body);
   ```
   ```cpp
   bool bStain = false;
   float GrowTime = 0.1f;
   float StretchK = 1.f;
   ```
8. `GSEffects.cpp`, i det anonyme navnerommet:
   ```cpp
   UMaterialInterface* GSBloodMaterial()   // same load-once pattern as GSFXMaterial
   // path: /Game/GS/Materials/M_GS_Blood.M_GS_Blood, LOAD_NoWarn | LOAD_Quiet
   TArray<TWeakObjectPtr<AGSShortFX>> GSStains;   // oldest first
   constexpr int32 MaxStains = 220;
   const FLinearColor BloodFresh(0.16f, 0.005f, 0.005f);
   const FLinearColor BloodDry(0.045f, 0.008f, 0.006f);
   ```
   Nye includes etter behov: `GSTypes.h` (for `GSWound`), `GameFramework/Pawn.h`, `CollisionQueryParams.h`.
9. `SpawnBloodStain`:
   - Returner `nullptr` hvis `!World`, dedikert server, eller `Size < 0.5f`.
   - Rydd `GSStains`: fjern ugyldige; så lenge `Num() >= MaxStains`: `Destroy()` den første og fjern den.
   - Retning: `N = Normal.GetSafeNormal()` (null → `FVector::UpVector`); `X = FVector::VectorPlaneProject(Along, N).GetSafeNormal()`;
     er `X` null: en tilfeldig vektor i planet (`FVector::VectorPlaneProject(FMath::VRand(), N).GetSafeNormal()`, og hvis også den er
     null: `N.Rotation().RotateVector(FVector::RightVector)`). `Rot = FRotationMatrix::MakeFromZX(N, X).Rotator()`.
   - Spawn `AGSShortFX` i `Location + N * FMath::FRandRange(0.25f, 0.45f)` (ulik høyde = ingen flimring mellom flekker).
   - Med `GSBloodMaterial()`: mesh `GSFXPlane()`, `FX->MID = UMaterialInstanceDynamic::Create(Mat, FX)`, sett `Color = BloodFresh`,
     `Roughness = 0.12`, `Opacity = 1`, `Soft = FMath::FRandRange(0.45f, 0.8f)`, og `SetMaterial(0, MID)`. Uten: mesh `GSUtils::Cylinder()` og
     `GSUtils::SetColor(FX->Mesh, FLinearColor(0.10f, 0.004f, 0.004f))` (Z-skala 0,002; ingen `MID`).
   - `bStain = true; Size0 = Size * 0.12f; Size1 = Size; StretchK = FMath::Clamp(Stretch, 1.f, 4.f); GrowTime = FMath::Max(GrowSeconds, 0.02f);`
     `Life = FMath::Max(LifeSeconds, 6.f); Age = -FMath::Max(Delay, 0.f);`
   - `FX->Mesh->SetCullDistance(Size < 12.f ? 2500.f : 7000.f);` (små flekker tegnes ikke langt unna).
   - `Delay > 0` → `SetActorHiddenInGame(true)`. `SetLifeSpan(Life + Delay + 0.1f)`, `SetActorTickEnabled(true)`, `Tick(0.f)`,
     legg i `GSStains`, returner.
10. `Tick`: ny gren **øverst**, før `if (!bSprite) return;`:
    ```cpp
    if (bStain)
    {
    	Age += DeltaSeconds;
    	if (Age < 0.f) return;                       // the drop has not landed yet
    	if (IsHidden()) SetActorHiddenInGame(false);
    	const float T = FMath::Clamp(Age / GrowTime, 0.f, 1.f);
    	const float SizeNow = FMath::Lerp(Size0, Size1, 1.f - FMath::Square(1.f - T));   // spreads fast, then creeps
    	Mesh->SetWorldScale3D(FVector(SizeNow * StretchK / 100.f, SizeNow / 100.f, MID ? 1.f : 0.002f));
    	const float Left = Life - Age;
    	if (MID)
    	{
    		const float Dry = FMath::Clamp((Age - GrowTime) / 60.f, 0.f, 1.f);   // darker and duller as it dries
    		MID->SetVectorParameterValue(TEXT("Color"), BloodFresh + (BloodDry - BloodFresh) * Dry);
    		MID->SetScalarParameterValue(TEXT("Roughness"), FMath::Lerp(0.12f, 0.55f, Dry));
    		MID->SetScalarParameterValue(TEXT("Opacity"), FMath::Clamp(Left / 5.f, 0.f, 1.f));
    	}
    	// once it has stopped spreading it only needs to dry: tick twice a second (every frame again while eroding)
    	SetActorTickInterval(T >= 1.f && Left > 5.5f ? 0.5f : 0.f);
    	return;
    }
    ```
11. `SpawnBloodHit(World, Point, Dir, Kind, Strength, Victim, bOwnView)`:
    - Tidlig retur: `!World` eller dedikert server. `S = FMath::Clamp(Strength, 0.f, 1.f)`. `D = Dir.GetSafeNormal()`, null → `FVector::DownVector`.
    - Lokal hjelper (lambda) `Trace(From, To, OutHit)`: `LineTraceSingleByChannel(..., ECC_Visibility, Params)` med
      `FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBlood), true); if (Victim) Params.AddIgnoredActor(Victim);`
      Returner bare `true` når noe er truffet **og** `!OutHit.GetActor() || !OutHit.GetActor()->IsA(APawn::StaticClass())`
      (ikke blodflekker som henger i lufta der en annen spiller sto).
    - Avstand til lokal kamera (`UGameplayStatics::GetPlayerCameraManager(World, 0)`): `bNear = avstand < 8000` (80 m).
      Sprites (sky, dråper i lufta) lages bare når `bNear`. Flekker lages alltid.
    - **`Armor`**: (bare hvis `bNear && !bOwnView`) to støvdotter `SpawnSprite(Point - D * 4.f, 5.f, FRandRange(22,34), FLinearColor(0.34f, 0.32f, 0.29f), 0.5f,
      FRandRange(0.5,0.8), -D * 70.f + VRand()*25, false, 1.f, -10.f, 3.f)`. Ingen blod. Retur.
    - **Sky ved såret** (alle andre typer, når `bNear && !bOwnView`): inngang: `SpawnSprite(Point - D * 3.f, 4.f, 16.f + 22.f * S, BloodFresh * 0.8f, 0.7f, 0.3f, -D * 60.f, false, 1.f, -200.f, 3.f)`.
      `Through`/`Head`/`Blast`: utgang: 2 (Head: 4) stk `SpawnSprite(Point + D * 22.f, 6.f, FRandRange(30,45) + 35.f * S, BloodFresh * 0.7f, 0.65f, FRandRange(0.35,0.6),
      (D + VRand() * 0.25f) * FRandRange(180,320), false, 1.f, -250.f, 3.f)`.
      Dråper i lufta: `N = 3 + RoundToInt(5 * S)` (Head +4): `SpawnSprite(Point + D * 10.f, FRandRange(1.2,2.4), 0.8f, BloodFresh * 0.7f, 0.95f, FRandRange(0.35,0.6),
      ((Kind == Flesh || Kind == Blade ? -D * 0.3f : D) + VRand() * 0.55f).GetSafeNormal() * FRandRange(220,560), false, 1.f, -980.f, 0.6f)`.
      (`BloodFresh * 0.8f` endrer også alfa; lag fargen som `FLinearColor(R, G, B)` hvis det gir rare resultater.)
    - **Sprut bak** (`Through`, `Head`, `Blast`): rekkevidde `R = Head ? 380 : Blast ? 260 : 320`; kjegle `Cone = Blast ? 35 : 13` grader.
      Antall store: `Head ? 5 : 2 + RoundToInt(3 * S)`. For hver `I`: `Dd = I == 0 ? D : FMath::VRandCone(D, DegreesToRadians(Cone))`;
      `Trace(Point + D * 5.f, Point + Dd * R, Hit)`; ved treff:
      `Far = Hit.Distance / R`; `Sz = FMath::Lerp(24.f, 7.f, Far) * FRandRange(0.7f, 1.3f) * (0.6f + 0.8f * S) * (Kind == Head ? 1.5f : 1.f)`;
      `Stretch = FMath::Clamp(1.f / FMath::Max(0.28f, FMath::Abs(Dd | Hit.ImpactNormal)), 1.f, 3.5f)`;
      `SpawnBloodStain(World, Hit.ImpactPoint, Hit.ImpactNormal, Sz, 0.08f, FRandRange(100,140), Dd, Stretch, Hit.Distance / 2500.f)`.
      Så fine prikker rundt: `3 + RoundToInt(4 * S)` stk med kjegle `Cone + 16`, størrelse `FRandRange(1.5f, 4.5f)`, samme kall.
    - **Dråper på gulvet** (alle typer unntatt `Armor`): `N = 1 + RoundToInt(3 * S)` (`Blast`: +4, `Head`: +2). For hver:
      `Off = FVector(FRandRange(-35,35), FRandRange(-35,35), 0) + FVector(D.X, D.Y, 0.f) * FRandRange(0,70)`;
      `Trace(Point + Off, Point + Off - FVector(0,0,260), Hit)`; ved treff og `Hit.ImpactNormal.Z > 0.5f`:
      `SpawnBloodStain(World, Hit.ImpactPoint, Hit.ImpactNormal, FRandRange(3,9) * (0.7f + 0.6f * S), 0.15f, FRandRange(90,130),
      FVector::ZeroVector, 1.f, FMath::Sqrt(2.f * Hit.Distance / 980.f))`.
    - Maks per treff blir ca. 13 sprites, 21 linjespor og 21 flekker (hodeskudd med full styrke). Ikke øk tallene.
12. `SpawnBloodPool(World, Location, Normal, Size, Body)`:
    - Tilpass gulvet: `R = Size * 0.5f`. Opptil 4 forsøk: sjekk 4 punkter `Location ± X * R * 0.8f`, `± Y * R * 0.8f` (X/Y = to
      vinkelrette vektorer i planet): linjespor (`ECC_Visibility`, ignorer `Body`) fra `P + Normal * 8` til `P - Normal * 8`. Mangler gulv
      under ett av dem: `R *= 0.7f` og prøv igjen. Etter 4 forsøk brukes det som er (minst 18 cm).
    - Hovedpøl: `SpawnBloodStain(World, Location, Normal, R * 2.f, 16.f, 85.f)`.
    - To lapper som gjør formen ujevn: forskyvning `FRandRange(0.25f, 0.45f) * R` i tilfeldig retning i planet, størrelse
      `R * FRandRange(0.9f, 1.3f)`, gro-tid `FRandRange(10,20)`, liv 85, `Stretch = FRandRange(1.f, 1.6f)`, `Along` = forskyvningsretningen.

### D. Figuren (`GSCharacter.h` / `GSCharacter.cpp`)
13. `GSCharacter.h`:
    - Ved siden av `IncomingAmmoType` (linje ~79): `bool bIncomingMelee = false;   // server: the hit being applied is a blade/fist`
    - Ved `MulticastMeleeFX`:
      ```cpp
      /** Everyone: this body was just hit here, from this direction (GSWound kind, strength 0..255). */
      UFUNCTION(NetMulticast, Unreliable) void MulticastWoundFX(FVector_NetQuantize Point, FVector_NetQuantizeNormal Dir, uint8 Kind, uint8 Strength);
      void ApplyDeathImpulse();
      void SpawnDeathPool();
      ```
    - Private felt (ved `LastDamageWeapon`):
      ```cpp
      // Local on every machine: the last wound this body took (ragdoll push, where the pool forms)
      FVector LastWoundPoint = FVector::ZeroVector;
      FVector LastWoundDir = FVector::ZeroVector;
      float LastWoundTime = -100.f;
      uint8 LastWoundKind = 0;
      float LastWoundStrength = 0.f;
      bool bDeathImpulseDone = false;
      FTimerHandle DeathPoolTimer;
      // Server: at most 3 wound effects per frame per body (a shotgun blast is 8-9 hits at once)
      uint64 WoundFrame = 0;
      int32 WoundsThisFrame = 0;
      ```
14. `DoMeleeHit`: rundt `Victim->TakeDamage(...)`: `Victim->bIncomingMelee = true;` før og `Victim->bIncomingMelee = false;` etter
    (samme mønster som `IncomingAmmoType` i `ServerFire`).
15. `TakeDamage` – **to små tillegg, ingenting flyttes**:
    a. Øverst sammen med `bHeadshot`: `FVector WoundPoint = GetActorLocation() + FVector(0.f, 0.f, 30.f); FVector WoundDir = FVector::ZeroVector; bool bPointHit = false;`
       og inni den eksisterende `if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))`-blokka (rett etter `PE`):
       `WoundPoint = PE.HitInfo.ImpactPoint; WoundDir = FVector(PE.ShotDirection); bPointHit = true;`
    b. Rett **etter** `ClientTookHit`-blokka og **før** `LastDamageWeapon = WeaponName;`:
       ```cpp
       // Everyone nearby sees the wound. Only real hits: hunger, thirst and poison have no DamageCauser.
       if (DamageCauser)
       {
       	const float Lost = HealthBefore - Health;
       	if (!bPointHit) WoundDir = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
       	if (WoundDir.IsNearlyZero()) WoundDir = FVector::DownVector;   // own grenade at your feet
       	uint8 Wound = GSWound::Blast;
       	if (bArmorHit && Lost < 12.f) Wound = GSWound::Armor;
       	else if (bHeadshot) Wound = GSWound::Head;
       	else if (bIncomingMelee) Wound = GSWound::Blade;
       	else if (bPointHit) Wound = Lost >= 18.f ? GSWound::Through : GSWound::Flesh;
       	if (Lost > 0.f || bArmorHit)
       	{
       		if (WoundFrame != GFrameCounter) { WoundFrame = GFrameCounter; WoundsThisFrame = 0; }
       		if (++WoundsThisFrame <= 3)
       		{
       			MulticastWoundFX(WoundPoint, WoundDir, Wound, static_cast<uint8>(FMath::Clamp(Lost / 60.f, 0.f, 1.f) * 255.f));
       		}
       	}
       }
       ```
       Den står **før** `Die(...)` kalles lenger ned: på verten kjøres multicasten lokalt med en gang, så `LastWound*` er
       satt når `OnRep_Dead` kjører. Sjekk at ingen `return` ligger mellom (Føniks-greina returnerer ikke).
16. Ny `MulticastWoundFX_Implementation` (legg den rett etter `MulticastMeleeFX_Implementation`):
    ```cpp
    void AGSCharacter::MulticastWoundFX_Implementation(FVector_NetQuantize Point, FVector_NetQuantizeNormal Dir, uint8 Kind, uint8 Strength)
    {
    	if (GetNetMode() == NM_DedicatedServer) return;
    	LastWoundPoint = Point;
    	LastWoundDir = FVector(Dir);
    	LastWoundKind = Kind;
    	LastWoundStrength = Strength / 255.f;
    	LastWoundTime = GetWorld()->GetTimeSeconds();
    	const bool bOwnView = IsLocallyControlled() && !bDead;
    	AGSShortFX::SpawnBloodHit(GetWorld(), LastWoundPoint, LastWoundDir, Kind, LastWoundStrength, this, bOwnView);
    	// The one who was hit already has his own 2D sounds (ClientTookHit); everyone else hears it from the body.
    	if (!bOwnView)
    	{
    		const TCHAR* Snd = Kind == GSWound::Armor ? TEXT("Hit_Armor") : Kind == GSWound::Head ? TEXT("Hit_Head") : TEXT("Hit_Body");
    		GSAudio::PlayAt(this, Snd, LastWoundPoint, 0.55f, 14.f, FMath::FRandRange(0.9f, 1.1f));
    	}
    	// The death may have reached this machine before the wound that caused it.
    	if (bDead) ApplyDeathImpulse();
    }
    ```
17. `OnRep_Dead`: bytt **bare** linja `Skel->AddImpulse(GetActorForwardVector() * -4000.f + FVector(0.f, 0.f, 1500.f), NAME_None, false);`
    med `ApplyDeathImpulse();`. Helt nederst i funksjonen (etter `ApplyGearVisuals();`):
    ```cpp
    // The pool forms once the body has come to rest.
    GetWorldTimerManager().SetTimer(DeathPoolTimer, this, &AGSCharacter::SpawnDeathPool, 1.6f, false);
    ```
18. Nye funksjoner rett etter `OnRep_Dead`:
    ```cpp
    void AGSCharacter::ApplyDeathImpulse()
    {
    	// A bullet does not throw a body: a small push where it hit, and the body collapses. Blasts push harder.
    	if (!bDead || !bHumanBody || bDeathImpulseDone) return;
    	if (GetWorld()->GetTimeSeconds() - LastWoundTime > 0.8f) return;   // no hit (hunger...) or the wound has not arrived yet
    	bDeathImpulseDone = true;
    	USkeletalMeshComponent* Skel = GetMesh();
    	if (LastWoundKind == GSWound::Blast)
    	{
    		Skel->AddImpulse((LastWoundDir + FVector(0.f, 0.f, 0.35f)) * (3000.f + 9000.f * LastWoundStrength), NAME_None, false);
    	}
    	else
    	{
    		Skel->AddImpulseAtLocation(LastWoundDir * (LastWoundKind == GSWound::Blade ? 2200.f : 2800.f), LastWoundPoint);
    	}
    }

    void AGSCharacter::SpawnDeathPool()
    {
    	if (!bDead || GetNetMode() == NM_DedicatedServer) return;
    	if (GetWorld()->GetTimeSeconds() - LastWoundTime > 8.f) return;   // not a violent death: no blood
    	FVector From = GetActorLocation() - FVector(0.f, 0.f, 60.f);
    	if (bHumanBody) From = GetMesh()->GetSocketLocation(LastWoundKind == GSWound::Head ? TEXT("head") : TEXT("spine_02"));
    	FHitResult Floor;
    	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSDeathPool), true, this);
    	if (GetWorld()->LineTraceSingleByChannel(Floor, From + FVector(0.f, 0.f, 30.f), From - FVector(0.f, 0.f, 160.f), ECC_Visibility, Params)
    		&& !Cast<APawn>(Floor.GetActor()))
    	{
    		AGSShortFX::SpawnBloodPool(GetWorld(), Floor.ImpactPoint, Floor.ImpactNormal, FMath::FRandRange(95.f, 135.f), this);
    	}
    }
    ```
    `ApplyDeathImpulse` kalles i `OnRep_Dead` **etter** at ragdollen er slått på (`SetSimulatePhysics(true)`, `WakeAllRigidBodies()`).
19. `Die`: bytt `SetLifeSpan(15.f);` med `SetLifeSpan(BodyLifeSeconds);` og legg i `GSCharacter.h` (ved `WalkSpeed`):
    `static constexpr float BodyLifeSeconds = 90.f;`. Rett før `SetLifeSpan` i `Die`:
    ```cpp
    // Never more than 12 bodies lying around: the oldest goes first.
    {
    	int32 Bodies = 0;
    	AGSCharacter* Oldest = nullptr;
    	for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It)
    	{
    		if (!It->bDead || *It == this || It->GetLifeSpan() <= 0.f) continue;
    		++Bodies;
    		if (!Oldest || It->GetLifeSpan() < Oldest->GetLifeSpan()) Oldest = *It;
    	}
    	if (Bodies >= 12 && Oldest) Oldest->SetLifeSpan(0.5f);
    }
    ```
    (`EngineUtils.h` er allerede inkludert i `GSCharacter.cpp`.)
    Grep etter andre steder som antar 15 s: `grep -n "15.f" Source/Gloshaugen/Private/GSCharacter.cpp Source/Gloshaugen/Private/GSPlayerController.cpp`
    og les `DeathCamBody`-bruken i `GSPlayerController.cpp` – dødskameraet må fortsatt slippe kroppen ved respawn (4 s), ikke
    når kroppen forsvinner. Endre ingenting der uten at det faktisk er koblet til levetiden; skriv i rapporten hva du fant.

### E. Bygg og logg
20. Bygg med kommandoen i `CLAUDE.md` (editoren lukket). Rett feil ved årsaken.
21. `IMPROVEMENTS.md`: ny overskrift øverst (over «Runde 7») med linjene under.
22. Commit (på grenen, ikke `main`): `Blood: wound spray, spatter behind the victim, growing pools, bodies stay and fall the way they were hit`.

## Ferdig når
- `Build.bat` går gjennom uten feil; `python -m py_compile Content/Python/gs_materials.py` gir ingen feil.
- Kode-sjekk: `MulticastWoundFX` kalles bare på serveren, bare når `DamageCauser` finnes, maks 3 ganger per bilde per offer,
  og alltid før `Die`. Ingen blodkode kjører på dedikert server.
- Kode-sjekk: `OnRep_Dead` inneholder ikke lenger det faste støtet; `ApplyDeathImpulse` kan kalles i begge rekkefølger
  (død før sår, sår før død) og virker bare én gang.
- I spillet (Andreas, to spillere / «Number of Players: 2», etter kampstart – i lobbyen tar ingen skade):
  skyt den andre foran en vegg → rød sky, mørkerøde flekker på veggen bak og dråper på gulvet; skyt på skrå → avlange
  flekker; vest → støv i stedet for blod; drep ham → kroppen segner i skuddretningen, en pøl vokser under den i ca.
  15 s, og kroppen ligger i 90 s. Dø av sult → ingen pøl.
- Første gang editoren åpnes etter runden står «Gloshaugen: bygger blodmateriale» i Output Log, og
  `Content/GS/Materials/M_GS_Blood` finnes. Uten materialet vises flate, mørkerøde skiver (reserve).

## Risiko
- **Rekkefølge online** (lærdom runde 1): multicasten er upålitelig og kan komme før/etter `bDead`, eller mistes. Håndtert:
  støt og pøl bruker «siste sår» med tidsvindu; mistes pakken blir det bare ingen støt/pøl på den maskinen. Ingen
  spilltilstand avhenger av dette (rent kosmetisk, ingenting nytt replikeres).
- **Kropper i 90 s**: flere ragdoller og pawn-aktører samtidig. Tak på 12. Sjekket i koden: alle løkker over `AGSCharacter`
  hopper over `bDead` (GameMode 235, HUD 433, PlayerController 474, Projectile 156/189), og `ApplyFlash` returnerer på `bDead`.
- **Ytelse**: hver flekk er én aktør med eget materiale. Tak på 220, små flekker tegnes ikke over 25 m, flekker ticker to
  ganger i sekundet etter at de har lagt seg. Automatild på én spiller: maks 3 sår per bilde per offer.
- **Materialet kan ikke testes ved kompilering**: Python-koden følger `_fx_master` linje for linje, er pakket i
  `try/except`, og C++ har reserve. Ser kanten for rund eller for hakkete ut, justeres `scale`/`output_min` i en senere
  runde (da må `M_GS_Blood` slettes i Content Browser så den bygges på nytt, eller få en egen versjonssjekk).
- **Flimring (z-fighting)** mellom flekk og vegg på lang avstand: derfor 0,25–0,45 cm avstand og tegneavstand-grense.
- **Flekker i lufta**: sjekket at `AGSPickup` (ingen kollisjon) og `AGSProjectile` (ignorerer `ECC_Visibility`) ikke fanger
  sporene; spillere hoppes over med `APawn`-sjekken. Dører som flytter seg finnes ikke ennå.
- **`TakeDamage` ble endret i runde 2** og kan få rettinger fra kontrolløren. Tilleggene her er to rene innskudd og flytter
  ingen linjer.
- **Lydene `Hit_Head`/`Hit_Armor`** er laget for offeret (2D). Som 3D-lyd på 14 m kan de bli for tydelige; volum 0,55 er
  satt forsiktig. Egen «våt» trefflyd står i backlogen.

## IMPROVEMENTS-linjer
```
## Runde 8: blod og kropper (26. nov.)

313. Skudd som treffer en spiller gir en sky av blod ved såret, synlig for alle i nærheten
314. Kuler som går gjennom kroppen spruter blod på veggen eller gulvet bak, opptil tre–fire meter unna
315. Blodspruten blir avlang når den treffer veggen på skrå, og mindre jo lenger unna veggen er
316. Bloddråper faller og lager flekker på gulvet under den som blir truffet
317. Hodeskudd gir mer blod i lufta og større sprut bak
318. Stopper vesten eller hjelmen kula, spruter det støv og fiber i stedet for blod
319. Eksplosjoner og nærkampvåpen gir egne blodmønstre
320. En blodpøl vokser sakte under døde kropper, under hodet ved hodeskudd, og renner ikke utover kanter og trapper
321. Blodet er blankt og vått med ujevn kant, tørker inn og blir mørkere, og forsvinner til slutt
322. Dør du av sult, tørst eller gift blir det ikke noe blod
323. Kropper blir liggende i 90 sekunder i stedet for 15
324. Kroppen segner sammen den veien kula traff i stedet for å bli kastet bakover og opp i lufta
325. Alle i nærheten hører når en kule treffer en kropp eller en vest, ikke bare den som blir truffet
326. Tak på antall blodflekker og kropper, så spillet ikke hakker i store skuddvekslinger
```
