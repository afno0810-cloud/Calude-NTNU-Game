# Plan – Spillbarhet runde 1: grunnmuren for Capture the Flag

Andreas har bestemt at spillet foreløpig har én modus: Capture the Flag (se `prioriteringer.md`, øverst).
Denne runden legger reglene på serveren, flagget som aktør, lagene, og en liten egen statusstripe som forklarer
modusen, slik at CTF kan spilles og testes med én gang, også før lag Kamp og lag Grafikk har koblet seg på.
Hovedmenyen (med lagvalg, innstillinger og taster) er plan-02. Utkastet `ko/utkast-knappehjelp.md` (H-oversikt og
hint) bygger videre på `GSHelp.*` fra denne runden og tas etter menyen.

Bygger på commit `55ca90f` + Kamps runder i køen (blod, kroppssoner); de rører ikke filene under.

## Mål
Når kampen starter står du på rødt eller blått lag ved egen base. Du kan ta motstanderens flagg, bære det hjem og
få poeng når ditt eget flagg står hjemme. Første lag til 3 erobringer vinner. Øverst på skjermen ser du hele tiden
stillingen og hvor flaggene er.

## Forbedrer (gammelt)
- **Kampregler** (`AGSGameMode::Tick`/`EndMatch`): «alle mot alle til 25 drap» byttes ut med lag og erobringer.
  Vinneren er et lag, og uavgjort finnes (funn 15: i dag kåres en vinner med 0 drap).
- **Spawn** (`PickSpawnTransform`): i dag et tilfeldig av 16 punkter på hele campus, der flere kan havne på samme
  koordinat (funn 12). Blir: ved egen base, spredt på egne plasser, satt på bakken.
- **Kampstart** (`StartMatch`): rydder miner, ild, røyk, slupne ting og lik, og fyller all loot på nytt (funn 6 +
  bestillingen fra Kamp om prosjektiler).
- **Lobbyen** (`SetReady`/`Tick`): tvungen nedtelling kan ikke avbrytes av én spiller (funn 7); 90 s-fristen
  starter når spiller nr. 2 kommer (lagets funn S1); en kamp startet alene avbrytes når en venn kobler til (S2).

## Nytt
- **To lag** (rødt/blått), automatisk jevn fordeling, bytte lag i lobbyen med **L**.
- **Flagget** som egen aktør (`AGSFlag`, nye filer): står på basen, tas ved å gå bort til det, bæres synlig på
  ryggen, blir liggende der bæreren dør, hentes hjem av eget lag ved å gå bort til det.
- **Erobring og poeng**: lagpoeng, erobringer og returer per spiller, kampslutt ved 3 erobringer eller tid.
- **Statusstripe og innføring** i egne `GSHelp.*`: stilling, hvor hvert flagg er, hvilket lag du er på, og en kort
  forklaring av CTF i lobbyen.
- **Datafelt lag Kamp har bestilt**: `Team`, `CarriedFlagTeam`, `bFriendlyFire`, `MulticastFlagEvent`.

## Hvorfor nå
- Går foran alt annet (Andreas, 26. nov.). CTF-grunnmuren er det beste fundamentet: Kamp venter på lagfeltene
  (venneild, lagfarger, fart for bæreren), Grafikk trenger å vite hva en base er, og hovedmenyens lagvalg og
  knappehjelpens forklaring trenger reglene.
- Alt er C++ i lagets egne filer + tre nye filer. Ingen .uasset, ingen endring i `catalog.py`.

## Filer
| Fil | Endring |
|---|---|
| `Public/GSFlag.h`, `Private/GSFlag.cpp` (NYE) | `AGSFlag` |
| `Public/GSHelp.h`, `Private/GSHelp.cpp` (NYE) | statusstripe, lobby-innføring, L-tasten, krok i `AHUD::OnHUDPostRender` |
| `Public/GSPlayerState.h`, `Private/GSPlayerState.cpp` | `GSTeam`, `Team`, `CarriedFlagTeam`, `Captures`, `Returns`, `ServerRequestTeam` |
| `Public/GSGameState.h`, `Private/GSGameState.cpp` | lagpoeng, `bFriendlyFire`, `MulticastFlagEvent`, siste flagghendelse, `EnsureHooked` |
| `Public/GSGameMode.h`, `Private/GSGameMode.cpp` | lag, baser, flagg, spawn, kampregler, rydding, lobby |
| `Public/GSPickup.h`, `Private/GSPickup.cpp` | `ResetForMatch()` |
| `Public/GSTerminal.h`, `Private/GSTerminal.cpp` | `CancelPrint()` |
| `IMPROVEMENTS.md` | ny seksjon øverst, 346–359 |

(alle under `Source/Gloshaugen/`). Ikke rør `GSCharacter.*`, `GSHUD.*`, `GSPlayerController.*`, `GSProjectile.*`,
`GSEffects.*`, `GSTypes.h`, `GSCampus*.h` – de kan leses og `#include`-s. `gen_cpp.py` skal ikke kjøres.
Gjør header-endringene ferdig først og i ett (andre lag inkluderer `GSPlayerState.h`/`GSGameState.h`).

## Steg

### 1. `GSPlayerState`
I `GSPlayerState.h`, over klassen:
```cpp
/** Capture the Flag teams. 0 = none yet. */
namespace GSTeam
{
	constexpr uint8 None = 0, Red = 1, Blue = 2;
	inline uint8 Other(uint8 T) { return T == Red ? Blue : (T == Blue ? Red : None); }
	inline const TCHAR* Name(uint8 T) { return T == Red ? TEXT("Rødt lag") : (T == Blue ? TEXT("Blått lag") : TEXT("Uten lag")); }
	inline const TCHAR* FlagName(uint8 T) { return T == Red ? TEXT("det røde flagget") : TEXT("det blå flagget"); }
	/** Sober cloth colours, not neon. */
	inline FLinearColor Color(uint8 T) { return T == Red ? FLinearColor(0.55f, 0.07f, 0.06f) : (T == Blue ? FLinearColor(0.07f, 0.17f, 0.50f) : FLinearColor(0.5f, 0.5f, 0.5f)); }
}
```
I klassen (alle `UPROPERTY(Replicated, BlueprintReadOnly)`, replikert til alle med `DOREPLIFETIME`):
`uint8 Team = 0;` · `uint8 CarriedFlagTeam = 0;` (0 = bærer ikke) · `int32 Captures = 0;` · `int32 Returns = 0;`
og `UFUNCTION(Server, Reliable) void ServerRequestTeam(uint8 NewTeam);` →
`if (AGSGameMode* GM = GetWorld()->GetAuthGameMode<AGSGameMode>()) GM->RequestTeam(this, NewTeam);`
(Navnene `Team` og `CarriedFlagTeam` er de Kamp har bestilt – ikke endre dem.)

### 2. `GSGameState`
`.h`:
```cpp
UPROPERTY(Replicated) uint8 RedScore = 0;
UPROPERTY(Replicated) uint8 BlueScore = 0;
/** Off: team mates cannot hurt each other (checked in AGSCharacter::TakeDamage by the combat team). */
UPROPERTY(Replicated) bool bFriendlyFire = false;
int32 TeamScore(uint8 Team) const { return Team == 1 ? RedScore : (Team == 2 ? BlueScore : 0); }
static constexpr int32 CaptureLimit = 3;
static constexpr float LobbyAutoStartSeconds = 90.f;

/** Event: 1 taken, 2 dropped, 3 returned, 4 captured (GSFlagEvent in GSFlag.h). Runs on every machine. */
UFUNCTION(NetMulticast, Reliable)
void MulticastFlagEvent(uint8 Event, uint8 FlagTeam, FVector_NetQuantize Where, APlayerState* Who);
/** Last flag event on this machine (for the status line and for sounds). LocalTime = GetWorld()->GetTimeSeconds(). */
struct FLastFlagEvent { uint8 Event = 0; uint8 FlagTeam = 0; FVector Where = FVector::ZeroVector; FString Who; float LocalTime = -100.f; };
FLastFlagEvent LastFlagEvent;
DECLARE_MULTICAST_DELEGATE_FourParams(FOnFlagEvent, uint8, uint8, const FVector&, APlayerState*);
FOnFlagEvent OnFlagEvent;   // the combat team binds sounds / HUD cues here
```
`KillLimit` beholdes som konstant (HUD-en til Kamp leser den), men brukes ikke lenger i reglene.
`.cpp`: `DOREPLIFETIME` for de tre nye; `MulticastFlagEvent_Implementation` fyller `LastFlagEvent`
(`Who ? Who->GetPlayerName() : FString()`) og kaller `OnFlagEvent.Broadcast(...)`.
`BeginPlay`, først: `if (GetNetMode() != NM_DedicatedServer) GSHelp::EnsureHooked();`

### 3. `GSFlag.h/.cpp` (ny aktør)
```cpp
namespace GSFlagEvent { constexpr uint8 Taken = 1, Dropped = 2, Returned = 3, Captured = 4; }
UENUM() enum class EGSFlagState : uint8 { Home, Carried, Dropped };

UCLASS()
class GLOSHAUGEN_API AGSFlag : public AActor
{
	GENERATED_BODY()
public:
	AGSFlag();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(ReplicatedUsing = OnRep_Look) uint8 Team = 0;               // whose flag
	UPROPERTY(ReplicatedUsing = OnRep_Look) EGSFlagState State = EGSFlagState::Home;
	UPROPERTY(ReplicatedUsing = OnRep_Look) TObjectPtr<APlayerState> Carrier;
	UPROPERTY(ReplicatedUsing = OnRep_Look) FVector_NetQuantize DropLocation;   // floor point where it lies

	/** Where the cloth is right now (base, carrier's back or on the ground). */
	FVector GetFlagLocation() const;
	/** Server: back to the base, no event (match start). */
	void ResetHome();

	static constexpr float TouchRadius = 130.f;     // cm, 2D
	static constexpr float CaptureRadius = 200.f;
	static constexpr float SafetyReturnSeconds = 180.f;   // a dropped flag nobody can reach goes home
protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;        // stays at the base
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BasePlate;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> FlagRoot;    // moved in world space
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Pole;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Cloth;
	UFUNCTION() void OnRep_Look();
	void ServerStep();
	void Take(class AGSCharacter* By);
	void Drop();
	void ReturnHome(class AGSPlayerState* By);   // By may be null (safety return)
	void Capture(class AGSPlayerState* By);
	void Announce(uint8 Event, const FVector& Where, APlayerState* Who, const TCHAR* Verb);
	float ServerAccum = 0.f;
	float DroppedAt = 0.f;
};
```
- **Konstruktør:** `bReplicates = true; bAlwaysRelevant = true; SetNetUpdateFrequency(10.f);` tick på. Aktøren
  **flytter seg aldri** (står på basen); bare `FlagRoot` flyttes, med `SetUsingAbsoluteLocation(true)` og
  `SetUsingAbsoluteRotation(true)`. Alle mesh uten kollisjon og uten overlapp (som i `AGSPickup`).
  `BasePlate`: sylinder, skala (1.2, 1.2, 0.04). `Pole`: sylinder, skala (0.035, 0.035, 2.3), relativ Z 115.
  `Cloth`: kube, skala (0.85, 0.015, 0.5), relativ (45, 0, 200). Mesh hentes som i `GSPickup.cpp`
  (`ConstructorHelpers` for `/Engine/BasicShapes/Cube` og `Cylinder`).
- **`OnRep_Look`** (kalles også fra `BeginPlay` og etter hver endring på serveren): farg `Cloth` og `BasePlate`
  med `GSUtils::SetColor(..., GSTeam::Color(Team))` bare når `Team` har endret seg siden sist (lagre `AppliedTeam`),
  `Pole` mørk grå (0.12). Flagget legges ned (`FlagRoot` roll 80°) når `State == Dropped`, ellers oppreist.
- **`GetFlagLocation`:** `Home` → `GetActorLocation()`; `Dropped` → `DropLocation`; `Carried` → bærerens pawn
  (`Carrier->GetPawn()`): `Loc - Forward * 28 + (0,0,-50)` (stanga langs ryggen, duken over hodet); er pawnen
  ikke kjent på denne maskinen: siste kjente posisjon.
- **`Tick`:** alle maskiner: `FlagRoot->SetWorldLocation(GetFlagLocation())`, og ved `Carried` yaw = pawnens yaw.
  Skjul `FlagRoot` (`SetHiddenInGame`) når `Carrier` er den lokale spilleren (ellers står stanga i synsfeltet i
  førsteperson) – sammenlign med `GetWorld()->GetFirstPlayerController()->PlayerState`. Server:
  `ServerAccum += Dt; if (ServerAccum >= 0.1f) { ServerAccum = 0; ServerStep(); }`.
- **`ServerStep`** (bare `HasAuthority()`, og bare når `GS->Phase == EGSMatchPhase::Playing`):
  - `Carried`: `AGSPlayerState* PS = Cast<AGSPlayerState>(Carrier)`, `AGSCharacter* C = PS ? Cast<AGSCharacter>(PS->GetPawn()) : nullptr`.
    Hvis `!PS || !C || C->bDead || PS->Team == Team` → `Drop()`. Ellers: finn eget lags flagg (det andre
    `AGSFlag` med `Team == PS->Team`, via `TActorIterator`); hvis det har `State == Home` og
    `FVector::Dist2D(C->GetActorLocation(), Own->GetActorLocation()) < CaptureRadius` og høydeforskjell < 250 →
    `Capture(PS)`. (Står eget flagg ikke hjemme, skjer ingenting – statusstripa forklarer hvorfor.)
  - `Home` / `Dropped`: for hver levende `AGSCharacter` (`!bDead`) med `AGSPlayerState` og `Team != 0`, innen
    `TouchRadius` (2D) og høydeforskjell < 200 fra `GetFlagLocation()`, og med fri sikt
    (`LineTraceTestByChannel` `ECC_WorldStatic` fra spillerens midte til flagget + 60 cm opp; treff = hopp over):
    motstander med `CarriedFlagTeam == 0` → `Take(C)`; eget lag og `Dropped` → `ReturnHome(PS)`. Stopp ved første.
  - `Dropped`: hvis `Now - DroppedAt > SafetyReturnSeconds` eller `DropLocation.Z < -5000` → `ReturnHome(nullptr)`.
- **`Take`:** `State = Carried; Carrier = PS; PS->CarriedFlagTeam = Team;` + `Announce(Taken, ..., TEXT("tok"))`.
- **`Drop`:** posisjon = bærerens pawn hvis den finnes, ellers siste `GetFlagLocation()`; linjespor
  (`ECC_WorldStatic`) fra +50 til −600 for gulvet → `DropLocation`. `State = Dropped; DroppedAt = Now;`
  nullstill `CarriedFlagTeam` på den gamle bæreren (hvis den finnes); `Carrier = nullptr;`
  `Announce(Dropped, ..., TEXT("mistet"))`.
- **`ReturnHome(By)`:** `State = Home; Carrier = nullptr;` `if (By) { By->Returns++; Announce(Returned, ..., TEXT("hentet hjem")); }`
  ellers `Announce` med `Who = nullptr`.
- **`Capture(By)`:** nullstill `By->CarriedFlagTeam`, `By->Captures++`, `By->SetScore(By->GetScore() + 5.f)`,
  `State = Home; Carrier = nullptr;` `Announce(Captured, ..., TEXT("erobret"))`, og
  `GetWorld()->GetAuthGameMode<AGSGameMode>()->OnFlagCaptured(By->Team)`.
- **`Announce`:** `GS->MulticastFlagEvent(Event, Team, Where, Who)`, og en linje i drapsloggen så den synes i
  dagens HUD uten endringer: `GS->AddKill(Who ? Who->GetPlayerName() : GSTeam::Name(Team), GSTeam::FlagName(Team), Verb, false)`
  (HUD skriver «Kari  [tok]  det blå flagget»). Kall `OnRep_Look()` og `ForceNetUpdate()` etter hver endring.
- **`ResetHome`:** som `ReturnHome(nullptr)` uten `Announce`; nullstill også `CarriedFlagTeam` på bæreren.

### 4. `GSGameMode`
`.h` (nye): `void RequestTeam(AGSPlayerState* PS, uint8 NewTeam);` · `void OnFlagCaptured(uint8 ScoringTeam);` ·
`static FVector BasePointM(uint8 Team);` · `void SpawnFlags();` · `void AssignTeam(AGSPlayerState* PS);` ·
`int32 TeamCount(uint8 Team) const;` · `void CleanWorldForMatch();` · `void StartCountdown(bool bForced);`
(erstatter den uten parameter) · `bool bCountdownForced = false;` · `int32 LobbyPlayersLast = 0;` ·
`bool bSoloMatch = false;` · `FTransform PickSpawnTransform(const AController* For) const;` (erstatter den uten
parameter) · `static constexpr int32 MaxPerTeam = 10;`

- **`BasePointM`** (meter) – ETT sted som vet hvor basene er. **Sjekk `GSCampus.h` rett før du skriver den:**
  - Finnes `GSCampus::Bases()` (Grafikks runde 1 lover den, se bestillingen deres øverst i `bestillinger.md`:
    indeks 0 «Vest» ved Gamle fysikk, flagg (−125, 10); indeks 1 «Øst» ved Gamle kjemi, flagg (125, 10); 7
    spawnpunkter og `SpawnYaw` per base): les flaggpunkt fra den (rød = indeks 0, blå = indeks 1), og la
    `PickSpawnTransform` bruke basens egne spawnpunkter og `SpawnYaw` som kandidater i stedet for viften under
    (samme bakke-spor og ledig-sjekk). Les feltnavnene i headeren, ikke gjett.
  - Finnes den ikke ennå: rød `(-60, -80, 0)`, blå `(60, -80, 0)` (to av dagens spawnpunkter, plassen foran
    Hovedbygningen) og viften under. Skriv `// TODO: GSCampus::Bases()` og si fra i rapporten, så tas byttet i
    neste runde.
  - Finnes `GSCampus::PlayArea()`: bruk den (krympet 5 m) for utendørs loot i `SpawnPickups` i stedet for
    `X −280…430, Y −110…520`, og legg ut 20 plasser ute i stedet for 60 (Grafikks ønske a).
- **`SpawnFlags`** (kalles i `BeginPlay` etter `SpawnTerminals`): for lag 1 og 2: linjespor ned
  (`ECC_WorldStatic`, fra Z +300 til −300 cm) ved basepunktet; aktørens plass = treffpunkt (ellers Z 0);
  `SpawnActor<AGSFlag>`, sett `Team`. (Campus er bygget før `GameMode::BeginPlay`: `UGSWorldSubsystem::OnWorldBeginPlay`.)
- **`AssignTeam`:** laget med færrest spillere; likt → tilfeldig. `RestartPlayer`: først
  `if (PS && PS->Team == 0) AssignTeam(PS);` så `RestartPlayerAtTransform(NewPlayer, PickSpawnTransform(NewPlayer))`.
- **`RequestTeam`:** bare i `Lobby`; `NewTeam` må være 1 eller 2 og ulik dagens; tillatt bare hvis
  `TeamCount(NewTeam) < TeamCount(Old)` og `< MaxPerTeam` (lagene blir aldri skjevere av et bytte). Sett
  `PS->Team`, `PS->bReady = false`, og flytt spilleren til ny base: `UnPossess` + `Destroy` pawn + `RestartPlayer`.
- **`PickSpawnTransform(For)`:** `Team` fra `For` sin `AGSPlayerState` (0 → behandle som 1). Base = `BasePointM(Team) * 100`,
  `Away` = enhetsvektor fra motstanderens base til egen. 12 kandidater: `Base + Away.RotateAngleAxis(A, Up) * R`
  med `A` jevnt fra −100° til +100° og `R` vekselvis 700 og 1300 cm. For hver: linjespor ned for bakken
  (+400 → −300), `Loc.Z = bakke + 100`; forkast hvis `OverlapBlockingTestByChannel(Loc, FQuat::Identity, ECC_Pawn,
  FCollisionShape::MakeCapsule(42, 92))` slår til, eller en levende `AGSCharacter` står nærmere enn 150 cm.
  Velg tilfeldig blant de gyldige; ingen gyldige → `Base + (0,0,120)` som i dag. Rotasjon: se mot motstanderens base.
- **`Tick`:**
  - `Lobby`: tell `Total`/`Ready` som i dag, så:
    ```cpp
    if (Total >= 2 && LobbyPlayersLast < 2) LobbyStartedAt = Now;   // the 90 s start when player two arrives
    LobbyPlayersLast = Total;
    GS->PhaseEndsAt = Total >= 2 ? LobbyStartedAt + AGSGameState::LobbyAutoStartSeconds : 0.f;
    if (Total > 0 && Ready == Total) StartCountdown(false);
    else if (Total >= 2 && Now - LobbyStartedAt > AGSGameState::LobbyAutoStartSeconds) StartCountdown(true);
    ```
    (`AGSHUD::DrawMatch` leser ikke `PhaseEndsAt` i lobby-fasen, så dette er trygt.)
  - `Playing`: fjern drapsgrensen. `if (GS->RedScore >= CaptureLimit || GS->BlueScore >= CaptureLimit || Now >= GS->PhaseEndsAt) EndMatch();`
    og `if (bSoloMatch && GS->PlayerArray.Num() >= 2) { /* a friend arrived: stop the solo match */ BackToLobby(); }`
    (`BackToLobby` må da også kalle `ResetHome()` på flaggene og sette `LobbyPlayersLast = 0` så fristen starter nå.)
- **`SetReady`:** `if (!bReady && GS->Phase == EGSMatchPhase::Countdown && !bCountdownForced) GS->Phase = EGSMatchPhase::Lobby;`
- **`StartCountdown(bForced)`:** som i dag + `bCountdownForced = bForced`.
- **`CleanWorldForMatch`** (`#include "GSProjectile.h"`, `"GSFlag.h"`). Samle i `TArray` først, slett etterpå:
  alle `AGSProjectile` → `Destroy()`; `AGSPickup` med `bRespawns == false` → `Destroy()`, resten →
  `ResetForMatch()`; alle `AGSTerminal` → `CancelPrint()`; alle `AGSFlag` → `ResetHome()`; alle `AGSCharacter`
  som er igjen (lik) → `Destroy()`.
- **`StartMatch`:** rekkefølge:
  1. fase, klokke, `Winner.Empty()`, `KillFeed.Reset()`, `RedScore = BlueScore = 0`; per spiller: `Kills`,
     `Deaths`, `Captures`, `Returns`, `CarriedFlagTeam` = 0, `SetScore(0)`;
  2. jevn ut lagene: spillere med `Team == 0` får lag; så lenge forskjellen er over 1, flytt en tilfeldig spiller
     fra det største laget;
  3. `bSoloMatch = (antall spillere == 1)`;
  4. `UnPossess` + `Destroy` på **alle** gamle kropper (ingen `RestartPlayer` ennå);
  5. `CleanWorldForMatch()`;
  6. `RestartPlayer(C)` for hver kontroller.
- **`OnFlagCaptured(Team)`:** `RedScore`/`BlueScore` += 1 (kampslutt sjekkes i `Tick`).
- **`EndMatch`:** `GS->Winner = Red > Blue ? GSTeam::Name(1) : (Blue > Red ? GSTeam::Name(2) : FString());`
  (tom = uavgjort; `GSHUD` linje 801 skriver da «Ingen vinner», og vår stripe skriver «Uavgjort»). Flaggene
  `ResetHome()`.
- **`OnPlayerKilled`:** drap på lagkamerat (`KillerPS->Team == VictimPS->Team`, `Team != 0`, ikke seg selv) gir
  ikke drap eller poeng. Resten som i dag (flagget slippes av `AGSFlag::ServerStep` når bæreren er død).

### 5. `GSPickup::ResetForMatch()` og `GSTerminal::CancelPrint()` (public, server)
```cpp
void AGSPickup::ResetForMatch() { GetWorldTimerManager().ClearTimer(RespawnTimer); Roll(); ForceNetUpdate(); }
void AGSTerminal::CancelPrint() { GetWorldTimerManager().ClearTimer(PrintTimer); PrintingId = NAME_None; PrintStart = PrintEnd = 0.f; ForceNetUpdate(); }
```

### 6. `GSHelp.h/.cpp` (nye, ingen UCLASS)
```cpp
namespace GSHelp
{
	/** Call once per frame for the local player's HUD: CTF status line, lobby intro, L = switch team. */
	GLOSHAUGEN_API void Draw(AHUD* HUD, UCanvas* Canvas);
	/** Draws through AHUD::OnHUDPostRender for as long as no HUD calls Draw itself. Safe to call many times. */
	GLOSHAUGEN_API void EnsureHooked();
}
```
- **Tegnehjelpere** i anonymt namespace, samme uttrykk som `AGSHUD::Text`/`Box` (`GSHUD.cpp` linje 18–45):
  `GEngine->GetMediumFont()`, svart skygge, `FCanvasTileItem` med `SE_BLEND_Translucent`. Nøkternt: mørkt panel
  `(0.02, 0.025, 0.03, 0.78)`, lys grå tekst, lagfargene fra `GSTeam::Color` lysnet litt for tekst. Ingen blink,
  ingen store poengtekster, ingen lyd.
- **`EnsureHooked`:** `static bool bHooked;` én gang: `AHUD::OnHUDPostRender.AddStatic(&OnPostRender);`
  `OnPostRender(AHUD* HUD, UCanvas* Canvas)`: returner hvis null, eller hvis HUD-en nylig har kalt `Draw` selv
  (`TMap<TWeakObjectPtr<const AHUD>, uint64> LastDirectFrame`; `GFrameCounter - Last <= 2`). Motoren sender
  `DebugCanvas` her (`Engine/Private/HUD.cpp` linje 240) – bruk `Canvas->SizeX/SizeY` fra lerretet du får.
- **`Draw`:** `PC = HUD->GetOwningPlayerController()`, bare lokal. `GS`, `PS` (egen `AGSPlayerState`). Ikke tegn
  når `AGSPlayerController::bMenuOpen` eller `IsOnTerminal()`.
  1. **Statusstripe** (alltid, øverst i midten, `Y = 8`, ca. 520 × 34; kampklokka til HUD står på Y 46–76 og
     lobby-lista fra Y 60, så det er fritt over): «RØDT  1 – 0  BLÅTT» i midten (lagnavn i lagfarge, tall hvite,
     skala 1.0), og under hver side flaggets tilstand i skala 0.7, lest fra de to `AGSFlag` (`TActorIterator`):
     «hjemme» / «bæres av Kari» / «ligger ute». Under `Lobby`/`Countdown` vises stripa uten tall («Capture the Flag»).
  2. **Egen linje** under stripa (skala 0.8), første som passer:
     - du bærer flagg (`PS->CarriedFlagTeam != 0`): «Du bærer flagget. Bær det til basen din.» – og hvis eget
       flagg ikke er hjemme: «Flagget ditt er borte. Du får ikke poeng før det er hentet hjem.»
     - siste flagghendelse yngre enn 6 s (`GS->LastFlagEvent`): «Kari tok det blå flagget» / «… mistet …» /
       «… hentet hjem …» / «… erobret …» / uten navn: «Det røde flagget er tilbake på basen».
     - ellers ingenting.
  3. **Lobby-panel** (`Lobby`/`Countdown`), venstre kant (`X = 28`, `Y = SizeY * 0.30`, bredde 400), overskrift
     «Capture the Flag», linjer i skala 0.8:
     - «Du er på %s.» (lagnavn i lagfarge) + «L bytter lag (når det andre laget har færre).»
     - «Ta motstanderens flagg ved å gå bort til det, og bær det til din egen base.»
     - «Du får bare poeng når ditt eget flagg står hjemme.»
     - «Dør du, blir flagget liggende. Eget flagg hentes hjem ved å gå bort til det.»
     - «Første lag til %d erobringer vinner, ellers flest når %d minutter er gått.» (`CaptureLimit`, `MatchMinutes`)
     - «Oppvarming nå, ingen skade. Trykk Enter når du er klar.»
     - hvis `Phase == Lobby && PhaseEndsAt > 0`: «Starter av seg selv om %d s.»
     (HUD-en til Kamp skriver fortsatt «Først til 25 drap …» under lobby-lista til bestillingen er levert.)
  4. **Kampslutt** (`Ended`): under stripa: «Rødt lag vant 3–1» / «Uavgjort 1–1» (skala 1.0).
  5. **L-tasten:** `if (GS->Phase == EGSMatchPhase::Lobby && PS && PC->WasInputKeyJustPressed(EKeys::L)) PS->ServerRequestTeam(GSTeam::Other(PS->Team));`
     (L er ikke brukt i `AGSPlayerController::SetupInputComponent`.)

### 7. `IMPROVEMENTS.md`: ny seksjon øverst med linjene under. Ikke rør andres linjer.

### 8. Bygg
`Get-Process UnrealEditor` (skal ikke kjøre). Bygg med kommandoen i `CLAUDE.md`, **med** `-WaitMutex`. Feiler
bygget i en fil laget ikke eier: vent og prøv igjen (inntil 4 ganger), ikke rett filen.

## Ferdig når
- Bygget går gjennom uten feil eller nye advarsler i lagets filer; `git status` viser bare filene i «Filer».
- Lest i koden: en spiller får `Team` før pawnen lages (`RestartPlayer`), og `StartMatch` gir aldri lag som
  avviker med mer enn 1.
- Lest i koden: flagget kan bare tas av motstander, bare under `Playing`, og erobring krever at eget flagg er
  `Home`. Død/frakoblet bærer → `Dropped` innen 0,1 s, og `CarriedFlagTeam` er 0 igjen.
- Lest i koden: ingen spiller får ny kropp i `StartMatch` før `CleanWorldForMatch()` er kjørt.
- Det Andreas skal se (Play med 2 spillere, «Play As Listen Server»): stripa øverst med «Capture the Flag» og
  panelet til venstre i lobbyen; L bytter lag når det er lov; begge trykker Enter; hver starter ved sin base med
  et flagg i lagets farge; gå bort til det andre flagget → det følger deg (den andre ser det på ryggen din), og
  stripa sier «bæres av …»; gå hjem til eget flagg → 1–0 og flagget er tilbake; tre ganger → «Rødt lag vant 3–0».
  Alene (1 spiller) kan han også starte og erobre det tomme lagets flagg.

## Risiko
- **Venneild:** til Kamp har lagt inn sjekken i `TakeDamage` (de har selv bestilt feltene) kan lagkamerater skade
  hverandre. Drap på lagkamerat gir ikke poeng.
- **Man ser ikke hvem som er venn:** spillerne har ingen lagfarge på kroppen før Kamp leverer (bestilling). Stripa
  og flaggfargene virker uansett.
- **Basene er foreløpige** (plassen foran Hovedbygningen, åpent lende) til Grafikk har valgt CTF-utsnittet.
  Alt går gjennom `BasePointM`, så byttet er én funksjon.
- **Flaggmodellen** er stang + duk av grunnformer (reserve). Ekte modell er bestilt hos Grafikk.
- **Tegning via `OnHUDPostRender`/`DebugCanvas`:** kan ha annen skala enn HUD-lerretet til Kamp kaller
  `GSHelp::Draw` direkte. Er `DebugCanvas` null, tegnes ingenting (ingen krasj). Delegaten registreres én gang og
  holder ingen pekere.
- **Replikering:** `APlayerState::GetPawn()` på klienter er null til pawnen er relevant; da brukes siste kjente
  posisjon. Flagget er `bAlwaysRelevant` (to aktører), så alle kjenner tilstanden. `MulticastFlagEvent` er
  pålitelig og sendes noen få ganger i minuttet.
- **Rekkefølge ved kampslutt/-start:** `ResetHome` nullstiller `CarriedFlagTeam`; `StartMatch` gjør det også per
  spiller (lærdom runde 1: sett tilstanden begge steder).
- **HUD-tekster fra Kamp som nå er feil** («Først til 25 drap», «Vinner: …» for enkeltspiller, poengtavle uten
  lag): bestilt, ufarlig i mellomtiden.
- Lik slettes ved kampstart (Kamp lar dem ligge 90 s); blodflekker eies av Kamp og ryddes ikke her.

## IMPROVEMENTS-linjer
```
## Runde 1 (Spillbarhet): Capture the Flag, grunnmuren (26. nov.)

346. Ny spillmodus: Capture the Flag. To lag, rødt og blått, med hver sin base og hvert sitt flagg
347. Du settes automatisk på laget med færrest spillere, og lagene jevnes ut når kampen starter
348. I lobbyen bytter du lag med L, så lenge det ikke gjør lagene skjevere
349. Flagget tas ved å gå bort til det, og bæres synlig på ryggen så alle ser hvem som har det
350. Dør bæreren, blir flagget liggende der han falt. Motstanderne kan plukke det opp igjen, og eget lag henter det hjem ved å gå bort til det
351. Du erobrer ved å bære flagget til din egen base, men bare når ditt eget flagg står hjemme
352. Første lag til 3 erobringer vinner, ellers laget med flest når tiden er ute. Lik stilling gir uavgjort
353. Du starter og kommer tilbake ved din egen base, på en ledig plass, vendt mot motstanderen (før: tilfeldig sted på hele campus)
354. Stripe øverst på skjermen viser stillingen og om hvert flagg står hjemme, bæres (og av hvem) eller ligger ute
355. Alle får beskjed når et flagg tas, mistes, hentes hjem eller erobres, også i hendelseslista til venstre
356. Lobbyen forklarer Capture the Flag i korte linjer, viser hvilket lag du er på og når kampen starter av seg selv
357. Kampstart rydder campus: miner, ild, røyk, slupne ting og lik fjernes, og all loot og alle låste kasser fylles på nytt
358. Nedtelling som starter fordi tiden er ute kan ikke stoppes av én spiller, og fristen på 90 sekunder starter først når spiller nummer to er inne
359. En kamp du startet alene avbrytes når en venn kobler til, så dere kan starte sammen. Drap på lagkamerat gir ikke poeng
```
