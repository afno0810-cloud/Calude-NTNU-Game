# Plan 02 (Interaktiv verden) – SKISSE: sprenge, bære og barrikadere

Skisse, ikke byggeklar. Skrives ut etter at runde 1 er kontrollert (bruk kontrollørens funn). Bygger på `AGSMapState`,
`IGSInteract`/`AGSInteractor`, `UGSGlassSet`, `AGSDoor` og testrommet fra runde 1.

## Mål
Granater og C4 river ned lettvegger og blåser dører ut av karmen så det åpnes nye ruter; møbler knuses; stoler, puffer,
søppelbøtter, flasker og brannslukkere kan plukkes opp, bæres, kastes og stables som barrikade foran en dør.

## Innhold
1. **Sprengbare lettvegger** (`GSPart::Breach`, Grafikks `BreachWalls()`: 4 per side, 2,4 × 2,8 m gips): vi overtar
   ISM-ene (tag `GS_Int_Breach`) som i runde 1 for glass. Tilstand 0 hel / 1 sprukket (kuler og slag, hull-merker) /
   2 hull (C4 eller granat innen 1,5 m: åpning 1,2 × 2,0 m – modell `SM_Int_Lettvegg_Hull` med takkete kanter og stendere
   som stikker ut) / 3 borte (to sprengninger). Lokale gipsbiter (10–15, `SM_Int_Gips_*`), støvsky (`SpawnSmokeCloud`
   hvit-grå, 4 s), gipsrester på gulvet som instanser. `OnWalkableChanged` med veggens boks → Bots. Kamp: gjennomslag i
   gips (allerede bestilt hos Kamp via materialene).
2. **Dører som sprenges:** innen 0,6 × R: bladet rives løs (`State` bit 3) – lokal fysikkdør som faller (`SM_Int_Door_*_Broken`
   i to deler), karmen står. Låsen er da uten betydning. Alarmdører (`Fire`, låst) kan bare åpnes slik.
3. **Møbler som knuses:** Grafikks faste møbler (pulter, bord, skjermer) som hel/ødelagt modell (`GSPart::Prop`, statisk
   variant): treff/trykk → ødelagt modell + biter. Liste fra Grafikk (`GSHelgasetr::Props()` med id, type, sted) –
   bestilles når runde 2 skrives.
4. **`AGSPhysicsProp`** (løse ting, `GSPhysicsProp.*`): stoler med hjul, puffer, søppelbøtter, flasker, kaffekopper,
   brannslukkere, bøker. **Replikert aktør** (ikke del av FastArray – de flytter seg), `bReplicateMovement`, server
   simulerer, klienter interpolerer; sover (`PutToSleep`) og går i dvale (`DORM_DormantAll`) når de ligger stille, våkner
   ved treff. Tak: 60 simulerer samtidig (helgasetr.md kap. 9), de eldste legges til ro. Kuler dytter (impuls fra
   `ApplyBulletHit`), trykkbølger kaster dem (`ApplyBlast` med fall-off), spark dytter.
5. **Plukke opp / bære / kaste** (`IGSInteract` på propen): `Tap` = plukk opp (bæres foran deg, festet til en
   `UPhysicsHandleComponent` på serveren, våpenet senkes – bestill «hender opptatt»-tilstand hos Kamp: ikke skyte mens
   du bærer, saktere sprint etter vekt), `Tap` igjen = sett ned, hold = kast (fart etter vekt, 4–12 m/s), slag mens du
   bærer = slipp. Små ting (flaske, kopp) kan kastes som distraksjon: lyd der den lander → `OnNoise` (Bots hører det).
   Glassflasker knuses mot vegg (lokale skår, lyd). Brannslukker skutt → hvit sky + dytt (backlog).
6. **Barrikader:** en båret stol/pult satt foran en dør blokkerer den: døra stopper mot propen (svingen sjekkes med en
   sweep på serveren; blokkert = `Open` stopper på vinkelen der den treffer) og kan bare åpnes ved å dytte (hold E:
   2 s, propen skyves) eller sparkes (3 spark velter barrikaden). Pult foran glassvegg = dekning.
7. **Testrommet** utvides med en lettvegg, en pult, 4 stoler, 2 puffer, en søppelbøtte, 6 flasker og en brannslukker.

## Bestillinger som trengs da
- Grafikk: `BreachWalls()` stabile id-er og tag; `Props()`-liste for faste møbler; modeller for ødelagte møbler kan vi
  lage selv i `Tools/Blender/interactive_props.py`.
- Kamp: «hender opptatt» (ikke sikte/skyte, ingen våpen i hånden) mens man bærer; lyder (gips, møbel som velter, flaske);
  impuls fra kulas fart i `ApplyBulletHit` (vi kan regne den selv hvis `Hit` + fart sendes).
- Bots: navnett-oppdatering ved `OnWalkableChanged` (alt bestilt i runde 1), og at de kan dytte/sparke barrikader.
- Spillbarhet: ingen.

## Risiko
Replikerte fysikkting er det dyreste i nettet: hold antallet nede (ca. 80 i hele Helgasetr), dvale når de ligger stille,
og ingen fysikk på klienten som påvirker spillere (props blokkerer `Pawn` men dytter ikke). Barrikader må ikke kunne stenge
en base for godt: en spark eller granat må alltid åpne.

## IMPROVEMENTS
Ca. 16 linjer; nummerserien deles ut av dirigenten.
