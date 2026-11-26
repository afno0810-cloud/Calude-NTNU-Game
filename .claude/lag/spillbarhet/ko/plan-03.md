# Plan-03 (Spillbarhet, runde 3): knappehjelp og hint, og CTF-regler som mangler

Bygger på koden etter plan-01 (commit `0f788c6`), det nye kartet (`48f5cbf`), Kamps venneild/85 % fart (`2f6f46b`) og
**plan-02 (hovedmenyen, `GSMenu.*`)**. Finnes ikke `GSMenu.h` med `KeyName` og `Controls` når byggeren starter: stopp og
meld fra til dirigenten. `kontroll/runde-01.md` fantes ikke da planen ble skrevet (ingen kontrollfunn å ta med).

Vertsvalg, regel utenfor området, sidebytte og respawn i bølger er lagt til **plan-04** for å holde runden liten.

## Mål
En ny spiller ser selv hvilke taster som gjør hva og hvordan Capture the Flag spilles (H-oversikt og korte hint når
de trengs), og kampen tåler det som skjer i ekte kamper: bæreren kobler fra, noen kommer sent, tiden går ut mens et
flagg er ute, og en bærer som gjemmer seg blir funnet.

## Forbedrer (gammelt)
- **Flagget når bæreren kobler fra** (funn C1): i dag oppdages det først av flaggets egen sjekk, uten beskjed til noen.
  Blir: flagget legges ned med en gang der han sto, og hendelseslista sier «X forlot kampen».
- **Kampslutt på tid** (`AGSGameMode::Tick`): i dag kuttes kampen midt i et flaggløp. Blir: overtid til flagget er
  erobret eller hentet hjem, høyst 2 minutter.
- **Lobby-panelet og statuslinja i `GSHelp`**: viser overtid, «motstanderne vet hvor du er», og beskjed til den som
  kommer inn midt i en kamp (funn C2).
- **Loot og kameraer** (Grafikks bestilling, P1/P9): 428 ting snurrer og 38 kameraer svinger hver frame med skygge på.
  Blir: bare det som er nær deg beveger seg, uten skygge, og lysene har avstandsgrense.

## Nytt
- **H åpner en oversikt** over alle taster (slik de er valgt i Innstillinger) og reglene i Capture the Flag.
- **Hint som dukker opp når de trengs**: 12 korte hint (loot, kasse, PC, printer, tomt magasin, skadet, flagget).
  Hvert vises høyst to ganger og huskes.
- **Flaggbæreren blir synlig for motstanderne etter 20 s** (Andreas' godkjente forslag): en nøktern markør som viser
  hvor han sist ble sett, oppdatert hvert 2. sekund.
- **Overtid** (ønskeliste 33).

## Hvorfor nå
0a (knappehjelp) er Andreas' eldste åpne ønske og hovedagentens bestilling for runden. `GSMenu::KeyName` finnes fra
plan-02, så hintene kan vise tastene spilleren faktisk har valgt. CTF-reglene her er små tillegg i funksjoner som
allerede finnes, og C1 er merket «Høy med flagg» i `funn.md`.

## Filer
| Fil | Endring |
|---|---|
| `Public/GSFlag.h`, `Private/GSFlag.cpp` | avsløring etter 20 s, `DropIfCarriedBy`, ingen nye flagg tas i overtid |
| `Public/GSGameState.h`, `Private/GSGameState.cpp` | `bOvertime` (replikert), to konstanter |
| `Public/GSGameMode.h`, `Private/GSGameMode.cpp` | `Logout`, overtid i `Tick`, beskjed ved sen innkobling |
| `Public/GSHelp.h`, `Private/GSHelp.cpp` | H-oversikt, hint, markør, overtid, sen innkobling, `ResetHints()` |
| `Private/GSMenu.cpp` | to små ting: H er reservert tast, knapp «Vis hintene på nytt» |
| `Private/GSPickup.cpp`, `Private/GSTerminal.cpp` | ytelse (steg 7) |
| `IMPROVEMENTS.md` | ny seksjon øverst, 460–475 |

Ikke rør `GSCharacter.*`, `GSHUD.*`, `GSPlayerController.*`, `GSTypes.h`, `catalog.py` (ikke kjør `gen_cpp.py`).
**Gjør de tre headerne (`GSFlag.h`, `GSGameState.h`, `GSGameMode.h`) ferdige først og i ett.** `static constexpr`-
verdiene `CaptureLimit`/`MatchMinutes` beholdes uendret (Kamp og `GSMenu` leser dem).

## Steg

### 1. `GSGameState`
- `.h`, under CTF-feltene: `UPROPERTY(Replicated) bool bOvertime = false;`
  `static constexpr float OvertimeMaxSeconds = 120.f;` `static constexpr float CarrierRevealSeconds = 20.f;`
- `.cpp`: `DOREPLIFETIME(AGSGameState, bOvertime);`

### 2. `GSFlag` – avsløring, frakobling, overtid
`.h` (public): 
```cpp
/** The carrier has had the flag for CarrierRevealSeconds: the flag's own team is told where he was last seen. */
UPROPERTY(Replicated) bool bRevealed = false;
UPROPERTY(Replicated) FVector_NetQuantize RevealLocation;   // refreshed every RevealInterval while bRevealed
/** Server: the carrier is leaving the game; lay the flag down where he stood. */
void DropIfCarriedBy(const APlayerState* PS);
static constexpr float RevealInterval = 2.f;
```
protected: `float TakenAt = 0.f; float LastRevealAt = 0.f;`

`.cpp`:
- `GetLifetimeReplicatedProps`: de to nye. (Flagget er `bAlwaysRelevant`, så alle får dem. Posisjonen sendes først når
  `bRevealed` er satt, så ingen kan lese den ut av nettrafikken før 20 s.)
- `Take`: `TakenAt = GetWorld()->GetTimeSeconds(); bRevealed = false;` (klokka starter på nytt for hver ny bærer).
- `Drop`, `ReturnHome`, `Capture`, `ResetHome`: `bRevealed = false;` (før `Announce`/`ForceNetUpdate`).
- `ServerStep`, grenen `Carried`, etter at bæreren er funnet gyldig og før erobringssjekken:
  ```cpp
  const float Now = World->GetTimeSeconds();
  if (Now - TakenAt >= AGSGameState::CarrierRevealSeconds && (!bRevealed || Now - LastRevealAt >= RevealInterval))
  {
      RevealLocation = C->GetActorLocation();
      bRevealed = true;
      LastRevealAt = Now;
      ForceNetUpdate();
  }
  ```
- `ServerStep`, grenen hjemme/mistet, der en motstander tar flagget (`PS->Team != Team`): legg til vilkåret
  `&& !(GS->bOvertime && State == EGSFlagState::Home)` – i overtid kan ingen ta et flagg som står hjemme (ellers kan
  overtiden holdes i gang). Et flagg som ligger ute kan fortsatt plukkes opp.
- `DropIfCarriedBy`: `if (HasAuthority() && State == EGSFlagState::Carried && Carrier == PS) Drop();`
  (`Drop` tåler at pawnen er borte: da brukes `LastCarrierLocation`, som serveren oppdaterer hver frame i `UpdatePose`.)

### 3. `GSGameMode` – frakobling, sen innkobling, overtid
`.h`: `virtual void Logout(AController* Exiting) override;` og (protected) `bool AnyFlagAway() const;`

**`Logout`** (motoren ødelegger pawnen i `APlayerController::Destroyed` → `PawnLeavingGame` *før* `Logout` kalles fra
`AController::Destroyed`, se `PlayerController.cpp` 1633–1646 og `Controller.cpp` 595–607; pawnen er altså borte her, men
`Exiting->PlayerState` lever ennå):
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
(Død, drap til siste skytter og at utstyret blir liggende krever Kamps `PawnLeavingGame` – bestilt, se under.)

**Sen innkobling** (`RestartPlayer`): der `AssignTeam(PS)` kalles fordi laget er `None`, og fasen er `Playing`: start en
timer på 2 s (navnet fra klienten rekker da fram) med svak peker til `PS`, som kaller
`GS->AddKill(PS->GetPlayerName(), FString(GSTeam::Name(PS->Team)).ToLower(), TEXT("ble med på"), false);`
(gir «Kari [ble med på] rødt lag» i hendelseslista hos alle). Spawn ved lagets base skjer allerede.

**`AnyFlagAway()`**: sann hvis noen `AGSFlag` har `State != Home`.

**`Tick`, `Playing`-grenen** byttes til:
```cpp
if (GS->RedScore >= AGSGameState::CaptureLimit || GS->BlueScore >= AGSGameState::CaptureLimit) EndMatch();
else if (!GS->bOvertime && Now >= GS->PhaseEndsAt)
{
	if (AnyFlagAway())   // a flag run is on: play it out
	{
		GS->bOvertime = true;
		GS->PhaseEndsAt = Now + AGSGameState::OvertimeMaxSeconds;
		GS->AddKill(TEXT("Tiden er ute"), TEXT("til flagget er hjemme"), TEXT("overtid"), false);
	}
	else EndMatch();
}
else if (GS->bOvertime && (Now >= GS->PhaseEndsAt || !AnyFlagAway())) EndMatch();
else if (bSoloMatch && GS->PlayerArray.Num() >= 2) BackToLobby();   // as today
```
`GS->bOvertime = false;` i `StartMatch` (ved klokka), `EndMatch` (etter at vinneren er satt) og `BackToLobby`.
Vinner regnes som i dag (flest erobringer, lik stilling = uavgjort). Kamps kampklokke leser `PhaseEndsAt` og teller
derfor ned overtiden av seg selv.

### 4. `GSHelp.h`
Oppdater kommentaren og legg til:
```cpp
/** Forget which hints were shown, so they come again (button in the menu). */
GLOSHAUGEN_API void ResetHints();
```

### 5. `GSHelp.cpp` – tilstand, H-oversikt, markør, linjer
Les fila slik den er etter plan-02 først (L-tasten er borte, `GSMenu.h` er inkludert). Nye inkluderinger:
`GSCharacter.h`, `GSPickup.h`, `GSTerminal.h`, `GSItems.h`, `Misc/ConfigCacheIni.h`.

**Tilstand per HUD** (flere PIE-vinduer deler statiske variabler): `struct FHelpState { bool bOpen = false; uint64
KeyFrame = 0; FName HintId; float HintStart = -100.f; float NextHintAt = 0.f; float FirstDraw = -1.f; bool
bJoinedMid = false; };` i en `TMap<TWeakObjectPtr<const AHUD>, FHelpState>` (rydd ugyldige nøkler når den har over 16).
Tider er `World->GetTimeSeconds()`.

**a) H-tasten** (rett etter at `PC`, `GS`, `PS` er funnet; før `bMenuOpen`-returen):
`GPC->bMenuOpen || GPC->IsOnTerminal() || GSMenu::IsModal(PC)` → `bOpen = false`. Ellers:
`if (GFrameCounter != St.KeyFrame && PC->WasInputKeyJustPressed(EKeys::H)) { St.KeyFrame = GFrameCounter; St.bOpen = !St.bOpen; }`
(Vakten trengs fordi `Draw` og reservekroken kan kjøre samme frame. Ikke F1: den er «viewmode wireframe» i utviklingsbygg.)

**b) Oversikten** (`bOpen`): tegnes sist, oppå alt annet i `GSHelp`, og hint tegnes ikke samtidig.
Panel midt på skjermen, bredde `Min(820, SizeX − 64)`, `PanelColor` med alfa 0.9. Innhold:
- Tittel «Taster og regler», til høyre (dempet, 0.75) «H lukker».
- **Taster:** `GSMenu::Controls(PC)` i to kolonner (del på midten), rad 22 px, skala 0.8: `Label` i `TextColor`,
  `Key` høyrestilt i kolonnen i dempet rav `(0.85, 0.65, 0.25)`. Sist: «Hjelp (denne siden)» / «H».
- **På en PC / 3D-printer** (to korte linjer, dempet): les `AGSPlayerController::TerminalInput` og skriv tastene slik
  de faktisk er (utkastet: PC «1 kameraer · 2 spor spillere · 3 tilbake · Q/F forrige/neste kamera · E logg ut»,
  printer «Q/1/2 velg · F eller venstre mus start»). Stemmer de ikke med koden: rett teksten, ikke koden.
- **Capture the Flag** (overskrift + linjer, skala 0.8):
  1. «Ta motstanderens flagg ved å gå helt bort til det, og bær det til din egen base.»
  2. «Du får bare poeng når ditt eget flagg står hjemme.»
  3. «Bæreren går saktere. Etter 20 sekunder får motstanderne vite hvor han er.» (`CarrierRevealSeconds`)
  4. «Faller bæreren, blir flagget liggende. Motstanderne kan ta det opp; eget lag sender det hjem ved å gå bort til det.»
  5. «Første lag til %d erobringer vinner, ellers flest når %d minutter er gått.»
  6. «Er et flagg ute når tiden går, spilles det overtid til det er erobret eller hjemme (høyst 2 minutter).»
  7. «Du kommer tilbake ved egen base. Det du bar, ligger igjen der du falt.»
- Blir panelet høyere enn `SizeY − 40`: bruk skala 0.72 og rad 19 px.

**c) Påminnelse om H:** en dempet linje nederst til høyre (`SizeX − 24`, `SizeY − 28`, høyrestilt, skala 0.7)
«H: taster og regler», de første 45 s av lobbyen og de første 20 s av en kamp, bare til H er trykket to ganger totalt
(tell i `[Gloshaugen.Help] OverviewOpened`, samme lagring som hintene). Les `DrawBars`/ammo-feltet i `GSHUD.cpp` og
flytt linja opp hvis noe ligger der.

**d) Lobby-panelet:** legg til én dempet linje sist: «H viser alle taster og regler.»

**e) Statuslinja og egen linje:**
- `GS->bOvertime` (under `Playing`): tegn «Overtid» (skala 0.7, dempet rav) midt i rad 2 i statusboksen, og vis i 8 s
  etter at overtiden startet (lokal tid, husk overgangen i `FHelpState`) egen linje: «Overtid. Kampen varer til flagget
  er erobret eller hentet hjem.»
- Bærer du flagget og flaggets `bRevealed` er sann: bytt «Du bærer flagget. Bær det til basen din.» med
  «Motstanderne vet hvor du er. Bær flagget til basen din.»
- **Kom inn midt i en kamp:** første gang denne HUD-en tegnes (`FirstDraw < 0`) og fasen er `Playing`:
  `bJoinedMid = true`. De første 10 s vises (når ingen annen egen linje står der, ellers linja under):
  «Kampen er i gang. Du er på %s. Stillingen er %d–%d.» (lagnavn i små bokstaver, i lagets tekstfarge).

**f) Markør for avslørt bærer** (bare `Playing`, bare for laget som eier flagget: `PS->Team == Flag->Team`, og
`Flag->State == Carried && Flag->bRevealed`). Mål = `Flag->RevealLocation + (0,0,110)`.
- `FVector CamLoc; FRotator CamRot; PC->GetPlayerViewPoint(CamLoc, CamRot);`
  `const FVector L = CamRot.UnrotateVector(Target − CamLoc);` (X fram, Y høyre, Z opp).
- Foran (`L.X > 10`) og `PC->ProjectWorldLocationToScreen(Target, P)` innenfor skjermen med 48 px marg: tegn der.
  Ellers: retning `D = FVector2D(L.Y, −L.Z)` (er `L.X < 0` og `D` nesten null: `D = (0, 1)`), normaliser, og sett
  punktet på kanten av rektangelet (marg 48 px) fra skjermens midte langs `D`.
- Tegning (nøkternt, ingen blink): åpen firkant 12×12 px av fire 2 px-striper i `TeamText(Flag->Team)` med alfa 0.85,
  under den «Flagget · %d m» (skala 0.7, avstand fra `CamLoc` i hele meter, rundet til nærmeste 5).
- Ikke tegn den når oversikten er åpen, eller når `GSMenu::IsModal(PC)`.

### 6. `GSHelp.cpp` – hint
**Lagring:** `[Gloshaugen.Help]` i `GGameUserSettingsIni`, nøkkel = hint-id, verdi = antall ganger vist. Les til en
statisk `TMap<FName, int32>` første gang (ikke hver frame). Tell opp og `GConfig->Flush(false, GGameUserSettingsIni)`
når et hint *begynner* å vises. Vises ikke når tallet er ≥ 2. `ResetHints()`: `GConfig->EmptySection(...)`, tøm
kartet, `Flush`.

**Når:** `Playing`, eller `Lobby` for de seks første under. Aldri når oversikten er åpen, `GPC->bMenuOpen`,
`GPC->IsOnTerminal()`, `GPC->bShowInventory`, `GPC->bShowScoreboard`, `GSMenu::IsModal(PC)`, eller pawnen mangler/er død.
Ett om gangen: 7 s (ton inn/ut 0,4 s), så minst 6 s pause. Velg første aktuelle i rekkefølgen under.
`K(x)` = `GSMenu::KeyName(PC, TEXT(x))`; er strengen tom, bruk standardtasten i parentes.
`C = Cast<AGSCharacter>(PC->GetPawn())`; bruk bare det som er `public` i `GSCharacter.h` (trengs noe privat: dropp
hintet og skriv det i rapporten).

| Id | Når | Tekst |
|---|---|---|
| `OwnTaken` | eget flagg `Carried` | «Flagget deres er tatt. Stopp bæreren, så blir det liggende.» |
| `OwnDropped` | eget flagg `Dropped` | «Flagget deres ligger ute. Gå helt bort til det, så er det hjemme igjen.» |
| `Carry` | `PS->CarriedFlagTeam != 0` | «Med flagget går du saktere. Etter 20 sekunder får motstanderne vite hvor du er.» |
| `EnemyFlag` | bærer ikke, motstanderens flagg er `Home`/`Dropped` og under 30 m unna | «Gå helt bort til flagget for å ta det.» |
| `Weapon` | `C->GetFocusPickup()` er et våpen (`GSItems::Find(ItemId)->IsWeapon()`), ikke `bLocked` | «%s tar våpenet. Du bærer fire. %s–%s eller rullehjulet bytter.» (`IA_Interact` E, `IA_Slot1` 1, `IA_Slot4` 4) |
| `Item` | fokus-pickup er noe annet, ikke låst | «%s legger tingen i sekken. %s blar, %s bruker, %s slipper.» (`IA_Interact`, `IA_NextItem` Q, `IA_UseItem` F, `IA_DropItem` G) |
| `Crate` | fokus-pickup `bLocked` | «Låst kasse. Adgangskort åpner den med en gang, dirkesett tar noen sekunder.» |
| `Computer` | `C->GetFocusTerminal()` er `EGSTerminalKind::Computer` | «%s: på PC-en ser du gjennom overvåkningskameraene.» (`IA_Interact`) |
| `Printer` | fokus-terminal er printer | «%s: printeren lager utstyr av filament du finner som loot.» |
| `Empty` | gjeldende våpen bruker ammo, magasinet er 0 og reserven > 0 (les feltnavnene i `FGSWeaponSlot`) | «Tomt magasin. %s lader.» (`IA_Reload` R) |
| `Limb` | `C->LegInjury > 0.3f` eller `C->ArmInjury > 0.3f` | «Skadd bein gjør deg treg, skadd arm gjør siktet ustøtt. Bandasje eller førstehjelpsskrin hjelper.» |
| `Hurt` | `C->Health < 60` | «Du er skadet. Velg medisin med %s og bruk den med %s. Det tar tid, så finn dekning først.» (`IA_NextItem`, `IA_UseItem`) |

(Sult og tørst får ikke hint: de virker ikke i praksis, funn B2. Sjekk skalaen på `LegInjury`/`ArmInjury` i
`GSCharacter.cpp` og juster grensen så hintet kommer ved en merkbar skade.)

**Tegning:** én linje (del i to ved over 70 tegn), midtstilt, `Y = SizeY * 0.68`, skala 0.85, `TextColor` med alfa 0.92,
`PanelColor`-boks bak med 10 px luft. Les `DrawPrompt` og hurtiglinja i `GSHUD.cpp` og flytt `Y` hvis noe ligger der.
Ingen ikon, ingen lyd, ingen blink.

**`GSMenu.cpp` (to små tillegg):** (1) legg `EKeys::H` blant de reserverte tastene i `AssignKey`, og ta H med i meldingen
(«Esc, F10, H, musa og hjulet er satt av …»). (2) På siden «Kontroller», ved «Tilbakestill alle taster»: knapp
**«Vis hintene på nytt»** → `GSHelp::ResetHints();` og `KeyNote` = «Hintene vises på nytt.»
(`#include "GSHelp.h"`). Finnes ikke tastesiden (steg 7 i plan-02 ble tatt ut): legg knappen på Innstillinger.

### 7. Ytelse (Grafikks bestilling, P1 og P9) – tas ut først hvis runden blir for stor
**`GSPickup.cpp`:**
- Konstruktør: `Mesh->SetCastShadow(false);` (tingen svever og snurrer; skyggen ugyldiggjør skyggecachen hver frame.
  `Crate` beholder skyggen – den står stille.)
- `BeginPlay`: `if (GetNetMode() == NM_DedicatedServer) SetActorTickEnabled(false);`
- `Tick` (ingen nye felt, så `GSPickup.h` endres ikke; tick-intervallet er tilstanden):
  ```cpp
  const bool bSpin = bAvailable && !bLocked;
  bool bNear = false;
  if (bSpin)
  {
      const APlayerController* LPC = GetWorld()->GetFirstPlayerController();
      FVector Eye; FRotator Rot;
      if (LPC && LPC->IsLocalController()) { LPC->GetPlayerViewPoint(Eye, Rot); bNear = FVector::DistSquared(Eye, GetActorLocation()) < FMath::Square(3000.f); }
  }
  SetActorTickInterval(bNear ? 0.f : 0.5f);
  if (!bNear) return;
  const float T = GetWorld()->GetTimeSeconds();
  Mesh->SetRelativeRotation(FRotator(0.f, FMath::Fmod(T * 60.f + (GetUniqueID() % 360), 360.f), 0.f));
  // height as today (BaseMeshZ + sin)
  ```
  (Vinkel fra klokka, så tingen ikke hopper når den våkner. 30 m holder: lenger unna ser man ikke at den står stille.)
**`GSTerminal.cpp`:**
- `AGSTerminal`-konstruktøren: `Glow->MaxDrawDistance = 4000.f; Glow->MaxDistanceFadeRange = 800.f;`
- `AGSSecurityCamera`-konstruktøren: `Mesh->SetCastShadow(false);` `RecLight->MaxDrawDistance = 3000.f;
  RecLight->MaxDistanceFadeRange = 600.f;`
- `AGSSecurityCamera::Tick`, sist: sving bare fort når noen ser det. `const APlayerController* LPC = GetWorld()->GetFirstPlayerController();`
  `const bool bWatched = LPC && LPC->GetViewTarget() == this;` nær = øyet under 40 m. `SetActorTickInterval(bWatched || bNear ? 0.f : 0.5f);`
  (Retningen regnes fra servertid, så et kamera som «sover» står riktig når noen logger inn på det. På dedikert server:
  intervall 0.5 alltid.)

### 8. `IMPROVEMENTS.md`: ny seksjon øverst med linjene under. Ikke rør andres linjer.

### 9. Bygg
Sjekk `Get-Process UnrealEditor`. Bygg med kommandoen i `CLAUDE.md` (**med** `-WaitMutex`). Feiler det i Kamps eller
Grafikks filer: vent og prøv igjen (inntil 4 ganger), ikke rett filene.

## Ferdig når
- Bygget går gjennom uten feil eller nye advarsler i lagets filer; `git status` viser bare filene i tabellen.
- Lest i koden: `Logout` legger ned flagget for den som går; flagg som står hjemme kan ikke tas mens `bOvertime` er
  sann; `bOvertime` nullstilles i `StartMatch`, `EndMatch` og `BackToLobby`; `bRevealed` nullstilles i alle fem
  overganger (`Take`, `Drop`, `ReturnHome`, `Capture`, `ResetHome`).
- Hver tast i hinttekstene kommer fra `GSMenu::KeyName`, og PC-/printer-linjene er sjekket mot `TerminalInput`
  (byggeren noterer det i rapporten).
- Det Andreas skal se (Play, én spiller): **H** åpner og lukker «Taster og regler»; se på en ting på bakken → ett kort
  hint nederst, som ikke kommer en tredje gang; loot langt unna står stille, nær deg snurrer det.
  Med to spillere: ta flagget og vent 20 s → den andre ser en liten firkant «Flagget · 85 m», og bæreren får
  «Motstanderne vet hvor du er»; lukk vinduet til bæreren → flagget ligger igjen og lista sier «forlot kampen».

## Risiko
- **Rekkefølge mot plan-02:** `GSHelp.cpp` og `GSMenu.cpp` endres av begge. Byggeren må lese filene slik de er i treet,
  ikke slik plan-02 beskriver dem. Mangler tastesiden, gjelder reserven i steg 6.
- **H-tasten** leses med `WasInputKeyJustPressed` i tegnekoden: virker bare for lokal spiller og ikke mens `GSMenu`
  svelger innputt (ønsket). Har spilleren fra før bundet en handling til H, skjer begge til han endrer den.
- **Avsløringen** går til alle klienter (flagget er alltid relevant); bare eierlaget tegner markøren. En jukser på
  bærerens lag lærer ingenting han ikke vet. Posisjonen er opptil 2 s gammel med vilje (melding, ikke veggsyn).
- **Overtid:** `PhaseEndsAt` flyttes 2 min fram, så klokka i Kamps HUD hopper fra 0:00 til 2:00. Statuslinja sier
  «Overtid», så det er forståelig. Poengtavla/sluttskjermen er uendret.
- **Frakobling:** kroppen og utstyret forsvinner fortsatt (motoren sletter pawnen før `Logout`), og ingen får drapet.
  Det er Kamps fil – bestilt. Flagget er uansett trygt.
- **Sen innkobling:** `bJoinedMid` settes også for verten hvis HUD-en lages på nytt midt i en kamp (skjer ikke i dag).
- **Ytelse:** `SetActorTickInterval` fra `Tick` gjelder fra neste tick; 428 ting på 2 Hz er neglisjerbart. Loot uten
  skygge kan se litt «løst» ut inne; det blir borte når loot legges på gulvet (backlog).

## IMPROVEMENTS-linjer
```
## Runde 3 (Spillbarhet): knappehjelp, hint og CTF-regler (26. nov.)

460. H åpner en oversikt over alle taster, slik du selv har satt dem i Innstillinger. H lukker den igjen
461. Oversikten forklarer Capture the Flag i sju korte linjer, og hva tastene gjør på en PC og en 3D-printer
462. En liten påminnelse «H: taster og regler» står nede til høyre de første sekundene, til du har brukt den
463. Hint første gang du ser på et våpen, en ting, en låst kasse, en PC og en 3D-printer
464. Hint første gang magasinet er tomt, du er skadet, eller har fått en skade i arm eller bein
465. Hint om flagget: første gang du er nær motstanderens flagg, bærer det, og når deres eget er tatt eller ligger ute
466. Hvert hint vises høyst to ganger og huskes til neste gang du spiller. Ingen hint mens meny, sekk, poengtavle eller PC er åpen
467. Hintene viser tastene du faktisk har valgt, og «Vis hintene på nytt» i menyen henter dem fram igjen
468. Flaggbæreren blir synlig for motstanderne etter 20 sekunder: en liten markør viser hvor han sist ble sett, med avstand
469. Bæreren får selv beskjed når motstanderne vet hvor han er
470. Overtid: er et flagg ute når tiden går, spilles kampen ferdig til flagget er erobret eller hentet hjem, høyst 2 minutter
471. I overtid kan ingen ta et flagg som står hjemme, så overtiden kan ikke holdes i gang
472. Kobler flaggbæreren fra, blir flagget liggende der han sto, og hendelseslista sier at han forlot kampen
473. Den som kommer inn midt i en kamp får vite laget sitt og stillingen, og de andre ser hvem som ble med på hvilket lag
474. Loot snurrer bare når du er nærmere enn 30 meter og kaster ikke skygge mens det svever, så bildet flyter jevnere
475. Overvåkningskameraene svinger bare fort når noen er nær eller ser gjennom dem, og lysene på PC-er, printere og kameraer tegnes ikke på lang avstand
```
