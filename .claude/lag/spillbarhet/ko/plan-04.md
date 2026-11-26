# Plan-04 (Spillbarhet, runde 4 / «3B»): CTF-regler som mangler – overtid, avslørt bærer, frakobling – og ytelse

Andre halvdel av den opprinnelige plan-03, skrevet om etter kontrollørens forhåndssjekk
(`kontroll/forhand-plan-03.md`: E1, E2, E4 og delen av E3 som gjelder overtid er tatt inn; hovedagenten valgte
**stedsmelding, ikke markør**). Bygger på koden etter plan-03 («3A»: H-oversikt og hint i `GSHelp`).
**Les filene slik de står i arbeidsmappen. Ikke rull tilbake objektsporene i `GSFlag.cpp` (rettingen etter runde 1).**

## Mål
Kampen tåler det som skjer i ekte kamper: et flaggløp som pågår når tiden går ut spilles ferdig, en bærer som gjemmer
seg blir meldt over radioen, og flagget forsvinner ikke når bæreren kobler fra. Og bildet flyter jevnere.

## Forbedrer (gammelt)
- **Kampslutt på tid** (`AGSGameMode::Tick`): kuttes i dag midt i et flaggløp. Blir: overtid når løpet kan avgjøre.
- **Flagget når bæreren kobler fra** (funn C1): legges ned med en gang, og hendelseslista sier «X forlot kampen».
- **Loot og kameraer** (Grafikks bestilling, P1/P9): 428 ting snurrer og 38 kameraer svinger hver frame med skygge.
  Blir: bare det som er nær deg beveger seg, uten skygge, og lysene har avstandsgrense.

## Nytt
- **Overtid** (ønskeliste 33).
- **Flaggbæreren blir synlig for motstanderne etter 20 s** (Andreas' godkjente forslag), som en **radiomelding med
  sted**: «Flagget sist sett: inne i Gamle fysikk, 2. etasje». Ingen markør, ingen meter, ingenting gjennom vegger.
  Punkt på kartet er bestilt fra Kamp.

## Filer
| Fil | Endring |
|---|---|
| `Public/GSGameState.h`, `Private/GSGameState.cpp` | `bOvertime` (replikert), tre konstanter |
| `Public/GSFlag.h`, `Private/GSFlag.cpp` | `bRevealed`/`RevealLocation`, `DropIfCarriedBy`, sperre for hjemmeflagg når tiden er ute |
| `Public/GSGameMode.h`, `Private/GSGameMode.cpp` | `Logout`, `AnyDecisiveFlagAway`, overtid i `Tick` |
| `Private/GSHelp.cpp` | overtid i statuslinja, stedsmelding, linje til bæreren, to regellinjer, to hint-justeringer |
| `Private/GSPickup.cpp`, `Private/GSTerminal.cpp` | ytelse (steg 6) |
| `IMPROVEMENTS.md` | ny seksjon øverst, 468–475 |

Ikke rør `GSCharacter.*`, `GSHUD.*`, `GSPlayerController.*`, `GSTypes.h`, `GSCampus.h` (leses bare), `catalog.py`.
**Gjør de tre headerne ferdige først og i ett.** `CaptureLimit`/`MatchMinutes` beholdes uendret.

## Steg

### 1. `GSGameState`
`.h`, under CTF-feltene:
```cpp
/** Time is out but a flag run that can still decide the match is on. */
UPROPERTY(Replicated) bool bOvertime = false;
static constexpr float OvertimeMaxSeconds = 120.f;
static constexpr float CarrierRevealSeconds = 20.f;
static constexpr float CarrierRevealInterval = 5.f;
```
`.cpp`: `DOREPLIFETIME(AGSGameState, bOvertime);`

### 2. `GSFlag`
`.h` (public):
```cpp
/** The carrier has had the flag for CarrierRevealSeconds: the flag's own team hears where he was last seen. */
UPROPERTY(Replicated) bool bRevealed = false;
UPROPERTY(Replicated) FVector_NetQuantize RevealLocation;   // refreshed every CarrierRevealInterval while bRevealed
/** Server: the carrier is leaving the game; lay the flag down where he stood. */
void DropIfCarriedBy(const APlayerState* PS);
```
protected: `float TakenAt = 0.f; float LastRevealAt = 0.f;`

`.cpp`:
- `GetLifetimeReplicatedProps`: de to nye.
- `Take`: `TakenAt = GetWorld()->GetTimeSeconds(); bRevealed = false;`
- `Drop`, `ReturnHome`, `Capture`, `ResetHome`: `bRevealed = false;` (før `Announce`/`ForceNetUpdate`).
- `ServerStep`, grenen `Carried`, etter at bæreren er funnet gyldig og før erobringssjekken:
  ```cpp
  const float Now = World->GetTimeSeconds();
  if (Now - TakenAt >= AGSGameState::CarrierRevealSeconds
      && (!bRevealed || Now - LastRevealAt >= AGSGameState::CarrierRevealInterval))
  {
      RevealLocation = C->GetActorLocation();
      bRevealed = true;
      LastRevealAt = Now;
      ForceNetUpdate();
  }
  ```
- `ServerStep`, der en motstander tar flagget: legg til vilkåret (E2 – GameMode ticker bare hvert 0,25 s, så selve
  tidspunktet må også sjekkes):
  `&& !(State == EGSFlagState::Home && (GS->bOvertime || GS->GetServerWorldTimeSeconds() >= GS->PhaseEndsAt))`
  Et flagg som ligger ute kan fortsatt plukkes opp.
- `DropIfCarriedBy`: `if (HasAuthority() && State == EGSFlagState::Carried && Carrier == PS) Drop();`
  (`Drop` tåler at pawnen er borte: da brukes `LastCarrierLocation`, som serveren oppdaterer hver frame.)

### 3. `GSGameMode`
`.h`: `virtual void Logout(AController* Exiting) override;` og (protected) `bool AnyDecisiveFlagAway() const;`

**`Logout`** (motoren sletter pawnen i `APlayerController::Destroyed` → `PawnLeavingGame` *før* `Logout` kalles fra
`AController::Destroyed`, `PlayerController.cpp` 1633–1646 og `Controller.cpp` 595–607; `Exiting->PlayerState` lever ennå):
```cpp
void AGSGameMode::Logout(AController* Exiting)
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	APlayerState* PS = Exiting ? Exiting->PlayerState : nullptr;
	if (GS && PS)
	{
		for (TActorIterator<AGSFlag> It(GetWorld()); It; ++It) It->DropIfCarriedBy(PS);   // the flag stays in the match
		if (GS->Phase == EGSMatchPhase::Playing) GS->AddKill(PS->GetPlayerName(), TEXT("kampen"), TEXT("forlot"), false);
	}
	Super::Logout(Exiting);
}
```
(Død, drap til siste skytter og utstyr som blir liggende krever Kamps `PawnLeavingGame` – bestilt.)

**`AnyDecisiveFlagAway()`** (E1): for hvert `AGSFlag F` med `State != Home`: angriperen er `GSTeam::Other(F->Team)`;
`D = GS->TeamScore(F->Team) − GS->TeamScore(Other(F->Team))`. Returner sann hvis `D == 0 || D == 1` for minst ett flagg
(en erobring gir seier fra uavgjort, eller uavgjort fra ett bak). Ellers usann.

**`Tick`, `Playing`-grenen:**
```cpp
if (GS->RedScore >= AGSGameState::CaptureLimit || GS->BlueScore >= AGSGameState::CaptureLimit) EndMatch();
else if (!GS->bOvertime && Now >= GS->PhaseEndsAt)
{
	if (AnyDecisiveFlagAway())   // a flag run that can change the result is on: play it out
	{
		GS->bOvertime = true;
		GS->PhaseEndsAt = Now + AGSGameState::OvertimeMaxSeconds;
		GS->AddKill(TEXT("Tiden er ute"), TEXT("til flagget er hjemme"), TEXT("overtid"), false);
	}
	else EndMatch();
}
else if (GS->bOvertime && (Now >= GS->PhaseEndsAt || !AnyDecisiveFlagAway())) EndMatch();
else if (bSoloMatch && GS->PlayerArray.Num() >= 2) BackToLobby();   // as today
```
`GS->bOvertime = false;` i `StartMatch` (ved klokka), `EndMatch` (etter at vinneren er satt) og `BackToLobby`.
Vinner som i dag (flest erobringer; lik stilling = uavgjort). Kamps kampklokke leser `PhaseEndsAt` og teller ned overtiden.

### 4. `GSHelp.cpp` – overtid og stedsmelding
**a) Overtid** (`GS->bOvertime`, under `Playing`): «Overtid» (skala 0.7, dempet rav) midt i rad 2 i statusboksen. I 8 s
etter at den startet (husk overgangen i `FHelpState`): egen linje «Overtid. Kampen varer til flagget er erobret eller
hentet hjem.»

**b) Til bæreren:** bærer du flagget og flaggets `bRevealed` er sann: bytt «Du bærer flagget. Bær det til basen din.»
med «Motstanderne vet hvor du er. Bær flagget til basen din.»

**c) Stedsmelding til laget som eier flagget** (`Playing`, `PS->Team == Flag->Team`, `Flag->State == Carried &&
Flag->bRevealed`): egen linje under statuslinja i samme stil som de andre (lagets tekstfarge, skala 0.8, ingen blink):
**«Flagget sist sett: %s»**. Teksten regnes hos klienten i en ny hjelper `FString PlaceName(const FVector& WorldCm)`:
- `P` = punktet i meter. For hvert bygg i `GSCampus::Buildings()` som ikke er `bBackdrop`: regn punktet om til byggets
  lokale akser (trekk fra `X`,`Y`, roter med `−Yaw`).
- **Inne:** `|lx| <= SizeX/2 && |ly| <= SizeY/2 && P.Z < Height` → `«inne i %s, %d. etasje»`, etasje =
  `Clamp(FloorToInt(P.Z / FloorHeight) + 1, 1, antall etasjer)`. Er `P.Z >= Height − 0.5` over fotavtrykket →
  `«på taket av %s»`.
- **Ute:** nærmeste bygg (avstand til fotavtrykket). Retning fra byggets midte til punktet, +X er øst og **+Y er sør**:
  velg nord/sør/øst/vest etter største komponent → `«ute, nord for %s»`. Er punktet over 60 m fra alle bygg: «ute i det åpne».
- Visningsnavn: `Hovedbygningen vest` → «Hovedbygningens vestfløy», `Hovedbygningen oest` → «Hovedbygningens østfløy»,
  ellers navnet som det står. (Sjekk listen i `GSCampus.h` slik den er nå – Grafikk endrer den.)
- Regn teksten bare når `RevealLocation` endrer seg (hold forrige punkt og tekst i `FHelpState`), ikke hver frame.
  Linja står så lenge `bRevealed` er sann og byttes når teksten endrer seg. Står en annen egen linje der: tegn under.
- **Ingen markør i verden, ingen avstand, ingen pil.**

**d) H-oversikten:** legg til to regler i `RuleLines` (etter «Bæreren går saktere.» og etter erobringsgrensen):
«Etter 20 sekunder med flagget får motstanderne melding om hvor bæreren sist ble sett.» og
«Kan et flagg som er ute avgjøre kampen når tiden går, spilles det overtid til det er erobret eller hjemme (høyst 2 minutter).»

**e) Hint (E3):** `EnemyFlag` vises ikke når flagget står hjemme og (`bOvertime` eller tiden er ute). I stedet, uten
telling og uten lagring: står du innen 5 m av motstanderens hjemmeflagg i overtid, vis linja
«Overtid: et flagg som står hjemme kan ikke tas.» på hintets plass. `Carry`-hintet får ny tekst:
«Med flagget går du saktere. Etter 20 sekunder får motstanderne vite hvor du er.»

### 5. Bestilling til Kamp (står i `bestillinger.md`)
Punkt på minikartet/PC-kartet for eierlaget fra `AGSFlag::RevealLocation`, «Overtid» ved klokka, `PawnLeavingGame`.
Planen virker uten at noe av det er levert.

### 6. Ytelse (Grafikks bestilling, P1 og P9) – tas ut først hvis runden blir for stor
**`GSPickup.cpp`:**
- Konstruktør: `Mesh->SetCastShadow(false);` (`Crate` beholder skyggen – den står stille.)
- `BeginPlay`: `if (GetNetMode() == NM_DedicatedServer) SetActorTickEnabled(false);`
- `Tick` (ingen nye felt; tick-intervallet er tilstanden):
  ```cpp
  bool bNear = false;
  if (bAvailable && !bLocked)
  {
      const APlayerController* LPC = GetWorld()->GetFirstPlayerController();
      if (LPC && LPC->IsLocalController())
      {
          FVector Eye; FRotator Rot; LPC->GetPlayerViewPoint(Eye, Rot);
          const float FOV = LPC->PlayerCameraManager ? FMath::Max(LPC->PlayerCameraManager->GetFOVAngle(), 5.f) : 90.f;
          const float Range = 3000.f * FMath::Max(1.f, 90.f / FOV);   // further through a scope
          bNear = FVector::DistSquared(Eye, GetActorLocation()) < FMath::Square(Range);
      }
  }
  SetActorTickInterval(bNear ? 0.f : 0.5f);
  if (!bNear) return;
  const float T = GetWorld()->GetTimeSeconds();
  Mesh->SetRelativeRotation(FRotator(0.f, FMath::Fmod(T * 60.f + (GetUniqueID() % 360), 360.f), 0.f));
  // height as today (BaseMeshZ + sin)
  ```
  (Vinkel fra klokka, så tingen ikke hopper når den våkner.)

**`GSTerminal.cpp`:**
- `AGSTerminal`-konstruktøren: `Glow->MaxDrawDistance = 4000.f; Glow->MaxDistanceFadeRange = 800.f;`
- `AGSSecurityCamera`-konstruktøren: `Mesh->SetCastShadow(false); RecLight->MaxDrawDistance = 3000.f;
  RecLight->MaxDistanceFadeRange = 600.f;`
- `AGSSecurityCamera::Tick`, sist: `bWatched = LPC && LPC->GetViewTarget() == this`; nær = øyet under 40 m.
  `SetActorTickInterval(bWatched || bNear ? 0.f : 0.5f);` (Retningen regnes fra servertid, så et kamera som «sover» står
  riktig når noen logger inn. Dedikert server: alltid 0.5.)

### 7. `IMPROVEMENTS.md`: ny seksjon øverst med linjene under. Ikke rør andres linjer.

### 8. Bygg – som i plan-03 (`-WaitMutex`, editoren lukket, ikke rett andre lags filer).

## Kontrollørens liste til byggeren (fra forhåndssjekken, delen som gjelder 3B)
1. Les `GSHelp.cpp`, `GSFlag.cpp` og `GSGameMode.*` slik de står i arbeidsmappen. Ikke rull tilbake objektsporene i `GSFlag.cpp`.
2. Overtid: `AnyDecisiveFlagAway()` **begge** steder i `Tick` (start og slutt); rekkefølgen i `Playing`-grenen som over.
3. `GSFlag::ServerStep`: hjemmeflagg kan ikke tas når `bOvertime` **eller** tiden er ute.
4. Avsløring: server som over, intervall 5 s. Framvisning som stedsmelding. Ingen meter-tall, ingen kantpil, ingen firkant.
5. `EnemyFlag`-hintet av for hjemmeflagg i overtid, og linja for den som står ved hjemmeflagget i overtid.
6. Ytelse som over, med FOV-skalert grense.
7. Rapporter hva som eventuelt ble tatt ut.

## Ferdig når
- Bygget går gjennom uten feil eller nye advarsler i lagets filer; `git status` viser bare filene i tabellen.
- Lest i koden: `bOvertime` nullstilles i `StartMatch`, `EndMatch` og `BackToLobby`; `bRevealed` nullstilles i `Take`,
  `Drop`, `ReturnHome`, `Capture` og `ResetHome`; `Logout` legger ned flagget for den som går.
- Regnet for hånd (byggeren skriver det i rapporten): 2–1 til rødt og rødt bærer blått flagg når tiden går → **ingen**
  overtid. 1–1 og et flagg ute → overtid. 1–2 for laget som bærer → overtid. 0–2 for laget som bærer → ingen overtid.
- `PlaceName` prøvd på fire punkter (meter): (−125, 10, 0) → «inne i Gamle fysikk, 1. etasje»; (−125, 10, 4.5) →
  «2. etasje»; (0, −30, 0) → «ute, nord for Hovedbygningen»; (125, 30, 0) → «ute, sør for Gamle kjemi».
- Det Andreas skal se (to spillere): ta flagget og vent 20 s → den andre får linja «Flagget sist sett: …» som bytter
  når bæreren flytter seg til et nytt sted, og bæreren får «Motstanderne vet hvor du er»; lukk vinduet til bæreren →
  flagget ligger igjen og lista sier «forlot kampen»; loot langt unna står stille, nær deg snurrer det.

## Risiko
- **Avsløringen** går til alle klienter (flagget er alltid relevant); bare eierlaget får meldingen. Den er ikke et
  vern mot juks: bærerens pawn replikeres uansett til alle (eget, gammelt hull hos Kamp – N1 i `funn.md`, bestilt).
- **Stedsnavn** avhenger av `GSCampus::Buildings()` (Grafikks data). Endrer de navn eller fotavtrykk, følger teksten med,
  men visningsnavnene for fløyene må sjekkes.
- **Overtid:** `PhaseEndsAt` flyttes 2 min fram, så Kamps klokke hopper fra 0:00 til 2:00; statuslinja sier «Overtid».
- **Frakobling:** kropp og utstyr forsvinner fortsatt, og ingen får drapet (Kamps fil – bestilt). Flagget er trygt.
  Et lag som blir tomt, og av/på-kobling som fyller hendelseslista, tas i plan-05.
- **Ytelse:** `SetActorTickInterval` fra `Tick` gjelder fra neste tick; 428 ting på 2 Hz er neglisjerbart. Loot uten
  skygge kan se litt løst ut inne, til loot legges på gulvet (backlog).

## IMPROVEMENTS-linjer
```
## Runde 4 (Spillbarhet): overtid, avslørt flaggbærer og frakobling (26. nov.)

468. Overtid: kan et flagg som er ute avgjøre kampen når tiden går, spilles kampen ferdig til det er erobret eller hentet hjem, høyst 2 minutter
469. Når tiden er ute kan ingen ta et flagg som står hjemme, så overtiden kan ikke holdes i gang
470. Etter 20 sekunder med flagget får motstanderne en melding om hvor bæreren sist ble sett, med bygg og etasje, oppdatert hvert femte sekund
471. Bæreren får selv beskjed når motstanderne vet hvor han er
472. Kobler flaggbæreren fra, blir flagget liggende der han sto, og hendelseslista sier at han forlot kampen
473. Loot snurrer bare når du er nær det (lenger unna i kikkertsikte) og kaster ikke skygge mens det svever, så bildet flyter jevnere
474. Overvåkningskameraene svinger bare fort når noen er nær eller ser gjennom dem, og kaster ikke skygge
475. Lysene på PC-er, 3D-printere og kameraer tegnes ikke lenger på lang avstand
```
