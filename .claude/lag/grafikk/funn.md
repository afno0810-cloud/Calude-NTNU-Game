# Funn fra kontrollørens feiljakt (Grafikk og kart)

Nyeste øverst.

## Områder som er gjennomgått
- 2026-11-26, feiljakt 2 (commit 058be3c): `Tools/Blender/props.py` (alle 45 rekvisitter), `cars.py`,
  `backdrop.py`, `nature.py` på nytt (vegetasjon: kort, UV, vind), `Tools/Textures/gen_textures.py` i detalj
  (og de ferdige filene i `SourceArt/Textures`), ytelse: lys, skygger, Nanite/LOD, dynamiske materialer,
  tick i `GSCampusBuilder`/`GSEnvironment` (og `GSPickup`/`GSTerminal` som påvirker oss), og Lumen-innstillingene
  i `Config/DefaultEngine.ini` (bare lest). **Ikke gjennomgått ennå:** `details.py`, `FurnishRoom` rom for rom,
  `building_hovedbygningen.py` innvendig etter rettelsene, ytelse målt med `stat gpu` i spillet (krever Andreas).
- 2026-11-26 (commit 55ca90f): `GSCampusBuilder.cpp/.h`, `GSCampus.h` (kartdata), `GSEnvironment.cpp/.h`,
  `GSWorldSubsystem.cpp`, `Tools/Blender/buildings.py`, `building_hovedbygningen.py`, `meshkit.py`,
  `nature.py`, `Content/Python/init_unreal.py`, `gs_materials.py` (master-materialene og instanslisten).
  Bare skummet: `details.py`, `gen_textures.py`.
- Metode: lest koden, regnet på målene, og (for F1, F2, F5) skutt stråler mot de eksporterte modellene
  `SourceArt/Models/SM_Hovedbygningen.blend` og `SM_Bygg_Kjelhuset.blend` i Blender (bare lesing).
  Feiljakt 2: talt trekanter, materialplasser og bladkort-areal i alle `.blend` i Blender (bare lesing),
  målt teksturfilene med Python, simulert `backdrop.py`-terrenget, og lest `Saved/Logs/Gloshaugen.log`.

## Feiljakt 2 – 2026-11-26 (commit 058be3c)

Alvorlighet som før: **HØY**, **MIDDELS**, **LAV**. Tall fra loggen (PIE 16:12): skjermkort
**RTX 3050 6 GB Laptop**, «campus built, **33 737** pieces in 85 groups, **31 567** props (**2,97 s**)»,
428 loot-plasser, 17 PC-er, 11 printere, 38 kameraer.

### P. Ytelse (Andreas: jevnere bildefrekvens, mindre hakking) – konkrete, målbare forslag

Slik måles alt under (før og etter, samme sted, samme vær, klokka låst): `stat unit` (Game/Draw/GPU ms),
`stat unitgraph` (hakk synes som topper), `stat gpu` (tid per pass: ShadowDepths, Lumen, BasePass,
Translucency, VolumetricFog), og `viewmode quadoverdraw` for bladverket. Test helst som «Standalone Game»:
i PIE kompileres skyggeleggere første gang et materiale vises, og det gir hakk som ikke finnes i ferdig spill.

**P1 – HØY – Sol-skyggene (Virtual Shadow Maps) tegnes trolig helt på nytt hver frame.** VSM sparer tid ved
å huske skyggesider som ikke endres. Det skjer aldri her, av fire grunner:
1. sola roteres hver frame (`GSEnvironment.cpp` linje 208, se F8);
2. alt løv har vind (World Position Offset) i `M_GS_Foliage` (`gs_materials.py` linje 300–325), og WPO gjør
   skyggesidene ugyldige hver frame for alle 220 trær, buskene og hekkene;
3. 428 loot-gjenstander snurrer og hopper hver frame med skygge på (`GSPickup.cpp` linje 103–111, Spillbarhet);
4. 38 overvåkningskameraer svinger hver frame (`GSTerminal.cpp` linje 216–231, Spillbarhet).
Forslag (vårt): sola i trinn (F8); på tre-/busk-gruppene i `AddModel`: `SetWorldPositionOffsetDisableDistance(4000)`
og `ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Rigid`. Bestilling til Spillbarhet:
loot snurrer bare når den er nær (< 30 m) eller nylig synlig (`WasRecentlyRendered`), med ett
`SetRelativeLocationAndRotation`-kall i stedet for to, og uten skygge på den snurrende modellen.
Mål: `stat gpu` → ShadowDepths/VirtualShadowMaps-tiden skal falle tydelig når man står stille.

**P2 – HØY – De 33 737 klossene kaster skygge, også bakken, veiene, oppmerkingen og fortauene.**
Alle `AddBox`/`AddBoxMat`-grupper (`GSCampusBuilder.cpp` linje 51–63, 132–140) bruker motorens kube, som ikke er
Nanite, med skygge på. Ikke-Nanite i VSM tegnes per skyggeside og er dyrt, og sammen med P1 skjer det hver frame.
Bakken (2,4 × 2,4 km, linje 186), veier (linje 215), 1 cm tykke malingsstriper (linje 267, 271), kantstein og
fortau (linje 279–280) kan ikke kaste synlig skygge på noe. Forslag: `SetCastShadow(false)` for grupper der
klossen er lavere enn 0,3 m (egen nøkkel i `ISMs`), og vurder en Nanite-kopi av kuben for resten.
Mål: ShadowDepths i `stat gpu`, og antall skyggekastere med `r.Shadow.Virtual.ShowStats 1` (om den finnes i 5.8).

**P3 – HØY – Trærne er tunge å tegne: store, gjennomsiktige bladkort, ikke Nanite, ingen LOD.** Målt i
`.blend`-filene: bladkort-areal per tre er **1 770 m²** (lønn), 942 m² (gran), 599 m² (bjørk), 549 m² (liten
gran), busk 55 m², hekk 68 m². Med fordelingen i `BuildTreesAndProps` (38/30/20/12 %) blir det ca.
**240 000 m² maskerte, tosidige kort** for 220 trær, som tegnes i dybdepass, grunnpass og skygge, lag på lag
(en lønnekrone på ca. 8 m bredde har 10–15 kortlag gjennom seg). `init_unreal.py` linje 21 utelater trær fra
Nanite med begrunnelse «masked», men Nanite støtter maskert materiale, og `M_GS_Foliage` er allerede merket
`used_with_nanite`. Forslag: prøv Nanite for `SM_Tree_*`, `SM_Bush`, `SM_Hedge`, og halver kortene i lønna
(`nature.py` linje 123: 7 → 4 per grentupp, linje 126: 40 → 20). Mål: `viewmode quadoverdraw` mot et tre,
og BasePass/PrePass/ShadowDepths i `stat gpu`.

**P4 – MIDDELS – `DefaultEngine.ini` hindrer grafikknivåene i å skru av ting (Andreas' fil, bare forslag).**
Loggen sier rett ut: «Setting the console variable 'r.VolumetricFog' with 'SetByScalability' was ignored as it
is lower priority than the previous 'SetByProjectSetting'» (samme for `r.TSR.ShadingRejection.Flickering`).
`Grafikk 0/1` kan altså ikke slå av volumetrisk tåke. Fjern de to linjene (standard er på uansett).
`r.Lumen.HardwareRayTracing.LightingMode=1` betyr «hit lighting» i refleksjoner: materialer og lys regnes ut på
nytt i hvert stråletreff. Det er tungt på en RTX 3050 og blir verre i regn, når `Wetness` gjør alle flater
blanke. Forslag: `LightingMode=0` (overflate-cache). `r.Lumen.TraceMeshSDFs=1` gjelder bare programvare-Lumen.
Mål: «Lumen Reflections» og «VolumetricFog» i `stat gpu`, i sol og i regn.

**P5 – MIDDELS – `Straaling 0` skrur ikke av strålesporingen, bare at Lumen bruker den** (`GSPlayerController.cpp`
linje 329–341, Kamp). Strålesporingsscenen bygges fortsatt hver frame, med 20 animerte spillere
(`r.RayTracing.Geometry.SkeletalMeshes=1`, `DynamicGeometry.MaxUpdatePrimitivesPerFrame=-1` i loggen), 31 567
rekvisitter og 1 800 regndråper. Bestilling til Kamp: sett også `r.RayTracing.Enable 0/1` (sjekk med § at den
finnes i 5.8). Mål: `stat gpu` med `Straaling 0` før/etter.

**P6 – MIDDELS – Regn og snø bygger tegneobjektet på nytt hver frame.** `GSEnvironment.cpp` linje 384:
`BatchUpdateInstancesTransforms(..., bMarkRenderStateDirty = true, ...)` for 900 dråper (alle 900 oppdateres også
når bare noen er synlige). Gruppen er `Movable`, med i strålesporing, Lumen og distansefelt (ingenting er skrudd av,
linje 131–141). Forslag: `SetVisibleInRayTracing(false)`, `bAffectDistanceFieldLighting = false`,
`SetAffectDynamicIndirectLighting(false)` på `RainISM`/`SnowISM`, og oppdater bare de synlige. Bedre på sikt:
Niagara på skjermkortet (løser også R9). Mål: `stat game`/Unreal Insights for `AGSEnvironment::Tick` i regn.

**P7 – MIDDELS – Ubegrenset bildefrekvens på en bærbar.** `GSPlayerController.cpp` linje 257:
`SetFrameRateLimit(0.f)` og ingen VSync. Skjermkortet går da for fullt hele tiden, blir varmt og klokker seg ned
i rykk, som gir ujevn bildetid. Bestilling til Kamp: standard tak 60 (eller skjermens frekvens minus 3), valg i
innstillingene. Mål: `stat unitgraph` – jevnheten (de laveste 1 %) er viktigere enn snittet.

**P8 – MIDDELS – Spillet fryser i 3 sekunder ved start og ved hver ny tilkobling.** Loggen: «(2.97 s)», og hele
PIE-starten tar 4,7 s. Alt bygges på spilltråden i én frame: 31 567 rekvisitter lagt inn én og én i HISM-er
(`AddProp` linje 821, `AddModel` linje 690), hver med egen kollisjonskropp (`bCollide = true` som standard,
`GSCampusBuilder.h` linje 60). Forslag: samle transformene og kall `AddInstances` én gang per gruppe, slå av
kollisjon på små ting ingen trenger å stå på eller ta dekning bak (f.eks. `Plant`, `TrashBin`), og mål samme
logglinje. Mål: under 1 s.

**P9 – LAV – Ca. 176 punktlys uten avstandsgrense.** 110 gatelys (`GSCampusBuilder.cpp` linje 744–753), 28 PC-/
printerlys og 38 kameralys (`GSTerminal.cpp` linje 51–54, 195–201). Ingen har skygge (bra: sola er eneste lys
med skygge), men ingen har `MaxDrawDistance`, så alle regnes med i lysrutenettet og Lumen om natta.
Forslag: `MaxDrawDistance = 8000`, `MaxDistanceFadeRange = 1500` på gatelysene (og bestilling på de to i
`GSTerminal`). Mål: `stat LightRendering` om natta.

**P10 – LAV – Biler, glass, bakgrunn og trær har ingen LOD.** Bare `SM_Hovedbygningen`, `SM_Bygg_*` og
`SM_Prop_*` blir Nanite (`init_unreal.py` linje 21). En bil er 8 700 trekanter og 9 materialplasser (60 biler,
også på 400 m), byggglasset opptil 8 500. Forslag: automatiske LOD-er ved import for `SM_Car_*` og `SM_Glass_*`.

Dynamiske materialer og tick ellers: ingen problem funnet. `GSCampusBuilder` tikker ikke; fargeinstanser lages
én gang per gruppe (85); `GSEnvironment` setter lys, tåke og MPC hver frame, men motoren hopper over like verdier.

### A. Ekte feil (fortsetter nummereringen fra feiljakt 1)

**F17 – MIDDELS – Bakgrunnsbyen er halvveis begravd, og åsene svever** (`backdrop.py`). Simulert med samme
støyfunksjon: av ca. 346 hus står **169 helt under terrenget** og 54 til mer enn halvt under, fordi husene
settes på z = 0 (linje 75) mens terrenget mellom 1 250 og 1 650 m ligger 1–40 m høyere (linje 49–51).
Åsenes indre kant (1 250 m) ligger 1–38 m over bakken, og bakken er et kvadrat som slutter på 1 200 m
(`GSCampusBuilder.cpp` linje 186). Rett nord/sør/øst/vest er det derfor 50 m med ingenting under åskanten, så
himmelen synes som en stripe under åsene (tydelig fra tak og tårn); på skrå ligger åskanten som et lokk over
den grønne bakken. Forslag: sett husene på terrenghøyden (samme `noise`), start indre ring på z = 0 ved
r = 1 150 m, og legg et «skjørt» ned til -5 m.

**F18 – MIDDELS – Synlig skjøt hver 2. meter på alle mur- og granittfasader.** Teksturene skal flise sømløst, men
`brick()` (linje 192: `dirt = fbm * smoothstep(0.3, 1.0, y)`) og `stone()` (linje 214: `stain`) gjør bunnen av
hver flis mørkere enn toppen. Målt i filene: nederste rad er **8–11 % mørkere** enn øverste i Brick, BrickBrown,
BrickYellow, Stone, StoneDark og Trim (venstre/høyre: 0–2 %). Det gir vannrette striper oppover hele
Hovedbygningen (granitt = `Stone`) og alle teglbygg. Forslag: fjern y-leddet (smuss etter høyde hører hjemme i
materialet, i verdenshøyde), eller gjør gradienten periodisk.

**F19 – MIDDELS – Bark-UV regnes rundt treets midtpunkt, ikke rundt grenen** (`nature.py` linje 90:
`atan2(c.y, c.x)`). Følger: (a) på hver stamme hopper u fra +0,6 til -0,6 = 1,2 fliser, som ikke er et helt tall,
så det blir en loddrett skjøt i barken; (b) en gren 2 m fra stammen med radius 0,1 m får bare ca. 2 cm tekstur
rundt en omkrets på 63 cm, altså barken strukket ca. 30 ganger. Forslag: lag UV i `branch()` (u = i/verts ×
avrundet omkrets i meter, v = avstand langs grenen).

**F20 – MIDDELS – Bilene har ingen hjulbuer; hjulene står inni karosseriet** (`cars.py` linje 49, 73–84).
For sedanen er siden av karosseriet ved z = 0,4 m på y ≈ 0,87 m, dekket går til 0,90, felgen til 0,91 og eikene
til 0,92. Øvre halvdel av hvert hjul er skjult inne i lakken, og felgen stikker 3–5 cm ut gjennom den. Den svarte
«hjulbue-skiven» (r = 0,39 m) stikker 4 cm ned i bakken. Bilene er den viktigste dekningen ute og synes overalt.
Forslag: smalere karosseri over hjulene (hakk i loftet), hjulene 2–3 cm utenfor siden.

**F21 – LAV – Gatelampene lyser ikke selv.** Lampehodet er `PlasticWhite` uten glød (`props.py` linje 908,
`gs_materials.py` linje 500), mens punktlyset står 17 cm under det. Om natta kommer et sterkt lys fra et mørkt
hode. Forslag: egen plass `LampGlow` med glød (som `LightPanel`), slått av og på med gatelysene (Custom
Primitive Data på HISM-en).

**F22 – LAV – To av fire sykler står inni sykkelstativet** (`props.py` linje 619 og 626). Bøylene står ved
y = -1,5 + 0,6·i, og syklene ved y = 0,3 og 0,9 står i samme plan som to bøyler: bøylen går gjennom eikene, og
setestammen krysser toppstanga (z = 0,72). Forslag: syklene midt mellom bøylene (-1,2, -0,6, 0,0, 0,6).

**F23 – LAV – Vinden i løvet** (`gs_materials.py` linje 300–325): utslaget er `1 - UV.y` i bladatlaset, så det
avhenger av hvilken rad i atlaset kortet viser (0–0,25 mot 0,75–1), og de tre kortene i ett kryss svinger ulikt.
Største utslag er 2,3 cm, altså nesten usynlig på et 10 m høyt tre, men med full kostnad (P1, P3). Andreas' ønske
nr. 5 (trær som beveger seg i vinden) krever uansett ny vind: utslag etter høyde over treets pivot, vindstyrke fra
`GSEnvironment` via MPC, og WPO av på avstand.

**F24 – LAV – Pakking av teksturfilene ødelegger kanalene.** ARM- og normalkart lagres som JPEG med 4:2:0
(Pillow-standard, bekreftet i filene). Fargekanalene deler da oppløsning og blør over i hverandre: målt
«metallic» (skal være 0) er opptil 33/255 i Brick/Asphalt, og 10 % av pikslene i Grass er over 0,04. Normalkartenes
X/Y får halv oppløsning. Forslag: lagre `_N` og `_ARM` som PNG (husk å slette de gamle `.jpg`, ellers importeres
begge til samme navn).

**F25 – LAV – Trestruktur som går feil vei, og mønstre finere enn pikslene.** `planks()` (linje 278–281):
plankene ligger langs Y etter `.T`, men årringene regnes fra den gamle `y` og går på tvers; målt er variasjonen
på tvers av planken nesten null (std 0,03/255), så gulvene ser malt ut. Betongforskaling (linje 265:
`sin(x * 600π)`, 3,4 piksler per periode) og stoffvev (linje 357: `k = 220`, 4,7 piksler) er nær
pikselgrensen og gir moiré. Forslag: bruk transponerte koordinater for årringene, og maks ca. 1/8 av
oppløsningen for fine mønstre.

**F26 – LAV – Bladatlaset har svart bakgrunn** (`gen_textures.py` linje 466, 487: farge 0 der alfa er 0). I
mip-nivåene blandes svart inn i kanten av bladene (mørke kanter på avstand), og maskert alfa krymper i mipene, så
trærne tynnes ut på avstand. Forslag: fyll fargen ut i de gjennomsiktige pikslene, og slå på
«alpha coverage»-skalering for `T_GS_Leaves_D` ved import.

### C. Bør sjekkes i editoren
- **Glass på Nanite-rekvisitter:** `VendingMachine`, `ServerRack`, `LabBench`, `CoffeeMachine`, `Printer3D` og
  `BusStop` har plassen `Glass` (gjennomsiktig `M_GS_Glass`) og importeres som Nanite (`init_unreal.py` linje 21).
  Nanite har tradisjonelt ikke støttet gjennomsiktige materialer. Loggen viser ingen advarsel, men se om glasset
  i busskuret og brusautomaten ser riktig ut (ikke grått eller helt borte).
- **Poly Haven-steinene tegnes med standardmaterialet:** loggen sier «Material .../boulder_01 missing usage flag
  InstancedStaticMeshes! Default Material will be used in game» (også `rock_07`), og `M_PH_Base` feiler for
  SM5. Steinene (`SM_Rock_A`/`B`) ser da ut som rutete grå klosser hos den som har Poly Haven-pakken.
  Forslag: sett `used_with_instanced_static_meshes` på disse materialene i `init_unreal.py`/`gs_materials.py`.
- **Kollisjon på rekvisitter:** vanlige rekvisitter får automatisk, forenklet kollisjon (`init_unreal.py` linje 56).
  Sjekk med «Show Collision» om pulter, stoler, gjerdet og syklene er massive klosser (kan ikke krype under en
  pult, kuler stopper mellom spilene i gjerdet).

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
