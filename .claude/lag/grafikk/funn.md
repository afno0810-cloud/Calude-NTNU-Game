# Funn fra kontrollørens feiljakt (Grafikk og kart)

Nyeste øverst.

## Områder som er gjennomgått
- 2026-11-26 (commit 55ca90f): `GSCampusBuilder.cpp/.h`, `GSCampus.h` (kartdata), `GSEnvironment.cpp/.h`,
  `GSWorldSubsystem.cpp`, `Tools/Blender/buildings.py`, `building_hovedbygningen.py`, `meshkit.py`,
  `nature.py`, `Content/Python/init_unreal.py`, `gs_materials.py` (master-materialene og instanslisten).
  Bare skummet: `details.py`, `gen_textures.py`. **Ikke gjennomgått ennå:** `props.py`, `cars.py`,
  `backdrop.py`, teksturene i detalj, `FurnishRoom` rom for rom, ytelse målt i spillet.
- Metode: lest koden, regnet på målene, og (for F1, F2, F5) skutt stråler mot de eksporterte modellene
  `SourceArt/Models/SM_Hovedbygningen.blend` og `SM_Bygg_Kjelhuset.blend` i Blender (bare lesing).

## Funn 2026-11-26 (commit 55ca90f)

Alvorlighet: **HØY** = ødelegger spilling eller er det første man ser, **MIDDELS**, **LAV**.

### A. Ekte feil

**F1 – HØY – Alle dørene i Hovedbygningen er sperret av en 1 m høy steinsokkel.**
`Tools/Blender/building_hovedbygningen.py` linje 152: `ob(a0, a1, 0.0, 1.0, 0.25, STONE_D)` legger sokkelen
langs hele veggen uten hull for dørene (`buildings.py` linje 205–209 lager hull, denne gjør det ikke).
Bekreftet i den eksporterte modellen: stråle inn hoveddøra treffer ved y = -18,25 for z = 0,3 og 0,9, fri
først ved z = 1,1. Samme ved begge endedørene (x = ±55,25), sørdørene (x = ±48) og døra inn til tårnet
(y = 14,0). Modellen har presis kollisjon (`init_unreal.py` linje 20). Spilleren hopper 0,94 m
(`JumpZVelocity` 430), og klatringen (`TryMantle`) bommer rett forfra fordi den måler 30 cm bak kanten, og
sokkelen er 25 cm dyp. Man kommer altså ikke inn ved å gå, bare med flaks i et hopp.
Forslag: del sokkelen rundt dørene (som i `buildings.py`), kjør skriptet på nytt, og sjekk med en stråle.

**F2 – HØY – Rampene i alle de 15 vanlige byggene ender i en lomme: rekkverket sperrer utgangen.**
`Tools/Blender/buildings.py` linje 244–245: rekkverk på begge sider i hele rampens lengde, 0,86–0,91 m over
rampen. Toppen av rampen ligger mot ytterveggen, og eneste vei videre er sidelengs ut på gulvet (åpningen i
gulvrekkverket på 2,6 m i linje 247 viser at det var meningen). Der står ramperekkverket i veien. Stående
kapsel er 1,84 m, huket 1,16 m, så man kommer verken over eller under. Bekreftet i `SM_Bygg_Kjelhuset`:
stråle fra rampen mot gulvet treffer rekkverket ved y = 6,125 i alle høyder langs rampen.
Følge: 2. etasje og oppover (med loot og terminaler) nås bare ved å hoppe over rekkverket, ned samme vei.
Forslag: kutt rekkverket på gulvsiden de siste 2,6 m mot toppen (samme åpning som gulvrekkverket).

**F3 – HØY – Snø og regn legger seg innendørs.**
`Content/Python/gs_materials.py` linje 167–199 (`_surface_master`): snømasken er bare «flaten peker opp ×
SnowCover», og våtheten gjelder alle flater uansett. Gulv (Terrazzo, Floor, Marble), bord og hyller inne
bruker samme master, så når det snør blir gulvene i alle bygg hvite, og når det regner blir vegger og tak
inne mørke og blanke. Løvmaterialet (`_foliage_master`) har ingen snø, så trærne står sommergrønne i snøen.
Forslag: egen master (eller bryter-parameter `Weather` = 0) for innvendige materialer, og snø på løv/bar.

**F4 – MIDDELS – Sola står 90 grader feil.**
`GSEnvironment.cpp` linje 207: `Azimuth = Hours / 24 * 360 + 180`. Med kartets akser (`GSCampus.h` linje 9:
X = øst, Y = sør) lyser sola mot øst kl. 12 (står i vest), står i sør kl. 06 og i nord kl. 18.
Riktig: `Hours / 24 * 360 + 90` (øst om morgenen, sør midt på dagen, vest om kvelden).

**F5 – LAV – Dør midt i en skillevegg i Hovedbygningen.**
`building_hovedbygningen.py` linje 231 og 234: på østsiden havner korridordøra ved x = 26 nøyaktig foran
tverrveggen ved x = 26 (begge sider av korridoren, alle fire etasjer). Bekreftet: stråle gjennom døra
treffer veggenden ved y = 5,1. Vestsiden går klar med 10 cm. Forslag: flytt dørrekken eller veggen 1,5 m.

**F6 – MIDDELS – Hekker står foran dørene på kortsidene.**
`GSCampusBuilder.cpp` linje 562–579: hekker (med kollisjon, 1,2 m høye) settes tett langs begge ±X-vegger,
0,85 m fra veggen. Kommentaren sier at dørene ikke er der, men `BuildWallSide`/`wall_side` lager dør på alle
fire vegger. Eksempel Kjelhuset: døra ved lokal y = 1,0–2,8, hekk fra 0,45 til 4,35. Gjelder også
endedørene i Hovedbygningen. Forslag: hopp over hekken der døra er (samme formel som i `BuildWallSide`).

**F7 – MIDDELS – Tre gangveier går inn i bygg.**
`GSCampus.h` (veilisten) mot bygglisten:
- Midtaksen (x = -2,5..2,5) går gjennom vestenden av Stripa (x = 0..210, y = 296..314).
- Gangveien y = 260 (258..262) går 2 m inn i Varmeteknisk (y = 260..300, x = 335..405).
- Gangveien x = -200 (-202..-198) går 2 m inn i Idrettsbygget (x = -280..-200, y = 442..497).
Veien er stengt av veggen, og belegningen (topp z = 0,04) stikker opp over gulvet inne (0,02).
I tillegg stopper gangveien til hovedinngangen ved y = -24, mens trappeavsatsen starter ved y = -20,5
(3,5 m gress imellom). Forslag: flytt/kort inn veiene, eller del dem ved bygg.

**F8 – MIDDELS (ytelse) – Sola flyttes hver frame.**
`GSEnvironment.cpp` linje 208–209: `SetWorldRotation` på sol og måne hver frame (døgnet er 48 min, altså
0,125 grader i sekundet). Da må skyggekartene (Virtual Shadow Maps) tegnes helt på nytt hver frame for
hele campus. Sammen med sanntids himmelopptak (linje 90), volumetrisk tåke og skyer er dette tungt på
RTX 3050. Forslag: flytt sola i trinn (f.eks. hver gang den har gått 0,25 grader), og mål før/etter.

**F9 – MIDDELS – Alt småinventar forsvinner på 90 m, også lyktestolper og busskur.**
`GSCampusBuilder.cpp` linje 815: `SetCullDistances(0, 9000)` gjelder alle `AddProp`, også `StreetLamp`,
`BusStop`, `Dumpster`, `NTNUSign`. Lyset fra lykten og kollisjonen blir igjen, så på avstand ser man
lyskjegler uten stolpe, og kuler stopper i noe usynlig. Forslag: egen, lengre avstand for store ting.

**F10 – MIDDELS – Gatelysene lyser gjennom vegger.**
`GSCampusBuilder.cpp` linje 744–753: ca. 110 punktlys uten skygger (`SetCastShadows(false)`), rekkevidde
22 m, 0,8 m fra veikanten. Står de ved et bygg, lyser de opp rommene innenfor veggen. Avstanden mellom
lyktene er 55 m (linje 722), så det blir mørke hull på 11 m; ekte gatelys står 25–35 m fra hverandre.

**F11 – MIDDELS – Campus er bare lik hos alle hvis alle har de samme importerte modellene.**
`GSCampusBuilder.cpp` linje 313–372, 502, 593, 640–649: hva som bygges avhenger av hvilke filer som finnes
lokalt (`SM_Hovedbygningen`, `SM_Bygg_*`, trær, biler, Poly Haven-steiner i `/Game/PolyHaven`). `Content/`
ligger ikke i git. Mangler en modell hos én spiller, får den andre kollisjon enn serveren, og fordi
tilfeldig-rekken brukes ulikt i de to grenene (linje 511–533) flytter også biler og busker seg.
Poly Haven-steinene (`boulder_01`, `rock_07`) erstatter våre med annen form og kollisjon bare der de er
lastet ned. Forslag: logg en tydelig advarsel når en modell mangler, og la serveren sende en «modell-sum»
som klienten sjekker (bestilling til Spillbarhet hvis det må inn i GameState).

**F12 – LAV – `MainBuildingMesh()` husker forsøket for alltid.**
`GSCampusBuilder.cpp` linje 69–79: `static bool bTried` + svak peker. Blir modellen ryddet bort mellom to
kart (pakket spill), returnerer funksjonen null resten av økten, og Hovedbygningen blir klossversjonen
(også i `GSGameMode` sin loot-plassering). `ModelMesh`/`PropMesh` laster på nytt, denne gjør ikke det.

**F13 – LAV – Vær-tilstand som ikke er lik hos alle.**
`GSEnvironment.cpp` linje 255–256: `Wetness` og `SnowCover` telles opp lokalt fra 0. En som kobler seg på
midt i snøvær ser bar bakke i to minutter mens de andre ser hvitt. Vær og klokke selv er replikert riktig.
Linje 248–249: regn og snø skrus helt av når kamera er under tak, så man ser ikke regnet ut av vinduet;
står man ute, faller dråpene tvers gjennom tak og inn i rommene (ingen sjekk per dråpe).

**F14 – LAV – Ting plasseres oppå hverandre.**
`GSCampusBuilder.cpp` linje 507–559 og 1218–1241: trær, steiner, blomsterbed og plenmøbler sjekkes bare mot
bygg og veier, aldri mot hverandre (og plenmøblene bruker en annen tilfeldig-rekke). Trær kan stå i
hverandre og piknikbord inne i stammer. Usynlig stammekollisjon er 0,35 m × skala for alle tresorter
(linje 518), men bjørka er 0,2 m og den lille grana 0,14 m: kuler stopper i luft ved siden av stammen.

**F15 – LAV – Lyktestolpene mangler på dedikert server.**
`GSCampusBuilder.cpp` linje 171–174: `BuildStreetLights` hoppes over på dedikert server, men stolpene har
kollisjon (linje 734). Spillet bruker lytteserver i dag, så dette slår først til med dedikert server.

**F16 – LAV – Buer uten søyler i Lysgården.**
`building_hovedbygningen.py` linje 287–288 fjerner søylene ved trappen i 2. etasje, men linje 298–301
lager fortsatt buene mellom dem, så tre bueføtter henger i lufta over trappen.

### B. Realismehull (campus mot virkeligheten)

Sikkert = jeg er trygg på dette. Antatt = bør sjekkes mot kart/bilder før det bygges.

**R1 – HØY – Byggene er tomme haller inni.** `buildings.py` lager bare yttervegger, dekker, ramper og
taklys. Ingen innervegger, korridorer, dører, søyler, trapper eller heis. Realfagbygget er ett åpent rom på
160 × 70 m per etasje med møbelgrupper på gulvet. Dørene er åpne hull uten dørblad i alle bygg.

**R2 – HØY – Ramper i stedet for trapper overalt** (25 grader, `GSCampusBuilder.cpp` linje 20), også
«spiralrampe» i tårnet. Ingen ekte bygg ser slik ut.

**R3 – HØY – Alle bygg har samme fasade.** Samme vindu (1,8 m bredt, brystning 1,0 m) og samme rytme på alle
15 bygg (`WINDOW_W`, `MIN_PIER`), og etasjehøyder på 4–6 m (`GSCampus.h`); ekte kontor- og
undervisningsbygg har ca. 3,3–3,6 m. Sentralbyggene blir dermed 52 og 44 m med 13 og 11 etasjer.
«Glassbyggene» (Stripa, Idrettsbygget) er mørke metallvegger med vanlige vinduer, ikke glass.

**R4 – HØY – Flatt terreng.** `BuildGround` er én flat boks. Sikkert: Gløshaugen er en høyde med bratte
skråninger mot nord (Høgskoleparken), vest og øst. `GSCampus.h` sier selv at kartet er en grov skisse som
skal erstattes av OpenStreetMap og Kartverket-data. Alle bygg står dessuten helt rett øst–vest (Yaw 0).

**R5 – MIDDELS – Hovedbygningen.** Sikkert: fronten vender mot nord med to tårn ved inngangen, og fasaden er
grov grå granitt; det stemmer. Antatt feil: det høye klokketårnet bak (38 m + spir til 50 m, linje 373–416)
finnes ikke i virkeligheten. Antatt: tårnhjelmene foran er irret kobber (grønne), modellen bruker skifer.
Sidefløyene (`Hovedbygningen vest/oest`) er egne modeller med egne vinduer og 4,5 m etasjer som står kant i
kant med hovedblokkens sørvegg, så hovedblokkens vinduer der ser rett inn i en vegg.

**R6 – MIDDELS – Plassering og manglende bygg.** Antatt: Stripa går nord–sør som campus' ryggrad, her
ligger den øst–vest. Kjemiblokkene er fem blokker, her ett bygg. Sikkert mangler: Gamle elektro,
Byggteknisk, Materialteknisk, Grønnbygget, Perleporten, Oppredningen, Verkstedteknisk, og parken i nord.
Høgskoleringen er tegnet som et rektangel på 800 × 670 m; campus er antatt nærmere 400 × 600 m.

**R7 – MIDDELS – Himmel og døgn passer ikke til Trondheim.** `GSEnvironment.cpp` linje 206: sola når 48
grader kl. 12 og -48 grader ved midnatt, med 12 timer dag hele året. Sikkert: på 63,4° nord er 48 grader
bare mulig ved midtsommer, og da går sola knapt under horisonten (lyse netter); om vinteren er den 3
grader over horisonten midt på dagen. Snø kan komme når som helst, med løv på trærne. Forslag: velg en
årstid per kamp (replikert), og la solhøyde, daglengde, løv og vær følge den.

**R8 – MIDDELS – Skyene er like i alt vær.** `GSEnvironment.cpp` linje 150–154: skymaterialet settes én
gang. I regn, snø og «overskyet» er det samme lette skydekke, og sola kaster skarpe skygger i regnvær
(bare styrken endres, linje 42–47). Forslag: styr skydekket per vær og slå av solskygger i gråvær.

**R9 – MIDDELS – Regn og snø er små klosser.** 900 terninger i en boks på 36 × 36 m (linje 161–169,
351–354): ingen sprut, ringer i pytter, drypp fra tak eller snø som legger seg i hauger.

**R10 – MIDDELS – Gater.** Sikkert: midtlinjen på norske veier er gul; her er alt hvitt
(`GSCampusBuilder.cpp` linje 239, 267). Sikkert: brannhydranter over bakken (linje 290–293) brukes knapt i
Norge; her står de langs alle fortau. Ingen gangfelt, skilt for parkering eller sykkelfelt.

**R11 – MIDDELS – Roboter som pynt.** `RobotGuard`, `RobotDog` og `RobotDelivery` (linje 1103–1104, 1173–
1177, 1238) står stille rundt på campus. Det bryter med «superrealistisk».

**R12 – MIDDELS – Vinduer er spøkelser.** Glasset har ingen kollisjon (`GSCampusBuilder.cpp` linje 366), så
kuler og spillere går gjennom uten at noe knuser eller får hull.

**R13 – LAV – Bakken.** Gress er en flat tekstur som gjentas hver 4. meter (`W_Grass`, tiling 400) på en
plen på 2,4 × 2,4 km, uten gresstrå. Alle teksturer er regnet ut (1024 px), ingen er skannet. Trærne er
strødd tilfeldig, ikke i alleer langs veiene.

### C. Bør sjekkes i editoren (ikke bekreftet i koden)
- **Natt som ikke blir mørk:** post-prosessen setter bare hastighet på auto-eksponering, ingen nedre eller
  øvre grense. Sola er 10 lux og månen 0,35, så kameraet kan lyse natten opp nesten til dagslys.
- **Tåketetthet:** `FogDensity` 0,30 i tåke, 0,10 i snø og 0,06 i regn er svært høyt (motorens standard er
  0,02). Sjekk at man ser mer enn noen få meter.
- **Nanite + presis kollisjon:** byggene importeres som Nanite (`init_unreal.py` linje 63–67) og bruker
  «complex as simple». Kollisjonen lages da fra reservemodellen, som kan være forenklet. Sjekk at tynne
  ting (rekkverk, karmer) og gulv har riktig kollisjon.
- **Kryssende veier på samme høyde:** bilveier (z -0,02) og gangveier (z -0,01) overlapper der de møtes;
  materialet er verdensprojisert, så det skal ikke flimre, men se etter i hjørnene.
- **Ytelse:** `DefaultEngine.ini` har maskinvare-strålesporing for Lumen på som standard. Mål på RTX 3050.
