# Plan – Spillbarhet runde 2: hovedmenyen (0b), lagvalg med mus og taster som kan endres

Bygger på koden slik den blir etter `ko/plan-01.md` (CTF-grunnmuren: `GSHelp.*`, `GSFlag.*`, `Team`,
`ServerRequestTeam`, `RequestTeam`, `AGSGameMode::MaxPerTeam`). **Start først når plan-01 er ferdig bygget.**
Plan-02 rører bare tre funksjoner fra plan-01: `GSHelp.cpp` `DrawImpl` (L-tasten flyttes, to tekstlinjer),
`AGSGameState::BeginPlay` (én linje) og `AGSGameMode` (ny overstyring `ChangeName`, ingen eksisterende funksjon endres).

Alt nytt ligger i **nye `GSMenu.h`/`GSMenu.cpp`**, C++ uten .uasset, tegnet på lerretet som `GSHelp` (samme krok
`AHUD::OnHUDPostRender`). Mus og tastatur tas imot gjennom motorens egne kroker i `UGameViewportClient`
(`OnOverrideInputKey`/`OnOverrideInputAxis`, `InputChar`), ikke gjennom Kamps filer. Menyen virker uten noe fra Kamp;
én liten bestilling gjør Esc-menyen deres pen (se steg 10).

## Mål
Når spillet starter kommer du til en rolig hovedmeny over campus: skriv navnet ditt, start et spill eller skriv inn
vennens adresse, still inn grafikk, mus, lyd og taster, og velg lag med musa i lobbyen – uten konsoll, også i et ferdig
pakket spill, med bekreftelse før du forlater en kamp og en forståelig melding når tilkoblingen feiler.

## Forbedrer (gammelt)
- **Bli med** (Kamps Esc-meny brukte bare forrige adresse; ny adresse krevde konsollen, som ikke finnes i et pakket
  spill – funn F1.1): nå et ekte tekstfelt med liming, siste adresse huskes.
- **Start server / Avslutt** i Esc-menyen kastet ut alle eller lukket spillet på ett klikk (F1.2): nå bekreftelse.
- **Innstillingene** (grafikk, oppløsning, strålesporing, følsomhet i 0,1-steg): samlet på én side, følsomhet med
  glidebryter, strålesporing huskes (ble glemt ved omstart), nytt: skjermmodus, V-sync, bildefrekvens.
- **Lagbytte** (plan-01: L bytter blindt): nå et panel med begge lagene, navn, klar-merker og nedtelling.
- **Spillernavn** (F2: alle het motorens standardnavn): eget navn, renset og gjort unikt på serveren.
- **Menytaster går videre til spillet** (funn I3, for våre menyer): mens en GSMenu-flate er åpen når ingen
  taster/klikk spillet.

## Nytt
- Hovedmeny (Start spill, Bli med, Øv alene, Innstillinger, Kontroller, Avslutt) med kamera som sakte panorerer.
- «Kobler til …» med sekunder og Avbryt; gis opp etter 20 s. Feilmelding ved mislykket/brutt tilkobling (F1.3, F6).
- Start spill viser adressen(e) vennene skal skrive inn.
- Lydstyrke (hovedvolum), lagret.
- **Taster som kan endres** (keybinds), lagret, og en Kontroller-oversikt som viser tastene slik de faktisk er.
- Lagvalg-panel med mus i lobbyen.

## Hvorfor nå
0b i prioriteringene, rett etter CTF-grunnmuren. Uten den kan venner ikke finne hverandre i et ferdig spill (F1 er
Høy). Lagvalget trenger feltene fra plan-01, som nå finnes. Knappehjelpen (0a, plan-03) skal lese tastene fra denne
rundens tasteliste, så tastene må komme først.

**Valg om taster (avvik fra prioriteringsteksten, begrunnet):** prioriteringene foreslår Enhanced Input sine
brukerinnstillinger (`UEnhancedInputUserSettings`). Det krever `PlayerMappableKeySettings` på hver handling i Kamps
`EnsureInputCreated`, `bEnableUserSettings` i Config og et SaveGame-spor (funn I2) – altså en stor bestilling. Kamps
`UInputMappingContext` lages i kode og er `public`, så vi kan sette `Key` på hver mapping direkte
(`GetMapping(i).Key`) og be subsystemet bygge på nytt (`RequestRebuildControlMappings`). Det er fortsatt Enhanced
Input, spilleren merker ingen forskjell, nye handlinger Kamp legger til dukker opp av seg selv, og lagringen
(`[Gloshaugen.Keys]` i `GameUserSettings.ini`) kan flyttes til brukerinnstillingene senere uten å endre menyen.
Sjekket i motoren: `GetMapping()` gir `FEnhancedActionKeyMapping&` med `public` `FKey Key`, og ombyggingen leser
`GetMappingsForProfile("")` = `DefaultKeyMappings` (`InputMappingContext.cpp` linje 113–121).

## Filer
| Fil | Endring |
|---|---|
| `Source/Gloshaugen/Public/GSMenu.h`, `Private/GSMenu.cpp` (NYE) | hele menyen, lagvalg, dialoger, taster, `UGSGameViewportClient` |
| `Private/GSHelp.cpp` | skjul seg bak hovedmenyen; L-tasten flyttes til GSMenu; to linjer leser tastenavn |
| `Private/GSGameState.cpp` | `GSMenu::EnsureHooked()` i `BeginPlay` |
| `Public/GSGameMode.h`, `Private/GSGameMode.cpp` | `ChangeName`-overstyring (rens og unike navn) |
| `Config/DefaultEngine.ini` (felles, én ny seksjon nederst) | `GameViewportClientClassName` |
| `Source/Gloshaugen/Gloshaugen.Build.cs` (felles, én ny linje) | `PrivateDependencyModuleNames`: `Sockets`, `ApplicationCore` |
| `IMPROVEMENTS.md` | ny seksjon øverst, 402–415 |

Ikke rør `GSHUD.*`, `GSPlayerController.*`, `GSCharacter.*` (Kamp). Fra `AGSPlayerController` brukes bare det som
allerede er `public`: `MappingContext`, `MouseSensitivity`, `LastJoinAddress`, `bMenuOpen`, `CloseMenu()`,
`ToggleReady()`, `Host()`, `Join()`, `Straaling()`, `GetGraphicsLevel()`, `GetResolutionPercent()`, `IsRayTracingOn()`,
`IsOnTerminal()`. Skriv `GSMenu.h` ferdig først og i ett (GSHelp og GSGameState inkluderer den).

## Steg

### 1. `GSMenu.h`
```cpp
#pragma once
#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "GSMenu.generated.h"

class AHUD;
class UCanvas;
class APlayerController;
class AGSPlayerController;

/** Gives the menu's text fields real characters (æ, ø, å, capitals). Set in DefaultEngine.ini. Without it the
 *  fields still take digits, '.', ':' and A–Z from the keys. */
UCLASS()
class GLOSHAUGEN_API UGSGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()
public:
	virtual bool InputChar(FViewport* InViewport, int32 ControllerId, TCHAR Character) override;
};

/** One line of the controls overview (also for the H overview in GSHelp later). */
struct FGSControlRow { FString Label; FString Key; bool bRebindable = false; };

namespace GSMenu
{
	/** Post-render hook + network/travel failure events. Once per process; safe to call many times. */
	GLOSHAUGEN_API void EnsureHooked();
	/** Opens the main menu (also from a match). */
	GLOSHAUGEN_API void OpenMain(AGSPlayerController* PC);
	/** "Avslutte spillet?" dialog. */
	GLOSHAUGEN_API void AskQuit(AGSPlayerController* PC);
	/** The main menu covers the screen (GSHelp and the HUD stay hidden). */
	GLOSHAUGEN_API bool IsFullscreen(const APlayerController* PC);
	/** A GSMenu panel or dialog has the mouse and keyboard. */
	GLOSHAUGEN_API bool IsModal(const APlayerController* PC);
	/** The key an action is on right now, Norwegian name ("E", "Mellomrom"). Action = "IA_Interact" or own "GS_Team". */
	GLOSHAUGEN_API FString KeyName(const APlayerController* PC, FName Action);
	/** Every control with its current key, in the order of the controls page. */
	GLOSHAUGEN_API TArray<FGSControlRow> Controls(const APlayerController* PC);
	/** Called by UGSGameViewportClient::InputChar; true = the menu took the character. */
	bool HandleChar(UGameViewportClient* VC, TCHAR Ch);
}
```

### 2. `GSMenu.cpp` – tilstand, tegnehjelpere, kroker
**Tilstand per vindu** (PIE med flere spillere i én prosess har flere vinduer i samme prosess, så ikke én global):
```cpp
namespace
{
	enum class EPage : uint8 { Root, Host, Join, Connecting, Settings, Controls };
	enum class EDialog : uint8 { None, Host, Join, Leave, Quit, Message };
	struct FHit { FBox2D R; FName Id; int32 Arg = 0; };
	struct FState
	{
		// --- per world (reset when World changes) ---
		TWeakObjectPtr<UWorld> World;
		bool bMainOpen = false, bMainDismissed = false, bTeamOpen = false, bTeamShownThisLobby = false;
		EGSMatchPhase LastPhase = EGSMatchPhase::Lobby;
		EPage Page = EPage::Root;
		EDialog Dialog = EDialog::None;
		FName Focus;                       // "Name" or "Address": the text field with the caret
		FString NameEdit, AddressEdit, PendingJoin;
		int32 ListenRow = INDEX_NONE;      // controls page: waiting for a new key
		FString KeyNote;                   // "J var brukt av …, de har byttet"
		FName DragSlider;                  // "Sens" / "Volume" while the left button is held
		bool bWorldSetupDone = false, bNameSent = false;
		TArray<FHit> Hits;                 // rebuilt every frame, read by the next click
		// --- per window (kept across map changes) ---
		FString MsgTitle, MsgText, MsgDetail; double MsgTime = 0.0;
		FString ConnectAddress; double ConnectStart = 0.0; bool bConnecting = false;
		double LeavingUntil = 0.0;         // we left on purpose: no "connection lost" message
		bool bShift = false, bCtrl = false;
	};
	TMap<TWeakObjectPtr<UGameViewportClient>, FState>& States();   // static map, prune invalid keys when > 8
	FState* StateFor(const UWorld* W);   // W->GetGameViewport(); null on a dedicated server
}
```
- **Tegnehjelpere:** kopier `Text`/`Box`/`TextWidth`/`LineHeight` fra `GSHelp.cpp` (anonymt namespace, samme utseende:
  `GEngine->GetMediumFont()`, svart skygge, `SE_BLEND_Translucent`). Farger: panel `(0.015, 0.018, 0.022, 0.86)`,
  tekst `(0.82, 0.83, 0.84)`, dempet `(0.55, 0.57, 0.60)`, aksent (dempet rav) `(0.85, 0.65, 0.25)`, sperret `(0.35, 0.36, 0.38)`.
  Ingen blink, ingen gul fylling, ingen lyd. Nye hjelpere som også legger en `FHit` i `S.Hits`:
  - `Button(S, Canvas, Id, Label, X, Y, W, H, Mouse, bEnabled = true, bSelected = false, Arg = 0)`: bakgrunn
    `(1,1,1,0.04)`, ved hover `(1,1,1,0.10)`; valgt/hover = 3 px aksentstrek til venstre; tekst venstrejustert X+16,
    skala 1.0; sperret = dempet tekst og ingen `FHit`.
  - `Field(S, Canvas, Id, Value, Placeholder, X, Y, W, H, Mouse)`: mørk boks `(0,0,0,0.45)` med 1 px ramme
    `(1,1,1,0.18)` (aksent når `S.Focus == Id`), tekst skala 0.95, plassholder dempet, markør «|» etter teksten når
    fokusert og `FMath::Fmod(FPlatformTime::Seconds(), 1.0) < 0.55`. Er teksten bredere enn feltet: tegn bare slutten.
  - `Slider(S, Canvas, Id, Value01, X, Y, W, Mouse)`: linje 4 px `(1,1,1,0.2)`, fylt del aksent, knott 10×18.
    `FHit` dekker hele raden (H 28).
  - `Choice(S, Canvas, Id, Label, ValueText, X, Y, W, Mouse)`: «Label ……… ◀ Verdi ▶» der ◀/▶ er knapper med Arg −1/+1.
    Bruk tegnene `<` og `>` hvis `◀`/`▶` mangler i fonten (sjekk med `Font->GetCharSize`/`GetStringSize` > 0).
- **`EnsureHooked`** (static bool): `AHUD::OnHUDPostRender.AddStatic(&OnPostRender)`;
  `GEngine->OnNetworkFailure().AddStatic(&OnNetworkFailure)`; `GEngine->OnTravelFailure().AddStatic(&OnTravelFailure)`.
- **Kroker per vindu** (i `OnPostRender`, første gang en `UGameViewportClient* VC = World->GetGameViewport()` sees):
  `VC->OnOverrideInputKey().BindStatic(&OnKey, TWeakObjectPtr<UGameViewportClient>(VC));`
  `VC->OnOverrideInputAxis().BindStatic(&OnAxis, TWeakObjectPtr<UGameViewportClient>(VC));`
  (sjekk `IsBound()` først; begge er enkeltdelegater som kalles før `PlayerController->InputKey`, se
  `GameViewportClient.cpp` linje 788–800 og 871–874). `OnAxis` returnerer `IsModal(...)` (svelger mus/hjul når modal).
- **`OnPostRender(AHUD* HUD, UCanvas* Canvas)`:** `PC = Cast<AGSPlayerController>(HUD->GetOwningPlayerController())`,
  bare `IsLocalController()`, `Canvas` ikke null. `FState& S`. Er `S.World != World`: nullstill per-verden-feltene
  (behold per-vindu-feltene), `S.World = World`. Så, i rekkefølge:
  1. **Oppsett én gang per verden** (`bWorldSetupDone`): `ApplyVolume(World)`; strålesporing: hvis
     `GConfig` har `[Gloshaugen] RayTracing` og den avviker fra `PC->IsRayTracingOn()` → `PC->Straaling(Saved)`;
     les navn (`EnsureName()`, se steg 6).
  2. **Taster** (`ApplySavedKeys(PC)`, steg 7): forsøk hver frame til `PC->MappingContext` finnes, så én gang per kontekst.
  3. **Navn til serveren** (`!bNameSent && PC->PlayerState`): `if (PS->GetPlayerName() != Saved) PC->ServerChangeName(Saved);` `bNameSent = true`.
  4. **Automatisk åpning:** `World->GetNetMode() == NM_Standalone && !bMainDismissed && !bMainOpen` → `OpenMainImpl(PC, S)`.
     Lagvalg: se steg 5.
  5. **Tilkobling:** se steg 4.
  6. `S.Hits.Reset()`; mus = `PC->GetMousePosition(MX, MY)`. Tegn: hovedmeny (fullskjerm) **eller** lagvalg, så
     dialog øverst. Mens modal: hvis `!PC->bShowMouseCursor` (f.eks. etter respawn) → sett markør og inndatamodus igjen.
  7. Glidebryter som dras: `if (S.DragSlider != NAME_None)` → ny verdi fra `(MX − X) / W`, bruk med en gang (steg 6).
- **Modal av/på** (`SetModal(PC, S, bOn)`): på → `if (PC->bMenuOpen) PC->CloseMenu();` `PC->bShowMouseCursor = true;`
  `FInputModeGameAndUI M; M.SetHideCursorDuringCapture(false); PC->SetInputMode(M);` `PC->FlushPressedKeys();`
  av → `bShowMouseCursor = false; SetInputMode(FInputModeGameOnly()); FlushPressedKeys();`
  `if (PC->MyHUD) PC->MyHUD->bShowHUD = true;` `S.Focus = NAME_None; S.ListenRow = INDEX_NONE; S.DragSlider = NAME_None;`
  (`FlushPressedKeys` gjør at en W som holdes når panelet åpnes ikke blir «hengende» når vi svelger slippet.)
- **`IsModal`** = `bMainOpen || bTeamOpen || Dialog != None`. **`IsFullscreen`** = `bMainOpen`.
  Når hovedmenyen åpnes: `PC->MyHUD->bShowHUD = false` (Kamps `DrawHUD` hoppes over; `OnHUDPostRender` sendes likevel,
  `HUD.cpp` linje 175–239), når den lukkes: `true`.

### 3. Innputt: `OnKey(FInputKeyEventArgs& Args, TWeakObjectPtr<UGameViewportClient> WeakVC)`
1. Oppdater `S.bShift`/`S.bCtrl` fra `LeftShift/RightShift/LeftControl/RightControl` (Pressed/Released), alltid.
2. `PC` = `Cast<AGSPlayerController>(VC->GetWorld()->GetFirstPlayerController())`; ingen → `return false`.
3. **Kamps Esc-meny (reserve til bestillingen er levert):** `PC->bMenuOpen && !IsModal && Key == LeftMouseButton &&
   Event == IE_Pressed` → `FIntPoint P; Args.Viewport->GetMousePos(P);`
   `const FHUDHitBox* Hit = PC->GetHUD() ? PC->GetHUD()->GetHitBoxAtCoordinates(FVector2D(P), true) : nullptr;`
   navn `Host` eller `Join` → `PC->CloseMenu(); OpenMain(PC); return true;` navn `Quit` → `PC->CloseMenu(); AskQuit(PC); return true;`
   (Kamps `MenuClick` kjører da aldri for disse tre: ingen kamp forlates uten bekreftelse.)
4. **Ikke modal:** `Key == TeamKey()` (steg 7, standard L), `IE_Pressed`, fase `Lobby`/`Countdown`, `!PC->bMenuOpen`,
   `!PC->IsOnTerminal()` → åpne lagvalg, `return true`. Ellers `return false` (alt går til spillet som før).
5. **Modal** (svelg alt, `return true` til slutt), bare `IE_Pressed` (og `IE_Repeat` for Backspace):
   - `S.ListenRow != INDEX_NONE` → `Escape` avbryter, ellers `AssignKey(PC, S, Row, Key)` (steg 7). Ferdig.
   - `LeftMouseButton` → `Click(PC, S, Pos)`: siste `FHit` (øverst) som inneholder `Pos` (`Args.Viewport->GetMousePos`);
     klikk utenfor et felt fjerner fokus. Slipp av venstre knapp (`IE_Released`) → avslutt dra og lagre (steg 6).
   - `Escape` → `Back()`: dialog → lukk; fokus → fjern; underside → `Root`; lagvalg → lukk; hovedmeny i kamp → lukk;
     hovedmeny alene (standalone) → ingenting.
   - `Enter`/`NumPadEnter` → dialog: «Ja»; fokus `Address` → koble til; fokus `Name` → lagre navnet; lagvalg → `PC->ToggleReady()`.
   - `BackSpace` → slett siste tegn i feltet med fokus. `V` med `bCtrl` → lim inn:
     `FString Clip; FPlatformApplicationMisc::ClipboardPaste(Clip);` (`HAL/PlatformApplicationMisc.h`), filtrert som tegn.
   - Reserve-tegn bare når `!Cast<UGSGameViewportClient>(VC)`: `Zero..Nine`, `NumPadZero..NumPadNine`, `Period`/`Decimal`
     → `.`, `A..Z` (små uten Shift), `SpaceBar`, `Hyphen`/`Subtract` → `-`.
6. **`HandleChar(VC, Ch)`** (fra `UGSGameViewportClient::InputChar`): ikke modal → `false`. Modal: `Ch < 32 || Ch == 127`
   → `true` (styretegn tas av `OnKey`); fokus `Address`: tillat `0–9 a–z A–Z . : -`, maks 64; fokus `Name`: alle
   skrivbare unntatt `" ? = # \`, maks 20. `return true`.
   `UGSGameViewportClient::InputChar`: `if (GSMenu::HandleChar(this, Character)) return true; return Super::InputChar(...);`

### 4. Hovedmenyen
Åpnes av seg selv i `NM_Standalone` (spillet startet, ikke koblet til noen – også etter en mislykket tilkobling eller
når man har forlatt en kamp, fordi motoren da laster kartet på nytt alene), og fra Esc-menyen i en kamp.
`OpenMainImpl`: `bMainOpen = true; Page = Root; SetModal(on); MyHUD->bShowHUD = false;`
`CloseMain`: `bMainOpen = false; bMainDismissed = true; SetModal(off)`.

**Bakgrunn:** hele skjermen `(0,0,0,0.30)`. I standalone panorerer kameraet sakte:
`R = PC->GetControlRotation(); R.Yaw += 2.f * Dt; R.Pitch = FMath::FInterpTo(FRotator::NormalizeAxis(R.Pitch), -4.f, Dt, 0.5f); PC->SetControlRotation(R);`

**Venstre panel** (X 0, bredde `PW = FMath::Max(460, SX * 0.30)`, hele høyden): tittel «GLØSHAUGEN STRIKE» (X 48,
Y `SY*0.12`, skala 1.9, hvit), under «Capture the Flag · NTNU Gløshaugen» (0.8, dempet). «Navn» (dempet, 0.8) +
`Field("Name")` (bredde `PW − 96`, H 36). Knapper fra Y `SY*0.32`, H 44, mellomrom 6, bredde `PW − 96`:
- **Alene:** Start spill (→ `Host`-side) · Bli med (→ `Join`, fokus på feltet) · Øv alene (→ `CloseMain`) ·
  Innstillinger · Kontroller · Avslutt (→ dialog `Quit`).
- **I kamp** (`NM_ListenServer`/`NM_Client`): Tilbake til kampen · Ny kamp som vert (→ `Host`) · Bli med i en annen
  (→ `Join`) · Innstillinger · Kontroller · Forlat kampen (→ dialog `Leave`) · Avslutt (→ dialog `Quit`).
- Valgt side har aksentstrek. Nederst til venstre (0.75, dempet): «Esc: tilbake».

**Høyre panel** (X `PW + 40`, Y `SY*0.12`, bredde `min(SX − X − 48, 900)`, høyde `SY*0.74`, samme panelfarge),
tittel skala 1.2, tekst 0.85, innrykk 24. Vises for alle sider unntatt `Root` (Root viser bare venstre panel).
- **Host («Start spill»):** «Du blir vert for en ny kamp. Vennene dine kobler seg til deg.» · «Venner på samme nett
  skriver inn:» + hver adresse på egen linje (skala 1.1, hvit) fra `LocalAddresses()` · «Over internett må port 7777
  (UDP) åpnes i ruteren din.» · «Første gang spør Windows om brannmuren. Trykk Tillat.» · i kamp i tillegg (aksent):
  «Kampen du er i nå avsluttes.» Knapp **«Start som vert»** → alene: `S.LeavingUntil = Now + 10; PC->Host();`
  i kamp: dialog `Host`.
  `LocalAddresses()`: `ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalAdapterAddresses(Addrs)`; behold IPv4
  (`GetProtocolType() == FNetworkProtocolTypes::IPv4`), ikke `127.`, ikke `169.254.`; `ToString(false)`; maks 3;
  regn ut én gang per prosess. Ingen → «Fant ingen nettverksadresse.»
- **Join («Bli med»):** «Adresse» + `Field("Address")` (forhåndsutfylt med `PC->LastJoinAddress`, fokus ved åpning)
  + knapp **«Koble til»** (aksent). Er `LastJoinAddress` ulik feltet: knapp «Siste: 192.168.1.20» som fyller feltet.
  Tekst: «Skriv adressen verten ser under Start spill, for eksempel 192.168.1.20. Ctrl+V limer inn.»
  `TryJoin(Addr)`: trim; tom → `KeyNote`-aktig linje «Skriv inn en adresse.»; mellomrom → «Adressen kan ikke ha
  mellomrom.»; alene → `StartJoin(Addr)`; i kamp → `PendingJoin = Addr`, dialog `Join`.
  `StartJoin(Addr)`: `S.bConnecting = true; S.ConnectAddress = Addr; S.ConnectStart = FPlatformTime::Seconds();
  S.Page = EPage::Connecting; PC->Join(Addr);` (Kamps `Join` lagrer adressen og kaller `ClientTravel`).
- **Connecting:** «Kobler til 192.168.1.20 … 7 s» + knapp **«Avbryt»** (og Esc) → `CancelConnect(World, S)`:
  `if (FWorldContext* Ctx = GEngine->GetWorldContextFromWorld(World)) { GEngine->CancelPending(*Ctx); Ctx->TravelURL.Empty(); }`
  `S.bConnecting = false; S.Page = EPage::Join;`
  I `OnPostRender` mens `bConnecting`: verden er `NM_Client` → ferdig (`bConnecting = false`). Over **20 s** →
  `CancelConnect` + melding «Fant ikke noe spill på %s» / «Sjekk adressen, og at verten har trykket Start spill og
  har åpnet for deg i brannmuren.»
- **Settings / Controls:** steg 6 og 7.

**Dialog** (sentrert boks 560 bred, panelfarge + 1 px ramme `(1,1,1,0.18)`, over alt annet, svelger klikk utenfor):
| Dialog | Tittel | Tekst | Ja-knapp → |
|---|---|---|---|
| `Host` | Starte en ny kamp? | vert med andre: «Du er vert. Kampen avsluttes for alle %d andre.» / ellers «Du forlater kampen du er i.» | «Ja, start ny» → `LeavingUntil = Now + 10; PC->Host();` |
| `Join` | Bli med i en annen kamp? | «Du forlater kampen du er i.» (+ vertlinja som over) | «Ja, koble til» → `StartJoin(PendingJoin)` |
| `Leave` | Forlate kampen? | som over | «Ja, forlat» → `LeavingUntil = Now + 10; UGameplayStatics::OpenLevel(PC, FName(*UGameplayStatics::GetCurrentLevelName(PC, true)));` |
| `Quit` | Avslutte spillet? | i kamp: som over; alene: ingen tekst | «Avslutt» → `UKismetSystemLibrary::QuitGame(PC, PC, EQuitPreference::Quit, false);` |
| `Message` | `MsgTitle` | `MsgText` + `MsgDetail` (0.75, dempet) | bare «OK» → lukk |
«Andre» = `GS->PlayerArray.Num() − 1` når `NM_ListenServer`. Andre knapp «Avbryt». Enter = ja, Esc = avbryt.
`Leave` laster kartet alene: verten stenger serveren (gjestene får meldingen under), gjesten kobles fra; begge havner i
hovedmenyen.

**Feilmeldinger** (F1.3, F6). `OnNetworkFailure(UWorld* W, UNetDriver*, ENetworkFailure::Type T, const FString& Err)` og
`OnTravelFailure(UWorld* W, ETravelFailure::Type T, const FString& Err)`: `S = StateFor(W)` (null → `GEngine->GameViewport`).
Hopp over hvis `FPlatformTime::Seconds() < S.LeavingUntil`, eller hvis en melding kom for under 2 s siden. Nettverksfeil
tas bare når `W` er `NM_Client` eller `S.bConnecting`, unntatt `NetDriverListenFailure`. Sett `MsgTitle/MsgText/MsgDetail = Err`,
`MsgTime`, `bConnecting = false`, og vis dialog `Message` (åpne hovedmenyen først hvis den er lukket og verden er standalone;
ellers vises dialogen der man er):
- `ConnectionLost`, `ConnectionTimeout` (koblet til): «Forbindelsen ble brutt» / «Verten kan ha avsluttet spillet,
  eller nettet falt ut.»
- `PendingConnectionFailure`, og reise-feilene `PendingNetGameCreateFailure`, `ClientTravelFailure`, `TravelFailure`:
  «Kunne ikke koble til» / «Fant ikke noe spill på %s.» (`ConnectAddress`, eller «adressen» hvis tom)
- `FailureReceived`: «Verten avviste tilkoblingen» / «Kampen kan være full, eller verten avsluttet.»
- `OutdatedClient`, `OutdatedServer`, `NetChecksumMismatch`, `NetGuidMismatch`: «Ulike versjoner» / «Du og verten har
  ikke samme versjon av spillet.»
- `NetDriverListenFailure`: «Kunne ikke starte som vert» / «Port 7777 er i bruk. Lukk andre kopier av spillet og prøv igjen.»
- `InvalidURL`: «Ugyldig adresse» / «Sjekk at adressen er skrevet riktig.» · `LoadMapFailure`, `PackageMissing`,
  `PackageVersion`, `NoLevel`: «Kartet kunne ikke lastes» / «Verten bruker et kart du ikke har.»
- annet: «Noe gikk galt med forbindelsen» / tom tekst.
Meldingen ligger i per-vindu-tilstanden og overlever at motoren laster standardkartet på nytt; den vises da i den nye
hovedmenyen.

### 5. Lagvalg med mus (oppå plan-01s `ServerRequestTeam`/`RequestTeam`)
- **Åpnes av seg selv:** verden `NM_ListenServer`/`NM_Client`, `GS->Phase == Lobby`, `PS && PS->Team != 0`,
  `!bTeamShownThisLobby`, `!bMainOpen`, `!PC->bMenuOpen`, `!PC->IsOnTerminal()` → `bTeamOpen = true; SetModal(on);`
  `bTeamShownThisLobby = true`. Når fasen går til `Ended`: `bTeamShownThisLobby = false` (neste lobby åpner igjen).
  Når fasen blir `Playing`: lukk panelet (`SetModal(off)`). Ellers med `TeamKey()` (standard L) i lobby/nedtelling.
- **Panel** sentrert, bredde `min(760, SX − 64)`: tittel «Velg lag»; under (0.85, dempet) «Oppvarming – ingen skade.»
  + «Starter av seg selv om %d s.» når `PhaseEndsAt > 0`, eller i `Countdown`: «Kampen starter om %d s.»
- **To kolonner** (rød venstre, blå høyre): topp 34 px i `GSTeam::Color(T)` (alfa 0.85) med «Rødt lag  3/10»;
  under én rad (26 px) per spiller med `Team == T` fra `GS->PlayerArray`: navn, og «klar» (dempet grønn
  `(0.45, 0.7, 0.45)`) til høyre når `bReady`; egen rad har bakgrunn `(1,1,1,0.08)`. Maks 10 rader.
- **Under hver kolonne** knapp «Bli med på rødt lag» / «… blått lag». Lov når `Phase == Lobby && T != Mine &&
  (Mine == 0 || Count(T) < Count(Mine)) && Count(T) < AGSGameMode::MaxPerTeam` (samme regel som serveren i
  `RequestTeam`). Ellers sperret med grunn under (0.75, dempet): «Du er her» / «Laget er fullt» / «Lagene blir skjeve» /
  «Bare før nedtellingen». Klikk → `PS->ServerRequestTeam(T)` (serveren bestemmer; respawn ved ny base kommer fra plan-01).
- **Nederst:** «Klar» / «Ikke klar likevel» (aksent, `PC->ToggleReady()`, tekst fra `PS->bReady`) og «Varm opp»
  (lukker). Verten (`NM_ListenServer`) får en linje: «Venner blir med på: 192.168.1.20» (første fra `LocalAddresses`).
  Nederst (0.75, dempet): «%s klar · %s lukk · %s åpner igjen» med `KeyName` for `IA_Ready`, «Esc» og `GS_Team`.

### 6. Innstillinger (lagres med en gang)
Lagres i `GGameUserSettingsIni`, seksjon `[Gloshaugen]` (samme seksjon og nøkler som Kamp der de finnes:
`MouseSensitivity`, `LastJoinAddress`); `GConfig->Flush(false, GGameUserSettingsIni)` etter hver endring (ikke hver
frame under dra – først når knappen slippes). Grafikk via `UGameUserSettings* GS = GEngine->GetGameUserSettings()`:
endre, `ApplySettings(false)`, `SaveSettings()` (som Kamps `Grafikk`/`Opplosning`, men uten konsollmeldinger).
Rader (etikett venstre, kontroll fra 45 % av bredden), `Choice` med ◀ ▶:
1. **Grafikk:** Lav · Middels · Høy · Episk · Film → `SetOverallScalabilityLevel(0..4)`; vis `PC->GetGraphicsLevel()`.
2. **Oppløsning:** 50 · 60 · 72 · 85 · 100 % → `SetResolutionScaleValueEx`; vis `PC->GetResolutionPercent()`.
3. **Strålesporing (RTX):** Av/På → `PC->Straaling(On)` + `[Gloshaugen] RayTracing`.
4. **Skjermmodus:** Fullskjerm · Fullskjerm i vindu · Vindu → `SetFullscreenMode(EWindowMode::Fullscreen /
   WindowedFullscreen / Windowed)`; fullskjerm-valgene får `SetScreenResolution(GetDesktopResolution())`; vindu får
   1600×900 hvis dagens oppløsning er lik skjermens. I editoren (`GIsEditor`) vis «(gjelder i ferdig spill)» etter verdien.
5. **Vertikal synk:** Av/På → `SetVSyncEnabled`.
6. **Bildefrekvens:** 30 · 60 · 90 · 120 · 144 · Ubegrenset → `SetFrameRateLimit(N / 0)`.
7. **Musfølsomhet:** `Slider` 0,20–3,00 (steg 0,05), tall til høyre med to desimaler → `PC->MouseSensitivity`
   (Kamp leser feltet direkte), lagres som `MouseSensitivity`.
8. **Lydstyrke:** `Slider` 0–100 % → `ApplyVolume`, lagres som `Volume` (standard 1.0).
   `ApplyVolume(UWorld* W)`: `if (FAudioDeviceHandle D = W->GetAudioDevice()) D->SetTransientPrimaryVolume(V);`
   (`AudioDevice.h`; trådsikker, sender selv til lydtråden – `AudioDevice.cpp` linje 7543). Kalles også ved hver ny verden (steg 2.1).
9. **Navn:** `Field("Name")`.
10. Knapp **«Taster …»** → side `Controls`.
Nederst (0.75, dempet): «Alt lagres med en gang.»
**Navn** (`EnsureName`): `[Gloshaugen] PlayerName`; finnes ikke → `FString::Printf(TEXT("Spiller %d"), FMath::RandRange(100, 999))`,
lagres. Lagring av feltet (Enter, klikk utenfor, lukking av siden): trim, maks 20; tomt → behold forrige; i en verden med
`PlayerState` → `PC->ServerChangeName(Navn)`.

### 7. Taster (keybinds) og Kontroller-siden
**Hvis runden blir for stor, kan dette steget tas ut uten at resten påvirkes** (Kontroller-siden blir da en ren
oversikt og `KeyName` leser bare dagens taster). Planen regner med at det tas med.
- **Rad-id:** `<handlingsnavn>.<tasten Kamp ga den>`, f.eks. `IA_Jump.SpaceBar`, `IA_Move.W`, `IA_Crouch.C`. Egne:
  `GS_Team.L` (lagvalg). Lagres i `[Gloshaugen.Keys]` som `IA_Jump.SpaceBar=J` (`FKey::ToString()`); bare avvik lagres.
- **Originaltaster:** `static TMap<TWeakObjectPtr<UInputMappingContext>, TArray<FKey>> Originals` – første gang en kontekst
  sees, kopier `Key` for hver `GetMappings()[i]`. Handlingsnavn: `Mapping.Action->GetFName()` (Kamps `MakeAction` gir
  navnene `IA_*`).
- **Utenfor listen** (kan ikke endres, men vises fast): handlingene `IA_Look`, `IA_Scroll`, `IA_Menu`, og alle mappinger
  med akse-taster (`IsAxis1D/2D/3D`). **Reserverte taster** = tastene i disse mappingene (Esc, F10, mus, hjul) + `Escape`.
- **`ApplySavedKeys(PC)`:** for hver mapping `i`: `Id`; lagret og `FKey(FName(*Str)).IsValid()` → `C->GetMapping(i).Key =`
  lagret, ellers original. Så `if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer())) Sub->RequestRebuildControlMappings();`
  Én gang per kontekst (`TSet<TWeakObjectPtr<UInputMappingContext>>`), og etter hver endring. (Kamps `SetupInputComponent`
  legger til den samme konteksten; vi endrer selve objektet, så rekkefølgen spiller ingen rolle.)
- **`TeamKey()`:** `[Gloshaugen.Keys] GS_Team.L`, ellers `EKeys::L`.
- **`AssignKey(PC, S, Row, NewKey)`:** godta bare `IsValid`, ikke gamepad/berøring/akse, ikke `MouseScrollUp/Down`, ikke
  reservert → ellers `KeyNote` «Esc, F10, musa og hjulet er satt av og kan ikke brukes.» Har en annen rad `NewKey`: bytt
  (den får radens gamle tast) og `KeyNote` «%s var brukt av «%s». De har byttet.» Lagre begge (fjern nøkkelen når den er lik
  originalen), `Flush`, bruk på konteksten, bygg på nytt. `ListenRow = INDEX_NONE`.
- **Side «Kontroller»:** to kolonner med rader (28 px): etikett (0.85) og en tastboks (bredde 170) med `KeyDisplay`;
  klikk på raden → `ListenRow = i` og boksen viser «Trykk en tast … (Esc avbryter)» i aksent. Rekkefølge = Kamps
  mapping-rekkefølge, `GS_Team` sist. Under: faste linjer (dempet) «Mus: se rundt», «Musehjul: bla i våpnene, eller i sekken
  når inventaret er åpent», «Esc / F10: meny». `KeyNote` (aksent). Knapp **«Tilbakestill alle taster»** →
  `GConfig->EmptySection(TEXT("Gloshaugen.Keys"), GGameUserSettingsIni)`, originaler tilbake, bygg på nytt.
  Nederst en kort boks «Capture the Flag»: «Ta motstanderens flagg og bær det til din egen base. Du får bare poeng når
  ditt eget flagg står hjemme. Første lag til %d erobringer vinner.» (`AGSGameState::CaptureLimit`).
- **Etiketter** (id → norsk; ukjent handling → navnet uten `IA_`, slik at nye handlinger fra Kamp vises av seg selv):
  `IA_Move.W` Gå fremover · `IA_Move.S` Gå bakover · `IA_Move.A` Gå til venstre · `IA_Move.D` Gå til høyre ·
  `IA_Jump` Hopp / klatre · `IA_Sprint` Løp · `IA_Crouch.C` Huk (løp + huk = skli) · `IA_Crouch.LeftControl` Huk (ekstra
  tast) · `IA_Fire` Skyt · `IA_Aim` Sikt · `IA_Reload` Lad · `IA_Slot1..4` Våpen 1..4 · `IA_Interact` Plukk opp / bruk PC
  · `IA_UseItem` Bruk valgt ting · `IA_NextItem` Neste ting i sekken · `IA_DropItem` Slipp valgt ting · `IA_Melee` Slag ·
  `IA_Inventory` Inventar · `IA_Scoreboard` Poengtavle (hold) · `IA_Inspect` Se på våpenet · `IA_Ready` Klar (lobby) ·
  `GS_Team` Velg lag (lobby). Slå opp `Id` først, så handlingsnavnet alene.
- **`KeyDisplay(FKey)`** norsk for de vanlige: `SpaceBar` Mellomrom, `LeftShift` V. Shift, `RightShift` H. Shift,
  `LeftControl` V. Ctrl, `RightControl` H. Ctrl, `LeftAlt` Alt, `LeftMouseButton` Venstre musknapp, `RightMouseButton`
  Høyre musknapp, `MiddleMouseButton` Midtre musknapp, `ThumbMouseButton` Musknapp 4, `ThumbMouseButton2` Musknapp 5,
  `Escape` Esc, `BackSpace` Rettetast, `Up/Down/Left/Right` Pil opp/ned/venstre/høyre; ellers `GetDisplayName().ToString()`.
- **`KeyName(PC, Action)`**: første mapping for handlingen i `PC->MappingContext` → `KeyDisplay`; `GS_Team` → `TeamKey()`;
  ukjent → tom streng. **`Controls(PC)`**: radene + de faste linjene (`bRebindable = false`).

### 8. `GSHelp.cpp` (plan-01-fila)
- Øverst i `DrawImpl`, etter at `PC` er funnet: `if (GSMenu::IsFullscreen(PC)) return;`
- Lobby-panelet (del 3): tegn bare når `!GSMenu::IsModal(PC)`. Første linje blir
  `FString::Printf(TEXT("%s: velg lag."), *GSMenu::KeyName(PC, TEXT("GS_Team")))`, og «Trykk Enter når du er klar.» bruker
  `GSMenu::KeyName(PC, TEXT("IA_Ready"))`.
- Fjern del 5 (L-tasten med `WasInputKeyJustPressed`) og `LastTeamKeyFrame` – L tas nå av `GSMenu::OnKey`.
- `#include "GSMenu.h"`.

### 9. `GSGameState.cpp` og `GSGameMode`
- `AGSGameState::BeginPlay`: rett etter `GSHelp::EnsureHooked();` (samme `if`): `GSMenu::EnsureHooked();`
  (rekkefølgen gjør at menyen tegnes etter, altså over, GSHelp). `#include "GSMenu.h"`.
- `AGSGameMode`: `virtual void ChangeName(AController* Controller, const FString& NewName, bool bNameChange) override;`
  Rens: fjern tegn `< 32` og `" ? = #`, `TrimStartAndEnd`, maks 20; tom → `return` (behold gammelt navn). Unikt: tatt av en
  annen `PlayerState` (uten hensyn til store/små bokstaver) → legg til « 2», « 3» … (kort ned grunnen så alt er ≤ 20).
  Så `Super::ChangeName(Controller, Clean, bNameChange)`. (Motorens `InitNewPlayer` kaller også `ChangeName`, så navn fra
  URL renses på samme måte. `APlayerController::ServerChangeName` er pålitelig og validert av motoren.)

### 10. Config, Build.cs, bestilling
- `Config/DefaultEngine.ini`, ny seksjon nederst (ikke rør andres linjer):
  ```
  [/Script/Engine.Engine]
  GameViewportClientClassName=/Script/Gloshaugen.GSGameViewportClient
  ```
- `Gloshaugen.Build.cs`: én ny linje etter `PublicDependencyModuleNames`:
  `PrivateDependencyModuleNames.AddRange(new string[] { "Sockets", "ApplicationCore" });`
- **Bestilling til Kamp** (planleggeren har skrevet den i `bestillinger.md`): i `GSHUD::DrawMenu` byttes «Start server …»
  og «Bli med: …» med én knapp «Hovedmeny» (id `Host`), og linja «Annen adresse: trykk § …» fjernes; i
  `AGSPlayerController::MenuClick` kalles `GSMenu::OpenMain(this)` for `Host`/`Join` og `GSMenu::AskQuit(this)` for `Quit`
  (etter `CloseMenu()`); hinttekstene i HUD bruker `GSMenu::KeyName(...)`. Til det er levert fanger `GSMenu::OnKey` klikkene
  på disse tre knappene (steg 3.3), så ingen kamp forlates uten bekreftelse.

### 11. `IMPROVEMENTS.md`: ny seksjon øverst (over runde 1) med linjene under. Ikke rør andres linjer.

### 12. Bygg
`Get-Process UnrealEditor` (skal ikke kjøre). Bygg med kommandoen i `CLAUDE.md`, med `-WaitMutex`. Feiler bygget i en fil
laget ikke eier: vent og prøv igjen (inntil 4 ganger), ikke rett filen.

## Ferdig når
- Bygget går gjennom uten feil eller nye advarsler i lagets filer; `git status` viser bare filene i «Filer» (+ planfiler).
- Lest i koden: `OnKey` returnerer `false` for alt når ingen GSMenu-flate er modal (bortsett fra lagvalg-tasten i
  lobbyen og de tre fangede Esc-meny-klikkene), og `true` for alle taster når en flate er modal; `SetModal` kaller
  `FlushPressedKeys` begge veier.
- Lest i koden: «Ny kamp som vert», «Bli med i en annen», «Forlat kampen» og «Avslutt» går alltid via dialog når
  verden er `NM_ListenServer`/`NM_Client`, også når de startes fra Kamps Esc-meny.
- Lest i koden: lagvalgknappen bruker samme regel som `AGSGameMode::RequestTeam`, og panelet lukkes når fasen blir `Playing`.
- Lest i koden: ingen konsollkommando trengs for noe i menyen (`Host`/`Join`/`OpenLevel`/`QuitGame` kalles direkte).
- Det Andreas skal se (Play, vanlig «Standalone»): hovedmenyen over campus som sakte dreier, uten HUD; WASD og klikk
  flytter ikke figuren. Skriv navn med ø; Innstillinger: Grafikk Lav → bildet endres; Lydstyrke 0 → stille;
  Kontroller: klikk «Hopp / klatre», trykk J → J står der; «Øv alene» → J hopper, Mellomrom ikke; stopp og start Play →
  fortsatt J; «Tilbakestill alle taster». Bli med → skriv 10.0.0.99 → «Kobler til … s» → etter 20 s «Kunne ikke koble til».
  Start spill → kartet lastes som vert og lagvalget åpnes. Med «Play As Listen Server» og 2 spillere: begge får
  lagvalget; klikk «Bli med på blått lag» (sperret når det gjør lagene skjeve), «Klar» hos begge → nedtelling → panelet
  lukkes når kampen starter; L åpner det igjen i lobbyen. I kampen: Esc → «Start server» → hovedmenyen med «Tilbake til
  kampen»; «Forlat kampen» spør først og sier at kampen avsluttes for den andre.

## Risiko
- **Innputt-krokene** `OnOverrideInputKey`/`OnOverrideInputAxis` er enkeltdelegater: binder noe annet seg senere
  (f.eks. CommonUI), vinner det siste. Ingen andre bruker dem i dag. Konsollen (§) får tastene før oss i editoren.
- **`GameViewportClientClassName`** mangler eller lastes ikke: tekstfeltene tar da bare 0–9, punktum, A–Z og bindestrek
  (reserven i `OnKey`) – Bli med virker, men navn uten æøå. Alt annet virker.
- **PIE:** reise/frakobling (`Host`, `OpenLevel`, mislykket `Join`) kan oppføre seg litt annerledes i editoren enn i et
  pakket spill (PIE kan avslutte økten ved nettverksfeil). Test feilmeldingen også med «Standalone Game». Flere PIE-vinduer
  i én prosess: tilstanden er per vindu.
- **Tegning på `DebugCanvas`** (samme krok som plan-01): finnes også i Shipping (`GameViewportClient.cpp` linje
  1514–1535, uten `#if`). Er lerretet null, tegnes ingenting og ingen innputt svelges (modal krever at vi tegner).
- **Kamp endrer en standardtast** i `EnsureInputCreated`: en lagret ombinding for den gamle raden blir foreldreløs (spilleren
  får den nye standardtasten). Kamp bes si fra i `bestillinger.md` når standardtaster endres. Nye handlinger vises av seg selv.
- **Hinttekster i Kamps HUD** viser standardtastene til bestillingen er levert (H-oversikten i plan-03 leser `GSMenu::Controls`).
- **`bShowHUD = false`** i hovedmenyen: settes tilbake i `CloseMain`/`SetModal(off)`; ny verden har ny HUD.
- **Lydstyrke** er hovedvolum på lydenheten (ingen egne lydklasser uten .uasset); multipliseres med alt GSAudio/GSEnvironment spiller.
- **Navn:** serveren renser og gjør unikt; en juksende klient kan bytte navn ofte (ufarlig). Vinneren lagres nå som lagnavn (plan-01).
- **Lagvalg-panelet** viser bare det klienten vet; serveren avgjør. Bytter man lag, får man ny kropp (plan-01) mens panelet står.
- **Feilmelding fra motorens egne tekster** (`Err`) er ofte engelsk; den vises bare som liten, dempet detalj under den norske teksten.

## IMPROVEMENTS-linjer
```
## Runde 2 (Spillbarhet): hovedmeny (26. nov.)

402. Hovedmeny når spillet starter: Start spill, Bli med, Øv alene, Innstillinger, Kontroller og Avslutt, med campus som rolig, sakte dreiende bakgrunn
403. Bli med: skriv vertens adresse med tastaturet eller lim den inn med Ctrl+V, og siste adresse huskes. Konsollen trengs ikke lenger, så det virker også i et ferdig pakket spill
404. Start spill viser adressen vennene dine skal skrive inn, og forklarer brannmur og port
405. Melding på skjermen når tilkoblingen feiler (ingen svar, avvist, ulik versjon, port i bruk) eller verten avslutter, og du kommer tilbake til hovedmenyen
406. «Kobler til …» med sekunder og Avbryt, og forsøket gis opp etter 20 sekunder i stedet for å henge
407. Bekreftelse før du forlater en kamp (ny kamp som vert, bli med i en annen, forlat kampen, avslutt), også fra Esc-menyen. Verten får vite at kampen avsluttes for alle
408. Velg ditt eget spillernavn, også med æ, ø og å. Det huskes, og serveren rydder navnet og skiller like navn
409. Innstillinger samlet på én side: grafikknivå, oppløsning, strålesporing, skjermmodus, V-sync og maks bildefrekvens, lagret mellom øktene
410. Musfølsomhet med glidebryter i fine steg, og strålesporing huskes nå til neste gang du starter spillet
411. Lydstyrke for hele spillet, lagret
412. Tastene kan endres: klikk en handling og trykk ny tast. Tasten byttes med den som hadde den fra før, alt lagres, og «Tilbakestill» gir standardtastene
413. Kontroller-oversikt i menyen med alle taster slik de faktisk er nå, og en kort forklaring av Capture the Flag
414. Lagvalg med mus i lobbyen: begge lagene med navn og klar-merke, bytt lag med ett klikk, Klar-knapp og nedtelling. Åpnes av seg selv når du kommer inn, og med L
415. Mens menyen, lagvalget eller en dialog er åpen, når ingen taster eller klikk spillet, så du ikke skyter, hopper eller slipper ting ved et uhell
```
