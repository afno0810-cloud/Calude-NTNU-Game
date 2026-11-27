# Plan 01 (Interaktiv verden): grunnmuren – kartets tilstand, bruk-tasten, dører, glass som knuses og varige kulehull

Les først: `.claude/lag/interaktiv/retning.md`, `.claude/lag/grafikk/helgasetr.md` kap. 4 (glassboksene, grupperommene
med kodelås), kap. 6 (interaktivitet – laget eier den nå), kap. 9 (ytelse) og `.claude/lag/grafikk/ko/plan-04.md` steg 1g
og 7 (glassfelt og dørplasser med id). Koordinater i Helgasetr-dataene er meter (X øst, Y sør, Z opp); i Unreal cm.

## Mål
Spilleren åpner og lukker dører med E, sparker inn låste dører, låser opp grupperommenes glassdører med adgangskort,
dirkesett eller ved å bryte opp kodelåsen – og glass i rekkverk, glassbokser og dører knuses av kuler, slag og granater
med lyd og skår på gulvet, mens kulehull blir sittende i veggene. Alt likt for alle spillere online, også for den som
kommer inn midt i kampen.

## Forbedrer (gammelt)
- **Kulenedslaget** (`AGSShortFX::SpawnImpact`, Kamp): «hullet» er i dag en mørk flis som forsvinner etter 40 s. Nå blir
  hullet sittende (varig merke per underlag: gips/betong, tre, metall, sprekkstjerne i laminert glass) til kampen
  startes på nytt, med tak per maskin.
- **Dirkesett og NTNU-adgangskort** (Spillbarhets gjenstander) åpnet bare låste kasser; nå åpner de også kodelåser på dører.
- **Glassfeltene i Helgasetr** (Grafikks plan-04: hele klosser som kulene går rett gjennom uten spor) blir ekte glass.
- **Granater** (`GSProjectile::Explode`, Kamp) skader bare spillere i dag; nå knuser trykkbølgen glass og blåser opp dører.

## Nytt
- `AGSMapState`: én replikert tilstand for alle interaktive deler (endrede deler i en `FFastArraySerializer`), sen
  klient får riktig tilstand, tilbakestilles ved kampstart.
- `IGSInteract` (felles bruk-grensesnitt) og `AGSInteractor` (én per spiller: fokus, bruk-tasten, trykk/hold, framdrift,
  hint på skjermen, server-RPC-er).
- `AGSDoor`: tre-, glass-med-kodelås-, dobbel- og branndører; åpne/lukke (vekk fra deg), sparke opp/inn, låst, kort/dirk/
  bryte opp/skyte låsen, branndører som lukker seg selv.
- Knusbart glass: herdet (smuldrer helt i ett treff) og laminert (sprekker, gir etter til slutt).
- Varige kulehull (lokal pool med tak).
- Testrom (`Interaktiv.Testrom`) med alle dør- og glasstypene, så alt kan prøves før Helgasetr er bygget.
- Modeller: `Tools/Blender/interactive_doors.py` (dørblad, håndtak, kodelås, skår, glassgranulat).

## Hvorfor nå
Andreas (27. nov.): «Fiks så mye på mappet er interaktivt», «vinduer kan knuse», «man kan skyte hull i vegger»; han likte
glass som knuser fra før. Grafikk bygger nå Helgasetr (4a/4b) med glassfelt og dørplasser som egne id-er – denne runden er
grunnmuren alt det andre (runde 2–3, Spillbarhets dører/loot, Bots) står på. Testrommet gjør at vi ikke venter på 4a.

## Filer
Nye (alle lagets egne): `Source/Gloshaugen/Public/GSMapState.h`, `Private/GSMapState.cpp`, `Public/GSInteract.h`,
`Private/GSInteract.cpp`, `Public/GSDoor.h`, `Private/GSDoor.cpp`, `Public/GSBreakable.h`, `Private/GSBreakable.cpp`,
`Tools/Blender/interactive_doors.py`, modellene `SourceArt/Models/SM_Int_*.fbx` (+ .blend).
Felles, bare tillegg: `IMPROVEMENTS.md` (646–661, egen seksjon øverst). `Gloshaugen.Build.cs` trenger ingen endring
(`NetCore`, `EnhancedInput` finnes). **Ingen andre lags filer endres** – krokene er bestilt i `bestillinger.md`.

Leses (ikke endres): `GSCharacter.h` (`Backpack`, `GetFocusPickup`, `GetFocusTerminal`, `bDead`, `GetTeam`),
`GSPlayerController.h` (`InteractAction`, `IsOnTerminal`), `GSItems.h` (`Find`, `Kind`), `GSPickup.h` (`SpawnDropped`),
`GSAudio.h` (`PlayAt`), `GSEffects.h` (`AGSShortFX::SpawnSprite`), `GSGameState.h` (`Phase`), `GSHelgasetr.h` (når den finnes).

## Steg

### 0. Felles navn og nøkler (`GSMapState.h`)
- `namespace GSPart`: kategorier `Glass = 1`, `Door = 2`, `Breach = 3` (runde 2), `Prop = 4` (runde 2), `Device = 5` (runde 3).
  Nøkkel: `int32 MakeKey(uint8 Cat, int32 Id) { return (Cat << 24) | (Id & 0xFFFFFF); }`, `CatOf`, `IdOf`.
  Testrommets id-er starter på `TestIdBase = 0xF00000` i hver kategori (kolliderer aldri med kartets id-er).
- Glasstyper: `EGSGlass : uint8 { Railing /*laminert, rekkverk*/, Wall /*herdet, boks-/romvegg*/, DoorPane /*herdet, i dør*/ }`.
- Dørarter: `EGSDoorArt : uint8 { Wood, GlassCode, Double, Fire }`. Lås: `0 = åpen lås, 1 = låst, 2 = lås ødelagt`.
- Merk: alle nye `enum`/`struct` ligger i lagets egne headere, ikke i `GSTypes.h`.

### 1. `AGSMapState` (replikert tilstand) – `GSMapState.h/.cpp`
- `USTRUCT FGSPartState : FFastArraySerializerItem` – bare deler som er **endret** fra standard står i lista:
  `int32 Key; uint8 State; int8 Open; uint8 Health; uint8 Flags; uint8 Cause; FVector_NetQuantizeNormal HitDir;
  float ChangedAt;` (server-tid fra `GetServerWorldTimeSeconds()`).
  - Glass: `State` 0 hel / 1 sprukket / 2 borte; `Health` = skade (laminert tåler 100); `HitDir` = retning skåra flyr.
  - Dør: `State` bit 0–1 lås (0/1/2), bit 2 = dørglasset knust; `Open` −100…100 (prosent av 95°, fortegn = side);
    `Health` = antall spark/treff (teller opp, gir «dunk» hos alle); `Flags` bit 0 = noen dirker/bryter nå.
  - `USTRUCT FGSPartArray : FFastArraySerializer` med `PostReplicatedAdd/Change/PreReplicatedRemove` som kaller
    `AGSMapState::ApplyLocal(Item, bRemoved)`; `NetDeltaSerialize` som vanlig.
- `UCLASS AGSMapState : AInfo`: `bReplicates`, `bAlwaysRelevant`, `NetUpdateFrequency = 20`, `NetPriority = 2`.
  Replikert: `Parts` (FGSPartArray), `int32 ResetCount` (OnRep), `bool bTestRoom` (OnRep).
- **Lokal register** (på hver maskin, ikke replikert): `TMap<int32, FGSPartRef>` der `FGSPartRef` peker på
  glassfelt (`UGSGlassSet*` + instansindeks) eller `AGSDoor*`. Bygges av `BuildRegistry()` (steg 6) – med en tidtaker
  hvert 0,5 s til kartet finnes (byggeren lager kartet i sin egen `BeginPlay`), maks 20 s, og så én gang til når
  `bTestRoom` blir sann. Når registeret er (gjen)bygd: `ApplyAllLocal(bFresh=false)` for alt som alt står i `Parts`
  (sen klient: riktig utseende, ingen biter og lyd).
- **Server-API** (statiske, gjør ingenting på klienter – sjekk `World->GetNetMode() != NM_Client`):
  - `static AGSMapState* Get(const UObject* Ctx)`.
  - `static void ApplyBlast(UWorld*, FVector OriginCm, float RadiusCm, float Strength)` (Kamp, steg 3).
  - `static void ApplyBulletHit(UWorld*, const FHitResult& Hit, float Damage)` (Kamp: ekte kule traff noe som ikke er en figur).
  - `static float BulletSegment(UWorld*, const FVector& From, const FVector& To, const FVector& Vel, float Damage, bool bReal)`
    – finner glass langs kulas strekning (steg 3), returnerer fartsfaktor (1 = ingenting, 0,92 per rute). Virker på
    både server (`bReal`: skader glasset) og klient (`!bReal`: bare lokalt glimt/sprekkemerke på glass som fortsatt er helt).
  - `static void ApplyMelee(AGSCharacter* User, const FVector& Start, const FVector& End, float Damage)` (Kamp, slag/kolbe).
  - `static void AddBulletMark(UWorld*, const FHitResult& Hit)` – lokal, alle maskiner (Kamp: vist kule traff vegg).
  - `static void ResetForMatch(UWorld*)` (Spillbarhet; virker også uten, se under).
  - `static void EnterTestRoom(APlayerController* PC)` (slår testrommet på og flytter PC-ens pawn dit; brukes av
    konsollen og kan brukes av Spillbarhets «Test kartet»-meny).
  - `static bool IsDoorPassable(UWorld*, const FVector& NearCm)` og `static void ForEachDoor(UWorld*, TFunctionRef<void(const AGSDoor&)>)` (Bots).
  - Delegater (statiske, hentes med funksjon så rekkefølgen ved oppstart ikke betyr noe):
    `OnWalkableChanged()` → `(UWorld*, const FBox& BoxCm)` når en dør låses opp/ødelegges eller en glassvegg forsvinner
    (Bots oppdaterer navnettet); `OnNoise()` → `(UWorld*, FVector LocCm, float LoudnessM)` ved spark, knust glass, brekking
    (Bots kan «høre» det; rekkevidden i meter).
- **Endre tilstand** (bare server): `SetPart(Key, Mutator)` finner/legger til element, setter `ChangedAt`,
  `MarkItemDirty`, og kaller `ApplyLocal(Item, false)` selv (FastArray-tilbakekall kjører ikke på serveren – ellers ser
  verten ingenting).
- **`ApplyLocal(Item)`**: slår opp i registeret; ukjent nøkkel → legg i `PendingKeys` (brukes når registeret er bygd).
  `bFresh = (ServerTime - ChangedAt) < 2 s` → fulle effekter (biter, lyd); ellers bare sluttutseendet (sen klient).
  Glass → `UGSGlassSet::ApplyState`; dør → `AGSDoor::ApplyState`.
- **Tilbakestilling:** `ResetForMatch` tømmer `Parts` (`Items.Reset(); MarkArrayDirty()`), øker `ResetCount`, avbryter
  alle dirkejobber, og kaller `ResetLocal()` på serveren. `OnRep_ResetCount` → `ResetLocal()`: alle glassfelt hele, alle
  dører til standard (lukket, lås som i dataene), alle skår, biter og kulehull fjernes – og så `ApplyAllLocal(false)` for
  det som eventuelt alt står i `Parts`. (Rekkefølgen mellom lista og telleren kan komme vilkårlig; begge rekkefølger ender
  riktig fordi `PreReplicatedRemove` også setter delen tilbake til standard.)
  **Virker uten Spillbarhets krok:** serveren leser `AGSGameState::Phase` hvert 0,5 s; går den fra `Lobby`/`Ended` til
  `Countdown` eller `Playing`, kalles `ResetForMatch`. Kaller Spillbarhet den i tillegg, gjør to tilbakestillinger
  etter hverandre ingen skade (tom liste).
- **Hvem lager den:** `UGSMapStateSubsystem : UWorldSubsystem` (i `GSMapState.cpp/.h`): `ShouldCreateSubsystem` bare
  for spillverdener (`Game`/`PIE`); `OnWorldBeginPlay` → hvis ikke klient: `SpawnActor<AGSMapState>`. Klienter får den
  ved replikering. Ikke rør `GSWorldSubsystem` (Grafikk).
- **Bots som går mot dører:** serveren sjekker 5 ganger i sekundet hver figur som styres av en `AAIController` innen 1,2 m
  foran en lukket, ulåst dør (innenfor 60° av døras normal mot figuren): døra åpnes for den (som `Tap`). Låste dører:
  Bots bruker `IsDoorPassable` (bestilt). Gjør at botene ikke setter seg fast før Bots-laget har gjort noe.
- **Konsollkommandoer** (`FAutoConsoleCommandWithWorldAndArgs`, i `GSMapState.cpp`, virker bare på vert/alene):
  `Interaktiv.Testrom [alle]` (slår testrommet på og flytter deg dit, med `alle` alle spillere),
  `Interaktiv.Tilbakestill` (= `ResetForMatch`), `Interaktiv.Vis` (tegner id og tilstand over hver del innen 15 m i
  10 s med `DrawDebugString` – for kontrolløren), `Interaktiv.Merke` (legger et kulehull der du ser, for å prøve poolen
  før Kamps krok finnes).

### 2. `IGSInteract` og `AGSInteractor` – `GSInteract.h/.cpp`
- `UINTERFACE(MinimalAPI) UGSInteract` / `class IGSInteract`:
  - `virtual int32 GetInteractKey() const = 0;`
  - `virtual FVector GetUsePoint() const = 0;` (håndtaket/kodelåsen – der fokus og avstand måles)
  - `virtual bool GetUsePrompt(const AGSCharacter* User, FGSUsePrompt& Out) const = 0;` – `FGSUsePrompt { FString Main;
    FString Sub; FLinearColor Color; bool bHasHold; float HoldSeconds; }` (bygges lokalt av det klienten ser: replikert
    tilstand + egen sekk).
  - `virtual void ServerUse(AGSCharacter* User, EGSUseVerb Verb) = 0;` – `EGSUseVerb : uint8 { Tap, HoldStart, HoldEnd }`.
  - `virtual void PredictUse(AGSCharacter* User) {}` – lokal forhåndsvisning (dør begynner å gå med en gang).
- `UCLASS AGSInteractor : AActor` (én per menneskelig spiller):
  - Server lager den: `AGSMapState` går hvert 1 s gjennom `APlayerController`-ene og spawner en for hver som mangler
    (`SetOwner(PC)`), fjerner dem for PC-er som er borte. `bReplicates`, `bOnlyRelevantToOwner`, `bAlwaysRelevant=false`.
  - **Inndata (hos eieren):** i `BeginPlay` når `Owner` er en lokal PC: lag `UEnhancedInputComponent`, bind
    `IA_Interact` (`Cast<AGSPlayerController>(PC)->InteractAction`; hvis ikke tilgjengelig: `FindObject<UInputAction>(PC,
    TEXT("IA_Interact"))`) til `Started` og `Completed`, og `PC->PushInputComponent(...)`. Prøv på nytt hvert 0,5 s til
    handlingen finnes (`EnsureInputCreated` kan komme senere). **Slipp-sjekk uavhengig av triggere:** hold-tiden måles i
    `Tick` med `PC->IsInputKeyDown(K)` for tastene `UEnhancedInputLocalPlayerSubsystem::QueryKeysMappedToAction` gir
    (så det virker også om mappingen har en «Pressed»-trigger og `Completed` aldri kommer). Slipp før 0,3 s = `Tap`;
    holdt 0,3 s = `HoldStart` (én gang); slipp etter det = `HoldEnd`.
  - **Sperrer:** ingenting når pawnen er død/mangler, `PC->bShowMouseCursor` (meny/inventar åpen), eller
    `PC->IsOnTerminal()`.
  - **Fokus (hos eieren, 20 Hz):** øye = `PC->GetPlayerViewPoint`. Kandidater = registrerte `IGSInteract` innen 2,2 m
    (`AGSMapState` holder en liste). Poeng som Kamps `UpdateFocusPickup`: `Dot − Dist/1000`, krav `Dot > 0,8` (eller
    `Dist < 0,9 m` og `Dot > 0,5`), og fri sikt: `LineTraceSingleByChannel(Visibility)` fra øyet til `GetUsePoint()` som
    ignorerer døra selv. Vi regner også beste `AGSPickup` (`bAvailable`) og `AGSTerminal` med Kamps formel; døra får
    fokus bare hvis den har bedre poeng. **Før Kamps krok (K4)** viker vi i tillegg hver gang `C->GetFocusPickup()` eller
    `C->GetFocusTerminal()` ikke er tom (ellers ville E gjøre to ting). `GSInteract::HasFocus(const AGSCharacter*)`
    setter et statisk flagg `bCharacterHooked = true` første gang Kamp kaller den – da gjelder bare poengregelen.
  - **Server-RPC-er** (eieren eier aktøren, så de virker fra klient): `ServerUse(int32 Key, EGSUseVerb Verb)` (Reliable).
    Server: pawnen lever, nøkkelen finnes, `Dist(UsePoint, pawn-øye) ≤ 250 cm`, tidssperre 0,2 s per spiller
    (lærdom: alt klienten velger kan jukses) → `IGSInteract::ServerUse`.
  - **Jobb (dirking/brekking/spark):** serveren holder `FGSUseJob { int32 Key; uint8 Kind; float Start; float End; }`,
    replikert bare til eieren (`COND_OwnerOnly`) for framdriftslinja. Avbrytes ved `HoldEnd`, avstand > 1,8 m, død, eller
    at døra endrer seg. Fullføres i serverens `Tick`.
  - **Bots:** `GSInteract::BotUse(AGSCharacter* Bot, int32 Key, EGSUseVerb Verb)` – samme sjekker, uten RPC.
  - **Tegning (hos eieren):** `AHUD::OnHUDPostRender.AddStatic` (som `GSHelp`), bare for HUD-en til PC-en som eier
    interaktoren. Hint midt på, `SizeY*0,5 + 60` (samme sted som Kamps `DrawPrompt`, som da står tom fordi vi bare har
    fokus når Kamp ikke har det): linje 1 `[E]  Åpne døra` (tasten via samme oppslag som Kamp: `QueryKeysMappedToAction`
    → `GetDisplayName`), linje 2 i mindre, grå skrift (`Hold [E]: spark opp`). Framdriftslinje under (210 × 3 px, som
    `DrawUseProgress`) med tekst «Dirker opp låsen … 4 s». Font `GEngine->GetMediumFont()`, tekst med 1 px mørk skygge.
    **Hva andre tegner samme sted:** Kamps `DrawPrompt` (pickup/terminal, samme punkt – aldri samtidig, se fokus),
    Kamps `DrawUseProgress` (venstre side, y-liste – vi tegner midt på, ikke der), `GSHelp`-hint (øverst/nede til venstre
    – ikke midt på). Sjekk i PIE at ingenting overlapper.

### 3. Knusbart glass – `GSBreakable.h/.cpp`
- `UCLASS UGSGlassSet : UInstancedStaticMeshComponent` (én per glasstype per kilde: kartet og testrommet), eid av
  `AGSMapState` på hver maskin (lokal komponent, ikke replikert). Mesh `SM_Mod_Helg_GlassPanel` (Grafikk, 1 × 1 m, origo
  midt nede, front +X) skalert til lengde × høyde, tykkelse fra typen (rekkverk 12 mm, vegg/dør 10 mm); finnes den ikke:
  `/Engine/BasicShapes/Cube` skalert, materiale `MI_GS_Glass` (`/Game/GS/Materials/MI_GS_Glass`; finnes ikke det: et
  gjennomsiktig MI fra `gs_materials.py` – byggeren sjekker navnet).
  - **Kollisjon:** objekttype `ECC_Destructible` (brukes ikke av noe annet i koden), `Block` mot `Pawn`, `PhysicsBody`,
    `WorldDynamic`, `Destructible`; `Ignore` mot `Visibility`, `Camera`, `WorldStatic`-sporet. Da ser siktspor (bots,
    HUD) gjennom glasset, eksplosjonens siktsjekk i `DamageAround` (WorldStatic) går gjennom, men ingen går gjennom.
    Ingen skygge, `ComponentTags` `Mat_Glass` + `GS_Int_Glass`. `SetCanEverAffectNavigation(true)` (rekkverk og vegger
    stenger).
  - Instansindeks ↔ nøkkel: egen `TArray<int32> KeyOfInstance` og `TMap<int32,int32> InstanceOfKey`.
  - **Skjule et felt:** `UpdateInstanceTransform(I, T med skala 0, bWorldSpace, bMarkRenderStateDirty, bTeleport=true)` –
    motoren fjerner fysikklegemet for instanser med skala 0 (byggeren bekrefter i `InstancedStaticMeshComponent.cpp`;
    gjør den ikke det, flytt instansen 100 km ned i tillegg). Aldri `RemoveInstance` (endrer indeksene).
- **Skademodell (server, `GSBreakable::DamageGlass(Key, Amount, Cause, Dir)`):**
  - Herdet (`Wall`, `DoorPane`): ethvert kuletreff, slag eller trykkbølge → `State 2` (herdet glass smuldrer helt når
    overflaten brytes – ekte oppførsel).
  - Laminert (`Railing`): kule `Health += Damage × 0,6`; første treff → `State 1`; `Health ≥ 100` → `State 2`. Slag +35.
    Trykkbølge: `Health += Strength × (1 − d/R2)`.
  - `State 2` på `Wall`/`Railing` → `OnWalkableChanged` med feltets boks + 1 m; `OnNoise` 40 m.
- **Kuler og glass:** `BulletSegment` gjør `LineTraceMultiByObjectType(From, To, FCollisionObjectQueryParams(ECC_Destructible))`
  (ett billig spor per kulesteg; bare glass har den typen) og håndterer hvert treff i rekkefølge: server + `bReal` →
  `DamageGlass`; alle maskiner, `!bReal` og ruta er laminert og fortsatt hel/sprukket → lokalt sprekkemerke (steg 5) +
  et lite hvitt glimt (`AGSShortFX::SpawnSprite`, 6 cm, 0,1 s). Returnerer `0,92^antall` ruter. Herdet glass gir ikke
  sprekkemerke (det forsvinner jo).
- **Eksplosjon:** `ApplyBlast` knuser glass innen `R2 = max(2 × R, 600 cm)` (trykkbølgen knuser vinduer langt utenfor den
  dødelige radien); glass mer enn 3,5 m over/under (annen etasje) bare innen `R`. Ingen siktsjekk for glass.
- **Effekter når et felt går i stykker (`bFresh`, alle maskiner):**
  1. Felt skjules; sprekkemerker med feltets nøkkel fjernes.
  2. Lyd: herdet `Glass_Shatter` (vol 1,0, 45 m), laminert `Glass_Collapse` (0,9, 40 m), 0,4 s senere `Glass_Tinkle`
     (0,5, 20 m) – med reserve (steg 8).
  3. Lokale fysikkbiter: herdet 10–14 «klumper» (`SM_Int_GlassClump`), laminert 4–6 store skår (`SM_Int_Shard_A…D`),
     som `UStaticMeshComponent` på én `AGSDebris`-aktør (i `GSBreakable`), `SimulatePhysics`, kollisjonsprofil:
     objekttype `PhysicsBody`, `Ignore` Pawn/Visibility/Camera, `Block` WorldStatic/WorldDynamic. Startfart langs
     `HitDir` (2–5 m/s) + litt tilfeldig. Etter 3,5 s: hver bit blir en instans i gulvskår-ISM der den ligger, og
     komponenten slettes. **Tak:** 150 simulerte biter per maskin (de eldste fryses først).
  4. Gulvskår: 30–50 instanser `SM_Int_GlassGranules` (flate små klumper, uten kollisjon og skygge, `CullDistance` 2500)
     spredt 0,2–1,6 m ut fra feltets fot, mest i `HitDir`; gulvet finnes med ett spor ned per gruppe (5 grupper).
     **Tak:** 3000 per maskin (ringbuffer: den eldste instansen flyttes). Tilfeldighet seedes med nøkkelen, så alle
     ser omtrent likt.
  5. Sen klient (`!bFresh`): bare 1 og 4.
- **Sprukket laminert (`State 1`) for sen klient:** 3 sprekkestjerner på seedede steder i ruta.

### 4. `AGSDoor` – `GSDoor.h/.cpp`
- `UCLASS AGSDoor : AActor, IGSInteract` – **lokal** (`bReplicates = false`), lagd på hver maskin av `AGSMapState` fra
  dørdataene (steg 6), samme nøkkel overalt. `Tick` bare mens den beveger seg eller har en tidtaker.
- Komponenter: `Root` i hengselen (gulvnivå), `Leaf` (mesh, ingen kollisjon), `LeafBox` (`UBoxComponent`, objekttype
  `WorldDynamic`, `BlockAll`, `SetCanEverAffectNavigation(false)` – Bots' bestilling G2), `Handle`, og for `GlassCode`:
  `Pane` (eget lite `UGSGlassSet`-felt med typen `DoorPane` og nøkkelen `Door`-id, så kula knuser det via samme spor –
  `LeafBox` er da bare rammen: fire tynne bokser, og `Pane` stenger åpningen), og `Keypad` (på veggen ved håndtaket,
  1,1 m over gulvet, 8 × 14 × 3 cm, `UBoxComponent` som blokkerer `Visibility` så kuler treffer den, tag
  `GS_Int_Keypad`, liten grønn/rød lysdiode som `UPointLightComponent` 20 cm, styrke lav, ingen skygge). `Double`: to blad
  à halv bredde, hengsler i hver side, går samme vei. `MarksISM` (egen liten ISM på bladet for kulehull, tak 12).
- Modeller (steg 7): `SM_Int_Door_Wood`, `SM_Int_Door_Fire`, `SM_Int_Door_GlassFrame`, `SM_Int_Handle`,
  `SM_Int_Keypad`; mangler de: kuber skalert med `MI_GS_WoodLight`, `MI_GS_PaintWhite`/`MI_GS_Steel`, `MI_GS_MetalDark`.
  Blad 4 cm (tre), 6 cm (brann), ramme 5 cm (glass); høyde fra dataene (2,1 m standard), 1 cm klaring over gulvet.
- **Bevegelse:** `TargetYaw = Open/100 × 95°`. Fart: vanlig 170°/s, spark/trykkbølge 650°/s, branndør som lukker seg
  60°/s med smell til slutt. Mens bladet går: `LeafBox` ignorerer `Pawn` (ingen dytting av spillere i nettet); ved stopp
  `Block` igjen (figurens egen avstøting tar den som står i veien). Lyd når den starter å åpnes (`Door_Open`), når den
  når 0 (`Door_Close`), og ved spark (`Door_Slam`).
- **Forhåndsvisning:** `PredictUse` på eierens maskin setter et lokalt mål med en gang (1 s levetid); kommer ikke
  serverens tilstand innen da, går døra tilbake.
- **Bruk (`ServerUse`, server):** side = tegnet til `(Bruker − Root) · Normal`; døra åpnes **vekk** fra brukeren.
  | Tilstand | `Tap` | `HoldStart` (0,3 s hold) |
  |---|---|---|
  | lukket, ulåst | åpne (Open ±100) | spark opp: Open ±100 raskt, `OnNoise` 35 m, `Health+1` |
  | åpen | lukk (Open 0) | – |
  | låst `GlassCode`, har `key` (adgangskort) | `Keypad_Beep`, lås 0 (låst opp), døra åpnes | – |
  | låst `GlassCode`, uten kort | «Låst. Kodelås.» + pip-feil | med `lockpick`: jobb 6 s (som kassene) → lås 0; uten: «Bryt opp kodelåsen» – jobb 4 s, høylytt (`Keypad_Break` hvert 1 s, `OnNoise` 30 m) → lås 2 |
  | låst `Wood` | «Låst.» | spark: én jobb à 1,0 s per spark; 3. spark → lås 2 og døra slår opp; `Door_Kick` per spark, `OnNoise` 40 m |
  | låst `Fire` (alarmdør) | «Alarmdør. Kan ikke åpnes herfra.» | – (bare sprengning, runde 2) |
  Kortet og dirkesettet brukes ikke opp (et dirkesett er et verktøy; Spillbarhet kan endre det, se bestilling).
  Gjenstandene finnes ved `GSItems::Find(S.Id)->Kind == "key"/"lockpick"` i `User->Backpack` (samme som kassene).
- **Skudd og slag:** `ApplyBulletHit` med `Hit.Component` = `Keypad` → lås 2 (gnister: `SpawnSprite` oransje, `Keypad_Break`);
  treff på `LeafBox` innen 12 cm fra håndtaket på en låst `Wood`-dør → lås 2 («skyte opp låsen»); andre treff i bladet:
  ingen tilstand (hullet kommer fra Kamps viste kule, steg 5). `ApplyMelee` (kule-sweep 22 cm mot `LeafBox`/`Keypad`/glass):
  på blad = ett spark; på kodelås = 2 slag → lås 2; på glass = `DamageGlass`.
- **Glassdør med knust glass** (`State` bit 2): `Pane` skjult, man går gjennom åpningen i rammen (`LeafBox`-rammene står),
  låsen spiller ingen rolle lenger → `OnWalkableChanged`.
- **Trykkbølge** innen `R`: Open = 100 vekk fra smellet, lås 2, glasset knust; fart 650°/s.
- **Branndører** (`Fire`) lukker seg selv 6 s etter at de ble åpnet (server setter Open 0), med smell.
- **Hint (`GetUsePrompt`)**, eksempler: «[E]  Åpne døra» / «Hold [E]: spark opp»; «[E]  Lukk døra»;
  «Grupperom – låst med kodelås» / «[E] adgangskort · Hold [E]: dirk opp (dirkesett)» eller «Hold [E]: bryt opp
  kodelåsen (høylytt)» + «Eller skyt låsen eller glasset»; «Låst dør» / «Hold [E]: spark inn»; «Alarmdør – låst».
  Farge: hvit (0,92); låst: gul (1, 0,85, 0,4) som Kamps «Låst kasse».
- **Lyd mens noen dirker:** `Flags` bit 0 → hos alle innen 12 m: `Lock_Pick` hvert 0,8–1,4 s.
- **Standard:** alle dører lukket; `GlassCode` låst; `Fire`/andre med `laast` i dataene låst; dører med `aapen` i dataene
  står åpne (Open +100).

### 5. Varige kulehull – `GSBreakable.h/.cpp`
- `UCLASS UGSMarks : USceneComponent` på `AGSMapState` (lokal, alle maskiner; lager sine ISM-er som barn): én ISM per merkeart, `NoCollision`, ingen skygge,
  `CullDistance` 3000, mesh `/Engine/BasicShapes/Cylinder` (flat skive) og `/Engine/BasicShapes/Cube` (sprekkestreker).
  - Gips/betong/stein: mørk kjerne Ø 0,9 cm (`MI_GS_Gunmetal`) + lysere avskallet krater Ø 2,5–4 cm (`MI_GS_PlasterWhite`,
    0,1 cm ut fra flaten, litt ujevnt skalert).
  - Tre: mørk kjerne Ø 0,8 + flisring i `MI_GS_WoodLight` (oval, 3 × 1,5 cm, tilfeldig vridd).
  - Metall: blank ring (`MI_GS_Steel`) Ø 1,4 + mørk kjerne.
  - Laminert glass: sprekkestjerne – 5–7 tynne streker (0,12 cm brede, 3–10 cm lange, `MI_GS_PlasticWhite`) ut fra punktet.
- Underlag velges av `Hit.Component->ComponentTags` (`Mat_Glass`, `Mat_Wood`, `Mat_Plaster`, `Mat_Metal`,
  `Mat_Concrete`), så `Hit.PhysMaterial`-navnet (`PM_GS_Tre` … når Grafikk har dem), ellers gips. Ingen merker på
  figurer, `AGSPickup`, eller noe som simulerer fysikk.
- **Merker på dører** går i dørens egen `MarksISM` (følger bladet når det svinger). **Merker på glass** lagres med
  feltets nøkkel og fjernes når feltet forsvinner.
- **Tak:** 400 merker per maskin (ringbuffer per ISM – den eldste instansen flyttes til det nye stedet; aldri
  `RemoveInstance`), og maks 30 nye merker per sekund (hagl). Alle fjernes ved tilbakestilling.
- Lokalt og kosmetisk: hver maskin setter hull der dens viste kuler treffer (det samme stedet for alle innen spredningen
  som alt sendes). Ingen replikering, ingen sen-klient-gjenoppretting (det er greit: hull er ikke spilltilstand).

### 6. Data: Helgasetr (Grafikks 4a) og testrommet – i `GSMapState.cpp`
- **Ett tilpasningssted:** `static void CollectMapDefs(TArray<FGSGlassDef>&, TArray<FGSDoorDef>&)`:
  `FGSGlassDef { int32 Id; FVector CenterCm; float LengthCm, HeightCm, YawDeg; EGSGlass Type; }`,
  `FGSDoorDef { int32 Id; FVector BaseCm /*gulv, midt i åpningen*/; float WidthCm, HeightCm, YawDeg /*normal ut*/;
  EGSDoorArt Art; bool bHingeLeft, bLocked, bOpen; }`.
  Inne i en `#if __has_include("GSHelgasetr.h")`-blokk (og `if constexpr (GSCampus::bHelgasetr)` hvis bryteren finnes):
  les `GSHelgasetr::GlassPanels()` og `GSHelgasetr::DoorSlots()` (meter → cm; typene `rekkverk`/`vegg`/`dor` og artene
  `tre`/`glass_kode`/`dobbel`/`brann`). **Les `GSHelgasetr.h` først** og bruk de feltnavnene som faktisk finnes; mangler
  yaw/hengsel/låst (bestilt), regn yaw ut fra hvilken vegg døra står i (nærmeste rom-kant i `FloorRects` er ikke verdt det:
  bruk `Yaw 0` hvis bredden ligger langs Y, ellers 90, og skriv i loggen at yaw er gjettet), hengsel venstre, ulåst
  (unntatt `glass_kode`). Ingen blokk = ingen kartdeler (bare testrommet). Blir det byggefeil i denne blokken fordi
  Grafikks navn er andre: rett blokken her (vår fil), aldri `GSHelgasetr.h`.
- **Grafikks egne glassinstanser:** finnes det komponenter med tag `GS_Int_Glass` (bestilt) eller ISM-er med mesh
  `SM_Mod_Helg_GlassPanel`/`GlassDoor` på byggeren (`TActorIterator<AGSCampusBuilder>`), skjul dem
  (`SetVisibility(false)`, `SetCollisionEnabled(NoCollision)`) når vi har lagt ut våre egne fra samme data – så det ikke
  står to lag glass. Logg «Interaktiv: N glassfelt, M dører (K glassinstanser fra byggeren skjult)».
- `BuildRegistry()`: lag `UGSGlassSet` per type og `AGSDoor` per dør (`SpawnActor` lokalt, `bNetLoadOnClient=false`,
  ikke replikert), registrer nøklene. Kjøres når `CollectMapDefs` gir data og byggeren har bygd (finnes
  `AGSCampusBuilder` i verden), ellers prøv igjen (steg 1).
- **Testrommet** (`bTestRoom`, replikert, så alle maskiner bygger det likt): origo `O = (0, 0, 6000)` cm (over taket på
  begge kartene, innenfor `PlayArea`, så kulegrensen i `GSBallistics` ikke tar kulene). Bygget av kuber (ISM, `BlockAll`,
  materialer `MI_GS_Terrazzo` gulv, `MI_GS_PlasterWhite_In` vegger/tak – byggeren sjekker navnene), 2 punktlys uten skygge:
  - Rom 16 × 12 × 3,4 m, gulv 0,3 m tykt, tak (så regn ikke kommer inn). Innvendig vegg på tvers med 4 dører:
    `Wood` (ulåst), `Wood` (låst), `GlassCode` (låst), `Double` (ulåst). På endeveggen en `Fire`-dør (ulåst, lukker seg
    selv) inn til et lite trapperom 3 × 3 m, og en `Fire`-dør (låst = alarmdør).
  - En glassvegg (4 × `Wall`, à 1,25 × 2,6 m) som deler den ene halvdelen – «grupperom».
  - Et galleri 1,4 m høyt (6 × 3 m, trapp/rampe opp) med rekkverk av 5 × `Railing` (1,2 × 1,1 m) – knus og hopp ned.
  - En gipsvegg og en treplate (`Mat_Plaster`, `Mat_Wood`) og en stålplate (`Mat_Metal`) til kulehull.
  - Et bord med ett NTNU-adgangskort (`Keycard`) og ett dirkesett (`Lockpick`) – `AGSPickup::SpawnDropped` (server; de
    ryddes ved kampstart som andre slupne ting, og legges ut på nytt hver gang kommandoen kjøres).
  - Id-er: `TestIdBase + 0…` per kategori. Teleport: vert/alene setter pawnen til `O + (−6, 0, 0,2) m` (`TeleportTo`).
- Testrommet virker også på det gamle campuskartet og i «Test kartet» (Explore).

### 7. Modeller – `Tools/Blender/interactive_doors.py`
Bruk `MeshKit`/`gs_common.py` som Grafikks skript (se toppen av `Tools/Blender/gen_models.py` for hvordan Blender kjøres
i bakgrunnen). Origo på gulvet midt under bladet, front +X, meter. Eksporter FBX + .blend til `SourceArt/Models/`
(`init_unreal.py` importerer dem ved neste editorstart; `SM_Int_` blir ikke Nanite og får enkel kollisjon – vi bruker
uansett egne bokser):
- `SM_Int_Door_Wood` 1,0 × 2,09 × 0,04 m, lys eik-finér med smal kantlist, materialspor `WoodLight_In`.
- `SM_Int_Door_Fire` stål, hvit lakk, panikkstang, lite trådglassvindu (glass som eget spor `Glass`).
- `SM_Int_Door_GlassFrame` svart/grønn aluminiumsramme 5 cm, uten glass (glasset er vår egen rute), prikkebånd som eget
  tynt spor.
- `SM_Int_Handle` (rustfritt U-håndtak), `SM_Int_Keypad` (svart boks 8 × 14 × 3 cm med 12 taster og lysdiode-spor).
- `SM_Int_GlassClump` (3 varianter samlet i én modell er nok: klump av 6–10 små terninger, 2–4 cm), `SM_Int_GlassGranules`
  (flat klynge 15 × 15 cm av 30 småbiter), `SM_Int_Shard_A`…`D` (flate, takkete skår 8–30 cm).
- `--render`: ett kontaktark (`contact_sheet.py`) til `.claude/lag/interaktiv/kontroll/r01-modeller.png`.
C++ faller tilbake til kuber for alt som mangler, så runden virker selv om Blender-steget feiler.

### 8. Lyd (reserve til Kamps lyder finnes)
`GSBreakable::PlayPartSound(Ctx, Name, Loc, Vol, RangeM, Pitch)`: sjekk én gang med
`LoadObject<USoundBase>(nullptr, "/Game/Sounds/S_GS_<Name>.S_GS_<Name>", …, LOAD_NoWarn | LOAD_Quiet)` (husk svaret i en
`TMap`); finnes den: `GSAudio::PlayAt`. Ellers reserve: `Glass_*` → `Impact_1…3` med pitch 1,7–2,0; `Door_Kick`/`Door_Slam` →
`Land` pitch 0,7; `Door_Open`/`Door_Close` → `Reload_In`/`Reload_Out` pitch 0,6, vol 0,5; `Lock_Pick` → `Reload_In` pitch 2,2
vol 0,3; `Keypad_Beep` → `Pickup` pitch 1,6; `Keypad_Error` → `Dry_Fire` pitch 1,3; `Keypad_Break` → `Impact_2` pitch 1,3.
Lydnavnene er bestilt hos Kamp.

### 9. Bygg og prøv
1. `Build.bat` med `-WaitMutex` (byggeren; editoren lukket). Rett feil bare i egne filer.
2. PIE, «Spill online»/vert alene, konsoll (§): `Interaktiv.Testrom`. Prøv hver dør (trykk og hold E), kort og dirkesett
   fra bordet, skyt kodelåsen, skyt glassveggen (herdet – smuldrer i ett treff), skyt rekkverket (sprekker, gir etter
   etter flere treff, hopp ned), slå med kolben, kast en granat inn i rommet (glass ryker, dører slår opp – **krever K2**;
   uten den: ingenting skjer med glass ved granat, noter det i rapporten).
3. PIE med **2 spillere** (vert + klient): klienten åpner en dør → verten ser den gå; verten knuser glass → klienten ser
   det. Så **sen klient:** start PIE med 1, knus 3 felt og bryt opp en dør, koble til spiller 2 (`open 127.0.0.1` i en
   ny PIE-klient eller «Bli med») → han ser de knuste feltene (uten biter i lufta) og den åpne døra.
4. `Interaktiv.Tilbakestill` → alt helt igjen, skår og hull borte. Start en CTF-runde (Lobby → Playing) → det samme.
5. Bruk-tasten: stå ved en pickup (fra bordet) med en dør bak – E skal plukke opp, ikke åpne døra (før K4).
   Med menyen/inventaret åpent skal E ikke åpne dører. Esc/F10 lukker fortsatt menyen (lærdom: input testes i PIE).
6. `stat unit` i testrommet før og etter at alt glass er knust (forventet < 0,5 ms forskjell når bitene har lagt seg).

## Ferdig når
- Bygger uten feil og uten nye advarsler i egne filer; ingen andre lags filer er endret.
- `Interaktiv.Testrom` virker; alle seks dørene og alle tre glasstypene oppfører seg som i tabellene over, med lyd
  (reservelyd er godt nok) og skår.
- Online med to spillere: dørbevegelse, låser og knust glass er like hos begge; sen klient får riktig tilstand; ingen
  biter eller lyd spilles av for gamle hendelser hos sen klient.
- Kampstart (fasen går til Countdown/Playing) setter alt tilbake – også uten Spillbarhets krok.
- E gjør aldri to ting på én gang; hint og framdrift vises midt på uten å overlappe Kamps hint.
- Kulehull: `Interaktiv.Merke` legger varige hull (rett underlag i testrommet); taket på 400 holder (skyt 600 ganger med
  K1c levert, eller kjør `Interaktiv.Merke` i en løkke).
- Med Helgasetr (4a) bygget: loggen sier hvor mange glassfelt og dører som ble lagt ut; uten: «ingen kartdata – bare
  testrommet», ingen feil.
- `retning.md`, `logg.md` og `IMPROVEMENTS.md` (646–661) er oppdatert.

## Risiko
- **Skala 0 på ISM-instans** fjerner kanskje ikke fysikklegemet i UE 5.8 → spillere kolliderer med usynlig glass.
  Sjekk motorkoden; ellers flytt instansen langt ned i tillegg.
- **Rekkefølge online:** registeret er ikke bygd når lista kommer (sen klient, eller kartet bygges etter oss) → løst med
  `PendingKeys` og `ApplyAllLocal` etter bygging. `OnRep` før `BeginPlay` (lærdom): `BeginPlay` må ikke nullstille `Parts`.
- **Glass med `ECC_Destructible`:** noe som sporer på objekttyper (`ObjectTypeQuery`) eller kanal `Destructible` kan bli
  overrasket – søk i `Source/` etter `ECC_Destructible`/`ObjectTypeQuery` før bygging (i dag: ingen treff).
- **Fysikkdører og bevegelse:** spillere som står i svingen havner inni bladet et øyeblikk; figurens avstøting tar det.
  Ser det stygt ut i PIE: stopp døra (behold `Open` på serveren, men vis den 10° før) – logg som funn.
- **Inndata:** vår `UEnhancedInputComponent` må ikke stjele E fra Kamps `Interact` – vi binder uten `bBlockInput` og
  uten å konsumere. Hold-målingen bruker tastestatus, ikke triggere. Prøv i PIE (Selected Viewport), ikke bare kompiler.
- **Ytelse:** 1 200 glassinstanser i Helgasetr i 3 ISM-er (få tegnekall), dører ca. 120 lokale aktører uten `Tick` når de
  står stille. Fysikkbiter og skår har tak. Bots-sjekken ved dører er 5 Hz og bare for AI.
- **Juks:** klienten sender bare nøkkel + verb; serveren sjekker avstand, tid og hva han har i sekken. Dirkejobben
  fullføres av serverens klokke.
- **Kroker ikke levert:** uten K1 går kulene gjennom glasset uten å knuse
  det (som i dag), uten K2 gjør granater ingenting med kartet, uten K3 knuser ikke kolben glass. Dørene, låsene, testrommet,
  tilbakestillingen og hint virker uansett. Rapporten sier tydelig hva som venter på Kamp.

## IMPROVEMENTS-linjer (646–661, egen seksjon øverst: «Runde 1 (Interaktiv verden): dører, knusbart glass og kulehull (27. nov.)»)
646. Ny felles tilstand for alt som kan åpnes og knuses i kartet, lik for alle spillere online
647. Den som kommer inn midt i kampen ser knust glass, åpne dører og brutte låser slik de er
648. Alt glass, alle dører og alle låser settes tilbake når en ny kamp starter
649. Bruk-tasten (E) virker på dører: trykk for å åpne eller lukke, hold for å sparke
650. Dører går opp vekk fra deg og kan sparkes opp med et høyt smell
651. Låste tredører kan sparkes inn (tre spark) eller skytes opp ved låsen
652. Glassdørene til grupperommene har kodelås: NTNU-adgangskort åpner med et pip
653. Kodelåsen kan dirkes opp med dirkesett (6 s) eller brytes opp med makt (4 s, høylytt), eller skytes i stykker
654. Branndører i trapperommene lukker seg selv etter noen sekunder
655. Herdet glass i glassbokser, glassvegger og glassdører smuldrer helt i ett treff, med lyd og glassbiter på gulvet
656. Laminert glass i rekkverkene sprekker der kula treffer og gir etter til slutt – da kan du hoppe ned en etasje
657. Granater knuser glass i en vid radius og blåser opp dører i nærheten
658. Kulehull blir sittende i vegger, dører, metall og glass (eget utseende per underlag) til neste kamp
659. Kort hint midt på skjermen ved dører (hva E gjør, hva hold E gjør, hva du trenger) med framdriftslinje ved dirking
660. Botene åpner ulåste dører de går mot
661. Testrom for alt det interaktive (konsoll: Interaktiv.Testrom), så alt kan prøves uten å lete i bygget
