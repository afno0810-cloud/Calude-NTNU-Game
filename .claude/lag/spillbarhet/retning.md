# Retning for laget Spillbarhet

Eies av lagets planlegger. Andreas' ønsker i `.claude/loop/andreas.md` er rammen
(superrealistisk, veldig groteskt, svært høye standarder, minst én gammel ting forbedret og én ny per runde).
Hovedagentens prioriteringer i `.claude/lag/prioriteringer.md` styrer rekkefølgen.

## Visjon
En kamp på Gløshaugen skal føles som en ekte, farlig kveld på campus, ikke som en arkadehall. Fra du starter
spillet til kampen er over skal alt henge sammen, forklare seg selv og være rettferdig:

1. **Du kommer inn uten hjelp.** En ordentlig hovedmeny, innstillinger som huskes, og spillet viser selv hvilke
   taster som gjør hva. Ingen skal trenge konsollen eller en README for å spille med venner.
2. **Ren start, lik for alle.** Hver kamp starter med full loot, tomme lommer, ingen feller fra oppvarmingen,
   og ingen spawner i siktet til en annen. Serveren bestemmer alt; ingen enkeltspiller kan ødelegge for resten.
3. **Det du finner er det som finnes i virkeligheten.** Ting ligger på gulv og bord i ekte størrelse. Våpen har
   riktig kaliber, kula gjør skade etter patronen, og en vest stopper det den vesten faktisk stopper.
4. **Kroppen er sårbar.** Du blør til du stopper blødningen, du blir tung av det du bærer, sulten og tørst.
   Medisin er noe du bruker tid på i dekning, ikke en knapp som fyller liv.
5. **Grunn til å spille en gang til.** Kampen har en tydelig bue, en nøktern oppsummering, og neste kamp er
   annerledes (vær, tid på døgnet, hvor lootet ligger).
6. **Tåler 20 spillere og juks.** Alt regnes på serveren, og nett og ytelse holder med fullt hus.

## Hva som finnes i dag (lest i koden 2026-11-26)
- `GSGameMode`: lobby → nedtelling → kamp (15 min / 25 drap) → resultat → lobby. Spawn på 16 faste punkter,
  lengst fra fiender. Ca. 430 lootplasser og 16 låste kasser legges ut tilfeldig ved oppstart.
- `GSGameState`: fase, klokke, vær, drapslogg. `GSPlayerState`: drap, død, klar.
- `GSItems` + `catalog.py`: ca. 170 gjenstander; loot trekkes gruppe → sjeldenhet → gjenstand.
- `GSPickup`: lootplass som fylles på nytt etter 75–150 s. Svever 60–95 cm over gulvet, snurrer, ring og lysstråle.
- `GSTerminal`: PC (kameraer, sporing) og 3D-printer (koster filament).
- Hos lag Kamp finnes en Esc-meny tegnet i `GSHUD` (vert, bli med siste adresse, grafikk, følsomhet) og en
  hjelpetekst som vises i 30 s. Spillet starter rett inn i kartet uten hovedmeny.
- Mangler helt: hovedmeny, tasteoversikt, LAN-søk, blødning, vekt, kaliber-riktig skade, beskyttelsesklasser.

## Neste ideer (backlog, viktigst øverst)

### Køen (i denne rekkefølgen)
1. **[plan-01, runde 1] Tasteoversikt, hint og innføring (0a)** i nye `GSHelp.*` + ren kampstart (funn 6, 7, 15 og
   Kamps bestilling om prosjektiler). Se `ko/plan-01.md`.
2. **[plan-02] Hovedmeny (0b)** i nye `GSMenu.*`, C++ uten .uasset:
   - Vises når spillet starter alene (NetMode Standalone, ikke koblet til noen): campus ligger bak som levende
     bakgrunn med et rolig kamera, spilleren er frosset (`SetIgnoreMoveInput/LookInput`, musepeker på).
   - **Start spill** (vert: `open Gloshaugen?listen`), **Bli med** (skriv inn IP i et felt – tall, punktum,
     slett, lim inn; siste adresse huskes), **Innstillinger** (grafikk, oppløsning, strålesporing, musfølsomhet,
     lydstyrke; lagres i `GGameUserSettingsIni`), **Kontroller** (`GSHelp::DrawControls`), **Avslutt**.
   - Tegnes med samme krok som `GSHelp` (ett kall fra `GSHUD`, reserve via `AHUD::OnHUDPostRender`), mus via
     `GetMousePosition` + `WasInputKeyJustPressed(LeftMouseButton)`. Nøktern: mørk flate, én skrift, ingen effekter.
   - Bestillinger til Kamp: Esc-menyen i `GSHUD` skal ikke åpnes oppå hovedmenyen; innstillingene i
     `GSPlayerController` (følsomhet, grafikk) leses fra samme lagrede verdier; «Til hovedmenyen» i Esc-menyen.
   - Åpent spørsmål til byggeren: lydstyrke uten .uasset (ingen SoundClass/SoundMix i prosjektet?) – undersøk
     `FAudioDevice::SetTransientPrimaryVolume` før det loves.
3. **[plan-03] Loot-vekter (bestilling fra Kamp) + loot som ligger på gulvet.**
   - `RandomLoot`: ammovekt per kaliber (9 mm vanligst, rakett/40 mm/.50 sjeldnest), sjeldenhet som ikke snus i
     små grupper, og låste kasser som gir våpen oftere (se `.claude/loop/kontroll/feilretting-01.md`).
   - `GSPickup` (funn 11 + arkade-preg): ting ligger stille på gulv/bord (server-linjespor ned ved utlegging;
     campus er bygget før `GameMode::BeginPlay`), våpen på siden, skala maks 2× (i dag 4×), ingen `Tick`,
     delte materialer per farge, `OnRep_State` gjør jobben én gang, nett-dvale (`DORM_DormantAll` +
     `FlushNetDormancy` i `Roll`/`OnTaken`/`Unlock`). Gi beskjed til Kamp: fokuspunktet flytter seg til 12 cm
     over gulvet.
   - Nye API-er som Kamp kan koble til: `AGSPickup::CanBeReachedFrom(Eye)` (funn 8, loot gjennom vegger) og
     `ArmorPoints` på pickup + parameter i `SpawnDropped` (funn 1, vester repareres ved bytte). Bestill bruken.
4. **[plan-04] Spawn (funn 12) tilpasset det nye, mindre kartet (prioritet 0).** Vent på Grafikks nye spillområde.
   - Kandidater rundt hvert spawnpunkt (bakke-spor + ledig kapsel), aldri der en levende fiende innen 120 m har
     fri sikt, aldri nærmere enn 25 m hvis det kan unngås, tak på avstandspoeng så man ikke alltid havner i
     samme hjørne. Spawn gjerne inne i bygg/bak dekning på et lite kart.
   - Loot-tetthet, antall kasser, påfyllingstid, kamplengde og drapsgrense skaleres etter areal og antall spillere.
   - Påfylling bare når ingen ser plassen (ting skal ikke dukke opp av løse lufta).
5. **[plan-05] Invitere venner, steg 1 (0c):** «Finn spill på nettverket» (LAN-beacon som lister åpne spill) og en
   kode/IP man kan kopiere. Koden legges opp mot OnlineSubsystem-sesjoner så Steam/EOS (steg 2) kan komme når
   Andreas har konto. Krever tillegg i `Gloshaugen.Build.cs` (felles fil, lite tillegg).

### Deretter
**Våpendata som følger virkeligheten (prioritet 2, `catalog.py`)**
- Kalibertabell: skade, munningsfart, kulevekt og gjennomslagsklasse per patron + liten løpsfaktor per våpen (R5).
  Hodeskudd uten hjelm dreper alltid; geværkaliber i brystet uten vest 1–2 skudd; pistol 2–4. Nye felt i
  `FGSWeaponStats` som Kamp bruker til ballistikk (samordnes via bestilling før Kamps ballistikk-runde).
- Riktige kalibre (R6): 7,62×25 (PPSh), 9×18 (Makarov), 7,92×57 (Kar98k), 6,5×55 (Krag), .30-06 (Garand), .338
  (L96), .357 og .50 AE hver for seg, med egne ammoesker.
- Beskyttelsesklasser (R7): vest/hjelm får klasse (myk IIIA, plate III, keramisk IV; hjelm = splint og pistol).
  Data hos oss, bruken i `TakeDamage` bestilles hos Kamp.
- Ekte skuddtakt og prosjektilfart (R13, R14), ildvelger-data, hvilke våpen som kan ha lyddemper (V12).
- Magasin-data (R8): +1 i kammer, ladetype (magasin, ramme, ett og ett skudd, belte).

**Realistisk overlevelse (prioritet 3)**
- Blødning som må stoppes (bandasje, trykkbandasje, tourniquet). Vi leverer gjenstander, tall og regler;
  `GSCharacter`-delen bestilles hos Kamp.
- Medisin på ekte: bandasje stopper blødning, men gir ikke liv; smertestillende demper skjelving; kirurgisett tar
  lang tid og må gjøres i ro.
- Vekt i kg på alt i katalogen; tung sekk = saktere, mer utholdenhet, mer støy.
- Roligere og mer troverdig sult/tørst. Kulde og væte i snø og regn.
- Boosts som er ren magi (se gjennom vegger, føniks): gjør svært sjeldne eller tone ned. Avklares med
  hovedagenten først (Andreas har selv ønsket boosts).

**Loot og økonomi**
- Loot som passer stedet: mat i kantiner, verktøy og filament i verksteder, medisin i førstehjelpsskap, våpen
  sjeldnere og samlet på få steder. Krever romtype fra `GSCampusBuilder` (bestilling til Grafikk).
- Våpen ligger med noen skudd i magasinet, ikke fulle. Ammo er knapp.
- Lysstråler og ringer erstattes av noe som finnes i verden (åpent skap, veske, kasse). Spør hovedagenten først:
  strålene står i README som en funksjon.
- Låste kasser som ekte våpenskap på faste, logiske steder. 3D-print tar ekte tid, bråker og kan stjeles.

**Kampflyt**
- Oppsummering etter kampen: drap, treffprosent, lengste skudd. Nøkternt, ikke medaljer.
- Spiller som kobler fra midt i kamp (tingene blir liggende; kampen avsluttes hvis én er igjen). Spiller som
  kommer inn midt i kamp.
- Kampformer: «ett liv», lag mot lag. Avstemning om neste kamp. Verten kan starte med en gang.
- Enkel vertsmeny: start nå, kast ut spiller, kamplengde.

**Feiljakt som gjenstår i egne filer**
- `GSTerminal` er ikke gjennomgått: sporing/kamera gjennom vegg eller i lobbyen, PC mens man er død,
  dobbelt-print, kameraer som alltid er relevante for alle (nettkost).

## Endringer i retningen
- 2026-11-26 (runde 1, senere samme dag): Andreas la inn 0a (knappehjelp), 0b (hovedmeny) og 0c (invitere
  venner) øverst, og kartet skal bli mindre. Plan-01 ble derfor byttet fra «ren kampstart + loot på gulvet + ny
  spawn» til knappehjelpen (den som trygt lar seg bygge og kompilere i én runde), med de små kampstart-fiksene
  som gammel-forbedring. Hovedmenyen er plan-02. Loot på gulvet er flyttet til plan-03 sammen med Kamps
  bestilling om loot-vekter, og spawn til plan-04 fordi den må passe det nye kartet uansett.
- 2026-11-26 (runde 1): visjon og backlog skrevet.
- 2026-11-26: laget opprettet.
