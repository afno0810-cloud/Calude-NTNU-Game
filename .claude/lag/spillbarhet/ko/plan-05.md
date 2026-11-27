# Plan 05 – runde 5 (lag Spillbarhet): ny hovedmeny – Spill mot bots, Spill online, Test kartet

Andreas 27. nov.: «Lag bedre menysystem etter også. Det skal ha spill mot bots, spill online og test mappet-funksjon.»
Bygges videre på `GSMenu` (tegning, krok, sider, dialoger og lagring finnes). Ingenting startes på nytt.
Bots kommer i plan-06; denne runden lager menyen, oppstartsveien og innstillingene de skal lese, og hele «Test kartet».

## Mål
Når spillet starter, velger Andreas mellom tre tydelige ting – **Spill mot bots**, **Spill online** og **Test kartet** –
og hvert valg starter riktig slags spill med én knapp, uten konsoll og uten å vente i en lobby.

## Forbedrer (gammelt)
- Hovedmenyens venstre liste (`DrawMain`): «Start spill / Bli med / Øv alene» blir tre klare valg + Innstillinger,
  Kontroller, Avslutt. Vert og Bli med samles på én side «Spill online».
- Vertsvalg som før var faste tall (`AGSGameState::MatchMinutes` 15, `CaptureLimit` 3, venneild av) kan nå velges
  for både bot-kamper og online-kamper (gammel backlog 4b, «vertsvalg»).
- Esc-/F10-/P-menyen inne i spillet viser knapper som passer det du holder på med (online, bots, test).
- «Øv alene» (åpnet bare menyen) erstattes av en ekte lokal kamp eller testmodus.

## Nytt
- «Test kartet»: gå fritt i kartet uten lobby og uten skade, fri flyging (gjennom vegger) av/på med N, flytt deg til
  basene, still klokke og vær for å se lyset – alt fra menyen.
- Side «Spill mot bots»: ditt lag, bots på ditt lag, bots på motstanderlaget, vanskelighetsgrad, kamplengde,
  erobringer, venneild, startutstyr – lagres og sendes til serveren som valg i adressen.
- Spillet står på pause når menyen er åpen i bot-kamp og test (aldri online).

## Hvorfor nå
Andreas ba om det i dag, og bots (plan-06) og det nye Helgasetr-kartet trenger en vei inn: Grafikk og Andreas må kunne
gå rundt i Helgasetr uten kamp for å se på kartet, og bots trenger en meny som starter kampen og sier hvor mange.

## Filer
- Endres: `GSMenu.cpp`, `GSMenu.h` (én ny funksjon), `GSGameMode.h/.cpp`, `GSGameState.h/.cpp`, `GSHelp.cpp`.
- Nye: `GSExplore.h`, `GSExplore.cpp` (Spillbarhet eier; føres inn i tabellen i `LAG.md` av dirigenten).
- Felles, lite tillegg: `GSTypes.h` – én ny verdi **nederst** i `EGSMatchPhase`: `Explore`. `IMPROVEMENTS.md` 598–613.
- Ikke endret: Kamps filer. (Én bestilling: HUD-ens «først til %d» skal lese `CaptureGoal`; til den er levert står det 3.)

## Bakgrunn fra koden (lest 27. nov.)
- `GSMenu.cpp`: `EPage { Root, Host, Join, Connecting, Settings, Controls }`, `DrawMain` (l. 1149) bygger venstrelista
  fra `bMatch = InMatch(World, S)` (NM_ListenServer eller klient med nettdriver). `HandleClick` (l. 1454), `DialogYes`
  (l. 1415), `OnKey` (l. 1580), tegning per bilde (l. 1900–2050). Hovedmenyen åpner seg selv bare i `NM_Standalone`, ikke i
  PIE, og ikke etter `bMainDismissed` (nulles per verden i `ResetWorld`).
- Vert: `AGSPlayerController::Host()` = `OpenLevel(samme kart, true, "listen")`. «Forlat» = `ReloadAlone` (`OpenLevel` uten valg).
- `AGSGameMode::Tick`: Lobby → Countdown (alle klare, eller 90 s etter spiller to) → Playing (`MatchMinutes`) → Ended → Lobby.
  `StartMatch` jevner lagene (±1). `DamageEnabled()` = `Phase == Playing`, og Kamps `TakeDamage` og `TickBleeding`
  spør den. Sult/tørst i `TickSurvival` går **ikke** gjennom `DamageEnabled` (setter `Health` og kan kalle `Die("Sult")`).
- Flagget gjør ingenting utenfor `Playing` (`AGSFlag::ServerStep`). Kamps HUD (`DrawMatch`) tegner lobby-boksen for
  `Lobby`/`Countdown`, klokka for `Playing`, resultatet for `Ended` og **ingenting** for andre verdier.
- Kamps HUD l. 1082 bruker `AGSGameState::CaptureLimit` (konstant). `GSHelp` leser også fasen (l. 431, 588, 710–730).

## Steg

### 1. Spillets art i `AGSGameState` (vår fil) og ny fase
1. `GSTypes.h`: legg `Explore` nederst i `EGSMatchPhase` (etter `Ended`). Ingenting annet i fila.
2. `GSGameState.h`: ny `namespace GSGameKind { constexpr uint8 Online = 0; constexpr uint8 Bots = 1; constexpr uint8 Explore = 2; }`
   og replikerte felt (alle settes bare av GameMode):
   `UPROPERTY(Replicated) uint8 GameKind = 0;` · `uint8 CaptureGoal = 3;` · `uint8 MatchMinutesSet = 15;` ·
   `uint8 BotSkill = 1;` (0 Lett, 1 Middels, 2 Vanskelig, 3 Ekspert) · `uint8 BotAllies = 0;` · `uint8 BotEnemies = 0;` ·
   `bool bStartKit = false;`. Legg dem til i `GetLifetimeReplicatedProps`. `CaptureLimit` og `MatchMinutes` blir stående
   som standardverdier (Kamp leser `CaptureLimit`); kommentar: «standard, se CaptureGoal/MatchMinutesSet».
3. Ny hjelper `bool IsLocalSolo() const { return GameKind != GSGameKind::Online; }` (bots og test er alltid alene på maskinen).

### 2. `AGSGameMode`: lese valgene fra adressen
1. Overstyr `InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)` (kall `Super` først) og
   lagre valgene i medlemmer (`PendingKind`, `PendingCaps`, …). Les med `UGameplayStatics::ParseOption`/`GetIntOption`:
   `Mode` (`Bots` / `Explore`, ellers Online), `Caps` (klem 1–10, standard 3), `Min` (klem 3–60, standard 15), `FF` (0/1),
   `Skill` (0–3), `Allies` (0–9), `Enemies` (1–10 i bots, ellers 0), `Team` (0 tilfeldig, 1 rød, 2 blå), `Kit` (0/1).
   **Alt klemmes på serveren** (lærdom: det klienten velger kan velges for å jukse – gjelder også en vert som får
   adressen fra en annen). `Mode=Bots`/`Explore` godtas bare når `GetNetMode() == NM_Standalone`; ellers Online.
2. I `BeginPlay`, før `SpawnPickups`: skriv valgene inn i `AGSGameState` (`GameKind`, `CaptureGoal`, `MatchMinutesSet`,
   `bFriendlyFire`, `BotSkill`, `BotAllies`, `BotEnemies`, `bStartKit`). Explore: `GS->Phase = EGSMatchPhase::Explore`.
3. Bruk innstillingene: `StartMatch` bruker `GS->MatchMinutesSet * 60` i stedet for `MatchMinutes`; `Tick` (Playing)
   bruker `GS->CaptureGoal` i stedet for `CaptureLimit`. `AnyDecisiveFlagAway` er uendret.
4. `Tick`: `if (GS->Phase == EGSMatchPhase::Explore) return;` øverst (ingen lobby, ingen nedtelling).
5. `DamageEnabled()` er uendret (Explore ≠ Playing → ingen skade, ingen blødning).
6. **Bot-kamp uten lobby-venting** (`GameKind == Bots`):
   - Når den første (eneste) menneskespilleren er inne (`PostLogin`), sett laget hans fra `Team` (0 = tilfeldig) og
     `bReady = true`, og start nedtelling på **5 s** (ny konstant `BotCountdownSeconds = 5`; `StartCountdown` får en
     varighet som parameter med standard `CountdownSeconds`).
   - `StartMatch`: hopp over lagjevningen (steg 2 i funksjonen) i bot-kamp – lagene er satt av menyen (og av plan-06).
     `bSoloMatch` settes ikke i bot-kamp.
   - Etter resultatet (`Ended` → `BackToLobby`): i bot-kamp er lobbyen 15 s, eller kortere når spilleren trykker Klar;
     så nedtelling 5 s. (Lobby-regelen «alle klare» virker allerede når bots i plan-06 er `bReady`.)
7. **Startutstyr** (`bStartKit`, standard på i bot-kamp, av i online): i `RestartPlayer` etter `RestartPlayerAtTransform`,
   når fasen er `Playing` eller `Countdown`: gi et nøkternt sett med Kamps offentlige `GiveWeapon`/`AddToBackpack`:
   ett gevær fra katalogen (første `Rifle`-klasse med `Rarity <= Uncommon`, valgt én gang per kamp så alle har det samme),
   én pistol, 2 bandasjer. Finn Id-ene ved oppslag i `GSItems` (ikke faste navn), og logg hva som ble valgt. Samme sett til
   mennesker og (fra plan-06) bots – rettferdig. Loot ligger som før.
8. **Explore**: i `RestartPlayer` gjelder ingen lagvalg; bruk `GSTeam::Red` for spawn (rød base), og logg
   «Test kartet: startet ved rød base». Lag `AGSExplore` (steg 4) i `BeginPlay` når `GameKind == Explore`.
   Flaggene settes ut som før (så man ser basene), men gjør ingenting (fasen er ikke `Playing`).

### 3. `GSMenu`: tre valg, sider og menyen inne i spillet
1. `EPage` får `Bots`, `Online`, `Explore` (behold `Host`/`Join` som underdeler av `Online`, se under).
   Ny hjelper `uint8 LocalKind(const UWorld* W)`: `GS ? GS->GameKind : Online` når `NetMode == NM_Standalone`, ellers Online.
   `bool InLocalGame(W, S) = NetMode == NM_Standalone && LocalKind(W) != Online`.
2. **Venstrelista i `DrawMain`** – tre tilstander:
   - Startmenyen (ikke `InMatch`, ikke `InLocalGame`): **Spill mot bots**, **Spill online**, **Test kartet**, (mellomrom
     12 px), Innstillinger, Kontroller, Avslutt. De tre første litt høyere knapper (BH + 6) med én dimmet linje under
     hver i høyre panel når siden åpnes (ingen ekstra tekst i lista).
   - I bot-kamp: Tilbake til kampen · Start kampen på nytt · Endre kampen … (åpner bot-siden med knappen
     «Start på nytt med disse») · Innstillinger · Kontroller · Til hovedmenyen · Avslutt.
   - I test: Tilbake · Fri flyging: av/på (viser tasten, `KeyName("GS_Fly")`) · Gå til … · Lys og vær … ·
     Innstillinger · Kontroller · Til hovedmenyen · Avslutt.
   - Online-kamp: som i dag (Tilbake, Ny kamp som vert, Bli med i en annen, …), men «Ny kamp som vert» og «Bli med»
     peker til `EPage::Online`.
3. **Side «Spill mot bots»** (`EPage::Bots`), med `Choice(...)` som Innstillinger-siden:
   | Rad | Verdier | Standard | Lagres som |
   |---|---|---|---|
   | Ditt lag | Tilfeldig / Rødt / Blått | Tilfeldig | `BotTeam` |
   | Bots på ditt lag | 0–9 | 4 | `BotAllies` |
   | Bots på motstanderlaget | 1–10 | 5 | `BotEnemies` |
   | Vanskelighetsgrad | Lett / Middels / Vanskelig / Ekspert | Middels | `BotSkill` |
   | Kamplengde | 5 / 10 / 15 / 20 / 30 min | 10 | `BotMinutes` |
   | Erobringer for å vinne | 1 / 2 / 3 / 5 | 3 | `BotCaps` |
   | Venneild | Av / På | Av | `BotFF` |
   | Startutstyr | Gevær, pistol og bandasjer / Bare det du finner | Gevær … | `BotKit` |
   Under radene, dimmet: én linje per vanskelighetsgrad (f.eks. Middels: «Reagerer som en vanlig spiller, bommer
   ofte på lang avstand.»), og «Lagene: du + 4 mot 5». Knappen **«Start kampen»** (stor, aksent) bygger
   `Mode=Bots?Team=…?Allies=…?Enemies=…?Skill=…?Min=…?Caps=…?FF=…?Kit=…` og kaller
   `UGameplayStatics::OpenLevel(PC, Kart, true, Valg)` (kartnavnet som `ReloadAlone`). Lagre i `[Gloshaugen]` i
   `GGameUserSettingsIni` med en gang en verdi endres (`CfgFlush` som før). I en online-kamp spør dialogen
   `EDialog::Host` først («Kampen du er i nå avsluttes»), som i dag.
   **Før plan-06 er bygget** finnes ingen bots: siden viser da (dimmet) «Bots kommer i neste oppdatering – nå spiller du
   alene på kartet.» Avgjøres med `GSMenu`-konstanten `bBotsAvailable` (false nå; plan-06 setter den til true).
4. **Side «Spill online»** (`EPage::Online`): to deler under hverandre (ingen faner – færre klikk):
   - **«Start som vert»**: adressene som i dag (`LocalAddresses`, port, brannmur-tekst), og tre valg: Kamplengde,
     Erobringer, Venneild (lagres som `HostMinutes`, `HostCaps`, `HostFF`). Knappen bygger `listen?Min=…?Caps=…?FF=…` og
     kaller `OpenLevel` (ikke Kamps `Host()`, som ikke tar valg).
   - **«Bli med»**: adressefeltet, «Koble til», «Siste: …» og forklaringen som i dag (`TryJoin` uendret).
   - Er vinduet for lavt (< 620 px innhold), vises «Bli med» først og vertsdelen bare som knappen «Start som vert …»
     som åpner den gamle `EPage::Host`-visningen. `EPage::Connecting` er uendret.
5. **Side «Test kartet»** (`EPage::Explore`) fra startmenyen: én kort forklaring («Gå fritt rundt i kartet. Ingen kamp,
   ingen skade. N slår fri flyging av og på.») og knappen **«Gå inn i kartet»** → `OpenLevel(…, "Mode=Explore")`.
6. **Sider i test** (`EPage::Goto`, `EPage::Light` – to små sider):
   - «Gå til»: én knapp per base (`GSCampus::Bases()`: «Rød base – flagget», «Rød base – startplassen», samme for blå;
     navnene fra `GSTeam::Name`), og «Der du sto sist» (forrige teleportering). Klikk → `GSExplore::Teleport(PC, Mål)`.
     Ingen koordinater i menyen: alt fra kartdataene, så det virker når kartet byttes.
   - «Lys og vær»: Klokke (`Choice`: 06, 08, 10, 12, 14, 16, 18, 20, 22, 00) og Vær (Klart, Skyet, Tåke, Regn, Snø) →
     `GSExplore::SetClock(PC, Timer)` / `SetWeather(PC, EGSWeather)`. Viser dagens verdi fra `GS->GetTimeOfDayHours()`/`Weather`.
7. **Til hovedmenyen** (bots og test): dialog `EDialog::Leave` med teksten «Tilbake til hovedmenyen? Kampen avsluttes.»
   (test: «Avslutte testen?»), så `ReloadAlone` (som før, laster kartet uten valg). Sett først `S.bOpenMainAfterTravel = true`
   (nytt felt i **per vindu**-delen av `FState`, overlever kartbytte); steg 5 i tegningen åpner da hovedmenyen også i PIE
   og nuller flagget. «Start kampen på nytt» = samme `OpenLevel` med valgene fra ini (dialog først).
8. **Startmenyen åpner seg selv** bare når `LocalKind == Online` og `NM_Standalone` (bakgrunnsverdenen uten valg), som i
   dag, eller når `bOpenMainAfterTravel`. Den roterende kameraturen (l. 2027) bare i bakgrunnsverdenen.
9. **Pause**: i `SyncModal`, når `InLocalGame` (bots eller test): `UGameplayStatics::SetGamePaused(World, bMainOpen || Dialog)`
   (husk egen tilstand `bPausedByUs`, slå av igjen bare det vi slo på). Aldri online. Teksten øverst i høyre panel
   sier «Spillet står på pause» i bot-kamp.
10. **Fri flyging-tast**: ny egen rad i tastelista som «GS_Team»: `GS_Fly` («Fri flyging (test kartet)»), standard **N**,
    kan endres, lagres med samme mekanisme (`TeamRowId`-mønsteret: `GS_Fly.N`). I `OnKey` (ikke-modal, `IE_Pressed`):
    `Key == FlyKey() && LocalKind == Explore && !PC->bMenuOpen && !PC->IsOnTerminal()` → `GSExplore::ToggleFly(PC)`,
    returner `true`. Utenfor test går N videre til spillet som før.
11. Ny offentlig funksjon i `GSMenu.h`: `GLOSHAUGEN_API FKey KeyFor(const APlayerController* PC, FName Action);`
    (tasten en handling ligger på nå, samme oppslag som `KeyName`) – `GSExplore` bruker den for hopp/huk/løp.
12. Logg én linje per valg (`GSMenu: Spill mot bots startes (du + 4 mot 5, Middels, 10 min, 3 erobringer)`,
    `GSMenu: Test kartet startes`, `GSMenu: vert startes (15 min, 3 erobringer, venneild av)`).

### 4. `GSExplore.h/.cpp` – «Test kartet»
`UCLASS() class AGSExplore : public AInfo` (finnes bare på maskinen som spiller, Explore er alltid `NM_Standalone`).
Tick hvert bilde (`PrimaryActorTick.bCanEverTick = true`, også under pause: `SetTickableWhenPaused(false)` – pause betyr
stillstand). Statiske inngangsfunksjoner i `namespace GSExplore` som finner aktøren med `TActorIterator`:
1. `ToggleFly(APlayerController*)`:
   - **På**: figuren (`AGSCharacter`) får `SetActorEnableCollision(false)`, `GetCharacterMovement()->SetMovementMode(MOVE_Flying)`,
     `MaxFlySpeed = 700` (1800 med løpetasten), `BrakingDecelerationFlying = 3000`. Husk siste trygge stå-sted.
   - Mens på (Tick): les tastene med `PC->IsInputKeyDown(GSMenu::KeyFor(PC, "IA_Jump"))` (opp) og
     `KeyFor(PC, "IA_Crouch")` + venstre Ctrl (ned), og kall `Char->AddMovementInput(FVector::UpVector, ±1)`. Fram/bak/sider
     kommer fra Kamps vanlige gange (`Move`). Står bevegelsesmodus ikke lenger på `MOVE_Flying` (Kamps klatring/glid kan sette
     den), sett den tilbake – maks én gang per 0,2 s, og logg første gang.
   - Ingen menyer åpne: ikke noe input når `GSMenu::IsModal(PC)` (spillet står uansett på pause da).
   - **Av**: finn en trygg plass: kapselprøve (`World->OverlapBlockingTestByChannel` med figurens kapsel, `ECC_Pawn`) der
     du er; er den fri, spor ned (objektspor `ECC_WorldStatic`, maks 30 m) og sett figuren på gulvet. Er den ikke fri (inne i
     en vegg): prøv 8 retninger × 0,5–3 m, så opp 0,5–3 m; ellers tilbake til siste trygge stå-sted. Så
     `SetActorEnableCollision(true)` og `SetMovementMode(MOVE_Falling)`.
   - Kort melding nederst (via `GSHelp`, se steg 5): «Fri flyging på – Mellomrom opp, C ned, N av» (taster fra `KeyName`).
2. `Teleport(PC, FVector Mål)`: slå av flyging først (samme trygg-plass-logikk), `TeleportTo(Mål + 1 m opp, Rot)`, og
   faller tilbake på stedet før hvis `TeleportTo` sier nei. Husk stedet før («Der du sto sist»).
3. `SetClock(PC, float Timer)`: `GS->StartHour = Timer; GS->ClockStartedAt = GS->GetServerWorldTimeSeconds();`.
   `SetWeather(PC, EGSWeather W)`: `GS->PreviousWeather = GS->Weather = W; GS->WeatherChangedAt = Nå − WeatherBlendSeconds;`
   og skyv neste værbytte langt fram (ny offentlig `AGSGameState::HoldWeather(float Seconds)` som setter `NextWeatherChange`).
   Sjekk at `GSEnvironment` (Grafikk) leser disse feltene hvert bilde (den gjør det i dag via `GetTimeOfDayHours`/`Weather`).
4. **Ingen skade, ingen sult**: hvert 5. s fyll `Hunger = Thirst = 100` på figuren (offentlige felt) – sult/tørst går utenom
   `DamageEnabled`. Skade og blødning er alt av (fasen er ikke `Playing`).
5. Tester man ødeleggelse: kuler og granater virker som før (glass, vegger fra Grafikks runde 2 går i stykker – det er
   poenget med testen). Våpen kan plukkes opp.
6. Logg: `GSExplore: fri flyging på/av (satt ned ved X)`, `GSExplore: flyttet til …`, `GSExplore: klokke 18, vær Regn`.

### 5. `GSHelp`: tekstene i de nye modusene
1. Test kartet (`Phase == Explore`): ingen lobby-panel, ingen CTF-hint. Lite panel øverst til venstre:
   «TEST KARTET · ingen kamp, ingen skade» og to linjer: «N: fri flyging (av)» / «F10 eller P: meny» (tastene fra
   `KeyName`; «Esc» i stedet for F10 når det ikke er editoren – bruk samme `IsEditorPlay`-regel som menyen). Når flyging er
   på: tredje linje «Mellomrom opp · C ned · Shift fort». H-oversikten virker som før.
2. Bot-kamp: lobby-panelet sier «Kamp mot bots: du + N mot M, Middels. Starter om X s.» i stedet for «venter på spillere»,
   og CTF-reglene står som før. Statusstripa og «Flagget sist sett» er uendret.
3. «først til N»-tekster i `GSHelp` leser `GS->CaptureGoal`, ikke `CaptureLimit`.

## Ferdig når
- [ ] Bygger uten feil og advarsler i våre filer.
- [ ] **I editorens Play (PIE)**: F10/P åpner menyen; lista viser de tre valgene øverst. «Test kartet» → «Gå inn i
      kartet» laster kartet, figuren står ved rød base, ingen lobby-boks, panelet «TEST KARTET» synes. N slår fri flyging
      på (gjennom vegger, Mellomrom opp, C ned), N igjen setter deg ned på et gulv, aldri inne i en vegg. «Gå til» flytter
      til alle fire punktene. Klokke 22 gjør det mørkt, Regn gir regn ute. Skudd skader ingen; sult står på 100.
- [ ] «Til hovedmenyen» fra test i PIE: kartet lastes på nytt og hovedmenyen står åpen (`bOpenMainAfterTravel`).
- [ ] «Spill mot bots» → «Start kampen»: kartet lastes, nedtelling 5 s, kampen starter med valgt lengde (klokka viser
      10:00 med standard), du står på valgt lag ved egen base med gevær, pistol og to bandasjer. Menyen setter spillet på
      pause (klokka står stille). Erobring nr. 3 (eller valgt antall) avslutter kampen.
- [ ] «Spill online» → «Start som vert» med 5 min og 1 erobring: en annen PIE-klient (Number of Players 2, Play As
      Listen Server) ser klokka 5:00 og «først til 1» i `GSHelp`. Online settes aldri på pause.
- [ ] Explore/Bots kan ikke startes fra en klient eller listen-server (`Mode` ignoreres, logg sier hvorfor).
- [ ] Ugyldige tall i adressen (`Caps=999`, `Allies=-4`) klemmes, ingen krasj.
- [ ] Tastelista har «Fri flyging (test kartet)» på N; kan endres og lagres; P og H kan ikke velges.
- [ ] Logglinjene i steg 3.12 og 4.6 står i `Saved/Logs/Gloshaugen.log` (les kommandolinja: PIE eller `-game`).

## Risiko
- **Pause og input**: når spillet er på pause må menyen fortsatt få klikk og taster (krokene er på viewporten og går
  utenom pausen – sjekk i PIE) og `SyncModal` må slå pausen av før input slippes; ellers står spillet fast på pause uten meny.
  `bPausedByUs` hindrer at vi slår av en pause noen andre satte.
- **Flyging mot Kamps bevegelseskode**: klatring, glid og gripekrok kan sette egen bevegelsesmodus. Vi setter `MOVE_Flying`
  tilbake (høyst 5 ganger i sekundet) og logger det; skjer det stadig, bestill en `bNoClip`-sjekk hos Kamp.
- **Kollisjon av** betyr at kuler fra andre ikke treffer (ingen andre i test). Kamps fottrinn/kamerahopp ved flyging kan
  se rart ut – kosmetisk.
- `EGSMatchPhase::Explore` i en `switch` uten `default` hos andre lag gir ingen feil i MSVC (C4062 er av), men sjekk at
  ingen `check`/`ensure` krever en av de fire gamle verdiene (søk `EGSMatchPhase::` i alle filer før bygging).
- `ParseOption` er case-insensitiv på nøkkelen, men verdiene (`Bots`/`Explore`) sammenlignes med `Equals(…, IgnoreCase)`.
- Kartbytte i PIE (`OpenLevel`) virker (brukt av `ReloadAlone` i dag), men viewport-krokene må sjekkes på nytt etter
  kartbyttet (håndtaket sjekkes hvert bilde i dag).

## IMPROVEMENTS-linjer (seksjon «Runde 5 (Spillbarhet): ny hovedmeny – bots, online og test kartet»)
598. Hovedmenyen har tre tydelige valg øverst: Spill mot bots, Spill online og Test kartet, og under dem Innstillinger, Kontroller og Avslutt
599. «Spill online» samler vert og bli med på én side, med adressene vennene skal skrive inn og feltet for å koble til
600. Som vert velger du kamplengde, antall erobringer og venneild før kampen starter, og alle som kobler til får de samme reglene
601. «Spill mot bots» har egen side: ditt lag, antall bots på hvert lag, vanskelighetsgrad, kamplengde, erobringer, venneild og startutstyr
602. Valgene på bot-siden og vertssiden huskes til neste gang du starter spillet
603. En kamp mot bots starter med én knapp: kartet lastes, fem sekunders nedtelling, og du står på valgt lag uten å vente i lobbyen
604. Med startutstyr får alle (også botene) det samme geværet, en pistol og to bandasjer ved hver start
605. Kampens lengde og antall erobringer for å vinne følger det du valgte, ikke faste 15 minutter og 3 erobringer
606. «Test kartet»: gå fritt rundt i hele kartet uten lobby, uten kamp og uten å ta skade eller bli sulten
607. Fri flyging i test kartet: N slår det av og på, Mellomrom flyr opp og C ned, og du går gjennom vegger
608. Når du slår av flygingen, settes du alltid ned på et gulv, aldri fast inne i en vegg
609. I test kartet kan du flytte deg rett til flagget og startplassen til hvert lag fra menyen
610. I test kartet kan du stille klokka og været fra menyen for å se kartet i morgenlys, kveld, regn eller tåke
611. Menyen inne i spillet viser knapper som passer det du gjør: start kampen på nytt og endre bots i bot-kamp, flyging, flytting og lys i test
612. Spillet står stille på pause mens menyen er åpen i bot-kamp og test kartet (aldri i online-kamp)
613. «Til hovedmenyen» tar deg tilbake til startmenyen, også i editorens Play, og tasten for fri flyging kan endres i Kontroller
