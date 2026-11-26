# UTKAST (ikke i køen) – knappehjelp: H-oversikt, hint og innføring

Skrevet før Andreas bestemte Capture the Flag. Tas som del av plan-03 (se `retning.md`): tekstene må oppdateres
til CTF, `GSHelp.*` og kampstart-fiksene finnes da allerede fra plan-01, og IMPROVEMENTS-numrene under er brukt opp.

feilene i lobby/kampstart fra `funn.md` (6, 7, 15) og bestillingen fra lag Kamp om å rydde prosjektiler.
Hovedmenyen (0b) er plan-02. Bygger på koden slik den er i commit `55ca90f` + Kamps runde 3 (blod), som ikke
rører filene under.

## Mål
En ny spiller skjønner uten å spørre noen hvilke taster som gjør hva og hva kampen går ut på: **H** viser alle
tastene, lobbyen forklarer kampen, og korte hint dukker opp første gang noe er aktuelt. Og når kampen starter, er
campus ryddet og likt for alle.

## Forbedrer (gammelt)
- **Kampstart** (`AGSGameMode::StartMatch`): i dag blir miner, molotov-ild, røyk, slupne ting, plukket loot og lik
  fra oppvarmingen/forrige kamp med inn i kampen (funn 6 + bestilling fra Kamp: miner varer nå evig). Blir: alt
  ryddes, og alle lootplasser og låste kasser fylles på nytt.
- **Lobbyen** (`SetReady`/`Tick`): én spiller kan i dag stoppe nedtellingen om og om igjen (funn 7), og 90 s-fristen
  løper fra serveren startet, så spiller nr. 2 kan komme inn og få kampstart etter 10 s. Blir: tvungen nedtelling
  kan ikke avbrytes, og fristen starter når spiller nr. 2 kommer.
- **Kampslutt** (`EndMatch`): i dag kåres første spiller i lista som vinner selv med 0 drap (funn 15). Blir: ingen
  vinner uten drap, delt seier ved helt lik stilling.
- **Hjelpeteksten** som i dag står 30 s midt på skjermen og så er borte for alltid (`AGSHUD::DrawHelp`): erstattes
  av oversikten under (selve fjerningen er en bestilling til Kamp; vårt virker uansett).

## Nytt
- **Tasteoversikt på H**: nøktern, mørk tavle med alle taster i grupper. H lukker igjen.
- **Innføring i lobbyen**: fem korte linjer om hva kampen går ut på, + «Starter automatisk om N s».
- **Hint når de er aktuelle**: første gang du ser på et våpen, en ting, en låst kasse, en PC, en 3D-printer; første
  gang du er skadet, sulten, tørst, tom for skudd, og første gang du dør. Hvert hint vises høyst to ganger og huskes
  mellom økter.
- Ny kode i **egne nye filer** laget eier: `GSHelp.h` / `GSHelp.cpp`.

## Hvorfor nå
- Direkte ønske fra Andreas, satt øverst (0a) av hovedagenten, og skal tas i runde 1 eller 2.
- Det er den av 0a/0b som trygt lar seg bygge og kompilere i én runde: ren tegning på Canvas, ingen nettkode,
  ingen .uasset. Tastelista gjenbrukes av hovedmenyen (0b, «Kontroller») i plan-02.
- Kampstart-fiksene er få linjer i `GSGameMode` og henger sammen med innføringen i lobbyen. Kamp har bestilt
  prosjektil-ryddingen.

## Filer
| Fil | Endring |
|---|---|
| `Source/Gloshaugen/Public/GSHelp.h` (NY) | `FGSControlRow`, `namespace GSHelp` |
| `Source/Gloshaugen/Private/GSHelp.cpp` (NY) | tasteliste, tegning, hint, innføring, reserve-krok |
| `Source/Gloshaugen/Public/GSGameState.h`, `Private/GSGameState.cpp` | konstant `LobbyAutoStartSeconds`, kall `GSHelp::EnsureHooked()` |
| `Source/Gloshaugen/Public/GSGameMode.h`, `Private/GSGameMode.cpp` | rydding i `StartMatch`, lobby-regler, `EndMatch` |
| `Source/Gloshaugen/Public/GSPickup.h`, `Private/GSPickup.cpp` | `ResetForMatch()` |
| `Source/Gloshaugen/Public/GSTerminal.h`, `Private/GSTerminal.cpp` | `CancelPrint()` |
| `IMPROVEMENTS.md` | ny seksjon øverst, 346–359 |

Ikke rør: `GSCharacter.*`, `GSHUD.*`, `GSPlayerController.*`, `GSEffects.*`, `GSTypes.h`, `GSProjectile.*`
(de kan *leses* og `#include`-s). `catalog.py` endres ikke, så `gen_cpp.py` skal ikke kjøres.

## Steg

### 1. `GSHelp.h` (ny, ingen UCLASS, ingen `.generated.h`)
```cpp
#pragma once
#include "CoreMinimal.h"

class AHUD;
class UCanvas;

/** One line in the control overview. */
struct FGSControlRow { const TCHAR* Group; const TCHAR* Key; const TCHAR* What; };

/** Shows the player which keys do what and how the game is played. Sober and small, never arcade. */
namespace GSHelp
{
	/** Every control, grouped. The one list: the H overview, the hints and the main menu all read it. */
	GLOSHAUGEN_API const TArray<FGSControlRow>& Controls();
	/** Draws the control table with its top-left at (X, Y), W wide. Returns the height used. */
	GLOSHAUGEN_API float DrawControls(UCanvas* Canvas, float X, float Y, float W);
	/** Call once per frame for the local player's HUD: H key, overview, lobby intro and hints. */
	GLOSHAUGEN_API void Draw(AHUD* HUD, UCanvas* Canvas);
	/** Draws through AHUD::OnHUDPostRender for as long as no HUD calls Draw itself. Safe to call many times. */
	GLOSHAUGEN_API void EnsureHooked();
}
```

### 2. `GSHelp.cpp` (ny)
Inkluder: `GSHelp.h`, `GSCharacter.h`, `GSPlayerController.h`, `GSPlayerState.h`, `GSGameState.h`, `GSPickup.h`,
`GSTerminal.h`, `GSItems.h`, `GameFramework/HUD.h`, `Engine/Canvas.h`, `Engine/Engine.h`, `CanvasItem.h`,
`Misc/ConfigCacheIni.h`, `InputCoreTypes.h`.

**a) Tegnehjelpere** (anonymt namespace), samme uttrykk som `AGSHUD::Text`/`Box` (les `GSHUD.cpp` linje 18–45):
`Txt(Canvas, S, X, Y, Color, Scale, bCenter)` med `GEngine->GetMediumFont()` og svart skygge, og
`Panel(Canvas, X, Y, W, H, Color)` med `FCanvasTileItem` + `SE_BLEND_Translucent`. Alle farger nøkterne:
panel `(0.02, 0.025, 0.03, 0.82)`, tekst hvit/lys grå, tastenavn dempet gul `(0.95, 0.8, 0.45)`. Ingen blink,
ingen store bokstaver utover overskrifter.

**b) `Controls()`** – statisk liste. Tastene skal stemme med `AGSPlayerController::SetupInputComponent`
(linje 70–111) og README. Kontroller mot koden, ikke gjett:

| Gruppe | Tast | Hva |
|---|---|---|
| Bevegelse | W A S D | Gå |
| | Shift | Løp (bruker utholdenhet). I kikkertsikte: hold pusten |
| | Mellomrom | Hopp, eller klatre opp på kanter og murer |
| | Ctrl / C | Huk. Mens du løper: gli |
| Våpen | Venstre mus | Skyt |
| | Høyre mus | Sikt |
| | R | Lad om |
| | 1 2 3 4 / rullehjul | Bytt våpen (du bærer fire) |
| | V | Slag med det du holder |
| | T | Se på våpenet |
| Ting | E | Plukk opp, åpne kasse, bruk PC eller 3D-printer |
| | Q | Neste ting i sekken |
| | F | Bruk tingen (spis, drikk, bandasjer, kast granat, sett på demper) |
| | G | Slipp tingen |
| | I | Sekken |
| Annet | Tab | Poengtavle |
| | Enter | Klar / ikke klar i lobbyen |
| | Esc / F10 | Meny |
| | H | Denne oversikten |
| På en PC | 1 / 2 / 3 | Kameraer / spor spillere / tilbake |
| | Q / F | Forrige / neste kamera |
| | E | Logg ut |
| På en 3D-printer | Q / 1 / 2 | Velg hva du vil lage |
| | F / venstre mus | Start (koster filament) |

(Sjekk PC- og printer-tastene mot `AGSPlayerController::TerminalInput` før de skrives inn.)
Norske tegn skrives som `å` osv. i `TEXT("...")`, slik resten av koden gjør.

**c) Tilstand per HUD.** `static TMap<TWeakObjectPtr<const AHUD>, FState>` (flere PIE-vinduer deler statiske
variabler, derfor per HUD). `FState`: `bool bOpen`, `uint64 LastDirectFrame`, `FName HintId`, `float HintStart`,
`float NextHintAt`, `float LastHealth`, `bool bWasDead`. Rydd ugyldige nøkler når kartet har over 8 innslag.

**d) `Draw(HUD, Canvas)`**
1. Returner hvis `!HUD || !Canvas`. `APlayerController* PC = HUD->GetOwningPlayerController()`; returner hvis
   null eller ikke `IsLocalController()`. Sett `State.LastDirectFrame = GFrameCounter` bare når kallet *ikke*
   kommer fra reserve-kroken (ha en intern `DrawImpl(HUD, Canvas, bFromHook)`).
2. `AGSPlayerController* GPC = Cast<AGSPlayerController>(PC)`. `bBusy = GPC && (GPC->bMenuOpen ||
   GPC->IsOnTerminal() || GPC->bShowInventory || GPC->bShowScoreboard)`.
3. Tast: `if (PC->WasInputKeyJustPressed(EKeys::H) && !(GPC && (GPC->bMenuOpen || GPC->IsOnTerminal())))
   State.bOpen = !State.bOpen;`. Lukk også når `GPC->bMenuOpen` blir sann.
   (Ikke F1: i utviklingsbygg er F1 bundet til «viewmode wireframe» i motorens `BaseInput.ini`. H er ledig.)
4. **Oversikten åpen:** panel midt på skjermen, bredde `Min(760, SizeX - 80)`, to kolonner (del lista omtrent på
   midten ved gruppeskift), overskrift «Taster», nederst «H lukker». Bruk `DrawControls`. Returner etterpå (ingen
   hint oppå).
5. **Lobby-innføring** når `GS->Phase` er `Lobby` eller `Countdown`, og `!bBusy`: panel i venstre kant
   (`X = 28`, `Y = SizeY * 0.30`, bredde 380), overskrift «Slik spiller du», linjer (skala 0.8, lys grå):
   - «Først til %d drap, eller flest drap når %d minutter er gått.» (`AGSGameState::KillLimit`, `MatchMinutes`)
   - «Du starter med en pistol. Bedre våpen, vester, mat og medisin ligger i byggene.»
   - «Spis og drikk, ellers taper du helse. Dør du, ligger tingene dine igjen.»
   - «Låste kasser har det beste. Du trenger adgangskort eller dirkesett.»
   - «Nå er det oppvarming uten skade. Trykk Enter når du er klar.»
   - «H viser alle taster.» (tastenavnet i dempet gul)
   - Hvis `Phase == Lobby && GS->PhaseEndsAt > 0`: «Starter av seg selv om %d s» (`PhaseSecondsLeft()`).
   Lobby-lista til `AGSHUD::DrawMatch` står øverst i midten (420 bred), så venstre kant er fri. Les `DrawBars`,
   `DrawMinimap`, `DrawKillFeed` i `GSHUD.cpp` og flytt panelet hvis det overlapper noe.
6. **Hint** (bare under `Playing`, eller i lobby for 1–5 under; aldri når `bBusy` eller oversikten er åpen):
   `AGSCharacter* C = Cast<AGSCharacter>(PC->GetPawn())`. Velg første aktuelle hint som ikke er brukt opp:

   | Id | Når | Tekst |
   |---|---|---|
   | `Weapon` | `C->GetFocusPickup()` er et våpen og ikke låst | «E tar våpenet. Du bærer fire. 1–4 eller rullehjulet bytter.» |
   | `Item` | fokus-pickup er noe annet, ikke låst | «E legger tingen i sekken. Q blar, F bruker, G slipper.» |
   | `Crate` | fokus-pickup `bLocked` | «Låst. Adgangskort åpner med en gang, dirkesett tar noen sekunder.» |
   | `Computer` | `C->GetFocusTerminal()` er PC | «På PC-en ser du gjennom overvåkningskameraene og kan spore andre spillere.» |
   | `Printer` | fokus-terminal er printer | «Printeren lager utstyr av filament-spoler. Spolene finner du som loot.» |
   | `Empty` | `GetCurrentWeapon()` bruker ammo, `Ammo == 0`, `Reserve > 0` | «Tomt magasin. R lader om.» |
   | `Hurt` | `C->Health < 60` og ikke død | «Du er skadet. Velg medisin med Q og bruk den med F. Det tar tid, så finn dekning først.» |
   | `Hungry` | `C->Hunger < 35` | «Du er sulten. Finn mat, velg den med Q og spis med F.» |
   | `Thirsty` | `C->Thirst < 35` | «Du er tørst. Finn noe å drikke, velg det med Q og drikk med F.» |
   | `Died` | `C == nullptr` eller `C->bDead`, under `Playing` | «Tingene dine ligger igjen der du døde. Du kommer tilbake et annet sted på campus.» |

   Vis ett hint om gangen i 7 s (ton inn/ut 0,4 s), deretter minst 5 s pause. Plassering: én eller to linjer,
   midtstilt, `Y = SizeY * 0.5 + 240` (under den gamle 30 s-teksten som slutter ved +230; juster hvis ammo/quickbar
   ligger der), skala 0.85, hvit med alfa 0.9, lite mørkt panel bak. Ingen ikon, ingen lyd.
   **Telling:** `GConfig->GetInt/SetInt(TEXT("GloshaugenHelp"), *Id.ToString(), N, GGameUserSettingsIni)` +
   `Flush`. Tell opp når hintet *begynner* å vises. Vises ikke når N ≥ 2. Les tallene én gang (statisk `TMap`),
   ikke hver frame.
   Bruk bare offentlige ting i `AGSCharacter` (`Health`, `Hunger`, `Thirst`, `bDead`, `Weapons`,
   `GetCurrentWeapon()`, `GetFocusPickup()`, `GetFocusTerminal()`). Trengs noe som er privat: dropp det hintet og
   skriv det i rapporten, ikke endre `GSCharacter.h`.

**e) `EnsureHooked()`** – reserve til Kamp har lagt inn kallet i `GSHUD`:
```cpp
static bool bHooked = false;
if (bHooked) return;
bHooked = true;
AHUD::OnHUDPostRender.AddStatic(&OnHUDPostRender);   // void OnHUDPostRender(AHUD* HUD, UCanvas* Canvas)
```
`OnHUDPostRender`: returner hvis `!HUD || !Canvas` eller hvis `State.LastDirectFrame + 2 >= GFrameCounter` (da
tegner HUD-en selv). Ellers `DrawImpl(HUD, Canvas, true)`. Motoren sender `DebugCanvas` her
(`Engine/Private/HUD.cpp` linje 240); bruk `Canvas->SizeX/SizeY` fra den du får, ikke fra HUD-en.

### 3. `GSGameState`
- `.h`: `static constexpr float LobbyAutoStartSeconds = 90.f;` ved de andre konstantene.
- `.cpp` `BeginPlay`: først i funksjonen (også på klienter): `if (GetNetMode() != NM_DedicatedServer)
  GSHelp::EnsureHooked();` + `#include "GSHelp.h"`.

### 4. `GSPickup`: `void ResetForMatch();` (public)
```cpp
void AGSPickup::ResetForMatch()
{
	GetWorldTimerManager().ClearTimer(RespawnTimer);
	Roll();          // new loot, crates lock again
	ForceNetUpdate();
}
```

### 5. `GSTerminal`: `void CancelPrint();` (public, server)
`ClearTimer(PrintTimer); PrintingId = NAME_None; PrintStart = PrintEnd = 0.f; ForceNetUpdate();`

### 6. `GSGameMode`
`.h`: `void StartCountdown(bool bForced);` (erstatter den uten parameter), `bool bCountdownForced = false;`,
`int32 LobbyPlayersLast = 0;`, `void CleanWorldForMatch();`.

**`SetReady`:** avbryt bare frivillig nedtelling:
`if (!bReady && GS->Phase == EGSMatchPhase::Countdown && !bCountdownForced) GS->Phase = EGSMatchPhase::Lobby;`

**`Tick`, `Lobby`-grenen** (etter tellingen av `Total`/`Ready`):
```cpp
if (Total >= 2 && LobbyPlayersLast < 2) LobbyStartedAt = Now;   // the 90 s start when player two arrives
LobbyPlayersLast = Total;
GS->PhaseEndsAt = Total >= 2 ? LobbyStartedAt + AGSGameState::LobbyAutoStartSeconds : 0.f;  // shown in the lobby
if (Total > 0 && Ready == Total) StartCountdown(false);
else if (Total >= 2 && Now - LobbyStartedAt > AGSGameState::LobbyAutoStartSeconds) StartCountdown(true);
```
`StartCountdown(bool bForced)` setter `bCountdownForced = bForced` i tillegg til det den gjør i dag.
(`AGSHUD::DrawMatch` leser ikke `PhaseEndsAt` i lobby-fasen, så å sette den der er trygt.)

**`CleanWorldForMatch()`** (ny, `#include "GSProjectile.h"`). Samle først i `TArray`, slett etterpå:
- alle `AGSProjectile` → `Destroy()` (miner, C4, ild, røyk, granater i lufta);
- alle `AGSPickup`: `bRespawns == false` → `Destroy()`, ellers `ResetForMatch()`;
- alle `AGSTerminal` → `CancelPrint()`;
- alle `AGSCharacter` som er igjen etter at spillernes kropper er slettet (lik fra forrige kamp) → `Destroy()`.

**`StartMatch`:** ny rekkefølge:
1. fase, klokke, `Winner.Empty()`, `KillFeed.Reset()`, nullstill poeng (som i dag);
2. samle kontrollerne; for hver: `UnPossess()` + `Destroy()` på gammel kropp (**alle først**, ingen `RestartPlayer` ennå);
3. `CleanWorldForMatch()`;
4. stokk kontroller-lista (enkel Fisher–Yates med `FMath::RandRange`), og kall `RestartPlayer(C)` for hver.
   Da teller hver ny kropp som «fiende» for neste, og `PickSpawnTransform` sprer spillerne av seg selv.

**`EndMatch`:**
```cpp
int32 BestKills = 0;
for (...) if (PS) BestKills = FMath::Max(BestKills, PS->Kills);
TArray<const AGSPlayerState*> Top;   // most kills, then fewest deaths
if (BestKills > 0) { /* find min Deaths among Kills == BestKills, collect all with that Kills and Deaths */ }
if (Top.Num() == 1) GS->Winner = Top[0]->GetPlayerName();
else if (Top.Num() == 2 || Top.Num() == 3) GS->Winner = names joined with TEXT(" og ");   // shared win
else GS->Winner.Empty();   // nobody scored, or too many tied: "Ingen vinner" (HUD line 801 handles empty)
```

### 7. `IMPROVEMENTS.md`
Ny seksjon **øverst** (over «Feilrettinger etter feiljakt»), med linjene under. Ikke rør andres linjer.

### 8. Bygg
Sjekk `Get-Process UnrealEditor` (skal ikke kjøre). Bygg med kommandoen i `CLAUDE.md` (**med** `-WaitMutex`).
Feiler bygget i `GSCharacter.*`/`GSEffects.*`/`GSTypes.h`: det er lag Kamp midt i en endring – vent og prøv
igjen (inntil 4 ganger), ikke rett filene.

## Ferdig når
- Bygget går gjennom uten feil eller nye advarsler i lagets filer.
- `GSHelp.h/.cpp` finnes, og ingen fil utenfor lista i «Filer» er endret (`git status`).
- Hver tast i `Controls()` er sjekket mot `SetupInputComponent` og `TerminalInput` (byggeren noterer det i rapporten).
- Lest i koden: `StartMatch` sletter alle prosjektiler og slupne pickups, ruller alle faste pickups, og ingen
  spiller får ny kropp før ryddingen er gjort.
- Lest i koden: «ikke klar» under tvungen nedtelling endrer ikke fasen.
- `EndMatch` gir tom `Winner` når beste spiller har 0 drap.
- Det Andreas skal se (Play, én spiller): panelet «Slik spiller du» til venstre i lobbyen; **H** åpner og lukker
  tasteoversikten; se på en ting på bakken → ett kort hint, som ikke kommer tredje gang. Med to spillere:
  «Starter av seg selv om N s» teller ned; legg en mine i lobbyen → borte når kampen starter.

## Risiko
- **Tegning via `OnHUDPostRender`/`DebugCanvas`** (reserven): debug-lerretet kan ha annen størrelse/DPI enn
  HUD-lerretet, så teksten kan bli litt større/mindre til Kamp har levert det direkte kallet. Derfor brukes bare
  mål fra lerretet vi får. Hvis `DebugCanvas` er null tegnes ingenting (ingen krasj).
- **Statisk delegat** overlever mellom Play-økter i editoren: registreres bare én gang, og holder ingen pekere.
- **Dobbelt hjelp** de første 30 s til Kamp fjerner `AGSHUD::DrawHelp` (gammel tekst midt på + vårt panel til
  venstre). Stygt, men ufarlig; bestilling er sendt.
- `WasInputKeyJustPressed` lest fra tegnekoden: virker for lokal spiller; tasten H er ikke brukt av andre
  handlinger. Skriver man «h» i konsollen (§) går tasten til konsollen, ikke hit.
- **Rydding ved kampstart:** å slette et molotov-prosjektil stopper skaden, men flammene hos klientene brenner ut
  av seg selv i opptil 8 s. Klienter som står langt unna en slettet pickup får slettingen når den blir relevant
  (vanlig motor-oppførsel).
- Kamps runde 3 lar lik ligge i 90 s; vi sletter dem ved kampstart. Blodflekker eies av Kamp og ryddes ikke her
  (nevn for dirigenten hvis de bør bort også).
- **Replikering:** ingen nye replikerte felt. `PhaseEndsAt` settes hvert server-tick i lobbyen til samme verdi, så
  det gir ingen ekstra trafikk.

## IMPROVEMENTS-linjer
```
## Runde 1 (Spillbarhet): spillet forklarer seg selv, og kampen starter rent (26. nov.)

346. H åpner en oversikt over alle taster, delt i bevegelse, våpen, ting, annet, PC og 3D-printer. H lukker den igjen
347. Lobbyen har en kort innføring til venstre: hva kampen går ut på, hva du starter med, og at Enter betyr klar
348. Lobbyen viser hvor mange sekunder det er til kampen starter av seg selv
349. Hint første gang du ser på et våpen på bakken: E tar det, du bærer fire, 1–4 bytter
350. Hint første gang du ser på en annen ting: E legger den i sekken, Q blar, F bruker, G slipper
351. Hint første gang du står ved en låst kasse, en PC og en 3D-printer
352. Hint første gang du er skadet, sulten eller tørst, og første gang magasinet er tomt
353. Hint første gang du dør: tingene ligger igjen, og du kommer tilbake et annet sted
354. Hvert hint vises høyst to ganger og huskes til neste gang du spiller. Ingen hint mens meny, sekk eller PC er åpen
355. Tasteoversikten og hintene henter tastene fra én felles liste, så de alltid stemmer med hverandre
356. Kampstart rydder campus: miner, C4, granater, ild og røyk fra oppvarmingen er borte
357. Ved kampstart fjernes ting som er sluppet på bakken og lik fra forrige kamp, og alle lootplasser og låste kasser fylles på nytt
358. Nedtelling som starter fordi tiden er ute kan ikke stoppes av én spiller, og fristen på 90 sekunder starter først når spiller nummer to er inne
359. Ingen kåres til vinner uten drap. Står to eller tre helt likt, deler de seieren
```
