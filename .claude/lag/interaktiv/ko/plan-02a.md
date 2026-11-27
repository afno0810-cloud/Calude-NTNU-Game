# Plan 02a (Interaktiv verden): lettvegger og dørblad som sprenges og skytes i stykker

Andreas: «granater kan få deler av mappet til å sprenge», «skyte hull i vegger», «ødelegge ting». Skrevet om etter
forhåndssjekken (`kontroll/forhand-plan-02.md`, punkt 1–7 og 19–21). Del 2 av 3: 02b = faste møbler og løse ting,
02c = bære/kaste/barrikader.

**Bygger på 1b-rettingene F1 og F2** (`kontroll/runde-01b.md`), som byggeren gjør nå og som ikke er en del av denne planen:
F1 = taket på biter tas når bitene lages (plass = 150 − i lufta, høyst ca. 40 nye per bilde, resten rett som rester på
gulvet); F2 = `ApplyMelee` knuser bare glass foran første vegg. 02a bruker begge og endrer dem ikke.

**Kamps grensesnitt holdes uendret** (Kamps plan-13 bygges nå i `GSBallistics/GSProjectile/GSCharacter`; 02a endrer ingen
av dem): `AGSMapState::GetGlass()`, `FGlassPane` (`State`, `Def.Id/Type/CenterCm/YawDeg/LengthCm/HeightCm`), `GetPart`,
`NumGlassBroken`, `IsRegistryBuilt`, og signaturene til `ApplyBlast`, `ApplyBulletHit`, `ApplyMelee`, `BulletSegment`,
`AddBulletMark`. `ApplyMelee` returnerer fortsatt «avstand fra Start til det som stoppet slaget»; en lettvegg som stopper
slaget gir sin avstand.

## Mål
En granat eller C4 river hull i lettveggene og blåser dører ut av karmen, og nok skudd gjør det samme – nye ruter midt i
kampen, likt for alle 20 spillere, uten ny nettkostnad per kule.

## Forbedrer (gammelt)
- **Granater mot dører** (1a): flyr i dag bare opp. Nå rives bladet ut av karmen innen 0,6 R (tre/glass) / 0,4 R (branndør).
- **Kuler mot tredører** (1a: bare låsen): bladet går i stykker etter ca. ett magasin.
- **Lettveggene** (Grafikks 6 `BreachWalls`, i dag døde): kan sprenges og skytes i stykker.
- **Kulehull** (1b): forsvinner sammen med veggen eller dørbladet de satt i; lettvegger får egen grense på 40 hull.
- **Dørsving** (funn 1b, dør 23/25/28 går 7–8 cm inn i veggen): største åpning per dør, regnet fra kartdataene.
- **`Interaktiv.Knus`**: tar `[antall] [skade]`.

## Nytt
- Lettvegg: hel → hull (1,2 × 2,0 m, gjennomgang) → borte; gipsbiter, støvsky, gipsrester på gulvet.
- Dørblad som rives løs og faller (to halvdeler for tre, helt blad for stål), splinter.
- Modeller i `Tools/Blender/interactive_breach.py`.

## Hvorfor nå
1a/1b står, Kamps K1–K3 er koblet på, så granater og kuler når oss allerede. Ødeleggelse av vegger og dører er det Andreas
ba mest om og endrer kampen mest. Alt ligger i lista som finnes.

## Filer
`GSMapState.h/.cpp`, `GSBreakable.h/.cpp`, `GSDoor.h/.cpp`, `Tools/Blender/interactive_breach.py` (ny), og et lite skript
`Tools/Blender/interactive_door_limits.py` (ny, vårt) som skriver åpningstabellen. `IMPROVEMENTS.md`.
Leser: `GSHelgasetr.h` (`BreachWalls()`, `DoorSlots()`, `Boxes()`), `AGSHelgasetrBuilder::FindBreachWall`,
`GSNavSetup::NotifyMapChanged`, `AGSShortFX::SpawnSmokeCloud`, `GSAudio::PlayAt`.

## Steg
1. **Delskade bare på serveren** (punkt 7): `AGSMapState` får `TMap<int32, float> ServerDamage` (ikke replikert, tømmes i
   `DoResetForMatch`). Lista replikerer bare `State` (lettvegg 0 hel / 1 hull / 2 borte) og `GSLock::LeafGone` for dører.
   `FGSPartState` endres ikke. Skade akkumuleres som flyttall (svake treff gir aldri 0).
2. **Lettvegger – data** (`GSBreakable.cpp`, som glasset i 1b): `FGSBreachDef { Id, CenterCm, SizeCm (lengde langs veggen,
   tykkelse, høyde), YawDeg }` fra `BreachWalls()` (yaw 90 når X-utstrekningen er minst; logg alle 6) + én testvegg
   (240 × 10 × 280 cm, id `TestIdBase+1`). Nøkkel `MakeKey(GSPart::Breach, Id)`. Ved `BuildRegistry`: `FindBreachWall(Id,
   Comp, Index)` lagres + omvendt kart (Comp, Index) → nøkkel (vent-og-prøv-igjen som glasset hvis byggerens kø ikke er tom).
3. **Lettvegger – skade (server), i hp:**
   - Kule (`ApplyBulletHit`, komponent med tag `GS_Int_Breach`): `+Damage`. ≥ 480 hp → `State 1`. (AG3 38 → 13 skudd,
     5,56 16, MP5 23, hagle 5.)
   - Slag (`ApplyMelee`, etter F2: første treff langs sveipen er lettveggen): +40 hp; returnerer avstanden til veggen.
   - Smell (`ApplyBlast`): d = avstand fra smellet til nærmeste punkt i veggboksen; fri linje (objektspor `WorldStatic`, et
     treff på selve veggen teller som fritt). d ≤ max(150 cm, 0,5 R) → ett trinn opp (0→1, 1→2). **`Strength ≥ 180`
     (bare C4, 200)** og d ≤ 0,5 R → rett til 2. Ellers `+Strength · (1 − d/R) · 4` hp.
   - Ny tilstand: `Cause`, `HitDir`, `OnWalkableChanged` med veggboksen, `GSNavSetup::NotifyMapChanged(World, Box)`,
     `OnNoise` 40 m.
4. **Lettvegger – lokalt** (`ApplyBreachLocal(Item, bFresh)` fra `ApplyLocal`, alle maskiner, også sen klient):
   - State ≥ 1: byggerens instans skaleres til 0 (`UpdateInstanceTransform`, én gang per brudd). ISM-en er `Static`:
     bekreft i PIE at veggen faktisk blir borte i bildet (punkt 20). Kulehull med veggens nøkkel fjernes
     (`Marks->RemoveKey`); `AddMark` setter derfor nøkkelen når treffet er en lettvegg, med egen grense 40 per vegg
     (`PerKeyCap` per kategori; dører beholder 12; totaltaket 400 står).
   - State 1: `SM_Int_Lettvegg_Hull` (synlig, `NoCollision`) skalert til veggens mål + tre usynlige kollisjonsbokser (venstre
     0,6 m, høyre 0,6 m, overligger 0,8 m) i én egen ISM per kilde med `Mat_Plaster`-tag og samme kollisjon som veggen.
     Mangler modellen: boksene vises med gipsmaterialet.
   - State 2: `SM_Int_Lettvegg_Rest` (sokkel + bøyde stendere, `NoCollision`).
   - bFresh: 12 gipsbiter gjennom en generalisert `AGSDebris::InitBits(Key, BoxTransform, SizeCm, Count, Meshes, Mat,
     HitDir, LitterVisual)` (glasset kaller den med dagens verdier) – **F1s tak gjelder** (bare så mange som får plass;
     resten legges rett i `Litter`). Støvsky `SpawnSmokeCloud(midt, 220, (0.82,0.80,0.76), 4 s, 10)`, lyd `Plaster_Break`
     (reserve `Door_Wood_Break`). Alltid (også sen klient): 6 gipsklumper i `Litter` (ny visual `GSLitterVis::Gips`,
     `SM_Int_Gips_A..C`), én gang per nøkkel.
5. **Dørblad som rives løs** (`GSDoor.cpp`; bit `GSLock::LeafGone` finnes):
   - **Hva er bladet** (punkt 21): komponentene i `Blockers` minus ruta, håndtaket og kodelåsen. Karm og vegg teller ikke.
   - Kule mot bladet på `Wood`/`Double`: `+Damage` i `ServerDamage`; ≥ 400 hp → `LeafGone`. Glassdørens ramme og `Fire`
     tar ikke kuleskade på bladet (bare låsen som før). Slag: +25 hp.
   - `ApplyBlast`: innen 0,6 R (`Fire`: 0,4 R) med fri linje → `LeafGone` + lås `Broken`; ellers som før (`ServerOpenAway`).
   - Lokalt: bladet skjules (synlighet av + `NoCollision`, også ruta og kodelåsen), kulehull med dørens nøkkel fjernes.
     bFresh: fallende dør gjennom `InitBits` (`SM_Int_Door_Wood_Broken_A/B`; stål: dørens bladmodell) langs `HitDir`,
     8 splinter, lyd `Door_Wood_Break` – innenfor F1s tak; får de ikke plass, legges bladet rett på gulvet. Sen klient:
     bladet ligger flatt på gulvet 1 m ut langs `HitDir` (fast transform, ikke kollisjon for figurer).
   - `GetUsePrompt` → false, `IsDoorPassable` → true, `FGSDoorInfo` får `bool bLeafGone` (og `bOpen = true`). `BotUse`
     hopper over dører uten blad.
6. **Største åpning per dør fra data** (punkt 4): `interactive_door_limits.py` leser `helgasetr_plan.py` (dører + bokser,
   samme kilde som `gen_helgasetr.py`) og regner rent geometrisk, for hver dør, største vinkel ≤ 95° der bladet (bredde ×
   4 cm, i plan) ikke går inn i en boks på samme plan (5 cm klaring), minst 80°. Skriver `GSDoorLimits.gen.inl`
   (`{ DoorId, MaxOpenDeg }`, bare dører under 95°), som `GSDoor.cpp` inkluderer. Likt på alle maskiner; `Open`-prosenten
   tolkes mot `MaxOpenDeg`. Testrommets dører: 95°. Skriptet logger dør 23/25/28 og motprøver én dør med fri sving (95°).
7. **Modeller** `Tools/Blender/interactive_breach.py`: `SM_Int_Lettvegg_Hull` (2 × 12,5 mm gips på stålstendere c/c 60,
   takkete kanter, stendere som stikker ut, isolasjon i kanten), `SM_Int_Lettvegg_Rest`, `SM_Int_Gips_A..C`,
   `SM_Int_Door_Wood_Broken_A/B`, `SM_Int_Splint_A..C`. Opphav gulv-midt, X fremover. Reserve: kuber.
8. **Konsoll og selvtest:** `Interaktiv.Knus [antall=1] [skade=30]` gir N ekte treff der du ser. Selvtesten: testveggen tåler
   3 × 30, får hull etter 16 × 30, blir borte etter et smell ved siden av; C4-styrke (200) 1 m unna river en hel vegg rett
   ned, splintgranat (120) bare til hull; tredøra rives løs av 14 × 30; smell 1 m fra en branndør river den løs, 2,5 m fra
   (R 300) bare åpner den; ingen kule mot vegg eller dør gir endring i lista før tilstanden skifter (tell `MarkItemDirty`);
   `ApplyAllLocal` på nytt (sen klient) gir samme bilde; dør 23/25/28 har `MaxOpenDeg < 95`. Alle tall med `UE_LOG`.

## Ferdig når
- `Build.bat` uten feil; hodeløs `Interaktiv.Selvtest` OK på gamle og nye punkter.
- PIE vert + klient: granat mot lettvegg i Helgasetr gir hull man går gjennom på begge maskiner; veggen er borte fra bildet
  (Static-ISM, punkt 20); én C4 river den helt ned; ett magasin mot en tredør river bladet løs; biter innenfor taket 150;
  sen klient ser hull og borte dører, ingen biter i lufta.
- **Siktreglene med åpne vegger** (punkt 5): Grafikks `check_helgasetr.py` sjekk c, d, e og e2 kjøres med alle 6 lettvegger
  borte (bryteren `--breach-open`, bestilt hos Grafikk) og er OK. Er bryteren ikke levert, kjører byggeren en egen kopi av
  sjekken med lettveggene fjernet og legger utskriften i rapporten.
- `stat unit`: ingen merkbar økning med alle 6 vegger borte og 10 dører revet løs.

## Risiko
- Hull-kollisjonen må ligge eksakt i veggplanet; feil yaw gir en vegg man går gjennom. Sjekk alle 6 (lærdom 1a) og i PIE.
- Kamps gjennomslag i gips gjelder hel vegg; i hullet er det ingen kollisjon – riktig.
- Åpningstabellen må lages på nytt når Grafikk flytter dører/vegger: skriptet kjøres i samme kjede som `gen_helgasetr.py`
  (byggeren skriver det i skriptets hode; Grafikk får beskjed i bestillingen).

## Notater til byggeren (fra forhåndssjekken)
- Pkt. 19–21 er tatt inn over. Pkt. 15–18 og 22 gjelder 02b/02c.

## IMPROVEMENTS-linjer
1000. Granater og C4 river hull i lettveggene i Helgasetr, så det åpnes nye ruter midt i kampen
1001. To granater eller én C4 river lettveggen helt ned
1002. Du kan skyte hull i en lettvegg – ca. ett magasin mot samme vegg
1003. Gipsbiter, støvsky og gipsrester på gulvet når en vegg går i stykker
1004. Dører blåses ut av karmen av granater som går av like ved (branndører tåler mer)
1005. Tredører kan skytes i stykker
1006. En dør som er revet løs ligger på gulvet, også for den som kommer inn sent
1007. Kulehull forsvinner sammen med veggen eller døra de satt i, og en lettvegg viser opptil 40 hull
1008. Dørene går ikke lenger inn i veggen når de åpnes helt (største åpning regnet fra kartet)
1009. Botene får vite at en sprengt vegg er en ny vei
1010. Skudd mot vegger og dører sendes ikke over nettet før noe faktisk går i stykker
1011. Nye modeller: lettvegg med hull, vegg-rester, gipsbiter, knust dør og splinter
1012. Testrommet har en lettvegg, og Interaktiv.Knus kan gi mange skudd på en gang
