**MÅ FIKSES**

# Kontroll av runde 2 (Grafikk og kart) – commit 1ee5e98

Kontrollert mot `ko/plan-02.md` og min forrige vurdering (`runde-01.md`, K1–K7). Arbeidsmappen er lik commiten
for `GSCampusBuilder.*`, `GSCampus.h` og `Tools/`, så det jeg kjørte gjelder commiten. Build.bat er ikke kjørt
(editoren er åpen). Byggeren melder at C++ kompilerte, men ikke at det er lenket. Alle nye funksjoner i headeren
har en definisjon, og motorkallene finnes i 5.8 med `ENGINE_API`, så lenkefeil er lite sannsynlig. Et fullt bygg
må likevel gå gjennom før runden regnes som ferdig.

Det meste er godt gjort. Det er tre ting som må rettes, og alle er små. To av dem bryter det runden lover
(«ingen kan gå ut av kartet» og «du starter skjermet»).

## Hva jeg kjørte selv
- `python Tools/Blender/check_campus.py`: alt OK, kode 0 (10 bygg, speiling, stier til endedører, spawn bak
  basene, med motprøver).
- `blender --background --python Tools/Blender/check_walkable.py`: alt OK, kode 0. Dette er med: dører med
  motprøve 1 m utenfor karmen, at tårnet er borte (med motprøve mot taket), frie ganger i Lysgården, sokkeltopp
  på z = 0,500, skjermvegger og fri passasje ved x ±5, og rampetrinn (0,95 m gangbar mot krav 0,65 m, med motprøve).
- `python -m py_compile Content/Python/gs_materials.py`: OK.
- Motor-API sjekket i headerne for UE 5.8:
  - `UStaticMeshComponent::SetWorldPositionOffsetDisableDistance(int32)` finnes (`StaticMeshComponent.h:489`).
  - `EShadowCacheInvalidationBehavior::Rigid` finnes (`SceneTypes.h:222`). Den slår av at WPO ødelegger skyggelageret
    (`PrimitiveSceneProxy.cpp:823`). Løvmaterialet bruker faktisk WPO (`gs_materials.py:338`), så grepet virker.
  - `AddInstances(const TArray<FTransform>&, bool bShouldReturnIndices, bool bWorldSpace, bool bUpdateNavigation)`
    finnes i både ISM og HISM.
  - Import med `auto_generate_collision` gir et 18-DOP-skall (`FbxStaticMeshImport.cpp:2434`). Det har betydning
    for gjerdet (se under).
- Egne regnestykker (Python): siktlinjer fra rutene til hvert spawnpunkt, mantle mot gjerdet og gripekrok-banen
  mot den usynlige veggen.

## «Ferdig når», punkt for punkt
| Krav | Resultat |
|---|---|
| Build.bat går gjennom | **Ikke vist.** C++ er kompilert, men ikke lenket. Må kjøres når editoren er lukket. |
| Sjekkskriptene sier OK, med motprøver | Ja (kjørt selv) |
| Sola roteres bare når `SunHours` endres | Ja (`GSEnvironment.cpp:210–218`) |
| Tre-gruppene har WPO-avstand og `Rigid` | Ja (`AddModel`, før `RegisterComponent`) |
| Lave klosser ligger i grupper uten skygge | Ja (`IsLowBox`, egne `_L`-nøkler) |
| Ingen `AddInstance` én og én | Ja. Eneste kall er `AddInstances` i `FlushPending`. Ingenting leser instanser før tømmingen. |
| `GSCampus.h`: ingen tårn, Kjemi 46 m med Yaw 180, stubbene på 11,6 og 8,4, 14 nye spawnpunkter | Ja |
| Ingen `Robot` i `GSCampusBuilder.cpp` | Ja |
| `BuildPerimeter`: gjerde rundt, 5 sperringer, 4 vegger, 2 riggområder, 4 gravefelt, 4 biler | Ja |
| Linjene (−125,−30)→(125,−30) og (−160,120)→(160,120) brytes | Ja (gravefeltene på x ±96 og ±100) |
| **Ingen spawnpunkter ses fra nordstien** | **Nei.** Alle sju i hver base ses gjennom 4 m-åpningen (M2). |
| `gs_materials.py`: `Weather`, `VERSION = 9`, `_In`, `INDOOR_MODELS` | Ja. Alle `_In`-plasser i Blender-skriptene har en `Surface`-oppføring. |
| Bildene finnes; README-linja rettet | Ja. Men README linje 80 og 83 nevner fortsatt tårnet og robotene (M3). |

## Andreas' faste regel
- **Forbedret (gammelt):** ytelse (sol i trinn, skyggelager for trær, ingen skygge fra lave klosser, samlet
  utsetting, gatelys med tegneavstand), tårnet og robotene er borte, Lysgården er åpnet, basene er like og stiene
  treffer endedøra (K3, K5), spawn er flyttet bak basene (K4), kantstein i kryss (K2), sjekkskriptene er sterkere
  (K6), og `Ok`-testen i `BuildCampusProps` er rettet.
- **Nytt:** gjerde rundt hele banen, sperrede veier, seks nye modeller, gravefelt og parkerte biler, riggområder,
  møblert bakgård og innendørsmaterialer uten vær.

Begge deler er oppfylt.

## Det som er sjekket og funnet i orden
- **Skygger:** bare klosser med topp under 0,3 m og uten vipping mistet skyggen (bakken, veidekket, oppmerking,
  fortau 0,12, kantstein 0,15 og gulvplatene i 1. etasje i klossbyggene). Vegger, gjerder, containere, biler
  (`AddModel`) og møbler kaster skygge som før. Bakken kaster ikke lenger skygge, og det kan gi lys nedenfra når sola
  står like under horisonten. Det skjer ikke her: sola er `AtmosphereSunLight`, så atmosfæren slukker lyset under
  horisonten.
- **Gjerdet er tett:** feltene strekkes til nøyaktig lengde (`N = ceil(L/3,5)`, skalert i Y), og hjørnene møtes i
  samme punkt. Forsiden peker innover på alle fire sidene (regnet ut: NE→NW gir sør, NW→SW gir øst osv.). Den
  usynlige veggen dekker hjørnene (lengde + 0,3).
- **Klatring over gjerdet:** `TryMantle` krever 0,55–2,2 m og plass til å stå (`ECC_Pawn`). Gjerdeskallet (18-DOP)
  er en 0,6 m tykk plate som smalner til en rygg på 4 cm i 2,0 m høyde. Kommer man skrått inn, kan man klatre opp
  og bli stående på ryggen av ytterkantgjerdet. Bak gjerdet er det bare 0,35 m til veggen, og kapselen er 0,8 m
  bred, så man kommer ikke lenger. Gravefelt og riggområder kan man klatre inn i og ut av, og det er greit.
- **Determinisme:** `FenceSegs` og `Reserved` fylles i `BuildPerimeter` før trær og småting trekkes. De er
  uavhengige av om modellene er importert. Alle frø er faste, og `TMap Pending` itereres i innsettingsrekkefølge.
  Vert og klient bygger altså likt.
- **Yaw 180:** `BuildBuilding`, `FurnishBuilding`, `Keep` og `BuildCampusProps` går via `BT`. `Ok`-lambdaen regner
  nå i meter (cm/m-feilen er rettet). Skjermveggene ligger på lokal y ±6, så de speiles riktig. Endedøra i øst ligger
  på y 8,36, og stubben på 8,4.
- **Flagget fra nord- og sørstien:** en linje gjennom døra (|x| ≤ 1,5 ved y ±11) treffer y ±6 innenfor |x| ≤ 0,82.
  Veggen er 6 m bred og går til taket. Flagget kan ikke ses gjennom nord- eller sørdøra.
- **Containere, sperringer, bakgård og Lysgården** ligger der planen sier, og de holdes unna av `Reserved` og
  `NearPerimeter`. Sola i trinn er lik på vert og klient (timen kommer fra GameState).
- **Søylen midt i aksen innenfor hoveddøra** (byggerens merknad) er arkadesøylen på (0, −9,45) bak sokkelen. På hver
  side er det 3,4 m åpning, og strålene i x 2,5 går fri. Den er ekte arkitektur, ikke en feil. La den stå.

## Funn

### Må fikses (små)

**M1 – HØY – Man kan komme ut av kartet med gripekroken.** `GSCampusBuilder.cpp` `BuildPerimeter` punkt 3:
`BlockH = 14.f`. Gripekroken (`GSCharacter.cpp:2356`) sporer på `ECC_WorldStatic`, og den treffer den usynlige
veggen. Spilleren skytes da mot punktet med inntil 26 m/s pluss 4,5 m/s ekstra løft. Jeg regnet banen: sikter man på
veggen 13,8–14 m opp fra 7–11 m avstand, når kapselens midtpunkt veggen i ca. 14,7 m høyde. Bunnen er da ca. 13,8 m,
altså rett på toppkanten, og man glir over eller blir stående oppå veggen. Det samme kan skje i sørøst: Elektrobygget
står bare 10 m utenfor veggen og er 20 m høyt. Står man ca. 21 m inne på banen, kan man sikte på det, og banen passerer
veggen like under 14 m (over den hvis man hopper først). En flaggbærer utenfor kartet ødelegger runden.
**Rett:** `BlockH` til 100 m. Veggen er usynlig og har ingen skygge, så det koster ingenting. En gripekrok-bane
topper på ca. 45 m selv fra bakken. Rett samtidig kommentaren og IMPROVEMENTS 421.

**M2 – MIDDELS – Spawnpunktene ses fra nordstien og sørruta gjennom 4 m-åpningen i riggområdet.** Dette er et
«Ferdig når»-krav («ingen spawnpunkter ses fra nordstien»), og IMPROVEMENTS 424 lover det. Planen (F5) lot det stå
en 4 m åpning mellom presenningsgjerdet og bygget, ved x −152…−148 på y −3 og y 23. Jeg regnet 2D-siktlinjer med
alle bygg som hindring og gjerdene som tette:
- Fra nordstien (y = −30) ses alle sju vestlige spawnpunkter. For eksempel ses (−155, 10) fra x −145…−130 på
  44–50 m, og (−161, 3) fra x −111…−53 på 60–110 m.
- Fra sør (y = 40…120) ses de også, fra ca. 36 m og ut til 300 m. (−161, 17) ses fra nesten hele sørpromenaden.
- Øst er speilbildet, med det samme problemet.

Trær og tilfeldige ting kan dekke noe, men det er tilfeldig.
**Rett:** gjør åpningene til en sluse. Legg et kort presenningsgjerde utenfor hver åpning, parallelt med det andre
og 2–2,5 m ut (f.eks. vest: `FenceRun((−157, −6), (−145, −6))` og `FenceRun((−157, 26), (−145, 26))`, speilet i
øst), eller la åpningen vende langs bygget. Bekreft med en stråle-/linjesjekk (se forslag 1). Inngangen fra spawn til
basen er uansett vestdøra, og man kan klatre over gjerdet.

**M3 – LAV – README sier fortsatt at tårnet og robotene finnes.** `README.md` linje 80: «og tårnet med spiralrampe
helt opp til klokketårnet». Linje 83: «… og roboter (robothund, vaktrobot, leveringsrobot)». Fjern begge, og nevn
heller byggegjerdene, gravefeltene og at banen er inngjerdet.

### Bør tas i neste runde

**N1 – MIDDELS – Nord- og sørruta er fortsatt åpne skytebaner 2–3 m ved siden av stien.** Gravefeltene (y −33…−28)
og bilene (y −31,9…−28,3) dekker bare selve stien. Linja y ≈ −26,5 fra x −125 til 125 har ikke noe som stopper sikt.
Trær og plenmøbler holdes minst 5 m fra stimidten, og busker står ofte ≥ 1 m utenfor stien uten kollisjon. Det er
akkurat der man går rundt gravefeltet. Sørpromenaden er lik: y ≈ 124,5 er fri. IMPROVEMENTS 425 lover mer enn det
som er gjort.
**Forslag:** lag dekningen forskjøvet. Gravefeltet i vest kan gå ut på plenen i sør, og det i øst på plenen i nord
(eller legg en container eller brakkerigg på skrå på plenen ved x ±60). Legg en siktlinjesjekk i `check_campus.py`
(stråler fra den ene basens dør til den andres, også 2–4 m til siden for stien).

**N2 – LAV – Flagget ses fra stien foran den indre endedøra.** Den øst-vestgående korridoren går rett fra endedøra
(y 11,64) til flagget. Fra stubben (−78…−100, 11,6) ses flagget gjennom døra på 24–47 m (en smal kjegle).
IMPROVEMENTS 426 («kan ikke lenger beskytes utenfra») er for bastant. Tverrveggene i plan-03 (x ±7) kan løse det.
Sjekk det da.

**N3 – LAV – Banen er fortsatt ikke lik for begge lag rundt sørruta.** Kjelhuset (−62, 105) har ingen motpart i øst.
Speilet om (0, 10) ville den stått på (62, −85). Vest får et bygg man kan gå inn i (med loot) ved sin sørrute, øst får
ikke det. `check_campus.py` d) sjekker bare basene.
**Forslag:** dekning eller et lite bygg i øst ved (62…78, 95…115), eller speilvendt plassering. Utvid d) til alle bygg
man kan gå inn i, og til containere og gravefelt.

**N4 – LAV – Kuler treffer den usynlige veggen i lufta.** `PerimeterBlock` er `BlockAll`, og kulene sporer på
`ECC_Visibility` (`GSBallistics.cpp:319`). Skudd over gjerdet gir da trefflyd, gnister og kulehull i løse lufta, 0,35 m
bak gjerdet. Det bryter med realismen.
**Forslag:** egen kollisjonsprofil for veggen, med Pawn, PhysicsBody, WorldDynamic og WorldStatic som blokkerer (så
gripekroken stoppes) og Visibility som ignoreres. Kuler utenfor banen fjernes uansett av `GSBallistics` (område +
margin). Dødskameraet (byggerens spørsmål): Camera-kanalen kan godt fortsatt blokkere. Det trekker bare kameraet inn
ved kanten, så man ikke ser ut av banen, og det er ønsket.

**N5 – LAV – Sola i trinn kan synes ved lav sol.** Ett trinn er 0,25° hvert 2. sekund. Ved 10° solhøyde er skyggen
av Hovedbygningen (22 m) ca. 125 m lang, og spissen hopper ca. 2,7 m (høyde) og 0,5 m (side) hvert 2. sekund. Ved
25° er hoppet ca. 0,4 m, midt på dagen 0,1 m. Hvert trinn tegner alle de virtuelle skyggesidene på nytt, så det kan
komme en liten topp i `stat unit` hvert 2. sekund. Andreas bør se etter begge deler når han måler. Synes det: 1/240
time (hvert 0,5 s) beholder nesten hele gevinsten.

**N6 – LAV – 18-DOP-skallet på det åpne byggegjerdet er 0,6 m tykt og massivt.** Kuler og spillere stopper 30 cm
foran nettingen på begge sider. Ved kanten er det greit, slik planen sier. Brukes `SiteFence` (åpent) senere inne på
banen, trenger den egen tynn kollisjon (UCX-boks 0,06 m), ellers føles det som en usynlig vegg.

## Byggerens merknader, kort
- `Ok`-testen (cm mot m): rettet riktig.
- Usynlig vegg som `BlockAll`: dødskameraet er ufarlig og bra. Kulene se N4. Høyden se **M1**.
- README 80/83: se **M3**.
- Søylen i aksen: arkitektur, la den stå.
- Sola i trinn: se N5. Det må ses i editoren.

## Forslag til neste runde
1. **En siktlinjesjekk i `check_campus.py`:** 2D-linjer fra rutene (nordsti, sørpromenade, passasjene) til hvert
   spawnpunkt og flagg, og base mot base 0–4 m til siden for stiene. Bygg, containere, presenningsgjerder og biler
   regnes som tette. Med motprøve: åpningen slik den er nå skal gi FEIL. Da kan M2 og N1 ikke komme tilbake.
2. **N1 og N3 sammen:** forskjøvet dekning langs nord- og sørruta, og en motpart til Kjelhuset i øst, så begge lag
   har like ruter.
3. **Plan-03 som planlagt** (flaggrommene, tverrveggene, K7). Sjekk N2 mot tverrveggene.

## Lærdom
- Åpninger i skjermende gjerder må regnes som siktlinjer fra rutene. Et punkt «bak bygget» er ikke det samme som
  skjult.
- Usynlige grensevegger må være høyere enn noe spilleren kan nå med gadgets (gripekrok, fallskjerm, hopp fra tak).
  Regn ut banen.
- Dekning som bare ligger på stien bryter én linje. Sjekk også parallelle linjer 2–3 m til siden, der spillerne
  faktisk går.
