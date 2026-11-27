**MÅ FIKSES** (to små rettinger: kodelås-tieren havner i korridoren, og én lootplass ligger inne i en trapp)

# Kontroll – Spillbarhet runde 6a/6b (3241a3a): loot, PC-er, kameraer og stedsnavn i Helgasetr, ResetForMatch

Kontrollert ved å lese commiten (`git show 3241a3a`, hele filer med `git show 3241a3a:<fil>`), ikke arbeidsmappen. `GSMenu.cpp`
(ikke commitet, avhenger av Kamps `GSAudio::PlayUI`) er lest med `git diff`. `Build.bat` er ikke kjørt (byggeren: Succeeded).
Tallene er regnet ut på nytt med to skript som leser `catalog.py` og `GSHelgasetr.gen.inl` direkte (eksakte sannsynligheter,
ikke trekninger), både for kartdataene i commiten og for Grafikks uncommittede 4b-data i arbeidsmappen.

## Andreas' faste regel
- **Forbedret:** `RandomLoot` (ammo etter kaliberets våpen, gamle kampus uendret), `SpawnPickups` (kartdata + gulvspor i stedet
  for bygg-løkka), `HelpPlaceName` (rom og etasje), `CleanWorldForMatch` (kartet tilbakestilles), tekstene til kort og dirk.
- **Nytt:** loot i Helgasetr med steder/dybde/balanse, 4 PC-er + 2 printere + 8 kameraer fra kartdata, `gs.loot.stats`,
  testromknappen og menylyder (de to siste bare i arbeidsmappen).
- Oppfylt.

## Tallene, regnet ut på nytt
**Kartet (58 plasser: 41 vanlige, 13 bedre, 4 kasser – planens «46» var feiltelt):**
- Med kartdataene i **commiten**: 0 kodelåsdører har `bLocked` → ingen oppgradering, balanse **rød 71,7 / blå 70,8**, 0 justert.
- Med **arbeidsmappens** 4b-data: 24 `glass_kode`-dører har `bLocked` (byggerens «11» er antallet *plasser* som ble oppgradert,
  ikke dører). 11 plasser går Tier 0 → 1 (13 → 24 bedre), balanse **rød 86,3 / blå 83,6** (3,1 %), 0 justert. Stemmer nøyaktig
  med byggerens hodeløse logg, så den kjørte på arbeidsmappens data.
- Steder: korridor 39, kantine 11, medisinsk 4, kasser 4. Etasjer: 12 / 10 / 16 / 20. Ingen plass nærmere enn 0,8 m en dør.
- Kassene: Kantina (dybde 0,79 → **Rare**, like under 0,8), Ringkorridoren 3. etasje (0,96 → Epic), Laberadene (1,0 → Epic),
  Østfløyens rom (1,0 → Epic). IMPROVEMENTS 884 nevner tre av fire; greit.

**`RandomLoot` (Max Legendary, eksakt):**

| Sted | Våpen | Medisin | Ammo | Rakett/.50 | 9 mm topp 3 | Epic+ D0 → D1 (gang) |
|---|---|---|---|---|---|---|
| Corridor, Min Common | 30,0 % | 18,0 % | 22,0 % | 0 | ja (1.) | 8,6 → 21,7 % (2,51×) |
| Corridor, Min Uncommon | 30,0 % | 18,0 % | 22,0 % | 0 | ja (1.) | 13,5 → 26,8 % (**1,99×**) |
| Corridor, Min Rare | 30,0 % | 18,0 % | 22,0 % | 0 | ja (2.) | 26,8 → 39,2 % (1,46×) |
| Medical, Min Common | 10 % | **60 %** | 10 % | 0 | ja | 6,5 → 19,7 % (3,03×) |
| Canteen, Min Common | 18 % | 15 % | 20 % (mat 35 %) | 0 | ja | 5,0 → 12,8 % (2,54×) |
| Crate, Min Rare / Epic | 45 % | 15 % | 2 % | 0 | – | 36,9 → 52,8 % / 98 % (flatt) |

- Alle gruppeandeler og ammo-kravene holder. Rakett (Carl Gustaf) og .50 (Barrett) er Mythic, så de får vekt 0 med Max Legendary.
- Epic+-doblingen: kravet holder for Min Common overalt, men **ikke** for Min Uncommon i korridor (1,99×) og kasse (1,95×), og
  aldri for Min Rare/Epic. Det er planens feil, ikke byggerens: når Epic+ allerede er 27–44 % ved dybde 0, kan den ikke dobles
  med en multiplikator per sjeldenhetstrinn. Byggerens melding («bare Rare-minimum») er litt for snill; Uncommon ligger like under.
  Godtas – virkningen (dypere = klart bedre) er der.
- `PrintDrone` (Epic, `kind=drone`) faller ut fordi unntaket sjekker `Kind == drone` og ikke bare Id `Drone`. Riktig etter
  realismeregelen (den gir veggsyn). Den kan fortsatt printes. Godtas.
- Dirkesett-teksten: `GSDoor.cpp` (3241a3a) linje 677–680 – dirken virker **bare** på `GlassCode`-dører; tre-/«låst»-dører sparkes,
  alarmdøra kan ikke åpnes. Byggerens «kasser og kodelåser» er altså riktig, og planens «dører» ville vært feil. Godtas.

## Gammel kampus (`bHelgasetr = false`)
Lest linje for linje: med `(Campus, Mythic, 0)` er `Excluded` alltid usann, `Usable` = `GroupTotal > 0` (alle Campus-vekter > 0),
`RarityW` = `RarityWeight` (grenen `Depth > 0` slår ikke til), og rekkefølgen på `Rng`-kallene er den samme (gruppe, sjeldenhet,
indeks). Bygg-løkka i `SpawnPickups`/`SpawnTerminals` og `HelpPlaceName` er flyttet uendret inn i `else`. `ResetForMatch` i
`CleanWorldForMatch` gjelder begge grener, men `AGSMapState::Get` returnerer bare `nullptr` når kartet ikke har noen
`AGSMapState`. Oppfører seg likt.

## Nett (vert / klient / sen klient)
- Loot: bare serveren trekker; `Place`/`MaxRarity`/`Depth` er serverfelt og brukes av `Respawn` og `ResetForMatch` → `Roll()`.
  `ItemId` replikeres som før. Sen klient får gjeldende `ItemId`. OK.
- Terminaler/kameraer: `Kind` (`OnRep_Kind`), `Label`, `SweepDeg` er replikert (`GSTerminal.cpp` 65–66, 227–228). Samme mønster som før.
- `ResetForMatch`: `DoResetForMatch` har `HasAuthority`-vakt, tømmer fast-array `Parts` + `MarkArrayDirty` + `ForceNetUpdate`, så
  klient og sen klient får hele dører/glass via replikering. To kall (vår + Interaktivs fase-overgang) er ufarlig.
- `HelpPlaceName` kjører lokalt på kartdata som er like overalt. Ingen ny replikering trengs.
- Menyen: testromknappen er bare aktiv og bare virksom med `HasAuthority()` og Explore (sjekket både ved tegning og ved klikk).

## Funn

**F1 – Kodelås-tieren havner i korridoren, ikke i de låste rommene (`GSGameMode.cpp`, `SpawnPickupsHelgasetr`, løkka over
`DoorSlots()`).** Avstandssjekken er 2D og uten side. Med arbeidsmappens data ligger **alle 11** oppgraderte plasser i
Ringkorridoren (3. og 4. etasje) eller på Mesaninen, *utenfor* grupperommene, 1,2–3,9 m fra døra – det finnes ingen lootplasser
inne i grupperommene i det hele tatt. Interaktivs ønske (3) var «bedre loot i de låste grupperommene – risiko gir belønning»; nå
får man belønningen uten å åpne noe, og antallet bedre plasser i korridorene nesten dobles (13 → 24). Bestillingen er likevel
merket LEVERT. Det følger planens ordlyd, men ødelegger meningen.
- **Gjør:** oppgrader bare når plassen ligger på romsiden av døra (f.eks. `HelgasetrRoomAt`/`ZoneAt` for plassen er ikke
  korridor/mesanin/atrium, eller fortegnet til `(plass − dør) · dørnormal` peker inn i rommet), slik at det med dagens data blir
  0 oppgraderinger. Skriv en bestilling til Grafikk om 1 lootplass inne i hvert (eller annethvert) låst grupperom, og sett
  Interaktiv (3) tilbake til ÅPEN med «venter på lootplasser i grupperommene». Logg antall oppgraderte i Helgasetr-loot-linja.
- Merk i tillegg: koden leser `bLocked` fra kartdataene, men `AGSDoor::MakeDefault` (`GSDoor.cpp` 327) låser *alle* `GlassCode`-
  dører uansett. Med 4b-dataene er de like; skriv i en kommentar at det er dataflagget som teller her.

**F2 – Én lootplass ligger inne i den gule trappa (`GSHelgasetr.gen.inl` LootM, plassen (11,0, 3,0, 9,9), Tier 0).** Den ligger
inne i trappeboksene (11,625, 3,15, z 9,89–11,72) og (11,625, 2,85, z 9,89–11,90), `Coll_Solid`, både i commitens og i
arbeidsmappens data. Sporet starter på 11,4 m, altså *inne i* trappa. Enten blir treffet startpunktet (tingen henger på
12,0 m inne i/like over trinnet), eller sporet går gjennom og tingen legges på gulvet inne i trappa. Begge er feil, og loggen
teller den ikke som «uten gulv». Byggerens «0 uten gulv» beviser derfor ikke at alt ligger fritt.
- **Gjør (vår kode):** forkast treff med `Hit.bStartPenetrating`, og sjekk den endelige plassen med en liten
  `OverlapAnyTestByObjectType` (`ECC_WorldStatic`, kule/boks ~15 cm). Hopper den over, logg «inne i noe» og tell det i
  sammendraget.
- **Gjør (bestilling til Grafikk):** flytt plassen ut av trappa (f.eks. til x 8,5) eller fjern den; vi endrer ikke dataene selv.

## Merknader (ikke blokkerende)
- **IMPROVEMENTS.md i commiten** har også avsnittene til Grafikk 4b (800–824), Bots 1a (840–855) og Kamps lydrunde (662–677),
  og 897/898 for `GSMenu.cpp`. Ingen av dem har koden sin i denne commiten. Ikke ta med andre lags linjer neste gang
  (`git add -p IMPROVEMENTS.md`). Det samme gjelder at Kamps menylyd-bestilling er merket LEVERT 06b, men `GSMenu.cpp` er ikke
  commitet; skriv «LEVERT, commites med Kamps lydrunde».
- `GSHelp.cpp`: den gamle doc-kommentaren til `HelpPlaceName` («Where a world point ...») står nå over `HelpRoomAt`. Flytt den ned.
- Romoppslaget finnes to ganger (`GSGameModeImpl::HelgasetrRoomAt` og `HelpRoomAt`). Planen sa én; greit fram til Grafikks
  `PlaceName()` kommer, men da må begge byttes.
- `GSMenu.cpp` (arbeidsmappen): ser riktig ut. Deaktiverte knapper registrerer ikke treff (`Button` linje 660), så de klikker ikke.
  Lyd før feltfokus og `Dlg.*` er greit. `UI_Back` bare når noe faktisk lukkes. Testromknappen er vakta både ved tegning og
  ved klikk. Den kan commites sammen med Kamps `GSAudio`.
- PIE-punktene i «Ferdig når» (E på bandasje og PC, kamerabilde, Gå til testrom, knust glass helt etter ny kamp, radiomelding
  med rom) er bare lest i koden, ikke prøvd. Andreas vil ikke teste nå; en hodeløs selvtest som logger `HelpPlaceName` for de
  fire basene/kassene og `ResetCount` etter `RestartMatch` ville gjort dem bevisbare.

## Forslag til neste runde
1. F1 + F2 (en halvtime), og lootplasser i de låste grupperommene via Grafikk, slik at «risiko gir belønning» faktisk virker.
2. En hodeløs Helgasetr-selvtest i GameMode (som Interaktivs): spor og overlapp for hver lootplass, PC og kamera (står ikke i
   vegg/trapp, kameraet under taket med fri sikt), og `HelpPlaceName` for kjente punkter – logget som én linje PASS/FAIL.
3. Bots' punkt (1)–(5): `GSBots.h` finnes nå i arbeidsmappen, så `bBotsAvailable` kan slås på når Bots 01a er godkjent.

## Lærdom
- Et gulvspor som starter inne i en kloss «treffer» likevel. Sjekk `bStartPenetrating` og overlapp på den endelige plassen,
  ellers beviser «0 uten gulv» ingenting om vegger og trapper. (Spillbarhet runde 6)
- «Nær en dør» må alltid si hvilken side av døra. Avstand alene gir belønning på feil side. (Spillbarhet runde 6)
- Når kontrollen kjører mot arbeidsmappens data i stedet for commitens, må rapporten si det, fordi tallene endres med andre lags
  uncommittede data. (Spillbarhet runde 6)
