# Forhåndssjekk plan 04 (Grafikk og kart) – Helgasetr råbygg

**BØR ENDRES**

Grunnlaget er godt: aksene, kotene og ringkorridoren stemmer med tegningene. Jeg har regnet aksesummene (51,50 / 67,77)
og etasjehøydene (5,6 / 4,3 / 4,0 / 4,3 / 4,0 / 4,0), og målt ringen på Plan 4 (s. 6, 2018). Ringen har 2,25–2,3 m fri
bredde, og atriekanten står på |X| 13,2 og |Y| 17,8. Trappeløpene går opp i tallene: løp 1 31 × 0,18 = 5,58, løp 2
24 × 0,18 = 4,32, løp 3 22 × 0,18 = 3,96, og inntrinn pluss repos får plass i lengdene.

Planen har likevel én ekte spillfeil mot koden (spawn), to plasseringsfeil i fasiten som stenger korridorer, en
sjekk som ikke kan bestås, hull i sikringen mot gripekroken og et åpent atrium uten dekning. Runden er dessuten for
stor. Alt kan rettes i planen før det bygges.

## A. Feil som må rettes

1. **Spawn havner på flagget (GameMode-sporet starter inne i P5-dekket).** `AGSGameMode::PickSpawnTransform` sporer fra
   `Base.Z + 400 cm` og ned. På Plan 4 blir starten 13,9 + 4,0 = 17,90 m. P5-dekket ligger fra 17,85 til 18,20, så
   starten er inne i dekket. Kompleks kollisjon er tosidig i spørringer, så sporet treffer undersiden (17,85) og legger
   spilleren 1 m over. Der overlapper kapselen dekket, så alle punkter avvises. Da faller koden tilbake til
   `Base + 120 cm`, og hele laget spawner oppå eget flagg.
   - **Endring:** bestill hos Spillbarhet at sporet starter fra `Base.Z + 200 cm` (flaggsporet fra +300 cm er trygt).
     Til den er levert kan Grafikk sette `FBase.Flag.Z = 13.4` (0,5 m under gulvet) i `Bases()`. Da starter sporene på
     17,4 og 16,4 m, under dekket, og reserven havner på 14,6 m. Skriv dette i kommentaren.
2. **Walk-sjekken kan ikke bestås.** Kravet «stråle opp fra Z+0,2 til Z+4,0 treffer ingenting» ser bare 3,95 m fri
   høyde på P4 og 3,65 m på P3. Strålen treffer alltid dekket over.
   - **Endring:** sjekken skal etterligne GameMode nøyaktig. Spawnsporet går fra Z+4,0 (eller +2,0 etter bestillingen)
     ned og skal treffe gulvet. Flaggsporet går fra Z+3,0 ned. I tillegg sjekkes at ingenting med kollisjon ligger
     mellom gulv og Z+2,2 (himlinger og lamper uten kollisjon).
3. **Heisene står feil i helgasetr.md kap. 2 og vil stenge ringen.**
   - Heis sør (X −11,0…−8,6, Y +18,5…+21,0) stikker 1,5 m inn i den 2,3 m brede sørkorridoren og dekker
     lobbyåpningen (−10,2, +20,3).
   - Heis øst (X +13,6…+15,6) står midt i østkorridoren (R3).
   - Fra tegningen: heis sør X −10,2…−6,6, Y +20,4…+23,8 (østre halvdel av lobbyen). Heis øst X +15,6…+18,2,
     Y −11,9…−9,9.
   - Tegningen viser også at Trapp sør begynner ved Y ≈ +26,9, med WC-blokk mellom. Lobbyen er X −13,5…−10,2, med
     3,2 m åpning mot ringen ved X −13,4…−10,2. Lobbyen ved Trapp øst er X +15,6…+18,5, Y −9,9…−7,5, og trapperommet
     starter ved X +18,5.
   - Rett fasiten og planens tabeller. Flytt døra fra lobbyen til Trapp sør bort fra heisen.
4. **`FloorRects` = FOOTPRINT − VOIDS mangler boksegulv og repos.** Glassboksene, fellessonene i atriet og reposet på
   løp 2/3 (P3 øst, X +7,5…+13,25, Y +6…+9) står inne i hullet |X| < 13,25. Lootpunktene i glassboksene (1e) vil da
   feile sjekk f.
   - **Endring:** `FloorRects` = FOOTPRINT − VOIDS + BOXES-gulv + repos.
5. **Rekkverkssjekken går bare rundt den faste ringkanten.** Åpne fellessoner på P3/P4 (nord/sør/øst) stikker ut i
   atriet og har egne kanter.
   - **Endring:** beregn rekkverk og stråler fra hele grensen mellom gulv og hull på hvert plan (polygonkanten av
     `FloorRects` mot VOIDS), ikke fra |X| 13,25 og |Y| 17,75.

## B. CTF-spillbarhet

6. **Siktlinje fra base til base gjennom atriet.**
   - Fordi basedørene er punktsymmetriske, går linja gjennom et dørpar alltid gjennom origo. Fra (3,84, −26) inne i
     Nordbasen ser man gjennom døra (+3, −20,3), over atriet og gjennom døra (−3, +20,3) til (−3,84, +26) i Sørbasen:
     52,6 m.
   - Linja går gjennom boksglass, som ikke stopper `Visibility`. Den passerer altså fritt, og sjekk e) ser den ikke,
     fordi den bare sjekker i gangretningen langs rutene.
   - **Endring:** ny sjekk e2): ingen fri linje > 42 m fra et punkt i én base (flagget ± 2 m og dørene) til den andre
     basen.
   - Rett det med (a) en kort skjerm/innervegg 1,5–2 m innenfor hver korridordør (L-inngang), og/eller (b) at boksenes
     side mot korridoren og skilleveggene mellom boksene er lettvegg (ugjennomsiktig, «whiteboard på lettveggen», kap. 4).
7. **Atriet er én skytebane.**
   - Glass skal slippe kuler gjennom (riktig for runde 5), men da gir glass ingen dekning.
   - R1 og R3 er 40 m lange og 2,3 m brede, og hele lengden ses fra motsatt korridor (26,5 m), fra P3 og fra trappene.
   - Skaprekkene står ved ytterveggen. Det er feil side for dekning mot atriet, og søyler på Ø 0,5 m hver 6. m er lite.
   - **Endring:** krav i sjekk e3). Langs R1/R3-midtlinja skal det minst hver 8. m være en «lomme» (≥ 1,5 m) uten sikt
     til motsatt ringside og til P3-ringen. Bruk ugjennomsiktige ekte ting: lettvegger mellom bokser, boksenes hvite
     undertak og rammer, trappevanger, og sofagrupper og skap ved knekkene på atriesiden.
   - Vurder også å bygge de gule løpene P4→P5→P6 som krysser atriet (bilde 01 og 06) som kulisse. De ligner mer på
     bygget og bryter sikt.
8. **Rød spawn ser rett ut i korridoren.**
   - Linja (−7,2, +18,9) → lobbyåpningen (−10,2, +20,3) → spawngangen → S1-døra (−15,4, +23,0) → spawnpunktet
     (−18, +24,4) er fri.
   - Enhver dør i vestveggen til lobbyen/spawngangen kan ses diagonalt fra korridoren. Sjekk c) vil feile med tallene
     i planen.
   - **Endring:** spawndørene skal ligge i en sluse med L-sving (lærdom runde 1–2), eller spawngangen stenges mot
     lobbyen og får egen utgang via venterommet.
9. **Skjevheten ved Trapp sør er verre enn «rutelengde».**
   - Blått kommer opp Trapp sør inn i røds lobby, 8 m fra rødt flagg, gjennom sidedøra (−6,77, +22). Rødts
     spawnutgang går gjennom den samme lobbyen.
   - Nordsiden har ingen trapp i lobbyen. Blått får dermed en skjult flankerute og en spawnfelle, og sjekk d)
     (±20 % korteste vei) fanger det ikke.
   - **Endring:** sjekk d2):
     - Avstanden fra hver trapperomsutgang på P4 til fiendens flagg skal være lik for begge lag (±20 %).
     - Ingen fiendtlig trapperomsutgang skal ha sikt til, eller være nærmere enn 12 m fra, egen spawnutgang.
     - Løs det med punkt 8 (rød spawn ut et annet sted). Fjern i tillegg sidedøra fra Sør-lobbyen til Sørbasen og
       motstykket i nord, eller la Trapp sør stoppe på P3 hvis tallene krever det.

## C. Fall, fastsetting og gripekrok

10. **Gripekroken (35 m, `ECC_WorldStatic`, kaster deg opp på målet) når P5/P6, boksetakene øverst og takfagverket.**
    Planen regner P5/P6 som utilgjengelige.
    - **Endring:** alt over P4 inne i atriet (P5/P6-bokser, fagverk, hengestenger og øvre trappeløp) blokkerer
      `Visibility`, men ignorerer `Pawn` og `WorldStatic`-spørringer.
    - P5/P6-kanten mot atriet får et usynlig hinder i full høyde, som blokkerer `Pawn` og `WorldStatic`.
    - Glasstaket beholder BlockAll.
    - Ta med i walk-sjekken: ingen flate over P4 man kan stå på og nå fra atriet.
11. **Lave rom under trappene.** Under løp 2 ved bunnen (mesaninen, X −1…+3) og løp 3 (P3-fellessonen øst) er det under
    2,0 m fri høyde. Sittetrappa (X +5…+12, Y +6,5…+9,5, topp 1,2) ligger under løp 2, så det blir bare ca. 1,6 m over
    øverste trinn. Kapselen er 1,84 m og kan bli sittende fast.
    - **Endring:** steng alle soner under trapper med < 2,0 m fri høyde med et massivt volum (vange/benk/plantekasse).
      Ingen sprekk på 0,3–0,9 m mellom trappeløp og dekkekant (løp 1 står 0,35 m fra kanten).
      Sjekk: kapselprøve (r 0,42, h 1,84) langs alle gulv.
12. Å falle ned i atriet er greit: GSCharacter har ingen fallskade, P1 henger sammen med trappene, og rekkverket på 1,1 m
    er høyere enn hopphøyden (≈ 0,94 m). Mantling over rekkverket feiler fordi toppsporet 30 cm forbi glasset bare
    treffer luft. Det må fortsatt holde når møbler kommer inntil rekkverket (runde 6). Skriv det som regel i
    helgasetr.md.

## D. Teknikk

13. **Reserven er for tynn til å spille på.** Med bare gulvbokser blir det ingen vegger, trapper eller trapperom. Det er
    det Andreas ser første gang han trykker Play, før editoren har importert modellen (importen skjer bare ved oppstart
    av editoren; i `-game` hoppes den over).
    - **Endring:** reserven bygges av de samme dataene som klosser: vegger fra `ROOMS` med døråpninger, dekker fra
      `FloorRects`, trinn som én boks per trinn og trapperommene.
    - Velg reserve **per modellbit**, ikke bare når `P4_NV` mangler. En delvis import skal ikke gi hull.
    - Da virker Play med en gang, uten at Andreas må gjøre noe.
14. **ID-ene til de interaktive delene.**
    - Glassfelt og lettvegger skal ha egne ISM-er som ikke går gjennom `GetISM`. `GetISM` deler komponent på
      nøkkelen mesh + farge, så reserveboksene kan blande seg med andre klosser, og da stemmer ikke indeks = id.
    - Lag i tillegg en tabell fra id til (komponent, indeks) i byggeren.
    - Fjern aldri instanser (runde 5 skalerer til 0).
15. Glassfeltene skal ha **enkel bokskollisjon ≥ 5 cm tykk**, ikke «complex as simple» på et tynt panel. Gripekroken
    kaster spilleren med opptil 2 600 cm/s, og kompleks kollisjon på tynne flater kan slippe kapselen gjennom. Ta
    derfor `SM_Mod_Helg_` ut av `COMPLEX_COLLISION_PREFIXES`; bare `SM_Glass_Helgasetr` skal inn.
16. **`GSHelgasetr.gen.inl` tas med i alle filer som leser `GSCampus.h`.** Generatoren må skrive `inline constexpr`
    tabeller eller funksjoner med `static`, aldri vanlige globale variabler (ODR og unity-bygg, jf. lærdom Spillbarhet
    runde 2).
17. **Lumen-oppdelingen holder ikke planens egen regel.** Kvadrantene er 25,75 × 35,66 m og bryter «ingen bit over ca.
    26 m». Del også ved |Y| = 18 (6 biter per plan).
    - `distance_field_resolution_scale` kan settes fra Python via
      `StaticMeshEditorSubsystem.get_lod_build_settings` / `set_lod_build_settings`. Gjør det, ikke dropp det.
18. Eksporten fra Blender skal bare skrive en FBX når innholdet er endret. Ellers importeres alle ca. 30 bitene på nytt
    (Nanite) hver gang editoren åpnes.

## E. Ytelse (RTX 3050)

19. **Glass.**
    - Ett billig glassmateriale: ingen refraksjon, ingen Lumen-refleksjon på gjennomsiktig, enkel lysmodus for
      gjennomsiktighet.
    - På alle glasskomponenter: `CastShadow` = false, `bAffectDistanceFieldLighting` = false og
      `bVisibleInRayTracing` = false.
    - P5/P6-glasset teller med i budsjettet på ca. 1 200 instanser. Bruk bare rekkverk der, ikke boksvegger på begge
      sider.
20. **Lys.** Punktlys uten skygge lyser gjennom vegger inn i naborom og etasjen over. Hold dempingen ≤ 600 cm i
    korridorene og hold lysene unna basevegger. Mål `stat unit` / `stat gpu` slik kap. 9 sier, og skriv det inn i
    «Ferdig når» (Andreas eller hovedagenten).

## F. Interaktive deler

21. `COVER` (skap, båser, plantekasser, salatbar, disk) skal **ikke** inn i strukturmodellen (steg 5). Det er nettopp
    de tingene som skal åpnes, veltes og flyttes i runde 5–7. Legg dem ut som egne ISM-klosser fra C++ med id, på samme
    måte som glassfeltene. Søyler, trappevanger og sittetrappa kan bli i modellen.
22. Resten er godt løst: glassfelt, dører (karm i modellen, `DOOR_SLOTS` med id), lettvegger og hengelamper ligger
    utenfor modellen, og fasade- og takglass er fast.

## G. Størrelse

23. **Runden er for stor** (9 nye filer, 6 endrede, hele bygget i Blender, to sjekkskript med motprøver, C++-bygger,
    60 loot, lys og lyd). Del den i to:
    - **4a – spillbart klossbygg fra data:**
      - `helgasetr_plan.py` (P1–P4 + kulissedekker), `gen_helgasetr.py`, `GSHelgasetr.h/.gen.inl` og bryteren i
        `GSCampus.h`.
      - `AGSHelgasetrBuilder` med full klossreserve (punkt 13), glassfelt og lettvegger som ISM, og
        `check_helgasetr.py` med e2/e3/d2.
      - Andreas kan trykke Play og spille samme dag.
    - **4b – Blender-modellen:** struktur P1–P6, trapper, tak og fagverk, fasadeglass, moduler, walk-sjekken,
      rendringer og `init_unreal.py` / `gs_materials.py`. Modellen erstatter klossene bit for bit.
    - Lys, kulisseblokker ute og lyd-/kamerasoner kan gå sist i 4b.
24. Andreas må få vite at runde 4 gir **CTF med bare startpistol**. Loot, PC-er og kameraer kommer først når
    Spillbarhet leser `LootSpots()`. Dirigenten bør legge den bestillingen rett etter 4a. Den mytiske kassa havner på
    (0, −7,5) i kantina (allerede bestilt).

## Liste til byggeren
1. `Bases()`: Flag.Z = 13,4 til Spillbarhet har flyttet spawnsporet til +2 m (bestilling). Walk-sjekken etterligner
   GameMode-sporene.
2. Heisene og lobbyene som på tegningen (punkt 3). Ingen heis i ringen.
3. `FloorRects` med boksegulv og repos. Rekkverk og rekkverkssjekk langs hele hullkanten.
4. Nye sjekker: e2 (base–base ≤ 42 m), e3 (dekningslommer hver 8. m på R1/R3), d2 (trapperomsutganger og spawnutganger),
   kapselprøve. Alle med motprøve.
5. Spawndører i sluse. Ingen spawnutgang i samme lobby som Trapp sør.
6. Tett soner under trapper med < 2,0 m fri høyde, og sprekker mellom trapp og dekke.
7. Atriet over P4: ignorer `Pawn` og `WorldStatic`, med hinder i full høyde på P5/P6-kanten (gripekrok).
8. Reserve = fullt klossbygg per manglende bit.
9. Glass og lettvegg i egne ISM-er og tabell fra id til instans. Glassfelt med bokskollisjon ≥ 5 cm.
   `SM_Mod_Helg_` ikke kompleks.
10. `gen.inl` bare med `inline constexpr`.
11. 6 biter per plan, DF-skala 2 via `StaticMeshEditorSubsystem`. FBX bare ved endring.
12. Glass uten skygge, DF eller ray tracing. Korridorlys ≤ 600 cm.
13. `COVER` som egne ISM-klosser, ikke i modellen.
14. Del runden i 4a/4b.
