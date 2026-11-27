# Plan 01a (lag Bots): bots som spiller Capture the Flag i Helgasetr

Første halvdel av den arvede plan-01 (Spillbarhets plan-06), skrevet om etter kontrollørens forhåndssjekk
(`kontroll/forhand-plan-01.md`, BØR ENDRES). 01a = navnett, bots lages, syn, sikte og skyting, roller.
01b (`ko/plan-01b.md`) = dekning, medisin, hørsel, deling, eskorte og ytelsesmåling. 01b bygger på 01a.

## Krav før byggeren starter (sjekk, ellers stopp og meld fra til dirigenten)
1. **Kamps K1 er levert og godkjent** (bygges nå av Kamp, ikke committet da planen ble skrevet). Sjekk i HEAD:
   - `AGSCharacter::IsLocalHuman()` finnes (public), og `friend class AGSBotController;` står i `AGSCharacter`
     (gir tilgang til beskyttede `Camera`, `ServerLastFireTime`, `StartFire`/`StopFire`/`StartAim`/`StopAim`/`Reload`/
     `SelectSlot`/`StartSprint`/`StopSprint`/`ToggleCrouch`/`ServerUseItem`/`ServerCancelUse`/`ServerInteract`/`QuickMelee`).
   - **Sikteretning for AI:** `FireOnce` sikter med `GetControlRotation()` for en bot, og `Tick` setter
     `Camera->SetWorldRotation(GetControlRotation())` når figuren er lokalt styrt men ikke menneske. (Kontrollørens feil C:
     uten dette skyter botene vannrett.) Står det ikke i HEAD: stopp. **Ikke** legg inn en egen kamerafiks i boten.
   - Ingen førstepersonskropp for AI: `SetupLocalBody` og `Tick`-grenen med `UpdateFocusPickup`/`UpdateLocalVisuals`
     krever `IsLocalHuman()`; `MulticastShotFX`/`PelletFX`/`MeleeFX` returnerer tidlig bare for `IsLocalHuman()`;
     `ClientTookHit`/`ClientHitMarker`/`ClientToast` returnerer for `!IsLocalHuman()`.
   - Kamps kontrollør har godkjent runden (se `.claude/loop/logg.md`).
2. **Grafikks Helgasetr 4a er bygget:** `Source/Gloshaugen/Public/GSHelgasetr.h` finnes og `GSCampus::bHelgasetr` er sann.
   (Botene har ingen koordinater i koden, men selvtesten og målene i «Ferdig når» gjelder Helgasetr.)

## Mål
Andreas trykker «Spill mot bots», og datastyrte soldater går gjennom Helgasetr på ulike ruter, angriper flagget hans,
forsvarer sitt eget og skyter tilbake – opp og ned trapper og over atriet – som mennesker med ulik ferdighet, ikke som
sikteroboter, og uten å se gjennom vegger.

## Forbedrer (gammelt)
- Plan-05s bot-kamp (598–605): i dag står du alene på kartet. Nå lages botene fra valgene `BotAllies`/`BotEnemies`/`BotSkill`
  i `AGSGameState`, og kampen fylles med motstandere og lagkamerater.
- Flaggets radiomelding «Flagget sist sett» (`AGSFlag::bRevealed`/`RevealLocation`) og flagghendelsene brukes av botene på
  samme måte som av mennesker – de får akkurat det et menneske får, aldri mer.
- Gjenoppliving, lag og spawn ved egen base (`RestartPlayer`, `OnPlayerKilled`, `StartMatch`) gjelder nå også datastyrte
  spillere, uten endring i `GSGameMode`.

## Nytt
- `UGSBotsSubsystem` lager botene i bot-kamp; `AGSBotController` styrer en vanlig `AGSCharacter` gjennom de samme handlingene
  som en spiller; `AGSBotTeam` fordeler roller (angripe, forsvare, hente eget flagg, bære hjem).
- Navigasjonsnett laget i kode for kartet som bygges i kode, med selvtest per base og per døråpning.
- Syn uten veggsyn, reaksjonstid, siktefeil som krymper, egen rekyl, skuddlinje-sjekk mot fast glass og lagkamerater.
- Fire vanskelighetsgrader (Lett, Middels, Vanskelig, Ekspert) som bare skiller på ferdighet.

## Hvorfor nå
Andreas vil ha «Spill mot bots» i hovedmenyen, og menyen (plan-05) står klar med bot-siden. Alle lag trenger bots for å
teste kart, våpen og regler alene. Kamps K1 er nå i bygging, og Helgasetr 4a gir kartet.

## Eierskap (bare lagets filer)
- **Nye (Bots):** `Public/GSBots.h`, `Private/GSBots.cpp` (subsystem, ferdighetstabell, navn, konsollkommandoer),
  `Public/GSNavSetup.h`, `Private/GSNavSetup.cpp` (navnett, klar-sjekk, selvtest, via- og forsvarspunkter),
  `Public/GSBotController.h`, `Private/GSBotController.cpp`, `Public/GSBotTeam.h`, `Private/GSBotTeam.cpp`.
- **Felles, små tillegg:** `Gloshaugen.Build.cs` (`"AIModule", "NavigationSystem", "GameplayTasks"` i
  `PublicDependencyModuleNames`), `Config/DefaultEngine.ini` (egne seksjoner nederst), `IMPROVEMENTS.md` 678–693.
- **Ingen endring** i `GSGameMode.*`, `GSGameState.*`, `GSMenu.*`, `GSHelp.*`, `GSPlayerState.*` (Spillbarhet),
  `GSCharacter.*` (Kamp), `GSDoor.*`/`GSPhysicsProp.*`/`GSBreakable.*`/`GSMapState.*` (Interaktiv), `GSHelgasetr*`/`GSCampus.h`
  (Grafikk). Det som trengs der står i bestillingene under; runden virker uten dem.

## Bestillinger (skrevet i `bestillinger.md`, ingen av dem stopper 01a)
- **Spillbarhet (små):** (1) `bBotsAvailable = true` i `GSGameState.h` når 01a er godkjent (vi melder fra i loggen) – til
  da tester vi med konsollkommandoen `gs.bots.fill`; (2) lagbytte i bot-kamp: `RequestTeam` i lobbyen flytter mennesket, og
  én bot på det nye laget flytter motsatt vei (finn den med `PS->IsABot()`); (3) `GSHelp`s lobby-panel: «du + N mot M (grad)»;
  (4) knappen «Vis navnett» i test-menyen som kaller `GSNavSetup::EnsureNavMesh(World)` og konsollen `show Navigation`.
- **Interaktiv verden:** dørblad, fysikkmøbler, skår og løse biter `SetCanEverAffectNavigation(false)` (ellers bygges
  fliser om hvert bilde mens de beveger seg); låst dør: `UNavModifierComponent` med `NavArea_Null`; knusbart glass beholder
  taggen `Mat_Glass`; en offentlig `bool AGSDoor::IsOpen() const`. Møbler som kan flyttes står utenfor navnettet.
- **Grafikk og kart:** `GSHelgasetr::NavBoundsM()` (P1 til like under P5); `DoorSlots()` med midtpunkt og retning (normal)
  for selvtesten; ønske: `BotSpots()` (forsvars-/utkikkspunkter).

## Viktig fra motorkoden (UE 5.8, sjekket av kontrolløren 27. nov.)
- `ANavMeshBoundsVolume` kan spawnes i spillet: utsatt spawn, `UBodySetup` med én `FKBoxElem`, `BrushBodySetup`,
  `FinishSpawning`, `OnNavigationBoundsUpdated`. `UBrushComponent::CalcBounds` bruker da `AggGeom` (`BrushComponent.cpp`
  l. 459–485). `bWholeWorldNavigable` er «broken» – ikke bruk.
- ISM-klossbygget registreres i nav-oktreet og blir nav-relevant når det blokkerer `Pawn`. Glassrekkverket og
  `AtriumBlockers` kommer med (riktig). Recast runder `AgentMaxStepHeight` 45 ned til 40 cm: sittetrinnene 0,45 m er ikke
  gangbare (riktig), trappetrinn ≤ 0,20 m er det.
- Recast runder agentradius **opp** til hele celler: radius 30 = 3 celler gir 40 cm stripe gjennom en 1,0 m dør; 36 ville
  gitt 20 cm og kan forsvinne. Derfor `AgentRadius=30`.
- `IsLocallyControlled()` er sann for AI på serveren. **Ikke** overstyr `IsLocalController()` i botkontrolleren:
  `CharacterMovementComponent` flytter bare lokalt styrte figurer.
- `APawn::AddControllerPitchInput` gjør ingenting for AI → boten lager rekylen selv. K1 hopper over `AddWeaponKick` for bots.
- `DetachFromControllerPendingDestroy` i `Die` frigjør kontrolleren, så `OnPlayerKilled` → `RestartPlayer` virker for bots.
- `AGSFlag::ServerStep` tar/erobrer ved berøring (`TouchRadius` 130 cm) – ingen egen flaggkode.
- Bot-kamp er alltid `NM_Standalone` (plan-05): ingen nettrisiko i denne runden.
- `AGSGameMode::PostLogin` setter menneskets lag og `bReady` straks; lobbyen starter nedtelling når alle i `PlayerArray` er
  klare (`Ready == Total`). Botene må derfor få `bReady = true` før de kommer i `PlayerArray` teller (sett det rett etter spawn).
- `StartMatch` finner alle kontrollere via `PlayerArray` → `GetOwningController()`, river kroppene og kaller `RestartPlayer`.
  Botene trenger ingen egen kode for ny kamp. `bSoloMatch` er alltid usann i bot-kamp (l. 271), så bots i `PlayerArray`
  sender ikke kampen tilbake til lobbyen.

## Steg

### 1. Oppsett: moduler og ini
`Gloshaugen.Build.cs`: legg til de tre modulene. `Config/DefaultEngine.ini` nederst:
```
[/Script/NavigationSystem.NavigationSystemV1]
bAutoCreateNavigationData=True
bAllowClientSideNavigation=False
DataGatheringMode=Lazy
+SupportedAgents=(Name="Soldat",Color=(R=140,G=255,B=0,A=164),DefaultQueryExtent=(X=50,Y=50,Z=250),NavDataClass=/Script/NavigationSystem.RecastNavMesh,AgentRadius=30,AgentHeight=184,AgentStepHeight=45,NavWalkingSearchHeightScale=0.5)

[/Script/NavigationSystem.RecastNavMesh]
RuntimeGeneration=Dynamic
CellSize=10
CellHeight=10
TileSizeUU=1600
AgentRadius=30
AgentMaxSlope=44
AgentMaxStepHeight=45
bDrawPolyEdges=True
```
Botene registrerer seg **ikke** som nav-invokers (ingen flisbygging under spill). Crowd-manageren tåler 50 agenter som
standard (nok for 19 bots).

### 2. Navnettet (`GSNavSetup.h/.cpp`, `namespace GSNavSetup`)
1. `void EnsureNavMesh(UWorld* World)` – kalles av `UGSBotsSubsystem` i bot-kamp (steg 3) og senere av «Vis navnett».
   Gjør ingenting andre gang (husk volumet i en `TWeakObjectPtr`).
   - `UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World)`; mangler den:
     `FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::GameMode)` og hent på nytt; logg begge.
   - **Grenser:** `GSHelgasetr::NavBoundsM()` (bestilt). Byggeren søker etter den i `GSHelgasetr.h` før bygging: finnes den,
     brukes den; ellers brukes reserven alene (bytt når den kommer). **Reserve:** `GSCampus::PlayArea()`
     i X/Y × Z −1…+17,5 m (P1 til like under P5-dekket). Ikke ta med P5–P6, glasstaket eller asfalten ute. × 100 til cm.
   - Spawn `ANavMeshBoundsVolume` utsatt (`SpawnActorDeferred`) i boksens midte; `UBodySetup* BS = NewObject<UBodySetup>(Volume)`,
     `BS->AggGeom.BoxElems.Add(FKBoxElem(Størrelse.X, Størrelse.Y, Størrelse.Z))`, `BS->CollisionTraceFlag = CTF_UseSimpleAsComplex`,
     `BS->CreatePhysicsMeshes()`; `Volume->GetBrushComponent()->BrushBodySetup = BS`; `FinishSpawning`;
     `Volume->GetBrushComponent()->UpdateBounds()`; `Nav->OnNavigationBoundsUpdated(Volume)`. Logg boksen (m).
2. **Klar-sjekk** (timer hver 0,5 s, inntil 20 s): `!Nav->IsNavigationBuildInProgress()` og `GetNavMeshTilesCount() > 0`
   på `ARecastNavMesh` → klar; logg «Navnett klart: N fliser, T s». Etter 20 s uten fliser: logg «Navnett: INGEN FLISER» og la
   botene stå og forsvare (ingen invoker-reserve – kontrollørens grensevei er bekreftet i motoren).
   `bool IsNavReady(const UWorld*)`.
3. `void NotifyMapChanged(UWorld*, const FBox& BoxCm)` → `Nav->AddDirtyArea(BoxCm, ENavigationDirtyFlag::All)` hvis nettet
   finnes. (Dynamisk generering gjør nettet skittent selv når en nav-relevant komponent fjernes; denne er for spesialtilfeller.)
4. **Projisering:** én hjelper `bool ToNav(UWorld*, FVector In, FVector& Out)` med utstrekning 100×100×250 cm. Alt som
   projiseres, starter på **gulv** (flaggfot, spawnpunkt, stipunkt) – aldri i lufta (midtpunktet mellom flaggene ligger midt i
   atriet, 14 m over kantina).
5. **Selvtest ved første klar-melding** (bare logg, ingen spillvirkning):
   - For hvert lag: sti fra hvert spawnpunkt (`GSCampus::SpawnPoints`/basens punkter) til motstanderens flagg
     (`FindPathToLocationSynchronously`). Logg lengde i m og antall punkter, og forskjellen rød→blå mot blå→rød.
     «INGEN STI fra X til Y» når den mangler.
   - **Per døråpning:** når `GSHelgasetr::DoorSlots()` finnes: for hver dør, projiser punkter 0,8 m på hver side langs
     dørens normal (uten normal: prøv ±X og ±Y og bruk paret som begge projiseres) og finn sti mellom dem. Logg
     «dør <id>: sti ja (L m)» eller «dør <id>: sti NEI» (sti lengre enn 4 m regnes som nei – den går rundt). Til slutt
     «dører: A av B gangbare». Nyttig for Grafikk og Interaktiv.
6. **Via-punkter (ruter uten koordinater):** for hver side, start fra **flagget projisert til nettet** og fra 2 av lagets
   spawnpunkter projisert til nettet (aldri midtpunktet). Fra hvert startpunkt: 40 × `GetRandomReachablePointInRadius`
   med radius = 0,8 × flagg–flagg-avstanden. Behold punkter som (a) ligger mellom 25 % og 75 % langs linja flagg→flagg i
   2D, (b) er minst 8 m fra begge flagg, (c) har Z under 17 m (ikke P5+), og (d) har sti fra begge flaggene (sjekk maks 40
   kandidater med `FindPathToLocationSynchronously`, spredt over flere bilder: 4 per bilde). Grupper i etasje
   (`GSHelgasetr::LevelAt(Z/100)` når den finnes, ellers Z-bånd på 2,5 m) og side (venstre/høyre av linja). Velg høyst 12,
   minst 6 m fra hverandre, med minst ett punkt per etasje som har kandidater (så R5 gjennom atriet og R6 i kantina
   kommer med). Logg «via-punkter: N (P1: a, P2: b, P3: c, P4: d; vest x, øst y)».
7. **Forsvarspunkter per base:** 24 nåbare punkter 4–14 m fra eget flagg (fra flagget projisert). Behold de som ser flagget
   (objektspor `ECC_WorldStatic` fra 150 cm over punktet til 100 cm over flaggfoten; komponenter med tag `Mat_Glass` hoppes
   over, sporet gjentas fra treffpunktet, maks 3 ganger) og har en vegg innen 1,2 m bak eller til siden (4 korte spor).
   Rangér etter hvor mange av inngangene de ser (innganger = der stiene fra via-punktene går inn i 12 m-sirkelen rundt
   flagget). Behold 6. Bruk `GSHelgasetr::BotSpots()` i stedet når den finnes. Logg antallet per base.

### 3. `UGSBotsSubsystem` og ferdigheter (`GSBots.h/.cpp`)
1. `UCLASS() class UGSBotsSubsystem : public UWorldSubsystem`. `ShouldCreateSubsystem`: bare spillverdener
   (`WorldType == Game || PIE`). `OnWorldBeginPlay`: start en timer hver 0,25 s (`Step`). Gjør ingenting hvis
   `GetNetMode() != NM_Standalone` eller `GS->GameKind != GSGameKind::Bots` (sjekkes i `Step`, fordi `GameKind` settes av
   GameMode og verdensdelsystemenes rekkefølge ikke er fast). Kartet er bygget av `UGSWorldSubsystem` før første `Step`.
2. `Step` (bare i bot-kamp):
   - Første gang: `GSNavSetup::EnsureNavMesh(World)`.
   - Når mennesket står i `GS->PlayerArray` med lag (`PS->Team != GSTeam::None`, `!PS->IsABot()`): sørg for at antall bots på
     hans lag = `GS->BotAllies` og på det andre = `GS->BotEnemies` (bare i `Lobby`/`Countdown`; under `Playing` endres
     ingenting). Mangler det: lag én bot per `Step` (unngå hakk). For mange: fjern den siste (`Pawn->Destroy()`, `Bot->Destroy()`).
   - Lage én bot: `World->SpawnActor<AGSBotController>()` (`bWantsPlayerState = true` lager `AGSPlayerState` via GameMode),
     `PS->SetIsABot(true)`, `PS->SetPlayerName(Navn + TEXT(" (bot)"))`, `PS->Team = Lag`, `PS->bReady = true`,
     `Bot->Skill = GS->BotSkill`, `Bot->BotIndex = løpenummer`; så `GM->RestartPlayer(Bot)` (public; laget er satt, så den
     spawner ved egen base og gir startutstyr når kampen pågår).
   - Hver bot registreres i lagets `AGSBotTeam` (lages første gang, én per lag, se steg 5).
3. **Konsollkommandoer** (`static FAutoConsoleCommandWithWorldAndArgs`, alle bare i bot-kamp eller test):
   - `gs.bots.fill <venner> <fiender> [grad 0–3]` – setter `GS->BotAllies/BotEnemies/BotSkill` (så kan vi teste før
     `bBotsAvailable` er slått på). I test kartet: lager navnettet og bots uten kamp (står og forsvarer).
   - `gs.bots.debug 0/1` – tegner hver bots rolle, mål, sti og via-punktene (`DrawDebugLine/String`, ikke i Shipping).
   - `gs.bots.think 0/1` – slår av tenkingen (botene står stille); brukes til ytelsesmålingen i 01b.
4. **Ferdighetstabell** (`struct FGSBotSkill` i `GSBots.h`, én `static const` tabell, logges ved kampstart):

| | Lett | Middels | Vanskelig | Ekspert |
|---|---|---|---|---|
| `ReactMin–ReactMax` (s) | 0,9–1,3 | 0,55–0,8 | 0,35–0,5 | 0,22–0,32 |
| `TurnDegPerSec` | 120 | 200 | 320 | 450 |
| `AimErrorDeg` (start) | 4,0 | 2,5 | 1,4 | 0,8 |
| `AimSettleHalfLife` (s) | 0,9 | 0,6 | 0,4 | 0,3 |
| `LeadError` | 0,6 | 0,4 | 0,2 | 0,1 |
| `RecoilControl` | 0,3 | 0,5 | 0,7 | 0,85 |
| `HeadChance` | 0,03 | 0,08 | 0,15 | 0,25 |
| `ViewFovDeg` / `SightM` | 100 / 35 | 110 / 45 | 120 / 55 | 120 / 60 |
| `BurstShortM` | 8 | 12 | 15 | 18 |
| `HearM` (01b) | 25 | 35 | 45 | 50 |
| `CoverUse` (01b) | 0,2 | 0,5 | 0,8 | 0,95 |
| `RetreatHealth` (01b) | 0 | 30 | 40 | 45 |

   Ingen grad får mer helse, skade eller treffsikkerhet enn våpenet gir. **Ingen nattfaktor** (Helgasetr er innendørs med lys
   på; mørke knyttes til lysbryterne senere). 01b-feltene ligger i tabellen nå, så 01b ikke endrer headeren.
5. **Navn:** 30 vanlige norske fornavn («Ola», «Kari», «Ingrid», «Magnus», «Sigrid», «Jonas», …), ingen like i samme kamp.

### 4. `AGSBotController` (`GSBotController.h/.cpp`)
Konstruktør: `ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent"))`,
`bWantsPlayerState = true`, `bSetControlRotationFromPawnOrientation = false`. `OnPossess`: hent `AGSCharacter`,
`ClearFocus(EAIFocusPriority::Gameplay)` (vi styrer rotasjonen selv), `GetCharacterMovement()->bUseRVOAvoidance = false`.
Velg sterkeste våpenslot (gevær før pistol) med `SelectSlot`. `Tick` i trinn, forskjøvet med `BotIndex`:
**syn 5 Hz, beslutning 4 Hz, sikte og bevegelse hvert bilde.** Alt bare på serveren (`HasAuthority`). Mål tenketiden med
`SCOPE_CYCLE_COUNTER(STAT_GSBotThink)` (egen `STATGROUP_GSBots`) og logg snittet hvert 30. s (UE_LOG, så hodeløse kjøringer
viser tallet).
1. **Hukommelse (ingen veggsyn):** `TMap<TWeakObjectPtr<AGSCharacter>, FBotSeen>` (sist sett sted, tid, synlig nå, frie
   punkter). Boten vet bare det den har sett, og det et menneske på laget får: fiendens flaggbærer når eget flagg har
   `bRevealed` (`RevealLocation`), og flagghendelsene (`State`, `DropLocation` på flaggene – samme som HUD-en viser).
   Aldri posisjonen til en fiende den ikke har sett.
2. **Syn (5 Hz):** levende fiender innen `SightM` og innenfor `ViewFovDeg` (360° innen 3 m): spor fra kameraet
   (`Char->Camera->GetComponentLocation()`) til hode, bryst og hofte. `ECC_Visibility`, hopper over komponenter med tag
   `Mat_Glass` (sporet gjentas fra treffpunktet, maks 3 ganger), ignorerer egen figur og lagkamerater. Synlig = minst ett
   punkt fritt; husk hvilke. Felles budsjett i `AGSBotTeam`: maks 48 synsspor per bilde for alle bots; resten neste bilde.
3. **Sikte (hvert bilde):** mål = synlig fiende med høyest trussel (nærmest, ser mot meg, bærer flagget ×2). Siktepunkt =
   bryst, eller hode (`HeadChance`, og alltid hode når bare hodet er fritt), pluss forsprang `fart × flytid × (1 − LeadError·tilfeldig)`
   (flytid fra katalogens kulefart). Feil: ved første syn trekkes en feilvektor (`AimErrorDeg`, tilfeldig retning) som halveres
   hver `AimSettleHalfLife`; ny halv feil når målet skifter retning brått (> 300 cm/s). Kontrollrotasjonen dreies mot
   `Mål + Feil` med høyst `TurnDegPerSec` (`FMath::RInterpConstantTo`), **både yaw og pitch**, `SetControlRotation` hvert bilde.
   K1 sørger for at kameraet og skuddet følger pitch. Uten mål: se langs stien (neste stipunkt, 1,6 m over gulvet), med rolige
   blikk til sidene i åpne rom (±25°, hvert 2–4 s).
4. **Skuddlinje (før hvert skudd og hver salve):** `ECC_Visibility`-spor fra kameraet til siktepunktet **uten** å hoppe over
   glass, ignorer egen figur. Skyt bare hvis første treff er målet (eller en annen fiende). Er første treff fast glass, en
   vegg eller en lagkamerat: ikke skyt; flytt 1–2 m til siden (projisert punkt med fri linje, høyst 4 kandidater) eller vent.
   (Rekkverksglasset ignorerer `Visibility` til Interaktiv gjør det knusbart – der går både syn og kuler gjennom, konsistent.)
5. **Skyte:** bare når (a) reaksjonstiden fra første syn er gått, (b) vinkelen mellom sikte og siktepunkt < 3° (8° under 6 m),
   (c) skuddlinja er fri (4). Under `BurstShortM`: hold `StartFire` til målet er borte eller magasinet tomt; over: 3–5 skudd,
   pause 0,3–0,6 s; halvautomatisk/repetér: ett skudd per `FireInterval × 1,3–2,0`. `StartAim` over 12 m når boten står eller går.
   **Rekyl:** når `Ammo` synker, legg `RecoilPitch × (1 + 0,05 × skudd i salven)` til kontrollrotasjonens pitch og trekk
   `RecoilControl` av den ned igjen over 0,15 s. Spredningen kommer fra Kamps `GetCurrentSpread()` uendret. Tomt magasin →
   `Reload()`. Mister målet: ikke skyt på huskede steder, men sikt dit i 2 s.
6. **Bevege seg:** `MoveToLocation(Mål, 80 cm, bUsePathfinding=true, bAllowStrafe=true)`. Sprint når ingen fiende er sett siste
   4 s, stien er > 15 m og blikket er innen 30° av fartsretningen; ellers gå. **Sitter fast** (ønsket fart, men < 50 cm på
   1,5 s): (a) når en `AGSDoor` finnes innen 1,5 m i fartsretningen og den ikke er åpen → `ServerInteract()` (skrevet som
   «når `AGSDoor` finnes»: `TActorIterator`/overlapp på klassen hvis den er kompilert inn; til den er bygget hoppes steget
   over); (b) ellers hopp én gang; (c) fortsatt fast etter 3 s: velg nytt via-punkt og logg «sitter fast ved (x, y, z), plan N».
7. **Ammo:** et `AGSPickup` med ammo til eget våpen innen 3 m fra stien → gå innom og `ServerInteract()`. Mer loot er plan-02.

### 5. `AGSBotTeam` (`GSBotTeam.h/.cpp`, én per lag, bare på serveren, `AInfo`)
Hvert 2. s, og straks ved flagghendelse (les flaggenes `State`/`Carrier` hvert 0,25 s og se etter endring): tell lagets bots
`N` og mennesker `H` (fra `PlayerArray`).
- **Forsvar:** `N + H <= 2` → 0; 3–5 → 1; 6–8 → 2; 9+ → 3. Velg blant botene nærmest eget flagg. Forsvareren går til et
  tildelt forsvarspunkt, huker (`ToggleCrouch`), ser mot inngangen punktet ser best, og bytter punkt etter 30–60 s eller
  når en fiende har sett den der.
- **Hent eget flagg:** eget flagg ikke hjemme → halvparten av angriperne (minst 1) går til sist kjente sted
  (`DropLocation` når det ligger, ellers `RevealLocation` når `bRevealed`, ellers eget flagg-område). Berøring sender det hjem
  (finnes i `AGSFlag`).
- **Bær hjem:** en bot som bærer fiendens flagg (`PS->CarriedFlagTeam`) går hjem via et annet via-punkt enn det den kom inn
  med (helst motsatt side eller etasje).
- **Angrip:** resten, fordelt på ulike via-punkter (høyst 2 på samme), nytt valg ved hver ny tur inn. Stien slutter på
  fiendens flaggplass.
- Felles sporbudsjett (syn 48 per bilde) ligger her.

## Ferdig når
- [ ] Kravene øverst er sjekket og står i rapporten (K1-commit og 4a-commit).
- [ ] Bygger uten feil. `Saved/Logs/Gloshaugen.log` (bot-kamp, `gs.bots.fill 4 5 1`) viser: navnettet klart (fliser > 0, tid),
      selvtesten med sti fra alle spawnpunkter til motstanderens flagg for begge lag (lengder, forskjell), «dører: A av B
      gangbare» når `DoorSlots()` finnes, via-punkter fordelt på minst to etasjer og begge sider, 6 forsvarspunkter per base,
      og botenes tenketid hvert 30. s. Ingen «INGEN STI» mellom spawn og flagg.
- [ ] **PIE, du + 4 mot 5, Middels:** botene løper ut på minst to ulike ruter; minst én blir igjen ved eget flagg. En bot tar
      flagget ditt innen ca. 3 min hvis du ikke forsvarer, og bærer det hjem en annen vei (erobring telles).
- [ ] **Sikte i høyden:** en bot på P4 treffer en spiller på P2 over atriet, og en bot i trappa treffer en spiller på reposet
      over (prøves med `gs.bots.debug 1`; skriv resultatet i rapporten).
- [ ] **Fast glass:** en bot bak et vindu/glassvegg som blokkerer kuler skyter ikke; den flytter seg eller venter.
- [ ] Botene skyter ikke før reaksjonstiden; på 30 m bommer Lett og Middels ofte, Ekspert sjelden. Ingen bot sikter mot deg
      gjennom betong; ingen vet hvor du er når du har vært usett lenge.
- [ ] Bots dør, blir liggende og gjenoppstår ved egen base; drapslista og poengtavla viser «Navn (bot)». Lagkamerat-bot skyter
      ikke når du står i skuddlinja.
- [ ] Verten hører botenes skudd i 3D der de står (K1) og har ingen førstepersonskropp for bots (`stat anim` viser ikke dobbel
      animasjon per bot).
- [ ] Ingen endring i andre lags filer (`git diff --stat` viser bare lagets filer, `Gloshaugen.Build.cs`, `DefaultEngine.ini`,
      `IMPROVEMENTS.md`).

## Risiko
- **K1 ikke levert eller uten AI-sikte:** botene skyter vannrett og verten hører feil lyder → stopp før bygging (krav 1).
- **Navnettet tomt:** loggen viser «INGEN FLISER»; sjekk at kartet er bygget før `EnsureNavMesh` (timer i subsystemet) og at
  grensene er riktige. Lazy innsamling kan ta noen sekunder; lobbyen i bot-kamp er 5–15 s, og botene står og forsvarer til
  nettet er klart.
- **`bAutoCreateNavigationData` i online og test:** lager en tom `RecastNavMesh` uten grenser – ingen fliser, liten kostnad.
  Mål oppstartstid i online før/etter; > 0,5 s lengre → `bAutoCreateNavigationData=False` og lag navdata i `EnsureNavMesh`.
- **Smale dører:** radius 30 gir 40 cm stripe; selvtesten viser hvilke dører som ikke er gangbare (meldes Grafikk/Interaktiv).
- **Dører og møbler som lager hull i nettet** før Interaktiv har levert bestillingen: flisene bygges om når de beveger seg
  (ytelse). Mål `stat navigation` når dører/møbler finnes.
- **Fysikkmøbler:** står utenfor navnettet (bestilt). Botene dytter dem eller går rundt; «sitter fast»-regelen tar resten.
- **Crowd-unnvikelse** virker bare mellom bots; mot mennesker glir CharacterMovement. To bots i samme dør: «sitter fast».
- **Juks/realisme:** botene går gjennom de samme handlingene og serversjekkene som spillere, får ingen informasjon et menneske
  ikke får, og endrer aldri felt i `AGSCharacter` direkte.

## IMPROVEMENTS-linjer (seksjon «Runde 1a (Bots): bots som spiller Capture the Flag»)
678. Bots: datastyrte soldater spiller Capture the Flag med deg og mot deg i Helgasetr, i samme kropp og med samme våpen, skade og blod som spillere
679. «Spill mot bots» fyller nå lagene med så mange bots du valgte på bot-siden, med vanskelighetsgraden du valgte
680. Botene finner veien gjennom ganger, rom, trapper og etasjer med et navigasjonsnett som lages når kartet bygges
681. Botene velger ulike ruter mot flagget: vest- og østkorridoren, rommene i fløyene, gjennom atriet og nede i kantina
682. Noen bots blir igjen og forsvarer eget flagg fra steder med utsyn mot dørene inn til baserommet, og bytter plass etter en stund
683. Botene tar flagget ditt og bærer det hjem en annen vei enn de kom, og henter sitt eget flagg tilbake når det er mistet
684. Botene ser bare det de faktisk ser: synsfelt, synsvidde og vegger teller, og de ser gjennom glass
685. Botene husker bare fiender de har sett, og vet om flaggbæreren bare når laget får den samme radiomeldingen som du
686. Botene trenger reaksjonstid før første skudd, og siktet blir bedre jo lenger de har sett deg
687. Botene sikter opp og ned: de treffer i trapper, mellom etasjer og tvers over atriet
688. Botene skyter salver på avstand, holder avtrekkeren inne på nært hold og må holde rekylen nede selv
689. Botene skyter ikke i fast glass de ikke kan skyte gjennom, og ikke når en lagkamerat står i skuddlinja – de flytter seg i stedet
690. Fire vanskelighetsgrader – Lett, Middels, Vanskelig og Ekspert – skiller bare på ferdighet, aldri på helse eller skade
691. Botene heter vanlige norske navn merket «(bot)» i drapslista og på poengtavla
692. Bots som står fast hopper og velger en annen rute, og plukker opp ammo til våpenet sitt når den ligger rett ved veien
693. Navnettet sjekker seg selv ved kampstart: veien mellom basene og gjennom hver døråpning skrives i loggen, og `gs.bots.debug` tegner botenes ruter
