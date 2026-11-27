# Retning for laget Grafikk og kart

Eies av lagets planlegger. Andreas' ønsker i `.claude/loop/andreas.md` er rammen (superrealistisk, veldig groteskt,
svært høye standarder, minst én gammel ting forbedret og én ny per runde). Hovedagentens prioriteringer står i
`.claude/lag/prioriteringer.md`.

## Visjon (fra 27. nov. 2026)
**Helgasetr Shootout spilles inne i Helgasetr** (Elgeseter gate 10 / Vangslunds gate 2, NTNU, åpnet 2024). Det store,
lyse atriet med glasstak og hvitt fagverk, de gule trappene som krysser lufta, glassboksene som henger ut over kantina,
ringkorridorene med betongsøyler og glassrekkverk – bygget så nøyaktig etter tegningene og bildene at en student fra
Helgasetr kjenner seg igjen på første blikk. Rødt lag forsvarer flagget i sørenden av 4. etasje, blått lag i nordenden,
og alt imellom – korridorer, laber, trapper, atriet og kantina fire etasjer under – er slagmarken. Så mye som mulig skal
kunne brukes, åpnes, knuses og sprenges.

**Fasiten er `.claude/lag/grafikk/helgasetr.md`** (mål, etasjer, rom, baser, ruter, siktlinjer, interaktivitet,
ødeleggelse, lys, materialer, ytelse, lyd). Den oppdateres når noe avklares.

### Faste regler for laget
- Mål fra tegningene går foran «det ser riktig ut». Alt som er anslått merkes, og rettes når bedre kilde finnes.
- Én datakilde: `Tools/Blender/helgasetr_plan.py`. Blender-strukturen, `GSHelgasetr.gen.inl` (generert) og sjekkene leser
  den. Ingen tall skrives to steder.
- Den store bygningsmodellen er bare det som **aldri** endrer seg. Alt som kan åpnes, knuses, sprenges eller flyttes er egne
  instanser/aktører med ID (helgasetr.md kap. 6).
- Aldri ødeleggbart: bærende vegger, søyler, dekker, trapper og repos, trapperom, fasade- og takglass, baserommenes vegger
  mot korridoren.
- Innendørs siktlinjeregel: ingen rute med mer enn 42 m fri sikt; spawn skal aldri synes fra ruter, trapper og lobbyer.
- Himlinger og lamper har aldri kollisjon (GameMode sporer ned fra +3/+4 m for flagg og spawn).
- Alt som står inne har materialspor som slutter på `_In`.
- Ytelse: RTX 3050 Laptop 6 GB, 60 bilder/s i 1080p Middels; budsjettet står i helgasetr.md kap. 9. Mål før og etter.
- Blender 5.3: `"C:\Program Files\Blender Foundation\Blender 5.3\blender.exe" --background --python <skript> -- <arg>`.
  Nye og endrede modeller rendres alltid til bilder som kontrolløren ser på (`.claude/lag/grafikk/kontroll/rNN-*.png`).
- PDF-tegningene leses med `.claude/lag/grafikk/verktoy/renderpdf.ps1` (Windows' PDF-motor) og `pdftotext -layout`.
- Gamle campus-filer slettes ikke før Andreas har spilt Helgasetr og sagt ja (spør hovedagenten).

## Rundeplan for Helgasetr
Hver runde: minst én gammel ting bedre og én ny. Detaljnivået legges der det spilles mest (Plan 4, atriet, kantina).

| Runde | Innhold | Hvorfor da |
|---|---|---|
| **4a (`ko/plan-04.md`)** | **Spillbart klossbygg fra data**: datakilden, generert `GSHelgasetr.gen.inl`, `AGSHelgasetrBuilder` bygger hele bygget (vegger, dekker, trinn, rekkverk, bokser, tak) som klosser, baser/spawn i sluser/flagg på P4, dekning hver 8. m, glassfelt/lettvegger/dekning som egne ISM-er med id, `GSCampus` peker på Helgasetr, sporprøve i loggen, `check_helgasetr.py` (inkl. e2/e3/d2, kapsel, gripekrok). **Bare startpistol** til Spillbarhet leverer loot | Andreas kan spille samme dag, uten import |
| **4b (`ko/plan-04b.md`)** | **Blender-modellen**: 6 biter per plan, trapper, over-atriet (med gule løp P4→P6 som kulisse), sagtak og fagverk, fast glass, moduler, hengelampe; reserve per bit; DF-skala 2; FBX bare ved endring; `check_helgasetr_walk.py` (GameMode-spor, rekkverk langs hele kanten, kapsel); rendringer | kvalitet: ser ut som bildene |
| **5 – Interaktiv 1: dører, knusbart glass, sprengning, flyttbar dekning** | `AGSMapState` (replikert tilstand: ødelagte ID-er, `FFastArraySerializer`); glass som sprekker/knuses (3 varianter + skår), ødelagt modell + lokale biter og støv, tak 150 biter; sprengbare lettvegger; dørmodeller (tre, glass med kodelås, dobbel, brann) og ødelagte dører; fysikkmøbler (stoler, puffer, små bord, søppelkasser, ≤ 60 som simuleres); fysiske materialer per materialinstans (`PM_GS_*`); glass slutter å ignorere `Visibility`. Bestillinger: Kamp kobler `ApplyBlast`/`ApplyBulletHit` og lager lyd; Spillbarhet lager `AGSDoor` (åpne/lukke/låse/dirke) og tilbakestiller kartet ved kampstart | Andreas: «så mye som mulig interaktivt», granater som sprenger; påvirker spillet mest |
| **6 – Atriet og kantina, detaljert** | Ryddekroken (kiosk, kjøleskap, sittegalleri), salatbar med trebjelker, kaféøya med mosesirkler, piano og sofaer i loungen, stålbur-båser, kantinemøbler som instanser, plantekasser med planter, oransje mesanin med sittetrapp, trelameller på dekkekantene, runde blå matter, skilt og info-skjermer; ekte sagtak og fagverk i detalj; hengelampene | det mest kjente og mest synlige |
| **7 – Basene og Plan 4-rommene + Interaktiv 2** | undervisningsrom (pulter, stoler, kateter, lerret, tavler), grupperom (svarte bord, kontorstoler, whiteboard, skjerm, kodelås), korridorer (stavparkett, søyler med svarte ringer, grønne/svarte skap, vasker), laber og treningsleilighet, flaggsokkel og flaggmodell (Spillbarhets ønske), lagfarger; lysbrytere og lysgrupper per rom, skjermer som knuses, skap og kiosk med loot | der basekampene står |
| **8 – Klatreruter og vertikalitet** | Ryddekroken-ruta, klatrehallen åpnet (buldrevegger som klatrerute og utkikk), bokhyllene i studentarbeidsplassene, `Climb`-tagger (Kamps bestilling), knust rekkverk som hopp ned; vurder Plan 5 som spillbar høy utkikk | Andreas liker klatring |
| **9 – Lys og ytelse** | hengelamper og downlights riktig, dagslys/natt, Lumen-finjustering, måling på RTX 3050, LOD/instanser, kulisse utenfor (Elgeseter gate, Høgskoleparken, St. Olavs) synlig gjennom vinduene | må ses i editoren av Andreas |
| **10 – Interaktiv 3 og liv** | heiser, brannalarm, piano og bordtennis som virker, hengelamper som svinger, rot, papirer, oppslag og NTNU-skilt, Plan 2–3-rommene innredet, toppetasjen (P6) med treet og de lilla puffene | det siste som gjør stedet levende |

Rekkefølgen kan endres etter Andreas' tester og kontrollørens funn; ytelsen måles i hver runde, ikke bare i runde 9.

## Backlog (utenfor rundeplanen, viktigst øverst)
1. Fra den gamle backloggen som fortsatt gjelder: vær mot glasstaket (regnlyd og dråper på glasset, snø som legger seg på
   taket, mørkere atrium i snøvær), sollys gjennom vinduer, flimrende lysrør i noen rom, slitasje på gulv og trapper,
   raskere oppstart.
2. Lyd-soner (`SoundZones`) finjusteres når Kamp har koblet romklang.
3. U1 (garderober, sykkelplaza, løpebane) som mulig senere utvidelse – bare hvis Andreas vil ha mer kart.
4. Novemberkveld-lyset (utredet 26. nov., se historikk): solbanen for Trondheim i november gjelder fortsatt, nå sett gjennom
   glasstaket.

## Historikk: Gløshaugen-kartet (skrotet 27. nov. 2026)
Runde 1–3 laget et CTF-utsnitt av Gløshaugen (Gamle fysikk og Gamle kjemi som baser, Hovedbygningen med Lysgården i
midten, tre ruter ute og inne, gjerdet kant). Dataene står fortsatt i `GSCampus.h` bak `GSCampus::bHelgasetr` og testes av
`check_campus.py`; modellene ligger i `SourceArt/Models`. Lærdom som tas med: siktlinjer og dekning teller mer enn åpne
ruter; åpninger er siktlinjer; motprøver i alle sjekker; bygg i biter langs linjer testes i begge ender. Utredningen av
novemberlyset (sol for 63,4° N: høyeste sol 6°, opp ca. 09.15, ned ca. 14.45; låst eksponering; `WhiteTemp` trolig snudd;
månen uten skygge) gjelder fortsatt for `GSEnvironment`.

## Endringer i retningen
- 2026-11-27 (forhåndssjekk av plan-04, hovedagentens avgjørelse): runde 4 er delt i **4a** (klossbygg fra data, spillbart
  uten import) og **4b** (Blender-modellen, walk-sjekk, rendringer). Klossbygget blir reserve per modellbit. CTF-rettelser:
  ingen sidedør i basene, L-skjermer og lettvegg mot korridoren på glassboksene (bryter base-til-base-sikten over atriet),
  dekningslomme hver 8. m, spawn ut gjennom sluse med L-sving, og Trapp sør stopper på P3 (ellers snarvei inn i rødt lags
  lobby). Heiser og lobbyer er rettet etter tegningen. Gripekroken skal ikke feste over P4 i atriet. `Flag.Z = 13,4` til
  spawnsporet er flyttet (bestilt hos Spillbarhet).
- 2026-11-27 (plan-04): Andreas: «mappet skal bare være inni Helgasetr … skrap det gamle mappet», «bruk god tid og lag det
  nye mappet veldig bra», basene i hver sin ende på samme etasje. Ny visjon og ny rundeplan. Basene legges på **Plan 4** fordi
  den er nesten punktsymmetrisk (ringen er symmetrisk om akse 5 × H), har undervisningsrom i begge ender og ser ned i hele
  atriet; kantina blir lavruta. **Teknikk: blanding** – fast struktur fra Blender (kvalitet, Nanite, kan rendres og
  strålesjekkes uten editor), alt interaktivt som C++-instanser med ID fra samme datakilde. Etter Andreas' nye ønsker samme
  dag («så mye som mulig interaktivt», granater som sprenger lettvegger, glass, dører og møbler) er glass, lettvegger og
  dører skilt ut fra modellen allerede i råbygget, og **Interaktiv 1** (dører, knusbart glass, sprengning, flyttbar dekning) er
  lagt som runde 5, rett etter råbygget, foran detaljene. Ødeleggelse uten Chaos-brudd (krever editoren): ferdig brukne
  modeller + lokale biter, replikert bare som tilstand.
- 2026-11-27 (runde 3): «spillbart først» – sikksakk-dekning, Varmeteknisk, repos, vegger i basene (Gløshaugen, nå historikk).
- 2026-11-26 (runde 2): ytelse først, så dekning og spawn for CTF, kanten, tårn og roboter bort.
- 2026-11-26 (runde 1): Capture the Flag som eneste modus; mindre og mer detaljert kart.
- 2026-11-26: laget opprettet.
