# Plan – runde 2: å bli truffet (skade-tilbakemelding for den som tar skade)

## Mål
Når du blir skutt merker du straks hvor det kom fra, hvor hardt det var og om vesten eller hjelmen tok det,
og når du har lite liv igjen hører du hjertet slå og bildet blir gråere – så du skjønner at du må gjemme deg.

## Hvorfor nå
- Runde 1 ga god tilbakemelding til den som skyter og den som dør. Den tredje situasjonen – *du blir truffet
  og lever* – er nå den svakeste: hele skjermen blinker rødt i 0,5 s uansett skade, én rød pil (en ny
  treffer overskriver den forrige), samme `Hit_Body`-lyd for alt (også sult), og ingenting som sier
  «lavt liv» bortsett fra stolpen nede i hjørnet. Kontrolløren foreslo dette etter runde 1.
- Alt er C++ + to små skriptendringer (lyd). Ingen .uasset, editoren trenger ikke kjøre. Kan bygges og sjekkes i én runde.
- Tar samtidig de to små svakhetene kontrolløren fant i runde 1.

## Filer
- `Source/Gloshaugen/Public/GSCharacter.h`, `Private/GSCharacter.cpp` – ny RPC `ClientTookHit`, liste over
  nylige treff, lavt-liv-tilstand (hjerteslag, metning), helings-blink. Rettelse i `Die()`.
- `Source/Gloshaugen/Public/GSHUD.h`, `Private/GSHUD.cpp` – blodkant i stedet for helrød skjerm, flere
  retningspiler, rustnings-/hodeskudd-/helings-blink, lavt-liv-puls.
- `Source/Gloshaugen/Public/GSTypes.h` – `FGSDeathInfo` får `KillerLocation` + `bHasKillerLocation`.
- `Source/Gloshaugen/Private/GSPlayerController.cpp` – lagre reserveposisjonen i karakteren.
- `Tools/Audio/gen_sounds.py` – fem nye lyder; `SourceArt/Sounds/S_GS_{Hit_Armor,Hit_Head,Armor_Break,Heartbeat,Ear_Ring}.wav` (nye, sporede).
- `IMPROVEMENTS.md` – linje 301–312.

Ikke rør `Config/DefaultEngine.ini` eller usporede filer i `Content/`, `SourceArt/PolyHaven/` osv.
Stage kun filene over, med navn (aldri `git add -A`/`.`).

## Steg

### 0. Forberedelse
1. Vi er på `claude/dodskamera` (runde 1 er committet: 8fce646, c055645). Fortsett på den grenen.
2. `Get-Process UnrealEditor` – kjører den, ikke bygg.

### A. Nye lyder (generert, samme stil som de andre)
3. `Tools/Audio/gen_sounds.py`: legg til en funksjon `hurt_sounds()` (etter `pickup()`), som lager:
   - `Hit_Armor` (0,35 s): metallisk «plink/klonk» – `metal_click`-aktig: sinuser 1900, 3100, 4700 Hz med rask
     decay (0,05–0,12 s) + kort lavpasset støyslag (600 Hz, decay 0,02). Peak 0,7.
   - `Hit_Head` (0,4 s): hardt «krakk» – høypasset støy (3000 Hz) med decay 0,006 + dyp dump (55 Hz sinus, decay 0,06). Peak 0,85.
   - `Armor_Break` (0,6 s): to–tre metalliske klikk etter hverandre (0, 0,06, 0,13 s) + lavpasset knase-støy (1200 Hz, decay 0,15). Peak 0,8.
   - `Heartbeat` (0,55 s): «lub-dub» – to dype sinuspulser 48 Hz og 40 Hz (0,0 s og 0,17 s), hver med attack 0,005,
     decay 0,07, lavpasset 180 Hz. Peak 0,9. (Spilles én gang per slag fra koden, ikke loopet.)
   - `Ear_Ring` (1,6 s): øresus – sinus 3600 Hz + svak 3610 Hz (svevning), attack 0,02, decay 0,6. Peak 0,35.
   Bruk `save(name, x, peak)` som de andre.
4. Kall `hurt_sounds()` **sist** i `if __name__ == "__main__":`, og legg til helt først i blokka:
   `import sys` (øverst i fila) og
   ```python
   if len(sys.argv) > 1 and sys.argv[1] == "hurt":
       hurt_sounds(); raise SystemExit
   ```
   Kjør så **bare** `python Tools/Audio/gen_sounds.py hurt` (ikke hele skriptet – da skrives alle gamle wav-er på nytt).
   Sjekk med `git status --short SourceArt/Sounds` at det kun er fem nye `??`-filer og ingen `M`.
   Lydene importeres automatisk neste gang Andreas åpner editoren (`gs_sounds.py`); manglende lyder hoppes stille over i spillet.

### B. Server sender treffinfo til den som blir truffet
5. `GSCharacter.h`: erstatt
   `UFUNCTION(Client, Unreliable) void ClientDamageFrom(FVector_NetQuantize From);` med
   ```cpp
   /** You were hit: where from (if anyone shot), how much health you lost, and what kind of hit. */
   UFUNCTION(Client, Unreliable) void ClientTookHit(FVector_NetQuantize From, bool bHasFrom, float HealthLost, uint8 Kind);
   ```
   og legg til (offentlig, ved `LastDamageFrom`):
   ```cpp
   namespace GSHurtKind { constexpr uint8 Body = 0, Armor = 1, Head = 2, HeadArmor = 3, ArmorBroke = 4; }  // over klassen, ved GSHitKind
   struct FGSRecentHit { FVector From; float Time; float Strength; };   // også over klassen
   TArray<FGSRecentHit> RecentHits;          // up to 4 directions shown at once (local only)
   float LastHurtFlashTime = -100.f;         // edge flash
   float LastHurtFlashStrength = 0.f;        // 0..1
   uint8 LastHurtKind = 0;
   float LastHealTime = -100.f;
   float GetLowHealthAlpha() const;          // 0 above 35 % health, 1 at 0
   ```
   Behold `LastDamageFrom`/`LastDamageFromTime` (settes fortsatt for kompatibilitet; `GSPlayerController.cpp:434` bruker `GetLastDamageTakenTime`, som ikke endres).
   Grep etter `ClientDamageFrom` og bytt alle kall (bare ett, i `TakeDamage`).
6. `TakeDamage` (GSCharacter.cpp ~2378–2480): før rustningsgrenene, ta vare på `const float HealthBefore = Health;`.
   La rustningsgrenene sette `bool bArmorBroke = true` når hjelmen/vesten går i stykker (der `ClientToast(... i stykker/ødelagt)` kalles).
   Etter `Health = FMath::Max(...)`:
   ```cpp
   const uint8 Kind = bArmorBroke ? GSHurtKind::ArmorBroke
       : bHeadshot ? (bArmorHit ? GSHurtKind::HeadArmor : GSHurtKind::Head)
       : bArmorHit ? GSHurtKind::Armor : GSHurtKind::Body;
   const bool bHasFrom = Shooter && Shooter != this;
   ClientTookHit(bHasFrom ? Shooter->GetActorLocation() : GetActorLocation(), bHasFrom, HealthBefore - Health, Kind);
   ```
   (erstatter den gamle `ClientDamageFrom`-linja). Kall den *før* `Die()` slik at det skjer i samme rekkefølge som før.
   Merk: `ClientTookHit` på listen-server-verten kjøres direkte lokalt – det er riktig.
7. `ClientTookHit_Implementation`:
   - `const float Now = GetWorld()->GetTimeSeconds();`
   - Hvis `bHasFrom`: sett `LastDamageFrom/LastDamageFromTime` som før, og legg i `RecentHits`: finn en eksisterende innslag
     der retningen (yaw fra spilleren) er innen 25° av den nye → oppdater `From`, `Time = Now`, `Strength = Max(gammel*0.5, ny)`;
     ellers legg til nytt. `Strength = Clamp(HealthLost / 35, 0.35, 1)` (rustningstreff med 0 helsetap gir 0,35).
     Fjern innslag eldre enn 2 s, og behold maks 4 (fjern det eldste).
   - `LastHurtFlashTime = Now; LastHurtKind = Kind; LastHurtFlashStrength = Clamp(HealthLost / 40, 0.25, 1);`
   - Lyd (2D, eksisterende `GSAudio::Play2D`):
     - Body: `Hit_Body` 0.8, tonehøyde 0.9–1.1 (som før).
     - Armor: `Hit_Armor` 0.7, 0.95–1.05, og `Hit_Body` 0.3 hvis `HealthLost > 1`.
     - Head: `Hit_Head` 0.9 + `Ear_Ring` 0.45 + `Hit_Body` 0.6.
     - HeadArmor: `Hit_Armor` 0.8 tonehøyde 1.25 + `Ear_Ring` 0.3.
     - ArmorBroke: `Armor_Break` 0.9 + `Hit_Body` 0.5.
   - Hodeskudd (Head/HeadArmor): gi et ekstra kraftig rykk i kameraet: sett et nytt felt `FlinchScale = 2.2f` (ellers 1.0),
     som ganges inn i `Flinch` i kamera-animasjonen (GSCharacter.cpp ~2842–2873). Nullstill til 1 når neste vanlige treff kommer.
8. `OnRep_Health()` (~2483): **fjern** `Hit_Body`-lyden derfra når et `ClientTookHit` kom for under 0,3 s siden
   (`Now - LastHurtFlashTime < 0.3f`); behold den, men med volum 0,4, ellers (sult/tørst-skade og annet som ikke går via
   `TakeDamage`). Behold `LastDamageTakenTime`-linja. Legg til helings-registrering:
   `if (IsLocallyControlled() && Health > LastSeenHealth + 4.f) LastHealTime = Now;` (før `LastSeenHealth = Health;`).

### C. Lavt liv: hjerteslag og gråere bilde
9. `GetLowHealthAlpha()`: `if (bDead) return 0; const float F = Health / Max(MaxHealth,1); return Clamp((0.35f - F) / 0.35f, 0, 1);`
10. I `UpdateLocalVisuals` (kun lokal spiller, ~2795), etter de eksisterende PP-innstillingene:
    - `const float Low = GetLowHealthAlpha();`
    - Metning: hvis `Low > 0.01f` **og** `Visual` ikke er 2 eller 5 (de bruker selv `ColorSaturation`):
      `PP.bOverride_ColorSaturation = true; const float S = 1.f - 0.6f * Low; PP.ColorSaturation = FVector4(S, S, S, 1.f);`
      Ellers, hvis Visual ikke er 2/5: `PP.bOverride_ColorSaturation = false` (sett dette eksplisitt, så det nullstilles når du heles).
    - Vignett: hvis `Low > 0.01f` og `Visual == 0`: `PP.bOverride_VignetteIntensity = true; PP.VignetteIntensity = 0.4f + 0.6f * Low;`
    - Hjerteslag: nytt felt `float NextHeartbeat = 0.f;`. Hvis `Low > 0.f && !bDead && T >= NextHeartbeat`:
      `GSAudio::Play2D(this, TEXT("Heartbeat"), 0.35f + 0.5f * Low, 1.f);` og
      `NextHeartbeat = T + FMath::Lerp(1.05f, 0.55f, Low);` (raskere jo lavere liv). Lagre også `LastHeartbeatTime = T` (nytt felt,
      offentlig getter `GetLastHeartbeatTime()`) så HUD-pulsen følger slaget.
    - Når død: sett `PP.bOverride_ColorSaturation = false` og ikke spill hjerteslag (dødskameraet viser kroppen fra samme kamera
      – sjekk at `UpdateLocalVisuals` fortsatt kjøres mens død; uansett skal `Low` være 0 når `bDead`).

### D. HUD
11. `GSHUD.cpp` `DrawHUD` (~87–92): fjern den helrøde boksen (`Box(0,0,SizeX,SizeY, ... 0.35 ...)`) og kall i stedet
    `DrawHurtOverlay(C);` (ny, deklareres i `GSHUD.h`). Rekkefølge: før `DrawScope/DrawCrosshair` som i dag.
12. `DrawHurtOverlay(AGSCharacter* C)`:
    - Hjelpefunksjon `void EdgeGlow(const FLinearColor& Col, float Alpha, float Depth)` (deklareres i `GSHUD.h`): tegner en kant
      langs alle fire sider som toner fra `Alpha` ved kanten til 0 innover, som 8 bånd per side med `Box` (bånd i = Depth/8 tykt,
      alfa = Alpha * (1 - i/8)^2). Hjørner får dobbelt alfa naturlig fordi båndene overlapper – greit.
    - Treff-blink: `Age = Now - C->LastHurtFlashTime`, varer 0,6 s, `A = Strength * (1 - Age/0.6)^1.5`:
      - Body/Head/ArmorBroke: mørk blodrød `(0.55, 0.0, 0.0)`, alfa `0.55*A`, dybde `Canvas->SizeY * (0.12 + 0.10*Strength)`.
      - Armor/HeadArmor: blågrå `(0.55, 0.7, 0.9)`, alfa `0.35*A`, dybde `SizeY*0.08`.
      - ArmorBroke i tillegg: kort hvit/blå tekst midt under siktet `VESTEN RAKK` / `HJELMEN RAKK` er unødvendig –
        toast finnes allerede. Ikke legg til mer tekst.
    - Hodeskudd (Head/HeadArmor): i tillegg et hvitt blink over hele skjermen, alfa `0.35*(1-Age/0.25)` de første 0,25 s
      (litt «svimmel»).
    - Lavt liv: `Low = C->GetLowHealthAlpha()`. Hvis > 0: puls som følger hjerteslaget:
      `P = exp(-(Now - C->GetLastHeartbeatTime()) * 6)`; tegn `EdgeGlow((0.45,0,0), (0.25 + 0.35*P) * Low, SizeY*(0.18 + 0.06*P))`.
      Tekst `LAVT LIV – finn dekning og heal` liten (skala 0.8), rødlig, sentrert 28 % ned fra toppen, kun når `Low > 0.6` og
      alfa følger pulsen. (Kort og tydelig; vises bare i alvorlige tilfeller.)
    - Heling: `HA = Now - C->LastHealTime < 0.7 ? 1 - (Now - C->LastHealTime)/0.7 : 0`; `EdgeGlow((0.2, 0.9, 0.35), 0.22*HA, SizeY*0.08)`.
13. `DrawDamageDirection` (~642): tegn én kile per innslag i `C->RecentHits` i stedet for bare `LastDamageFrom`.
    - Levetid 2,0 s; alfa `0.9 * (1 - Age/2)`; kilens bredde `24 + 18*Strength` (halvbredde) og lengde `20 + 16*Strength`.
    - Kilen tegnes med to trekanter: en mørk «skygge» 2 px større (svart, alfa 0.4*alfa) under den røde, så den synes i snø.
    - Bruk den eksisterende matematikken (relativ yaw fra `GetControlRotation()`), slik at kilene dreier når du snur deg.
    - Ikke tegn noe hvis listen er tom. (Fjern ikke `LastDamageFrom`-feltet; bruk det ikke lenger her.)

### E. To rettelser fra runde 1 (dødskamera)
14. `GSTypes.h` `FGSDeathInfo`: legg til `UPROPERTY() FVector_NetQuantize KillerLocation = FVector::ZeroVector;` og
    `UPROPERTY() bool bHasKillerLocation = false;`.
15. `GSCharacter.cpp` `Die()` (~2510–2530): fyll `Info.Killer` fra `Killer->PlayerState` når `Killer` finnes og `Killer != GetController()`
    – *uavhengig* av om han har en pawn (granat/kule fra en som allerede er død). Liv/rustning/avstand og `KillerLocation`
    fylles bare når `KillerPawn` finnes (`Info.KillerLocation = KillerPawn->GetActorLocation(); Info.bHasKillerLocation = true;`).
    NB: `Die` kalles også med `Killer == GetController()` ved selvmord (egen granat) – da skal `Info.Killer` være tom.
16. `AGSCharacter`: nytt offentlig felt `FVector DeathCamKillerLoc = FVector::ZeroVector; bool bDeathCamHasKillerLoc = false;`.
    `AGSPlayerController::ClientDeathCam_Implementation`: sett disse fra `Info` ved siden av `Body->DeathCamKiller = KillerPawn;`.
17. `CalcCamera` (~2656): hvis `DeathCamKiller` er null/ugyldig men `bDeathCamHasKillerLoc`, behandle det som «se mot drapsmannen» med
    `DeathCamKillerLoc` som mål (samme +60 cm løft). Lag en lokal `FVector KillerLook; bool bLookAtKiller` som dekker begge tilfellene
    og bruk den der koden i dag leser `Killer->GetActorLocation()`. Er pawnen gyldig, bruk den levende posisjonen som før.

### F. Bygg, logg, commit
18. Bygg (editoren må være lukket):
    `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" GloshaugenEditor Win64 Development -Project="C:\Users\afno0\Documents\GitHub\Calude-NTNU-Game\Gloshaugen.uproject" -WaitMutex`
    Fiks alle feil og nye advarsler fra `Source/Gloshaugen`.
19. `IMPROVEMENTS.md`: ny seksjon øverst (over `## Runde 6`) med linjene under.
20. Commit kun de endrede/nye filene: `Combat feedback: hurt overlay, hit directions, low-health heartbeat, typed hit sounds`.

## Ferdig når
- [ ] `Build.bat` fullfører uten feil og uten nye advarsler fra `Source/Gloshaugen`.
- [ ] `grep ClientDamageFrom Source/` gir ingen treff; `ClientTookHit` kalles fra `TakeDamage` før `Die()`.
- [ ] Den helrøde fullskjermsboksen er borte; treff gir mørkerød kant som er sterkere og dypere jo mer liv du mistet.
- [ ] Treff i vest/hjelm gir blågrå kant og `Hit_Armor`; vest/hjelm som ryker gir `Armor_Break`; hodeskudd gir hvitt blink, øresus og kraftigere rykk.
- [ ] Opptil fire retningskiler vises samtidig (to skyttere fra ulike kanter gir to kiler), med skygge, og de dreier når du snur deg.
- [ ] Under 35 % liv: hjerteslag som blir raskere jo lavere livet er, pulserende rød kant i takt, gråere bilde og sterkere vignett. Alt forsvinner når du heles over 35 % og når du dør/respawner.
- [ ] Heling (mat, bandasje, sprøyte) gir et kort grønt skjær i kanten.
- [ ] Sult-/tørsteskade gir fortsatt en (svakere) lyd og kant via `OnRep_Health`/`LastDamageTakenTime`.
- [ ] Rus-effektene (Visual 2/5) som selv bruker `ColorSaturation` blir ikke ødelagt av lavt-liv-koden.
- [ ] `FGSDeathInfo` har `KillerLocation`; dødskortet viser navnet også når drapsmannen døde før kula/granaten hans drepte deg; kameraet snur seg mot reserveposisjonen når drapsmannen er utenfor rekkevidde.
- [ ] Fem nye wav-filer i `SourceArt/Sounds`, ingen gamle endret (`git status`).
- [ ] Ingen endring i `Config/DefaultEngine.ini` eller usporede filer er staget.
- [ ] IMPROVEMENTS.md har 301–312 øverst.

## Risiko
- **Nettverk:** `ClientTookHit` er `Unreliable` (som den gamle `ClientDamageFrom`): mister vi en pakke, mangler bare ett blink/én pil –
  helse og død er fortsatt riktige via replikering. Haglskudd gir opptil ~9 RPC-er per skudd (én per hagl som treffer), som før;
  sammenslåingen innen 25° hindrer ni piler, og blink/lyd startes bare på nytt. Vurder å hoppe over lyden hvis forrige `ClientTookHit`
  var < 0,04 s siden (unngår ni lyder oppå hverandre) – gjør det.
- **Rekkefølge RPC vs. `OnRep_Health`:** RPC-en sendes straks i `TakeDamage`, helse replikeres senere i samme nett-tick, så RPC-en
  kommer normalt først og `OnRep_Health` hopper over lyden. Kommer de i motsatt rekkefølge, høres to lyder én gang – ufarlig.
- **Post-process:** kameraets `ColorSaturation` overstyrer værgraderingen fra `GSEnvironment` så lenge du har lavt liv. Det er meningen.
  Pass på at override slås *av* igjen (ellers blir bildet stående gråt etter heling). Ikke rør Visual 2/5.
- **Listen-server-vert:** `ClientTookHit` og `OnRep_Health` (kalles direkte i `TakeDamage`) kjøres lokalt – samme kode, dekket.
- **Lyder finnes ikke før editoren har importert dem:** `GSAudio` hopper stille over manglende lyder, så bygget og spillet virker uansett.
  Andreas må åpne editoren én gang for at de nye lydene skal høres.
- **Ytelse:** ~8 bånd × 4 sider × 2–3 lag `Box`-kall bare når effekten er aktiv; ubetydelig.

## IMPROVEMENTS-linjer
```
## Runde 7: å bli truffet (26. nov.)

301. Når du blir truffet blør kanten av skjermen i stedet for at hele bildet blinker rødt, sterkere jo mer liv du mistet
302. Opptil fire røde piler viser samtidig hvor skuddene kommer fra, også når to skyter på deg fra ulike kanter
303. Pilene har skygge så de synes på snø og himmel, og blir større ved harde treff
304. Eget metallisk «klonk» og blågrå kant når vesten eller hjelmen tar kula
305. Egen lyd når vesten eller hjelmen ryker
306. Hodeskudd du overlever gir hvitt blink, øresus og et kraftigere rykk i kameraet
307. Hagl som treffer samtidig gir én lyd, ikke ni oppå hverandre
308. Under 35 % liv hører du hjertet slå, raskere jo lavere livet er
309. Bildet blir gråere og mørkere i kantene når du har lite liv, med rød puls i takt med hjertet
310. Kort grønt skjær i kanten når du heles
311. Dødskortet viser navnet på drapsmannen også når han selv døde før kula eller granaten hans traff deg
312. Dødskameraet snur seg mot drapsmannen også når han står for langt unna til at spillet ditt ser ham
```
