# Plan 02b (Interaktiv verden): faste møbler som knuses og løse ting med fysikk

Andreas: «ødelegge ting», «knuse ting», «så mye som mulig interaktivt». Skrevet om etter forhåndssjekken
(`kontroll/forhand-plan-02.md`, punkt 6, 8–12, 15, 16, 22). Bygges etter 02a. Bære/kaste/barrikader er i 02c.
Kamps grensesnitt holdes uendret (se 02a); `BulletSegment` får nye treff innenfor dagens signatur.

## Mål
Bord og veggmonitorer knuses, og stoler, puffer, flasker, søppelbøtter og brannslukkere dyttes, veltes, kastes av smell og
knuses av kuler – en skutt brannslukker sprekker i en hvit sky. Alt likt for 20 spillere, og ting som ligger stille koster
ingenting i nettet.

## Forbedrer (gammelt)
- **Kuler** (`BulletSegment`, 1b): finner nå også løse ting på samme strekning; de tar skade og dytt, og kula bremses litt.
- **Smell og slag** (K2/K3): dytter, velter og knuser møbler, ikke bare dører og glass.
- **Glasskårene** (1b): gjenbrukes når en flaske knuses.
- **Grafikks møbler** (05b): de faste knusbare blir levende; de løse tar vi over.

## Nytt
- Faste møbler som knuses: kantinebord, rundbord, grupperomsbord, pult; veggmonitor (`skjerm`).
- `AGSPhysicsProp` (`GSPhysicsProp.h/.cpp`): trestol, kontorstol, puff, flaske, brannslukker, søppelbøtte.
- Sovende instanser som vekkes til replikerte aktører; dvale med nettvekking; hardt tak 24 simulerer / 160 aktører.
- Brannslukker som sprekker (hvit sky 9 s, spinner bort); flaske som knuses og lager støy.
- Modeller `Tools/Blender/interactive_props.py` og knuste møbler i `interactive_breach.py`.

## Filer
`GSPhysicsProp.h/.cpp` (ny), `GSMapState.h/.cpp`, `GSBreakable.cpp`, `GSInteract.cpp` (bare fokusregelen i steg 9),
`Tools/Blender/interactive_props.py` (ny), `interactive_breach.py`, `IMPROVEMENTS.md`.

## Steg
1. **Data:** vakt `#if defined(GS_HELGASETR_HAS_PROPS)`; uten `Props()` har bare testrommet ting. Nøkkel
   `MakeKey(GSPart::Prop, Id)` (test: `TestIdBase+`).
2. **Faste møbler** (`GSBreakable.cpp`):
   - Knusbare: `kantinebord`, `rundbord`, `grupperomsbord`, `pult` (240 hp i `ServerDamage`; smell innen 0,5 R knuser) og
     **bare veggmonitoren** `skjerm` (første kule ≥ 10, eller smell innen R). **Aldri** skjermvegger eller noe annet som
     Grafikks siktregler (e2) bruker. Møbler med `bCover` (dekningsdeler i e3) tar bare kulehull.
   - Omvendt kart (Comp, Index) → nøkkel fra `FindProp`. Kuler finner dem via tag `GS_Int_Prop`.
   - `State 2` lokalt: byggerens instans skaleres til 0, `SM_Int_<Type>_Knust` (ISM per type, ignorerer `Pawn`, blokkerer
     kuler) på samme sted; bFresh: trebiter / gnister + glassgranulat innenfor F1s tak, lyd `Furniture_Topple` /
     `Screen_Break`; `OnWalkableChanged`.
3. **Typetabell for løse ting** (`EGSPropKind`): trestol 5 kg (80 hp, smell ≤ 0,5 R), kontorstol 12 kg (smell ≤ 0,3 R),
   puff 4 kg (knuses ikke), flaske 0,5 kg (1 hp eller treff > 4,5 m/s), brannslukker 9 kg (25 hp eller ethvert smell innen R),
   søppelbøtte 3 kg (knuses ikke). Kulefaktor (punkt 9): stol/puff/bøtte 0,95, flaske 1,0, brannslukker 0 (stopper).
   Modeller: Grafikks `SM_Mod_Helg_Trestol/Kontorstol/Puff`, våre `SM_Int_Flaske/Brannslukker(_Tom)/Soppelbotte`;
   reserve: kube med typens mål.
4. **Sovende ting:** `UGSPropSet` (ISM per type og kilde, lokal, fra `Props()` med `bLos` + testrommet). Objekttype
   `WorldDynamic`, **ignorerer `Visibility`** (kuler går ikke på dem som vegg; de finnes i steg 6), blokkerer `PhysicsBody` og
   `Pawn` (ikke flaske). `SetCanEverAffectNavigation(false)`. Ingen tick.
5. **Vekking (server)** `GSPhysicsProp::ServerWake(World, Key)` → `SetPart(State 1)` + spawn av `AGSPhysicsProp` på
   instansens transform. Alle maskiner: ved `State ≥ 1` slås **instansens kollisjon av med en gang** (punkt 11); synligheten
   skrus av først når aktøren har meldt seg (`PropActors`), høyst 1 s etter. `State 2` → skjult med en gang.
6. **Kuler mot løse ting** (punkt 9): i `BulletSegment` samme objektspor også mot prop-settene og prop-aktørene på
   strekningen. Ekte kule på serveren: skade + impuls (`Damage × 15` kg·cm/s langs kula) og vekking; vist kule: ingenting.
   Faktoren ganges inn i returverdien (brannslukker: 0 – kula stopper der). Kamps kall er uendret.
7. **`AGSPhysicsProp`** (replikert, `SetReplicateMovement(true)`, `NetCullDistanceSquared` = (150 m)² så hele bygget dekkes,
   `NetPriority` 1, `SetCanEverAffectNavigation(false)`):
   - Server simulerer (masse, demping 0,3/0,8). Klienter simulerer ikke: `PostNetReceiveLocationAndRotation` → mål, og
     **interpolering hvert bilde** mens den beveger seg (punkt 16); tick av etter 0,5 s stille.
   - **Dvale med nettvekking** (punkt 10): stille 1,5 s → `ForceNetUpdate` + `DORM_DormantAll`. Bind `OnComponentWake` på
     roten → `FlushNetDormancy` + `DORM_Awake`, så en stol som dyttes av en figur eller et dørblad i dvale vises flytte seg.
     0,25 s-sjekken går over alle våkne kropper.
   - **Hardt tak** (punkt 12): ≤ 24 simulerer, ≤ 160 aktører. Et smell vekker høyst de 24 nærmeste; de andre innen R får
     skade/knuses uten å bli aktører. Er det 160 aktører, tas den eldste rolige tilbake til instans: lista får sted og yaw
     (`FVector_NetQuantize` + `uint8` yaw, ca. 10 B) i en ny `TArray<FGSPropRest> Rests` (FastArray, nøkkel + sted), aktøren
     slettes, og instansen flyttes dit og får kollisjon igjen på alle maskiner (også sen klient). Bruk-vekking venter ikke.
   - Smell: impuls `Strength × (1 − d/R) × 40` fra smellet + litt opp; slag 300 kg·cm/s.
   - Knust → `State 2`, `bBroken` (replikert, `OnRep_Broken`: skjul, biter, lyd), `SetLifeSpan(1 s)`.
   - **Figurer dytter** (punkt 15): server, ting < 15 kg som overlapper en figurs kapsel får 120 cm/s bort fra den (vekkes);
     ingen stabel av stoler kan stenge en spawnsluse.
8. **Flaske og brannslukker:** flaske knust → 6 skår i `Litter` (glassets `ShardA..`), `Glass_Break_Small`, `OnNoise` 20 m.
   Brannslukker → `bBurst` (replikert, `OnRep_Burst`): `SpawnSmokeCloud(sted, 350, (0.93,0.93,0.95), 9 s, 26)` + lyd
   `Extinguisher_Burst` (reserve `Glass_Break_Small` lav tonehøyde); server: 0,8 s skyvekraft 9000 kg·cm/s² motsatt
   treffet, så tom modell. `GSMapState::ForEachCloud(World, Fn(LocCm, RadiusCm, UntilTime))` for Bots.
9. **Bruk-tasten i 02b:** **Hold E** mot en ting = dytt/velt (300 cm/s vannrett), vekker den først. Fokus på løse ting
   (kort spor 220 cm mot `WorldDynamic` + `PhysicsBody`) **viker for Kamps `FocusPickup`** som dørene gjør (punkt 18). Ta opp
   og kast kommer i 02c.
10. **Testrom:** grupperomsbord, veggmonitor, 4 trestoler, 2 kontorstoler, 2 puffer, 6 flasker på bordet, 2 brannslukkere,
    1 søppelbøtte.
11. **Modeller:** `interactive_props.py`: `SM_Int_Flaske`, `SM_Int_Brannslukker` + `_Tom`, `SM_Int_Soppelbotte`,
    `SM_Int_Stolbit_A..D` (UCX, ikke Nanite). `interactive_breach.py`: `SM_Int_Kantinebord/Rundbord/Grupperomsbord/Pult/
    Skjerm_Knust`.
12. **Selvtest:** vekking slår av kollisjon straks og skjuler instansen når aktøren finnes; kule gjennom en puff → faktor
    0,95, gjennom brannslukker → 0 + sky + `ForEachCloud` 1; flaske skutt → skår + støy; smell blant 40 ting → ≤ 24
    aktører nye; 161. aktør → den eldste rolige blir instans på sitt nye sted (og etter `ApplyAllLocal`); 30 rolige → alle i
    dvale etter 3 s, et dytt vekker én på nettet; bord og monitor knuses; `ResetForMatch` → 0 aktører, alle instanser hjemme.

## Ferdig når
- `Build.bat` uten feil; hodeløs selvtest OK.
- PIE vert + klient med `NetEmulation PktLag=150`: å gå gjennom en klynge stoler gir ikke rykk; en skutt brannslukker gir
  sky på begge; en klient 80 m unna ser samme ting; sen klient ser tingene der de ble liggende.
- 20 spillere (bots) i Helgasetr: nettrafikk fra ting ≈ 0 når ingenting rører seg; `stat physics` med 40 ting i testrommet.

## Risiko
- `Props()` kan komme etter 02b: Helgasetr får da bare testrommets ting (vakten).
- `Rests`-lista er en ny replikert liste: bare skrevet når taket nås, så den er nesten alltid tom.
- Klientinterpolering gir 70–100 ms etterslep; skade regnes alltid på serveren.

## Notater til byggeren
- Pkt. 16: interpoler hvert bilde mens tingen beveger seg; høyst 24 er i bevegelse.
- Pkt. 15: dyttingen er server-side og kun for ting under 15 kg.

## IMPROVEMENTS-linjer
1013. Kantinebord, rundbord, grupperomsbord og pulter knuses av granater og mange skudd
1014. Veggmonitorer knuses av ett skudd
1015. Stoler, kontorstoler, puffer, flasker, søppelbøtter og brannslukkere er løse ting med ekte vekt
1016. Kuler, slag og granater dytter, velter og kaster løse ting
1017. Kuler går gjennom stoler og puffer, men stopper i en brannslukker
1018. Trestoler og flasker kan knuses
1019. Knuste flasker lager lyd som botene hører
1020. En skutt brannslukker sprekker i en hvit pulversky og spinner bort
1021. Hold E mot en ting for å dytte eller velte den
1022. Du skyver løse ting unna når du går inn i dem – ingen kan stenge en spawnutgang med stoler
1023. Ting som ligger stille sover og koster ingenting i nettet, og våkner for alle når de dyttes
1024. Ting ingen har rørt er bare tegnet, ikke egne nettobjekter – plass til mange møbler
1025. Fast tak: høyst 24 ting i bevegelse og 160 vekket; de eldste legges tilbake der de ligger
1026. Den som kommer inn sent ser tingene der de ble liggende
1027. Nye modeller: flaske, brannslukker, søppelbøtte, stolbiter og knuste bord og monitor
1028. Testrommet har bord, monitor, stoler, puffer, flasker og brannslukkere
