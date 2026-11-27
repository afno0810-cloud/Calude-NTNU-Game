# Plan 03 (Interaktiv verden) – SKISSE: bygget som virker

Skisse, ikke byggeklar. Skrives ut etter runde 2. Alt bygger på `AGSMapState` (`GSPart::Device`), `IGSInteract` og
testrommet. Modellene lager vi i `Tools/Blender/interactive_devices.py`; plasseringene kommer fra Grafikks data.

## Mål
Bygget lever: lysbrytere slukker rommet (godt for bakhold), heisene går mellom Plan 1 og 4, kiosken og kjøleskapene i
Ryddekroken kan tømmes for mat og drikke, skapene kan åpnes, pianoet og bordtennisbordet kan brukes, brannalarmen kan
utløses, og skjermer og hengelamper reagerer på skudd.

## Innhold (prioritert)
1. **Lysbrytere** per rom: `Device`-del med `State` på/av; rommets lys (Grafikks lysgrupper – bestill `LightGroups()` med
   rom-id og lys-indekser, og at lysene får en tag/id vi kan finne) slås av/på lokalt fra tilstanden. Skutte lysrør og
   hengelamper (`Health`) slukker sin lampe og flimrer først. Klikk-lyd.
2. **Heiser** (sør og øst, Plan 1–4): kabin som replikert bevegelig plattform (én `AGSElevator` per heis – replikert aktør,
   kabinens posisjon fra serverens klokke `StartTime + etasje`, så klienten regner den selv uten å sende posisjon hvert bilde),
   dører i hver etasje som `AGSDoor`-varianter som bare åpnes når kabinen står der, knapper (`IGSInteract`) inne og ute.
   Pling og motorlyd. Fare: man kan skytes når døra åpnes – realistisk. Bots: heis er ikke en del av navnettet (bestilles
   ikke i første omgang).
3. **Kiosk og kjøleskap (Ryddekroken), salatbaren:** dører som åpnes (`AGSDoor`-variant «skapdør», lys inni); innholdet er
   loot fra Spillbarhet (bestill: de legger `AGSPickup` inni etter våre plasseringer, eller kaller `GSMapState::StockAt`).
   Glasskjermen på salatbaren er `UGSGlassSet` (herdet).
4. **Skap** (garderobeskap/oppbevaringsskap i korridorene): dør som åpnes (lyd), loot inni (Spillbarhet), kan sparkes opp
   hvis låst. Skapene på tegningene («44 skap», «56 skap») – Grafikk gir plasser.
5. **Brannalarm:** knapp i trapperom/korridorer (`Tap` = knus glasset og trykk), alarmlyd og blinkende lys i hele bygget i
   60 s; alle hører (dekker fottrinn – Kamps lydmiks må tåle det), `OnNoise` stor. Én gang per 3 min.
6. **Piano** (P1 loungen): `Tap` spiller en kort frase (tilfeldig av 6), lyden høres i atriet – avslører hvor du er.
7. **Bordtennis:** `Tap` serverer ballen (lokal fysikkball, pling-pong-lyd), mest for stemning.
8. **Skjermer og TV-er:** knuses ved treff (bilde slukker, sprekk-modell, gnister), info-skjermene viser NTNU-tekst.
9. **Hengelamper** over atriet svinger når de treffes/ved trykkbølge (lokal fjær-animasjon fra `HitDir`) og kan skytes ned
   (faller, knuses mot gulvet – stor lyd).

## Bestillinger som trengs da
- Grafikk: lysgrupper per rom, plasser for brytere/skap/alarmknapper/piano/bordtennis/heiser (med id), modeller av
  kiosk/kjøleskap/piano/bordtennisbord som deler (dører og lokk som egne biter).
- Spillbarhet: loot i skap, kiosk og kjøleskap; hint for nye bruk-ting (eller vi tegner dem selv som i runde 1).
- Kamp: lyder (bryterklikk, heis-pling og motor, alarm, piano-fraser, bordtennis, skjerm som knuses), og at mørke rom
  faktisk er mørke (eksponering) – Grafikk/Kamp avgjør.

## Risiko
Lys som slås av og på med Lumen kan gi treg oppdatering av indirekte lys (et sekund «etterglød») – mål i Helgasetr.
Heisen er den eneste replikerte bevegelige plattformen: figurer som står i den må følge med på klientene (basering i
`CharacterMovement`), prøv med to spillere i PIE.

## IMPROVEMENTS
Kan bli to runder (3a: brytere, skap, kiosk, alarm, skjermer, lamper; 3b: heiser, piano, bordtennis). Nummerserien deles ut
av dirigenten.
