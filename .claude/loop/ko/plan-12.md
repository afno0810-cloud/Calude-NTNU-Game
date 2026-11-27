# Plan 12 – lydrunde 2 (lag Kamp) – SKISSE: Helgasetr høres ut som Helgasetr

**Status: skisse.** Skrives ferdig når (a) plan-11 er bygget og kontrollert, (b) Grafikk har levert
`GSHelgasetr::SoundZones()` og fysiske materialer `PM_GS_*` (Grafikks runde 5–6), og (c) Interaktiv verden har
`GSMapState::OnBroken` og dørene (`AGSDoor`). Bygges etter plan-09 (se `retning.md`). IMPROVEMENTS-numre deles ut av
dirigenten da.

## Mål
Hvert rom i Helgasetr har sin egen lyd: romklang og romtone etter sonen, glass og dører som knuses og smeller med ekte
lyd, skudd som høres gjennom gipsvegger men ikke betong, og et bygg som lever (ventilasjon, lysrør, fjerne stemmer).

## Forbedrer (fra plan-11)
- `GSAudio::RoomAt`: bytter automatisk rommåling med Grafikks soner (`ZoneRT60` kobles). Romprøven blir reserve.
- Okklusjon fra av/på til vegg for vegg: egne spor lytter→kilde (maks 3 treff, budsjett 16 spor per bilde, cache 0,1 s
  per kilde), demping per materiale: glass −3 dB, gips −8 dB, tre −6 dB, betong −18 dB og lavpass 400 Hz. Dører som står
  åpne slipper lyden gjennom (spor gjennom døråpningen).
- Skudd i nabo-rom: «skudd i trapperommet under deg» høres med trapperommets hale.
- Nedslag: gjennomslag (når Kamps ballistikk del 2 finnes) får egen lyd: kula går gjennom gips (puff + smuler), tre (splint), glass.

## Nytt
- **Romtone per sone** (bestilles hos Grafikk, som eier `GSEnvironment`): `Amb_Corridor` (West Wolf skolekorridor),
  `Amb_Classroom`, `Amb_Atrium` (stort rom, svak mumling `Amb_Walla_Far` langt borte, lavpasset så språket ikke høres),
  `Amb_Stair` (hotell-trapperom), `Amb_Vent` (aircondition i tekniske rom og over himlinger), `Amb_Tech` (fyrrom),
  `Amb_Light_Hum` (lysrør, sterkere når lyset flimrer), `Amb_Rain_Window` (regn på glass når været er regn).
  Grafikk krysstoner mellom sonene; Kamp leverer filene og navnene.
- **Interaktiv verden** spiller med Kamps navn: glass (`Glass_Break_Large/Small`, `Glass_Shards` som faller etterpå,
  knas under føttene `Step_Glass`), dører (åpne/lukke/smell/spark/bryte, panikkstang, kodelås-pip, heisdør),
  gips som sprenges, møbler som velter og skrapes, skjerm som knuses. Kamp legger `GSAudio::PlayBreak(Kind, Loc, Size)`
  som velger variant og romhale, så Interaktiv bare gjør ett kall fra `OnBroken`.
- **Kroppen høres** (backlog 14): stønn ved treff, gisp ved lavt liv, pust etter sprint, døende pust – fra Pole Position
  The Grunts Library (GDC 2024), bare ekte stemmer, ingen «spill-skrik».
- **Flagg-radio** med ekte radiosus og (valgfritt) norske radiomeldinger Andreas leser inn selv («Flagget er tatt»):
  `SourceArt/Sounds/Voice/` – han spiller inn på mobilen; vi klipper med `prep_sounds.py`.
- **Menylyder**: `GSAudio::PlayUI` i `GSMenu` (bestilles hos Spillbarhet), Kenney Interface Sounds.
- **Vurder MetaSound fra C++** (`UMetaSoundBuilderSubsystem`) bare for to ting: sømløs automatild (løkke + hale i
  stedet for én fil per kule) og ventilasjon med variasjon. Hvis byggeren ikke får det til uten editoren innen en time,
  dropp det; fil-per-skudd fra plan-11 holder.

## Filer (forventet)
`GSAudio.*`, `GSBallistics.cpp` (gjennomslagslyd), `GSCharacter.cpp` (kroppslyder), `GSHUD.cpp` (radio), `Tools/Audio/*`,
`SourceArt/Sounds/*`. Andre lag: `GSEnvironment` (Grafikk), `GSDoor`/`GSBreakable`/`GSMapState` (Interaktiv), `GSMenu`
(Spillbarhet) – bare via bestillinger.

## Risiko
Vegg-for-vegg-okklusjon koster spor: budsjett per bilde og cache. Romtone i 6 soner samtidig = 6 løkker; bare de to
nærmeste spilles. Avhenger av tre andre lag – planlegg slik at hver del virker alene.
