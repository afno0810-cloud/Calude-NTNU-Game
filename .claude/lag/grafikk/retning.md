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
- **Midten: Hovedbygningen** med Lysgården (mytisk kasse på sokkel, trappa, galleriene). Klokketårnet bak
  fjernes i runde 2 (finnes ikke i virkeligheten); der blir det en møblert bakgård med bakdør inn til Lysgården.
- **Tre ruter mellom basene:**
  1. *Nordruta (ute, åpen):* stien langs fasaden over forplassen. Lange siktlinjer, trær og biler som dekning.
  2. *Midtruta (under tak):* inn endedøra på Hovedbygningen, korridoren, Lysgården, ut den andre endedøra.
  3. *Sørruta (ute, trang):* passasjene, bakgården mellom fløyene, promenaden i sør, Kjelhuset.
  Fløyene (Hovedbygningen vest/øst) er flanke- og utkikksposisjoner.
- **Kulisser:** Sentralbygg 1 og 2 (høyblokkene i sør), Bergbygget, Elektrobygget. Ingen loot, ingen møbler.
- **Tatt ut:** IT-bygget, Stripa, Kjemiblokkene, Varmeteknisk, Realfagbygget, Idrettsbygget
  (modellfilene ligger igjen i `SourceArt/Models`, de bare brukes ikke).
- **Symmetri (fra runde 2):** begge basene er 46 × 22 m, og Gamle kjemi er snudd 180°, så banen er lik fra
  begge sider (snudd om punktet (0, 10)). Dekning i rutene legges alltid likt på begge sider.
  Gjenstående skjevhet: Kjelhuset finnes bare på vestsiden.
- **Spawn (fra runde 2):** bak egen base, i et riggområde mellom bygget og gjerdet.
- **Kanten (fra runde 2):** byggegjerder 0,5 m innenfor `PlayArea()`, sperringer der veier krysser, og en
  usynlig vegg nøyaktig på kanten bak gjerdet.
- **Regel for ruter:** ingen sti skal ha mer enn ca. 80 m fri sikt. Brytes med ting som finnes på en
  campus under ombygging (gravefelt med gjerde, parkerte biler, containere), ikke med tilfeldige kasser.

### Faste regler for laget
- Verden holder «spillskala» på lys: en solbelyst vegg er ca. 10 lux, og effekter (røyk, munningsild,
  blod) er laget for det. Natta gjøres mørk ved å senke nattlysene og låse eksponeringen, ikke ved å
  gjøre sola fysisk riktig (100 000 lux). Å gå over til fysiske enheter krever at lag Kamp er med.
- Alt bygges fortsatt i kode (`GSCampusBuilder`) og Blender-skript. Blender 5.3 finnes på PC-en:
  `"C:\Program Files\Blender Foundation\Blender 5.3\blender.exe" --background --python <skript> -- <argumenter>`.
  Nye og endrede modeller skal alltid rendres til et bilde (`contact_sheet.py` / `render_building.py`)
  som kontrolløren ser på, siden editoren ikke er åpen.
- Alt som står inne får materialnavn som slutter på `_In` (Blender-skriptene) eller står i
  `INDOOR_MODELS` (`gs_materials.py`), så det ikke får snø og regn (fra runde 2).
- Ytelse går foran pynt: maskinen er en RTX 3050 6 GB Laptop. Nye ting som settes ut i mengder skal være
  instanser med få trekanter, uten skygge når de er lave, og uten vind/WPO på avstand.
- Poly Haven-ressursene Andreas har hentet (`Content/PolyHaven`, `SourceArt/PolyHaven`: 25 materialer, 28
  modeller) brukes der de er bedre enn våre egne. Vi endrer dem ikke.

## Neste ideer (backlog, viktigst øverst)
Gjort: runde 1 (CTF-utsnitt, baser, åpne dører og ramper, plasser, allé).
I køen: `ko/plan-02.md` (ytelse P1/P2/P8 og litt P3, kanten, dekning og spawn K4, tårn og roboter bort,
Lysgården åpnet, like baser K5, K1–K3, K6, F3, F9) og `ko/plan-03.md` (flaggrommene, faner, K7).

1. **Ytelse, del 2** (etter at Andreas har målt plan-02 med `stat unit`/`stat gpu`): Nanite for trær,
   busker og hekker (P3), ny vind som slås av på avstand (F23), regn/snø ut av strålesporing og
   distansefelt (P6), LOD for biler og glass (P10), Nanite-kopi av kuben (P2). P4 (Lumen «hit lighting»,
   to linjer i `DefaultEngine.ini`) er Andreas' fil: foreslås via hovedagenten. Automatisk grafikknivå og
   DLSS/FSR (ønske 46, 49).
2. **Novemberkveld: lyset** (ferdig utredet, se «Utredet» under): riktig solbane for Trondheim i
   november, låst eksponering, gatelys som spotlys med skygge (F10) og lampehoder som gløder (F21), lys i
   vinduer. Må ses i editoren av Andreas.
3. **Basene, del 3:** rom og vegger i 2.–4. etasje, trapper i stedet for ramper (R1/R2), dørblad med ekte
   tykkelse, et motstykke til Kjelhuset på østsiden, flaggsokkel og flaggmodell (bestilt av Spillbarhet).
4. **Materialmerking for kuler (bestilling fra Kamp):** fysisk materiale på hver materialinstans
   (`gs_materials.py`). `_In`-instansene arver fra forelderen, så det holder å merke foreldrene.
5. **Teksturer og modeller som ser feil ut på nært hold:** skjøt hver 2. meter på mur og granitt (F18),
   bark-UV (F19), biler uten hjulbuer (F20), sykler i stativet (F22), JPEG som ødelegger ARM/normal (F24),
   treverk og moiré (F25), bladatlas med svart kant (F26), bakgrunnsbyen (F17 hvis ikke tatt i plan-02).
6. **Hovedbygningens fasade og inngang:** ordentlig trapp og dør, vindusdetaljer, takrenner og nedløp,
   lykter som lyser, skilt, irret kobber på tårnhjelmene foran. Render og sammenlign med foto.
7. **Lysgården og midtruta inne:** gulv, søyler, rekkverk, dører i korridoren (F5), skilt, oppslagstavler,
   NTNU-plakater (ønske 30), lys i taket som faktisk lyser, buene uten søyler (F16).
8. **Knusbare vinduer og klatreruter** (Andreas liker glass som knuser og klatring): vinduer i første
   etasje som innganger, stillas/containere/nedløp som klatrerute til andre etasje og tak. Samordnes med Kamp.
9. **Trær i november:** nakne lønner og bjørker, løv på bakken, snø på greiner og bar (resten av F3).
10. **Rutene ute, del 2:** brostein med renner og kummer, varelevering ved Kjelhuset, lave murer, trapper,
    flaggstenger, lyktestolper på rekke, gul midtlinje og ingen hydranter (R10). Kanten: anleggsmaskin og
    brakkerigg bak gjerdet, låst port, hekk og parkskråning i nord.
11. **Bakke som ikke er flat** (krever at spawn/loot følger bakken: bestilling), rennestein, sluk.
12. **Vær:** regn som ekte dråper, ikke regn gjennom tak (F13), sølepytter, snø på tak og biler, skydekke
    som følger været (R8).
13. **Kulissene:** enklere modeller uten innside, lys i noen vinduer om kvelden.
14. **Flater for blod og kulehull** når lag Kamp bestiller.
15. **Fra Andreas' godkjente lister (`prioriteringer.md`, `onskeliste.md`), tas inn der de passer:** vær som
    påvirker spillet; lysbrytere, heis, brannalarm; dører og vegger som kan sprenges; flyttbare pulter og
    stoler; mer liv inne; flimrende lysrør og sollys gjennom vinduer; trær og gress i vind; kjellerganger,
    tak som utkikk, bibliotek, auditorier, kantine (ønske 21–25); raskere oppstart.

### Åpne funn (`funn.md`, `kontroll/runde-01.md`) som ikke er med i plan-02/03
Feiljakt 1: F5, F10, F11, F12, F13, F14 (delvis tatt), F15, F16, R1/R2 (øvre etasjer, trapper), R3, R4, R6,
R7–R9, R10, R12, R13. Feiljakt 2: P3 (Nanite), P6, P10, F17 (valgfri i plan-02), F18–F26. Hos andre lag
(bestilt av hovedagenten): P1 punkt 3–4, P5, P7, P9 for `GSTerminal`. Andreas' fil: P4.
Tatt i plan-02: F3 (unntatt løv), F8/P1, F9, P2, P8, P9 (gatelys), R5, R11, K1–K6. Plan-03: K7, R1 (1. etasje).

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
- 2026-11-26 (runde 2): Hovedagenten setter ytelse først (feiljakt 2: RTX 3050 6 GB, skygger som tegnes
  på nytt hver frame), så dekning og spawn for CTF (K4) sammen med kanten, så tårn og roboter (Andreas'
  avgjørelse) og småfeilene. Detaljløftet er delt ut i plan-03 (flaggrommene). Valgt foran lysrunden
  fordi flaggrom kan sjekkes med stråler og bilder uten editor; lyset må ses i spillet. Basene gjøres
  like store og snus om midten. Ny fast regel: ingen sti med mer enn ca. 80 m fri sikt.
- 2026-11-26 (runde 1, kl. 17.20): Andreas har bestemt Capture the Flag som eneste modus.
  Utsnittet er derfor valgt som en CTF-bane: Gamle fysikk og Gamle kjemi som baser, Hovedbygningen i
  midten, tre ruter. Baserunden er flyttet opp til nr. 2 i backlogen.
- 2026-11-26 (runde 1): Andreas vil ha et mindre og mye mer detaljert kart. Hovedsporet er nå å stramme
  inn til kjernen rundt Hovedbygningen og gjøre den ferdig område for område. Lysrunden jeg hadde
  utredet (novemberkveld) er flyttet ned i backlogen; funnene er tatt vare på over.
- 2026-11-26: laget opprettet.
