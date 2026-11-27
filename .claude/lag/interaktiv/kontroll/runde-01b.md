**MÅ FIKSES** (F1 og F2, begge i egne filer og små; resten er godkjent)

Kontrollert: commit `1adda33` (`GSBreakable.h/.cpp`, `GSMapState.h/.cpp`, `GSDoor.h/.cpp`, `GSInteract.h/.cpp`,
`interactive_doors.py`, logg/funn/retning, kontaktarket). IMPROVEMENTS 718–733 og tekstrettingene 653/655/658/660 er
sjekket i `IMPROVEMENTS.md` slik den står i `1adda33` (kom med Grafikks `fbf7de7`). Lest mot `plan-01b.md`,
`runde-01a.md`, Kamps `plan-13.md`, Kamps `UGSBallistics::Step`/`FireShown`/`FireReal` og `AGSCharacter::DoMeleeHit`
i commiten, `AGSHelgasetrBuilder::BuildGlassPanels`, `GSHelgasetr.gen.inl` (`PanelsM`) og motorens
`UInstancedStaticMeshComponent::UpdateInstanceTransform` (UE 5.8). Selvtestloggen `Saved/Logs/Interaktiv1b_Selvtest.log`:
343 glassfelt, 2 byggerkomponenter skjult, 53 dører, **36 OK, 0 FEIL**. Ikke bygget på nytt (byggeren har bygget).

## Fast regel (forbedret + nytt)
- **Forbedret:** glassdørene fra 1a kan knuses og gås gjennom (eller gripes inn i), `Interaktiv.Knus`/`Smell` virker på
  glass, dørene har fått Blender-modeller i stedet for kuber, dørlyder bruker Kamps navn per dørtype, og alle fire funn fra
  1a er rettet (slag stoppes av vegger, `BotUse` gir `false` når ingenting skjedde, dør 31 hoppes over, IMPROVEMENTS-teksten).
- **Nytt:** knusbart glass (343 ruter i Helgasetr + 9 i testrommet), herdet/laminert skademodell, biter og skår,
  varige kulehull etter underlag, `BulletSegment`/`ApplyMelee`/`AddBulletMark`. Regelen er oppfylt.

## «Ferdig når», punkt for punkt
- Bygger, ingen andre lags filer i commiten: ja (bare egne filer + egne lagfiler).
- Tre glasstyper med `Knus`/`Smell`, lyd og skår: ja i selvtesten (herdet smuldrer, laminert sprekker → gir etter ved 100,
  dørrute knust → passerbar). Lyd kan ikke høres hodeløst.
- Online likt, sen klient uten gamle effekter, tilbakestilling: riktig i koden (se under), **ikke prøvd** med vert + klient
  (planens steg 9.4). Tilbakestilling prøvd hodeløst.
- Kulehull 400 i alt, 30/s: ja (600 lagt → høyst 400; 30 av 60 i samme øyeblikk).
- Fast glass urørt, logg med antall: ja. Skjulingen tar bare `GS_Int_Glass` eller `Mat_Glass` + ignorerer `Visibility`;
  4a-glasset ligger i `AGSHelgasetrBuilder` (underklasse av `AGSCampusBuilder`, så iteratoren finner det), og byggeren
  bygger synkront før registeret, så rekkefølgen holder. 4b-modellen har bare fast glass (`SM_Glass_Helgasetr_*`).
- `logg.md`, `retning.md`, `IMPROVEMENTS.md`: ja.

## Særskilte sjekker

### Signaturene mot Kamps plan-13
Alle passer nøyaktig: `float BulletSegment(UWorld*, From, To, Vel, float Damage, bool bReal)`,
`void ApplyBulletHit(UWorld*, const FHitResult&, float)`, `void AddBulletMark(UWorld*, const FHitResult&)`,
`void ApplyBlast(UWorld*, Origin, RadiusCm, Strength)`, `float ApplyMelee(AActor*, Start, End, float)` (returnerer
`BIG_NUMBER` uten glass), `bool GSInteract::HasFocus(const AGSCharacter*)`. Ingen av dem er tomme lenger (plan-13 krever
det før steg 6). Viste kuler kjøres også hos verten (`FireShown` bare ikke på dedikert server), så verten får kulehull.

### Kan glassfaktoren brukes til å skyte gjennom vegger? Nei.
- Faktoren er `0,92^antall ruter`, alltid ≤ 1: den kan bare bremse og svekke, aldri gi gjennomslag.
- Kamps spor er `LineTraceSingleByChannel(…, ECC_Visibility)` og `End = bHit ? Hit.ImpactPoint : NewPos`. Glasset
  ignorerer `Visibility`, så kula stopper ved første vegg, og `BulletSegment(B.Pos, End)` ser bare glass foran veggen.
  Glass bak en vegg blir ikke truffet eller knust av en ekte kule.
- Knuste ruter har skala 0 og ingen kropp, dørruter sjekkes med `IsPaneBroken`, og samme rute telles én gang per segment
  (`Seen`). Klienten kan bremse en vist kule med 8 % der serveren nettopp knuste ruta (godtatt i plan-13).
- **Men slaget gjør det:** se F2.

### Replikering (vert, klient, sen klient) og båndbredde
- Vert: `SetPart` → `ApplyLocal(true)` → `ApplyGlassLocal`. Klient: `PostReplicatedAdd/Change`, fersk bare ved
  `0 ≤ dt < 2 s` og etter første pakke. Sen klient: `ApplyAllLocal` (aldri fersk) gir skjulte ruter, skår på gulvet og 3
  seedede sprekkestjerner, ingen lyd og ingen biter. Testrommets glass venter i `PendingKeys` til `OnRep_TestRoom`.
  Tilbakestilling virker i begge rekkefølger (fjernet element → `ResetGlassLocal`; `OnRep_ResetCount` → alt + biter
  ødelagt). Dørruta følger samme vei (`PaneChange` i `ApplyState`, `ResetLocal` setter den tilbake). Ingen feil funnet.
- **Båndbredde (regnet):** et nytt element i lista er ca. 25–30 byte (nøkkel 32 bit, 5 × 8 bit, `HitDir` 48 bit,
  `ChangedAt` 32 bit, håndtak og `ReplicationID`), en endring litt mindre. Ingenting sendes når ingenting skjer (bare
  skitne elementer). Regnet mot `PanelsM`: en splintgranat (6 m → glassradius 12 m) kan i verste fall ta **82 ruter** på
  én gang (C4 7 m: 102). Det er ca. **2,4 KB én gang per tilkobling** (≈ 45 KB fra verten til 19 klienter, fordelt over
  noen bilder). Alt knust: ca. 410 elementer ≈ **12 KB** til en sen klient, én gang. Laminert glass under LMG-ild: én
  endring per kule, samlet til 20 Hz. Godt innenfor; ingen grunn til å endre formatet.

### Ytelse
- Ingen tick: `AGSDebris` (tidtaker 0,5 s fra 3,5 s), `UGSInstancePool`, `AGSMapState` og `UGSGlassSet` tikker ikke.
  Dører tikker som i 1a bare når de går.
- Alle ISM-oppdateringer bruker `bMarkRenderStateDirty = false`; motoren (5.8) sender likevel endringen via
  `PrimitiveInstanceDataManager.TransformChanged`, og skala 0 fjerner kroppen. Riktig.
- Tak: 400 merker (2–8 instanser hver, ringbuffer), 3000 skår (én instans per oppføring), 12 per dør/rute. Holdt.
- `BulletSegment`: ett objektspor per kulesteg (60 Hz per kule) + en `TArray`/`TSet` på heapen per kall. Billig nok.
- **150 biter i lufta: taket holdes, men på feil måte** – se F1.

## Funn

**F1 (må fikses) – en granat lager hundrevis av fysikkbiter på én gang, og de som går over taket fryses i lufta for
resten av kampen.** `AGSMapState::GlassBreakFX` (`GSBreakable.cpp` l. 739–747) kaller `MakeRoomForBits(14)` og lager så
10–14 (herdet) / 4–6 (laminert) nye `UStaticMeshComponent` med fysikk for **hver** rute. `MakeRoomForBits` →
`AGSDebris::SettleSome` → `SettleBit` (l. 442–462) gjør de eldste bitene om til skår **der de er akkurat nå**. Når mange
ruter knuses i samme bilde, er «de eldste» biter som ble laget mikrosekunder før, fortsatt i rutas plan. Regnet mot
`PanelsM`: en splintgranat kan be om ca. **650 biter** (82 ruter), `Interaktiv.Smell` (3 m) ca. 310. Da:
1. ca. 650 komponenter registreres med fysikk i ett bilde og ca. 500 slettes straks (hakk hos alle maskiner, og
   IMPROVEMENTS 730 lover det motsatte);
2. ca. 500 «skår» blir hengende i lufta i rutenes plan som glassbiter som svever, til de skyves ut av 3000-taket.
**Rett:** håndhev taket når bitene lages: `Init` får et tak (`Room = 150 − NumFlyingBits()`, og gjerne høyst ca. 40 nye
per bilde), en rute uten plass får bare skårene på gulvet (som allerede legges). Biter som må tas før tiden, slettes uten
skår (eller får et spor ned til gulvet først). Legg til en selvtest: `Interaktiv.Smell 6` midt i glassrommene i Helgasetr →
høyst 150 biter, og ingen skår mer enn ca. 30 cm over gulvsporet.

**F2 (må fikses før Kamps K3 går live) – kniv/kolbe knuser glass gjennom vegger.** `GSMapState::ApplyMelee`
(`GSMapState.cpp` l. 950–958) sveiper først bare mot `ECC_Destructible`, uten å se etter vegger. Et slag mot en vegg
(rekkevidde 160 cm, kule 22 cm) knuser en rute som står bak en 10–25 cm vegg, og returnerer avstanden til ruta, så Kamp
behandler slaget som stoppet i glass. Samme feil som 1a-funn 2 (rettet for dører, ikke for glass). **Rett:** kjør
`WorldStatic|WorldDynamic`-sveipet først (det finnes allerede, l. 960–964) og bruk glasstreffet bare hvis det ligger
nærmere enn første treff som ikke er en `AGSDoor` (dørruta er `Destructible`, ikke `WorldDynamic`, så den forstyrres ikke).

**F3 (liten) – hagl gir én sprekkelyd per hagl hos verten.** `ApplyGlassLocal` (`GSBreakable.cpp` l. 685–688) spiller
`Glass_Crack` for hver økning i `Health`. På serveren kalles `SetPart` per hagl (8–12 i samme bilde), så verten hører
8–12 like lyder oppå hverandre (klientene får dem samlet til én). Lærdom runde 4: én gang per øyeblikk per rute (f.eks.
0,1 s). Ta det med i rettingen.

**F4 (liten, til neste runde) – merker på flyttbare ting lager egne ISM-er per komponent.** `AddMark` bruker
`Mobility == Movable` som foreldre (l. 1001). Riktig for dørblad, men komponenter som har motorens standard (`Movable`),
som PC-er (`GSTerminal`) og andre løse ting, får hver sin ISM per merkeart (opptil 4) som aldri fjernes. Ikke farlig nå
(bygget og testrommet er `Static`), men blir flere tegnekall i runde 2 med løse ting. Bruk forelder bare for `AGSDoor`
(eller ting som faktisk kan flytte seg), ellers verdens-ISM-ene.

**Ikke prøvd:** to spillere + sen klient (planens steg 9.4) og `stat unit` (9.8). Koden er gjennomgått for alle tre, men
lærdommen fra 1a gjelder fortsatt: en hodeløs vert + klient som logger glasstilstand hos begge ville bevist det.

## Forslag til neste runde
1. **Hodeløs vert + klient + sen klient** (`-server` / `127.0.0.1 -game -nullrhi`): knus 5 ruter med `Interaktiv.Knus`,
   koble til en tredje etterpå, og logg `NumGlassBroken`, skår og biter hos alle tre. Byggerens funn om at statiske spor
   bommer hodeløst bør undersøkes samtidig (det kan gjelde ekte spill med `-nullrhi`-server).
2. **Lyd- og effektbudsjett for granater:** én knuselyd per klynge av ruter (f.eks. høyst 3 knus + 3 skårlyder per
   smell, plassert ved de nærmeste), i stedet for 80 lyder og 80 tidtakere.
3. **Granaten og veggene:** 12 m uten siktsjekk betyr at glass bak flere vegger knuses. Mer realistisk: full radius med
   fri linje, halv radius (eller bare sprekk) uten. Og skygge av på dørmodeller, håndtak og kodelås (1a-forslag 3).

## Lærdom
- Tak på kosmetiske ting skal håndheves når de lages (lag færre), ikke ved å avbryte de eldste midt i lufta; regn taket
  mot verste tilfelle (én granat midt i flest mulig ruter).
- Et spor mot én objekttype (glass, `Destructible`) ser ikke vegger: klipp det ved første vegg, ellers virker det
  gjennom vegger. (Gjentatt fra 1a, nå for glass.)
