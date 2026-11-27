**GODKJENT**

# Kontroll av runde 4b (Grafikk og kart) – commit fbf7de7, Helgasetr som Blender-modell

Kontrollert mot `ko/plan-04b.md`, `kontroll/runde-04a.md` (funn 1–5) og fasiten `helgasetr.md`. Jeg har lest commiten med
`git show`. `Tools/Blender/*`, `GSHelgasetr*` og `Content/Python/*` i arbeidsmappen er like commiten, så skriptene jeg kjørte
gjelder commiten. Build.bat er ikke kjørt. Importen i Unreal er ikke prøvd, så alt om modellene i spillet er lest ut av kode
og data, ikke sett.

Kjørt:
- `python Tools/Blender/gen_helgasetr.py --check`: OK.
- `python Tools/Blender/check_helgasetr.py`: «Alt OK». a–k går gjennom, også de nye j2 (veggtopper) og k (id-manifest), og
  alle motprøvene blir avvist.
- `blender --background --python Tools/Blender/check_helgasetr_walk.py`: alle 6 sjekkene OK, og alle 6 motprøvene blir
  avvist (spor 2/2 + 16/16, 53 dører åpne, 289 trinn, 528 rekkverksstråler, ingenting å stå på over P4, 40 811 gulvpunkter).
- Min egen prøve i Blender (samme verden som walk-sjekken, også `AboveAtrium`-delene): fri høyde 2,2 m over alle trinn og
  repos, 9 punkter per trinn, 2 601 stråler. Ingen treff. (Planens krav om fri høyde over trinn var ikke med i walk-sjekken.)
- `building_helgasetr.py` uten `--force` på tre deler: alle «uendret», ingen filer endret (K18 holder).

## Andreas' regel: forbedret og nytt
- **Forbedret:**
  - Klossbygget fra 4a byttes del for del mot modeller, med klossene som reserve per del.
  - Alle fem funnene fra 4a er tatt:
    - Funn 1: `Flag.Z` er 13,9, og `ProbeGameModeTraces` kopierer `SpawnFlags`/`TryPlace` slik de står i `GSGameMode.cpp`
      l. 444 og 770–781. Jeg sammenlignet linje for linje.
    - Funn 2: `bNoOpening` er lagt til, og bestillingen til Interaktiv er skrevet.
    - Funn 3: `Coll_High` stopper `Pawn` og er ugangbar, og veggtoppene på P4 har fått en kappe. Sjekk j2 dekker dette, med
      motprøve.
    - Funn 4: `bLocked` gjelder nå for `glass_kode`.
    - Funn 5: korridorlysene på P3 er fjernet, og flatelysene rekker 13 m.
  - Glassmaterialet er billigere.
- **Nytt:**
  - 57 modelldeler pluss hengelampen.
  - Sagtak med glasset mot sør og fagverk av rør.
  - Lameller, downlights og søyleringer.
  - Kulissebroen P5→P6.
  - Tre nye materialer.
  - Id-manifestet (sjekk k).
  - Walk-sjekken i Blender.
  - Rendringer.

Begge deler er oppfylt.

## Sjekket spesielt

**1. Dobbelt eller hull (BuildModelParts / UseModelPart).**
- `BuildModelParts()` kjører før `BuildStructure()`. `ModelPartUsed[I]` blir bare `true` når `ModelMesh()` fant ressursen og
  komponenten er registrert.
- `BuildStructure` hopper over en kloss bare når `BoxPartM[i] != 255 && UseModelPart(...)`.
- `static_assert` sikrer at `BoxPartM` har samme lengde som `BoxesM`.
- `building_helgasetr.py` lager hver del av nøyaktig de klossene `model_part_of()` gir den, fra samme `derive()`. Unntaket er
  `Takfagverk`, som er tegnet på nytt som rør, men den er kulisse med `Above`-kollisjon.
- Klossene som alltid blir værende (255) er 18 stykker:
  - 8 L-skjermer i tre
  - 4 blokkeringer
  - 6 baseplater
- Glassfeltene, lettveggene og dekningen er egne lister og berøres ikke.
- Hengelampen: med modell lages 0 glødeklosser, og de 3 vaierne treffer festene på ringen (samme vinkler, r 1,2).
- Jeg fant verken doble deler eller hull. **Men** reserven slår bare inn når ressursen mangler, ikke når den finnes uten
  riktig kollisjon (funn 1).

**2. Kollisjon på modelldelene.**
- `SM_Bygg_` står i både `COMPLEX_COLLISION_PREFIXES` og `MEGASCANS_PREFIXES`, så delene blir Nanite med kompleks kollisjon.
- `SM_Glass_Helgasetr` får kompleks kollisjon, men ikke Nanite. Det er riktig for gjennomsiktig glass.
- `SM_Mod_Helg_` er ikke kompleks (K15), men modulene er ikke laget (se avvik).
- Trappene har ekte trinn i `.blend`. Walk-sjekk 3 bekrefter ≤ 0,20 m og at trinnene står der dataene sier.
- Hvordan kollisjonen blir etter Nanite-importen er ikke sett (funn 1).

**3. `Coll_High` er nå `QueryAndPhysics` og stopper `Pawn`. Stenger det noen rute?**
- Den laveste `Coll_High`-flaten er 16,82 m (hengestolper ved P4-boksene), altså 2,92 m over P4-gulvet.
- Kroppen er 1,84 m, og hoppet når 0,94 m (`JumpZVelocity` 430 → 430²/1960). Toppen av hoppet er da 2,78 m. Det gir 14 cm
  klaring, så ingen rute stenges.
- Ingen trapp går over P4. P5–P6 kan ikke nås, og der står blokkeringer langs atriekanten. Kulisseløpene, broen og
  fagverket (bunn 23,6 m) står derfor bare i veien for luft.
- Klatring (`TryMantle`) sporer `ECC_WorldStatic`-kanalen, som `Coll_High` ignorerer. Gripekroken går også gjennom.
- GameModes spor er objektspor av typen `WorldStatic` og treffer `Coll_High`, men ingen spawnpunkter eller flagg ligger under
  slike flater. Walk-sjekk 1 bekrefter at alle 18 treffer gulvet.
- Fysikkbiter (`PhysicsBody`) og regn påvirkes ikke.
- Merk: «ugangbar» hindrer ikke at en kropp blir liggende på en flat topp i fallmodus. Det betyr ingenting her, fordi ingen
  kan nå dit.

**4. Glassmaterialet (`_glass_master`, VERSION 10).**
- Det er billig nok:
  - `TLM_SURFACE` (lys fra translucensvolumet) i stedet for `TLM_SURFACE_PER_PIXEL_LIGHTING`.
  - Ingen refraksjon.
  - SSR, kontaktskygger og hastighetsutdata er av.
  - Fresnel og to lerp er bare noen få instruksjoner.
- Komponentene for fast glass har ingen skygge, ingen avstandsfelt og ingen strålesporing.
- Det blir 3 fastglass-mesher pluss 2 ISM-er for glassfelt, altså få tegnekall.
- Det ødelegger ikke annet glass:
  - Parameternavnene `Tint` og `Opacity` er de samme.
  - `MI_GS_Glass` og `MI_GS_GlassDark` bygges på nytt, fordi `build_masters()` gir `forced`.
  - Ingen kode setter glassparametere med MID (sjekket `GSDoor`, `GSMapState`, `GSBreakable` og `GSEffects`).
  - Dørglass, rekkverk og glasset i det gamle campuset blir litt klarere rett forfra (0,16 mot 0,28). Det er greit.
- Små ting (forslag 3):
  - `GlassDark` (tastaturskjermen på kodelåsen, linser) får grønnlig, lys `EdgeTint` på skrå, fordi den ikke overstyres.
  - `two_sided` på lukkede glassbokser gir 4 lag gjennomsiktighet i stedet for 2.

**5. `init_unreal.py` og `-game`.**
- `fix_distance_fields(imported)` står inne i `else`-grenen etter `_running_as_game()`, sammen med importen, så den er
  beskyttet.
- Den gjør bare noe med nyimporterte `SM_Bygg_Helgasetr_*`.
- `set_lod_build_settings` ligger i try/except med advarsel, slik planens risiko sa.

**6. Rendringene mot bildene (01, 06, 22, 26).**
- Det ligner i oppbygning:
  - stablede bokser med hvitt undertak
  - gule løp med hele vanger
  - mørke lameller på dekkekantene
  - søyler med to svarte ringer
  - ringlamper
  - fagverket under sagtaket
  - oransje mesanin
- Fargene ligger nær bildene (bildene er mer mettet oransje, Unreal-tinten 1,15/0,62/0,06 er nærmere enn Blender-forhåndsvisningen).
- Det som skiller mest fra bildene:
  - Rommene er tomme og hvite (møbler kommer i plan-05).
  - Hjørnene er rette.
  - Vaierne og hengestengene er tydelige i r04b-atriet-ovenfra.
  - Vangene på løpene ved mesaninen stikker opp som gule trekanter over gulvkanten (r04b-mesanin-mot-kantina, venstre).
    Det kommer av at de vippede vangeklossene er lengre enn løpet (forslag 2).
- r04b-p4-nord-mot-sor er tatt rett mot en vegg og viser lite.

## Avvik fra planen (godtatt, men skriv dem i retning/helgasetr.md)
- `SM_Mod_Helg_*` (glassfelt, glassdør, lettvegg, karm) er ikke laget, og ISM-ene bruker fortsatt `GSUtils::Cube()`.
  Interaktiv har bedt om `SM_Mod_Helg_GlassPanel` som rutemodell (bestilling l. 12, punkt 5). Lag den eller svar at kuben
  blir stående.
- Atriehjørnene er fortsatt rette. Grunnen står i kap. 14: id-ene ville flyttet seg. Det er godtatt, men IMPROVEMENTS har ikke
  planens linje 631, så ingen påstand er feil.
- Kulisseløpene går P5→P6, ikke P4→P5→P6.
- IMPROVEMENTS-numrene er 800–824 i stedet for 630–645. 630-tallene var brukt, og det er ingen kollisjon.
- «Ferdig når» i spillet er ikke oppfylt: modelldelene er ikke vist i loggen, og `stat unit`/`stat gpu` er ikke målt. Det er
  plan-05s første punkt (hodeløs import og rapport), så det godtas her.

## Funn

**1. Modelldelenes kollisjon er ikke sikret etter importen.**
- Filer: `Content/Python/init_unreal.py` (`import_models`, `fix_building_collision`) og `GSHelgasetrBuilder.cpp`
  `BuildModelParts`.
- Når spillet går, kan det ikke rette dette selv. En feil her betyr at man faller gjennom gulvet eller går gjennom vegger
  uten at reserven slår inn.
- (a) Nanite brukes sammen med kompleks kollisjon.
  - I UE5 lages den komplekse kollisjonen til en Nanite-mesh fra *fallback-meshen*. Den er forenklet etter
    `fallback_relative_error`, og standardverdien er ikke 0.
  - På en del på 26 m kan det flytte hjørner på trinn (18 cm), karmer og 6 cm kapper.
  - Walk-sjekken prøver `.blend`-filene, ikke den importerte kollisjonen.
  - Gjør: sett `nanite_settings.fallback_relative_error = 0` (eller 100 % trekanter) for `SM_Bygg_Helgasetr_*` ved import.
    Delene er lavpoly (16–35 kB FBX), så det koster nesten ingenting. Eller ta Nanite av for delene med kollisjon.
- (b) Kollisjonsfiksen kjører bare én gang.
  - `fix_building_collision` kjører bare på navnene som ble importert i denne oppstarten.
  - `.imported`-stempelet skrives for alle oppgaver før fiksen, også om fiksen kaster en feil.
  - Feiler fiksen én gang, får delen aldri `CTF_USE_COMPLEX_AS_SIMPLE`. Uten auto-kollisjon har den da ingen enkel kollisjon,
    og kropper faller gjennom.
  - Gjør: kjør fiksen hver oppstart på alle `_is_building`-mesher der flagget ikke er satt. Det er billig og idempotent.
- (c) I C++ bør `BuildModelParts` bruke klossene også når en del med `Full` eller `Above` mangler `BodySetup` eller har feil
  `CollisionTraceFlag`. Logg det som reserve.
- Ta (a) og (b) inn i plan-05 steg 2. Import-rapporten bør vise fallback-feilen og kollisjonsflagget per del.

**2. Treffflater på de sammenslåtte modelldelene blir feil.**
- Kulehull er fra Interaktiv 728/729, nedslagslyd fra Kamp 673 og fottrinn fra Kamp 671.
- `GSBreakable` (merking) og `GSAudio::SurfaceAt` leser først `ComponentTags`, så `Hit.PhysMaterial` og til slutt
  `Comp->GetMaterial(0)`.
- Med klossene var hver ISM ett materiale, så `GetMaterial(0)` stemte.
- En modelldel som `SM_Bygg_Helgasetr_P4_V_MN` har parkett, gips, betong og metall i én komponent uten tagger. Da gir
  `GetMaterial(0)` samme flate for hele delen: kulehull i gips på betongsøyler og samme fottrinn overalt.
- `PM_GS_*` finnes ikke ennå. `gs_materials.py` lager dem ikke.
- Gjør, før eller sammen med at modellen tas i bruk (plan-05):
  - Lag `PM_GS_Gips/Betong/Tre/Stal/Stein/Stoff/Glass` (+ `Lino`, ønsket i bestillingen l. 6) i `gs_materials.py`.
  - Sett dem på `MI_GS_*` og `MI_GS_*_In`.
  - Med kompleks kollisjon gir trimesh-en riktig `PhysMaterial` per trekant når sporet ber om det
    (`bReturnPhysicalMaterial`, som Kamp allerede setter).
  - Ta med en logglinje som teller treff per `PM` i den hodeløse prøven.

**3. Commiten har med andre lags filer.**
- Interaktivs `SM_Int_*` (dører, håndtak, kodelås, skår, glassbiter: 22 filer) er med.
- Interaktivs IMPROVEMENTS-seksjon «Runde 1b» (718–733) er med, sammen med endringene i linjene 653–660.
- Koden de beskriver (`GSBreakable.cpp` m.fl.) er ikke commitet. Ved fbf7de7 påstår IMPROVEMENTS derfor ting som ikke finnes
  i treet.
- Ingenting går i stykker av det, men det blander eierskap og gjør det vanskelig å rulle tilbake én runde.
- Gjør: ikke rull tilbake. Hovedagenten sier fra til Interaktiv om at filene deres ligger i fbf7de7, og at koden må commites
  snart. Byggeren legger heretter bare til egne filer (`git add <sti>`, ikke `-A`).

## Forslag til neste runde
1. **Plan-05: bevis importen med funn 1 og 2 som krav.**
   - Fallback-feilen er 0 eller Nanite er av for kollisjonsdelene, og kollisjonsfiksen kjører hver oppstart.
   - `PM_GS_*` er satt på materialene.
   - Import-rapporten har per del: kollisjonsflagg, Nanite-fallback, materialspor og `PM`.
   - En hodeløs sjekk med en kapsel langs alle trinn og gjennom alle dører i *Unreal* (ikke bare Blender).
2. **Vangene på atrietrappene.**
   - De vippede vangeklossene (`add_rot_box … pitch=ang`, lengde `hypot(L, dz)`) stikker opp som trekanter over
     mesaninkanten.
   - Klipp dem mot gulvet i toppen og bunnen, eller lag vangene i `building_helgasetr.py` som en firkant med skrå
     over- og underkant.
   - Gjør det samme med kulisseløpene.
3. **Glass og lys:**
   - Gi `MI_GS_GlassDark` en mørk `EdgeTint`.
   - Vurder `two_sided = False` for de lukkede glassboksene (halvparten av lagene).
   - Sett ringlampenes demping per lampe til ca. `L.Z + 1 m`. På 13 m får kantinegulvet under lampene på 12–12,4 m bare ca.
     3 % av lyset, mens rom i 2. etasje innen 13 m fortsatt får lys gjennom dekket.

## Lærdom
- En reserve som slår inn når «ressursen mangler», må også slå inn når ressursen finnes, men er feil (kollisjon, materialer).
  Sjekk egenskapen, ikke bare at fila finnes.
- Når mange flater slås sammen til én mesh, forsvinner «ett materiale per komponent». Alt som leser `GetMaterial(0)` eller
  `ComponentTags` for underlaget må få fysiske materialer per spor i samme runde.
- Nanite-mesher får kollisjon fra fallback-meshen. Gulv, trinn og karmer som skal gås på, trenger fallback-feil 0 eller ikke
  Nanite.
