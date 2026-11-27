# Plan 06 – runde 6 (lag Spillbarhet): bots som spiller Capture the Flag

Bygges etter plan-05 (menyen, `GameKind`, `BotAllies/BotEnemies/BotSkill`, startutstyr). Taktikken som gjør dem gode
(eskorte, flanker, dører, granater, loot, undertrykkende ild) er plan-07 (skisse). Denne runden: bots som går i bygget,
tar og forsvarer flagg, skyter som mennesker (reaksjonstid, spredning, rekyl), søker dekning, tar medisin, og har fire
vanskelighetsgrader – i samme `AGSCharacter` som spillerne, så skade, blod, kropp og animasjoner er like.

**Kontrolløren forhåndssjekker denne planen før bygging** (lærdom: store planer forhåndssjekkes).

## Mål
Andreas trykker «Spill mot bots», og et fullt lag med datastyrte soldater går gjennom Helgasetr, angriper flagget hans,
forsvarer sitt eget og skyter tilbake som mennesker med ulik ferdighet – ikke som sikteroboter.

## Forbedrer (gammelt)
- Plan-05s bot-side starter nå en ekte kamp mot bots (i dag: alene på kartet; `bBotsAvailable = true`).
- Lagvalg i lobbyen i bot-kamp: bytter du lag (L), bytter en bot plass med deg, så lagstørrelsene står (`RequestTeam`).
- Poengtavla, drapslista, «Flagget sist sett» og lobby-lista viser bots som vanlige spillere merket «(bot)».
- `StartMatch`/`RestartPlayer`/`OnPlayerKilled` virker for datastyrte kontrollere (gjenoppliving, lag, spawn ved base).

## Nytt
- `AGSBotController` (AIController) som styrer en vanlig `AGSCharacter` gjennom de samme handlingene en spiller har.
- Navigasjonsnett (NavMesh) som lages **i kode ved oppstart** for kartet som bygges i kode – virker for både gamle
  Gløshaugen og Helgasetr uten faste koordinater.
- Lagets hjerne (`AGSBotTeam`): angripere, forsvarere, flagghentere; roller fordeles på nytt hvert 2. s.
- Syn og hørsel uten veggsyn, reaksjonstid, siktefeil som krymper, rekyl som må holdes nede, dekning, medisin.
- Fire vanskelighetsgrader (Lett, Middels, Vanskelig, Ekspert) – samme kropp og våpen, bare ferdighet skiller.

## Hvorfor nå
Andreas: «spill mot bots». CTF krever folk; Andreas tester alene. Bots gjør også at alle tre lag kan teste kart, våpen og
regler uten venner.

## Viktig fra motorkoden (UE 5.8, lest 27. nov.)
- **NavMesh i et kart bygget i kode:** sett `RuntimeGeneration=Dynamic` (DefaultEngine.ini). Navigasjonssystemet lager da
  selv en `ARecastNavMesh` ved oppstart (`SpawnMissingNavigationDataInLevel` hopper bare over `Static` uten grenser).
  Grensene: `ANavMeshBoundsVolume` kan spawnes i spillet, men størrelsen kommer fra penselen, og utenfor editoren regner
  `UBrushComponent::CalcBounds` bare med `BrushBodySetup->AggGeom` (`BrushComponent.cpp` l. 459–485) – penselbyggerne
  (`UCubeBuilder`) finnes bare i editoren. **Derfor:** spawn volumet utsatt (`SpawnActorDeferred`), lag en `UBodySetup` med
  én `FKBoxElem` i riktig størrelse, sett `GetBrushComponent()->BrushBodySetup` (offentlig felt), `FinishSpawning`, og kall
  `NavSys->OnNavigationBoundsUpdated(Volume)` (offentlig, `NavigationSystem.h` l. 1096).
  **Reserve** hvis volumet ikke gir fliser (logg: 0 polygoner etter 10 s): `NavSys->RegisterInvoker(*Anker, Radius, Radius+500,
  FNavAgentSelector(), ENavigationInvokerPriority::Default)` for en usynlig aktør midt i kartet med radius som dekker
  `PlayArea`, og `bGenerateNavigationOnlyAroundNavigationInvokers=True` i ini. Byggeren prøver volumet først.
- **`IsLocallyControlled()` er sann for en AI-styrt figur på serveren.** Kamps kode bruker den mange steder for «det er
  meg» (2D-lyder, førstepersonsting, `UpdateLocalVisuals`, `OnRep_Health`, skuddeffekter som hoppes over i
  `MulticastShotFX`). `Client`-RPC-er (`ClientTookHit`, `ClientHitMarker`, `ClientToast`) på en figur uten klientforbindelse
  kjøres **på serveren**. Uten en krok hos Kamp hører verten botenes skudd som sine egne 2D-skudd, hører deres hjerteslag,
  og ser ikke munningsflammen deres. → Bestilling K1 (se under). Selve skytingen (`FireOnce` → `ServerFire` → `HandleFire`)
  virker for AI uendret: `FireOnce` sikter med kameraet, og kameraet følger kontrollrotasjonen (`bUsePawnControlRotation`).
- `APawn::AddControllerPitchInput` gjør ingenting for en AIController, så Kamps `TickRecoil` gir botene **ingen** rekyl.
  Botkontrolleren lager rekylen selv fra våpendataene (`FGSWeaponStats::RecoilPitch`, vår katalog).
- `DetachFromControllerPendingDestroy` i `Die` frigjør kontrolleren, så `OnPlayerKilled`s gjenoppliving (`RestartPlayer`
  når `!C->GetPawn()`) virker for bots uten endring.
- `AGSFlag::ServerStep` tar/erobrer på avstand (`TouchRadius` 130 cm) på serveren – bots trenger ingen egen flaggkode.

## Bestillinger (skrevet i `bestillinger.md`)
- **Kamp K1 (må til før botene er gode å spille mot; uten den virker alt, men verten hører feil lyder):**
  (a) `bool IsLocalHuman() const { return IsLocallyControlled() && IsPlayerControlled(); }` i `AGSCharacter` (public) og bruk
  den i stedet for `IsLocallyControlled()` der det betyr «spilleren bak skjermen»: 2D-lyd og lokale effekter i `FireOnce`
  (skuddlyd, `FireShown`, munningsflamme ved førstepersonsvåpenet, hylser, rekyl), tidlig retur i `MulticastShotFX`/
  `MulticastPelletFX`/`MulticastMeleeFX`, `bOwnView` i sårhendelsen, `OnRep_Health`, førstepersonsbein/-kropp (`FPLegs`,
  14a sin `FPBody`/`SetupLocalBody`), `Tick`-grenen med `UpdateFocusPickup`/`UpdateLocalVisuals`, spøkelsesflimmer og
  tredjepersonsvåpen (`!IsLocallyControlled()` → `!IsLocalHuman()`). **Behold** `IsLocallyControlled()` i `FireOnce`s
  inngang og i `HandleFire` (der betyr den «skuddet kom fra denne maskinen»). (b) Første linje i `ClientTookHit_Implementation`,
  `ClientHitMarker_Implementation`, `ClientToast_Implementation`: `if (!IsLocalHuman()) return;`. (c) `friend class
  AGSBotController;` i `AGSCharacter`, så boten kan bruke de beskyttede handlingene (`StartFire`, `StopFire`, `StartAim`,
  `StopAim`, `Reload`, `SelectSlot`, `StartSprint`, `StopSprint`, `ToggleCrouch`, `ServerUseItem`, `ServerCancelUse`,
  `ServerInteract`, `QuickMelee`) og lese `ServerLastFireTime`. Botkontrolleren endrer aldri felt direkte, bare kaller
  handlingene. (d) Sjekk 14a/14b: alt som lages «bare for den lokale spilleren» skal kreve `IsLocalHuman()`.
- **Kamp K2:** HUD-ens «først til %d» (`GSHUD.cpp` l. 1082) leser `GS->CaptureGoal` (fra plan-05).
- **Grafikk G1:** `GSCampus::NavBoundsM()` → `FBox` i meter som dekker alt gangbart (Helgasetr: Plan 1–4 med trapperom;
  gamle campus: `PlayArea` × bakke til øverste etasje). Til den finnes: `PlayArea()` × Z −15 … +45 m.
- **Grafikk G2:** dører, fysikkmøbler, skår og løse biter `SetCanEverAffectNavigation(false)` (bots åpner dører selv i
  plan-07; flyttbare ting skal ikke lage hull i nettet). Faste ting (vegger, rekkverk, skranker, faste bord) påvirker nettet.
- **Grafikk G3:** når `AGSMapState` ødelegger noe som endrer hvor man kan gå (lettvegg, dør), kall
  `GSBots::NotifyMapChanged(World, BoxCm)` (én linje; gjør ingenting uten bots).
- **Grafikk G4 (ønske, ikke krav):** `GSCampus::BotSpots()` – navngitte punkter (forsvar, utkikk, dekning) i kartdataene.
  Botene lager egne punkter av navnettet når listen er tom.

## Filer
- Nye (Spillbarhet): `Public/GSBots.h`, `Private/GSBots.cpp` (navnett, `AGSBotTeam`, ferdighetstabell, navn),
  `Public/GSBotController.h`, `Private/GSBotController.cpp`.
- Endres (våre): `GSGameMode.h/.cpp` (lage/fjerne bots, lagbytte i bot-kamp), `GSMenu.cpp` (`bBotsAvailable = true`,
  «Vis navnett» i test), `GSPlayerState.h/.cpp` (ingen nye felt: bruk `SetIsABot(true)`; `GSHelp`/meny viser «(bot)»), `GSHelp.cpp`.
- Felles, små tillegg: `Gloshaugen.Build.cs` (`"AIModule", "NavigationSystem", "GameplayTasks"` i `PublicDependencyModuleNames`),
  `Config/DefaultEngine.ini` (seksjonene under), `IMPROVEMENTS.md` 614–629.

## Steg

### 1. Oppsett: moduler og ini
`Gloshaugen.Build.cs`: legg de tre modulene til. `Config/DefaultEngine.ini` (nederst, egne seksjoner):
```
[/Script/NavigationSystem.NavigationSystemV1]
bAutoCreateNavigationData=True
bAllowClientSideNavigation=False
DataGatheringMode=Lazy
+SupportedAgents=(Name="Soldat",Color=(R=140,G=255,B=0,A=164),DefaultQueryExtent=(X=50,Y=50,Z=250),NavDataClass=/Script/NavigationSystem.RecastNavMesh,AgentRadius=36,AgentHeight=184,AgentStepHeight=45,NavWalkingSearchHeightScale=0.5)

[/Script/NavigationSystem.RecastNavMesh]
RuntimeGeneration=Dynamic
CellSize=10
CellHeight=10
TileSizeUU=1600
AgentMaxSlope=44
AgentMaxStepHeight=45
bDrawPolyEdges=True
```
Agentradius 36 (kapselen er 40): dører på 1,0 m må gi en sammenhengende stripe (100 − 72 = 28 cm); bevegelsen glir langs
karmen. Mål etter bygget: `GSBots` logger om det finnes sti gjennom hver base-dør (steg 2.4).

### 2. Navnettet (`GSBots.cpp`, `namespace GSBots`)
1. `void EnsureNavMesh(UWorld* World)` – kalles fra `AGSGameMode::BeginPlay` (etter at kartet er bygget) **bare** når
   `GameKind == Bots` (og fra test-menyens «Vis navnett»). Ingen kostnad i online-kamper.
   - `UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World)`; mangler den:
     `FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::GameMode)` og hent på nytt; logg begge.
   - Boks: `GSCampus::NavBoundsM()` hvis den finnes (G1), ellers `PlayArea()` × Z −15…+45 m, × 100 til cm.
   - Spawn `ANavMeshBoundsVolume` utsatt i boksens midte; `UBodySetup* BS = NewObject<UBodySetup>(Volume)`,
     `BS->AggGeom.BoxElems.Add(FKBoxElem(Størrelse.X, Størrelse.Y, Størrelse.Z))`, `BS->CollisionTraceFlag = CTF_UseSimpleAsComplex`,
     `BS->CreatePhysicsMeshes()`; `Volume->GetBrushComponent()->BrushBodySetup = BS`; `FinishSpawning`;
     `Volume->GetBrushComponent()->UpdateBounds()`; `Nav->OnNavigationBoundsUpdated(Volume)`.
   - Logg boksen og at volumet er sendt.
2. **Klar-sjekk** (timer hver 0,5 s i inntil 20 s): `Nav->IsNavigationBuildInProgress()` usann og
   `ARecastNavMesh::GetNavMeshTilesCount() > 0` → klar; logg antall fliser og tid. Etter 10 s uten fliser: reserven med
   invoker (se motorkoden over) og logg «reserve: invoker». `bool IsNavReady(World)` brukes av botene og av GameMode
   (nedtellingen i bot-kamp venter til nettet er klart, maks 20 s; deretter starter den uansett og botene står og
   forsvarer til nettet kommer).
3. `void NotifyMapChanged(UWorld*, const FBox& BoxCm)` → `Nav->AddDirtyArea(BoxCm, ENavigationDirtyFlag::All)` hvis nettet
   finnes (G3).
4. **Selvtest ved kampstart** (logg, ingen spillvirkning): sti fra hver base sine spawnpunkter til motstanderens flagg
   (`FindPathToLocationSynchronously`, punktene projisert med `ProjectPointToNavigation`, utstrekning 100×100×250 cm).
   Logg lengde i meter og antall punkter, og «INGEN STI» med start og mål når den mangler – det er nyttig for Grafikk
   (dører som er for smale, trapper uten forbindelse). Logg også forskjellen i lengde rød→blå mot blå→rød.
5. **Ruter uten koordinater (via-punkter):** ved kampstart, trekk 60 tilfeldige nåbare punkter
   (`GetRandomReachablePointInRadius` fra midtpunktet mellom flaggene, radius = halve diagonalen til boksen). Behold punkter
   der projeksjonen på linja flagg→flagg ligger mellom 25 % og 75 %, og som ikke ligger innen 8 m fra et flagg.
   Grupper dem i høyde (etasjer: avstand i Z > 2,5 m) og i side (venstre/høyre av linja); behold høyst 12, spredt
   (minst 6 m mellom). Et angrep velger ett via-punkt → ulike korridorer, trapper og etasjer uten at noe er skrevet inn
   for kartet. Byttes kartet, regnes alt på nytt.
6. **Forsvarspunkter per base (uten koordinater):** ved kampstart, 24 nåbare punkter 5–14 m fra eget flagg; behold de som
   ser flagget (spor fra 150 cm over punktet til 100 cm over flaggfoten, objektspor `ECC_WorldStatic`, glass teller som
   gjennomsiktig – se 4.2) og som har en vegg eller fast ting innen 1,2 m bak eller til siden (4 korte spor). Rangér etter
   antall innganger de ser (innganger = punktet der stiene fra via-punktene går inn i 12 m-sirkelen rundt flagget).
   Behold de 6 beste. G4-punktene brukes i stedet når de finnes.

### 3. Ferdighetstabell og navn (`GSBots.h`)
```cpp
struct FGSBotSkill {
    float ReactMin, ReactMax;      // s fra første syn til første skudd
    float TurnDegPerSec;           // hvor fort sikte flyttes
    float AimErrorDeg;             // startfeil når målet blir sett
    float AimSettleHalfLife;       // s til feilen er halvert
    float LeadError;               // 0..1 feil i forsprang på målet som beveger seg
    float RecoilControl;           // 0..1 andel av rekylen som trekkes ned
    float HeadChance;              // andel skudd mot hodet når hodet synes
    float ViewFovDeg, SightM;      // synsfelt og synsvidde (dagslys)
    float HearM;                   // hører skudd (ikke lyddempet) innen
    float BurstShortM;             // over denne avstanden: korte salver
    float CoverUse;                // 0..1 hvor ofte den søker dekning når den blir beskutt
    float RetreatHealth;           // trekker seg tilbake under denne helsa
};
```
| | Lett | Middels | Vanskelig | Ekspert |
|---|---|---|---|---|
| Reaksjon (s) | 0,9–1,3 | 0,55–0,8 | 0,35–0,5 | 0,22–0,32 |
| Sving (°/s) | 120 | 200 | 320 | 450 |
| Startfeil (°) | 4,0 | 2,5 | 1,4 | 0,8 |
| Halvering (s) | 0,9 | 0,6 | 0,4 | 0,3 |
| Forsprangfeil | 0,6 | 0,4 | 0,2 | 0,1 |
| Rekylkontroll | 0,3 | 0,5 | 0,7 | 0,85 |
| Hode | 0,03 | 0,08 | 0,15 | 0,25 |
| Synsfelt / vidde | 100° / 35 m | 110° / 45 m | 120° / 55 m | 120° / 60 m |
| Hører skudd | 25 m | 35 m | 45 m | 50 m |
| Korte salver over | 8 m | 12 m | 15 m | 18 m |
| Dekning | 0,2 | 0,5 | 0,8 | 0,95 |
| Trekker seg under | 0 | 30 HP | 40 HP | 45 HP |
Ingen grad får mer helse, skade eller treffsikkerhet enn våpenet gir. Navn: 30 vanlige norske fornavn i en liste
(«Ola», «Kari», «Ingrid», «Magnus», …); `PS->SetPlayerName(Navn + " (bot)")`, ingen like navn i samme kamp.

### 4. `AGSBotController` (`GSBotController.h/.cpp`)
Konstruktør: `ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent"))`,
`bWantsPlayerState = true`, `bSetControlRotationFromPawnOrientation = false`, `bStartAILogicOnPossess = true`.
`OnPossess`: hent `AGSCharacter`, `ClearFocus(EAIFocusPriority::Gameplay)` (vi styrer rotasjonen selv), sett
`GetCharacterMovement()->bUseRVOAvoidance = false` (crowd tar unnvikelsen). Tenker med `Tick` i trinn:
**sanser 5 Hz, beslutning 4 Hz, sikte og bevegelse hvert bilde**, forskjøvet med `BotIndex` så ikke alle tenker samme bilde.
1. **Hukommelse (ingen veggsyn):** `TMap<TWeakObjectPtr<AGSCharacter>, FBotSeen>` med sist sett sted, tid, om synlig nå.
   Boten vet bare det den har sett, hørt, eller fått på «radio»: motstanderflaggets bærer når laget hans får
   «Flagget sist sett» (`AGSFlag::bRevealed`/`RevealLocation` – samme melding menneskene får), og eget flagg tatt/mistet
   (`MulticastFlagEvent`). Aldri posisjonen til en fiende den ikke har sanset.
2. **Syn (5 Hz):** for hver levende fiende innen `SightM` (× 0,6 når `GS->GetTimeOfDayHours()` er 21–06 og fienden ikke har
   lommelykt på – `bFlashlightOn`) og innenfor `ViewFovDeg` (360° for fiender innen 3 m): spor fra øyet (kamera) til tre
   punkter på fienden (hode, bryst, hofte). Sporet bruker `ECC_Visibility`, hopper over komponenter med tag `Mat_Glass`
   (helt glass er gjennomsiktig; sporet gjentas fra treffpunktet, maks 3 ganger) og over egne lagkamerater. Synlig = minst
   ett punkt fritt; husk hvilke punkter (bryst/hode) som er frie. Budsjett: maks 48 synsspor per bilde for alle bots til
   sammen (felles teller i `AGSBotTeam`); det som ikke rekker, tas neste bilde.
3. **Hørsel:** for hver fiende innen `HearM` som har skutt siste 0,3 s (`ServerLastFireTime` via K1c, ikke lyddempet –
   lyddemper: 40 % av `HearM`): husk retning og omtrentlig sted (±3 m tilfeldig). Fiende som løper innen 10 m (fart > 450):
   husk som «hørt fottrinn».
4. **Sikte (hvert bilde):** mål = synlig fiende med høyest trussel (nærmest, sikter mot meg, bærer flagget ×2). Siktepunkt =
   bryst, eller hode (`HeadChance`, og alltid hode når bare hodet er fritt), pluss forsprang `fart × kulens flytid × (1 − LeadError·tilfeldig)`.
   Feil: ved første syn trekkes en feilvektor (`AimErrorDeg`, tilfeldig retning), som halveres hver `AimSettleHalfLife`; ny,
   halv feil når målet skifter retning brått (> 300 cm/s endring). Kontrollrotasjonen dreies mot `Mål + Feil` med høyst
   `TurnDegPerSec` (`FMath::RInterpConstantTo`). `SetControlRotation` hvert bilde. Uten mål: se langs stien (neste
   stipunkt, 1,6 m over gulvet), med små rolige blikk til sidene i åpne rom (±25°, hvert 2–4 s).
5. **Skyte:** bare når (a) reaksjonstiden fra første syn er gått, (b) vinkelen mellom sikte og siktepunkt < 3°
   (8° under 6 m), (c) skuddlinja er fri for lagkamerater (spor `ECC_Pawn` fra øyet 60 m; første figur er en lagkamerat → ikke
   skyt, flytt 1 m til siden). Salver: under `BurstShortM` – hold avtrekkeren til målet er borte eller magasinet tomt; over –
   3–5 skudd, pause 0,3–0,6 s; halvautomatisk/repetér: ett skudd per `FireInterval × 1,3–2,0`. Siktet (`StartAim`) over 12 m
   når boten står stille eller går. **Rekyl:** etter hvert skudd (registrert når `Ammo` synker) legg `RecoilPitch × mønster`
   til kontrollrotasjonen (samme klatring som Kamps `FireOnce`: ×(1 + 0,05 × skudd i salven)), og trekk `RecoilControl` av den
   ned igjen over 0,15 s. Spredningen kommer fra Kamps `GetCurrentSpread()` (bevegelse, sikte, skade i arm) – uendret.
   Tomt magasin → `Reload()` (dekning først hvis fienden er synlig og avstanden > 10 m). Mister målet: skyt ikke på huskede steder
   (det er plan-07: undertrykkende ild), men sikt dit i 2 s.
6. **Bevege seg:** `MoveToLocation(Mål, Aksept 80 cm, bUsePathfinding=true, bAllowStrafe=true)`. Sprint (`StartSprint`) når
   ingen fiende er sett siste 4 s, stien er lengre enn 15 m, og blikket er innen 30° av fartsretningen; ellers gå. Sitter fast
   (har ønsket fart, men har flyttet < 50 cm på 1,5 s): hopp én gang, så velg nytt via-punkt; logg «sitter fast ved X» (hjelper Grafikk).
7. **Dekning (enkel – Andreas ba om det):** når boten blir truffet eller ser en fiende den ikke straks kan skyte (`CoverUse`-sjanse),
   eller lader med fiende synlig: let etter dekning innen 8 m: 12 nåbare punkter (`GetRandomReachablePointInRadius` rundt
   seg); et punkt er **dekning** når sporet fra fiendens øye (husket sted) til punktet + 150 cm er blokkert av fast verden
   (ikke glass), og **skuddstilling** når + 100 cm er blokkert, men + 150 cm er fritt (huk for å gjemme seg, reis seg for å skyte).
   Velg nærmeste skuddstilling, ellers nærmeste dekning; løp dit. I skuddstilling: huk (`ToggleCrouch`) mens den lader eller
   venter, reis seg og skyt i 1–2,5 s, huk igjen. Budsjett: høyst 2 dekningssøk per bilde for alle bots.
8. **Medisin:** blør (`GetBleedRate() > 0,3`) og ingen fiende sett siste 3 s: gå i dekning (eller stå stille), velg i sekken
   den første som stopper blødningen (`Kind` / bandasje-Id-ene fra katalogen) og `ServerUseItem(Index)`; ikke skyt eller løp
   før bruken er ferdig (`UseItemId` blir `None`); avbryt (`ServerCancelUse`) hvis en fiende dukker opp innen 15 m.
   Helse under 60 og trygg: bruk det som gir mest helse. Under `RetreatHealth` med fiende synlig: trekk mot egen base/
   lagkamerater til helsa er over igjen eller fienden er borte i 6 s.
9. **Våpen:** ved spawn, velg sterkeste våpenslot (gevær før pistol); bytt til pistol når geværet er tomt og fienden er
   innen 15 m (raskere enn å lade). Loot plukkes ikke i denne runden (plan-07), bortsett fra: gå over en pickup med ammo til
   våpenet sitt innen 3 m fra stien → `ServerInteract`.
10. **Mål fra laget (steg 5):** `Role` = Angrip / Forsvar / Hent eget flagg / Bær hjem. Angrip: via-punkt → fiendens flagg
    (stien slutter på flaggets plass, `AGSFlag::ServerStep` tar det ved berøring). Bær hjem: et annet via-punkt enn på vei inn
    (helst motsatt side), aldri sprint gjennom åpne rom der fiender ble sett siste 10 s hvis en annen rute er < 40 % lengre.
    Forsvar: gå til tildelt forsvarspunkt, huk, se mot den inngangen punktet ser best; bytt punkt etter 30–60 s eller når
    en fiende har sett deg der. Hent eget flagg: gå til flaggets `RevealLocation`/mistet sted; ved berøring sendes det hjem (finnes).

### 5. Lagets hjerne `AGSBotTeam` (`GSBots.cpp`, én per lag, bare på serveren)
Hvert 2. s (og straks ved flagghendelse): tell lagets bots `N` og mennesker `H`.
- Forsvarere: `N + H <= 2` → 0 (alle angriper); 3–5 → 1; 6–8 → 2; 9+ → 3. Velg dem blant botene nærmest eget flagg.
- Eget flagg ikke hjemme: halvparten av angriperne (minst 1) blir «Hent eget flagg» (mot sist kjente sted).
- En bot bærer fiendens flagg: den får «Bær hjem»; én angriper nærmest ham følger ham (enkel eskorte: gå 6 m bak) – full
  eskorte er plan-07.
- Resten angriper, fordelt på ulike via-punkter (ikke mer enn 2 på samme), byttet ved hver ny runde inn.
- Felles budsjett for spor (syn 48, dekning 2 per bilde) og felles hukommelse light: en fiende som en bot har sett siste 3 s
  deles med lagkameratene innen 25 m («ropte») – realistisk, ikke radar.

### 6. `AGSGameMode`: bots inn og ut
1. I bot-kamp, etter at menneskespilleren er inne (plan-05 steg 2.6) og før nedtellingen: lag `BotAllies` bots på hans lag
   og `BotEnemies` på det andre. Per bot: `SpawnActor<AGSBotController>`, `PS->SetIsABot(true)`, navn, `PS->Team`,
   `PS->bReady = true`, `BotSkill` fra `GS`, `RestartPlayer(Bot)` (spawn ved egen base som alle andre). `MaxPerTeam` gjelder.
2. `StartMatch` (bot-kamp): lagene står (plan-05), botene får ny kropp som alle andre (løkka over `PlayerArray` finner
   kontrolleren via `GetOwningController`, som er boten selv).
3. **Lagbytte i bot-kamp** (`RequestTeam`, bare i lobbyen): mennesket flytter, og én bot fra det nye laget flytter motsatt
   vei; regelen «ikke skjevere» gjelder ikke i bot-kamp.
4. `Logout`/kartbytte: botene forsvinner med verdenen. `GS->PlayerArray` inneholder bots – telling av «spillere» i lobbyen
   (`Total >= 2`-regelen for 90 s) gjelder bare online og skal ikke telle `IsABot()`.
5. Konsollkommando for testing (`Exec` i GameMode er ikke nok – legg den i `AGSBotController` som `static FAutoConsoleCommandWithWorldAndArgs`):
   `gs.bots.debug 1` tegner hver bots rolle, mål og sti (`DrawDebugLine`, bare `!UE_BUILD_SHIPPING`).

### 7. Meny og hjelp
- `GSMenu`: `bBotsAvailable = true`; teksten under vanskelighetsgradene står. I test: knappen «Vis navnett» (kaller
  `GSBots::EnsureNavMesh` og konsollen `show Navigation`) – for Grafikk og kontrolløren.
- Poengtavla er Kamps; navnet har «(bot)» i seg, så den trenger ingen endring. `GSHelp`s lobby-panel viser
  «du + N mot M (Middels)».

## Ferdig når
- [ ] Bygger uten feil; modulene lastes; `Saved/Logs/Gloshaugen.log` viser: navnettet laget (fliser > 0, tid), selvtesten
      med sti rød→blått og blått→rødt flagg (lengder), og via-punkter og forsvarspunkter (antall). Ingen «INGEN STI».
- [ ] **PIE, Spill mot bots, du + 4 mot 5, Middels:** etter nedtellingen løper botene ut fra basene på minst to ulike
      ruter; minst én blir igjen ved eget flagg. En bot tar flagget ditt innen ca. 3 minutter hvis du ikke forsvarer, og
      bærer det hjem (erobring telles). Forsvarerne skyter deg når du kommer inn i baserommet.
- [ ] Botene skyter ikke før reaksjonstiden; på 30 m bommer Lett og Middels ofte (salver spres og klatrer), Ekspert sjelden.
      Ingen bot sikter eller skyter gjennom en betongvegg; ingen vet hvor du er når du har vært usett i lenge.
- [ ] En bot som blir skutt søker dekning eller skuddstilling (huk/reis), bandasjerer seg når den blør og er trygg.
- [ ] Bots dør, blør, blir liggende og gjenoppstår ved egen base etter `RespawnDelay`; drapslista og poengtavla viser navnet
      med «(bot)»; lagkamerat-bot skader deg ikke med venneild av, og skyter ikke gjennom deg.
- [ ] Verten (du) hører botenes skudd som 3D-lyd der de står, ikke som egne skudd – **krever K1**; uten K1 noteres det som
      kjent feil i loggen, ikke som avvist runde.
- [ ] 10 mot 10 (du + 9 mot 10) på Helgasetr: bildefrekvensen faller høyst 10 % mot samme scene uten bots (`stat unit`,
      mål og skriv tallene i loggen); botenes tenking < 1,5 ms per bilde (`stat game`/egen `SCOPE_CYCLE_COUNTER`).
- [ ] Byttes kartet (Grafikk setter `bHelgasetr`), virker botene uten endring i koden (ingen koordinater i `GSBots*`).

## Risiko
- **Navnettet bygges ikke** (volumet uten pensel): reserven med invoker; begge logges. Lazy innsamling: første fliser kan
  ta noen sekunder på gamle campus – nedtellingen venter (maks 20 s).
- **Ytelse ved oppstart i andre moduser:** `RuntimeGeneration=Dynamic` og `bAutoCreateNavigationData` lager en tom
  `RecastNavMesh` også i online og test. Mål oppstartstid i online før/etter; er den > 0,5 s lengre, sett
  `bAutoCreateNavigationData=False` og lag navdata selv i `EnsureNavMesh` (`Nav->Build()`), og noter det.
- **Kamps kode og AI:** uten K1 hører verten feil lyder og kan få førstepersonsting laget for bots. Alt som kaller
  `Cast<AGSPlayerController>(GetController())` tåler `nullptr` (lest: inputblokk, terminal, meny – alle sjekker `PC`).
- **Crowd-unnvikelse** virker bare mellom bots; mot mennesker glir CharacterMovement. Trange dører: to bots samtidig kan
  stå og dytte; «sitter fast»-regelen løser det.
- **Juks/nett:** botene kjører bare på serveren og går gjennom de samme RPC-ene og sjekkene som spillere (skuddkonto,
  sikteretning). Klienter ser vanlige figurer. Bots i online-kamp (fylle opp lag) er lagt opp til, men ikke slått på.
- **Realisme:** ingen bot får informasjon en spiller ikke har (lærdom: avsløring som radiomelding, ikke veggsyn).

## IMPROVEMENTS-linjer (seksjon «Runde 6 (Spillbarhet): bots i Capture the Flag»)
614. Bots: datastyrte soldater spiller Capture the Flag med deg og mot deg, i samme kropp, med samme våpen, skade og blod som spillere
615. Botene finner veien i bygget gjennom ganger, rom og trapper med et navigasjonsnett som lages når kartet bygges, også når kartet byttes
616. Botene velger ulike ruter mot flagget hver gang, gjennom forskjellige korridorer og etasjer
617. Noen bots blir igjen og forsvarer eget flagg fra steder med utsyn mot inngangene, og bytter plass etter en stund
618. Botene tar flagget ditt, bærer det hjem en annen vei enn de kom, og henter sitt eget flagg tilbake når det er mistet
619. Botene ser bare det de faktisk ser: synsfelt, synsvidde, mørke og vegger teller, og glass kan ses gjennom
620. Botene hører skudd og løping i nærheten og snur seg mot lyden; lyddempede våpen høres kortere
621. Botene trenger reaksjonstid før første skudd, og siktet blir bedre jo lenger de har sett deg
622. Botene skyter salver på avstand, holder avtrekkeren inne på nært hold og må kjempe mot rekylen som alle andre
623. Botene søker dekning når de blir beskutt, og skyter fra huk-stillinger bak lave ting
624. Botene bandasjerer seg når de blør og tar medisin når de er trygge, og trekker seg tilbake når de er hardt såret
625. Fire vanskelighetsgrader – Lett, Middels, Vanskelig og Ekspert – skiller bare på ferdighet, aldri på helse eller skade
626. Botene skyter ikke når en lagkamerat står i skuddlinja, og tar ikke skade fra lagkamerater når venneild er av
627. Botene heter vanlige norske navn merket «(bot)» i drapslista, poengtavla og lobbyen
628. Bytter du lag i lobbyen i en bot-kamp, bytter en bot plass med deg så lagene blir like store
629. Botene deler det de ser med lagkamerater i nærheten, slik spillere roper til hverandre, uten å se gjennom vegger
