# Forhåndssjekk av bots/ko/plan-01.md (arvet plan-06 fra Spillbarhet)

**BØR ENDRES**

Sjekket 27. nov. mot HEAD (`3845291`, med Kamps 14a `c616f84`), arbeidsmappen der Spillbarhets plan-05 bygges (bare lest),
Grafikks `ko/plan-04.md` + `helgasetr.md`, og motorkoden i UE 5.8. Grunnideen holder: navnett i kode, egne syns-/hørselssjekker
uten veggsyn, samme `AGSCharacter` og samme handlinger som spillerne, alt på serveren. Men planen har fire feil som ville
gitt en runde som ikke virker (eierskap, sikte i høyden, via-punktene i atriet, K1 som «kjent feil»), og den er for stor.

## Svar på spørsmålene

**1. Virker navnettet i kode i et innendørskart med flere etasjer og trapper av klosser? Ja, med to rettelser.**
- Motoren bekrefter planens vei: `ANavMeshBoundsVolume::PostRegisterAllComponents` melder grensene (`NavMeshBoundsVolume.cpp`
  l. 117–125), `OnNavigationBoundsUpdated` leser `GetComponentsBoundingBox(true)` (`NavigationSystem.cpp` l. 4136), og
  `UBrushComponent::CalcBounds` bruker `BrushBodySetup->AggGeom` når det ikke finnes editor-pensel (`BrushComponent.cpp`
  l. 459–485; i PIE er `Brush` null, så samme vei). Utsatt spawn med `FKBoxElem` før `FinishSpawning` gir riktige grenser.
  `bWholeWorldNavigable` er ikke et alternativ (kommentert som «broken», `NavigationSystem.h` l. 368).
- Klossbygget (`AGSHelgasetrBuilder`, ISM-er) virker med dynamisk generering: en ISM registreres i nav-oktreet når første
  instans legges til og melder skitne områder per instans (`InstancedStaticMesh.cpp` l. 3794 og 5415). Nav-relevant = blokkerer
  `Pawn` – derfor kommer glassrekkverket (blokkerer Pawn, ignorerer Visibility) og `AtriumBlockers` med i nettet, og tingene over
  P4 som ignorerer Pawn kommer ikke med. Det er riktig for Helgasetr.
- Kartet er bygget før `AGSGameMode::BeginPlay` (`UGSWorldSubsystem::OnWorldBeginPlay` → `Build()`), så `EnsureNavMesh` etter
  bygget er riktig rekkefølge. Kartfila har `NavigationSystemModuleConfig`, så navigasjonssystemet lages.
- Trinn: atriumtrappa 0,18 m og trapperom ≤ 0,20 m er godt under steghøyden. Recast runder `AgentMaxStepHeight` 45 ned til
  hele celler (45/10 → 40 cm), så sittetrinnene på 0,45 m blir **ikke** gangbare i nettet – bra, botene bruker den vanlige delen.
- **Feil A (stopper rutene):** via-punktene trekkes fra «midtpunktet mellom flaggene». I Helgasetr står begge flaggene på Plan 4
  (Z 13,9) i nord og sør, så midtpunktet (≈ 0, 0, 13,9) er **lufta midt i atriet**, 14 m over kantinegulvet. Projeksjonen
  (utstrekning Z 250 cm) finner ikke nett → 0 via-punkter → alle angrep går korteste vei. Start fra hvert flagg projisert til
  nettet (eller fra spawnpunktene) og trekk punkter derfra.
- **Feil B (grenser):** reserven `PlayArea() × Z −15…+45 m` tar med Plan 5–6 (kulisse), glasstaket (BlockAll, gangbart på
  toppen) og asfalten ute. Øyer der gjør ingen skade når punktene trekkes fra flagget, men koster byggetid. Bruk
  `PlayArea() × Z −1…+17,5 m` som reserve (P1 til like under P5) og be Grafikk om `NavBoundsM()` i `GSHelgasetr`.
- **Dører (1,0 m):** Recast runder agentradius **opp** til hele celler: 36 cm / 10 → 40 cm hver side → 20 cm stripe gjennom en
  1,0 m dør, og den kan forsvinne når karmen ikke ligger på cellegrensen. Bruk `AgentRadius=30` (3 celler, 40 cm stripe) og la
  selvtesten gå gjennom **hver** døråpning i `GSHelgasetr::DoorSlots()` når den finnes («dør X: sti ja/nei»).

**2. Får botene riktig kropp etter 14a? Ja for det verten ser – men dobbelt så dyrt, og bare med K1.**
- Kroppen andre ser er riktig: `GetMesh()` er `SetOwnerNoSee`, eieren er boten, verten ser den. `WorldSpaceRepresentation`
  skjuler bare for visningen der eieren er view-aktør, så den gjør ingen synlig skade.
- Men `SetupLocalBody` (`GSCharacter.cpp` l. 3353) sjekker `IsLocallyControlled()`, som er sann for AI. Hver bot får da
  `FPBody` synlig med `AlwaysTickPoseAndRefreshBones`, `GetMesh()` likeså, og `bEnableUpdateRateOptimizations = false`: to fulle
  animasjoner per bot hvert bilde, også bak vegger. `Tick`-grenen l. 4270 kjører `UpdateFocusPickup` (spor), `UpdateLocalVisuals`
  (postprosess på botens kamera) og `UpdateWeaponAnimation` for hver bot. Treffsonene er kapselbokser (l. 3217), ikke bein, så
  serveren trenger ikke positurer for bots utenfor syne.
- **Ikke** prøv å omgå K1 med `IsLocalController() = false` i botkontrolleren: `CharacterMovementComponent` flytter bare figurer
  som er lokalt styrt – boten ville stått stille.

**3. Treffer de realistisk uten veggsyn? Ja i opplegget, men to hull.**
- **Feil C (stopper skytingen i høyden):** planen sier «`FireOnce` sikter med kameraet, og kameraet følger kontrollrotasjonen».
  Det stemmer ikke for AI. `UCameraComponent` setter `bUsePawnControlRotation`-rotasjonen bare i `GetCameraView`
  (`CameraComponent.cpp` l. 418–434), og den kalles bare av en spillers kameramanager. For en bot følger kameraet figurens yaw
  (`bUseControllerRotationYaw`), men **pitch blir 0**. `FireOnce` bruker `Camera->GetForwardVector()` (l. 904) → botene skyter
  vannrett: de treffer aldri noen i trapper, på andre etasjer eller over atriet. Rettelse i botkontrolleren (krever K1c-friend):
  `Char->Camera->SetWorldRotation(GetControlRotation())` hvert bilde før skudd. (Verten og klientene ser riktig sikte på
  tredjepersonsvåpenet: `GetBaseAimRotation` går via kontrolleren, og `RemoteViewPitch` settes for AI i `PreReplication`.)
- **Feil D (skyter i fast glass):** synet regner `Mat_Glass` som gjennomsiktig (riktig), men fast glass i Helgasetr (vinduer,
  klatrehallens glassvegg, vindfang, glasstak) blokkerer `ECC_Visibility`, som er kulekanalen (`GSBallistics.cpp` l. 319).
  Botene vil tømme magasinet i glass de ikke kan skyte gjennom. Legg til skuddlinje-sjekk: `ECC_Visibility` fra kameraet til
  siktepunktet; første treff må være målet (eller en fiende), ellers ikke skyt – flytt deg eller vent. (Rekkverksglasset ignorerer
  Visibility til Interaktiv gjør det knusbart, så der går både syn og kuler gjennom – konsistent.)
- Mørke: «× 0,6 kl. 21–06» passer ikke innendørs med lys på. Dropp den i Helgasetr (eller knytt den til lysbryterne senere).
- Resten er godt: hukommelse uten veggsyn, reaksjonstid, feil som halveres, rekyl laget av boten selv (`AddControllerPitchInput`
  gjør ingenting for AI – stemmer), ingen ekstra helse/skade, deling bare innen 25 m, flagget bare via samme radiomelding.
  Egne sjekker i stedet for AIPerception er riktig valg (billigere og samme regler som mennesker).

**4. Ytelse med 19 bots på RTX 3050-laptop.**
- Tenkebudsjettet (syn 5 Hz, 48 spor/bilde, 2 dekningssøk/bilde, forskjøvet per bot) er bra og holder under 1,5 ms.
- Det dyre er ikke tenkingen, men kroppene: 19 × `CharacterMovement` på serveren + 19 × `GSBodyAnim` + `GSCharacter::Tick`.
  Uten K1 dobles animasjonen (se 2). Med K1 får botene `OnlyTickPoseWhenRendered` og URO som andre spillere.
- **Kriteriet «høyst 10 % fall»** kan ikke nås og måler feil ting: 19 ekstra figurer koster det samme som 19 mennesker, uansett
  hvem som styrer dem. Mål i stedet (a) botenes tenking ≤ 1,5 ms (`SCOPE_CYCLE_COUNTER`), (b) spilltråden med 19 bots mot 19 bots
  med `gs.bots.think 0` (står stille), (c) absolutt `stat unit` i atriet, 1080p Middels, og skriv tallene i loggen.
- Planen må nevne: botene registrerer seg ikke som nav-invokers (ingen flisbygging under spill), og crowd-manageren tåler 50
  agenter som standard.

**5. Nettverk.** Bot-kamp er alltid `NM_Standalone` (plan-05, `GSGameState.h` l. 8 og `GSGameMode.cpp` l. 484), så ingen
nettrisiko i denne runden. For senere online: all logikk er på serveren; klientene ser vanlige simulerte figurer; `Client`-RPC-er
på bots kjøres på verten (K1b); `bAllowClientSideNavigation=False` gjør at klienter ikke lager nav-system. Båndbredde: bots
replikeres som spillere, innenfor dagens tak (20 figurer).

**6. Juks/rettferdighet.** I orden. Botene går gjennom `ServerFire`/`HandleFire` som alle; retningssjekken (l. 1010) hoppes over
for lokalt styrte, men det er serverkode og boten endrer ingen felt direkte. Hørsel leser `ServerLastFireTime`, men gir bare
det et menneske ville hørt (retning, ±3 m). Botene får samme startutstyr (plan-05 `bStartKit`). Ingen grad får mer helse eller
skade.

**7. Er runden for stor? Ja.** 4 nye filer + 5 endrede andres filer + ini + 16 IMPROVEMENTS-linjer + 10 delsystemer i
kontrolleren + lagets hjerne. Del i to.

## Endringer planleggeren må gjøre

1. **Eierskap (laget eier nå bare `GSBotController.*`, `GSBotTeam.*`, `GSBots.*`, `GSNavSetup.*`):** planen endrer
   `GSGameMode.*`, `GSMenu.cpp`, `GSPlayerState.*`, `GSHelp.cpp` – alle Spillbarhets, og tre av dem bygges om i plan-05 nå.
   Løsning som nesten ikke trenger andre: en `UGSBotsSubsystem` (`UWorldSubsystem`, i `GSBots.*`) som i bot-kamp
   (`GS->GameKind == GSGameKind::Bots`, standalone) kaller `GSNavSetup::EnsureNavMesh`, venter til mennesket står i
   `PlayerArray`, lager botene og kaller `AGSGameMode::RestartPlayer` (public). `StartMatch` finner dem via `PlayerArray` som før.
   Bestilling til Spillbarhet (liten): `bBotsAvailable = true` (constexpr i `GSGameState.h`), lagbytte mot en bot i
   `RequestTeam`, «du + N mot M» i `GSHelp`, «Vis navnett» i test-menyen (kaller `GSNavSetup::EnsureNavMesh`). Lobby-tellingen
   (`Total >= 2`) gjelder bare online og trenger ingen endring nå. Navnettet i `GSNavSetup.*`, lagets hjerne i `GSBotTeam.*`.
2. **K1 er et krav før bygging, ikke «kjent feil».** I standalone hører Andreas hvert botskudd som sitt eget 2D-skudd og ser ingen
   munningsflamme/sporlys fra botene (`MulticastShotFX` returnerer). K1 står ÅPEN i `bestillinger.md` og er ikke i Kamps planer.
   Utvid K1 med: (e) `SetupLocalBody` og `Tick`-grenen l. 4270 krever `IsLocalHuman()` (14a), (f) `friend` må dekke `Camera`
   (protected, `GSCharacter.h` l. 197) og `ServerLastFireTime` (l. 379). Hovedagenten bør be Kamp ta K1 i sin neste runde.
3. **Sikte i høyden (feil C):** botkontrolleren setter `Camera`-rotasjonen fra kontrollrotasjonen hvert bilde. Ferdig når:
   en bot på P4 treffer en spiller på P2 over atriet, og en bot i trappa treffer en spiller på reposet over.
4. **Via-punkter (feil A):** trekk fra flaggene/spawnpunktene projisert til nettet, ikke fra midtpunktet.
5. **Skuddlinje (feil D):** ikke skyt når første `ECC_Visibility`-treff ikke er målet.
6. **Ini:** `AgentRadius=30`; reservegrense Z −1…+17,5 m; selvtest per døråpning. Behold `bAllowClientSideNavigation=False`.
7. **Bestillinger flyttes:** G2/G3 gjelder nå Interaktiv verden (eier `GSDoor`, `GSBreakable`, `GSPhysicsProp`, `GSMapState`).
   Fra motoren: dynamisk generering gjør nettet skittent av seg selv når en nav-relevant komponent fjernes eller flyttes, så G3
   (`NotifyMapChanged`) trengs nesten ikke. Men **fysikkmøbler, skår og dørblad må `SetCanEverAffectNavigation(false)`**, ellers
   bygges fliser om hvert bilde mens de beveger seg (ytelsesfelle). Låste dører: `UNavModifierComponent` med `NavArea_Null`.
   Knusbart glass beholder taggen `Mat_Glass`. G1 (`NavBoundsM`) og G4 går til Grafikk som før.
8. **Lukket dør i stien:** inntil plan-02: står boten fast og en `AGSDoor` er innen 1,5 m i fartsretningen → `ServerInteract`.
   Skriv det som «når `AGSDoor` finnes» (den er ikke bygget).
9. **Ytelseskriteriet** som i punkt 4 over.
10. **IMPROVEMENTS:** 614–629 var Spillbarhets numre; dirigenten deler ut nye.

## Forslag til deling
- **01a – bots som spiller:** `GSNavSetup` (navnett, klar-sjekk, selvtest, via- og forsvarspunkter), `GSBots` (subsystem,
  ferdighetstabell, navn, lage bots), `GSBotController` (hukommelse, syn, sikte med kamerafiks, skudd med reaksjon/feil/rekyl og
  skuddlinje, bevegelse, sitter fast), `GSBotTeam` (roller: angrip, forsvar, hent, bær hjem). Krav: K1 levert, Helgasetr 4a bygget.
- **01b – bots som overlever:** dekning og skuddstilling, medisin, tilbaketrekking, hørsel, deling innen 25 m, enkel eskorte,
  ytelsesmåling med 19 bots.

## Liste til byggeren (når planen er skrevet om)
- [ ] K1 (inkl. e/f) er levert av Kamp før du starter; ellers stopp og meld fra.
- [ ] Bare lagets fire filpar + små tillegg i `Gloshaugen.Build.cs` og `DefaultEngine.ini`. Ingen endring i `GSGameMode`/`GSMenu`/`GSHelp`.
- [ ] `ANavMeshBoundsVolume` utsatt, `FKBoxElem`, `FinishSpawning`, så `OnNavigationBoundsUpdated`; logg fliser og tid.
- [ ] Via-punkter fra projiserte flagg/spawn; logg antall per etasje og side. Selvtest per base og per dør.
- [ ] `Camera->SetWorldRotation(GetControlRotation())` hvert bilde i boten; prøv skudd opp og ned trapp og over atriet.
- [ ] Skuddlinje på `ECC_Visibility` før hvert skudd/salve.
- [ ] Synsspor hopper over `Mat_Glass`; ingen nattfaktor innendørs.
- [ ] Ikke overstyr `IsLocalController`. Botene endrer aldri felt i `AGSCharacter` direkte (bortsett fra kamerarotasjonen).
- [ ] Mål ytelse som i punkt 4 og skriv tallene i loggen.

## Lærdom
- AI-figurer får ikke kamerarotasjon fra kontrolleren: `bUsePawnControlRotation` virker bare i `GetCameraView`, som bare en
  spillers kameramanager kaller. Kode som sikter med kameraet må få rotasjonen satt for bots.
- Punkter som projiseres til navnettet må starte på gulv: midtpunktet mellom to steder kan ligge i lufta (atrium, trapperom).
- Arvede planer må sjekkes mot ny eiertabell: filene planen endrer kan ha byttet eier.
