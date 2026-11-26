# Plan 5b – runde 6 (lag Kamp): ballistikk del 1b. Lysspor som flyr, smell når kula passerer, holdemerker

Andre halvdel av den delte ballistikkplanen (se `kontroll/forhand-plan-05.md`, punkt 2, 4, 7 og D). **Bygges
etter at 5a (`ko/plan-05.md`) er bygget, kontrollert og committet.** Alt under bygger på 5a: `UGSBallistics`,
`FGSBullet`, `ServerFire(Start, AimDir, ShotIndex, SpreadQ)`, `MulticastShotFX(Start, Dir, Flags)` og
`GSDrawShotStreak`. Les de funksjonene slik de faktisk ble; avviker de fra 5a-planen, følg koden.
Lagfarger/CTF-HUD skal være committet før `GSHUD.cpp` røres. Bruk funksjonsnavn, ikke linjenumre.

## Mål
Vanlige kuler er usynlige i lufta, lysspor flyr med kula i en bue, kuler som går tett forbi hodet ditt smeller
eller suser, og kikkerten har holdemerker som stemmer med kulefallet.

## Forbedrer (gammelt)
- **Lysspor** (funn R12): streken som er der med en gang fjernes. Bare lysspor-ammo og hver femte kule fra
  maskingevær lyser, og sporet følger kula. Lysspor-ammo gir ikke lenger bedre treff.
- **`ServerFire`** (funn V7): automatvåpen sender skudd som raske meldinger som kan gå tapt, så MG3 med pakketap
  ikke kan koble deg fra. Alt annet (enkeltskudd, boltrifle, hagle, rakettkaster) er fortsatt pålitelig.
- **Kikkertsiktet**: prikkene under midten, som ikke betydde noe, byttes med holdemerker regnet fra patronen.

## Nytt
- **Overlydssmell og sus** når en annen spillers kule passerer nær hodet ditt (to nye lyder).
- **Holdemerker** for 150–300 m i sterke kikkerter.
- **Flygende lysspor** som én strek per kule, med felles tak per maskin.

## Hvorfor nå
Fullfører ballistikk del 1 (prioritering 3). 5a ga kulene fart og fall, men spilleren ser og hører dem ikke ennå.

## Avgrensning
- Ingen gjennomslag, ingen V8 (tilbakespoling – neste Kamp-runde), ingen forsinket skuddlyd (backlog).
- Hagl får ikke lysspor.

## Filer
- `Source/Gloshaugen/Public/GSBallistics.h`, `Private/GSBallistics.cpp`
- `Source/Gloshaugen/Public/GSCharacter.h`, `Private/GSCharacter.cpp` (bare skytefunksjonene og én linje i `GetStats`)
- `Source/Gloshaugen/Public/GSEffects.h`, `Private/GSEffects.cpp` (to små nye funksjoner i `AGSShortFX`; ingenting annet)
- `Source/Gloshaugen/Private/GSHUD.cpp` (`DrawScope`)
- `Tools/Audio/gen_sounds.py` + nye `SourceArt/Sounds/S_GS_Bullet_Crack.wav`, `S_GS_Bullet_Whiz.wav`
- `IMPROVEMENTS.md` (linje 446–459), `.claude/lag/bestillinger.md` (én linje)

Ikke rør `TakeDamage`, `GSAudio.*`, `GSTypes.h`, `GSItems.*`, `catalog.py`, `GSPlayerController.*`. Stage filene med navn.

## Steg

### 0. Forberedelse
1. `git log --oneline -5`: 5a skal være committet. `Get-Process UnrealEditor` – kjører editoren: ikke bygg.
   Gjør headerne (`GSBallistics.h`, `GSEffects.h`, `GSCharacter.h`) ferdige i ett.

### A. Upålitelig ild bare for automatvåpen (`GSCharacter.*`)
2. `GSCharacter.h`: ved siden av `ServerFire` (som forblir `Reliable`):
   ```cpp
   /** Fast automatic weapons only: a lost shot is not missed, and a burst can never overflow the reliable buffer. */
   UFUNCTION(Server, Unreliable) void ServerFireFast(FVector_NetQuantize Start, FVector_NetQuantizeNormal AimDir, int32 ShotIndex, uint8 SpreadQ);
   void HandleFire(const FVector& Start, const FVector& AimDir, int32 ShotIndex, uint8 SpreadQ);
   ```
3. `GSCharacter.cpp`: gi kroppen i `ServerFire_Implementation` navnet `HandleFire`. Begge `_Implementation` kaller bare
   `HandleFire(Start, AimDir, ShotIndex, SpreadQ);`. Ingen annen endring i logikken: `Jump <= 0` forkaster skudd som
   kommer for sent eller dobbelt, og hopp over 3 (flere tapte pakker på rad) gir serverens eget mønster for det skuddet.
4. `FireOnce`, nederst:
   ```cpp
   // One costly shot must arrive (bolt rifle, shotgun, single-use launcher). A stream of them may lose one.
   if (S.Mode == EGSFireMode::Hitscan && S.bAutomatic && S.FireInterval < 0.15f) ServerFireFast(Start, Aim, ShotIndex, SpreadQ);
   else ServerFire(Start, Aim, ShotIndex, SpreadQ);
   ```
   I rapporten: list våpnene som havner på den raske veien (fra `catalog.py`).

### B. Hvilke kuler lyser (`GSCharacter.*`)
5. Filstatisk hjelper over `FireOnce`:
   `static bool GSIsTracer(const FGSWeaponStats& S, uint8 AmmoType, int32 ShotIndex) { return AmmoType == GSAmmoType::Tracer || (S.Class == EGSWeaponClass::LMG && ShotIndex % 5 == 0); }`
6. `MulticastShotFX`-kommentaren i headeren: legg til `8 = tracer round`. I `HandleFire`, vanlig skudd:
   flagget blir `(S.bSilent || Mods.bSilent ? 1 : 0) | (GSIsTracer(S, AmmoType, ShotIndex) ? 8 : 0)` (bruk kopien av ammotypen fra 5a).
7. `FireOnce`: `Ballistics->FireShown(this, Start, D, Ball, GSIsTracer(S, W->AmmoType, ShotIndex));` og **fjern** kallet til
   `GSDrawShotStreak`. `MulticastShotFX_Implementation`: `FireShown(…, (Flags & 8) != 0)` og fjern `GSDrawShotStreak`-linja.
   `DrawPellets`: fjern streken og parameteren `bFromMuzzle` (header, begge kall); `FireShown(…, false)`.
   Slett `GSDrawShotStreak`. `grep -n "SpawnTracer"` i `GSCharacter.cpp` skal være tom.
8. `GetStats`: fjern linja `if (Slot.AmmoType == GSAmmoType::Tracer) { S.HipSpread *= 0.85f; S.AimSpread *= 0.85f; }`.

### C. Streken (`GSEffects.*`)
9. `GSEffects.h`, i `AGSShortFX` (public):
   ```cpp
   /** A glowing streak that lives until it is destroyed. GSBallistics owns one per tracer bullet and moves it every frame. */
   static AGSShortFX* SpawnBulletStreak(UWorld* World);
   void SetStreak(const FVector& Tail, const FVector& Head);
   ```
10. `GSEffects.cpp`: `SpawnBulletStreak` som `SpawnTracer` (kube, additivt materiale med samme farge/`Glow`/`Soft`,
    reservefarge uten materialet), men **uten** `SetLifeSpan`, laget skjult i origo, returnerer aktøren; `nullptr` på
    `NM_DedicatedServer`. `SetStreak`: `Delta = Head - Tail`; `SetActorLocationAndRotation((Tail + Head) * 0.5f, Delta.Rotation())`,
    `Mesh->SetWorldScale3D(FVector(Delta.Size() / 100.f, 0.012f, 0.012f))`, `SetActorHiddenInGame(false)`.
    Les `AGSShortFX::Tick`: den skal ikke flytte, skalere eller fjerne en slik aktør (gjør den det, gi aktøren et flagg
    `bStreak` som hopper over det). `SpawnTracer` kan stå (ingen bruker den etter dette; fjern den hvis `grep` er tom).

### D. Lysspor og smell i `UGSBallistics`
11. `GSBallistics.h`: `FireShown(…, const FGSBallistic& B, bool bTracer);`. `FGSBullet` får `bool bTracer = false;`,
    `bool bPassedEar = false;`, `TWeakObjectPtr<class AGSShortFX> Streak;`. Klassen får `int32 LiveStreaks = 0;`,
    `float LastCrackTime = -100.f;` og `void EndBullet(FGSBullet& B);`.
12. `GSBallistics.cpp`: `#include "GSAudio.h"`, `GameFramework/PlayerController.h`, `Camera/PlayerCameraManager.h`.
    `constexpr int32 MaxStreaks = 40;` (felles tak per maskin). `EndBullet`: `if (B.Streak.IsValid()) { B.Streak->Destroy(); --LiveStreaks; }`.
    Kall `EndBullet` **alle** steder en kule fjernes (i `Tick`, og når taket på 600 kaster den eldste viste kula).
    I `Step`, ved treff: sett `B.Pos = End;` før `return false;` (så siste strekk er kjent i `Tick`).
13. `Tick`, for **viste** kuler (`!bReal` – verten har både ekte og vist kule for samme skudd; ellers blir det dobbelt):
    ta `const FVector Prev = B.Pos;` og `const float Speed = B.Vel.Size();` før `Step`, og etter `Step` (også når den ga `false`):
    - **Smell/sus** (én gang per kule): finn øret én gang per `Tick` (`GetWorld()->GetFirstPlayerController()`,
      `PC->PlayerCameraManager->GetCameraLocation()`; ingen lokal spiller: hopp over). Hvis `!B.bPassedEar`,
      `PC->GetPawn() != B.Shooter.Get()` og `FMath::PointDistToSegment(Ear, Prev, B.Pos)` er under 300 cm (kule over
      lydfarten, `Speed > 34300.f`) eller under 150 cm (langsommere): `B.bPassedEar = true;`, og hvis
      `Now - LastCrackTime > 0.05f` (en haglskur er ett smell, og høyst 20 i sekundet):
      `LastCrackTime = Now; GSAudio::PlayAt(this, Speed > 34300.f ? TEXT("Bullet_Crack") : TEXT("Bullet_Whiz"), FMath::ClosestPointOnSegment(Ear, Prev, B.Pos), Speed > 34300.f ? 0.9f : 0.6f, 30.f, FMath::FRandRange(0.92f, 1.08f));`
    - **Lysspor** (bare `B.bTracer`, bare hvis kula lever): tennes 10 m ute (`B.Travelled > 1000.f`). Har den ingen strek
      og `LiveStreaks < MaxStreaks`: `B.Streak = AGSShortFX::SpawnBulletStreak(World); if (B.Streak.IsValid()) ++LiveStreaks;`.
      Har den strek: `Len = Clamp(Speed * 0.02f, 200.f, 1500.f)` (så langt kula går på 1/50 s – slik øyet ser et lysspor),
      aldri lengre enn `B.Travelled - 1000.f`; `B.Streak->SetStreak(B.Pos - B.Vel.GetSafeNormal() * Len, B.Pos);`.
    Streken flyttes i `Tick`, aldri i `Step`, og det lages aldri en ny aktør eller et nytt materiale per bilde.

### E. Kikkerten (`GSHUD.cpp`, `DrawScope`)
14. `#include "GSBallistics.h"`. Les FOV først: `const float FOV = PlayerOwner && PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetFOVAngle() : 25.f; const bool bStadia = FOV <= 16.f;`
    (8x-kikkert og skarpskyttergeværenes egne sikter; i 4x ligger merkene for tett til å brukes på dette kartet.)
    I prikk-løkka: når `bStadia`, ta ikke med prikkene under midten (`FVector2D(CX, CY + D)`). Rett etter løkka:
    ```cpp
    // Hold marks below the centre, like the drop stadia on a real scope: where to hold at 150, 200, 250, 300 m with
    // the cartridge in this gun (the same numbers the bullets fly by). Whole hundreds are wide and numbered.
    const FGSWeaponSlot* W = C->GetCurrentWeapon();
    if (bStadia && W)
    {
    	const FGSBallistic B = GSBallistics::ForWeapon(C->GetStats(*W));
    	const float PxPerRad = (Canvas->SizeX * 0.5f) / FMath::Tan(FMath::DegreesToRadians(FOV * 0.5f));
    	float LastDy = 0.f;
    	for (int32 M = 50; M <= 300; M += 50)
    	{
    		if (M <= B.ZeroM) continue;
    		const float Dy = FMath::Tan(GSBallistics::HoldUnder(B, float(M))) * PxPerRad;
    		if (Dy > R * 0.85f) break;
    		if (Dy - LastDy < 4.f) continue;
    		LastDy = Dy;
    		const bool bMajor = M % 100 == 0;
    		const float Half = bMajor ? 14.f : 7.f;
    		Box(CX - Half, CY + Dy - 0.5f, Half * 2.f, 1.f, Ink);
    		if (bMajor) Text(FString::FromInt(M / 100), CX + Half + 5.f, CY + Dy - 9.f, Ink, 0.7f);
    	}
    }
    ```
    Kontrolltall (8x, 1920 px bredt, 7,62 NATO): 150 m ≈ 4 px, 200 m ≈ 9 px, 250 m ≈ 13 px, 300 m ≈ 19 px under midten.

### F. Lydene (`Tools/Audio/gen_sounds.py`)
15. Ny funksjon `bullet_sounds()` (før `loop_save`):
    - `Bullet_Crack` (overlydssmellet, en «N-bølge»): 0,35 s; et skarpt trykksprang på ca. 1 ms (sagtann over ~45
      samples) fulgt av høypassfiltrert støy med 4 ms forfall, så `reverb(x, 0.3, 0.25)`. `save("Bullet_Crack", x, 0.9)`.
    - `Bullet_Whiz`: 0,3 s båndpass-støy (`highpass(lowpass(noise, 3000), 600)`) med `env(n, 0.08, 0.1)` og en tone
      som synker fra 1400 til 700 Hz (doppler), blandet inn 30 %. `save("Bullet_Whiz", x, 0.6)`.
    I `__main__`: `if len(sys.argv) > 1 and sys.argv[1] == "bullet": bullet_sounds(); raise SystemExit` (ved `hurt`),
    og `bullet_sounds()` i den fulle kjøringen. Kjør `python Tools/Audio/gen_sounds.py bullet` og sjekk at de to
    `.wav`-filene finnes (editoren importerer dem neste gang den åpnes; til da er lydene stille uten feil).

### G. Bygg, logg, bestilling
16. Bygg (`-WaitMutex`, editoren lukket). `IMPROVEMENTS.md`: ny overskrift øverst med linjene under.
    Commit: `Combat: ballistics part 1b - flying tracers, bullet crack and whiz, unreliable fire for automatics, scope hold marks`.
17. Én linje øverst i `.claude/lag/bestillinger.md`:
    `- <dato> · Kamp (bygger) → Spillbarhet · catalog.py · Teksten til «Lysspor» sier «Litt bedre treff»; fra Kamps runde 6 gir lysspor ikke bedre treff, bare synlige kuler (og alle ser deg) – endre teksten. · ballistikk 5b · ÅPEN`

## Ferdig når
- `Build.bat` går gjennom uten feil.
- `ServerFireFast` (Unreliable) kalles bare for `Hitscan` + `bAutomatic` + `FireInterval < 0.15`; alt annet går via
  `ServerFire` (Reliable). Begge ender i `HandleFire`. Hagle, boltrifle, muskett og rakettkastere går aldri den raske veien.
- Lysspor: ingen `SpawnActor` eller `UMaterialInstanceDynamic::Create` per bilde; én strek per kule, flyttet i `Tick`,
  høyst 40 samtidig, fjernet når kula er ferdig (alle veier ut går gjennom `EndBullet`). Bare for `!bReal`-kuler.
- Smell/sus bare for `!bReal`-kuler, én gang per kule, aldri for egne kuler, høyst ett per 0,05 s.
- `GetStats` gir ikke lysspor-ammo bedre spredning. `TakeDamage` er urørt.
- I spillet (Andreas, to spillere, «Play As Listen Server», etter kampstart):
  - Vanlige kuler synes ikke i lufta. Bytt til lysspor-ammo, eller skyt MG3: lysende streker flyr bortover i en bue
    (med MG3 omtrent hver femte kule). Streken starter et stykke foran løpet.
  - La den andre skyte forbi hodet ditt (litt til siden): du hører et skarpt smell rett ved deg, i tillegg til
    skuddet. Med Thompson/UMP (.45) hører du et sus i stedet. Du hører ikke smell fra dine egne kuler.
  - Sett 8x-kikkert på et gevær: små streker under midten, de brede merket 2 og 3. Skriv `gs.ShowBullets 1`, sikt
    med 2-streken på en vegg ca. 200 m unna (langs nordsiden av plassen) og se at buen ender der streken peker.
  - Hold inne avtrekkeren på MG3 i klientvinduet i 10 sekunder: du blir ikke koblet fra, og ammoen går ned som den skal.
  - Skyt L96, hagle og M72 i klientvinduet: hvert skudd går (aldri smell uten kule).

## Risiko
- **Tapt automatskudd:** klienten har trukket fra en patron serveren ikke har brukt; telleren kan vise ett for lite
  til neste lading. Godtatt – bare for våpen der ett skudd ikke merkes.
- **Blandet rekkefølge:** et pålitelig og et upålitelig skudd kan bytte plass rett etter våpenbytte; det eldste forkastes
  (`Jump <= 0`). Sjelden, og bare ett skudd.
- **Taket på 40 streker:** med flere MG-er samtidig vises ikke alle lysspor. Kulene flyr og treffer som før.
- **`AGSShortFX::Tick`** kan ha logikk som fjerner eller skalerer aktører; sjekk (steg 10).
- **Smell i dødskamera/tilskuer:** øret er kameraet, så du hører kuler der kameraet er. Riktig nok.
- **Holdemerkene** bruker kaliberet i reservetabellen; når Spillbarhet leverer ekte munningsfart flytter de seg av seg selv.
- **Lydene finnes ikke før editoren har importert dem** (første åpning etter bygget).

## IMPROVEMENTS-linjer
```
## Runde 11 (Kamp): ballistikk 1b – lysspor, smell og holdemerker (26. nov.)

446. Vanlige kuler er usynlige i lufta, som i virkeligheten; bare lysspor-ammo og hver femte kule fra maskingevær lyser
447. Lysspor flyr med kula i dens egen fart og bue, i stedet for en strek som er der i samme øyeblikk
448. Lyssporet er så langt som kula rekker på et øyeblikk: en rask geværkule gir en lang strek, en pistolkule en kort
449. Lyssporet tennes ti meter foran løpet, så det ikke dekker skjermen til den som skyter
450. Lysspor-ammo gir ikke lenger bedre treff, bare synlige kuler
451. Kuler som passerer nær hodet ditt gir et skarpt overlydssmell rett ved deg
452. Langsomme kuler (.45) suser forbi i stedet for å smelle
453. En haglskur som går forbi deg er ett smell, ikke ni
454. Automatvåpen sender skuddene som raske meldinger, så maskingevær med dårlig nett ikke lenger kan koble deg fra serveren
455. Enkeltskudd, boltrifler, hagler og rakettkastere sendes fortsatt sikkert: et skudd du har hørt smelle forsvinner aldri på veien
456. Skudd som kommer for sent eller to ganger til serveren forkastes
457. Sterke kikkerter har holdemerker for 150, 200, 250 og 300 m, regnet ut fra patronen i våpenet
458. Prikkene under midten i kikkerten, som ikke betydde noe, er borte
459. Hvert lysspor er én strek som flyttes, og høyst 40 tegnes samtidig, så det ikke hakker når flere skyter maskingevær
```
