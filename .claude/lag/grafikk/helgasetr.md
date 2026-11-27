# Helgasetr – kartbeskrivelse (CTF)

Eies av lagets planlegger. Skrevet 27. nov. 2026 fra plantegningene, møbleringsplanene, vinnerforslaget og de 27
bildene i `SourceArt/Reference/Helgasetr/`. Dette er fasiten for alle rundene på det nye kartet. Rundeplanen står i
`retning.md`, første runde i `ko/plan-04.md`.

**Målt** = lest av tegning med mål (aksemål, koter, arealer). **Anslått** = målt med linjal på tegningen uten mål, lest
av bilder eller lagt inn for spillets skyld. Alt som er anslått er merket *(anslått)*.

## 0. Kilder og hvordan de leses

| Kilde | Hva den gir |
|---|---|
| `plans/plantegninger_2018.pdf` (Statsbygg/LINK, forprosjekt 19.03.2018, 1:200) | s. 1 Plan U1, s. 2 Plan U2 (001/002), s. 3 Plan 1, s. 4 Plan 2, s. 5 Plan 3, s. 6 Plan 4, s. 7 Plan 5, s. 8 Plan 6, s. 9 Snitt AA, s. 10 Snitt BB. **Aksemål og koter herfra.** |
| `plans/moblering_U2-6_des2023.pdf` (Eggen arkitekter, detaljprosjekt 12.12.2023, 1:250, A3) | Slik bygget ble (as-built): s. 1–2 U-etasjer, s. 3 Plan 1 … s. 8 Plan 6, med møbler. Nyest – går foran 2018 der de er uenige. Adresse på tegningen: «Helgasetr, Vangslunds gate 2». |
| `plans/moblering_U2-6_aug2023.pdf` | Eldre utgave av den over. Brukes ikke. |
| `plans/vinnerforslag.pdf` (jan. 2017) | Ideen: «maurtue», studenttorget som husets hjerte, trappa som «slynger seg opp gjennom atriet», grupperom som «hengende bokser», klatrehall åpen over tre etasjer. |
| `plans/plankart.jpg` | Tomta (Vangslunds gate i vest, Elgeseter gate i øst, Høgskoleparken i nordøst). |
| 27 bilder `helgasetr_01…27` | Materialer, farger, møbler, lys (se kap. 8). |
| NTNU (ntnu.no/campusutvikling/elgesetergate-10) | Totalt 18 800 m² BTA, Statsbygg og Sit eier, NTNU leier, LINK arkitektur + Backe vant 2017. |

PDF-ene kan ikke leses som bilder med Read-verktøyet på denne PC-en (mangler pdftoppm). Bruk Windows' egen PDF-motor:
`powershell -File .claude/lag/grafikk/verktoy/renderpdf.ps1 -Pdf <pdf> -OutDir <mappe> -Prefix plan -Scale 2`
(gir PNG 8980 × 6350 per side for plantegningene; skaler ned med PIL før du ser på dem). Teksten (romnavn, arealer, koter)
får du med `pdftotext -layout` (finnes i Git Bash).

## 1. Koordinater (samme konvensjon som før)

Meter, **X = øst, Y = sør, Z = opp** (Unreal: cm = m × 100). Origo = akse 5 × akse H på gulvet i Plan 1 (kote +17,000).
Det gir et bygg som er nesten punktsymmetrisk om origo: atriet og ringkorridoren er symmetriske i begge retninger.

### Akser (målt, fra Plan 1–4: «10 350 · 1 860 · 6 770 ×4 · 1 860 · 3 800 · 6 550 = 51 500» og «5 860 · 5 950 · 3 550 · 2 270 · 6 010 ×6 · 2 270 · 6 150 · 5 660 = 67 770»)

| Akse X | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| X (m) | −25,75 | −15,40 | −13,54 | −6,77 | 0 | +6,77 | +13,54 | +15,40 | +19,20 | +25,75 |

| Akse Y | N | M | L | K | J | I | H | G | F | E | D | C | B | A |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Y (m) | −32,11 | −26,45 | −20,30 | −18,03 | −12,02 | −6,01 | 0 | +6,01 | +12,02 | +18,03 | +20,30 | +23,85 | +29,80 | +35,66 |

Nord = −Y (akse N), sør = +Y (akse A). Vest (akse 1) = Vangslunds gate, øst (akse 10) = Elgeseter gate.

### Etasjer (målt, Snitt AA/BB)

| Plan | Kote | Z i spillet | Etasjehøyde | Spillbar? |
|---|---|---|---|---|
| U2 | +8,050 | −8,95 | – | nei (idrettshall, ikke med) |
| U1 | +12,800 | −4,20 | – | nei (garderober, sykkelplaza) |
| **Plan 1** | +17,000 | **0,0** | 5,6 | **ja** – kantina/atriegulvet |
| **Plan 2** | +22,600 | **5,6** | 4,3 | **ja** – mesaninen |
| **Plan 3** | +26,900 | **9,9** | 4,0 | **ja** |
| **Plan 4** | +30,900 | **13,9** | 4,3 | **ja** – basene |
| Plan 5 | +35,200 | 18,2 | 4,0 | kulisse (kortleser, «kun ansatte») |
| Plan 6 | +39,200 | 22,2 | 4,0 | kulisse (toppetasjen under glasstaket) |
| Tak / gesims | +43,200 | 26,2 | – | nei |
| Sagtakets møne | ca. +46,0 | ca. 29,0 *(anslått fra snitt)* | – | nei |

Dekketykkelse 0,35 m *(anslått)*. Himling i rom 3,0–3,2 m over gulv *(anslått)*; i korridorene åpent med trelister.

## 2. Fotavtrykk og fløyer

Ytre mål **51,5 × 67,77 m** (målt). Arealet per etasje er ca. 2 900–3 300 m² (Plan 4: sum av rom og korridor ≈ 2 950 m²).

```
            akse 1     2  3        4        5        6        7  8     9        10
   N −32,1  ·········· ┌────────── nordfløy (rom) ──────────────┬──────────────┐
            (utenfor   │  Undervisningsrom / Nordbasen (P4)     │ grupperom NØ │
   L −20,3  på P3–P6)  ├──┬───────── ringkorridor ─────────────┬─┼──────────────┤
   K −18,0  ┌──────────┤  │╭──────────────────────────────────╮│ │ gang/WC      │
            │ vestfløy │  ││ glassbokser   ATRIET   (åpent fra ││ │ Trapp øst    │
   H   0    │ (laber,  │ r││ P1 til glasstaket)         trapp ││r│ østfløy      │
            │ under-   │ i││                                  ││i│ (ferdighets- │
   E +18,0  │ visning) │ n│╰──────────────────────────────────╯│n│  rom)        │
   D +20,3  ├──────────┼──┴──────────── ringkorridor ──────────┴─┼──────────────┤
            │ grupperom│Trapp│  Undervisningsrom / Sørbasen (P4) │ undervisning │
   B +29,8  │   SV     │ sør │───────── åpent ned (ute) ─────────│   SØ         │
   A +35,7  └──────────┴─────┴───────────────────────────────────┴──────────────┘
```

- **Atriet:** innsiden av ringen er åpen fra Plan 1 til glasstaket (26 m). Ringkorridoren går mellom akse 2 og 3 (vest), 7 og 8
  (øst), K og L (nord) og D og E (sør). Atriekanten (glassrekkverk) står ved |X| = 13,25 og |Y| = 17,75, med avrundede hjørner
  R ≈ 1,5 m *(anslått, tegnet som bue)*. Søylene står på aksene 3 og 7 ved hver bokstavakse, rett på kanten.
- **Fri bredde i ringkorridoren:** 2,0–2,3 m på tegningen; i spillet **2,3 m** (vegg på |X| = 15,55, kant på |X| = 13,25;
  nord/sør vegg på |Y| = 20,05 *(anslått, rundet for spillets skyld)*).
- **Utsparinger** (utendørs, ingen kan gå dit): NV-hjørnet X < −13,54, Y < −12,02 fra Plan 3 og oppover (tak over klatrehallen
  under); vestterrassen X < −15,4, Y +6…+18 på Plan 3 («åpent ned (ute)»); sørterrassen X −6,77…+13,54, Y +29,8…+35,66 fra
  Plan 3 og oppover. Fra Plan 1–2 er vestsiden nord for H en utendørs forplass med hovedinngangen.
- **Trapperom (betong, målt plassering):** *Trapp sør* X −13,54…−6,77, Y +23,85…+35,66 (57 m²); *Trapp øst* X +16,9…+25,75,
  Y −12,5…−7,5 (32 m²). Heis sør X −11,0…−8,6, Y +18,5…+21,0; heis øst X +13,6…+15,6, Y −12,5…−10,0.

## 3. Etasjene, en for en

### Plan 1 (Z 0) – kantina («studenttorget»)
Det store, lyse gulvet under atriet, ca. X −15,4…+15,4, Y −20,3…+5,6, i lys terrazzo.
- **Nord:** glassvegg mot **klatrehallen** (X −13,54…+6,77, Y −32,1…−20,3), åpen fra U1 til Plan 3 med buldre-/klatrevegger
  (vinnerforslaget: «klatrevegg … åpen over tre etasjer»). **Servicesenteret** NØ (disk, X +6,8…+13,2, Y −20,3…−16).
  **Loungen** NV med piano og sofaer.
- **Vest:** hovedinngangen med vindfang (X −17,4…−15,4, Y −10…−6), låst i spillet; utenfor ser man forplassen.
  **Kaféøya** (veggen med grønne mosesirkler og «Kaféøy»-tekst) mot vestsiden *(anslått plassering)*.
- **Atriumtrappa, løp 1** (gul/oransje, ekte trinn) langs vestsiden: X −12,9…−9,9, bunn ved Y −6,5 på Plan 1, topp ved
  Y +5,6 på Plan 2 (31 opptrinn à 0,18 m, inntrinn 0,30 m, repos midtveis) *(plassering målt på Plan 2, retning anslått)*.
- **Sosialtrappa / sittetrappa:** X −9,6…−6,0, Y −5,0…+5,6, brede sittetrinn med vanlig trapp i én side *(anslått)*.
- **Ryddekroken:** det gule tårnet, X −5,8…−1,0, Y +1,0…+5,6, 5,6 m høyt; kiosk med kjøleskapsrekke og isboks på nordsiden,
  info-skjerm, søppelsortering ved siden av; sittegalleri på toppen i Plan 2-høyde, koblet til mesaninen *(anslått, as-built-bilder
  08/09/05)*.
- **Salatbaren** med trebjelker (pergola av limtre) nordvest for Ryddekroken; **serveringsdisken** langs Y +5,6 med kjøkkenet
  bak (lukket).
- **Midt på gulvet:** rader av svarte kvadratiske bord med trestoler, runde svarte bord, lange hvite bord, en rad plantekasser
  (ca. 0,9 m høye, god dekning), to–tre **svarte stålbur-båser** med grønne sofaer langs østsiden (bilde 08, 13).
- **Øst:** Sit-treningssenteret bak glass (styrketrening, spinning) – lukket; åpning til **Trapp øst**-lobbyen (X +15,4…+16,9,
  Y −12,5…−7,5).
- **Sørvest:** korridor X −15,4…−12,9, Y +5,6…+20,3 forbi studentlokalet (vest, lukket) og kjøkkenet (øst, lukket) til
  **Sør-lobbyen** (X −13,54…−6,77, Y +20,3…+23,85) og Trapp sør.

### Plan 2 (Z 5,6) – mesaninen
- **U-formet ring:** vest, sør og øst. I nord er det ingen gulv mellom atriet og klatrehallen (glassvegg).
- **Hullet ned til kantina:** X −9,9…+9,0, Y −17,75…+5,6 (målt på møbleringsplan 2), minus Ryddekroken-galleriet.
- **Mesaninen (sør i atriet):** X −13,25…+13,25, Y +5,6…+18,03, **oransje gulv**. Tre grupperom i glass står på gulvet
  (X −8,8…−4,3, −4,3…+0,3 og +8,8…+12,7; Y +11,9…+17,6, målt). **Sittetrappa med plantekasse** i oransje (bilde 25) ved
  X +5…+12, Y +6,5…+9,5. Her starter **atriumtrappa, løp 2**.
- **Studentarbeidsplassene** (sør): X −6,77…+13,54, Y +18,03…+29,8, **dobbel høyde** helt opp til Plan 4-dekket (13,9) –
  rett under Sørbasen. Pulter i rader, runde bord, bokhyller. To brede åpninger mot mesaninen.
- **Vest:** undervisningsrom 120 pl (X −25,75…−15,4, Y −20,3…−1,8), skap i korridoren, utendørs terrasse bak glass.
- **Øst:** møterom og studentforeningen i glass (X +9…+13,25, Y −12…−1), fysioterapi-rom (lukket), Trapp øst.
- **NØ:** kondisjonssal bak glass (lukket).

### Plan 3 (Z 9,9)
- **Hel ring.** Glassbokser henger inn i atriet (se kap. 4).
- **Atriumtrappa, løp 2** krysser atriet i lufta: fra mesaninen (bunn X ≈ −1, Y +6,2…+8,6, Z 5,6) østover til reposet
  X +7,5…+13,25, Y +6,0…+9,0 på Plan 3 (24 opptrinn, 4,3 m) *(målt plassering på Plan 3, retning anslått)*.
- **Sør:** «åpent ned» over studentarbeidsplassene – glassrekkverk langs Y +20,3 mellom X −6,77 og +13,54. Herfra skyter man
  ned i rommet under Sørbasen.
- **Vest:** undervisningsrom 50 pl (Y −12…−1), grupperom 30 pl (Y −1…+6), deretter vinduer ut mot vestterrassen (Y +6…+18).
- **Nord:** to undervisningsrom 100 pl og grupperom (rett over klatrehallen).
- **Øst:** Trapp øst, ergoterapi-rom (treningskjøkken), barnevern-musikkrom.

### Plan 4 (Z 13,9) – baseetasjen
- **Hel ring**, glassbokser, atriumtrappa løp 3 langs østkanten.
- **Nordbasen** (blått lag): undervisningsrom 50 pl ×2 med foldevegg (åpen), X −13,54…+6,77, Y −32,11…−20,3 (11,8 m dyp).
- **Sørbasen** (rødt lag): undervisningsrom 100 pl, X −6,77…+13,54, Y +20,3…+29,8 (9,5 m dyp). Vindusvegg mot
  sørterrassen (åpent ned, ute).
- **Vestfløyen:** audiologi-laber (audiometrirom, psykoakustisk lab, øreproppslab), mottak og venterom – en rekke rom med
  dører mellom seg: en parallell rute til vestkorridoren.
- **Østfløyen:** Trapp øst, garderober, treningsleiligheten, ferdighetsrom for vernepleiere og radiografi – parallell rute til
  østkorridoren.
- **Hjørner:** grupperom SV (rødt lags spawn), grupperom NØ (blått lags spawn), undervisningsrom 100 pl SØ (lukket, motstykke
  til NV-utsparingen).

### Plan 5 og 6 (Z 18,2 / 22,2) – kulisse
Synlige fra hele atriet: ringkorridor med glassrekkverk, glassbokser, kontorer bak glass, de røde sofabåsene i fellessonene
(bilde 01). **Plan 6** er toppetasjen under glasstaket: runde grønne bord rundt et tre, lilla puffer, hvitt fagverk tett over
(bilde 23). Ingen kommer opp: atriumtrappa videre fra Plan 4 og trapperommene har glassdør med kortleser («Kun ansatte»).
Kan gjøres spillbart senere (høy utkikk) – se retning.

## 4. Atriet, trappene og glassboksene

**Atriumtrappa** (gul/oransje, lakkert stål med gul overflate på trinnene, svart rekkverk, ekte trinn i kollisjonen):

| Løp | Fra → til | Plassering | Opptrinn |
|---|---|---|---|
| 1 | P1 → P2 | vestsiden X −12,9…−9,9, Y −6,5 (bunn) → +5,6 (topp), repos midtveis | 31 × 0,18 |
| 2 | P2 → P3 | tvers over atriet: X ≈ −1 (bunn, Y +6,2…+8,6) → X +7,5 (topp), repos | 24 × 0,18 |
| 3 | P3 → P4 | østkanten X +10,0…+13,0, Y +6,0 (bunn) → −1,8 (topp) | 22 × 0,18 |
| 4 | P4 → P5 | fortsetter nordover langs østkanten – **sperret** med glassport ved bunnen | – |

Løpene henger fritt med gule vanger og bunnplater (bildene 02, 03, 06, 07, 17). Bredde 2,4–3,0 m *(anslått)*. Runde blå
matter på noen repos (bilde 03).

**Glassboksene** («hengende bokser», vinnerforslaget): rom med svart stålramme, glass på tre sider, hvitt tykt undertak
(0,4–0,5 m, bilde 07, 11), hengt i hvite stenger fra takfagverket. Stablet: Plan 3-boksen har Plan 4-boksen rett over.

| Plan | Nord-raden (Y −17,75…−12,3) | Vest-søylen (X −13,25…−8,3) | Sør-raden (Y +12,3…+17,75) | Øst (X +9,0…+13,25) |
|---|---|---|---|---|
| P3 | fellessone (åpen) X −6,6…−0,1; grupperom 15 X +0,1…+6,6; grupperom 15 X +6,8…+12,9 | 4 × grupperom 5 pl, Y −6,0…+5,9 (à 2,9 m) | fellessone (åpen) X −11,0…−6,7; grupperom 10 X −6,6…−0,1; grupperom 10 X +3,3…+6,6; grupperom 10 X +6,8…+12,9 | fellessone (åpen) Y −6,0…+6,0, trapperepos Y +6,0…+9,0 |
| P4 | grupperom 10 X −6,6…−0,1; grupperom 15 X +0,1…+6,6; fellessone + miljøstasjon X +6,8…+12,9 | 4 × 5 pl Y −12,0…−0,1; grupperom 15 Y 0…+4,9; grupperom 10 Y +5,0…+10,0 | fellessone X −6,6…−0,1; fellessone X +6,8…+12,9 | 2 × 5 pl Y +6,2…+12,0; trappeløp 3 |

(Målt med linjal på Plan 3/4 2018; grupperommene på bildene 18–21 bekrefter innholdet.) Det åpne atriet i Plan 4 blir da
ca. X −8,3…+9,0, Y −12,3…+12,3.

**Grupperommene** (bilde 18–21): glassdør i grønn/svart ramme med **kodelås** (tastatur), prikkebånd på glasset, brunt
linoleum/kork, svarte bord på svart fot, grå og grønne kontorstoler på hjul, en trestol, whiteboard på lettveggen,
glassvegg ut mot atriet.

## 5. Spillet: CTF i Helgasetr

### Basene (begge på Plan 4, i hver sin ende, rotert 180° om origo)

| | Sørbasen (indeks 0, rødt lag) | Nordbasen (indeks 1, blått lag) |
|---|---|---|
| Rom | undervisningsrom 100 pl (4.019 i 2018-nummerering) | undervisningsrom 50 pl ×2, foldeveggen åpen (4.014 + 4.015) |
| Rommet | X −6,77…+13,54, Y +20,3…+29,8 | X −13,54…+6,77, Y −32,11…−20,3 |
| Flagg (m) | (+3,4, +26,0, 13,9) | (−3,4, −26,0, 13,9) |
| Dører til korridoren | (+10,0, +20,3) og (−3,0, +20,3), 1,0 m | (−10,0, −20,3) og (+3,0, −20,3), 1,0 m |
| Sidedør til egen lobby | (−6,77, +22,0) → Sør-lobbyen | (+6,77, −22,0) → Nord-lobbyen |
| SpawnYaw | −90° (ser nordover) | +90° (ser sørover) |

Flagg til flagg: 52,4 m i luftlinje. Begge flaggene står 5,7 m fra korridorveggen (sør-rommet er grunnere, så sørflagget står
3,8 m fra yttervinduet, nordflagget 6,1 m). Tre innganger per base (to fra korridoren, én fra egen side) – Spillbarhets ønske
om «minst to innganger» er oppfylt.

**Spawn** (rom bak egen base, med to utganger hver):
- Sør (rødt): grupperom SV, S1 X −25,75…−15,4, Y +20,3…+26,0 og S2 Y +26,0…+32,11. Gangen X −15,4…−13,54, Y +20,3…+32,11
  er lukket mot ringkorridoren og åpen mot Sør-lobbyen. Andre utgang: dør fra S1 nordover til venterommet i vestfløyen
  (−24,0, +20,3). Punkter: (−23,0/−18,0) × (+22,2, +24,4, +27,8, +30,4).
- Nord (blått), samme rotert: N1 X +15,4…+25,75, Y −26,0…−20,3, N2 Y −32,11…−26,0, gangen X +13,54…+15,4, dør fra N1 til
  gangen nord for Trapp øst (+24,0, −20,3). Punkter: (+23,0/+18,0) × (−22,2, −24,4, −27,8, −30,4).
- Lobbyene: Sør-lobby X −13,54…−6,77, Y +20,3…+23,85 (heis sør, dør til Trapp sør ved (−10,2, +23,85)), åpning 1,8 m til
  sørkorridoren ved (−10,2, +20,3). Nord-lobby X +6,77…+13,54, Y −23,85…−20,3, åpning 1,8 m ved (+10,2, −20,3). Rommet bak
  Nord-lobbyen (grupperom 4.022, X +6,77…+13,54, Y −32,11…−23,85) står der Trapp sør står på sørsiden.

### Ruter mellom basene

| Rute | Hvor | Lengde (flagg–flagg, ca.) | Preg |
|---|---|---|---|
| R1 Vestkorridoren | P4 ringen vest, forbi 5-pl-boksene og skapene | 62 m | rask, 40 m rett korridor, søyler og skap som dekning, innsyn til atriet |
| R2 Laberadene | P4 vestfløyens rom: venterom → audiologi → psykoakustisk lab → audiologi → korridor | 66 m | nærkamp, dører, rom i rom |
| R3 Østkorridoren | P4 ringen øst, langs atriumtrappa | 62 m | trappa gir innsyn ovenfra og nedenfra |
| R4 Østfløyens rom | radiografi → vernepleier → treningsleiligheten → Trapp øst-lobbyen | 66 m | nærkamp, kjøkken og senger som dekning |
| R5 Gjennom atriet | P4 → trappeløp 3 ned → P3 → løp 2 over atriet → mesaninen P2 → opp igjen | 75–85 m | vertikal, utsatt i trappene, mye å se |
| R6 Kantina (lavruta) | ned Trapp sør / Trapp øst / løp 1 til P1, over kantina, opp på andre siden | 95–110 m | lang, men usett fra Plan 4; dekning: Ryddekroken, salatbaren, plantekasser, båsene |
| R7 Klatreruter (runde 5) | se under | – | Andreas liker klatring |

**Kjent skjevhet:** Trapp sør kommer opp rett ved rødt lags lobby (8 m fra sørflagget), mens Trapp øst kommer opp 15 m fra
blått lags side. Til gjengjeld lander atriumtrappa (løp 3) nærmest Nordbasen. Sjekkskriptet regner rutelengdene fra hver
spawn til motstanderens flagg og krever under 20 % forskjell; justeres med dørplassering, ikke med vegger som ikke finnes.

**Klatreruter (planlagt, runde 5):** Kamps mantling tar kanter 0,55–2,2 m over føttene. Kandidater: (a) Ryddekroken –
kiosktaket (2,1 m) → tårnets hylle → sittegalleriet på P2; (b) klatrehallen – buldreveggene som trinn fra P1 opp til et
utkikksgalleri mot atriet på P2/P3 (nord), med motstykke i studentarbeidsplassenes bokhyller i sør (P2 → P3-kanten);
(c) knust glassrekkverk = hopp ned en etasje. Alt som skal klatres på får `ComponentTags` `Climb` (Kamps bestilling).

### Siktlinjer og dekning

- **Lengste siktlinjer:** vest-/østkorridoren 40 m; tvers over atriet 26–36 m gjennom glass (rekkverk og bokser er
  gjennomsiktige, men gir dekning mot kuler når de blir skuddfaste/knusbare). Ingen sti skal ha mer enn **42 m** fri sikt
  (innendørsregelen, erstatter 80 m ute), heller ikke 1,0 m til siden av midtlinjen i korridorer (de er bare 2,3 m brede).
- **Dekning i korridorene:** betongsøyler Ø 0,5 m med svarte ringer (bilde 22) på atriesiden hver 6. m, skap (grønne/svarte
  oppbevaringsskap, «44 skap», «56 skap» på tegningene) mot veggen, sofagrupper og info-skjermer ved knekkene.
- **Dekning i atriet:** glassboksenes hvite undertak og svarte rammer, trappevangene, plantekasser, sittetrappa.
- **Spawn skal aldri synes** fra ringkorridoren, trappene, lobbyene eller rutene R1–R6 (2D-sikt gjennom åpne dører og
  glass, regnet av `check_helgasetr.py`).
- **Lærdom tatt med:** åpninger er siktlinjer (dører, glass); sjekk både midtlinja og ved siden av; usynlige grenser trengs ikke
  (ytterveggene er ekte), men takluka og glasstaket må tåle gripekrok.

### Hva Spillbarhet og Kamp trenger (bestilt i `bestillinger.md`)
- **Data i `GSHelgasetr.h`** (Grafikk eier, generert fra `Tools/Blender/helgasetr_plan.py`): etasjer, sonene/stedsnavn,
  baser og spawn, `LootSpots()` (ca. 60 plasser på P1–P4 med nivå), `Terminals()` (PC-er, printere, kameraer med navn),
  `SoundZones()` (romklang), `FloorRects(nivå)` (for minikart og sjekker), `DoorSlots()` og `Breakables()` (fra runde 2).
- **`GSCampus.h` peker på Helgasetr** (`GSCampus::bHelgasetr = true`): `Bases()`, `SpawnPoints()`, `PlayArea()`,
  `Buildings()` (fem soner: nordfløyen, sørfløyen, vestfløyen, østfløyen, atriet, med etasjehøyde 4,3 m så «N. etasje» blir
  riktig for alle seks planene) og `Roads()` (korridorene på P4, for minikartet) virker uten at andre lag endrer noe. Loot,
  PC-er og kameraer forsvinner likevel til Spillbarhet leser de nye listene (de gamle kom fra byggene).

## 6. Interaktivitet (Andreas: «fiks så mye på mappet er interaktivt»)

Prinsipp: **alt som kan åpnes, knuses, sprenges eller flyttes er en egen bit med egen ID**, aldri en del av den store
bygningsmodellen. Bygningsmodellen er bare det som aldri endrer seg. Derfor lages glassfelt, dører, lettvegger og
møbler som moduler i Blender og settes ut av C++ fra lister i `GSHelgasetr.h` – allerede i råbygget, så runde 2 bare
bytter klossene mot ekte oppførsel.

| Ting | Hvor | Grafikk (modell, plassering, fysikk, knusing) | Spillbarhet (bruk, loot, regler, nett for bruk) | Kamp (lyd, skudd/eksplosjon, spark) | Runde |
|---|---|---|---|---|---|
| **Dører** (tre, glass med kodelås, dobbeltdører, branndører i trapperom) | alle rom | dørblad, karm, kodelås-panel, ødelagt dør; `DoorSlots()` med type | `AGSDoor`: åpne/lukke, låst, dirke opp kodelås (tid, avbrytbar), replikert | spark opp, skyte opp låsen, smell- og knirkelyd | 2 |
| **Knusbart glass** – grupperom, glassbokser, glassrekkverk, glassdører, skillevegger | P1–P4 | glassmoduler med ID, sprekker → hull → knust, skår på gulvet, `AGSMapState` (replikert tilstand) | tilbakestill ved kampstart | kall ved kuletreff og eksplosjon, knuselyd | 2 |
| **Flyttbar dekning** – stoler, puffer, små bord, søppelkasser | kantina, mesaninen, grupperom | fysikkmøbler (lav vekt, replikert bevegelse, tak på antall) | bære/sette ned (barrikade foran dør) | slag/skudd dytter dem, lyd ved velt | 2 (dytte/velte), 4 (bære) |
| **Sprengbare lettvegger** (gips) | utvalgte vegger mellom rom og korridor | panel med hel/ødelagt modell, støv, biter | – | kall fra eksplosjon | 2 |
| **Lysbrytere** | hvert rom | lysgrupper per rom, brytermodell, tilstand i `AGSMapState` | bruk-handling (E) | klikk | 3 |
| **Skjermer / TV-er** (info-skjermer, «Iskaffe», tavleskjermer) | kantina, rom | skjerm som lyser, knust skjerm ved treff | – | treffkall, lyd | 3 |
| **Garderobeskap og oppbevaringsskap** | korridorene | skap med dør som åpner seg | loot inni, bruk | lyd | 3 |
| **Kjøleskap og kiosk i Ryddekroken** | P1 | kjøleskapsdører, lys inni | mat/drikke-loot | lyd | 3 |
| **Salatbaren** | P1 | boller, glasskjerm (knusbar) | mat-loot | – | 3 |
| **Piano** | P1 loungen | modell, lokk | bruk = spiller | pianolyd (Tools/Audio) | 4 |
| **Bordtennisbord** | P1/P2 | bord, ball (fysikk) | bruk = server ballen | pling-pong | 4 |
| **Hengelamper** | atriet | svinger og flimrer når de treffes, kan knuses | – | treffkall, lyd | 4 |
| **Heiser** (sør og øst) | P1–P4 | kabin og dører som går, tilstand i `AGSMapState` | knapper, bruk | pling, motor | 5 |
| **Brannalarm** | trapperom, korridorer | knapp, blinkende lys | bruk = alarm (støy, alle hører) | alarmlyd | 5 |

### Ødeleggelse (granater og eksplosiver)
- **Kan ødelegges:** glass i grupperom, glassbokser, glassdører og rekkverk; dører (alle); utvalgte lettvegger mellom
  grupperom/laber og korridor (merket i dataene, likt fordelt på begge sider); møbler (stoler, bord, puffer, skjermer, lamper).
- **Kan aldri ødelegges:** bærende vegger (betongkjerner, trapperom, yttervegger), søyler, dekker og gulv, alle trapper og
  repos, glasstaket og fasadeglasset (laminert; ellers kan man falle ut av kartet), veggene i baserommene mot korridoren
  (dørene kan sprenges, veggen ikke – så hver base beholder tre kjente innganger).
- **Teknikk:** Chaos Geometry Collections krever at noen bruker bruddverktøyet i editoren – det passer ikke en løkke som
  jobber uten editor. I stedet: **ferdig brukket modell** fra Blender (glass: 3 sprekke-/hullvarianter og 8–12 skår per
  panelstørrelse; lettvegg: panel med hull og 10–15 gipsbiter; dør: knust blad). Når noe går i stykker: serveren setter
  tilstanden i `AGSMapState` (replikert liste over ødelagte ID-er, `FFastArraySerializer`); alle maskiner bytter helt mot
  ødelagt modell (ISM-instansen skaleres til 0, ødelagt modell vises) og lager **lokale** fysikkbiter og støv/glass-partikler
  som ikke kolliderer med spillere og forsvinner etter 15 s (tak 150 biter per maskin). Skår og gipsrester blir liggende som
  billige instanser. De som kommer inn senere får bare ødelagt utseende, ikke bitene på nytt. Alt tilbakestilles ved kampstart.
- **Kobling:** Kamp kaller to statiske funksjoner (én linje hver): `GSMapState::ApplyBlast(World, OriginCm, RadiusCm,
  Strength)` fra eksplosjonen i `GSProjectile`, og `GSMapState::ApplyBulletHit(World, Hit, Damage)` fra kulesporet i
  `GSBallistics`. Begge gjør ingenting på klienter. Lyd spilles av Kamp ut fra hendelsen (`OnBroken`-delegat med type og sted).

## 7. Lys

- **Dagslys:** sagtak over hele atriet (X −15,4…+15,4, Y −20,3…+20,3), mønene øst–vest, glass i de bratte flatene, hvitt
  stålfagverk (Warren-fagverk) under (bilde 03, 10, 15, 26). Sola fra `GSEnvironment` kommer inn og legger skygger av
  fagverket på glassboksene. Glasstaket skal blokkere `Visibility` så regn og snø stopper der (`IndoorFactor`).
- **Hengelamper:** store runde LED-ringer/skiver Ø ca. 2,4 m (bilde 02, 07, 17) i lange vaiere i ulike høyder over kantina og
  i atriet (8–10 stk.); mindre runde hvite pendler Ø 0,6 m i grupper over bordene (bilde 08). Lysende overflate (emissive) +
  ett rektangellys per stor lampe, uten skygge.
- **Downlights:** runde innfelte lys i de hvite undertakene under hver etasjekant og glassboks (bilde 06, 07) – emissive, Lumen
  tar resten. Korridorene: runde downlights hver 3. m, ett punktlys uten skygge per ca. 10 m.
- **Grupperom og undervisningsrom:** lysrør/paneler i himlingen; fra runde 3 egne lysgrupper per rom (lysbrytere).
- Kveld/natt: alt innelys står på (bygget er åpent til kl. 23); glasstaket blir mørkt.

## 8. Materialer og møbler, koblet til bildene

| Flate / ting | Materialspor i Blender | Bilder |
|---|---|---|
| Terrazzo, lys grå med korn – kantinegulvet P1 | `Terrazzo_In` | 04, 08, 12, 13 |
| Oransje gulv på mesaninen og sittetrappa (epoxy/linoleum) | `HelgOrange_In` (ny) | 06, 14, 16, 25 |
| Gul/oransje lakk – atriumtrappa, Ryddekroken | `HelgYellow_In` (ny) | 02, 05, 17, 27 |
| Lys parkett (eik, stavparkett) – korridorene | `WoodLight_In` | 22 |
| Brunt linoleum/kork – grupperom | `Floor_In` | 20, 21 |
| Hvit gips – vegger, undertak under dekkekanter og bokser | `PlasterWhite_In` | 07, 11 |
| Mørke trelameller på dekkekantene, treoverkant på rekkverk | `Wood_In` | 01, 02, 25 |
| Glassrekkverk (klart, med treoverkant), glassvegger, bokser | `Glass` (eget nett) | 01, 04, 25 |
| Svarte stålrammer (bokser, dører, båser), svart rekkverk | `MetalDark_In` | 05, 07, 08 |
| Hvitt stålfagverk og takbjelker | `PaintWhite_In` (ny) | 03, 10, 15, 26 |
| Betongsøyler (lys, glatt) med svarte ringer | `ConcreteSmooth_In` + `MetalDark_In` | 22 |
| Grønne og svarte oppbevaringsskap | `PaintGreen_In`, `Gunmetal_In` | 22 |
| Mosesirkler på kaféøya (grønne) | ny tekstur senere (runde 3) | 04, 09, 14 |
| Fasade ute: Kebony-kledning (grå-brun tre), glass/alu nederst | `Wood`, `MetalDark` | (vinnerforslag) |

**Møbler (runde 3–4, Blender `props.py`/ny `props_helgasetr.py`, instanser):** svarte kvadratiske kantinebord på søylefot,
runde svarte bord, trestoler (bøyd kryssfiner på svarte ben), lange hvite bord med svarte ben, plantekasser (svarte, med
grønne planter), svarte stålbur-båser med grønne sofaer og treplater (bilde 08), turkise/grønne sofaer og puffer, oransje
puffer, lilla puffer (P6), runde grønne bord rundt et tre (P6), røde sofabåser med svarte bord (fellessoner, bilde 01),
salatbar med trebjelker, kjøleskapsrekke og isboks (Ryddekroken), søppelsortering (fire farger), piano, bordtennisbord,
info-skjermer, grupperom (svarte bord, kontorstoler, whiteboard, skjerm), undervisningsrom (hvite bord, svarte stoler, kateter
med PC, lerret, bilde 24), skap i korridorene, vasker (bilde 22), runde blå matter.

## 9. Ytelse (RTX 3050 Laptop 6 GB, 16 GB RAM)

Mål: **60 bilder/s i 1080p på Middels** (TSR), aldri under 45 i atriet. Budsjett per bilde (GPU ≈ 14 ms):

| Post | Budsjett | Hvordan |
|---|---|---|
| Lumen GI og refleksjoner | 4–5 ms | programvare-strålesporing, ingen «hit lighting»; vegger ≥ 0,15 m og bygget delt i biter ≤ ca. 26 m (avstandsfelt med god oppløsning, ellers lekker lyset) |
| Skygger (VSM) | ≤ 2 ms | bare sola kaster skygge; innelys uten skygge (maks 4 med skygge senere, f.eks. salatbaren) |
| Nanite grunnpass | ≤ 2,5 ms | bygget og møbler som Nanite; glass og fysikkmøbler ikke |
| Gjennomsiktig (glass) | ≤ 1,5 ms | ett enkelt glassmateriale, ingen Lumen-refleksjon på glass, maks ca. 3 lag glass i et vanlig utsnitt |
| Etterbehandling | ≈ 1,5 ms | som i dag |

Tall: lys ≤ 48 (alle uten skygge unntatt sola, `MaxDrawDistance` 40 m), ikke-Nanite-tegnekall ≤ 1 500, fysikkmøbler ≤ 60
som simuleres samtidig, løse biter ≤ 150 per maskin, glassmoduler ≈ 1 200 instanser (få komponenter), teksturer innenfor
dagens pool. Måles med `stat unit` / `stat gpu` fra Plan 4 nord-korridor ser mot sør, og fra kantina ser opp, før og etter.

## 10. Lyd og kuler (Kamp)

- **Romklang** (`SoundZones()`): atriet (stort og hardt: terrazzo, glass, ca. 2,5–3 s), mesaninen (2 s), ringkorridorene
  (parkett, 1–1,2 s), trapperommene (betong, 2 s, tydelig ekko), grupperom (0,4–0,6 s), undervisningsrom (akustikkhimling,
  0,7 s), klatrehallen (1,8 s). Hver sone er en boks per plan med navn og RT60.
- **Gjennomslag:** glassrekkverk og glassvegger 10–12 mm herdet/laminert (kula går gjennom, glasset knuses fra runde 2);
  lettvegger av gips (ca. 100 mm: 2 × 12,5 mm gips på hver side av stålstendere) – gjennom; dører av tre (40 mm) – gjennom;
  skap av stål (1 mm) – gjennom; betongkjerner, søyler (Ø 0,5 m), dekker og yttervegger – stopper. Fysiske materialer per
  materialinstans kommer i runde 2 (`PM_GS_Glass`, `PM_GS_Gips`, `PM_GS_Betong`, `PM_GS_Tre`, `PM_GS_Stal`, `PM_GS_Stein`
  (terrazzo), `PM_GS_Stoff`).
- **Fottrinn:** terrazzo (hardt), parkett (tre), linoleum, trinn i stål/gult belegg (metallisk klang), betong i trapperommene.

## 11. Teknikk – hvorfor en blanding

| | Blender-modeller (som Hovedbygningen) | C++-klosser/instanser (som gamle campus) |
|---|---|---|
| Kvalitet | ekte trinn, buede hjørner, sagtak, fagverk, avfasinger, UV | rette bokser, ingen buer |
| Ytelse | Nanite, få tegnekall, godt for Lumen hvis delt i biter | tusenvis av bokser, svake avstandsfelt |
| Kontroll uten editor | rendres til bilder og strålesjekkes i Blender | bare tall |
| Interaktivt | nei – én modell kan ikke knuses i biter | ja – hver instans har en ID |

**Valg: blanding med én felles datakilde.** `Tools/Blender/helgasetr_plan.py` holder aksene, etasjene, rom, dører, hull,
trapper, bokser, baser, spawn og plasser. Fra den lager
1. `building_helgasetr.py` den faste strukturen i Blender (dekker, vegger, søyler, trapper, trapperom, boksenes gulv/tak/
   rammer, fasade, tak og fagverk), delt i biter per plan og kvadrant (`SM_Bygg_Helgasetr_P4_NV` osv.), Nanite og
   «complex as simple»-kollisjon (finnes fra før for `SM_Bygg_`), og fast glass (fasade, tak) i `SM_Glass_Helgasetr_*`;
2. `gen_helgasetr.py` `Source/Gloshaugen/Public/GSHelgasetr.gen.inl` (generert, aldri rediger for hånd – som
   `GSItemCatalog.gen.inl`);
3. `AGSHelgasetrBuilder` (C++) setter modellene på plass og legger ut alt som skal være interaktivt som instanser med ID:
   glassmoduler (rekkverk, boksvegger, glassdører), lettvegg-paneler, dørplasser, lys, lagfargene ved basene, og senere møbler.
4. `check_helgasetr.py` (data) og `check_helgasetr_walk.py` (stråler i Blender) sjekker begge.

## 12. Hvordan det nye kartet erstatter det gamle
- De gamle filene slettes ikke ennå: `GSCampusBuilder.*`, gamle data i `GSCampus.h`, `buildings.py`,
  `building_hovedbygningen.py`, `check_campus.py`, `check_walkable.py` og modellene i `SourceArt/Models` står.
- `GSCampus::bHelgasetr = true` velger kart. `UGSWorldSubsystem` lager `AGSHelgasetrBuilder` (arver fra
  `AGSCampusBuilder`, så `GSEnvironment` og hjelpefunksjonene virker) i stedet for campusbyggeren.
- `check_campus.py` tester bare de gamle dataene (frosset). Løkken kjører `check_helgasetr.py` og
  `check_helgasetr_walk.py` i stedet. Når Andreas har spilt Helgasetr og sagt ja, slettes campusdelen i en egen opprydning
  (spør først: det er Andreas' innhold).
