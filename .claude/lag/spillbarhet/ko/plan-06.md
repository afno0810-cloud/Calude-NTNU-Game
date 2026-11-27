# Plan 06 (Spillbarhet): Helgasetr blir komplett – våpen, ammo, medisin, PC-er og kameraer der de hører hjemme

Delt i to runder som bygges hver for seg: **06a** (loot) og **06b** (PC-er, kameraer, stedsnavn, bestillinger).
06b endrer ingen funksjoner som 06a endrer, bortsett fra at begge legger en gren i `GSGameMode.cpp` (ulike funksjoner).

Grunnlag lest i koden 27. nov. (etter `bebbf7c`): `GSCampus::bHelgasetr = true`. `AGSGameMode::SpawnPickups()` og
`SpawnTerminals()` går fortsatt gjennom `GSCampus::Buildings()`, og alle fem Helgasetr-sonene har `bEnterable = false`.
Derfor kommer **ingen loot, PC-er eller kameraer** på kartet. Det eneste som lages er den mytiske kassa på (0, −7,5, 0,85)
(`bCustomMain` er usann, så den havner på (0, 4, 0,35)). Spilleren har bare startpistolen.
`GSHelgasetr::LootSpots()` har 58 plasser: 46 av typen Tier 0, 13 av Tier 1 og 4 av Tier 2. Alle står på gulv (sjekket mot
`FloorRects`). Fordelingen er skjev: 8 Tier 1 på sørsiden (rødt lag) mot 5 på nordsiden (blått).
`Terminals()` har 4 PC-er og 2 printere med navn, og `Cameras()` har 8 kameraer.
Botene er ikke levert ennå: `bots/logg.md` har bare «oppsett», og `GSBots.*` finnes ikke.

---

## 06a – Loot i Helgasetr

**Mål:** Du finner nye våpen, riktig ammo, bandasjer og medisin rundt i Helgasetr. Det vanlige ligger nær basene og det
bedre lenger ut, mot midten. Medisinsk utstyr ligger i ferdighetsrommene og mat i kantina. Begge lag har like mye å hente.

**Forbedrer (gammelt):**
- `GSItems::RandomLoot`: ammo trekkes nå etter hvor mange våpen som bruker kaliberet. Dette retter Kamps kontrollfunn om at
  rakett, 40 mm og .50 var 3× vanligere enn 9 mm.
- `AGSGameMode::SpawnPickups`: leser kartdataene i stedet for de gamle byggene, og legger tingene på gulvet med et spor.
- Den mytiske kassa som svevde i kantina, fjernes på Helgasetr.

**Nytt:**
- Lootsteder etter rom (korridor, medisinsk rom, kantine, kasse) med egne vekter.
- Sjeldenheten blir bedre jo lenger fra basene en plass ligger.
- Automatisk balanse mellom lagene.
- Plasser ved låste kodelås-dører blir én tier bedre (Interaktivs ønske 3).
- Konsollkommandoen `gs.loot.stats` regner ut fordelingen.

**Hvorfor nå:** Hovedagentens prioritering 27. nov. kl. 10.10 (plan-08 rett etter at råbygget er levert) og Andreas'
punkt 2. Uten dette er bot-kampen og online-kampen bare startutstyr.

**Filer:** `GSItems.h/.cpp`, `GSPickup.h/.cpp`, `GSGameMode.cpp`. Leser `GSHelgasetr.h`, men endrer den ikke.

**Steg:**
1. **`GSItems.h`:** legg til `enum class EGSLootPlace : uint8 { Campus, Corridor, Medical, Canteen, Crate };` og en ny
   overbelastning:
   `FName RandomLoot(FRandomStream& Rng, EGSRarity MinRarity, EGSRarity MaxRarity, EGSLootPlace Place, float Depth);`
   Den gamle signaturen blir stående og kaller den nye med `(Min, Mythic, Campus, 0)`, så den gamle kampuskoden oppfører seg
   helt som før.
2. **`GSItems.cpp`, ny `RandomLoot`** (samme tre trinn som i dag):
   - **Gruppevekter per sted** i rekkefølgen Weapon, Armor, Helmet, Food, Drug, Medicine, Throwable, Gadget, Ammo:

     | Sted | Weapon | Armor | Helmet | Food | Drug | Medicine | Throwable | Gadget | Ammo |
     |---|---|---|---|---|---|---|---|---|---|
     | Campus | 20 | 7 | 5 | 20 | 8 | 13 | 7 | 8 | 12 (som i dag) |
     | Corridor | 30 | 6 | 5 | 4 | 3 | 18 | 8 | 4 | 22 |
     | Medical | 10 | 2 | 2 | 4 | 6 | 60 | 2 | 4 | 10 |
     | Canteen | 18 | 3 | 2 | 35 | 2 | 15 | 5 | 0 | 20 |
     | Crate | 45 | 12 | 8 | 0 | 3 | 15 | 12 | 3 | 2 |
   - **Unntak** når `Place != Campus`: hopp over Id-ene `Parachute`, `Drone` og `Oyet`. Fallskjermen har ingen bruk inne,
     og dronen og Øyet gir veggsyn, som strider mot realismeregelen. `MaxRarity` gjelder også (Helgasetr bruker `Legendary`,
     så ingenting mytisk dukker opp).
   - **Dybde:** sjeldenhetsvekten for nivå `R` ganges med `FMath::Pow(1.f + 0.6f * Depth, R)`. Ved `Depth` 0 er vektene
     uendret. Ved `Depth` 1 ganges Legendary med 6,5.
   - **Ammo (gruppen Ammo, `Place != Campus`):** hopp over sjeldenhetstrinnet. Vekten for `Ammo_<kal>` er summen av
     `RarityWeight[våpen.Rarity]` for alle våpen i utvalget (innenfor Min…Max og ikke unntatt) med `Caliber == kal`.
     `AmmoBox` får gjennomsnittet av de vektene som ikke er null. Ammo uten våpen i utvalget får vekt 0, så det kommer
     ikke rakett-ammo når Carl Gustaf er mytisk. `MinRarity` gjelder ikke ammo (ammo er Common/Uncommon), bortsett fra
     på stedet Crate, som har vekt 2.
   - **Konsollkommando** (`FAutoConsoleCommand`, `gs.loot.stats [N=2000]`): trekker N ganger for hvert sted, med Min
     Common/Uncommon/Rare og Depth 0 og 1, og logger andelen per gruppe, andelen Epic+ og de tre vanligste ammo-kaliberne.
     Bruk den navngitte namespace-en `GSItemsImpl` (lærdom om unity-bygg).
3. **`GSPickup.h/.cpp`:** nye felt `EGSLootPlace Place = EGSLootPlace::Campus;`, `EGSRarity MaxRarity = EGSRarity::Mythic;`
   og `float Depth = 0.f;`. Feltene er bare på serveren, uten replikering. `Roll()` kaller den nye `RandomLoot` med disse
   feltene. Respawn og `ResetForMatch` bruker dem dermed av seg selv.
4. **`GSGameMode.cpp`, `SpawnPickups`:** øverst, `if constexpr (GSCampus::bHelgasetr) { SpawnPickupsHelgasetr(Rng); return; }`.
   Ny privat funksjon i en navngitt namespace `GSGameModeImpl` (eller som medlem, med deklarasjon i `GSGameMode.h`):
   - **Sted per plass:** finn den *minste* `SoundZones()`-boksen som inneholder punktet (Z med 0,3 m slakk).
     - Navnene `Laberadene` og `Østfløyens rom` gir Medical. Dette er helsefagbygget: ferdighetsrom og laber.
     - `Kantina` og `Studentarbeidsplassene` gir Canteen.
     - Alt annet gir Corridor.
     - Tier 2 gir alltid Crate.
   - **Dybde:** `D = min(3D-avstand til flagget i Sørbasen, til flagget i Nordbasen)` i meter, fra `GSHelgasetr::Bases()`.
     Regn ut `Depth = clamp((D − 12) / 16, 0, 1)`.
   - **Tier til sjeldenhet:**
     - Tier 0 gir Min Common.
     - Tier 1 gir Min Uncommon.
     - Tier 2 gir en låst kasse med Min Rare. Er `Depth ≥ 0,8`, gir den Epic.
     - Max er alltid Legendary.
   - **Kodelås-rom:** står plassen 4 m eller mindre fra en `DoorSlots()`-dør med `Kind == 1` (glass_kode) og `bLocked` på
     samme `Level`, øk tier med 1 (høyst til 1; en kasse blir ikke mer). I 4a er ingen slike dører låst, så dette gjør
     ingenting ennå.
   - **Balanse:** en plass hører til rød side når avstanden til Sørbasen er minst 3 m kortere enn til Nordbasen, og
     motsatt for blå. Verdien er `(1 + 2·tier) · (1 + Depth)`. Er forskjellen mellom sidene over 10 % av den største,
     sett Tier 1-plassen nærmest egen base på den rikeste siden ned til Tier 0. Gjenta til forskjellen er under 10 %,
     høyst 4 ganger.
   - **Plassering:** gjør et objektspor (`ECC_WorldStatic`, ikke kanal – lærdom) fra punktet + 150 cm til punktet − 150 cm.
     Ved treff settes Z til `ImpactPoint.Z + 60 cm`. Uten treff hoppes plassen over, og den telles som «uten gulv».
     Ellers spawn som i `Spawn`-lambdaen, med `Place`, `Depth` og `MaxRarity` satt før `Roll()`.
   - Ingen mytisk kasse og ingen utendørs loot på Helgasetr.
   - **Logg (UE_LOG, så en hodeløs kjøring beviser tallene):**
     `Helgasetr-loot: %d plasser (%d vanlige, %d bedre, %d kasser), %d uten gulv; korridor %d, medisinsk %d, kantine %d; balanse rød %.1f / blå %.1f (%d justert)`.
5. Bygg. Kjør `gs.loot.stats` og lim tallene inn i rapporten.

**Ferdig når:**
- `Build.bat` går uten feil.
- Loggen viser 58 plasser og 0 uten gulv, og balansen er innenfor 10 %.
- `gs.loot.stats` viser:
  - Corridor: våpen 25–35 %, medisin 15–22 %, ammo 18–26 %.
  - Medical: medisin ≥ 50 %.
  - Ingen `Ammo_rocket` eller `Ammo_50bmg` når Max er Legendary.
  - 9 mm er blant de tre vanligste ammotypene.
  - Andelen Epic+ ved Depth 1 er minst dobbelt så høy som ved Depth 0.
- I PIE («Test kartet»): det ligger ting på gulvet i alle fire etasjer, ingen ting svever i atriet eller står i en vegg,
  det står låste kasser i Laberadene, i Østfløyens rom og i kantina, og en bandasje kan tas med E.
- På den gamle kampusgrenen (`bHelgasetr = false`) er koden uendret.

**Risiko:**
- Bare serveren ruller loot. `ItemId` replikeres som før, og de nye feltene trenger ikke nettet.
- Mange ting på ett lite kart: 58 snurrende ting er trygt for ytelsen (`Tick` med intervall).
- Sporet kan treffe et bord eller en hylle. Det er ønsket, for da ligger tingen på bordet.
- Faller en dør i sporet (Interaktivs `AGSDoor` på `DoorSlots()`), blir tingen liggende på dørbladet. Ingen
  lootplass ligger i en døråpning (sjekk i loggen: legg til en advarsel hvis en plass ligger nærmere enn 0,8 m fra en dør).

**IMPROVEMENTS-linjer (06a):**
880. Helgasetr har loot: 58 plasser i alle fire etasjer, med våpen, ammo, medisin, vester og granater
881. Det vanlige ligger nær basene og det sjeldne lenger ut mot midten av bygget
882. Ferdighetsrommene og laberadene har mest bandasjer, førstehjelpsskrin og smertestillende
883. Kantina og studentarbeidsplassene har mat og drikke i tillegg til våpen og ammo
884. Låste kasser med gode våpen i laberadene, i østfløyen og i kantina
885. Ammo kommer etter hvor mange våpen som bruker kaliberet: mye 9 mm og 5,56, ingen rakettammo uten rakettkaster
886. Begge lag har like mye og like god loot innen rekkevidde (automatisk balanse)
887. Tingene legges på gulvet eller bordet der de står, aldri i lufta over atriet
888. Ingen mytiske ting, fallskjermer, droner eller veggsyn i Helgasetr
889. Konsollen `gs.loot.stats` viser hvordan loot fordeler seg per rom og sjeldenhet

---

## 06b – PC-er, kameraer, stedsnavn og de åpne bestillingene

**Mål:** PC-ene, 3D-printerne og overvåkingskameraene står i ekte Helgasetr-rom med ekte navn. Radiomeldingen om
flaggbæreren sier for eksempel «i Laberadene, 4. etasje». Kampene starter med hele dører og helt glass. Menyen kan ta
deg til testrommet.

**Forbedrer (gammelt):**
- `GSHelp`s `HelpPlaceName`: Helgasetr-rom og etasje fra `ZoneAt`/`LevelName`/`SoundZones` i stedet for byggformelen,
  som gir «inne i Helgasetr sørfløy».
- `CleanWorldForMatch`: tilbakestiller også kartet.
- Tekstene til nøkkelkort og dirkesett blir riktige nå som dørene finnes.
- Menyknappene får lyd (Kamps bestilling).

**Nytt:**
- `SpawnTerminals` for Helgasetr: 4 PC-er, 2 printere og 8 kameraer med navn.
- Knappen «Testrom for dører og glass» i «Test kartet».

**Filer:** `GSGameMode.cpp`, `GSHelp.cpp`, `GSMenu.cpp`, `Tools/ItemCatalog/catalog.py` (og den genererte
`GSItemCatalog.gen.inl`), `GSGameState.h` (bare hvis steg 6 slår til).

**Steg:**
1. **`SpawnTerminals`:** `if constexpr (GSCampus::bHelgasetr)`. Bruk de eksisterende lambdaene `Terminal`/`Camera`:
   - For hver `GSHelgasetr::Terminals()`: `Terminal(Kind == 1 ? Printer : Computer, FVector(X, Y, Z), Yaw, Name)`.
   - For hver `Cameras()`: `Camera(FVector(X, Y, Z), Yaw, Name, Sweep)`. `Sweep` er 50° når navnet inneholder `Atriet`,
     `Kantina` eller `Mesaninen`, ellers 35°.
   - Logg `Helgasetr: %d PC-er, %d 3D-printere, %d kameraer`, og `return` før bygg-løkka.
2. **`GSHelp.cpp`, `HelpPlaceName`:** på Helgasetr gjelder dette:
   - Bruk navnet til den minste `SoundZones()`-boksen som inneholder punktet. `Atriet` brukes bare når ingen mindre boks
     passer.
   - Ellers `ZoneAt`, og utenfor bygget «ute».
   - Etasjen er `GSHelgasetr::LevelName(GSHelgasetr::LevelAt(Z))`.
   - Format: `i %s, %s` (for eksempel «i Kantina, 1. etasje», «i Sørbasen, 4. etasje»).
   - Samme oppslag som i 06a: legg det som én `inline`-funksjon `GSHelgasetr`-uavhengig i `GSHelp.cpp` (ikke i
     `GSHelgasetr.h`, som Grafikk eier). Grafikks `PlaceName()` (deres runde 6) erstatter den senere.
   - Den gamle kampusgrenen blir stående bak `else`.
3. **`CleanWorldForMatch`:** `#include "GSMapState.h"` og `AGSMapState::ResetForMatch(World);` etter at prosjektilene er
   samlet (den statiske funksjonen finnes i den committede headeren). Interaktiv tilbakestiller også selv, og to
   tilbakestillinger er ufarlig.
4. **`catalog.py`:**
   - Keycard får teksten «Åpner låste kasser og kodelåser på dører med en gang.»
   - Lockpick får teksten «Åpner låste kasser og dører på 6 sekunder.»
   - Kjør `python Tools/ItemCatalog/gen_cpp.py`.
   - Sjekk i `GSInteract`/`GSDoor` at dørene virkelig tar kort og dirk (commit `8ab2084`). Hvis ikke, behold de gamle
     tekstene og skriv det i rapporten.
5. **`GSMenu.cpp`, `DrawGotoPage`:**
   - Under «Der du sto sist» kommer knappen `Goto.TestRoom` med teksten «Testrom for dører og glass».
   - Knappen er aktiv bare når `GS->GameKind == GSGameKind::Explore` og `PC->HasAuthority()`.
   - I klikkhåndteringen: `GSMapState::EnterTestRoom(PC);` og logg `GSMenu: gå til testrommet`. Lukk menyen som de andre
     Gå til-knappene.
6. **`bBotsAvailable`:** sett den til `true` **bare hvis** `bots/logg.md` på byggetidspunktet har en godkjent 01a og
   `GSBots.h` finnes. Ellers skal den ikke røres; skriv «venter på Bots 01a» i bestillingen. Bots' punkt 2–5
   (lagbytte med bot, lobbytekst, «Vis navnett») tas i en egen runde når botene finnes.
7. **Menylyder (Kamps bestilling):** `#include "GSAudio.h"`.
   - `GSAudio::PlayUI(this, TEXT("UI_Click"))` ved knappetrykk.
   - `UI_Confirm` ved «Start kampen», «Gå inn i kartet» og lagring av innstillinger.
   - `UI_Back` ved Esc/tilbake.
   - Konteksten er PC-en eller viewporten der funksjonen ikke er en `UObject`. Mangler filen, skjer ingenting.
8. Oppdater statusen i `bestillinger.md`: Grafikks Helgasetr-bestilling (1)–(3) LEVERT 06a/06b, Interaktivs (1), (2) og (4)
   LEVERT 06b og (3) LEVERT 06a, Kamps menylyder LEVERT 06b, og Kamps kontrollbestilling om ammovekter LEVERT 06a.

**Ferdig når:**
- `Build.bat` går uten feil.
- Loggen viser `Helgasetr: 4 PC-er, 2 3D-printere, 8 kameraer`.
- I PIE:
  - PC-en i Studentforeningen (2. etasje) og printeren i grupperom 3.N1 kan brukes med E.
  - Kameraene henger på veggen under taket, og «Kantina, klatreveggen» kan ses gjennom en PC.
  - F10 → Gå til → «Testrom for dører og glass» flytter deg inn i testrommet.
  - Et knust glass er helt igjen etter «Start kampen på nytt».
  - Bærer du flagget i over 20 s, sier radiomeldingen et romnavn og en etasje.
- Knappetrykk i menyen gir klikklyd, eller ingenting hvis lydfila mangler, men ingen feil.

**Risiko:**
- Terminalene står mot ytterveggen (x ±24,8). Hvis modellen stikker inn i veggen, meld det til Grafikk; endre ikke
  dataene.
- `EnterTestRoom` er bare for serveren, derav `HasAuthority`.
- `HelpPlaceName` kjøres hos alle klienter, så kartdataene er de samme overalt. Ingen ny replikering.
- `GSMapState.h` har uncommittede endringer fra Interaktiv. Bruk bare den statiske `AGSMapState::ResetForMatch` og
  `GSMapState::EnterTestRoom`, som begge finnes i commit `8ab2084`.

**IMPROVEMENTS-linjer (06b):**
890. Fire PC-er i ekte Helgasetr-rom: Audiologi 4.056, ferdighetsrommet for vernepleiere, Studentforeningen og grupperom 2.1
891. To 3D-printere i grupperommene i 3. etasje
892. Åtte overvåkingskameraer med navn, fra klatreveggen i kantina til korridorene i 4. etasje
893. Kameraene over atriet og kantina dekker en større vinkel enn kameraene i korridorene
894. Radiomeldingen om flaggbæreren sier rom og etasje («i Laberadene, 4. etasje»)
895. Dører, låser og knust glass settes tilbake når en ny kamp starter
896. Nøkkelkortet og dirkesettet forteller at de også åpner dører
897. «Test kartet» → Gå til → «Testrom for dører og glass»
898. Menyknappene gir et nøkternt klikk, og bekreftelse og tilbake har egne lyder
