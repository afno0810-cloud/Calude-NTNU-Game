# Plan 02 (Interaktiv verden): sprenge, skyte i stykker, bære og knuse – delt i 02a og 02b

Andreas: «så mye som mulig interaktivt», «granater kan få deler av mappet til å sprenge», «skyte hull i vegger»,
«ødelegge ting», «plukke opp ting», «knuse ting». Bygger på 1a/1b (commit 1adda33): `AGSMapState` (én replikert liste),
`IGSInteract`/`AGSInteractor`, `AGSDoor`, `UGSGlassSet`, `UGSInstancePool` (marks/litter), `AGSDebris`, testrommet.
Kamps kroker K1–K3 er koblet på (`GSBallistics.cpp:372`, `GSProjectile.cpp:302`, `GSCharacter.cpp:1175`), så kuler,
granater og slag når oss allerede – alt under virker med ekte våpen fra første bygg.

**02a** = alt som står fast (lettvegger, dørblad, faste møbler): bare den replikerte lista, ingen nye nettaktører.
**02b** = løse ting (`AGSPhysicsProp`): plukke opp, bære, kaste, knuse, brannslukker, barrikader.
02a bygges og kontrolleres før 02b. Begge virker i testrommet uten Grafikks `Props()`.

---

# 02a – vegger, dører og møbler som sprenges og skytes i stykker

## Mål
En granat eller C4 river hull i lettveggene og blåser dører ut av karmen, nok skudd gjør det samme, og bord og skjermer
knuses – så det åpnes nye ruter gjennom Helgasetr midt i kampen, likt for alle 20 spillere.

## Forbedrer (gammelt)
- **Granater mot dører** (1a): i dag flyr de bare opp. Nå blåses bladet ut av karmen innen 0,6 × radius (tre- og
  glassdører) / 0,4 × radius (branndører).
- **Kuler mot tredører** (1a: bare låsen): bladet tar skade og går i stykker etter ca. ett magasin.
- **Lettveggene** (Grafikks 6 `BreachWalls`, i dag død kloss/modell): kan sprenges og skytes i stykker.
- **Kulehull** (1b): hull i en vegg/et dørblad som forsvinner fjernes sammen med det (ikke svevende merker).
- **Dørsving** (funn 1b, dør 23/25/28 går 7–8 cm inn i veggen): hver dør får sin egen største åpning.
- **`Interaktiv.Knus`**: tar `[antall] [skade]`, så man kan skyte en vegg i stykker fra konsollen.

## Nytt
- Lettvegg: hel → hull (1,2 × 2,0 m, gjennomgang) → borte; gipsbiter, støvsky, gipsrester på gulvet.
- Dørblad som rives løs og faller (to halvdeler for tre, helt blad for stål), splinter.
- Faste møbler som knuses: kantinebord, rundbord, grupperomsbord, pult (tre/laminat) og skjermer.
- Nye modeller (`Tools/Blender/interactive_breach.py`).

## Hvorfor nå
Grunnmuren (1a/1b) står, og Kamps kroker er på – granater og kuler når oss allerede. Ødeleggelse av vegger og dører er
det Andreas ba mest om og endrer kampen mest (nye ruter). Alt i 02a ligger i lista som finnes; ingen ny nettkostnad.

## Filer
`GSMapState.h/.cpp`, `GSBreakable.h/.cpp`, `GSDoor.h/.cpp`, `Tools/Blender/interactive_breach.py` (ny), `IMPROVEMENTS.md`.
Leser (endrer ikke): `GSHelgasetr.h` (`BreachWalls()`, `Props()` når den finnes), `AGSHelgasetrBuilder::FindBreachWall`
/`FindProp`, `GSNavSetup::NotifyMapChanged`, `AGSShortFX::SpawnSmokeCloud`, `GSAudio::PlayAt`.

## Steg
1. **Lista:** `FGSPartState` får `UPROPERTY() uint8 Damage = 0;` (skade i enheter à 4 hp, 0–255). Kommentaren oppdateres:
   Breach: `State` 0 hel / 1 hull / 2 borte. Prop (fast): 0 hel / 2 knust. Dør: `Damage` = bladets skade, `Health` = spark
   som før. Nullstilles med resten i `DoResetForMatch`.
2. **Lettvegger – data** (`GSBreakable.cpp`, kodet som glasset i 1b): `FGSBreachDef { Id, CenterCm, SizeCm (lengde langs
   veggen, tykkelse, høyde), YawDeg }` fra `BreachWalls()` (yaw 90 når X-utstrekningen er minst; logg alle 6) + én testvegg
   (240 × 10 × 280 cm, id `TestIdBase+1`) i testrommet. Nøkkel `MakeKey(GSPart::Breach, Id)`. Ved `BuildRegistry`: slå opp
   byggerens instans med `FindBreachWall(Id, Comp, Index)` og lagre den (testveggen: vår egen kube-ISM) + omvendt kart
   (Comp, Index) → nøkkel. Byggerens ISM tegner hel vegg til den går i stykker.
3. **Lettvegger – skade (server):**
   - Kule (`ApplyBulletHit`, komponent med tag `GS_Int_Breach`): `Damage += Damage/4`. ≥ 120 enheter (≈ 480 hp, ca. ett
     AG3-magasin mot samme vegg) → `State 1`.
   - Slag (`ApplyMelee`: sveipen treffer veggen først): +10 enheter.
   - Smell (`ApplyBlast`): d = avstand fra smellet til nærmeste punkt på veggboksen. Fri linje kreves (objektspor
     `WorldStatic`; et treff på selve veggen teller som fritt). d ≤ max(150 cm, 0,5 R) → ett trinn opp (0→1, 1→2);
     `Strength ≥ 250` (C4/rakett – sjekk katalogens tall) og d ≤ 0,5 R → rett til 2. Ellers `Damage += Strength·(1−d/R)/4`.
   - Ved ny tilstand: `Cause`, `HitDir`, `OnWalkableChanged` med veggboksen, `GSNavSetup::NotifyMapChanged(World, Box)`,
     `OnNoise` 40 m.
4. **Lettvegger – lokalt** (`ApplyBreachLocal(Item, bFresh)` fra `ApplyLocal`, alle maskiner, også sen klient):
   - State ≥ 1: byggerens instans skaleres til 0 (`UpdateInstanceTransform`, én gang per brudd, ikke per bilde). Kulehull
     med veggens nøkkel fjernes (`Marks->RemoveKey`) – `AddMark` setter derfor nøkkelen når treffet er en lettvegg (sjekk at
     dørblad gjør det samme).
   - State 1: `SM_Int_Lettvegg_Hull` (synlig, `NoCollision`) skalert til veggens mål + tre usynlige kollisjonsbokser
     (venstre 0,6 m, høyre 0,6 m, overligger 0,8 m) i én egen ISM per kilde med `Mat_Plaster`-tag og samme kollisjon som
     veggen. Mangler modellen: boksene vises med gipsmaterialet.
   - State 2: `SM_Int_Lettvegg_Rest` (sokkel + bøyde stendere, `NoCollision`), ingen bokser.
   - bFresh: 12 gipsbiter (generaliser `AGSDebris::Init` til `InitBits(Key, BoxTransform, SizeCm, Count, Meshes, Mat,
     HitDir, LitterVisual)`; glasset kaller den med dagens verdier, taket 150 gjelder), støvsky `SpawnSmokeCloud(midt, 220,
     (0.82,0.80,0.76), 4 s, 10)`, lyd `Plaster_Break` (reserve `Door_Wood_Break`). Alltid (også sen klient): 6 gipsklumper i
     `Litter` (ny visual `GSLitterVis::Gips`, `SM_Int_Gips_A..C`), én gang per nøkkel (som `LitteredKeys`).
5. **Dørblad som rives løs** (`GSDoor.cpp`; bit `GSLock::LeafGone` finnes):
   - Kule mot bladet (ikke håndtak/kodelås) på `Wood`/`Double`: `Damage += Damage/4`; ≥ 100 → `LeafGone`. Glassdørens ramme
     og `Fire` tar ikke kuleskade på bladet (bare låsen som før). Slag: +6.
   - `ApplyBlast`: dører innen 0,6 R (`Fire`: 0,4 R) med fri linje → `LeafGone` + lås `Broken`; ellers som før
     (`ServerOpenAway`).
   - Lokalt: bladet skjules (synlighet av + `NoCollision`, også ruta og kodelåsen på bladet), kulehull med dørens nøkkel
     fjernes. bFresh: lokal fallende dør (`AGSDebris::InitBits` med `SM_Int_Door_Wood_Broken_A/B`; stål: dørens egen
     bladmodell) langs `HitDir`, 8 splinter, lyd `Door_Wood_Break`. Sen klient: bladet ligger flatt på gulvet 1 m ut langs
     `HitDir` (fast transform, ikke kollisjon for figurer).
   - `GetUsePrompt` → false, `IsDoorPassable` → true, `FGSDoorInfo` får `bool bLeafGone` (og `bOpen = true`). `BotUse`
     hopper over dører uten blad.
6. **Faste møbler** (Helgasetr bare når Grafikks `Props()` finnes; vakt `#if defined(GS_HELGASETR_HAS_PROPS)`):
   - Knusbare typer: `kantinebord`, `rundbord`, `grupperomsbord`, `pult` (60 enheter = 240 hp; smell innen 0,5 R knuser),
     `skjerm` (første kule ≥ 10 skade, eller smell innen R). Andre faste typer tar bare kulehull i denne runden.
   - Omvendt kart (Comp, Index) → nøkkel ved `BuildRegistry` fra `FindProp(Id, ...)` for hver fast, knusbar id. Kuler finner
     møbelet via tag `GS_Int_Prop`. `ApplyBlast` går gjennom lista (avstandsfilter først, så linje).
   - Lokalt ved `State 2`: byggerens instans skaleres til 0, vår `SM_Int_<Type>_Knust` (én ISM per type, ignorerer `Pawn`,
     blokkerer kuler) på samme sted. bFresh: 8 trebiter / skjerm: gnistsprite + glassgranulat; lyd `Furniture_Topple` /
     `Screen_Break`. `OnWalkableChanged` med møbelboksen.
   - Testrommet: ett grupperomsbord og én veggskjerm med test-id-er, så alt kan prøves uten `Props()`.
7. **Største åpning per dør** (funn 1b): ved `BuildRegistry` på hver maskin sveipes bladets boks (bredde × 4 cm × høyde −
   10 cm) til 95°, 90°, 85°, 80° mot `WorldStatic` (objektspor, døra selv ignorert). Første vinkel uten treff blir
   `MaxOpenDeg` (minst 80°); `Open`-prosenten tolkes mot den. Logg antall dører under 95°. (Hodeløst treffer statiske spor
   ingenting – da blir alt 95°; prøves i PIE.)
8. **Modeller** `Tools/Blender/interactive_breach.py`: `SM_Int_Lettvegg_Hull` (2 × 12,5 mm gips på stålstendere c/c 60,
   takkete kanter, stendere som stikker ut, isolasjon i kanten), `SM_Int_Lettvegg_Rest`, `SM_Int_Gips_A..C`,
   `SM_Int_Door_Wood_Broken_A/B` (knekt på skrå, splintret kjerne), `SM_Int_Kantinebord_Knust`, `SM_Int_Rundbord_Knust`,
   `SM_Int_Grupperomsbord_Knust`, `SM_Int_Pult_Knust`, `SM_Int_Skjerm_Knust`, `SM_Int_Splint_A..C`. Opphav på gulvet i
   midten, X fremover (samme regel som Grafikks møbler). Reserve: kuber.
9. **Konsoll og selvtest:** `Interaktiv.Knus [antall=1] [skade=30]` gir N ekte treff der du ser. Selvtesten utvides:
   testveggen tåler 3 treff, får hull etter 16 × 30, blir borte etter et smell ved siden av; tredøra rives løs av 14 × 30;
   smell 1 m fra en branndør river den løs, 2,5 m fra (R 300) bare åpner den; bordet knuses av et smell; skjermen av ett
   skudd; `ApplyAllLocal` på nytt (sen klient) gir samme bilde. Alle tall med `UE_LOG` (lærdom 14a).

## Ferdig når (02a)
- `Build.bat` uten feil; hodeløs `Interaktiv.Selvtest` OK på alle gamle og nye punkter.
- PIE vert + klient: granat mot lettvegg i Helgasetr gir hull man går gjennom på begge maskiner; to granater gjør veggen
  borte; ett magasin mot en tredør river bladet løs; biter og støv bare lokalt; en klient som kobler til etter ser hull,
  borte dører og knuste bord, men ingen biter i lufta.
- `stat unit`: ingen merkbar økning med alle 6 veggene borte og 10 dører revet løs.

## Risiko (02a)
- Byggerens ISM må aldri miste instanser (vi skalerer til 0). `FindBreachWall`/`FindProp` kan svare før byggerens kø er
  tømt – samme vent-og-prøv-igjen som glasset (`TryBuildRegistry`).
- Hull-kollisjonen må ligge eksakt i veggplanet; feil yaw gir en vegg man går gjennom. Sjekk alle 6 med et lite skript
  (lærdom 1a) og i PIE.
- Kamps gjennomslag i gips gjelder hel vegg; i hullet finnes ingen kollisjon, så kuler går fritt – riktig.

---

# 02b – løse ting: plukke opp, bære, kaste, knuse, brannslukker, barrikader

## Mål
Stoler, puffer, flasker, søppelbøtter og brannslukkere kan plukkes opp, bæres, kastes, veltes og knuses; en skutt
brannslukker sprekker i en hvit sky; og en stol satt foran en dør stopper den til noen dytter eller sparker seg gjennom.

## Forbedrer (gammelt)
- **Bruk-tasten** (1a): finner også løse ting, og E/hold E får mening når man bærer noe.
- **Dører** (1a): stopper mot ting som står i svingen (barrikade) i stedet for å gå gjennom dem; spark skyver tingene unna.
- **Smell, kuler og slag** (K1–K3): dytter og velter løse ting, ikke bare dører og glass.
- **Glasskårene** (1b): gjenbrukes når en flaske knuses.
- **Faste bord fra 02a**: flaskene står på dem og faller når bordet knuses.

## Nytt
- `AGSPhysicsProp` (`GSPhysicsProp.h/.cpp`): seks slag løse ting med ekte vekt.
- Sovende instanser (null nettkostnad) som vekkes til en replikert aktør først når noen rører dem.
- Bære, sette ned, kaste (hold E), velte/dytte; flaske som lokkemiddel (botene hører den knuse).
- Brannslukker som sprekker: hvit pulversky 9 s, den spinner bort i et sekund.
- Barrikader foran dører med egen dytte-jobb.
- Modeller `Tools/Blender/interactive_props.py`.

## Filer
`GSPhysicsProp.h/.cpp` (ny), `GSMapState.h/.cpp`, `GSInteract.h/.cpp`, `GSDoor.cpp`, `GSBreakable.cpp` (gjenbruk av skår),
`Tools/Blender/interactive_props.py` (ny), `IMPROVEMENTS.md`.

## Steg
1. **Typetabell** (i `GSPhysicsProp.cpp`, `EGSPropKind`):

   | Type | kg | Knuses av | Kastefart | Modell |
   |---|---|---|---|---|
   | Trestol | 5 | 80 hp kuler/slag, smell ≤ 0,5 R | 6 m/s | Grafikks `SM_Mod_Helg_Trestol` |
   | Kontorstol | 12 | smell ≤ 0,3 R | 4 m/s | `SM_Mod_Helg_Kontorstol` |
   | Puff | 4 | aldri | 7 m/s | `SM_Mod_Helg_Puff` |
   | Flaske | 0,5 | 1 hp, eller treff > 4,5 m/s | 14 m/s | `SM_Int_Flaske` |
   | Brannslukker (6 kg pulver) | 9 | 25 hp kuler, ethvert smell innen R | 5 m/s | `SM_Int_Brannslukker` |
   | Søppelbøtte | 3 | aldri | 6 m/s | `SM_Int_Soppelbotte` |

   Alle kan bæres. Reserve når modellen mangler: kube med typens mål og farge.
2. **Sovende ting:** `UGSPropSet` (ISM per type og kilde, lokal på alle maskiner, bygget fra `Props()` der `bLos` er sann +
   testrommet; nøkkel `MakeKey(GSPart::Prop, Id)`). Objekttype `WorldDynamic`, blokkerer kuler, `PhysicsBody` og `Pawn`
   (ikke flaske). `SetCanEverAffectNavigation(false)` (Bots' bestilling). Ingen tick.
3. **Vekking (server):** `GSPhysicsProp::ServerWake(World, Key)` → `SetPart(State = 1)` og spawner `AGSPhysicsProp` på
   instansens transform. Utløses av kule (`ApplyBulletHit`: instans i `UGSPropSet` eller aktøren), slag, smell innen R og
   bruk-tasten. Alle maskiner: instansen skjules (skala 0) **først når aktøren med nøkkelen har kommet** (aktørens
   `BeginPlay` melder seg til `AGSMapState::PropActors`) – aldri tomrom, aldri dobbel. Lista sier 2 → skjules med en gang.
4. **`AGSPhysicsProp`** (replikert, `SetReplicateMovement(true)`, `NetCullDistanceSquared` = (50 m)², `NetPriority` 1):
   - Server simulerer (`SetSimulatePhysics`, `SetMassOverrideInKg`, demping 0,3/0,8). Klienter simulerer ikke: de leser
     `ReplicatedMovement` (overstyr `PostNetReceiveLocationAndRotation` → mål) og glir mot målet i `Tick` (12/s); tick av
     når de har stått stille 0,5 s.
   - Server sjekker hvert 0,25 s: står den stille 1,5 s → `ForceNetUpdate`, så `SetNetDormancy(DORM_DormantAll)`;
     vekkes → `FlushNetDormancy` + `DORM_Awake`. `NetUpdateFrequency` 15 i bevegelse.
   - Tak (server): ≤ 24 simulerer samtidig (de eldste legges til ro med `PutRigidBodyToSleep`), ≤ 160 vekkede aktører per
     kamp (over taket vekker kuler ikke flere; bruk og smell gjør det fortsatt). Logg tallene.
   - Treffimpuls: kule `Damage × 15` kg·cm/s langs kulas retning (`TraceStart → ImpactPoint`); smell
     `Strength × (1−d/R) × 40` fra smellet + litt opp; slag 300 kg·cm/s.
   - Skade i lista (`Damage`); knust → `State 2`, aktøren setter `bBroken` (replikert, `OnRep_Broken`: skjul, biter, lyd) og
     `SetLifeSpan(1 s)`, så effekten ikke tapes om slettingen kommer før lista.
5. **Bære/kaste** (`IGSInteract` på aktøren + fokus på sovende instanser):
   - `AGSInteractor::UpdateFocus`: etter `Usables` et kort spor (220 cm, objekttyper `WorldDynamic` + `PhysicsBody`) fra
     øyet; treff i `UGSPropSet` eller `AGSPhysicsProp` gir fokusnøkkelen. Bærer man noe, er fokus alltid det man bærer.
   - `HandleUse`: finnes ingen aktør for en `Prop`-nøkkel, vekkes den først (steg 3), så sjekkes avstand og linje mot
     aktøren som vanlig.
   - Ikke båret: **E** = ta opp (ingen andre bærer den, man bærer ikke noe). **Hold E** = dytt/velt (300 cm/s vannrett).
   - Båret: **E** = sett ned foran deg (sveip ned til gulvet, fysikk på, fart = figurens). **Hold E** = kast
     (siktretning × kastefart + figurens fart).
   - `UPROPERTY(ReplicatedUsing=OnRep_Carrier) TObjectPtr<AGSCharacter> Carrier`. Båret: ingen fysikk, ingen
     `ReplicateMovement`, kollisjon bare for spørringer (kuler treffer den; den dytter ingen), ikke dvale. Hver maskin legger
     den selv i `Tick`: øye + sikteretning × holdeavstand (stol 75 cm, flaske 45 cm) − 25 cm ned; eieren bruker
     `ControlRotation`, andre `GetBaseAimRotation()` (lærdom); et kulespor (15 cm) mot `WorldStatic` trekker den inn så den
     ikke går i veggen. Serveren regner sitt eget sted ved slipp (klienten sender ingenting).
   - Slipp av seg selv (server): bæreren død/borte, `UseItemId` satt, på terminal, eller tingen sitter fast > 2,5 m fra
     holdepunktet i 0,5 s. `ResetForMatch` sletter alle aktører, instansene kommer tilbake.
   - Tidssperre: ett ta-opp/kast per 0,4 s per spiller (lærdom: RPC-er som flytter ting).
   - Offentlig for Kamp (K6): `GSInteract::GetCarryMass(const AGSCharacter*)` (kg, 0 = ingenting) og `GetCarried(...)`.
6. **Treff mot figurer:** kastet ting (server `OnComponentHit`, fart > 6 m/s, mot `AGSCharacter`, ikke kasteren de første
   0,3 s) → `UGameplayStatics::ApplyDamage(min(20, kg × fart_m/s × 0,4), kasterens kontroller)`. Byggeren sjekker at
   `AGSCharacter::TakeDamage` gir drapskreditt for det; gjør den ikke det, droppes skaden og det skrives i `funn.md`.
7. **Flaske:** treff > 4,5 m/s eller én kule → knust: 6 skår i `Litter` (glassets `ShardA..`), `Glass_Break_Small`,
   `OnNoise` 20 m. Ingen skade.
8. **Brannslukker:** 25 hp eller smell innen R → `bBurst` (replikert, `OnRep_Burst`): alle maskiner `SpawnSmokeCloud(sted,
   350, (0.93,0.93,0.95), 9 s, 26)` + lyd `Extinguisher_Burst` (reserve `Glass_Break_Small`, lav tonehøyde). Server: 0,8 s
   skyvekraft 9000 kg·cm/s² motsatt treffretningen (den spinner og flyr), deretter tom (`_Tom`-modell) og vanlig løs ting.
   Server holder skyene: `GSMapState::ForEachCloud(World, Fn(LocCm, RadiusCm, UntilTime))` for Bots.
9. **Barrikader** (`GSDoor.cpp`, server, før enhver åpning eller lukking som ikke er smell eller spark-gjennom):
   - Finn vekkede `AGSPhysicsProp` (fra `PropActors`, avstand ≤ 1,5 × bredde fra hengselet) som ikke bæres og hvis boks
     ligger i bladets sving mellom nåværende og ønsket vinkel. Summert vekt ≥ 8 kg → døra stopper 5° før første ting.
     Brukeren får «Noe står i veien for døra».
   - **Hold E** på en blokkert dør = ny jobb `GSJob::Push` (2 s, «Dytter døra opp»): tingene får 150 cm/s ut av svingen, så
     åpnes døra helt. Spark (eksisterende jobb) skyver dem 300 cm/s og åpner. Smell som før.
   - Sovende instanser sjekkes ikke (Grafikk holder møbler ute av dørsving).
10. **Testrom:** 4 trestoler, 2 kontorstoler, 2 puffer, 6 flasker på bordet fra 02a, 2 brannslukkere, 1 søppelbøtte; plass
    foran tredøra til å bygge barrikade.
11. **Modeller** `Tools/Blender/interactive_props.py`: `SM_Int_Flaske` (grønn 0,5 l), `SM_Int_Brannslukker` (rød 6 kg pulver,
    slange, manometer) og `SM_Int_Brannslukker_Tom`, `SM_Int_Soppelbotte` (metall med lokk), `SM_Int_Stolbit_A..D` (sete,
    rygg, bein). Enkle kollisjonsbokser (UCX), ikke Nanite.
12. **Selvtest:** vekking skjuler instansen først når aktøren finnes; ta opp → `Carrier` satt, ingen fysikk; sett ned →
    ligger på gulvet; kast flaske mot vegg → knust + støy; skutt brannslukker → sky + `ForEachCloud` 1; stol foran døra
    stopper den under 95 % og `Push` åpner; 30 vekkede ting stille → alle i dvale etter 3 s; `ResetForMatch` → 0 aktører og
    alle instanser synlige. Test også selve E-tasten i PIE med vert + klient (lærdom 1a).

## Ferdig når (02b)
- `Build.bat` uten feil; hodeløs selvtest OK.
- PIE vert + klient: klienten tar opp en stol, bærer den, setter den foran døra; verten får den ikke opp før han holder E;
  en skutt brannslukker gir sky på begge maskiner; flasker knuses og skår blir liggende; sen klient ser tingene der de ble
  lagt.
- 20 spillere (bots) i Helgasetr: nettrafikk fra ting ≈ 0 når ingenting rører seg (alle i dvale), ≤ 24 simulerer.

## Risiko (02b)
- **Bæring før Kamps K6:** man kan skyte mens man bærer, og farten er som før. Ufarlig, men uferdig – bestilt.
- Klientinterpolering gir 70–100 ms etterslep på kastede ting; greit, skaden regnes på serveren.
- Fysikk på serveren: tak 24 simulerer, dvale – mål med `stat physics` i testrommet med 40 ting.
- En barrikade må aldri stenge en base for godt: dytt (2 s), spark og smell åpner alltid.
- `Props()` kan komme etter 02b: da har Helgasetr bare testrommets ting (vakten).

---

# Svar til Grafikk (bestillingen om `Props()`, deres plan-05b)
1. **Løse typer (`bLos = true`, vi legger dem ut):** `trestol`, `kontorstol`, `puff`. **Legg også til** som løse, med plass i
   deres sjekk: `brannslukker` (på gulvet ved hver trappedør/branndør, ca. 10), `flaske` (på kantinebordene, ca. 20),
   `soppelbotte` (ca. 8, korridorer og kantina). Alt annet fast.
2. **Format** (generert, med vakt så vi kompilerer før og etter 05b): `#define GS_HELGASETR_HAS_PROPS 1` og
   `struct FPropM { int32 Id; const TCHAR* Type; float X, Y, Z, Yaw, Sx, Sy, Sz; bool bLos; const TCHAR* Room; };`
   `inline TConstArrayView<FPropM> Props()`. `Sx/Sy/Sz` = modellens mål i meter (vi skalerer reserve- og knuste modeller).
3. **Tagg:** én felles `GS_Int_Prop` på alle faste møbel-ISM-er + `Prop_<type>` per ISM; ingen egen tag per ting. Siden
   indeks ≠ id med én ISM per type: **legg til** `bool AGSHelgasetrBuilder::FindProp(int32 Id, UInstancedStaticMeshComponent*&
   OutComp, int32& OutIndex) const` som `FindBreachWall`. Instanser fjernes aldri (vi skalerer til 0).
4. **Ødelagte modeller lager vi selv** (kantinebord, rundbord, grupperomsbord, pult, skjerm). Fra dere: opphav på gulvet i
   midten, X fremover, og de løse modellene som `SM_Mod_Helg_Trestol/Kontorstol/Puff` uten Nanite, med enkle kollisjonsbokser.
5. **Lettveggene:** modellen i samme ISM, tag `GS_Int_Breach`, oppslag med `FindBreachWall` – riktig; hull-modellen er vår.
6. **Runde atriehjørner: ja.** Glass-id-ene er bare nøkler i kjøretid; hold manifestet append-only og gjenbruk aldri en id.

# Bestillinger (står også i `bestillinger.md`)
- **Kamp (K6):** les `GSInteract::GetCarryMass(this)`: > 0 → ikke skyte/sikte/kaste, våpen senket, fart × max(0,6, 1 − kg/40)
  i alle fartsverdier (lærdom runde 4), kolbeslag = slipp. Lyder: `Plaster_Break`, `Door_Wood_Break`, `Furniture_Topple`,
  `Screen_Break` (finnes i deres liste) + nye `Bottle_Break`, `Extinguisher_Burst`, `Prop_Impact_Wood`, `Prop_Impact_Metal`,
  `Prop_Impact_Soft`, `Prop_Pickup`.
- **Bots:** dører uten blad (`FGSDoorInfo::bLeafGone`) er åpne ruter; blokkert dør → `BotUse(Hold)` starter dytt-jobben;
  `GSMapState::ForEachCloud` for sky som stenger sikt; kastede flasker gir `OnNoise`.

# IMPROVEMENTS-linjer
02a:
1000. Granater og C4 river hull i lettveggene i Helgasetr, så det åpnes nye ruter midt i kampen
1001. To smell (eller én C4) river lettveggen helt ned
1002. Du kan skyte hull i en lettvegg – ca. ett magasin mot samme vegg
1003. Gipsbiter, støvsky og gipsrester på gulvet når en vegg går i stykker
1004. Dører blåses ut av karmen av granater som går av like ved (branndører tåler mer)
1005. Tredører kan skytes i stykker
1006. En dør som er revet løs ligger på gulvet, også for den som kommer inn sent
1007. Kantinebord, rundbord, grupperomsbord og pulter knuses av granater og mange skudd
1008. Skjermer knuses av ett skudd
1009. Kulehull forsvinner sammen med veggen eller døra de satt i
1010. Dørene går ikke lenger inn i veggen når de åpnes helt
1011. Botene får vite at en sprengt vegg er en ny vei
1012. Nye modeller: lettvegg med hull, vegg-rester, gipsbiter, knust dør, knuste bord og skjerm
1013. Testrommet har lettvegg, bord og skjerm, og Interaktiv.Knus kan gi mange skudd på en gang

02b:
1014. Stoler, kontorstoler, puffer, flasker, søppelbøtter og brannslukkere er løse ting med ekte vekt
1015. Trykk E for å ta opp en ting og bære den, trykk igjen for å sette den ned
1016. Hold E for å kaste det du bærer – lette ting langt, tunge kort
1017. Hold E mot en ting for å dytte eller velte den
1018. Kuler, slag og granater dytter, velter og kaster løse ting
1019. Trestoler og flasker kan knuses
1020. Kastede flasker knuses og lager lyd som botene hører – lokk dem dit
1021. En skutt brannslukker sprekker i en hvit pulversky og spinner bort
1022. En kastet stol gjør litt skade på den den treffer
1023. Sett stoler eller annet foran en dør for å barrikadere den
1024. Hold E på en barrikadert dør for å dytte den opp; et spark skyver tingene unna
1025. Ting som ligger stille sover og koster ingenting i nettet; maks 24 simulerer samtidig
1026. Ting ingen har rørt er bare tegnet, ikke egne nettobjekter – plass til mange møbler
1027. Den som kommer inn sent ser tingene der de ble lagt
1028. Nye modeller: flaske, brannslukker, søppelbøtte og stolbiter
1029. Testrommet har stoler, puffer, flasker, brannslukkere og en dør å barrikadere
