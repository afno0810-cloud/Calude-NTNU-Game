**FORHÅNDSSJEKK (ikke bygget): ingen sikre kompileringsfeil funnet i lag Kamps filer. Kan bygges når editoren er lukket. Små ting bør rettes, se under.**

Lest fra arbeidsmappen (ucommittet), ikke bygget, fordi Andreas har editoren åpen. Endelig GODKJENT kan først gis
når `Build.bat` har gått gjennom.

## 1. Bygg og lenking

Gått gjennom linje for linje mot headerne slik de står nå (`GSHelp.h`, `GSFlag.h`, `GSGameState.h`,
`GSPlayerState.h`, `GSAudio.h`, `GSCampus.h`, `GSUtils.h`, `GSTypes.h`) og mot motorkoden i UE 5.8.

Alt dette stemmer:
- `GSHelp::Draw(AHUD*, UCanvas*)`, `GSFlagEvent::Taken/Dropped/Returned/Captured`, `AGSGameState::OnFlagEvent`
  (signaturen `(uint8, uint8, const FVector&, APlayerState*)` er lik `AGSHUD::HandleFlagEvent`), `RedScore`,
  `BlueScore`, `TeamScore()`, `CaptureLimit`, `MatchMinutes`, `GSTeam::Name` (gir `const TCHAR*`, så `%s` i
  `Printf` er lov), `GSTeam::Color`, `AGSPlayerState::Team/Captures/Returns/CarriedFlagTeam`,
  `GSAudio::Play2D(const UObject*, const FString&, float, float)`, `GSCampus::PlayArea()` (`FBox2D`),
  `FBuilding::bBackdrop/bEnterable/Name`, `FRoad::A/B/Width`, `AGSHUD::FillClipped` (finnes fra før),
  `GetLegPain()` (offentlig), `FGSTrackedPlayer::Name`.
- Include: `GSHUD.cpp` har `GSHelp.h`, `GSFlag.h`, `GSAudio.h`, `EngineUtils.h`. `GSCharacter.cpp` har
  `GSPlayerState.h`, `Engine/SkeletalMesh.h` (gir `FReferenceSkeleton`) og `GSUtils.h` (gir
  `MaterialInstanceDynamic.h`). `GSPlayerController.cpp` har `IConsoleManager.h`, `GameUserSettings.h`,
  `ConfigCacheIni.h`, `Engine/Engine.h`.
- `StartTime` og `DrawHelp` er fjernet både i `.h` og `.cpp`, ingen rester.
- Ingen skyggevariabler funnet (UE gjør dem til feil): `Mine` i `DrawMatch` står i to søsken-blokker, ikke i hverandre.
- `r.RayTracing.Enable` finnes i 5.8 og kan slås av og på mens spillet kjører (`r.RayTracing.EnableOnDemand` er 1 som standard).
- Materialparameteren `Paint Tint` finnes i `M_Mannequin`, `MI_Manny_01_New` og `MI_Quinn_01`.
- De fire lydene `S_GS_Flag_*` er både laget (`SourceArt/Sounds`) og importert (`Content/Sounds`).

To ting som KAN stoppe bygget, men som ikke er feil i Kamps kode:

**B1. `Source/Gloshaugen/Public/GSMenu.h` finnes, men `GSMenu.cpp` finnes ikke (lag Spillbarhet er midt i arbeidet).**
Headeren har `UCLASS() UGSGameViewportClient` med `virtual bool InputChar(...) override`. Uten en `.cpp` som
definerer den gir hele modulen lenkefeil (`LNK2001 UGSGameViewportClient::InputChar`), også for Kamp.
Rettelse: ikke bygg før Spillbarhets bygger har lagt inn `Private/GSMenu.cpp`. Kamp skal ikke røre filen.

**B2. Ikke bruk Live Coding (Ctrl+Alt+F11) for denne runden.**
`GSCharacter.h` får to nye komponenter som lages i konstruktøren (`ArmbandL`, `ArmbandR`) og nye medlemmer, og
`GSHUD.h`/`GSPlayerController.h` endrer også klasseoppsettet. Live Coding lager ikke komponentene på nytt i
figurer som alt finnes, og da er `ArmbandL` null i `ApplyTeamLook` (`GSCharacter.cpp:2684`) og `Tick`
(`:3634`) – krasj. Rettelse: Andreas lukker Unreal, så vanlig `Build.bat ... -WaitMutex`.

## 2. Funn (ekte, men små)

**F1. Tom navnelinje i radioen – `GSHUD.cpp`, `HandleFlagEvent` (ca. linje 953 og 966).**
Når `Who` er null hos en klient (spillerstatusen er ikke koblet ennå når den pålitelige multicasten kommer),
blir linja « har flagget deres. Dekk bæreren.» og « erobret flagget deres.». Bare «tatt av»-grenen sjekker
`Name.IsEmpty()`.
Rettelse: `const FString Name = Who ? Who->GetPlayerName().Left(20) : FString(TEXT("En lagkamerat"));` og behold
`IsEmpty`-grenen ved å teste `!Who` i stedet.

**F2. Samme flagghendelse vises tre steder samtidig.**
(1) `AGSFlag::Announce` legger en linje i hendelseslista («Kari [tok] det blå flagget», `GSFlag.cpp:302`),
(2) `GSHelp` viser «Kari tok det blå flagget» midt oppe i 6 s (`GSHelp.cpp:156`), (3) den nye radiolinja rett
under hendelseslista. Det er rot, og strider mot «heller færre ting som ser ferdige ut».
Rettelse: bestilling til Spillbarhet i `.claude/lag/bestillinger.md`: fjern `AddKill`-linjene i
`AGSFlag::Announce` og punkt 2 sin «siste hendelse»-gren i `GSHelp` (behold «Du bærer flagget»), nå som HUD-en
har radiolinja. Kamps kode trenger ingen endring.

**F3. Resultatet tegnes to ganger når kampen er over.**
`GSHelp.cpp:177` («4. Match over») skriver «Rødt lag vant 2–1» ved y ≈ 50, og `DrawMatch` skriver det samme i
boksen på y = 96+. Lobbyen har det samme i mindre grad: regelen og «Trykk Enter når du er klar» står både i
HUD-ens lobbyboks (`GSHUD.cpp:789–793`) og i `GSHelp`-panelet til venstre.
Rettelse: samme bestilling: Spillbarhet fjerner punkt 4 i `GSHelp`. Kamp kan selv fjerne regellinja under
lobbyboksen (`GSHUD.cpp:792–793`), siden `GSHelp` forklarer reglene bedre.

**F4. `t.MaxFPS` – `GSPlayerController.cpp:335–338`.**
- Settes med `ECVF_SetByGameSetting`. Har Andreas skrevet `t.MaxFPS 120` i konsollen (høyere prioritet), blir
  `Bildefrekvens 60` stille ignorert, men meldingen sier likevel «høyst 60». Rettelse: i `Bildefrekvens` (ikke i
  `BeginPlay`) bruk `ECVF_SetByConsole`, f.eks. en parameter `EConsoleVariableFlags Priority` til `ApplyFrameLimit`.
- I editoren blir 60-taket stående etter at Play er stoppet (editorvinduet er da også låst til 60 til Unreal
  startes på nytt). Ufarlig, men bør stå i loggen/til Andreas. Kan rettes med en `EndPlay` som setter
  `t.MaxFPS` tilbake til 0 når `GIsEditor`.
- `BeginPlay` har `IsLocalController()`-vakt før dette, så verten får ikke taket satt på nytt for hver klient. Bra.

**F5. Poengtavla på sluttskjermen kan legge seg over resultatboksen – `GSHUD.cpp:1034`.**
Med 20 spillere er tavla 628 px høy. På 720 p klemmes `Y` opp til 82 og dekker boksen (96–218). På 1080 p
(Andreas' laptop) går det bra (266 + 628 = 894). Rettelse: når `Y` er gitt utenfra, klem aldri over den
(`Y = FMath::Max(YInn, ...)`) eller krymp radhøyden.

**F6. Kommentaren i `ServerFire` lover mer enn formelen – `GSCharacter.cpp:889–891`.**
`1 + Clamp(0.15/Intervall, 0.5, 2)` er formelen jeg selv ba om i runde 4, og den er riktig inne. Men kontoen
rommer «minst to skudd» bare når intervallet er 0,15 s eller kortere; for tregere våpen er taket 1,5–2. Det er
greit i praksis (to ærlige skudd med 200 ms mellom havner ikke i samme serverbilde), men rett kommentaren til
«minst halvannet skudd, to eller flere for våpen raskere enn 400 skudd/min».

**F7. Lik som ligger når en sen klient kobler til er grå uten armbind.**
Liket har mistet spillerstatusen, så `GetTeam()` gir 0 i `BeginPlay`, og `Tick`-vakten hopper over døde. Lite
synlig. Kan rettes senere ved å replikere lagnummeret på figuren (`UPROPERTY(Replicated) uint8 BodyTeam`).

## 3. Det som er sjekket og er i orden

- **Vert / klient / sen klient, lagfarge:** laget ligger i spillerstatusen (replikert til alle). `BeginPlay`
  prøver, og `Tick`-vakten (`!bDead && AppliedTeam != GetTeam()`, én sammenligning per figur per bilde) tar
  resten: sen spillerstatus, L i lobbyen, utjevning ved kampstart. Materialer lages bare når laget faktisk
  endres. Døde beholder fargen.
- **Armbind:** ingen kollisjon (`SetupVisual`), skjult for eieren til han dør (`OnRep_Dead` slår av
  `OwnerNoSee`), følger spøkelsesblinkingen. Plasseres fra skjelettet én gang.
- **Navn over lagkamerater:** bare eget lag, innen 25 m, én siktlinje per lagkamerat i rekkevidde per bilde
  (høyst 9), bare på egen maskin. Billig nok. Ikke tegnet i kikkertsikte eller kikkert. Aldri noe over fiender.
- **Flagg-radio:** `OnFlagEvent` bindes én gang per game state med svak peker, virker på vert og klient.
  `FlagTeam` er flaggets lag, så «vårt/deres» blir riktig. Lyden mangler ikke stille: filene er importert.
- **Dobbelttegning med GSHelp:** `GSHelp::Draw` kalles fra HUD-en, og kroken i `OnHUDPostRender` hopper da
  over samme bilde. Statusstripa (y 8–42), klokka (46–76) og egen linje (84) kolliderer ikke. Kompasset
  (nederst, 152–126 px fra bunnen) går klar av gjenstandsnavnet (112 px) og sekkelinja (88 px).
- **Bærer:** 1,6 × utholdenhet og 0,85 × fart regnes likt på server og eier. Halting gjelder nå også huk.
- **Hagl i bein/arm:** ett rykk per 0,06 s (lærdom fra runde 4 fulgt).
- **Straaling:** slår av hele strålesporingen; gir ett hakk idet den byttes, ikke ellers.
- **`gen_sounds.py flag`:** skriver bare de fire nye lydene, nøkterne radiotoner, ingen fanfare.
- **Andreas' faste regel:** oppfylt. Forbedret: kroppsfarge (432), poengtavle (435), sluttskjerm (436),
  PC-kartet (442). Nytt: armbind (433), navn over lagkamerater (434), flagg-radio (438–439),
  bildefrekvens-tak (444). `IMPROVEMENTS.md` 432–445 er ført. De små rettingene fra runde 4 (MaxCredit,
  ett rykk per hagleskudd, halting i huk, riktig tekst om beinet) er ikke ført – før dem gjerne som 446–449.

## Forslag til neste runde
1. Rydd flaggmeldingene til ett sted (F2/F3) sammen med Spillbarhet.
2. Replikér lagnummer på figuren, så lik og figurer uten spillerstatus alltid har riktig farge (F7).
3. Navn over lagkamerater bør også skjules i røyk (i dag stopper bare vegger siktlinja).

## Lærdom
- Når to lag tegner om samme hendelse (HUD, GSHelp, hendelsesliste), må planen si hvem som eier meldingen, ellers vises den flere ganger.
- Runder som legger til komponenter i en konstruktør eller endrer en header kan ikke tas med Live Coding; editoren må lukkes.
- En halvferdig header med `UCLASS` uten `.cpp` fra et annet lag stopper lenkingen for alle; legg alltid inn `.cpp` samtidig med headeren.
