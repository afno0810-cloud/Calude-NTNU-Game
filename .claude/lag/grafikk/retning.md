# Retning for laget Grafikk og kart

Eies av lagets planlegger. Andreas' ønsker i `.claude/loop/andreas.md` er rammen
(superrealistisk, veldig groteskt, svært høye standarder, minst én gammel ting forbedret og én ny per runde).
Hovedagentens prioriteringer står i `.claude/lag/prioriteringer.md`.

## Visjon
Et lite, gjennomarbeidet stykke Gløshaugen en novemberkveld: kjernen rundt Hovedbygningen, laget for
Capture the Flag. Alt spilleren kan se og gå på i dette utsnittet skal tåle å bli sett på nært: fasader,
innganger, rom, møbler, skilt, bakke, trær og lys. Heller 330 × 265 meter som ser ekte ut enn 800 × 670
meter med plen og esker. Det som ligger utenfor er kulisse: det skal se riktig ut på avstand, og koster
ellers ingenting.

### Kartet (bestemt i runde 1)
Koordinater i meter, X = øst, Y = sør, origo midt i Hovedbygningen (som i `GSCampus.h`).

- **Spillområdet:** X −165 … 165, Y −125 … 140 (`GSCampus::PlayArea()`).
- **Base vest: Gamle fysikk** (midt −125, 10). **Base øst: Gamle kjemi** (midt 125, 10). To røde
  teglbygg som står mot hverandre med Hovedbygningen imellom. Ca. 250 m mellom flaggene (rundt 45 s løp).
- **Midten: Hovedbygningen** med Lysgården (mytisk kasse, scenen, galleriene, tårnet).
- **Tre ruter mellom basene:**
  1. *Nordruta (ute, åpen):* stien langs fasaden over forplassen. Lange siktlinjer, trær og biler som dekning.
  2. *Midtruta (under tak):* inn endedøra på Hovedbygningen, korridoren, Lysgården, ut den andre endedøra.
  3. *Sørruta (ute, trang):* passasjene, bakgården mellom fløyene rundt tårnet, promenaden i sør, Kjelhuset.
  Fløyene (Hovedbygningen vest/øst) og tårnet er flanke- og utkikksposisjoner.
- **Kulisser:** Sentralbygg 1 og 2 (høyblokkene i sør), Bergbygget, Elektrobygget. Ingen loot, ingen møbler.
- **Tatt ut:** IT-bygget, Stripa, Kjemiblokkene, Varmeteknisk, Realfagbygget, Idrettsbygget
  (modellfilene ligger igjen i `SourceArt/Models`, de bare brukes ikke).
- **Kjent skjevhet:** Gamle kjemi er 10 m lengre enn Gamle fysikk, og Kjelhuset finnes bare på vestsiden.
  Rettes i baserunden (backlog 2), eller ved at lagene bytter side mellom kampene (Spillbarhet).

### Faste regler for laget
- Verden holder «spillskala» på lys: en solbelyst vegg er ca. 10 lux, og effekter (røyk, munningsild,
  blod) er laget for det. Natta gjøres mørk ved å senke nattlysene og låse eksponeringen, ikke ved å
  gjøre sola fysisk riktig (100 000 lux). Å gå over til fysiske enheter krever at lag Kamp er med.
- Alt bygges fortsatt i kode (`GSCampusBuilder`) og Blender-skript. Blender 5.3 finnes på PC-en:
  `"C:\Program Files\Blender Foundation\Blender 5.3\blender.exe" --background --python <skript> -- <argumenter>`.
  Nye og endrede modeller skal alltid rendres til et bilde (`contact_sheet.py` / `render_building.py`)
  som kontrolløren ser på, siden editoren ikke er åpen.
- Poly Haven-ressursene Andreas har hentet (`Content/PolyHaven`, `SourceArt/PolyHaven`: 25 materialer, 28
  modeller) brukes der de er bedre enn våre egne. Vi endrer dem ikke.

## Neste ideer (backlog, viktigst øverst)
1. **Kanten av kartet.** Når Spillbarhet har flyttet spawn og loot: steng spillområdet på en måte som
   finnes i virkeligheten. Byggegjerder (campus er en byggeplass i disse årene) med presenning og skilt,
   hekker, parkerte anleggsmaskiner, låste porter. Ingen usynlige vegger uten noe synlig foran.
   Gatelys, benker og småting bare innenfor og like utenfor kanten.
2. **Basene.** Lette å kjenne igjen og forsvare: lagfarget banner/fane på fasaden og over dørene
   (ny Blender-modell), ett tydelig flaggrom i første etasje, færre åpne dører (noen stengt med
   sperringer), dekning inne. Gjør de to byggene like store (regenerer `SM_Bygg_Gamle_kjemi` 46 m bred
   eller begge 50 m) og gi østsiden et motstykke til Kjelhuset.
3. **Materialmerking for kuler (bestilling fra Kamp):** fysisk materiale på hver materialinstans
   (`gs_materials.py`: glass, tre, puss, tegl, betong, metall, jord) og ekte tykkelse på dører og tynne
   vegger. Glassrutene merkes allerede i runde 1 (`Mat_Glass`).
4. **Novemberkveld: lyset** (ferdig utredet i runde 1, se «Utredet» under).
5. **Hovedbygningens fasade og inngang** (`building_hovedbygningen.py`): ordentlig trapp og dør,
   vindusdetaljer, takrenner og nedløp, lykter som lyser, skilt. Render og sammenlign med foto.
6. **Lysgården og midtruta inne:** gulv, søyler, rekkverk, dører i korridoren, skilt, oppslagstavler,
   lys i taket som faktisk lyser (ikke bare glødende plater).
7. **Knusbare vinduer og klatreruter** (Andreas liker glass som knuser og klatring): vinduer i
   første etasje på basene og Hovedbygningen som innganger, stillas/containere/nedløp som klatrerute opp
   til andre etasje og tak, som alternative veier til flagget. Samordnes med lag Kamp.
8. **Bakgården og sørruta:** brostein med renner og kummer, sykkelstativ, varelevering ved Kjelhuset,
   containere, lasterampe. Dekning som ser ut som den hører hjemme.
9. **Forplassen og nordruta:** plenkanter, lave murer, trapper, flaggstenger, lyktestolper på rekke.
10. **Trær i november:** nakne lønner og bjørker (greinverk uten blader, noen få brune), løv på bakken,
    gran som før. `nature.py` + render.
11. **Bakke som ikke er flat:** svak helling ned mot Høgskoleringen (krever at spawn/loot følger bakken:
    bestilling), rennestein på bilveien, sluk der vannet faktisk ville rent.
12. **Vær:** regn som ekte dråper og striper (ikke 900 terninger), sølepytter der bakken er lav, snø som
    legger seg på tak og biler, skydekke som følger været.
13. **Innvendig i basene:** ordentlige rom med vegger og dører (i dag er hver etasje én åpen hall med
    møbelgrupper), trapper i stedet for ramper.
14. **Kulissene:** enklere modeller uten innside (billigere), lys i noen vinduer om kvelden.
15. **Flater for blod og kulehull** når lag Kamp bestiller.
16. **Fra Andreas' godkjente lister (se `prioriteringer.md` og `.claude/lag/onskeliste.md`), tas inn der de
    passer i rundene over:** vær som påvirker spillet (replikerte verdier for regn, tåke og vind som Kamp
    kan lese); bygg man kan bruke (lysbrytere, heis, brannalarm); dører og vegger som kan sprenges og
    åpner nye ruter; flyttbare pulter og stoler som barrikader; mer liv inne (rot, papirer, oppslag,
    NTNU-skilt); flimrende lysrør og sollys gjennom vinduer; trær og gress i vind; ytelse på RTX 3050
    (mål og logg før/etter), raskere oppstart.

### Åpne funn fra kontrolløren (`funn.md`) som ikke er tatt i runde 1
F3 (snø og regn inne – `gs_materials.py`, runde 2), F5 (dør foran tverrvegg i Hovedbygningen), F8 (sola
flyttes hver frame – ytelse), F9 (lyktestolper forsvinner på 90 m), F10 (gatelys gjennom vegger – lysrunden),
F11–F16, og realismehullene R1–R13 (R1/R2 innvendige rom og trapper = backlog 13, R3 like fasader, R4 flatt
terreng = backlog 11, R5 tårnet bak Hovedbygningen finnes ikke i virkeligheten – spør hovedagenten før det
fjernes, R7–R9 = lys og vær, R10 gul midtlinje og hydranter, R11 roboter som pynt – Andreas' eget innhold,
spør før de fjernes, R12 glass uten kollisjon = backlog 7).

### Utredet: novemberkveld-lyset (til backlog 4)
Funn fra gjennomgangen av `GSEnvironment.cpp` og gatelysene, så neste plan kan skrives raskt:
- Solbanen er vårjevndøgn (48° midt på dagen), og sola står i vest klokka 12. Riktig for Trondheim
  (63,4° N) sist i november: `sin(h) = −0,3132 + 0,4191·cos(H)`, H = (time − 12)·15°. Høyeste sol 6°,
  opp ca. 09.15, ned ca. 14.45. Retning: regn ut en vektor og bruk `Dir.Rotation()`; midt på dagen skal
  lyset gå mot nord (−Y).
- Eksponeringen er ikke låst (`ExtendDefaultLuminanceRange` er på), så natta blir løftet til grå dag.
  Lås med `AutoExposureMinBrightness ≈ −1`, `AutoExposureMaxBrightness ≈ 4,5` (EV100) og litt minus-bias om natta.
- `WhiteTemp` brukes trolig motsatt av hensikten: i Unreal gir lavere verdi kaldere bilde. I dag blir da
  natta gul (8500) og solnedgangen blå (5200). Sjekk mot Epics dokumentasjon før det snus.
- Månen kaster ikke skygge og lyser derfor inn gjennom tak. Gi den skygge, og egen bane (høy vintermåne).
- Gatelysene er punktlys uten skygge (6000 lumen, 55 m mellom): de lyser gjennom vegger og er like
  sterke som sola. Forslag: spotlys rett ned (ca. 70 candela, kjegle 75°), skygger, 36 m mellomrom,
  tegnes ikke lenger unna enn 90 m, lyktehodet gløder når lyset er på (`MI_GS_Bulb`).
- Med lav sol må `SunLux` for klarvær opp til ca. 40 for at solvegger skal få ca. 10 lux
  (atmosfæren tar ca. 70 % ved 6°). `DayFactor` og «golden hour»-tersklene må regnes om for 6° sol.
- Ingen lys i vinduene om kvelden, ingen stjerner. Krever nytt materiale i `gs_materials.py`.
- Bestilling som hører til: Spillbarhet lar kampen starte mellom kl. 13.30 og 17.30 (skumring til kveld).

## Endringer i retningen
- 2026-11-26 (runde 1, kl. 17.20): Andreas har bestemt Capture the Flag som eneste modus.
  Utsnittet er derfor valgt som en CTF-bane: Gamle fysikk og Gamle kjemi som baser, Hovedbygningen i
  midten, tre ruter. Baserunden er flyttet opp til nr. 2 i backlogen.
- 2026-11-26 (runde 1): Andreas vil ha et mindre og mye mer detaljert kart. Hovedsporet er nå å stramme
  inn til kjernen rundt Hovedbygningen og gjøre den ferdig område for område. Lysrunden jeg hadde
  utredet (novemberkveld) er flyttet ned i backlogen; funnene er tatt vare på over.
- 2026-11-26: laget opprettet.
