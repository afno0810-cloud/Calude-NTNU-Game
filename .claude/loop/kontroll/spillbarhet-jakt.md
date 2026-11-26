# Spillbarhetsjakt i Lag Kamps filer (kontrollør Kamp, 26. nov. kl. 23)

Lest fra HEAD (`4feff56`): `GSPlayerController.*`, `GSCharacter.*` (input, bevegelse, død, kamera, skyting),
`GSHUD.cpp` (Esc-menyen), og samspillet med `GSMenu.cpp`, `GSHelp.cpp` og `GSGameMode.cpp`. Motorkoden i
UE 5.8 er sjekket der funnene bygger på hvordan motoren oppfører seg. Loggen: `Saved/Logs/Gloshaugen.log`,
Play-økten 21:53:11–21:54:28 (UTC; 22:53 norsk tid). Ingen kode er endret, ingenting er bygget.

**Kort svar:** Kamps input er satt opp riktig og er ikke grunnen til at testen stoppet. Ifølge loggen kom
Andreas aldri ut av hovedmenyen, så Kamps styring ble aldri prøvd. Det alvorligste Kamp-funnet er at **Esc
stopper Play i editoren**. Både Kamps Esc-meny og GSMenus «Esc = tilbake» er dermed døde i editoren, og
GSMenu-panelene har ingen annen tast å lukkes med.

---

## 1. HØY (forklarer testen): økten kom aldri ut av hovedmenyen. Kamps styring ble aldri nådd
- **Logg:** ved start (21:53:14.659) skifter visningsfeltet til `CapturePermanently` (GSMenus
  `SetInputMode(GameOnly)` når verdenen starter, `GSMenu.cpp:1785`) og rett etterpå til `DoNotLock` +
  `CaptureDuringMouseDown` (`FInputModeGameAndUI`, hovedmenyen åpnes, `GSMenu.cpp:668-670`). I de neste
  74 sekundene kommer **ingen ny** `MouseCaptureMode Changed`. Hadde menyen lukket seg, ville
  `SetInputMode(FInputModeGameOnly())` (`GSMenu.cpp:679`) ha logget «CaptureDuringMouseDown ->
  CapturePermanently». Det kom heller ingen spor av at noe i menyen ble valgt (ingen reise eller
  vertsstart, ingen grafikkendring). Kl. 21:54:28 kom `Window 'Gloshaugen - Unreal Editor' being
  destroyed` og `Cmd: QUIT_EDITOR`: Andreas **lukket hele editoren** i stedet for å stoppe Play.
- Mens hovedmenyen er åpen, stopper `OnKey`/`OnAxis` (`GSMenu.cpp:1580-1588`, `1654-1657`) alle taster
  og all musebevegelse før kontrolleren. Det forklarer «styre figuren virket ikke».
- **Kamps del virket:** feilen `Gloshaugen: Enhanced Input is not set up` (`GSCharacter.cpp:376`) står
  ikke i loggen. Figuren fikk altså sine bindinger.
- **Rettelse:** dette er GSMenu-agentens sak (hvorfor klikk ikke treffer og menyen ikke lukkes). Kamp
  trenger ikke endre noe her.

## 2. HØY (test i editoren): Esc stopper Play før spillet ser tasten
- **Motoren:** `UGameViewportClient::InputKey` (`GameViewportClient.cpp:776-782`) kjører
  `UEditorEngine::ProcessDebuggerCommands` (`PlayLevel.cpp:1089`) **før** konsollen, `OnOverrideInputKey`
  (GSMenu) og kontrolleren. Der er `StopPlaySession` bundet til `EKeys::Escape`
  (`DebuggerCommands.cpp:358`) og kan alltid kjøres når Play går (`HasPlayWorld`, linje 471-473).
  F10 er bare bundet til «Step Over», og den kan kjøres bare ved et stoppunkt (linje 561-565). Når
  handlingen ikke kan kjøres, returnerer `ProcessCommandBindings` false (`UICommandList.cpp:232-241`).
  **F10 når altså spillet, Esc gjør det ikke.**
- **Kamp:** `GSPlayerController.cpp:112-113` binder IA_Menu til Escape og F10, og det er riktig. Men
  `GSHUD.cpp:188` skriver «Esc lukker menyen». I editoren stopper Esc hele økten.
- **Samspill med GSMenu (til Spillbarhet):** `Back()` ligger bare på Escape (`GSMenu.cpp:1616-1619`).
  F10 er satt av (`IsFixedAction` IA_Menu, `GSMenu.cpp:272`), men i et åpent panel blir F10 bare
  svelget (`return true` på linje 1651). **I editoren går det derfor ikke an å lukke et GSMenu-panel
  med tastaturet**, bare med klikk. Lagpanelet åpner seg selv i lobbyen (`GSMenu.cpp:1850-1855`). Da kan
  man verken gå eller se seg om før man treffer «Lukk» med musa eller trykker L.
- **Rettelse Kamp:** `GSHUD.cpp:188`: skriv «F10 lukker menyen (Esc stopper Play i editoren)» når
  `GetWorld()->WorldType == EWorldType::PIE`, ellers «Esc eller F10 lukker menyen». Samme regel i alle
  hinttekster Kamp eier som nevner Esc.
- **Rettelse Spillbarhet (bestilling):** i `OnKey`, behandle F10 (og gjerne
  `ActionKey(PC, "IA_Menu")`-tastene) som Escape, slik at `Back()`, lukk lagpanel og lukk dialog også
  virker i editoren.
- **For Andreas (valgfritt):** under Editor Preferences, Keyboard Shortcuts, Play World, sett «Stop» til
  Shift+Esc. Eller test med «Standalone Game», der Esc når spillet.
- **Lærdom bør presiseres:** «må alltid kunne lukkes med Esc» → «… med Esc, og med F10 i editoren (Esc
  stopper Play der)».

## 3. MIDDELS: Esc-menyen gjør mindre enn den ser ut til, og passer ikke i små vinduer
- **a) To menyer for det samme.** Kamps Esc-meny har fortsatt «Start server», «Bli med» og «Avslutt»
  (`GSHUD.cpp:175-176, 186`). GSMenu fanger klikkene ved å slå opp HUD-ens treffbokser fra forrige
  bilde (`GSMenu.cpp:1541-1563`). Hvis GSMenus krok ikke er bundet (den bindes først ved første
  `OnPostRender`, `GSMenu.cpp:1756-1763`), kjører Kamps `MenuClick` `Host()`/`Join()`/`quit` direkte,
  uten bekreftelse (`GSPlayerController.cpp:191-192, 209`). Bestillingen fra Spillbarhet (ÅPEN i
  `bestillinger.md`) løser dette: én knapp «Hovedmeny» → `CloseMenu(); GSMenu::OpenMain(this);`,
  `Quit` → `GSMenu::AskQuit(this)`, og linja om konsollen fjernes (`GSHUD.cpp:187`).
- **b) Menyen er ca. 610 px høy og starter på `SY/2 - 290` (`GSHUD.cpp:170`).** Når visningsfeltet er
  lavere enn ca. 640 px (Play i et lite visningsfelt i editoren på laptop), havner tittelen over kanten
  og raden for musfølsomhet og «Avslutt» under kanten. Rettelse: skaler med
  `S = FMath::Min(1.f, SY / 700.f)` på høyder og avstander, eller start på `FMath::Max(16.f, …)` og
  stable tettere.

## 4. MIDDELS: åpen Esc-meny stopper bare gange, sikting, skyting og hjul
- `ToggleMenu` stopper gange og blikk (`GSPlayerController.cpp:148-149`). `Look`, `StartFire` og
  `Scroll` sjekker `bMenuOpen` (`GSCharacter.cpp:427, 630, 731`). Alt annet virker bak menyen: hopp
  (`StartJump` 435), huk, lading, våpenbytte, **E (åpner en PC bak menyen)**, F, G, V, T, I og Enter
  (klar). Mellomrom eller E ved et uhell mens menyen er åpen gir rare følger.
- **Etter gjenoppliving eller kampstart med menyen åpen** kaller motoren `ResetIgnoreInputFlags()` i
  `ClientRestart` (`PlayerController.cpp:820`). Da kan man gå rundt mens menyen og musepekeren vises.
- **Rettelse:** lag én vakt i kontrolleren, `bool IsGameInputBlocked() const { return bMenuOpen ||
  GSMenu::IsModal(this); }`, og la hver handling i `AGSCharacter` returnere tidlig på den (utenom det
  som skal virke på en PC). Sett i tillegg `SetIgnoreMoveInput(true)`/`SetIgnoreLookInput(true)` på nytt
  i `AcknowledgePossession` når `bMenuOpen` er sann.

## 5. LAV: `ClearAllMappings()` i `SetupInputComponent`
- `GSPlayerController.cpp:125` fjerner **alle** inputkontekster for den lokale spilleren, ikke bare
  Kamps. Det skader ingenting i dag: ingen andre legger til kontekster, `DefaultInput.ini` har ingen
  `DefaultMappingContexts`, og malfilene `Content/Input/IMC_Default`/`IMC_MouseLook` (ikke i git) brukes
  ikke. Men en kontekst fra GSMenu eller en plugin ville forsvinne uten varsel.
- **Rettelse:** `Sub->RemoveMappingContext(MappingContext); Sub->AddMappingContext(MappingContext, 0);`.

## 6. LAV: lyder lastes midt i spillet (små hakk)
- `GSAudio::Get` (`GSAudio.cpp:44-58`) lagrer lydene med svake pekere og laster dem med `LoadObject`
  første gang de brukes. Loggen har 13 synkrone lastinger (`FlushAsyncLoading … 1 QueuedPackages`) mellom
  21:53:25 og 21:53:42. Det er ikke sikkert at alle er lyder, men lyder som ryddes bort av
  minnerydderen, lastes på nytt og hakker igjen.
- **Rettelse:** sterke referanser (en `UPROPERTY`-liste i et subsystem eller `TStrongObjectPtr`), og
  last skudd, treff, fottrinn og hjerteslag på forhånd i `BeginPlay`.

---

## Sjekket og funnet i orden
- **Mapping-kontekst:** lages én gang (`EnsureInputCreated`) og legges til med prioritet 0 i
  `SetupInputComponent`, med alle 23 handlinger. WASD med swizzle/negate er riktig, hjulet ligger på
  `MouseWheelAxis` (Axis1D) og menyen på Esc og F10.
- **Bindinger:** `GSCharacter::SetupPlayerInputComponent` (368-404) binder alle handlingene til figuren,
  også `ScrollAction` (Triggered, `Scroll` med oppsamling av hjulutslag). Kontrolleren binder poengtavle,
  inventar, klar og meny. Ingen handling er glemt.
- **ServerFire/ServerFireFast:** begge er deklarert (`GSCharacter.h:295` Reliable, `297` Unreliable) og
  implementert (`GSCharacter.cpp:879, 884`), begge går til `HandleFire`, og `FireOnce` velger riktig
  (864-865).
- **Possess:** `AGameModeBase` kaller `RestartPlayer` (vår override, 480-505) →
  `RestartPlayerAtTransform`. `StartMatch` og `RequestTeam` gjør UnPossess, Destroy og RestartPlayer.
  `ClientRestart` → `PawnClientRestart` → `SetupPlayerInputComponent` på den nye figuren.
  `SetPawn`/`AcknowledgePossession` rydder dødskameraet.
- **Ingen tilstand låser bevegelse feil:** `Move` stoppes bare av PC-terminal og `bDead`. `MaxWalkSpeed`
  (3613) er aldri 0: `Mods.Speed` er 1 som standard, og beinskade gir minst 0,55. `DisableMovement`
  kjøres bare på den døde kroppen (`OnRep_Dead`, 3200). Lobbyen låser ikke bevegelse.
- **Kamera:** kameraet følger kontrollerens rotasjon. `CalcCamera` endrer bare noe når man er død.
  Visningen går tilbake til figuren etter PC-kamera og dødskamera.
- **Esc-menyen henger ikke fast:** `ToggleMenu` og `CloseMenu` er symmetriske, og tellerne nullstilles.
  GSMenus `SyncModal` lukker Kamps meny før den åpner sin egen (`GSMenu.cpp:666`). PC-terminalen
  lukkes med Esc/F10 eller E, og når man går bort, dør eller blir skutt (`PlayerTick` 497-501).
- **GSMenus tastebytte skader ikke Kamps kontekst:** den endrer bare `.Key` på Kamps egne mappinger etter
  indeks (`GSMenu.cpp:406, 422`), så modifikatorene (WASD) blir stående. Konflikter byttes, det finnes
  ingen UnmapAll, og `RequestRebuildControlMappings` kjøres. Saved keys legges på én gang per kontekst
  (1789-1803). Esc, F10, H og akser er satt av.
- **Bildefrekvenstaket:** 60 i editoren (loggen sier «max tick rate 60») og tas av i `EndPlay`.

## Hva Andreas skal teste når GSMenu er rettet
1. Trykk Play, velg «Øv alene» med musa, og sjekk at du kan gå (WASD) og se deg om (mus).
2. Trykk **F10** (ikke Esc i editoren): menyen kommer. Klikk «Fortsett»: du kan gå og se igjen.
3. Hjulet bytter våpen, I og hjulet blar i sekken, og E ved en PC åpner den (E lukker).
