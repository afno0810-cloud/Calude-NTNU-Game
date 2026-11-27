# Plan 05 (Grafikk og kart): bevis 4b i Unreal, lys og bilde, rom med innhold – delt i 05a og 05b

Bygger på 4a (bebbf7c) og 4b (fbf7de7). Fasit: `.claude/lag/grafikk/helgasetr.md` (kap. 7 lys, 9 ytelse, 13–14 id-er og modell).
Hovedagentens prioriteringer: (1) bevis 4b hodeløst, (2) lys og bilde, (3) det 4b ikke rakk. Pluss to små rettinger fra
Spillbarhets kontroll (`.claude/lag/spillbarhet/kontroll/runde-06.md` F1/F2).

**Avklart før planen (ikke gjør dobbelt):** Interaktiv har i sin 1b laget knusbare glassfelt og dørmodeller selv (IMPROVEMENTS
718–733, `Tools/Blender/interactive_doors.py`), og tar lettveggene i sin runde 2 (`interaktiv/ko/plan-02.md`). Grafikk lager
derfor **ikke** `SM_Mod_Helg_GlassPanel/GlassDoor`. Det som gjenstår for oss er: hel lettveggmodell, faste møbler med id-liste
(`Props()`), og at selve bygget ser ferdig ut. **Runde atriehjørner utsettes**: de flytter rekkverksfeltene, og glass-id-ene er
nå Interaktivs (343 felt i manifestet) – står i backlog til Interaktiv sier ja i `bestillinger.md`.

---

# 05a – bevis, lys og bilde (bygges først)

## Mål
Andreas ser et klart, lyst atrium der sola står inn gjennom glasstaket i synlige stråler med fagverkets skygger, korridorene
er riktig eksponert (ikke utbrent eller grå), og vi har bilder og målinger fra ekte spill som beviser det – uten at han åpner
editoren.

## Forbedrer (gammelt)
- 4b-modellen: importen og bruken bevises hodeløst (rapport + logglinje «alle N modelldeler fra Blender er i bruk»).
- `init_unreal.py`: importen tåler låste filer og en annen Unreal-prosess (hovedagenten: Kamps hodeløse kjøring reimporterte
  81 modeller og fikk låst `M_GS_Surface`), og skriver en rapport.
- Eksponering: i dag bare fart, ingen grenser (funn C «natt som ikke blir mørk») → faste EV-grenser innendørs.
- Lyset i Helgasetr: sola er 10 lux mot lamper på 1800 lm, så dagslyset drukner → fysisk riktige enheter.
- Lootdata: plassen (11,0, 3,0, 9,9) inne i den gule trappa flyttes (F2).

## Nytt
- `Grafikk.Foto`: konsollkommando som kjører en fast kamerarute i spillet, tar HighResShot-bilder, måler tider per utsnitt,
  logger dem og lukker spillet selv.
- `Tools/Unreal/run_import.ps1` og `run_foto.ps1`: hodeløs import og hodeløs fotorunde (en kommando hver).
- Volumetrisk tåke i atriet med solstråler gjennom sagtaket; Lumen-innstillinger for innendørs.
- 4 skyggekastende lys (budsjettet i kap. 9 tillater maks 4).
- Lootplasser inne i de låste grupperommene (risiko gir belønning), med id i manifestet.

## Hvorfor nå
4b er ikke sett i Unreal ennå: uten bevis bygger alt videre på antakelser. Lys er det Andreas har bedt om flest ganger
(«bedre grafikk», «tydeligere», «mye bedre lyseffekter»), og et bilde-/måleverktøy trengs for all lysjustering etterpå.

## Filer
Nye: `Tools/Unreal/run_import.ps1`, `Tools/Unreal/run_foto.ps1`, `Content/Python/gs_headless_import.py`,
`Source/Gloshaugen/Private/GSFoto.cpp` (konsollkommandoen; Grafikk eier den – legg filen til i LAG.md-tabellen under Grafikk).
Endres: `Content/Python/init_unreal.py`, `Content/Python/gs_materials.py` (bare låse-toleranse), `GSEnvironment.cpp/.h`,
`GSHelgasetrBuilder.cpp`, `Tools/Blender/helgasetr_plan.py` (loot), `gen_helgasetr.py`, `GSHelgasetr.gen.inl` (generert),
`Tools/Blender/check_helgasetr.py`, `Tools/Blender/helgasetr_ids.json`, `.claude/lag/grafikk/helgasetr.md` (kap. 7, 9, 13),
`IMPROVEMENTS.md` (960–975).

## Steg
1. **Lootdata (liten, først – F2 og F1):**
   1. `helgasetr_plan.py` `loot(P3, …)`: flytt (11,0, 3,0) ut av trappa (f.eks. x 8,5 – sjekk med steg 1.3) med samme tier.
   2. **Nye plasser legges SIST i `LOOT`** (eksisterende indekser flytter seg ikke): én plass per rom med `glass_kode`-dør
      (Grupperom 2.1–2.3, Møterom, Studentforeningen, 3.N1–N2, 3.V*, 3.S1–S3, 4.N1–N2, 4.V*, 4.O1–O2). Sted: romsenteret
      flyttet 1,0 m bort fra døra langs dørnormalen inn i rommet, minst 0,6 m fra vegger og fra `COVER`. Tier 0 (Spillbarhet
      oppgraderer én tier). Unntak: rom innen 8 m fra et flagg får ingen (ikke gi forsvarerne mer).
   3. `check_helgasetr.py`, ny sjekk **l**: hver lootplass ligger i et rom/korridor med gulv, ikke inne i noen `Coll_Solid`-boks
      (kule r 0,15 m fra gulv + 0,1 til + 0,4 m), og de nye ligger på romsiden av sin dør (fortegn av (plass − dør)·normal).
      *Motprøve:* den gamle (11,0, 3,0) skal feile.
   4. `helgasetr_ids.json`: ny nøkkel `"loot"` (append-only, samme regel som dører/glass: en eksisterende id kan ikke flytte
      mer enn 0,1 m uten at sjekk k feiler; flyttingen i 1.1 er unntaket – skriv den inn i manifestet i samme runde).
   5. Kjør `gen_helgasetr.py`. Skriv i `bestillinger.md` (→ Spillbarhet): «N lootplasser inne i de låste grupperommene ligger
      sist i `LootM` (indeks X–Y); F1-sjekken deres på romsiden finner dem nå».
2. **Hodeløs import (`gs_headless_import.py` + `run_import.ps1`):**
   1. `run_import.ps1`: stopp med tydelig melding hvis `Get-Process UnrealEditor*` finner noe (også `-Cmd`); ellers kjør
      `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<prosjekt>\Gloshaugen.uproject"
      -run=pythonscript -script="<prosjekt>\Content\Python\gs_headless_import.py" -unattended -nosplash -nop4 -stdout
      -FullStdOutLogOutput` (ikke `-game`, ikke `-nullrhi` hvis Nanite/avstandsfelt da ikke bygges – prøv begge, skriv ned
      hva som virket). Tidsgrense 30 min, logg til `Saved/Logs/headless_import.log`.
   2. `init_unreal.py` kjører allerede ved oppstart av commandleten (Kamps kjøring viste det). Del koden så importen er en
      funksjon `run_all()` som returnerer en rapport (importert / hoppet over / feilet per modell, materialer, kollisjon,
      avstandsfelt), og la oppstartsdelen kalle den. `gs_headless_import.py` kaller ikke import på nytt hvis oppstarten
      allerede gjorde det (modulvariabel), men **sjekker**: for hver FBX i `SourceArt/Models/` som matcher
      `SM_Bygg_Helgasetr_*`, `SM_Glass_Helgasetr_*`, `SM_Prop_Helg_*`: finnes `/Game/Models/<navn>`, har den Nanite/kompleks
      kollisjon etter reglene, og materialspor koblet (ingen `WorldGridMaterial`). Skriver `Saved/Logs/gs_import_report.txt`
      og logger «Gloshaugen: import-rapport: N/58 Helgasetr-modeller OK, M feil» og avslutter med kode ≠ 0 ved feil.
   3. **Låste filer:** i `gs_materials.run()` og modellimporten: fang feil per asset (ikke stopp hele løkka), prøv lagring på nytt
      inntil 3 ganger med 2 s mellom, og før dem i rapporten som «låst». `run_import.ps1` kjører importen på nytt én gang hvis
      rapporten har låste filer.
   4. Kjør den. 58 Helgasetr-modeller forventes (48 etasjebiter + Trapper, Trapperom, Overatriet, Tak, Takfagverk, Pynt +
      3 glass + Hengelampe = 58; tell fra `ModelParts()` og si fra hvis tallet er et annet).
3. **`Grafikk.Foto` (`GSFoto.cpp`, `FAutoConsoleCommandWithWorldAndArgs`):**
   1. Venter til `AGSHelgasetrBuilder` har bygget (eller 10 s), slår av HUD-tegning lokalt (`showhud`-ekvivalent), spawner én
      `ACameraActor` og `SetViewTargetWithBlend` på første lokale `APlayerController`.
   2. Fast rute (meter, fra helgasetr.md kap. 9 + kap. 3; samme som Blender-rendringene i 4b så de kan sammenlignes):
      A P4 nordkorridor mot sør, B kantina (P1) opp mot glasstaket, C mesaninen ned i kantina, D P4 vestkorridor,
      E rødt lags base fra slusa, F atriet ovenfra fra P4-rekkverket, G et grupperom på P3 (lys inne). Kl. 12 og kl. 15
      (sett tid via `GSEnvironment` sin eksisterende tid-/vær-styring; klart vær), pluss B og E kl. 22.
   3. Per utsnitt: vent 3 s (Lumen og eksponering legger seg), mål i 3 s: snitt og maks for bildetid, spilltråd, rendertråd og
      GPU (`FApp::GetDeltaTime()`, `GGameThreadTime`, `GRenderThreadTime`, `RHIGetGPUFrameCycles()` → ms). Logg én linje:
      `Grafikk.Foto: A kl12 frame 15.8/19.2 game 4.1 draw 6.3 gpu 14.9/17.0 ms`. Ta `HighResShot 1920x1080` til
      `Saved/Screenshots/foto/<utsnitt>_<tid>.png`. Ett utsnitt med `stat gpu` på skjermen (`Shot`, ikke HighResShot) for A og B.
   4. Til slutt: `Straaling 0` (Kamps kommando, programvare-Lumen) og mål A og B en gang til, så logges begge.
      Så `FPlatformMisc::RequestExit(false)`.
   5. `run_foto.ps1`: samme prosess-sjekk som 2.1, så `UnrealEditor.exe "<uproject>" /Game/Maps/Gloshaugen?Mode=Explore
      -game -windowed -ResX=1920 -ResY=1080 -nosound -ExecCmds="Grafikk.Foto"` med tidsgrense 6 min (drep prosessen etter).
      Kopier bildene til `.claude/lag/grafikk/kontroll/r05-*.png` og skriv tallene til `.claude/lag/grafikk/kontroll/r05-maaling.md`.
      Kjør først **før** steg 4–6 (dagens lys = «før»), så etter.
4. **Eksponering og fysisk riktig lys (`GSEnvironment`, bare når `GSCampus::bHelgasetr`):**
   1. Sola i lux som ute: klart 75 000, overskyet 20 000, regn/tåke 10 000, snø 15 000 (med `DayFactor` som i dag); himmellys
      tilsvarende (sjekk skalaen mot `SkyLight` real-time capture). Månen 0,3 lux.
   2. Post-prosess: `AutoExposureMethod = AEM_Histogram`, `AutoExposureMinBrightness`/`MaxBrightness` i EV100 (prosjektet
      har `ExtendDefaultLuminanceRange=True`): min 3, maks 13; `AutoExposureBias` 0,5; `HistogramLogMin/Max` −4 / 16;
      `LowPercent` 70 / `HighPercent` 95 så glasstaket ikke styrer innendørs. Fart: opp 3, ned 1,5 (øyet: inn i mørkt rom
      tar lenger tid). Grensene gjør at natt blir natt (funn C).
   3. Innelys i ekte enheter: korridor-punktlys 1 800 lm og rektangellys 6 000 lm står; sjekk etter bildene at korridorene er
      EV ca. 6–8 og atriet ca. 9–11 midt på dagen (logg `GetSceneLuminance`-lignende tall hvis lett, ellers bildene).
   4. Bildene i steg 3 avgjør justeringer; skriv tallene du endte på i helgasetr.md kap. 7.
5. **Volumetrisk tåke og solstråler:**
   1. Helgasetr-modus: høydetåke med svært lav tetthet (0,004–0,01) og `FogHeightFalloff` slik at den bare fyller bygget
      (bunn Z 0, avtar over taket), `VolumetricFogExtinctionScale` 1, `VolumetricFogScatteringDistribution` 0,7–0,8 (fremover-
      spredning gir stråler mot sola), albedo lys grå. Ingen synlig dis i korridorene på 30 m (sjekk bilde A).
   2. Sola: `VolumetricScatteringIntensity` 1,5–3, `bCastVolumetricShadow` på, så fagverkets skygger blir striper i lufta.
      Takglasset må ikke blokkere sola (kaster ikke skygge, 4b), fagverket må kaste skygge (sjekk `SM_Bygg_Helgasetr_Takfagverk`
      har `CastShadow` på).
   3. Innelys: `VolumetricScatteringIntensity` 0 (ellers betaler hvert lys for tåka), unntatt de 4 i steg 6.
   4. Ytelse: `r.VolumetricFog.GridPixelSize` 16 og `GridSizeZ` 64 på Middels (sett i `GSEnvironment` når Helgasetr, ikke i ini).
6. **Fire skyggekastende lys** (budsjett kap. 9: maks 4): to av hengelampene nærmest kantinas midte (`URectLightComponent`
   `SetCastShadows(true)`, `BarnDoorAngle` 60 så skyggene blir myke men retningsbestemte), og ett spotlys (`USpotLightComponent`,
   ytre vinkel 55°, 2 500 lm, 3 500 K, rekkevidde 7 m, `CastShadows` på) i taket i hver base-lobby på P4 – dramatisk skygge der
   basekampene står. Velg lysene i dataene (`helgasetr_plan.py`: felt `shadow=True` på 4 lys, generert), ikke hardkodet.
7. **Lumen innendørs (post-prosess, Helgasetr-modus):** `LumenSceneLightingQuality` 1, `LumenFinalGatherQuality` 1,5,
   `LumenSceneDetail` 1,5 (små ting som søyleringer og rekkverk), `LumenMaxTraceDistance` 60 m (bygget er 68 m),
   `LumenSkylightLeaking` 0,05 (hindrer svarte hjørner uten å lyse opp lukkede rom), `LumenRayLightingMode` overflatecache.
   Mål A og B med og uten `Straaling 0` (steg 3.4). **Bytt ikke standardverdien i ini** – skriv anbefalingen i `r05-maaling.md`;
   hovedagenten avgjør (Kamp eier `Straaling`).
8. **Prøv:** `check_helgasetr.py` og `check_helgasetr_walk.py` (alt OK, motprøver feiler), `Build.bat … -WaitMutex`,
   `run_import.ps1`, `run_foto.ps1`. Sjekk loggen for «alle N modelldeler fra Blender er i bruk» og sporprøven (flagg 2/2,
   spawn 16/16).

## Ferdig når
- `gs_import_report.txt`: 58/58 (eller tallet fra `ModelParts()`) OK, 0 feil, 0 låste; prosesskode 0.
- Spill-loggen: «Helgasetr: alle N modelldeler fra Blender er i bruk», flagg 2/2 og spawn 16/16, og ingen reserve-melding.
- `kontroll/r05-*.png` (før og etter) og `r05-maaling.md` med tall for alle utsnitt; Foto-runden lukket spillet selv.
- Etter-bildene: synlige solstråler med fagverksskygger i B og F kl. 12; korridorene A og D verken utbrent eller grå; natt
  (kl. 22) er mørk ute bak glasset med innelyset på.
- Ytelse A og B i Middels: GPU-snitt ≤ 16,6 ms, maks ≤ 22 ms (45 bilder/s-gulvet i kap. 9). Er det over: skru volumetrisk tåke
  ned først (GridPixelSize 24), så skyggelysene til 2, og skriv det ned.
- `check_helgasetr.py` sjekk l OK, motprøven feiler; manifestet har `loot`.

## Risiko
- `-run=pythonscript` kan bygge Nanite/avstandsfelt annerledes uten RHI → prøv uten `-nullrhi` først.
- To Unreal-prosesser samtidig låser `.uasset`-filer: skriptene nekter å starte da; kjør aldri mens Andreas har editoren oppe.
- HighResShot i et vindu krever at Andreas' skjerm er på (ikke låst PC) – logg «ingen bilder» hvis filene mangler, ikke heng.
- 75 000 lux + histogram kan gi blaff ved overgang korridor → atrium; justér fart, ikke grenser.
- Andre lag (Interaktiv, Spillbarhet) bruker lootindekser/id-er: bare legg til sist, aldri flytt (unntatt 1.1, meldt i bestillinger).

## IMPROVEMENTS-linjer (960–975, seksjon «Runde 5a (Grafikk og kart): lys i Helgasetr og bevis i Unreal»)
960. Helgasetr-modellen er nå sjekket i Unreal uten at noen åpner editoren: alle 58 modelldeler er importert og i bruk
961. Importen av modeller og materialer tåler at en fil er låst av en annen prosess, prøver igjen og skriver en rapport
962. Sola er like sterk som ekte sol, så dagslyset fyller atriet og lampene lyser som ekte lamper
963. Kameraet tilpasser seg lyset som et øye: korridorer og atrium er riktig opplyst, og natta er mørk
964. Solstråler står ned gjennom glasstaket i en lett dis, med skyggene av fagverket i lufta
965. Lumen er stilt inn for innendørs: mindre lekkasje i hjørner og bedre lys på små ting som rekkverk og søyleringer
966. De to midterste hengelampene over kantina kaster myke skygger av bord og folk
967. Basene har et skyggekastende taklys i lobbyen, så du ser skyggen av den som kommer rundt hjørnet
968. Innelysene betaler ikke for tåka, så solstrålene ikke koster bildefrekvens
969. Ny konsollkommando Grafikk.Foto: fast kamerarute med bilder og tidsmåling, som lukker spillet selv
970. Hodeløs import og fotorunde med én kommando hver, som nekter å starte hvis Unreal allerede kjører
971. Målinger av bildetid og GPU-tid fra Helgasetr før og etter, med og uten maskinvare-strålesporing
972. Lootplassen som lå inne i den gule trappa i 3. etasje er flyttet ut
973. Nye lootplasser inne i de låste grupperommene: bryt opp døra og finn bedre utstyr
974. Kartsjekken stopper lootplasser som ligger inne i vegger, trapper eller møbler
975. Lootplassene har faste id-er i manifestet, så de ikke flytter seg mellom runder

---

# 05b – rom med innhold (bygges etter 05a, etter svar fra Interaktiv)

## Mål
Rommene i Helgasetr er ikke lenger tomme bokser: kantina, grupperommene og undervisningsrommene har møbler som på bildene,
og lettveggene ser ut som gipsvegger i stedet for kuber.

## Forbedrer (gammelt)
- Lettveggene (`BreachWalls`, 6 stk. i manifestet) tegnes med ekte modell `SM_Mod_Helg_Lettvegg` (gips på stendere, 10 cm,
  sokkel, whiteboard-variant `SM_Mod_Helg_LettveggTavle`) i stedet for `GSUtils::Cube()` – samme ISM, tag `GS_Int_Breach` og
  instansindeks = id (Interaktiv overtar ISM-en i sin runde 2 og lager hull-modellen selv).
- Dekningsklossene (`COVER`, 24) blir møbler med samme kollisjon: kantinebenker/skranker, plantekasser, skap.

## Nytt
- `Tools/Blender/props_helgasetr.py`: kantinebord (svart, kvadratisk på søylefot), rundt bord, trestol, langbord, plantekasse
  med planter, stålbur-bås med grønn sofa, grupperomsbord, kontorstol, whiteboard, skjerm på vegg, undervisningspult, kateter,
  skap (grønt/svart), puff (oransje/turkis). Materialspor `_In`, Nanite for faste, ikke for de Interaktiv gjør løse.
- `GSHelgasetr::Props()` (generert): `Id` (stabil, i manifestet under `"props"`, append-only), type, sted, yaw, rom,
  `Fast`/`Los` (løs = Interaktivs `AGSPhysicsProp`; vi legger bare ut faste, de løse er data til dem).
- Møblering etter helgasetr.md kap. 8 i kantina (P1), mesaninen (P2), grupperommene på P3–P4 og de to undervisningsrommene i
  basene. Instanser (én ISM per type), ingen skygge fra små ting (stoler), skygge fra bord og båser.

## Steg (kort – detaljeres når 05a er bygget)
1. **Bestilling først** (skrives nå i `bestillinger.md`, se under): Interaktiv bekrefter formatet på `Props()` og hvilke typer
   som er løse. Til svaret kommer: alle stoler og puffer er `Los`, alt annet `Fast`.
2. `props_helgasetr.py` + rendring `kontroll/r05b-*.png` mot bilde 04, 08, 20–22, 24.
3. Plassering i `helgasetr_plan.py` med regler: ikke i dørsving (dørbredde fra hengsel), ikke på ruter/spawn/flagg (kapsel-
   prøve i walk-sjekken), dekningsmøbler beholder høyde/bredde fra `COVER`, siktlinjeregelen (≤ 42 m) uendret.
4. `check_helgasetr.py`: møbler ikke i vegg, ikke i dørsving, manifest-sjekk for `props`. Motprøver.
5. Builder: ISM per type; reserve-klosser hvis modell mangler (som 4b). Foto-runden fra 05a før/etter, og tellingen av
   ikke-Nanite tegnekall (≤ 1 500, kap. 9).

## Ferdig når (05b)
Rendringer og Foto-bilder viser møblerte rom; sjekkene OK med motprøver; ruter, spawn og flagg som før; GPU-tid i A/B innenfor
+1 ms av 05a.

## IMPROVEMENTS-linjer (976–990, skrives ut av 05b-planen)

## Risiko (05b)
- Id-er: møbler Interaktiv gjør løse må ha samme id i begge lag – derfor bestilling før bygging.
- Instansbudsjett og skygger fra mange små ting; stoler uten skygge.
