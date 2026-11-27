# Plan 04b (Grafikk og kart): Helgasetr – Blender-modellen (lagets runde 4b)

Bygger på 4a (`ko/plan-04.md`): datakilden `Tools/Blender/helgasetr_plan.py`, `GSHelgasetr.gen.inl` og klossbygget i
`AGSHelgasetrBuilder` finnes og er sjekket. Fasit: `.claude/lag/grafikk/helgasetr.md`. Punktene «K…» viser til
`kontroll/forhand-plan-04.md`.

## Mål
Klossene byttes bit for bit mot en ekte modell av Helgasetr: buede atriehjørner, sagtak med hvitt fagverk, gule trappeløp
med vanger og bunnplater, glassbokser med hvitt undertak, trelameller på dekkekantene – slik at atriet ser ut som på
bildene, uten at spillet endrer seg.

## Forbedrer (gammelt)
- Klossbygget fra 4a (vegger, dekker, trapper, tak) erstattes av modeller; klossene blir reserve per bit.
- `init_unreal.py`-importen: importerer bare modeller som faktisk er endret, og kan gi bygningsbiter bedre avstandsfelt.

## Nytt
- `SM_Bygg_Helgasetr_*` (struktur), `SM_Glass_Helgasetr_*` (fast glass), modulene `SM_Mod_Helg_*`, hengelampen,
  kulisseløpene P4→P5→P6 over atriet, `check_helgasetr_walk.py`, rendringer, tre nye materialer.

## Filer
Nye: `Tools/Blender/building_helgasetr.py`, `Tools/Blender/check_helgasetr_walk.py`.
Endres: `Tools/Blender/helgasetr_plan.py` (bitnavn), `Tools/Blender/gen_helgasetr.py` (`ModelParts`),
`GSHelgasetr.gen.inl` (generert), `GSHelgasetrBuilder.cpp` (modell per bit, reserve per bit),
`Content/Python/init_unreal.py`, `Content/Python/gs_materials.py`, `IMPROVEMENTS.md` (630–645).

## Steg
1. **`building_helgasetr.py`** (`blender --background --python Tools/Blender/building_helgasetr.py -- [--render]`), med
   `MeshKit` og `helgasetr_plan.py`. Eksporterer FBX + .blend til `SourceArt/Models/` **bare når innholdet er endret**
   (sammenlign en hash av geometri + materialer med forrige eksport; ellers rør ikke fila – K18).
   - `SM_Bygg_Helgasetr_<P1…P6>_<V|Ø>_<N|M|S>`: **6 biter per plan**, delt ved X = 0 og Y = ±18 (ingen bit over ca. 26 m,
     K17). Dekker med hull, undersider og dekkekanter (hvit gips, mørke trelameller mot atriet), yttervegger med vindushull,
     innervegger med åpninger og karmer, søyler med svarte ringer, trapperom, boksenes gulv/tak/stolper, Ryddekroken,
     rekkverkenes treoverkant og stolper, L-skjermer. **Ikke** glass, lettvegger, `COVER`, dører eller himlinger (K21).
   - `SM_Bygg_Helgasetr_Trapper` (atriumtrappa løp 1–3 og sosialtrappa, ekte trinn, vanger, bunnplater, fyll under lave
     soner) og `SM_Bygg_Helgasetr_Trapperom` (Trapp sør og øst).
   - `SM_Bygg_Helgasetr_Overatriet`: alt over P4 inne i atriet – P4-boksenes tak, P5/P6-boksene, **gule løp P4→P5→P6 som
     krysser atriet** (bilde 01, 06; K7, kulisse), hengestenger. Byggeren gir denne biten kollisjonen fra 4a steg 5.5
     (blokker `Visibility`, ignorer `Pawn`/`WorldStatic`).
   - `SM_Bygg_Helgasetr_Tak`: sagtak (møner øst–vest, glass i de bratte flatene), hvitt Warren-fagverk, flatt tak over
     fløyene.
   - `SM_Glass_Helgasetr_Fasade` og `SM_Glass_Helgasetr_Tak`: fast glass (BlockAll).
   - Moduler: `SM_Mod_Helg_GlassPanel` (1 × 1 m, 6 cm tykk), `SM_Mod_Helg_GlassDoor` (svart ramme, prikkebånd,
     kodelås-boks), `SM_Mod_Helg_Lettvegg` (1 × 1 m, 10 cm, whiteboard-variant `SM_Mod_Helg_LettveggTavle`),
     `SM_Mod_Helg_Karm`; `SM_Prop_Helg_Hengelampe` (ring Ø 2,4 m, lysende underside `LightPanel`).
   - Materialspor som helgasetr.md kap. 8 (`_In` inne).
   - `--render`: ovenfra-snitt P1–P4 og perspektiver (P4 nordkorridor mot sør, mesaninen ned i kantina, P4 vestkorridoren,
     kantina opp mot glasstaket) til `.claude/lag/grafikk/kontroll/r04b-*.png`, sammenlignet med bilde 01, 06, 22, 26.
2. **`init_unreal.py`:** `"SM_Glass_Helgasetr"` inn i `COMPLEX_COLLISION_PREFIXES` (ikke Nanite). **`SM_Mod_Helg_` skal
   ikke være kompleks** – modulene får enkel bokskollisjon ≥ 5 cm (automatisk generert boks er nok) (K15). For
   `SM_Bygg_Helgasetr_*`: sett `distance_field_resolution_scale` = 2 via `unreal.StaticMeshEditorSubsystem`
   `get_lod_build_settings` / `set_lod_build_settings` (K17).
3. **`gs_materials.py`** `INSTANCES`: `"HelgYellow": ("PaintedMetal", (1.15, 0.62, 0.06), 2, 0.8, 0, "Surface")`,
   `"HelgOrange": ("Plastic", (1.1, 0.42, 0.08), 2, 1.0, 0, "Surface")`, `"PaintWhite": ("PaintedMetal", (1.1, 1.1, 1.08),
   2, 0.9, 0, "Surface")`. Glassmaterialet (K19): ingen refraksjon, ingen Lumen-refleksjon på gjennomsiktig, enkel
   gjennomsiktig lysmodus – endre `_glass_master()` bare hvis det ikke er slik allerede, og si fra.
4. **`helgasetr_plan.py` / `gen_helgasetr.py`:** `MODEL_PARTS` = bitnavn → hvilke klossgrupper i `gen.inl` den erstatter, og
   kollisjonsart (`Full`, `GlassFull`, `AboveAtrium`, `None`).
5. **`GSHelgasetrBuilder.cpp`:** for hver bit: finnes `/Game/Models/<Name>`, legg den ut med riktig kollisjon og hopp over
   klossene den erstatter; ellers bygg klossene (reserve **per bit**, K13). Moduler: bytt `GSUtils::Cube()` mot
   `SM_Mod_Helg_*` i de egne ISM-ene når de finnes (samme id-rekkefølge). Glasskomponenter: `CastShadow`,
   `bAffectDistanceFieldLighting` og `bVisibleInRayTracing` av (K19). P5/P6: bare rekkverksglass, ikke boksvegger på begge
   sider (instansbudsjett ca. 1 200).
6. **`check_helgasetr_walk.py`** (Blender, leser .blend-filene; Unreal-koordinater, Y speilet) – med motprøver:
   - **Etterligner GameMode** (K2): for hvert spawnpunkt en stråle fra (x, y, `Flag.Z` + 4,0) ned til −3,0 skal treffe gulvet
     innen 0,05 m fra Z 13,9 – og etter Spillbarhets endring fra + 2,0; for hvert flagg en stråle fra +3,0 ned. I tillegg
     ingenting med kollisjon mellom gulvet og gulv + 2,2 m over punktene. *Motprøve:* punkt under trapperommets repos.
   - Dører åpne (stråler gjennom senteret i 1,0 og 1,9 m), *motprøve* 0,7 m til siden.
   - Trinn ≤ 0,20 m, fri høyde ≥ 2,2 m over trinn; under trapper < 2,0 m er fylt; ingen spalte 0,3–0,9 m (K11).
   - Rekkverk langs **hele** hullkanten (`RAIL_EDGES`, K5): stråle utover 1,0 m over gulvet hver 0,5 m treffer glassfelt
     (regnet fra `GLASS_PANELS`) eller struktur innen 0,3 m. *Motprøve:* ett felt fjernet.
   - Ingen flate man kan stå på over P4 i atriet i modellbitene med `Full`-kollisjon (K10). *Motprøve:* en testplate.
   - Kapselprøve (r 0,42, h 1,84) langs alle gulv: ingen steder der kapselen står fast mellom gulv og noe over/ved siden.
7. **Prøv:** kjør generatoren, `check_helgasetr.py`, Blender-skriptet med `--render`, walk-sjekken; bygg C++
   (`Build.bat … -WaitMutex`); Andreas åpner Unreal (importen skjer da) og trykker Play.

## Ferdig når
- Begge sjekkskript gir bare OK, alle motprøver feiler; FBX skrives ikke på nytt ved en ny kjøring uten endringer.
- Rendringene viser atriet med glasstak og fagverk, de gule løpene som krysser atriet, stablede glassbokser med hvitt
  undertak, ringkorridor med søyler, terrazzo i kantina og oransje mesanin (kontrolløren sammenligner med bildene).
- I spillet: samme ruter og sporprøve som i 4a (flagg 2/2, spawn 16/16), og loggen viser at modellbitene brukes (ingen
  reserve-melding). `stat unit` / `stat gpu` før (4a) og etter (4b) på de samme to stedene, innenfor budsjettet i
  helgasetr.md kap. 9.

## Risiko
- Første import tar tid (ca. 40 biter, Nanite + kollisjon). Reserven per bit holder spillet helt imens.
- `set_lod_build_settings` kan mangle i Python-API-et i 5.8: si fra, ikke hopp over oppdelingen.
- Modell og klosser må ligge på nøyaktig samme sted (samme datakilde) – ellers dobbelvegger eller hull der én bit mangler.

## IMPROVEMENTS-linjer (630–645, seksjon «Runde 4b (Grafikk og kart): Helgasetr, modellen»)
630. Helgasetr er nå en ekte modell i stedet for klosser, delt i biter som byttes inn én og én
631. Atriets hjørner er runde slik de er i bygget, med glassrekkverk som følger buen
632. Sagtaket over atriet har glass i de bratte flatene og hvitt stålfagverk under, og sola kaster fagverkets skygger ned i atriet
633. Dekkekantene mot atriet har mørke trelameller og hvit underside med runde downlights
634. De gule trappeløpene har vanger, bunnplater og svart håndløper, og ser ut som de henger fritt i atriet
635. Gule trappeløp krysser atriet også over 4. etasje, som på bildene, uten at man kan komme opp dit
636. Glassboksene har hvitt, tykt undertak og svarte stolper, og henger i hvite stenger fra taket
637. Ryddekroken er et gult tårn med kiosknisje og sittegalleri på toppen
638. Betongsøylene har de to svarte ringene, og korridorgulvene er lys stavparkett
639. Trapp sør og Trapp øst er betongtrapperom med stålrekkverk og repos
640. Nye materialer: Helgasetr-gult på trappene, oransje mesaningulv og hvitmalt stål
641. Store runde hengelamper (2,4 m) henger i lange vaiere over kantina og lyser ned
642. Glasset kaster ikke skygge og tegnes enklere, så atriet holder bildefrekvensen på en RTX 3050
643. Bygningsbitene har bedre avstandsfelt, så lyset ikke lekker gjennom vegger og dekker
644. Modellene importeres bare på nytt når de faktisk er endret, så editoren starter raskere
645. Nytt Blender-sjekkskript: GameModes spawn- og flaggspor, åpne dører, trinn, rekkverk langs hele atriekanten og kapselprøve – med motprøver
