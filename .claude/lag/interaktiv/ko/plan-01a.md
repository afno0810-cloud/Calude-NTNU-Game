# Plan 01a (Interaktiv verden): grunnmuren – kartets tilstand, bruk-tasten og dører

Runde 1 er delt i to (hovedagentens avgjørelse etter forhåndssjekken `kontroll/forhand-plan-01.md`): **1a** (denne) kan
bygges og prøves helt uten Kamp; **1b** (`plan-01b.md`: glass, biter, skår, kulehull, modeller) bygger på 1a.
Den gamle `plan-01.md` er bare en peker nå.

Les først: `.claude/lag/interaktiv/retning.md`, `.claude/lag/grafikk/helgasetr.md` kap. 4–6 og 9,
`.claude/lag/grafikk/ko/plan-04.md` steg 1g/7 (dørplasser med id), og forhåndssjekken. Koordinater i Helgasetr-dataene er
meter (X øst, Y sør, Z opp); i Unreal cm.

## Mål
Spilleren åpner og lukker dører med E, sparker opp og inn dører, låser opp grupperommenes glassdører med adgangskort,
dirkesett eller ved å bryte opp kodelåsen, og kommer seg alltid ut innenfra – likt for alle online, også for den som
kommer inn midt i kampen.

## Forbedrer (gammelt)
- **Dirkesett og NTNU-adgangskort** (Spillbarhets gjenstander) åpnet bare låste kasser; nå åpner de også kodelåser på dører.
- **Bruk-tasten E** (Kamps `Interact`) virket bare på loot og PC-er; nå også på dører, med hint som følger samme stil.
- **Bots** som går mot en lukket dør blir ikke stående: ulåste dører åpnes for dem.

## Nytt
- `AGSMapState`: én replikert tilstand for alle interaktive deler (`FFastArraySerializer` med bare endrede deler), sen
  klient får riktig tilstand, tilbakestilles ved kampstart.
- `IGSInteract` + `AGSInteractor` (én per spiller: fokus, trykk/hold, framdrift, hint, server-RPC).
- `AGSDoor`: tre-, glass-med-kodelås-, dobbel- og branndører; åpne/lukke vekk fra deg, sparke, låst, kort/dirk/bryte opp,
  rømning innenfra, branndører som lukker seg (men aldri oppå en spiller).
- Testrom med alle dørtypene, og konsollkommandoer for å prøve alt (`Interaktiv.Testrom`, `.Smell`, `.Knus`, `.Vis`,
  `.Tilbakestill`).

## Hvorfor nå
Andreas: «Fiks så mye på mappet er interaktivt». Dørene og den felles tilstanden er grunnmuren for alt annet (glass i 1b,
sprengning og bæring i runde 2, brytere og heiser i runde 3, Spillbarhets loot bak låste dører, Bots). Alt i 1a kan prøves
uten krokene fra andre lag.

## Filer
Nye (lagets egne): `Source/Gloshaugen/Public/GSMapState.h`, `Private/GSMapState.cpp`, `Public/GSInteract.h`,
`Private/GSInteract.cpp`, `Public/GSDoor.h`, `Private/GSDoor.cpp`. Felles, bare tillegg: `IMPROVEMENTS.md` (646–661).
**Ingen andre lags filer endres.** (`GSBreakable.*` og Blender-modellene kommer i 1b.)
Leses: `GSCharacter.h` (`Backpack`, `GetFocusPickup`, `GetFocusTerminal`, `UseItemId`, `bDead`), `GSPlayerController.h`
(`InteractAction`, `IsOnTerminal`, `IsGameInputBlocked`, `bShowInventory`), `GSItems.h`, `GSPickup.h` (`SpawnDropped`),
`GSAudio.h` (`PlayAt`), `GSGameState.h` (`Phase`), `GSHelgasetr.h` (når den finnes).

## Steg

### 0. Felles navn og nøkler (`GSMapState.h`)
- `namespace GSPart`: `Glass = 1`, `Door = 2`, `Breach = 3`, `Prop = 4`, `Device = 5`. `MakeKey(Cat, Id) = (Cat << 24) |
  (Id & 0xFFFFFF)`, `CatOf`, `IdOf`. Testrommets id-er starter på `TestIdBase = 0xF00000` i hver kategori.
- `EGSGlass : uint8 { Railing, Wall, DoorPane }` (brukes fra 1b, men legges inn nå så headeren er ferdig i én omgang).
- `EGSDoorArt : uint8 { Wood, GlassCode, Double, Fire }`. Lås: 0 åpen, 1 låst, 2 ødelagt.
- Alle nye `enum`/`struct` i lagets egne headere, ikke i `GSTypes.h`. Gjør headerne ferdige i én omgang (andre lag bygger samtidig).

### 1. `AGSMapState` – `GSMapState.h/.cpp`
- `USTRUCT FGSPartState : FFastArraySerializerItem`: `int32 Key; uint8 State; int8 Open; uint8 Health; uint8 Flags;
  uint8 Cause; FVector_NetQuantizeNormal HitDir; float ChangedAt;` (server-tid).
  - Dør: `State` bit 0–1 lås, bit 2 = dørglasset knust (1b), bit 3 = bladet revet ut (runde 2); `Open` −100…100 (prosent av
    95°, fortegn = side); `Health` teller spark/treff; `Flags` bit 0 = noen dirker/bryter.
  - Glass (1b): `State` 0 hel / 1 sprukket / 2 borte, `Health` = skade.
- `USTRUCT FGSPartArray : FFastArraySerializer` med `PostReplicatedAdd/Change/PreReplicatedRemove` → `AGSMapState::ApplyLocal`.
- `UCLASS AGSMapState : AInfo`: `bReplicates`, `bAlwaysRelevant`, `NetUpdateFrequency = 20`, `NetPriority = 2`.
  Replikert: `Parts`, `int32 ResetCount` (OnRep), `bool bTestRoom` (OnRep). `BeginPlay` rører aldri `Parts` (lærdom:
  `OnRep` kan komme før `BeginPlay`).
- **Lokalt register** (hver maskin): `TMap<int32, FGSPartRef>` (`AGSDoor*` nå; glass i 1b). `BuildRegistry()` prøves hvert
  0,5 s (maks 20 s) til kartet er bygd (byggeren bygger synkront i `GSWorldSubsystem::OnWorldBeginPlay`), og én gang til når
  `bTestRoom` blir sann. Etterpå `ApplyAllLocal()` for alt i `Parts` og `PendingKeys`.
- **Fersk eller gammel (forhåndssjekk 3):** full effekt (lyd, rask sving, biter i 1b) bare når
  `0 ≤ ServerTime − ChangedAt < 2 s` **og** elementet ikke kom i klientens første replikering av lista (`bFirstRepDone`
  settes etter første `PostReplicatedAdd`-runde / første `OnRep` – bruk et flagg som settes i `PostNetReceive` første gang),
  **og** det ikke ble brukt via `PendingKeys`/`ApplyAllLocal`. Alt annet: bare sluttutseendet (døra står der den står, ingen lyd).
- **Server-API** (statiske, ingenting på klienter, ingenting med figurer, tåler å bli kalt før kartet finnes):
  `Get(Ctx)`; `ApplyBlast(World, OriginCm, RadiusCm, Strength)` (1a: dører innen R slår opp vekk fra smellet, lås 2;
  glass i 1b); `ApplyBulletHit(World, Hit, Damage)` (1a: kodelås/lås på dør); `ApplyMelee(User, Start, End, Damage)` →
  returnerer `float BlockDistCm` (avstand til første hele glassrute langs slaget, `BIG_NUMBER` når ingen; i 1a alltid
  `BIG_NUMBER`; 1a: treff på dørblad = ett spark, på kodelås = slag); `ResetForMatch(World)`; `EnterTestRoom(PC)`;
  `IsDoorPassable(World, PosCm)`, `ForEachDoor(World, Fn)` (Bots). `BulletSegment` og `AddBulletMark` deklareres nå med
  tom kropp (1b fyller dem), så Kamp kan legge inn K1 når de vil.
- Delegater (statiske, hentes med funksjon): `OnWalkableChanged()` `(UWorld*, const FBox&)` når en dør låses opp/brytes;
  `OnNoise()` `(UWorld*, FVector LocCm, float LoudnessM)` ved spark, brekking, smell.
- **Endre tilstand** (server): `SetPart(Key, Mutator)` → `ChangedAt`, `MarkItemDirty`, og `ApplyLocal(Item, bFresh=true)`
  selv (FastArray-tilbakekall kjører ikke på serveren).
- **Tilbakestilling:** `ResetForMatch` tømmer `Parts` (`MarkArrayDirty`), øker `ResetCount`, avbryter dørjobber,
  `ResetLocal()` på serveren. `OnRep_ResetCount` → `ResetLocal()` (alle dører til standard) + `ApplyAllLocal()`;
  `PreReplicatedRemove` setter også delen tilbake, så begge rekkefølger ender riktig. **Uten Spillbarhets krok:** serveren
  leser `AGSGameState::Phase` hvert 0,5 s; overgang fra `Lobby`/`Ended` til `Countdown`/`Playing` → `ResetForMatch`.
- **Hvem lager den:** `UGSMapStateSubsystem : UWorldSubsystem` (i vår fil) – bare spillverdener; `OnWorldBeginPlay` → ikke
  klient → `SpawnActor<AGSMapState>`. `GSWorldSubsystem` (Grafikk) røres ikke.
- **Bots ved dører:** 5 Hz på serveren: hver `AGSCharacter` med `!IsPlayerControlled()` (lærdom 14a) innen 1,2 m foran en
  **lukket og ulåst** dør, innen 60° av normalen mot figuren → døra åpnes vekk fra den.
- **Konsollkommandoer** (`FAutoConsoleCommandWithWorldAndArgs`, bare vert/alene; i PIE via §-konsollen):
  `Interaktiv.Testrom [alle]`, `Interaktiv.Tilbakestill`, `Interaktiv.Vis` (id + tilstand med `DrawDebugString` innen
  15 m, 10 s), **`Interaktiv.Smell [radius m, standard 3]`** (`ApplyBlast` der du ser, første treff i et Visibility-spor
  ≤ 30 m) og **`Interaktiv.Knus [skade, standard 40]`** (gir delen du ser på et kuletreff: i 1a dør/kodelås via
  `ApplyBulletHit`; 1b utvider den til glass via `DamageGlass`). Begge virker uten Kamps kroker.

### 2. `IGSInteract` og `AGSInteractor` – `GSInteract.h/.cpp`
- `IGSInteract`: `GetInteractKey()`, `GetUsePoint(const FVector& UserEye)` (**følger brukerens side** – forhåndssjekk 4),
  `GetUsePrompt(const AGSCharacter* User, FGSUsePrompt& Out)` (`Main`, `Sub`, `Color`, `bHasHold`), `ServerUse(User, Verb)`
  (`EGSUseVerb { Tap, HoldStart, HoldEnd }`), `PredictUse(User)`.
- `AGSInteractor : AActor`, én per menneskelig spiller: server lager dem fra `AGSMapState` (hvert 1 s, `SetOwner(PC)`,
  fjerner for PC-er som er borte). `bReplicates`, `bOnlyRelevantToOwner`.
- **Bare hos eieren** (`PC->IsLocalController()` – verten har interaktorer for klientene sine; de binder ikke input og
  tegner ikke): `UEnhancedInputComponent` med `IA_Interact` (`AGSPlayerController::InteractAction`, ellers
  `FindObject<UInputAction>(PC, TEXT("IA_Interact"))`, prøv hvert 0,5 s til den finnes), bundet til `Started`;
  `PushInputComponent`; `PopInputComponent` i `EndPlay`. Ingen `bBlockInput`, ingen konsumering (motoren konsumerer ikke
  som standard – sjekket i forhåndssjekken).
- **Eierskap ved trykket (forhåndssjekk 1):** i `Started` avgjøres med én gang om trykket er vårt: `bPressMine = vi har
  fokus && !sperret && (Kamp har ikke pickup/terminal i fokus || bCharacterHooked)`. Ikke vårt → hele trykket ignoreres til
  tasten er sluppet (ingen `Tap` ved slipp selv om fokus skifter). Slipp/hold måles i `Tick` med `PC->IsInputKeyDown(K)` for
  tastene `QueryKeysMappedToAction` gir: slipp < 0,3 s = `Tap`; 0,3 s = `HoldStart` (én gang); slipp etter = `HoldEnd`.
- **Sperrer (forhåndssjekk 2, som Kamps):** `PC->IsGameInputBlocked() || PC->bShowInventory || PC->IsOnTerminal()`,
  pawnen død eller mangler, **eller `C->UseItemId` er satt** (bandasjering osv.) – da verken fokus, hint eller trykk.
- **Fokus (20 Hz):** kandidater innen 2,2 m; poeng `Dot − Dist/1000` med `Dot > 0,8` (eller `Dist < 0,9 m` og `Dot > 0,5`)
  mot `GetUsePoint(Eye)`. Fri vei: **objektspor** `LineTraceSingleByObjectType` mot `WorldStatic | WorldDynamic |
  Destructible` fra øyet til brukspunktet, med døra selv unntatt (forhåndssjekk 5 – glass ignorerer `Visibility`, så et
  kanalspor ville sett gjennom glassvegger). Døra får fokus bare hvis den slår beste `AGSPickup`/`AGSTerminal` vi regner med
  Kamps formel. `GSInteract::HasFocus(const AGSCharacter*)` (Kamps K4) setter `bCharacterHooked = true`.
- **Server:** `ServerUse(Key, Verb)` (Reliable): pawn lever, nøkkel finnes, `UseItemId` tom, avstand fra
  `GetPawnViewLocation()` til `GetUsePoint` ≤ 250 cm, **samme objektspor** fra pawnens øye (med døra unntatt), tidssperre
  0,2 s → `IGSInteract::ServerUse`. `GSInteract::BotUse(Bot, Key, Verb)`: samme sjekker uten RPC.
- **Jobb:** `FGSUseJob { Key; Kind; Start; End; }`, `COND_OwnerOnly`. Avbrytes ved `HoldEnd`, avstand > 1,8 m, død, at en
  gjenstand tas i bruk, eller at døra endrer seg. Fullføres av serverens klokke i `Tick`.
- **Tegning:** `AHUD::OnHUDPostRender.AddStatic` (som `GSHelp`), bare for HUD-en til den lokale eieren. Hint midt på,
  `SizeY*0,5 + 60` (Kamps `DrawPrompt`-plass – aldri samtidig, se fokus), linje 2 mindre og grå. **Framdriftslinja** på
  `SizeY*0,5 + 110` (under hintet). **Kamps `DrawUseProgress` står midt på `SizeY*0,5 + 40`** (GSHUD.cpp ca. l. 449);
  vi tegner ingenting når `UseItemId` er satt, så de møtes aldri. `GSHelp`-hintene står ikke midt på. Tast via
  `QueryKeysMappedToAction` → `GetDisplayName`. Font `GEngine->GetMediumFont()`, 1 px mørk skygge.

### 3. `AGSDoor` – `GSDoor.h/.cpp`
- Lokal aktør (`bReplicates = false`), lagd på hver maskin av `AGSMapState` fra dørdataene, samme nøkkel overalt. `Tick`
  bare mens den beveger seg eller har en tidtaker.
- Komponenter: `Root` i hengselen; `Leaf` (mesh, ingen kollisjon; i 1a kube med `MI_GS_WoodLight`/`MI_GS_PaintWhite`/
  `MI_GS_MetalDark`, i 1b Blender-modeller); `LeafBox` (`UBoxComponent`, `WorldDynamic`, `BlockAll`,
  `SetCanEverAffectNavigation(false)`); `Handle`. **`GlassCode`:** `LeafBox` er bare rammen (fire tynne bokser), og en
  **`Pane` – vanlig `UStaticMeshComponent` på bladet** (forhåndssjekk, mindre funn), objekttype `ECC_Destructible`, blokkerer
  `Pawn`/`PhysicsBody`/`WorldDynamic`, ignorerer `Visibility`/`Camera`/`WorldStatic`-sporet, tag `Mat_Glass`, materiale
  `MI_GS_Glass` (eller det gjennomsiktige MI-et `gs_materials.py` har). I 1a kan den ikke knuses ennå (1b). `Keypad` på
  veggen ved håndtaket **på utsiden**, 1,1 m, 8 × 14 × 3 cm, `UBoxComponent` som blokkerer `Visibility` (så kuler og
  `Interaktiv.Knus` treffer den), tag `GS_Int_Keypad`, **lysdiode som emissivt materiale** (dynamisk MI, grønn/rød), ikke
  punktlys. `Double`: to blad, hengsel i hver side.
- **Bevegelse:** mål `Open/100 × 95°`; 170°/s vanlig, 650°/s spark/smell, branndør som lukker seg 60°/s med smell. Mens bladet
  går ignorerer `LeafBox` og `Pane` `Pawn`; ved stopp `Block`. Lyd `Door_Open` (start), `Door_Close` (når 0), `Door_Slam`
  (spark) – bare når ferskt.
- **Side:** `Side = sign((Øye − Root) · Normal)`; utsiden = forsiden (`Yaw`). Døra åpnes alltid vekk fra brukeren.
- **Bruk (server):**
  | Tilstand | `Tap` | `HoldStart` |
  |---|---|---|
  | lukket, ulåst | åpne | spark opp (650°/s, `OnNoise` 35 m, `Health+1`) |
  | åpen | lukk | – |
  | låst, **fra innsiden** (alle arter) | åpne med vrider/panikkstang; låsen forblir låst (døra smekker i lås igjen når den lukkes) – forhåndssjekk 4 | – |
  | låst `GlassCode` utenfra, har `key` | `Keypad_Beep`, lås 0, åpne | – |
  | låst `GlassCode` utenfra, uten kort | «Låst. Kodelås.» + `Keypad_Error` | `lockpick`: jobb 6 s → lås 0; uten: bryt opp, jobb 4 s, `Keypad_Break` hvert 1 s, `OnNoise` 30 m → lås 2 |
  | låst `Wood` utenfra | «Låst.» | spark: jobb 1,0 s per spark, 3. spark → lås 2, døra slår opp; `Door_Kick`, `OnNoise` 40 m |
  | låst `Fire` (alarmdør) utenfra | «Alarmdør. Kan ikke åpnes herfra.» | – (sprengning i runde 2) |
  Kort og dirkesett brukes ikke opp. Gjenstander: `GSItems::Find(S.Id)->Kind == "key"/"lockpick"` i `User->Backpack`.
  Innsiden: dører med `Laast` fra dataene (og alle `GlassCode`) har en innside = baksiden (`−Normal`).
- **Skudd/slag/smell:** `ApplyBulletHit` på `Keypad` → lås 2 (oransje gnist `AGSShortFX::SpawnSprite`, `Keypad_Break`); på
  `LeafBox` innen 12 cm fra håndtaket på låst `Wood` → lås 2. `ApplyMelee` (sveip 22 cm mot `LeafBox`/`Keypad`) = ett spark /
  slag (2 slag på kodelåsen → lås 2). `ApplyBlast` innen R: Open 100 vekk fra smellet, lås 2, 650°/s.
- **Branndører** lukker seg 6 s etter åpning – **men bare når ingen `Pawn` overlapper svingboksen** (bladets bredde × bredde
  på begge sider); ellers prøv igjen hvert 1 s.
- **`PredictUse`** (eierens maskin) bare når replikert tilstand sier ulåst (eller brukeren står på innsiden) og ingen jobb;
  lokalt mål i 1 s, ellers tilbake.
- **Dirkelyd:** `Flags` bit 0 → alle innen 12 m: `Lock_Pick` hvert 0,8–1,4 s.
- **Hint:** «[E]  Åpne døra» / «Hold [E]: spark opp»; «[E]  Lukk døra»; utenfra låst grupperom: «Grupperom – låst med
  kodelås» / «[E] adgangskort · Hold [E]: dirk opp» eller «Hold [E]: bryt opp kodelåsen (høylytt)»; «Låst dør» / «Hold [E]:
  spark inn»; «Alarmdør – låst»; innenfra: «[E]  Åpne døra». Låst = gul (1, 0,85, 0,4), ellers hvit (0,92).
- **Standard:** lukket; `GlassCode` låst; `Laast` i dataene låst; `Aapen` i dataene åpen.
- **Lyd med reserve:** `GSMapState::PlayPartSound(Ctx, Name, Loc, Vol, RangeM, Pitch)` sjekker én gang med `LoadObject`
  (`LOAD_NoWarn | LOAD_Quiet`, svaret i en `TMap`) om `/Game/Sounds/S_GS_<Name>` finnes, ellers reserve: `Door_Kick`/`Slam` →
  `Land` pitch 0,7; `Door_Open`/`Close` → `Reload_In`/`Reload_Out` pitch 0,6 vol 0,5; `Lock_Pick` → `Reload_In` pitch 2,2
  vol 0,3; `Keypad_Beep` → `Pickup` pitch 1,6; `Keypad_Error` → `Dry_Fire` pitch 1,3; `Keypad_Break` → `Impact_2` pitch 1,3.

### 4. Data – i `GSMapState.cpp`
- `CollectMapDefs(TArray<FGSDoorDef>&)` (1b legger til glass): `FGSDoorDef { Id; BaseCm; WidthCm; HeightCm; YawDeg; Art;
  bHingeLeft; bLocked; bOpen; }`. Bak `#if __has_include("GSHelgasetr.h")`: les `GSHelgasetr::DoorSlots()` (plan-04 kaller
  strukturen `FDoorM`; **les headeren** og bruk feltnavnene som finnes). Mangler yaw/hengsel/låst/åpen (bestilt, ikke i
  plan-04): yaw 0 hvis bredden ligger langs Y, ellers 90 (logg «yaw gjettet»), hengsel venstre, låst bare `glass_kode`.
  Byggefeil i blokken rettes her, aldri i `GSHelgasetr.h`. Ingen blokk/data → bare testrommet, logg «ingen kartdata».
- `BuildRegistry()`: `AGSDoor` per dør (lokal `SpawnActor`), registrer nøkler, logg «Interaktiv: N dører».

### 5. Testrommet (bare dører i 1a)
- `bTestRoom` replikert; alle maskiner bygger det likt. Origo `O = (0, 0, 6000)` cm (over taket, innenfor `PlayArea`).
  Kuber i ISM (`BlockAll`), `MI_GS_Terrazzo` gulv, `MI_GS_PlasterWhite_In` vegger/tak (byggeren sjekker navnene), 2 punktlys
  uten skygge. ISM-oppdateringer med `bMarkRenderStateDirty = false`.
- Rom 16 × 12 × 3,4 m med tak. Tverrvegg med 4 dører: `Wood` ulåst, `Wood` låst, `GlassCode` låst (innside = bortre side),
  `Double` ulåst. Endevegg: `Fire` ulåst (lukker seg) inn til trapperom 3 × 3 m, og `Fire` låst (alarmdør, panikkstang
  innenfra – kom inn i trapperommet gjennom den første). Bord med `Keycard` og `Lockpick` (`AGSPickup::SpawnDropped`, server,
  legges ut på nytt hver gang kommandoen kjøres).
- `EnterTestRoom`: pawnen til `O + (−6, 0, 0,2) m` (`TeleportTo`); `alle` flytter alle.

### 6. Bygg og prøv
1. `Build.bat … -WaitMutex` (byggeren, editoren lukket). Rett bare egne filer.
2. PIE (Selected Viewport), vert: `Interaktiv.Testrom`. Alle seks dørene: trykk og hold E, kort og dirkesett fra bordet,
   bryt opp kodelåsen uten verktøy, spark inn den låste tredøra, kom ut av grupperommet og trapperommet innenfra,
   `Interaktiv.Knus` på kodelåsen og ved låsen på tredøra, `Interaktiv.Smell` mellom dørene.
3. **Input (forhåndssjekk 14):** pickup på gulvet med dør bak – trykk og slipp E: bare plukket opp. Inventaret åpent,
   P/F10-menyen åpen, bandasjering ved døra: E gjør ingenting med døra, ingen hint over Kamps framdrift. Esc/F10 lukker.
4. Stå i branndøras åpning: den lukker seg ikke før du går.
5. To spillere (vert + klient): klienten åpner/dirker/sparker → verten ser det; omvendt. **Sen klient:** bryt opp to dører
   og la en stå åpen, koble til spiller 2 → riktig utseende, ingen lyd og ingen svingning fra gamle hendelser.
6. `Interaktiv.Tilbakestill` og en ny CTF-kamp (Lobby → Playing): alle dører tilbake.
7. En bot (hvis Bots finnes) eller AI-figur går gjennom ulåste dører.

## Ferdig når
- Bygger uten feil eller nye advarsler i egne filer; ingen andre lags filer endret.
- Alle dørtypene oppfører seg som tabellen i testrommet, med reservelyd; man kommer alltid ut innenfra.
- E gjør aldri to ting; ingen dørhandling eller hint mens meny, inventar, terminal eller gjenstandsbruk er aktiv.
- Online: likt hos vert og klient, sen klient riktig uten gamle effekter, tilbakestilling virker også uten Spillbarhets krok.
- Med Helgasetr-data: loggen viser antall dører; uten: «ingen kartdata – bare testrommet», ingen feil.
- `logg.md`, `retning.md` og `IMPROVEMENTS.md` (646–661) oppdatert.

## Risiko
- **Rekkefølge online:** register ikke bygd når lista kommer → `PendingKeys` + `ApplyAllLocal` (aldri ferske).
- **Lokale dører:** samme sluttilstand fra samme `Open`; en liten retting når en dør lukkes (serverens blad blokkerer
  ca. RTT/2 før klientens) – godtatt (forhåndssjekken).
- **Input:** prøv i PIE, ikke bare kompiler (lærdom). Kamps `Interact` og vår binding får begge `Started`; `bPressMine`
  avgjør ved trykket.
- **Juks:** klienten sender bare nøkkel + verb; serveren sjekker avstand, objektspor, tid, sekk; jobber fullføres av serverklokka.
- **Navn i 4a** kan avvike; alt er samlet i én funksjon bak `__has_include`.

## IMPROVEMENTS-linjer (646–661, egen seksjon øverst: «Runde 1a (Interaktiv verden): dører og bruk-tasten (27. nov.)»)
646. Ny felles tilstand for alt som kan åpnes og ødelegges i kartet, lik for alle spillere online
647. Den som kommer inn midt i kampen ser åpne dører og brutte låser slik de er, uten gamle lyder
648. Alle dører og låser settes tilbake når en ny kamp starter
649. Bruk-tasten (E) virker på dører: trykk for å åpne eller lukke, hold for å sparke
650. E gjør aldri to ting: den tingen du ser på når du trykker, er den som brukes
651. Dører går opp vekk fra deg, med lyd når de åpnes og smekker igjen
652. Dører kan sparkes opp med et høyt smell som andre hører
653. Låste tredører kan sparkes inn (tre spark) eller skytes opp ved låsen
654. Glassdørene til grupperommene har kodelås: NTNU-adgangskort åpner med et pip
655. Kodelåsen kan dirkes opp med dirkesett (6 s) eller brytes opp med makt (4 s, høylytt), eller skytes i stykker
656. Låste dører og alarmdører kan alltid åpnes innenfra, som ekte rømningsdører
657. Branndører i trapperommene lukker seg selv, men aldri oppå noen som står i døråpningen
658. Granater blåser opp dører i nærheten og ødelegger låsene
659. Kort hint midt på skjermen ved dører med framdriftslinje ved dirking og brekking
660. Botene åpner ulåste dører de går mot
661. Testrom for dørene (konsoll: Interaktiv.Testrom, Interaktiv.Smell, Interaktiv.Knus)
