# Plan – runde 1: dødskamera, drapskort og tydelige treffmarkører

## Mål
Når du dør ser du kroppen din falle fra et kamera som trekker seg bakover og så snur seg mot den som drepte deg,
med et kort som sier hvem, med hva, fra hvor langt unna og hvor mye liv han hadde igjen; og når du treffer andre
ser du straks om det var kropp, vest/hjelm, hodeskudd eller drap.

## Hvorfor nå
- Kamp er kjernen i spillet, og tilbakemeldingen i de to viktigste øyeblikkene (du treffer / du dør) er i dag den
  svakeste delen: treffmarkøren er fire små røde prikker i 0,18 s uansett hva som skjer, og når du dør fryser
  bildet i øyehøyde bak en rød hinne i 4 sekunder (pawnen kobles fra kontrolleren i `Die()`, kameraet blir stående).
- Alt kan gjøres i C++ (ingen .uasset), og verifiseres med kompilering. Ingen nye ressurser trengs.
- Ingenting av dette finnes fra før (sjekket `GSHUD.cpp`, `GSCharacter.cpp`, `GSPlayerController.*`, `GSGameMode.cpp`).

## Filer
- `Source/Gloshaugen/Public/GSTypes.h` – ny struct `FGSDeathInfo`.
- `Source/Gloshaugen/Public/GSCharacter.h` / `Private/GSCharacter.cpp` – dødskamera (`CalcCamera`), ny
  `ClientHitMarker`-signatur, rustningstreff, kall til kontrolleren i `Die()`.
- `Source/Gloshaugen/Public/GSPlayerController.h` / `Private/GSPlayerController.cpp` – `ClientDeathCam`-RPC og
  lagret dødsinfo.
- `Source/Gloshaugen/Private/GSHUD.cpp` (+ `Public/GSHUD.h`) – nye treffmarkører, nytt dødskort med nedtelling.
- `IMPROVEMENTS.md` – nye linjer 289–300.

Ikke rør `Config/DefaultEngine.ini` eller noen usporede filer i `Content/`, `SourceArt/`, `Config/GSMeshOverrides.ini`,
`*.slnx`, `.vsconfig`. Commit kun filene over (stage dem eksplisitt med navn, aldri `git add -A`/`.`).

## Steg

### 0. Gren
1. Lag ny gren fra nåværende `claude/agent-oppsett`: `git switch -c claude/dodskamera`.

### A. Treffmarkører (kropp / rustning / hode / drap)
2. `GSCharacter.h`: endre `UFUNCTION(Client, Unreliable) void ClientHitMarker(bool bKill);` til
   `UFUNCTION(Client, Unreliable) void ClientHitMarker(bool bKill, bool bHeadshot, bool bArmor);`.
   Legg til (privat/protected, ved `LastHitMarkerTime`): `uint8 LastHitKind = 0;` og offentlig getter
   `uint8 GetLastHitKind() const { return LastHitKind; }` ved siden av `GetLastHitMarkerTime()`.
   Betydning: 0 = kropp, 1 = rustning (vest/hjelm tok imot), 2 = hodeskudd, 3 = drap. Lag gjerne en liten
   `namespace GSHitKind { constexpr uint8 Body = 0, Armor = 1, Head = 2, Kill = 3; }` i `GSCharacter.h`.
3. `GSCharacter.cpp` `TakeDamage`: lag `bool bArmorHit = false;` og sett den til `true` når `Soak > 0.f` i begge
   rustningsgrenene (hjelm og vest). Endre kallet til `Shooter->ClientHitMarker(bKilled, bHeadshot, bArmorHit);`.
4. `ClientHitMarker_Implementation(bool bKill, bool bHeadshot, bool bArmor)`:
   - `LastHitKind = bKill ? 3 : bHeadshot ? 2 : bArmor ? 1 : 0;` og `LastHitMarkerTime = now` som før.
   - Lyd med den eksisterende lyden `Hit_Marker` (ingen nye lydfiler): drap volum 0.9 tonehøyde 0.8 (som før);
     hodeskudd 0.85 / 1.45; rustning 0.55 / 1.7; kropp 0.6 / 1.0. Hodeskudd som også er drap: spill drapslyden
     og i tillegg hodeskuddlyden (to kall) så det kjennes ekstra godt.
5. `GSHUD.h/.cpp`: ny hjelpefunksjon `void DrawHitMarker(AGSCharacter* C, float CX, float CY);` som tegner markøren
   som fire diagonale streker (en X med hull i midten) med `Canvas->K2_DrawLine` (brukes allerede i fila):
   - Kropp: hvit, strek fra 7 til 14 px fra sentrum, tykkelse 2, varer 0,20 s, fader ut.
   - Rustning: lys blågrå `(0.6, 0.8, 1.0)`, samme størrelse, 0,20 s.
   - Hodeskudd: gul/gull `(1.0, 0.82, 0.2)`, 8–17 px, tykkelse 2.5, 0,30 s.
   - Drap: rød `(1.0, 0.15, 0.1)`, 9–20 px, tykkelse 3, 0,50 s; skaleres fra 1,35× ned til 1,0× de første 0,12 s
     (“smell”), og fader ut de siste 0,2 s. Hvis drapet var hodeskudd (lagre det i tillegg, f.eks.
     `bLastKillHeadshot` i karakteren), tegn liten gul tekst `HODESKUDD` 34 px under sentrum.
   - Tegn en 1 px mørk skygge (svart, alfa 0.5) under hver strek, forskjøvet (1,1), så markøren synes på snø og himmel.
6. Bytt ut den gamle prikk-koden i `DrawCrosshair` (linje ~199–209) med `DrawHitMarker(C, CX, CY)`, og den gamle
   røde firkanten i `DrawScope` (linje ~608–613) med `DrawHitMarker(C, CX, CY)` (samme CX/CY som siktet bruker).
   Merk: `DrawCrosshair` returnerer tidlig ved kikkert – flytt ikke på det.

### B. Dødsinfo fra serveren
7. `GSTypes.h`: legg til
   ```cpp
   USTRUCT()
   struct FGSDeathInfo
   {
       GENERATED_BODY()
       UPROPERTY() FString Killer;        // tom = ingen spiller (sult, fall, egen granat ...)
       UPROPERTY() FString Weapon;        // våpennavn eller årsak ("Sult", "Tørst", "Overdose", "Verden")
       UPROPERTY() float DistanceM = 0.f;
       UPROPERTY() float KillerHealth = 0.f;
       UPROPERTY() float KillerMaxHealth = 100.f;
       UPROPERTY() float KillerArmor = 0.f;   // vest + hjelm, poeng igjen
       UPROPERTY() bool bHeadshot = false;
   };
   ```
8. `GSPlayerController.h`: legg til
   ```cpp
   UFUNCTION(Client, Reliable) void ClientDeathCam(class AGSCharacter* Body, APawn* KillerPawn, FGSDeathInfo Info);
   FGSDeathInfo DeathInfo;
   float LocalDeathTime = -100.f;   // lokal GetTimeSeconds() da du døde
   bool bHasDeathInfo = false;
   ```
   `GSPlayerController.cpp` `ClientDeathCam_Implementation`: lagre `DeathInfo = Info; bHasDeathInfo = true;
   LocalDeathTime = GetWorld()->GetTimeSeconds();`, sett `Body->DeathCamKiller = KillerPawn` (se steg 10) hvis
   `Body` er gyldig, og kall `SetViewTarget(Body)` hvis `Body` er gyldig (lokalt, idempotent).
   Nullstill `bHasDeathInfo = false` i `AGSPlayerController::OnPossess`/`AcknowledgePossession` når en ny pawn tas
   over (overstyr `virtual void AcknowledgePossession(APawn* P) override;` – den kjøres på eierklienten og på
   listen-server-verten; kall `Super` først).
9. `GSCharacter.cpp` `Die(...)`, **før** `DetachFromControllerPendingDestroy()`:
   - `AGSPlayerController* VictimPC = Cast<AGSPlayerController>(GetController());`
   - Finn drapsmannens pawn: `APawn* KillerPawn = Killer ? Killer->GetPawn() : nullptr; if (KillerPawn == this) KillerPawn = nullptr;`
   - Fyll `FGSDeathInfo`: `Killer` = `Killer->PlayerState->GetPlayerName()` når `KillerPawn` finnes (ellers tom),
     `Weapon = WeaponName`, `bHeadshot`, `DistanceM = FVector::Dist(KillerPawn->GetActorLocation(), GetActorLocation()) / 100`,
     og fra `AGSCharacter* K = Cast<AGSCharacter>(KillerPawn)`: `KillerHealth = K->Health`, `KillerMaxHealth = K->MaxHealth`,
     `KillerArmor = K->ArmorPoints + K->HelmetPoints` (serveren har de riktige verdiene selv om de er OwnerOnly).
   - Etter `DetachFromControllerPendingDestroy()`: `if (VictimPC) { VictimPC->SetViewTarget(this); VictimPC->ClientDeathCam(this, KillerPawn, Info); }`.
     `SetViewTarget` på serveren sender selv `ClientSetViewTarget` til klienten, og RPC-ene kommer i rekkefølge, slik
     at standardoppførselen (bildet låses på kontrolleren) blir overstyrt. Ved ny spawn setter `Possess` view target
     tilbake til den nye karakteren automatisk – ikke gjør noe ekstra der.
   - `Die(nullptr, ...)` fra sult (linje ~2601) skal også gi dødskamera (uten drapsmann) – det følger av koden over.

### C. Dødskameraet
10. `GSCharacter.h`: `virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;`, offentlig
    `TWeakObjectPtr<APawn> DeathCamKiller;`, og private `float DeathCamStart = -1.f; FVector DeathEyeLoc; FRotator DeathEyeRot; FVector DeathCamPos; FRotator DeathCamRot; bool bDeathCamInit = false;`.
11. `OnRep_Dead()` (kjøres både på klienter og – via `Die()` – på serveren): helt øverst etter `if (!bDead) return;`,
    lagre `DeathCamStart = GetWorld()->GetTimeSeconds(); DeathEyeLoc = Camera->GetComponentLocation(); DeathEyeRot = Camera->GetComponentRotation();`.
    Gjør også egen kropp synlig for eieren (ellers ser du ingen kropp i dødskameraet):
    `if (bHumanBody) GetMesh()->SetOwnerNoSee(false); else { BodyMesh->SetOwnerNoSee(false); HeadMesh->SetOwnerNoSee(false); }`
    (sjekk hvilke av dem som faktisk er satt `OwnerNoSee` i konstruktøren og speil det), og skjul alle førsteperson-delene:
    `FPWeaponMesh, FPArmR, FPArmL, FPMag, FPAttSight, FPAttMuzzle, FPAttLaser, FPAttGrip, FPItemMesh, FPItemHand` → `SetVisibility(false)`
    (ev. en løkke over alle `UPrimitiveComponent` med `bOnlyOwnerSee`). Kontroller at `ApplyWeaponVisuals()` som kalles
    lenger ned i `OnRep_Dead` ikke gjør dem synlige igjen – hvis den gjør det, legg `if (bDead)`-vakt der.
12. `CalcCamera`: kall `Super::CalcCamera(DeltaTime, OutResult)` først (beholder FOV og etterbehandling fra kamerakomponenten).
    Hvis `!bDead || DeathCamStart < 0` → return. Ellers, med `T = now - DeathCamStart`:
    - Fokuspunkt: `GetMesh()->GetSocketLocation("pelvis")` når `bHumanBody` (ragdollen), ellers `GetActorLocation() - (0,0,60)`.
    - Fase 1 (0–0,35 s, “du faller”): posisjon = `DeathEyeLoc` senket opp til 45 cm (ease-in), rotasjon = `DeathEyeRot`
      med pitch dratt 20° ned og roll 0→18°.
    - Fase 2 (0,35–1,6 s): mål-posisjon = fokus − (horisontal retning av `DeathEyeRot`) × 280 + (0,0,180). Interpoler fra
      sluttpunktet i fase 1 med `FMath::SmoothStep`. Rotasjon: `(Fokus − Pos).Rotation()`, roll tilbake til 0.
    - Fase 3 (> 1,6 s): hvis `DeathCamKiller` er gyldig og ikke død: løft kameraet ytterligere til +230 cm og snu rotasjonen
      jevnt (`FMath::RInterpTo`, hastighet 2,5) mot `DeathCamKiller->GetActorLocation() + (0,0,60)`. Ellers: sakte bane rundt kroppen
      (yaw +10°/s) og fortsett å se på fokus.
    - Kollisjon: gjør `SweepSingleByChannel` med kule radius 12 fra fokus (+30 cm opp) til ønsket posisjon på `ECC_Camera`,
      ignorer `this` og `DeathCamKiller`; ved treff, bruk `Hit.Location + Hit.ImpactNormal * 8`. Gjør dette i alle faser etter fase 1,
      så kameraet aldri går gjennom vegger eller inn i bakken (viktig inne i byggene).
    - Glatt ut posisjonen med `FMath::VInterpTo(DeathCamPos, Ønsket, DeltaTime, 10)` (init første gang), så sveipet ikke hakker når
      kollisjon slår inn/ut. Skriv `OutResult.Location/Rotation`.

### D. Dødskortet i HUD-en
13. Skriv om `AGSHUD::DrawDeathScreen()`:
    - Hent `AGSPlayerController* PC`. Hvis `!PC || !PC->bHasDeathInfo` → tegn ingenting (fjerner «DU ER DØD» før første spawn).
    - `Since = now - PC->LocalDeathTime`. Rød hinne som toner inn: alfa `0.25 * clamp(Since/0.6)`, og en mørk vignett: fire
      kantbokser 120 px med alfa 0.35.
    - Midt øverst (Y ≈ 0.18 × høyde): `DU ER DØD`, skala 2, rød, toner inn over 0,3 s.
    - Kort nederst på midten (bredde 460, Y ≈ 0.68 × høyde), mørk bakgrunn `(0.05,0.05,0.06,0.78)` med 2 px ramme:
      - Med drapsmann: linje 1 `DREPT AV <navn>` (skala 1.4, hvit); linje 2 `<våpen> · <N> m` og til høyre gul `HODESKUDD` hvis
        bHeadshot; linje 3: to små stolper «Liv» (rød→grønn etter andel) og «Rustning» (blå, maks 200) med tall, så du vet hvor skadet han var.
      - Uten drapsmann: `DU DØDE` og `Årsak: <Weapon>`.
    - Under kortet: `Tilbake om <ceil(RespawnDelay − Since)> s` med en tynn fremdriftsstolpe (bruk `AGSGameMode::RespawnDelay`,
      den er `static constexpr` i headeren). Når tiden er ute: `Kommer tilbake ...`.
    - Behold teksten «Tingene dine ligger der du døde.» som liten grå linje nederst.
14. Sjekk at kallet i `DrawHUD` fortsatt går til `DrawDeathScreen()` når pawnen er null eller død (det gjør det i `else`-grenen nå),
    og at kill feed / topp-høyre / scoreboard fortsatt tegnes.

### E. Bygg og logg
15. Kompiler (editoren er lukket):
    `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" GloshaugenEditor Win64 Development -Project="C:\Users\afno0\Documents\GitHub\Calude-NTNU-Game\Gloshaugen.uproject" -WaitMutex`
    Sjekk først `Get-Process UnrealEditor` – kjører den, ikke bygg. Fiks alle feil og advarsler som kommer fra våre filer.
16. Legg linjene under inn i `IMPROVEMENTS.md` som en ny seksjon **øverst** (rett under innledningen, over `## Runde 4`),
    siden CLAUDE.md sier nyeste runde øverst.
17. Commit kun de endrede filene: `Combat feedback: death cam, killer card and hit marker types`.

## Ferdig når
- [ ] `Build.bat` fullfører uten feil, og uten nye advarsler fra `Source/Gloshaugen`.
- [ ] `ClientHitMarker` har tre parametre og alle kall er oppdatert (grep viser ingen gamle kall).
- [ ] Treffmarkøren har fire tydelige typer (kropp hvit, rustning blå, hode gull, drap rød X med «smell») i både vanlig sikte og kikkertsikte.
- [ ] Ved død: kameraet faller, trekker seg bakover til ~3 m bak/over kroppen uten å gå gjennom vegger, og snur seg mot drapsmannen etter ~1,6 s.
- [ ] Dødskortet viser navn, våpen, avstand, hodeskudd og drapsmannens liv/rustning; nedtelling til respawn stemmer med 4 s.
- [ ] Ved sultedød vises «DU DØDE / Årsak: Sult», og kameraet går i bane rundt kroppen.
- [ ] Etter respawn er kameraet tilbake i førsteperson med armer og våpen synlige (FP-deler på den *nye* karakteren er ikke skjult).
- [ ] Ingen endring i `Config/DefaultEngine.ini` eller usporede filer er staget/committet.
- [ ] IMPROVEMENTS.md har linje 289–300 øverst.

## Risiko
- **Replikering/rekkefølge:** Standard `UnPossess` setter view target til kontrolleren og kan sende `ClientSetViewTarget(PC)` til klienten.
  Vi kaller `SetViewTarget(this)` på serveren etter `DetachFromControllerPendingDestroy`, så den siste reliable RPC-en vinner, og
  `ClientDeathCam` gjør det samme lokalt. Hvis kameraet likevel låser seg på klienten, er det her feilen ligger.
- **`OnRep_Dead` kan komme før eller etter at kontrolleren har sluppet pawnen** på klienten – derfor bruker dødskameraet kun data på selve
  karakteren (`DeathCamStart`, øyeposisjon) og ikke `IsLocallyControlled()`.
- **Egen kropp synlig:** å slå av `OwnerNoSee` på den døde kroppen påvirker bare den døde pawnen (den nye spawnes med normale verdier).
  Pass på at FP-deler skjules *bare* på den døde.
- **Listen-server-verten:** `OnRep_Dead` kalles manuelt i `Die()`, og `ClientDeathCam` kjøres lokalt der – test-scenarioet må dekke begge.
- **Ragdoll uten Manny:** uten `bHumanBody` finnes ingen `pelvis`-socket; bruk fallback-fokus.
- **Ytelse:** én sveip per bilde bare mens du er død – ubetydelig.
- **Haglskudd:** mange hagl gir mange `ClientHitMarker`-kall per skudd (som før). Markøren startes på nytt hver gang; det er greit.
  Ikke la en senere kropps-markør overskrive en drapsmarkør innen samme 0,5 s: hvis `LastHitKind == 3` og det er < 0,5 s siden, behold drap.

## IMPROVEMENTS-linjer
```
## Runde 6: kamp-tilbakemelding (26. nov.)

289. Dødskamera: når du dør faller bildet, trekker seg bakover og viser kroppen din i stedet for å fryse i øyehøyde
290. Dødskameraet snur seg mot den som drepte deg, så du ser hvor skuddet kom fra
291. Dødskameraet går aldri gjennom vegger, tak eller bakken, heller ikke inne i byggene
292. Uten drapsmann (sult, tørst, egen granat) går kameraet sakte i bane rundt kroppen
293. Dødskort: hvem som drepte deg, med hvilket våpen og fra hvor mange meter
294. Dødskortet viser hvor mye liv og rustning drapsmannen hadde igjen
295. Dødskortet merker hodeskudd, og viser årsaken når du døde av noe annet enn en spiller
296. Nedtelling med stolpe til du kommer tilbake
297. Treffmarkøren er nå en tydelig X med skygge som synes på snø og himmel
298. Egen gullfarget markør og lysere lyd for hodeskudd
299. Blå markør og «klink»-lyd når kula tas av vest eller hjelm
300. Rød, større drapsmarkør med et lite «smell» og «HODESKUDD» når drapet var et hodeskudd, også i kikkertsikte
```
