**GODKJENT**

# Kontroll av runde 1 (Grafikk og kart) – commit 48f5cbf

Kontrollert mot `ko/plan-01.md`. Bygget er ikke kjørt på nytt (byggeren: «Result: Succeeded»).
Arbeidsmappen er lik commiten for `GSCampus.h`, `Tools/Blender/` og `SourceArt/Models/`, så sjekkene under
gjelder commiten.

## Hva jeg kjørte selv
- `python Tools/Blender/check_campus.py`: bare OK (11 bygg, 15 veier, 2 baser, 18 spawnpunkter), kode 0.
- `blender --background --python Tools/Blender/check_walkable.py`: alle 21 dørstråler og alle rampestråler OK, kode 0.
- **Egne kontrollstråler** mot `SM_Hovedbygningen.blend` og `SM_Bygg_Gamle_fysikk/Gamle_kjemi/Kjelhuset.blend`
  (bare lesing), for å se om sjekken tester det den sier:
  - Hver dør i Hovedbygningen: fri i midten og 0,4 m innenfor hver karm, og **treff i veggen** 1 m utenfor
    karmen (y = −18,25 ved hoveddøra, x = ±55,25 ved endedørene osv.). Strålene går altså virkelig gjennom
    veggplanet der døra står. Fri også i 1,9 og 2,2 m høyde. Ingen terskel (gulv 0,02 m).
  - Basene: nord- og sørdør står midt på (x = ±125), fri. Øst-/vestdør står ved lokal y = +1,64
    (stråle ved y = 0 treffer veggen).
  - Rampetoppen: rekkverket er borte de siste 2,6 m. Gulvkanten er et trinn opp fra rampen: 0,19 m helt
    øverst, 0,47 m én meter før toppen (rampe 3,83/3,55 m, gulv 4,02 m). Med `MaxStepHeight` 50 cm og
    kapselradius 40 cm kan man gå av rampen på de øverste ca. 0,65 m. Det holder, men er trangt.
- **Mutasjonstest av `check_campus.py`** (på en kopi): spawn inne i bygg, spawn utenfor området, flagg
  utenfor eget bygg, sti inn i Hovedbygningen og bygg utenfor området ble alle fanget med FEIL og kode 1.
- Regnet ut kantsteinen (samme formel som `BuildPathEdges`): 720 biter, 670 inne i spillområdet.

## «Ferdig når», punkt for punkt
| Krav | Resultat |
|---|---|
| `check_walkable.py` bare OK | Ja (kjørt selv) |
| `check_campus.py` bare OK | Ja (kjørt selv) |
| Bygget går gjennom | Ja, ifølge byggeren (ikke kjørt på nytt) |
| To renderbilder | Ligger der. `r01-hovedinngang.png` viser tydelig åpen dør. `r01-rampetopp-kjelhuset.png` viser åpningen, men er tatt ovenfra uten tak og er vanskelig å lese; strålene er beviset. |
| Ingen endring i `gs_materials.py`, `Config/`, `Content/` | Ja, commiten rører ingen av dem |
| Steg 13–20 i `GSCampusBuilder.cpp` og sola | Alle med: kulisser, `Mat_Glass`, plasser, kantstein, allé, gatetrær, hekker, biler 22, plenmøbler 8, asimut +90 |

## Andreas' faste regel
- **Forbedret (gammelt):** dørene i Hovedbygningen er åpne (F1), rampene kan forlates i toppen (F2), sola
  står riktig (F4), hekkene står ikke foran dørene (F6), stinettet er lagt om (F7).
- **Nytt:** spillområde og baser som kartdata, kulissebygg, brolagt forplass og bakgård, kantstein, allé og
  gatetrær, to sjekkskript.
Begge deler er oppfylt.

## Det som er sjekket og funnet i orden
- **Sola:** `Azimuth = Hours/24*360 + 90`. Kl. 12 er yaw 270°, lyset går mot −Y (nord), sola står i sør.
  Kl. 06: lyset går mot vest (sola i øst). Kl. 18: lyset går mot øst (sola i vest). Høyde 48° kl. 12.
  Månen følger med (+180°). Ingen andre steder bruker asimuten.
- **Determinisme:** `AGSCampusBuilder` er ikke replikert og bygges på hver maskin med faste frø
  (1337, 777, navne-hash, 4242). `PlantTree` og `Placed` trekker i fast rekkefølge. Likt hos vert og klient
  så lenge begge har de samme importerte modellene (kjent forbehold F11, står i planen).
- **Hekkformelen** er lik `BuildWallSide`: basene får dør ved lokal y = 1,64 (6 vinduer, indeks 3), og
  hekkene ved y = 0,4 og 4,6 hoppes over. Fløyene og Hovedbygningen: dør i y = 0, riktig hoppet over.
- **Spawnpunktene:** alle 18 er inne i området og minst 1,5 m fra fotavtrykk (nærmeste er 13 m fra vegg).
  Trær, busker og plenmøbler holder 3,5–4 m avstand. `PickSpawnTransform` sporer ned til bakken og tester
  plass til en kropp, så et punkt som likevel er opptatt blir hoppet over.
- **Flaggpunktene** (±125, 10) ligger i krysset mellom de to korridorene i basen. `FurnishBuilding` holder
  korridorene (±1,5 m) fri for møbler. Fire innganger, som Spillbarhet ba om.
- **Kollisjon:** kantstein uten kollisjon (ingen hekter seg). Plassene har kollisjon, topp 3 cm.
  Trestammene har egen kollisjon med riktig tykkelse per tresort.
- **Ytelse:** 11 bygg (var 17), 118 trær (220), 10 steiner (70), 4 bed (30), 22 biler (60), 8 plenmøbler
  (45). Nytt: 720 kantsteiner i én instansgruppe uten kollisjon (vanlig ISM, ett tegnekall) og 2 plasser.
  Modeller og møbler går fortsatt i HISM. Klart lettere enn før.
- **Kanten av banen** er ikke stengt, og kulissene kan nås og gås inn i (åpne dører, tomt inne). Det er et
  kjent hull som står i planens risiko og i bestillingen til Spillbarhet (gjerder i runde 2, regel bak).
  `GSGameMode` hopper over loot og terminaler i kulissene (`bEnterable = false`).

## Funn (ingen stopper runden; rettes i runde 2)

**K1 – LAV – `README.md` linje 118: ødelagt Blender-kommando.** Det står et backspace-tegn (0x08) i
stedet for `\b`: `Blender 5.3<BS>lender.exe`. Skal være `...\Blender 5.3\blender.exe`. Kopierer Andreas
linja, virker den ikke.

**K2 – LAV – Kantstein ligger tvers over stimunninger.** `GSCampusBuilder.cpp` `BuildPathEdges`: bare
midtpunktet av hver 4 m-bit testes mot andre stier. Ca. 24 biter stikker inn i en kryssende sti, vei eller
plass. Verst: ved (±80,1, 8) og (±80,1, 12) dekker to biter hele bredden av stien inn mot basenes endedør,
så det ligger en 3 cm høy betongstripe tvers over stien. Også 1 m inn i passasjene ved (±75, ±1,6), 0,5 m
ved forplassen (±16, −30), ved promenaden (Y = 116/124) og ved Høgskoleringen (Y = −104). Bare utseende
(ingen kollisjon). Rett: test begge ender av biten (eller kutt biten ved krysset).

**K3 – LAV – Stien til basenes endedør treffer veggen, ikke døra.** `GSCampus.h`: stiene
(−100,10)–(−78,10) og (78,10)–(95,10) ligger i Y = 10, men døra i ±X-veggen står ved Y = 11,64
(10,74–12,54). Stien (8,5–11,5) dekker bare 0,76 m av døra. Flytt de to stiene til Y = 11,6.
`check_campus.py` bør få en sjekk for «sti som ender ved et bygg, ender ved en dør».

**K4 – MIDDELS (kartdesign, ikke kodefeil) – Nordruta er en åpen skytebane, og spawn ligger i fri sikt.**
- Stien Y = −30 er 316 m snorrett. Ingen trær kan stå nærmere enn 5 m fra midten, alleen stopper ved
  Y = −41, og forplassen er tom. Fra (−125, −30) foran base vest er det fri sikt til (125, −30) foran base
  øst: 250 m uten dekning.
- Fem av sju spawnpunkter per lag ligger på åpen plen 14–16 m fra der motstanderens nordrute kommer inn
  (±110…140, −14/−16), og tre til ligger like åpent på sørsiden. Bare punktet bak bygget (−157/159, 10) er
  skjermet. Ellers er det ca. 35 tilfeldige trær, 10 steiner og 8 plenmøbler på hele banen.
- Flagget står i rett linje bak nord- og sørdøra: det kan ses og beskytes fra nordstien 40 m unna.
Dette må løses før CTF føles rettferdig: dekning langs nord- og sørruta (murer, hekker, biler,
byggegjerder, sykkelstativ), spawn inne i basen eller bak den, og noe som bryter siktlinja dør–flagg.

**K5 – LAV – Banen er ikke speilvendt.** Gamle kjemi er 10 m lengre enn Gamle fysikk (endedør til flagg
28 m mot 23 m), og Kjelhuset står bare på vestsiden av sørruta. Avstand flagg–flagg 250 m. Står allerede
i bestillingen (sidebytte); jevnes ut med et bygg/dekning ved (62, 105) eller ved å korte Gamle kjemi.

**K6 – LAV – `check_walkable.py` tester rekkverket, ikke trinnet.** Rampestrålene går 0,3 m over
etasjegulvet 1,3 m før toppen. Der er gulvkanten 0,6 m over rampen, altså ikke gangbar. Sjekken ville
sagt OK selv om gulvet lå en meter over rampen. Legg til stråler rett ned på rampen og på gulvet ved
siden av, og krev trinn ≤ 0,45 m på minst 0,8 m lengde. Dørsjekken bør også skyte én stråle 1 m utenfor
karmen som SKAL treffe (slik at feil dørkoordinat ikke gir falsk OK), og `check_campus.py` bør kreve et
fast antall bygg (11), ikke bare «flere enn 0».

**K7 – LAV – Rampetoppen er trang.** Man må helt opp til ytterveggen før man kan gå av (de øverste
0,65 m). Bedre: et flatt repos på 1,2 m i toppen, eller senk rampen slik at toppen møter gulvet 1,5 m før
veggen. Tas når innvendig arbeid i basene planlegges.

## Forslag til neste runde
1. **Dekning og spawn for CTF (K4):** bryt siktlinja på nordruta, gi sørruta dekning, flytt spawn bak
   eller inn i basene, og skjerm flagget. Steng samtidig kanten av banen med synlige gjerder (avtalt).
2. **Småfeilene K1–K3** i samme runde (README-linja, kantstein i kryss, stien til endedørene), og styrk
   sjekkskriptene (K6) så de fanger slikt selv.
3. **Jevn ut basene (K5)** og gjør dem lette å kjenne igjen (lagfarge/skilt ved dørene), så rødt og blått
   har like forhold.

## Lærdom
- Et sjekkskript må ha en motprøve: minst én stråle/verdi som SKAL feile, ellers kan det si OK av feil grunn.
- Ting som legges i biter langs en linje (kantstein, fortau, hekk) må testes i begge ender av biten, ikke bare i midten.
- Når kartdata sier «sti til dør», regn ut hvor døra faktisk står (dørformelen gir bare midtstilt dør ved oddetall vinduer).
