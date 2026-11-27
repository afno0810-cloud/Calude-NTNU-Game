# Plan 01b (Interaktiv verden): glass som knuses, biter og skår, varige kulehull, modeller

Bygger på 1a (`plan-01a.md`: `AGSMapState`, register, ferskhetsregelen, `AGSDoor`, testrommet, konsollen). Forhåndssjekk:
`kontroll/forhand-plan-01.md`. Les også `.claude/lag/grafikk/helgasetr.md` kap. 4, 6, 9 og `grafikk/ko/plan-04.md` steg 1g/7.
Glass kan prøves fullt med `Interaktiv.Knus`/`Interaktiv.Smell` uten Kamp; med Kamps K1–K3 knuser kuler, granater og slag det.

## Mål
Glass i rekkverk, glassbokser, glassvegger og glassdører knuses som ekte glass, med lyd, biter i lufta og skår på gulvet, og
kulehull blir sittende i veggene til neste kamp – likt for alle online.

## Forbedrer (gammelt)
- **Kulenedslaget** (`AGSShortFX::SpawnImpact`, Kamp): den mørke flisa forsvinner etter 40 s. Nå blir hullet sittende,
  med utseende etter underlag (via K1c).
- **Glassfeltene i Helgasetr** (4a: hele klosser som kulene går gjennom uten spor) blir ekte, knusbart glass.
- **Glassdørene fra 1a** kan knuses og gås gjennom; **`Interaktiv.Knus`/`.Smell`** virker nå også på glass.
- **Granater** knuser glass i vid radius (via K2).

## Nytt
- `UGSGlassSet` (ISM per glasstype) med herdet (smuldrer helt) og laminert (sprekker, gir etter) glass.
- Lokale fysikkbiter (`AGSDebris`) og skår på gulvet, med tak.
- Varige kulehull (`UGSMarks`): 400 i alt per maskin.
- `BulletSegment` (kuler finner glass), `ApplyMelee` som stopper ved glass, `AddBulletMark`.
- Blender-modeller: `Tools/Blender/interactive_doors.py` (dørblad, håndtak, kodelås, skår, granulat).
- Glass i testrommet: glassvegg, galleri med rekkverk, flater til kulehull.

## Hvorfor nå
Andreas: «vinduer kan knuse», «man kan skyte hull i vegger»; han likte glass som knuser. Kroken K1 er bedt tatt tidlig av
Kamp, så 1b kommer når den er klar.

## Filer
Nye: `Source/Gloshaugen/Public/GSBreakable.h`, `Private/GSBreakable.cpp`, `Tools/Blender/interactive_doors.py`,
`SourceArt/Models/SM_Int_*.fbx` (+ .blend). Endres (våre fra 1a): `GSMapState.h/.cpp` (glass i register/data/API, fyll
`BulletSegment`/`AddBulletMark`, `ApplyMelee` returnerer avstand), `GSDoor.cpp` (knusbar rute, modeller, `MarksISM`).
Felles: `IMPROVEMENTS.md` (718–733). Ingen andre lags filer.

## Steg

### 1. `UGSGlassSet` – `GSBreakable.h/.cpp`
- `UInstancedStaticMeshComponent`, én per glasstype per kilde (kart, testrom), lokal på `AGSMapState`. Mesh
  `SM_Mod_Helg_GlassPanel` hvis den finnes, ellers `/Engine/BasicShapes/Cube`; skalert lengde × høyde × tykkelse (rekkverk
  12 mm, vegg 10 mm). Materiale `MI_GS_Glass` (eller det gjennomsiktige MI-et i `gs_materials.py`).
- **Kollisjon:** objekttype `ECC_Destructible`; `Block` `Pawn`, `PhysicsBody`, `WorldDynamic`, `Destructible`; `Ignore`
  `Visibility`, `Camera`, `WorldStatic`-sporet. Kollisjon slått på **før** `RegisterComponent` (så `bPhysicsStateCreated` er
  sann). Ingen skygge. Tags `Mat_Glass`, `GS_Int_Glass`. `SetCanEverAffectNavigation(true)`.
- `KeyOfInstance` / `InstanceOfKey`. **Skjule:** `UpdateInstanceTransform(I, skala 0, bWorldSpace, bMarkRenderStateDirty =
  false, bTeleport = true)` – motoren sletter legemet ved skala 0 og lager det igjen ved skala 1 (sjekket i forhåndssjekken,
  InstancedMeshComponentBodies.cpp l. 152/169). Ingen flytting langt ned, aldri `RemoveInstance`.
- **Alle ISM-oppdateringer i 1b bruker `bMarkRenderStateDirty = false`** (glass, skår, merker) – ellers bygges hele proxyen
  på nytt for hver rute/hvert merke.
- Effekter av samme kategori fra 1a-reglene: ferskt bare ved `0 ≤ dt < 2 s`, ikke ved første replikering, ikke via
  `PendingKeys`/`ApplyAllLocal`.

### 2. Skademodell (server) – `GSBreakable::DamageGlass(Key, Amount, Cause, Dir)`
- Herdet (`Wall`, `DoorPane`): ethvert treff (kule, slag, `Interaktiv.Knus`, trykkbølge) → `State 2`.
- Laminert (`Railing`): kule `Health += Amount × 0,6`, første treff → `State 1`, `Health ≥ 100` → `State 2`; slag +35;
  trykkbølge `Strength × (1 − d/R2)`.
- `State 2` på `Wall`/`Railing` → `OnWalkableChanged` (feltets boks + 1 m) og `OnNoise` 40 m.
- **Dørruta:** `BulletSegment`/`ApplyMelee`/`Knus` som treffer en komponent på en `AGSDoor` (`Hit.GetActor()`) gir skaden
  til døra: `State` bit 2 → `Pane` skjult og uten kollisjon, man går gjennom rammen, låsen spiller ingen rolle →
  `OnWalkableChanged`. Effekter som en herdet rute.
- `ApplyBlast`: glass innen `R2 = max(2R, 600 cm)`; mer enn 3,5 m over/under bare innen `R`. Ingen siktsjekk for glass.
- `Interaktiv.Knus [skade]` treffer nå også glass (objektspor mot `Destructible` først, så `Visibility`).

### 3. Kuler og slag mot glass – `GSMapState.cpp`
- `float BulletSegment(World, From, To, Vel, Damage, bReal)`: `LineTraceMultiByObjectType(From, To, Destructible)`; hvert
  treff i rekkefølge: server + `bReal` → `DamageGlass` (eller dørrute); `!bReal` og ruta er laminert og ikke borte → lokalt
  sprekkemerke (steg 5) + hvitt glimt (`SpawnSprite` 6 cm, 0,1 s). Returnerer `0,92^antall`. **Kamp sender `To = bHit ?
  Hit.ImpactPoint : End`** (forhåndssjekk 6 – ellers telles glass bak en vegg).
- `float ApplyMelee(User, Start, End, Damage)`: sveip (22 cm) som objektspor mot `Destructible` → første hele rute: server
  gir skaden; returnerer avstanden fra `Start` til ruta (`BIG_NUMBER` hvis ingen). Kamp kaster treffet på en figur som står
  lenger unna enn dette (forhåndssjekk 7). Dører/kodelås som i 1a.

### 4. Biter og skår (lokalt, ferskt)
1. Rute skjult; sprekkemerker med rutas nøkkel fjernes.
2. Lyd: herdet `Glass_Shatter` (1,0, 45 m), laminert `Glass_Collapse` (0,9, 40 m), 0,4 s etter `Glass_Tinkle` (0,5, 20 m);
   laminert treff uten å gi etter: `Glass_Crack` (0,6, 25 m). Reserve: `Impact_1…3` pitch 1,7–2,0.
3. `AGSDebris` (én aktør per knusing): herdet 10–14 `SM_Int_GlassClump`, laminert 4–6 `SM_Int_Shard_A…D`, som
   `UStaticMeshComponent` med `SimulatePhysics`, objekttype `PhysicsBody`, `Ignore` Pawn/Visibility/Camera, `Block`
   WorldStatic/WorldDynamic. Fart langs `HitDir` 2–5 m/s + litt tilfeldig. Etter 3,5 s → instans i gulvskår-ISM, komponenten
   slettes. **Tak:** 150 simulerte biter per maskin (de eldste fryses først).
4. Gulvskår: 30–50 instanser `SM_Int_GlassGranules` (ingen kollisjon/skygge, `CullDistance` 2500) 0,2–1,6 m fra rutas fot,
   mest i `HitDir`, ett spor ned per gruppe (5 grupper). **Tak 3000** (ringbuffer: eldste instans flyttes). Seedet med nøkkelen.
5. Gammel hendelse/sen klient: bare 1 og 4. Sprukket laminert for sen klient: 3 stjerner på seedede steder.

### 5. Varige kulehull – `UGSMarks : USceneComponent` på `AGSMapState`
- ISM-er per merkeart (barn), `NoCollision`, ingen skygge, `CullDistance` 3000; `/Engine/BasicShapes/Cylinder` (skive) og
  `/Engine/BasicShapes/Cube` (streker). Gips/betong: kjerne Ø 0,9 cm `MI_GS_Gunmetal` + krater Ø 2,5–4 cm
  `MI_GS_PlasterWhite`; tre: kjerne + flisring `MI_GS_WoodLight`; metall: blank ring `MI_GS_Steel` + kjerne; laminert glass:
  5–7 streker 0,12 × 3–10 cm `MI_GS_PlasticWhite`.
- Underlag: `Hit.Component`-tags (`Mat_Glass`, `Mat_Wood`, `Mat_Plaster`, `Mat_Metal`, `Mat_Concrete`), så
  `Hit.PhysMaterial`-navn (`PM_GS_*`), ellers gips. Aldri på figurer, `AGSPickup` eller ting som simulerer fysikk.
- **Taket er 400 merker i alt per maskin**, med én felles ringbuffer av merke-id-er; hvert merke eier 2–8 instanser (i
  ett eller flere ISM-er). Når bufferet er fullt, flyttes det eldste merkets instanser til det nye stedet (skala 0 på det
  som ikke trengs). Maks 30 nye merker per sekund.
- Merker på dører: i dørens `MarksISM`, **laget først ved første merke** (ikke 120 tomme ISM-er), tak 12 per dør, teller med
  i de 400. Merker på glass lagres med rutas nøkkel og fjernes når ruta forsvinner. Alt fjernes ved tilbakestilling.
- Lokalt og kosmetisk, ingen replikering. `Interaktiv.Merke` legger et merke der du ser (prøve uten K1c).

### 6. Data og 4a-glass – `GSMapState.cpp`
- `CollectMapDefs` får glass: `FGSGlassDef { Id; CenterCm; LengthCm; HeightCm; YawDeg; Type; }` fra
  `GSHelgasetr::GlassPanels()` (`FPanelM` i plan-04; les headeren), samme `__has_include`-blokk. `dor`-felt som står i en
  `DoorSlots`-åpning hoppes over (døra har egen rute).
- **Skjule 4a-glasset (forhåndssjekk):** 4a lager rutene med `GSUtils::Cube()` og taggen `Mat_Glass`, og fast glass (tak,
  vinduer) har *også* `Mat_Glass`. Skjul derfor bare komponenter på `AGSCampusBuilder` som har taggen `GS_Int_Glass`
  (bestilt), **eller** `Mat_Glass` **og** `GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore` (bare de knusbare
  rutene ignorerer `Visibility`; fast glass blokkerer). Skjul = `SetVisibility(false)` + `SetCollisionEnabled(NoCollision)`.
  **Aldri bare etter `Mat_Glass` eller mesh-navn.** Logg «Interaktiv: N glassfelt, K glasskomponenter fra byggeren skjult».

### 7. Testrommet får glass
Glassvegg 4 × `Wall` (1,25 × 2,6 m) som lager et «grupperom»; galleri 1,4 m høyt (6 × 3 m, rampe) med 5 × `Railing`
(1,2 × 1,1 m) – knus og hopp ned; gipsvegg (`Mat_Plaster`), treplate (`Mat_Wood`), stålplate (`Mat_Metal`) til kulehull.
Glassdøra fra 1a kan nå knuses.

### 8. Modeller – `Tools/Blender/interactive_doors.py`
`MeshKit`/`gs_common.py` som Grafikks skript (se toppen av `gen_models.py` for hvordan Blender kjøres). Origo på gulvet midt
under bladet, front +X, meter. Til `SourceArt/Models/` (FBX + .blend; `SM_Int_` blir ikke Nanite):
`SM_Int_Door_Wood` (1,0 × 2,09 × 0,04, lys eik, spor `WoodLight_In`), `SM_Int_Door_Fire` (hvit stål, panikkstang, lite
trådglassvindu), `SM_Int_Door_GlassFrame` (svart/grønn alu 5 cm, uten glass, prikkebånd-spor), `SM_Int_Handle`,
`SM_Int_Keypad` (12 taster, spor for lysdioden), `SM_Int_GlassClump`, `SM_Int_GlassGranules`, `SM_Int_Shard_A…D`.
`--render`: kontaktark til `.claude/lag/interaktiv/kontroll/r01b-modeller.png`. `AGSDoor` bytter kubene mot modellene når
de finnes; kuber er reserven.

### 9. Bygg og prøv
1. `Build.bat … -WaitMutex`. Rett bare egne filer.
2. PIE: `Interaktiv.Testrom`. `Interaktiv.Knus` på glassveggen (smuldrer i ett), på rekkverket (sprekker; flere ganger →
   gir etter; hopp ned), på glassdøra (gå gjennom rammen). `Interaktiv.Smell` midt i rommet: alt glass ryker, dørene slår opp.
3. Med K1–K3 levert: skyt, slå med kolben (spilleren bak en hel rute skal **ikke** treffes), kast en granat. Uten: noter i
   rapporten hva som venter på Kamp.
4. To spillere + sen klient: knust glass likt; nykommeren ser knuste ruter og skår, men ingen biter i lufta og ingen lyd.
5. Kulehull: `Interaktiv.Merke` i en løkke (600 ganger) → aldri mer enn 400; riktig utseende på gips/tre/stål/glass.
6. Tilbakestilling: alt glass helt, skår og hull borte.
7. Glasstaket og vinduene i Helgasetr (hvis 4a finnes) står fortsatt etter at rutene våre er lagt ut.
8. `stat unit` i testrommet før og etter at alt glass er knust (< 0,5 ms forskjell når bitene har lagt seg).

## Ferdig når
- Bygger uten feil eller nye advarsler i egne filer; ingen andre lags filer endret.
- Alle tre glasstypene oppfører seg som over med `Knus`/`Smell` (og med kuler/granater/slag når K1–K3 finnes), med lyd og skår.
- Online likt, sen klient uten gamle effekter, tilbakestilling fjerner alt.
- Kulehull: 400 i alt, rett underlag, ingen hakking ved hagl (30/s).
- Fast glass i Helgasetr er urørt; loggen viser antall ruter og skjulte 4a-komponenter.
- `logg.md`, `retning.md`, `IMPROVEMENTS.md` (718–733) oppdatert.

## Risiko
- `bMarkRenderStateDirty = true` noe sted gir hakk ved granater – søk gjennom egne filer før levering.
- Glass som objekttype `Destructible`: sjekket spor for spor i forhåndssjekken. Små kjente funn som godtas: gripekroken
  (`WorldStatic`-spor) fester seg bak en hel rute; granater spretter av hele ruter; `DamageAround` skader gjennom en hel rute
  (men K2 knuser den først). Klatring ser ikke glassrekkverk, men kapselprøven stopper.
- 150 fysikkbiter som egne komponenter: opptil 150 tegnekall i 3,5 s – akseptabelt med taket.
- Uten K1 knuser ikke kuler glass; uten K2 ikke granater; uten K3 slår kolben gjennom glass som i dag.

## IMPROVEMENTS-linjer (718–733, egen seksjon øverst: «Runde 1b (Interaktiv verden): glass som knuses og varige kulehull (27. nov.)»)
718. Glass i rekkverk, glassbokser, glassvegger og glassdører kan knuses
719. Herdet glass smuldrer helt i ett treff, slik ekte herdet glass gjør
720. Laminert glass i rekkverkene sprekker der det treffes og gir etter til slutt – da kan du hoppe ned en etasje
721. Knust glass gir lyd: knus, sprekk og skår som faller
722. Glassbiter flyr i retningen treffet kom fra og legger seg på gulvet
723. Glasskår blir liggende på gulvet til neste kamp
724. Glassdører kan knuses, og da kan du gå gjennom rammen uten å bry deg om låsen
725. Granater knuser glass i en vid radius, også utenfor den dødelige radien
726. Kuler knuser glasset de går gjennom og bremses litt av hver rute
727. Slag med kolbe eller kniv knuser glass, men treffer ikke noen som står bak en hel rute
728. Kulehull blir sittende i vegger, dører, metall og glass til neste kamp
729. Kulehull ser forskjellige ut i gips, tre, metall og glass
730. Tak på kulehull, biter og skår, så bildefrekvensen holder også etter lange kamper
731. Den som kommer inn sent ser knust glass og skår, men ingen gamle lyder eller biter i lufta
732. Nye modeller for dører, håndtak, kodelås og glassbiter
733. Testrommet har glassvegg, galleri med glassrekkverk og flater for kulehull (konsoll: Interaktiv.Knus, Interaktiv.Merke)
