# Plan 07 – runde 13 (lag Kamp): blødning, blodspor og sår på kroppen; frakobling og Esc-menyen

Bygges etter at runde 12 (`ko/plan-06.md`, commit `4d74e7c`) er kontrollert. Rettinger fra den kontrollen tas først.
Runde 12 endret skytefunksjonene, `GSBallistics`, lysspor i `GSEffects` og `DrawScope`; denne runden rører **ikke**
dem. Bruk funksjonsnavn, ikke linjenumre. Les koden slik den står; avviker den fra det som står her, følg koden og
skriv det i rapporten.

## Mål
Et sår blør til du bandasjerer det: helsa renner ut, du drypper blod på bakken som andre kan følge, og kula setter en
blodflekk på kroppen der den traff. Den som kobler fra midt i kampen dør der han står, og Esc-menyen fører til den
nye hovedmenyen.

## Forbedrer (gammelt)
- **Bandasje, plaster og førstehjelpsskrin** får sin egentlige oppgave: de stopper blødning (i dag gir de bare helse
  og leger lemmer).
- **Hjerteslaget** følger blodtapet, ikke bare lite liv.
- **Blodpølen under liket** kommer også når man blør i hjel lenge etter skuddet (i dag bare ved drap innen 8 s).
- **Dødsårsak og drap ved «tappe»-skade** (Spillbarhets S3, Kamps retting 9): brannammo gir drapet til skytteren;
  dødskortet etter blødning bruker der skuddet kom fra, aldri skytterens nye kropp.
- **Esc-menyen** (bestilling fra Spillbarhet): «Hovedmeny» i stedet for «Start server»/«Bli med», Avslutt via
  bekreftelsen i `GSMenu`, konsoll-linja bort, riktig grafikknivå, stille bildefrekvens for menyen.
- **Hinttekstene** i `DrawPrompt` og «Trykk R» viser spillerens egne taster. **Ammoesker**: påbegynt eske regnes brukt (funn 13).

## Nytt
- **Blødning** (replikert): mest ved mageskudd, små sår stopper av seg selv, store gjør det ikke.
- **Blodspor**: dråper på bakken etter den som blør, med eget tak per maskin.
- **Sår på kroppen**: blodflekk festet til beinet (bone) der kula traff, følger animasjon og ragdoll.
- **`PawnLeavingGame`**: den som kobler fra i kamp dør, utstyret og flagget blir liggende, siste skytter får drapet.

## Hvorfor nå
Andreas: «veldig groteskt» med blodspor og sår, og godkjent ønske om skader («blødning som må bandasjeres»). Backlogens
neste skade-punkt etter ballistikken. De små bestillingene (C1-frakobling, Esc-meny, grafikknivå, stille
bildefrekvens, taster i hint) ligger åpne hos Kamp og henger sammen med CTF og den nye menyen. V8 (tilbakespoling) er
runde 14.

## Avgrensning (ikke i denne runden)
V8, N1 (nettrelevans), bærer-punkt på kartet (venter på `AGSFlag::bRevealed` fra Spillbarhets plan-04 – ikke bygget
ennå), blod på egne hender, lik som kan treffes (R15), egne `Kind`-gjenstander (tourniquet/skinne – Spillbarhet).

## Filer
| Fil | Endring |
|---|---|
| `Public/GSCharacter.h`, `Private/GSCharacter.cpp` | blødning, spor, sår, S3, `DieLeavingGame`, `Die`, `OnRep_Health`, hjerteslag, ammoeske |
| `Public/GSEffects.h`, `Private/GSEffects.cpp` | `SpawnBloodDrip` (eget tak), `MakeBodyWound` |
| `Private/GSHUD.cpp` | `DrawBars` (blødningstekst), `DrawMenu`, `DrawPrompt`/«Trykk R» |
| `Public/GSPlayerController.h`, `Private/GSPlayerController.cpp` | `PawnLeavingGame`, `MenuClick`, `GetGraphicsLevel`, `ApplyGraphicsLevel`, `SetFrameLimit` |
| `IMPROVEMENTS.md` (500–515), `.claude/lag/bestillinger.md` (status + to linjer) | |

Ikke rør `GSTypes.h`, `GSItems.*`, `catalog.py`, `GSMenu.*`, `GSHelp.*`, `GSGameMode.*`, `GSFlag.*`, `GSBallistics.*`.
Stage filene med navn.

## Steg

### 0. Forberedelse
1. `git log --oneline -5` og `git status`: Spillbarhet bygger samtidig i `GSHelp`/`GSMenu`/`GSGameMode` – ikke rør dem.
   `Get-Process UnrealEditor`: kjører editoren, ikke bygg. Gjør `GSCharacter.h`, `GSEffects.h` og
   `GSPlayerController.h` ferdige i ett.

### A. Blødning på serveren (`GSCharacter.*`)
2. `GSCharacter.h`:
   - public: `UPROPERTY(Replicated) uint8 BleedLevel = 0;` – blødning i trinn på 1/32 HP/s (0 = blør ikke, 128 = 4 HP/s),
     til **alle** (andre trenger det for blodsporet). `float GetBleedRate() const { return BleedLevel / 32.f; }`.
     `void DieLeavingGame();` (se D).
   - protected/private (server): `float Bleeding = 0.f;` (HP/s), `float BleedAccum = 0.f;`,
     `TWeakObjectPtr<AController> BleedInstigator; FString BleedWeapon; TWeakObjectPtr<APawn> BleedShooterPawn;
     FVector BleedFrom = FVector::ZeroVector; bool bHasBleedFrom = false; bool bDyingFromBleed = false; bool bLeftGame = false;`
     `TWeakObjectPtr<AController> BurnInstigator; FString BurnWeapon;` og `void TickBleeding(float Dt); void StopBleeding(float Amount);`
   - `DOREPLIFETIME(AGSCharacter, BleedLevel);` i `GetLifetimeReplicatedProps`.
3. Konstanter (anonymt namespace øverst i `GSCharacter.cpp`, **navngitt** `GSBleed` – lærdom om unity-bygg):
   ```cpp
   namespace GSBleed
   {
       // HP/s of bleeding per HP the hit took. Belly worst (no clotting to speak of), arm least.
       constexpr float Stomach = 0.065f, Chest = 0.04f, Leg = 0.045f, Arm = 0.035f, Head = 0.04f, Blast = 0.025f, Melee = 0.04f;
       constexpr float Max = 4.f;        // HP/s
       constexpr float Clot = 0.02f;     // HP/s per second: a wound slows by itself; total loss untreated = B^2 / (2 * Clot)
   }
   ```
   Regnet mot katalogen (uten vest, før skadefall): AK-47 i brystet 34 → 1,36 HP/s → 46 HP tapt uten bandasje (overlever
   med ca. 20); AK i magen 31 → 2,0 HP/s → 100 HP (dør på ca. 45 s uten bandasje); AK i beinet 22 → 1,0 → 25 HP; MP5 i
   brystet 21 → 0,84 → 18 HP; AK i armen 19 → 0,66 → 11 HP. Ingen treff-til-drap endres: blødning kommer bare i tillegg
   etter skuddvekslingen. Skriv tabellen inn i rapporten hvis tallene i katalogen avviker.
4. `TakeDamage`, rett etter limb-blokken (`if (Health > 0.f && HealthBefore > Health)`), i samme `if`:
   ```cpp
   // An open wound keeps bleeding. Not from fire, and not when the plate/helmet stopped it.
   const float Lost = HealthBefore - Health;
   if (!bBurn && (bBullet || DamageCauser) && !(bArmorHit && Lost < 12.f))
   {
       const float K = bIncomingMelee ? GSBleed::Melee : !bPointHit ? GSBleed::Blast
           : Zone == GSZone::Stomach ? GSBleed::Stomach : Zone == GSZone::Leg ? GSBleed::Leg
           : Zone == GSZone::Arm ? GSBleed::Arm : Zone == GSZone::Head ? GSBleed::Head : GSBleed::Chest;
       Bleeding = FMath::Min(GSBleed::Max, Bleeding + Lost * K);
       BleedInstigator = EventInstigator;   // the last one who opened a wound gets the kill if you bleed out
       BleedWeapon = WeaponName;
       BleedShooterPawn = (Proj && !bShooterThrewIt) ? nullptr : Shooter;
       // BleedFrom / bHasBleedFrom: the same From/bHasFrom that ClientTookHit gets below (hoist them out of that block)
   }
   ```
   Sett `BleedLevel` gjennom én hjelper `void SyncBleedLevel()` (`uint8(FMath::Clamp(FMath::RoundToInt(Bleeding * 32.f), 0, 255))`,
   sett bare når verdien endres, så den ikke replikeres hver frame).
   Samme sted for **S3**: der `Ammo == GSAmmoType::Incendiary` legger på «Brennende»: `BurnInstigator = EventInstigator; BurnWeapon = WeaponName;`.
5. `TickBleeding(Dt)` (kalles fra `Tick` rett etter `TickSurvival`, samme `HasAuthority() && !bDead`-grein):
   - `GM && !GM->DamageEnabled()` (lobby, nedtelling, resultat): `Bleeding = 0; BleedAccum = 0; SyncBleedLevel(); return;`
   - `if (Bleeding <= 0.f) return;` `BleedAccum += Bleeding * Dt;` `Bleeding = FMath::Max(0.f, Bleeding - GSBleed::Clot * Dt);` `SyncBleedLevel();`
   - Hele poeng som i `TickSurvival` (`if (BleedAccum < 1.f) return;` trekk av `Trunc`), `OnRep_Health()`.
   - `Health <= 0`: Føniks som i `TickSurvival` (reis deg, `Bleeding = 0`). Ellers:
     ```cpp
     AController* K = BleedInstigator.Get();
     bDyingFromBleed = true;
     Die(K, false, K ? FString::Printf(TEXT("%s (blødning)"), *BleedWeapon) : FString(TEXT("Blødning")));
     ```
     `Die` håndterer selvdrap (egen granat) og lagdrap som før (`OnPlayerKilled`). **Ikke** nullstill `BleedLevel` i
     `Die`: klientene bruker den til blodpølen (steg 13).
6. `StopBleeding(Amount)`: `Bleeding = FMath::Max(0.f, Bleeding - Amount); if (Bleeding < 0.05f) Bleeding = 0.f; SyncBleedLevel();`
   I `CompleteUse`, `EGSItemCat::Medicine`-grenen (ved siden av limb-behandlingen), med en filstatisk tabell på Id –
   **ikke** på `Heal`, så Spillbarhet kan endre helse-tallene fritt:
   ```cpp
   // How much bleeding (HP/s) a dressing stops: a plaster small wounds, a bandage nearly all, a kit everything.
   static float GSBleedStopFor(const FGSItemDef& D)
   {
       if (D.Kind == FName(TEXT("tourniquet"))) return 99.f;   // agreed name for Spillbarhet's coming item
       if (D.Id == FName(TEXT("Plaster"))) return 0.6f;
       if (D.Id == FName(TEXT("Bandage"))) return 2.5f;
       if (D.Id == FName(TEXT("Medkit")) || D.Id == FName(TEXT("SurgeryKit"))) return 99.f;
       return 0.f;   // pills, painkillers: no
   }
   ```
   Med bleed over 0 og ingen ting som stopper: `ClientToast(TEXT("Det stopper ikke blødningen"))` bare når `GSBleedStopFor == 0`
   og varen er medisin. Når en bandasje stopper alt: `ClientToast(TEXT("Blødningen har stoppet"))`; ellers
   `ClientToast(TEXT("Blødningen er mindre"))`.
7. **S3** i `TickSurvival`, der `Health <= 0` og `Die(nullptr, false, Cause)`: velg i rekkefølge
   Sult → Tørst → effekten «Brennende» aktiv → `Die(BurnInstigator.Get(), false, BurnInstigator.IsValid() ? BurnWeapon + TEXT(" (brann)") : FString(TEXT("Brann")))`
   → «Matforgiftning» aktiv → `"Matforgiftning"` → ellers `"Overdose"`. (Finn effekt-id-ene i katalogen/`FxPoison`.)
8. `Die`, **retting 9** og frakobling:
   - etter `KillingProjectile`-linja som nuller `KillerPawn`: `if (bDyingFromBleed && KillerPawn != BleedShooterPawn.Get()) KillerPawn = nullptr;`
     (skytteren kan ha fått ny kropp eller stå et annet sted nå; da vises ikke hans helse).
   - utvid `else if (KillingProjectile && …)`: `else if (bDyingFromBleed && bHasBleedFrom && !Info.Killer.IsEmpty())` →
     `KillerLocation = BleedFrom`, `bHasKillerLocation = true`, `DistanceM` fra `BleedFrom`.
     NB: med `KillerPawn` satt brukes hans posisjon nå (som før for vanlige drap) – greit når det er samme kropp.
   - `GM->OnPlayerKilled(...)` bare `if (!(bLeftGame && !Killer))` (ingen «Verden»-linje i tillegg til Spillbarhets «forlot kampen»).
   - `ClientDeathCam` og `SetViewTarget` bare `if (VictimPC && !bLeftGame)`.

### B. Det spilleren merker selv (`GSCharacter.cpp`, `GSHUD.cpp`)
9. `OnRep_Health`: en nedgang på høyst 3 HP mens `BleedLevel > 0` er blødning: oppdater `LastSeenHealth`, men **ikke**
   `LastDamageTakenTime`, dunk-lyd eller rødt blink (ellers dunker det og kameraet rykker hvert sekund). Sjekk at
   `ClientTookHit` fortsatt gir treff-lyd/blink for ekte treff mens man blør.
10. Hjerteslag i `UpdateLocalVisuals`: `const float BleedK = FMath::Clamp((GetBleedRate() - 0.8f) / 2.f, 0.f, 1.f);`
    `const float Beat = FMath::Max(Low, BleedK * 0.7f);` – slå når `Beat > 0`, volum `0.35f + 0.5f * Beat`, intervall
    `Lerp(1.05f, 0.55f, Beat)`. `ApplyLowHealthPostProcess` og HUD-kanten følger fortsatt bare `Low`.
11. `GSHUD::DrawBars`, i blokka for skadde lemmer (samme farge og størrelse), øverst:
    `if (C->GetBleedRate() > 0.05f) { Text(C->GetBleedRate() >= 1.2f ? TEXT("Du blør kraftig") : TEXT("Du blør"), X, IY, Col, 0.75f); IY -= 18.f; }`
    Nøkternt: ingen ikon, ikke blinkende.

### C. Blodspor og sår på kroppen (alle maskiner unntatt dedikert server)
12. `GSEffects.h/.cpp`:
    - `SpawnBloodStain` får en siste parameter `bool bTrail = false`. Med `bTrail` brukes en egen liste `GSTrail` med
      `MaxTrail = 90` (samme «eldste først»-logikk), så blodsporet aldri tar plassen til sprut på vegger og pøler.
    - `static void SpawnBloodDrip(UWorld* World, const FVector& From, float Size, const AActor* Victim);` – spor rett ned
      (`ECC_Visibility`, ignorer offeret, ikke pawns, `ImpactNormal.Z > 0.5`, 250 cm), så
      `SpawnBloodStain(World, Hit.ImpactPoint, Hit.ImpactNormal, Size, 0.12f, FMath::FRandRange(70.f, 100.f), FVector::ZeroVector, 1.f, FMath::Sqrt(2.f * Hit.Distance / 980.f), true)`.
    - `static UStaticMeshComponent* MakeBodyWound(AActor* Owner, float Size);` – ny `UStaticMeshComponent` (`NewObject`,
      `RegisterComponent`) med `GSFXPlane()` og en MID av `GSBloodMaterial()` (farge mørkere enn `BloodFresh`, roughness
      0,25, `Soft` 0,6), skala `Size / 100`, ingen kollisjon, ingen skygge, `SetCullDistance(3000)`,
      `bAffectDynamicIndirectLighting = false`. Uten materialet: flat sylinder i mørk rød (som reserven i `SpawnBloodStain`).
      Komponenten tilhører eieren, så den forsvinner med kroppen. Ingen tick.
13. `GSCharacter.h`: `UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> BodyWounds;`, `FName BleedBone;`,
    `float NextDripIn = 0.f;`, `void AddBodyWound(const FVector& Point, const FVector& Dir, uint8 Kind); void TickBloodTrail(float Dt);`
    - `MulticastWoundFX_Implementation`, etter `SpawnBloodHit`: `if (bHumanBody && Kind != GSWound::Armor && Kind != GSWound::Blast) AddBodyWound(LastWoundPoint, LastWoundDir, Kind);`
    - `AddBodyWound`: punkt på huden med `GetMesh()->GetClosestPointOnPhysicsAsset(Point, CP, false)` (bruk `CP.ClosestWorldPosition`,
      `CP.Normal`, `CP.BoneName` når `CP.Distance < 25`). **Les motorkoden** (`SkeletalMeshComponentPhysics.cpp`): virker den
      uten fysikktilstand (levende kropp har `NoCollision`)? Gjør den ikke det: `FindClosestBone(Point, nullptr, 0.f, true)`,
      punktet selv og normal `-Dir`. Størrelse: hode 6, kjøtt 5–7, gjennom (`Through`) 7–10, kniv 8 cm. Ny komponent fra
      `MakeBodyWound`, `SetWorldLocationAndRotation(P + N * 0.3f, FRotationMatrix::MakeFromZX(N, tilfeldig tangent).Rotator())`,
      `AttachToComponent(GetMesh(), KeepWorldTransform, Bone)`, `SetOwnerNoSee(!bDead)`. Høyst 8 per kropp: den eldste
      `DestroyComponent()`. `BleedBone = Bone`.
    - `OnRep_Dead`: `SetOwnerNoSee(false)` på `BodyWounds` (som armbindene), så du ser sårene i dødskameraet.
    - `TickBloodTrail` (fra `Tick`, `GetNetMode() != NM_DedicatedServer && !bDead && BleedLevel > 0`):
      `NextDripIn -= Dt * (1.f + GetVelocity().Size2D() / 300.f)`; ved ≤ 0: nytt intervall
      `FMath::FRandRange(0.7f, 1.3f) * FMath::Clamp(1.6f / GetBleedRate(), 0.3f, 4.f)`. Bare når lokal kamera er innen 40 m
      (som `SpawnBloodHit` gjør). Fra: `bHumanBody && !BleedBone.IsNone() ? GetMesh()->GetSocketLocation(BleedBone) : GetActorLocation()`
      + tilfeldig 10 cm sideveis. Størrelse `FMath::FRandRange(2.5f, 5.f) * (0.8f + 0.25f * GetBleedRate())`. Egen kropp også
      (du ser dråpene når du ser ned).
    - `SpawnDeathPool`: hopp bare over når `Now - LastWoundTime > 8.f && BleedLevel == 0` (den som blødde i hjel får pøl).

### D. Frakobling (`GSPlayerController.*`, `GSCharacter.cpp`)
14. `AGSCharacter::DieLeavingGame()`:
    ```cpp
    if (!HasAuthority() || bDead) return;
    const float Now = GetWorld()->GetTimeSeconds();
    AController* Killer = nullptr; FString Weapon;
    if (Bleeding > 0.f && BleedInstigator.IsValid()) { Killer = BleedInstigator.Get(); Weapon = BleedWeapon; }
    else if (LastDamageInstigator.IsValid() && Now - LastDamageTakenTime <= 10.f) { Killer = LastDamageInstigator.Get(); Weapon = LastDamageWeapon; }
    if (Killer == GetController()) Killer = nullptr;
    bLeftGame = true;
    Die(Killer, false, Weapon);
    ```
15. `AGSPlayerController`: `virtual void PawnLeavingGame() override;` (sjekk at den er `virtual` i 5.8 og hvem som kaller
    den: `APlayerController::Destroyed`, bare på serveren når forbindelsen er borte).
    ```cpp
    void AGSPlayerController::PawnLeavingGame()
    {
        AGSCharacter* C = Cast<AGSCharacter>(GetPawn());
        const AGSGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
        if (C && !C->bDead && GM && GM->DamageEnabled() && !GetWorld()->bIsTearingDown)
        {
            C->DieLeavingGame();   // the body falls and stays, gear and flag on the ground, the last shooter gets the kill
            return;                // not Super: that would destroy the body
        }
        Super::PawnLeavingGame();
    }
    ```
    Les i motoren hva `DetachFromControllerPendingDestroy` → `PawnPendingDestroy` gjør med en kontroller som er midt i
    `Destroyed` (ingen krasj, `SetPawn(nullptr)` skjer). Spillbarhets `Logout` kommer etterpå og legger ned flagget hvis
    det fortsatt bæres (det gjør det ikke når `Die` har sluppet det) og skriver «X forlot kampen».

### E. Esc-menyen og innstillinger (`GSHUD.cpp`, `GSPlayerController.*`)
16. `DrawMenu`: bytt knappene `Host` og `Join` med **én** `MenuButton(TEXT("Host"), TEXT("Hovedmeny"), …)`, og fjern
    teksten «Annen adresse: trykk § …». «Esc lukker menyen» står.
17. `MenuClick` (`#include "GSMenu.h"`): `Host`/`Join` → `CloseMenu(); GSMenu::OpenMain(this);`, `Quit` → `CloseMenu(); GSMenu::AskQuit(this);`.
    (`GSMenu` fanger i dag de samme klikkene selv – begge veier gjør nå det samme, så det er trygt.) `Host()`/`Join()` står (konsoll og `GSMenu` bruker dem).
18. Grafikk: `GetGraphicsLevel()` → `FMath::Clamp(S->GetShadowQuality(), 0, 4)` (som `GSMenu`; totalnivået gir −1 med egen
    oppløsning). Ny public `void ApplyGraphicsLevel(int32 Level)`: husk `GetResolutionPercent()`, `SetOverallScalabilityLevel`,
    sett oppløsningen tilbake, `ApplySettings(false)`, `SaveSettings()` – uten chatlinje. `Grafikk` (konsoll) = `ApplyGraphicsLevel` + meldingen.
    `MenuClick("Graphics")` → `ApplyGraphicsLevel((GetGraphicsLevel() + 1) % 5)`.
19. Ny public `void SetFrameLimit(int32 Fps)` = de to første linjene i `Bildefrekvens` (uten `ClientMessage`); `Bildefrekvens` kaller den og skriver meldingen.

### F. Små rettelser
20. `DrawPrompt`: de tre «[E]» og «Trykk R for å lade» bruker `GSMenu::KeyName(PlayerOwner, TEXT("IA_Interact"))` /
    `TEXT("IA_Reload")` (sjekk formatet KeyName gir; behold hakeparentesene rundt). PC-/printer-/kamera-tekstene inne i
    terminalen står (faste taster der).
21. `TakeFromPickup`, ammo: `UsedBoxes = FMath::DivideAndRoundUp(Before - Rounds, Def->Rounds)` (funn 13; `LeftBoxes` kan ikke bli negativ – klem til 0).

### G. Logg og bestillinger
22. `IMPROVEMENTS.md`: seksjonen under, øverst.
23. `bestillinger.md`: merk som LEVERT i Kamps runde 13: C1/`PawnLeavingGame` (punkt 1 i plan-03/04-bestillingen), Esc-meny
    (1)–(2) og hint (3) for `DrawPrompt`, grafikknivå + stille bildefrekvens, ammoesken. Ny linje øverst
    Kamp → Spillbarhet: «(1) Blødning finnes: `AGSCharacter::BleedLevel` (replikert til alle, /32 = HP/s) og `GetBleedRate()`.
    Hint i `GSHelp` når `GetBleedRate() > 0.3`: «Du blør. Bruk bandasje for å stoppe det.» med `KeyName(IA_UseItem)`.
    (2) Hva som stopper blødning ligger i en Id-tabell i `GSCharacter.cpp` (plaster 0,6 HP/s, bandasje 2,5, skrin og
    feltkirurgsett alt), uavhengig av `Heal` – endre `Heal` fritt. Ny gjenstand med `Kind = "tourniquet"` stopper all
    blødning uten mer kode. (3) Menyen kan bruke `ApplyGraphicsLevel` og `SetFrameLimit` (stille), og trenger ikke lenger
    fange `Host`/`Join`/`Quit` i Esc-menyen. (4) Den som kobler fra i kamp dør nå i `PawnLeavingGame` før `Logout`; uten
    skytter de siste 10 s skrives ingen drapslinje fra Kamp, så deres «forlot kampen» står alene.»

## Ferdig når
- `Build.bat … -WaitMutex` uten feil (eller «Target is up to date» etter byggerens bygg).
- To spillere (PIE, lytte-server + klient): skudd i magen gir «Du blør kraftig», helsa faller ca. 2 HP/s, dråper på bakken
  hos **begge** maskiner der den sårede går, blodflekk på kroppen der kula traff (følger når han går og når han faller).
- Bandasje stopper blødningen (tekst borte, helsa står); plaster stopper bare små sår; pille stopper ingenting.
- Blør han i hjel: drapslisten «Skytter [AK-47 (blødning)] Offer», skytteren får drapet, pøl under liket, dødskortet viser
  hvor skuddet kom fra. Ingen dunk/rødt blink per dråpe; hjerteslag ved kraftig blødning.
- Brannammo-drap: skytteren får drapet, årsak «… (brann)».
- Klienten lukker vinduet midt i kampen: kroppen faller og blir liggende med utstyret, flagget ligger på bakken, skytteren
  (hvis skadet siste 10 s) får drapet, ingen krasj på verten. I lobbyen: kroppen forsvinner som før.
- Esc: «Hovedmeny» åpner `GSMenu`, Avslutt spør først, Grafikk viser riktig nivå og oppløsningen står etter bytte.
- `grep -n "Annen adresse"` i `GSHUD.cpp` er tom. Hint viser endrede taster.

## Risiko
- **Blødning × dunk-lyd**: `OnRep_Health` tolker hver nedgang som skade (steg 9). Uten den rettelsen blir spillet uutholdelig.
- **Replikering**: `BleedLevel` endres hvert ~1,6 s under størkning – billig. Ikke repliker `Bleeding` som float.
- **Balanse**: blødning gjør hver skuddveksling dyrere. Bandasje er «Uncommon» i loot; blir det for sjelden, si fra i
  rapporten (Spillbarhets loot-runde). Clot-tallet er den ene knappen å skru på.
- **`GetClosestPointOnPhysicsAsset`** kan kreve fysikktilstand; reserven (nærmeste bein) må virke alene.
- **Sår på kroppen og første person**: festede komponenter arver ikke `OwnerNoSee`; sett det selv, ellers ser du flekker
  svevende foran kameraet.
- **`PawnLeavingGame` under `Destroyed`**: klient-RPC-er til en lukket forbindelse droppes (derfor ingen dødskamera);
  sjekk at `OnPlayerKilled` sin respawn-tidtaker tåler en kontroller som forsvinner (den bruker svak peker).
- **Kosmetikk-tak**: spor har egen liste (90), sår høyst 8 per kropp og bare på kropper med mannekeng; dråper bare innen 40 m.
- **Andre lag**: `GSMenu` fanger fortsatt Esc-klikkene – samme handling, ingen dobbel meny (sjekk at `OpenMain` to ganger ikke
  åpner to lag). Ingen endring i Spillbarhets filer.

## IMPROVEMENTS-linjer

## Runde 13 (Kamp): blødning, blodspor og sår på kroppen; frakobling og Esc-menyen (26. nov.)

Gammelt som er forbedret: bandasje og plaster (502), drap og dødsårsak ved blødning og brann (506–508), blodpølen (509), hjerteslaget og skadeblinket (510), Esc-menyen (513, 514), hinttekster og ammoesker (515).
Nytt: blødning (500, 501, 511), blodspor (503, 504), sår på kroppen (505), død ved frakobling (512).

500. Skudd- og knivsår blør: helsa renner sakte ut etter treffet, mest ved mageskudd, minst i armen
501. Små sår slutter å blø av seg selv etter en stund; et mageskudd gjør det ikke, og dreper på under ett minutt uten bandasje
502. Bandasje stopper blødningen, plaster stopper små sår, førstehjelpsskrin og feltkirurgsett stopper alt; piller hjelper ikke
503. Den som blør drypper blod på bakken der han går, så andre kan følge sporet; jo kraftigere blødning, jo tettere dråper
504. Blodsporet har sitt eget tak per maskin, så det aldri fjerner blodsprut på vegger eller pøler under lik
505. Blodflekk på kroppen der kula traff (hode, bryst, mage, arm eller bein), som følger kroppen når den går og faller
506. Den som blør i hjel gir drapet til den som skjøt ham, med våpenet og «(blødning)» i drapslisten
507. Dødskortet etter å ha blødd i hjel viser hvor skuddet kom fra, aldri skytterens nye kropp eller helse
508. Drap med brannammo går til skytteren med årsaken «brann», og matforgiftning heter matforgiftning, ikke «Overdose»
509. Den som har blødd i hjel blir liggende i en blodpøl, også når skuddet kom lenge før
510. Hjerteslaget begynner når du blør kraftig og slår fortere jo mer du blør; blødningen gir ikke dunk og rødt blink for hver dråpe
511. «Du blør» eller «Du blør kraftig» står nøkternt over helsestolpen
512. Den som kobler fra midt i en kamp dør der han står: utstyret og flagget blir liggende, og den som skadet ham de siste 10 sekundene får drapet
513. Esc-menyen har én knapp «Hovedmeny» i stedet for «Start server» og «Bli med», Avslutt spør før du forlater kampen, og konsoll-linja er borte
514. Grafikk i Esc-menyen viser riktig nivå og beholder oppløsningen din når du bytter; bildefrekvensen kan settes fra menyen uten chatlinje
515. Hinttekstene ved loot, PC, printer og lading viser tastene du selv har valgt, og en påbegynt ammoeske regnes som brukt (ingen gratis ammo)
