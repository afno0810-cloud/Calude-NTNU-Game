# Plan 07 – lag Spillbarhet: medisin og stoffer som i virkeligheten (virkning, varighet, bivirkninger, overdose)

Andreas' punkt 3: «alle ting virker, også medisin og boosts, med animasjoner og skjermeffekter». Kamp lager
bevegelsene og skjermen (`.claude/loop/ko/plan-10.md`, skisse). Vi eier katalogen og `GSItems`, så vi leverer
**selve mekanikken**: hva hvert stoff gjør, når det slår inn, hvor lenge, hva som kommer etterpå, hva som skjer når
man blander eller tar for mye, og de nye tingene Kamp har bestilt (sigarett, amfetamin, alkohol, nye skjerm-numre,
`onset`/`fade`, svar på R2/B3). Kamp endrer vi ikke; vi gir dem én funksjon å kalle.

**Kollisjon med Kamp:** plan-10 skal skrive om `UpdateItemUse`, `CompleteUse`, `UpdateLocalVisuals` og `TickEffects` i
`GSCharacter.cpp`. Vi rører ingen av dem. Vi legger nye felt *nederst* i structene i `GSItems.h` og ett felt nederst
i `FGSActiveEffect` (`GSTypes.h`, felles fil, bare tillegg). Kamps reservetabell på effekt-Id blir overflødig når
dette er levert. Effekt-Id-ene Kamps kode bruker beholdes: `Foniks`, `Glide`, `Avslort`, `Blendet`, `Matforgiftning`,
`Smertestillende`, `Brennende`, `Sjokk`, `Adrenalin`, `Turbo`. Gjenstands-Id-ene `Plaster`, `Bandage`, `Medkit`,
`SurgeryKit` beholdes (brukt i `GSBleed::StopFor`). Ingen ny Id får «Bandage» i Id eller «Bandasje» i navnet
(`GSGameMode` velger startbandasjen på det).

## Mål
Hvert stoff du tar slår inn gradvis, virker en realistisk tid, har kjente bivirkninger og en nedtur, og blander du
eller tar for mye blir du uvel, forgiftet og til slutt dør du av overdose; ingen pille gjør deg skuddsikker eller rask
som en bil.

## Forbedrer (gammelt)
1. **Tak på stabling (funn B3).** `FGSEffectMods::Combine` klemmer hver verdi etter hver ganging: fart maks 1,10,
   skade tatt minst 0,85, stamina-forbruk minst 0,4, stamina-henting maks 1,6, rekyl/spredning minst 0,75, skade maks
   1,10, hopp maks 1,05, ladetid minst 0,85, regen maks 1,0 HP/s, lifesteal maks 0. Virker **med en gang**, uten Kamp.
2. **Overnaturlige boosts ut (funn R2).** Nytt katalogfelt `loot=False` → `FGSItemDef::bNoLoot`; `Excluded()` i
   `GSItems.cpp` utelater dem i Helgasetr: Berserk, Bulk, Lynet, Iskald, Nattøye, Pansertablett, Regenerator,
   Blodtørst, Spøkelse, Føniks, Hjertestarter, Øyet (Øyet er allerede ute; flytt den til feltet). Id-ene blir i
   katalogen (Kamps kode og modellene bruker dem), men finnes ikke lenger på kartet.
3. **Stoffene som blir igjen gjøres realistiske** (tall nedenfor): ingen `taken` under 1 på noe stoff (et stoff
   stopper ingen kule), ingen `regen` på stoffer og mat, `heal=0` på all mat og alle stoffer (funn R1: mat leger
   ikke, adrenalin/morfin leger ikke – de demper smerte). Turbo heter nå **Amfetamin**, Drøm heter **Sovetablett**
   (ikke lenger immun mot blendgranat).
4. **Medisin tar tid å virke.** Førstehjelpsskrin: 35 HP med en gang + «Stell» (0,35 HP/s i 90 s); feltkirurgsett
   (heter nå «Stort sanitetssett»): 30 HP + «Stell». Smertestillende virker etter 20 s, ikke med en gang.
5. **Spisetid som passer måltidet (R7):** Grandiosa 12 s, kebab 10 s, matpakke og baguett 8 s.
6. **Beskrivelsene** på alle stoffer og all medisin skrives om til nøktern norsk om hva det gjør og hva det koster
   (ingen «+30 % skade»).

## Nytt
1. **Innslag, topp og nedtur per stoff.** Nye felt i `FGSEffectDef`: `Onset` (s til full virkning), `Fade` (s
   nedtur på slutten), `Load` (belastning per dose), `Class` (`EGSDrugClass`: None, Stimulant, Opioid, Depressant,
   Alcohol, Nicotine, Caffeine, Cannabis, Antidote). `GSItems::EffectStrength(Def, EndTime, Now)` gir 0..1
   (start = `EndTime − Duration`, så ingen nye replikerte tider trengs).
2. **`GSItems::CombineActive(const TArray<FGSActiveEffect>&, float Now)`** – erstatter løkka i Kamps `GetMods`:
   hver effekt skaleres med styrken (`1 + (m−1)·s`), Nalokson blokkerer opioider mens den virker, og
   **belastningen** legges på til slutt (under). Samme regning på klient og server (bare replikerte data).
3. **Belastning og overdose.** L = Σ `Load · Doses · styrke`; opioid sammen med sovetablett/alkohol ganges med 1,5
   (pustestans – den farligste blandingen i virkeligheten), to ulike sentralstimulerende ×1,25.
   - L ≥ 1,2 **Uvel**: spredning ×1,15, stamina-henting ×0,8, skjelve-lag på skjermen.
   - L ≥ 2,0 **Forgiftet**: fart ×0,85, spredning ×1,4, ladetid ×1,3, −0,5 HP/s, uklart bilde.
   - L ≥ 3,0 **Overdose**: −2,5 HP/s, fart ×0,5, tunnelsyn. Du dør på ca. 40 s hvis ikke belastningen synker
     (stoffet går ut) eller du får Nalokson (bare mot opioider).
   - Eksempler: 2 morfin = forgiftet, 3 morfin = overdose, morfin + 2 pils = overdose, 2 amfetamin + energidrikk =
     forgiftet, røyk + snus + plaster = ingenting.
4. **`FGSActiveEffect::Doses`** (`uint8`, standard 1, replikert, nederst i structen i `GSTypes.h`). Kamp øker den i
   `AddEffect` når samme stoff tas igjen (maks 5). Til det er levert teller blandinger, ikke gjentatte doser.
5. **`GSItems::ActiveVisuals(Effects, Now, TArray<FGSVisualLayer>&)`** – liste med `{Visual, Strength}` til Kamps
   skjerm i plan-10, også belastningslagene (Uvel = 3 skjelving, Forgiftet = 1 uklart, Overdose = 10 tunnelsyn).
   Nye numre: **8 rød/varm (røyk), 9 hvit/overeksponert (amfetamin), 10 tunnelsyn (adrenalin, overdose), 11
   dobbeltsyn (alkohol)**.
6. **Nye flagg i `FGSEffectMods`** (nederst): `bNumb` (smerte dempet: Kamp kan bruke det i stedet for å sjekke
   Id-en `Smertestillende`) og `bCough` (hoste ved sprint etter røyk). `Combine` slår dem sammen med `|=`.
7. **Nye ting:** Sigarett, Pils, Årepresse (tourniquet, `Kind="tourniquet"` – Kamps `GSBleed::StopFor` leser den
   allerede, så den stopper blødning fra dag én), Skinne (`Kind="splint"`, `Treat=1`, ute av loot til Kamp leser
   `Treat`), Nalokson nesespray.
8. **Nye felt i `FGSItemDef`** (nederst): `Treat` (0..1, hvor mye et skadet bein/arm behandles; Kamp leser i dag
   `Heal/60`), `UseAnim` (FName, standard = modellens `arch`; sigarett = `cigarette`), `bNoLoot`.
9. **Selvtest `gs.fx.test`** (konsoll, hodeløs) og sjekker i `gen_cpp.py` (under).

## Hvorfor nå
Runde 6 la loot i Helgasetr, så man nå *finner* medisin og stoffer – men de er overnaturlige, stables uten tak og
har ingen følger (B3, R1, R2 står som Høy). Kamp venter på svar fra oss før de skriver plan-10, og Andreas har
dette som punkt 3. Alt ligger i våre filer; Kamp trenger bare å bytte én løkke.

## Filer
- `Tools/ItemCatalog/catalog.py`, `Tools/ItemCatalog/gen_cpp.py`, generert `GSItemCatalog.gen.inl`
- `Source/Gloshaugen/Public/GSItems.h`, `Source/Gloshaugen/Private/GSItems.cpp`
- `Source/Gloshaugen/Public/GSTypes.h` (bare ett felt nederst i `FGSActiveEffect`)
- `.claude/lag/bestillinger.md`, `IMPROVEMENTS.md`

## Steg
1. **Katalogtabellen.** I `catalog.py`, nye nøkler i `EFFECTS`: `onset`, `fade`, `load`, `cls` (streng: "stim",
   "opioid", "depressant", "alcohol", "nicotine", "caffeine", "cannabis", "antidote"), flaggene `numb`, `cough`.
   Oppdater kommentaren øverst (inkl. visual 8–11). Spilltid er komprimert (en kamp er 15 min), forholdene er ekte.
   Sett disse (resten av feltene fjernes der de ikke står):

   | Id | navn | cls | load | onset | fade | dur | virkning | after |
   |---|---|---|---|---|---|---|---|---|
   | Turbo | Amfetamin | stim | 0,8 | 20 | 15 | 90 | speed 1,05, stamina_drain 0,5, reload 0,92, spread 1,1, hunger 0,3, thirst 1,5, visual 9 | TurboKrasj |
   | TurboKrasj | Amfetaminkrasj | – | 0 | 0 | 10 | 60 | stamina_regen 0,6, spread 1,2, reload 1,1, visual 1 | |
   | Adrenalin | Adrenalin | stim | 0,6 | 1 | 5 | 20 | numb, speed 1,05, stamina_drain 0,6, spread 1,15, visual 10 | AdrenalinKrasj |
   | AdrenalinKrasj | Etter adrenalin | – | 0 | 0 | 5 | 20 | stamina_regen 0,5, spread 1,15 | |
   | Morfin | Morfin | opioid | 1,0 | 8 | 20 | 120 | numb, speed 0,95, spread 1,1, reload 1,15, visual 1 | |
   | Smertestillende | Smertestillende | – | 0,2 | 20 | 15 | 120 | numb | |
   | Gronn | Grønn | cannabis | 0,5 | 10 | 20 | 90 | speed 0,95, recoil 0,9, reload 1,15, hunger 2,5, visual 4 | |
   | Snus | Snus | nicotine | 0,15 | 15 | 20 | 120 | reload 0,95 | |
   | Nikotin | Nikotinplaster | nicotine | 0,15 | 60 | 30 | 240 | stamina_drain 0,95 | |
   | Royk (ny) | Røyk | nicotine | 0,2 | 2 | 20 | 90 | cough, stamina_regen 0,85, spread 0,95, visual 8 | |
   | Koffein | Koffein | caffeine | 0,15 | 15 | 30 | 180 | stamina_drain 0,9 | |
   | KoffeinPlus | Mye koffein | caffeine | 0,3 | 10 | 30 | 150 | stamina_drain 0,85, reload 0,97, spread 1,05 | |
   | Fokus | Fokus | stim | 0,4 | 20 | 20 | 120 | spread 0,9, recoil 0,92, hunger 0,5 | |
   | Alkohol (ny) | Alkohol | alcohol | 0,5 | 15 | 30 | 150 | numb, speed 0,98, spread 1,15, recoil 1,1, reload 1,15, visual 11 | |
   | Drom | Sovetablett | depressant | 0,6 | 20 | 20 | 120 | speed 0,93, spread 0,92, reload 1,15, visual 1 | |
   | Lattergass | Lattergass | – | 0,3 | 1 | 5 | 25 | speed 0,95, spread 1,3, visual 2 | |
   | Nalokson (ny) | Nalokson | antidote | 0 | 2 | 10 | 60 | (blokkerer opioider i `CombineActive`) | |
   | Stell (ny) | Sårstell | – | 0 | 0 | 0 | 90 | regen 0,35 | |
   | Kirurgi | Sårstell (stort) | – | 0 | 0 | 0 | 90 | regen 0,35 | |
   | Tran / Julestemning | – | – | 0 | – | – | uendret | fjern `regen` og `taken`; behold stamina_regen | |

   De overnaturlige effektene (Berserk, Bulk/BulkKrasj, Lynet, Iskald/Skjelving, Spokelse, Pansertablett,
   Regenerator, Oyet/Forvirret, Foniks, Blodtorst, Nattoye/Lysomfintlig) og spill-effektene (Glide, Avslort,
   Blendet, Brennende, Sjokk, Matforgiftning, Sukkerrush, Mett) står **uendret** – de første ligger bare på ting
   som går ut av loot.
2. **Tingene.** I `catalog.py`:
   - `drug()`, `med()` og `food()` får `**kw` som sendes videre (for `loot`, `kind`, `treat`, `anim`).
   - `loot=False` på: Bulk, Lynet, Iskald, NightEye, Berserk, Ghost, ArmorPill, Regenerator, Oyet, Phoenix,
     Bloodthirst, Defib.
   - `heal=0` på alle `drug` (Adrenaline 35 → 0, Morphine 20 → 0) og alle `food` (OrangeSoda, Lunchbox, Grandiosa,
     Kebab, Taco, Apple, Salmon, Brunost).
   - TurboPill: navn «Amfetamin», Rare, use 1,5. Drom: navn «Sovetablett». Nye beskrivelser (én nøktern setning
     om virkning og pris) på alle drug/medisin.
   - Medisin: Plaster heal 15, `treat=0.25`; Bandage heal 30, `treat=0.5`; Painkillers heal 0, `treat=0`;
     Medkit heal 35, effect `Stell`, `treat=1.0`; SurgeryKit navn «Stort sanitetssett», heal 30, effect `Kirurgi`,
     `treat=1.0`, use 10.
   - Nye: `drug("Cigarettes", "Sigarett", "Common", 6.0, "Royk", dict(arch="tin", color=(0.95, 0.95, 0.95)),
     "...", anim="cigarette")`; `food("Beer", "Pils (0,5 l)", "Common", 6.0, 0, 25, effect="Alkohol",
     model=dict(arch="can", color=(0.85, 0.7, 0.2), h=0.17, r=0.033))`; `med("Tourniquet", "Årepresse",
     "Uncommon", 5.0, 0, "", dict(arch="bandage"), kind="tourniquet")`; `med("Splint", "Skinne", "Uncommon", 8.0,
     0, "", dict(arch="bandage"), kind="splint", treat=1.0, loot=False)`; `med("Naloxone", "Nalokson nesespray",
     "Rare", 2.0, 0, "Nalokson", dict(arch="dropper", color=(0.9, 0.9, 0.95)))`. Bare eksisterende `arch`-er
     (`gen_models.py` er Grafikks og krasjer på ukjente); egne modeller bestilles (steg 8).
   - Spisetid: Grandiosa 12, Kebab 10, Lunchbox 8, Baguette 8.
3. **`gen_cpp.py`:** skriv `E.Onset`, `E.Fade`, `E.Load`, `E.Class = EGSDrugClass::...`, `E.Mods.bNumb`,
   `E.Mods.bCough`; for ting `D.Treat`, `D.UseAnim` (`anim` eller `model["arch"]`), `D.bNoLoot`, og `D.Kind` også
   for Medicine. Legg inn `assert`-er som stopper genereringen: hver `effect`/`after` finnes i `EFFECTS`; hver
   drug har `cls`; Id-ene Kamps kode bruker (listen øverst) finnes; ingen effekt på en ting med loot har `taken < 1`,
   `damage > 1`, `regen > 0` (unntatt Stell/Kirurgi), `lifesteal`, `ghost`, `wallhack`, `phoenix`. Kjør
   `python Tools/ItemCatalog/gen_cpp.py`.
4. **`GSItems.h`** (ett samlet header-bytte, raskt, så andre lags bygg ikke stopper):
   - `enum class EGSDrugClass : uint8 { None, Stimulant, Opioid, Depressant, Alcohol, Nicotine, Caffeine, Cannabis, Antidote };`
   - `FGSEffectMods`: nederst `bool bNumb = false, bCough = false;`. `Combine`: `|=` for dem, og klemming som i
     «Forbedrer 1» (en `static void ClampMods(FGSEffectMods&)` som `Combine` kaller til slutt).
   - `FGSEffectDef`: nederst `float Onset = 0.f, Fade = 0.f, Load = 0.f; EGSDrugClass Class = EGSDrugClass::None;`
   - `FGSItemDef`: nederst `float Treat = 0.f; FName UseAnim; bool bNoLoot = false;`
   - `struct FGSVisualLayer { int32 Visual = 0; float Strength = 0.f; };`
   - I `namespace GSItems`: `float EffectStrength(const FGSEffectDef&, float EndTime, float Now);`,
     `FGSEffectMods CombineActive(const TArray<FGSActiveEffect>&, float Now);`,
     `float DrugLoad(const TArray<FGSActiveEffect>&, float Now);`,
     `void ActiveVisuals(const TArray<FGSActiveEffect>&, float Now, TArray<FGSVisualLayer>& Out);`
     (`#include "GSTypes.h"` hvis ikke allerede).
5. **`GSTypes.h`:** nederst i `FGSActiveEffect`: `UPROPERTY(BlueprintReadOnly) uint8 Doses = 1;`
6. **`GSItems.cpp`:**
   - `EffectStrength`: `Start = EndTime − Duration`; `t = Now − Start`; opp lineært over `Onset`, 1 i midten, ned
     lineært over `Fade` før `EndTime`; 0 utenfor; `Onset/Fade = 0` → 1 hele tiden.
   - `DrugLoad`: summer `Load · max(1, Doses) · styrke` per klasse; opioider teller 0 mens en Antidote har styrke
     ≥ 0,5; har både Opioid og (Depressant eller Alcohol) > 0 → ganger de to med 1,5; to ulike Stimulant-effekter
     → stim ×1,25. Returner sum.
   - `CombineActive`: for hver effekt med `EndTime ≥ Now`: hent def, regn styrke, lag en skalert kopi av `Mods`
     (multiplikatorer `1 + (m−1)·s`; `Regen`/`Drain`/`Lifesteal` `·s`; flagg og `Visual` bare når s ≥ 0,5; opioid
     med aktiv motgift hoppes over), `Combine`. Legg så på belastningstrinnene (Forbedrer/Nytt 3) med egne
     ganginger etter klemmingen, slik at overdose kan gjøre deg tregere enn taket tillater nedover.
   - `ActiveVisuals`: ett lag per effekt med `Visual ≠ 0` og styrke > 0,05, pluss belastningslaget (styrke =
     hvor langt over terskelen, 0..1). Sortert sterkest først.
   - `Excluded()`: `D.bNoLoot ||` i tillegg til dagens (Place ≠ Campus). Fjern `IdOyet` derfra (dekkes nå av feltet).
   - **`gs.fx.test`** (`FAutoConsoleCommand`, samme mønster som `gs.loot.stats`): bygger kunstige
     `TArray<FGSActiveEffect>` og logger linje for linje med `PASS`/`FAIL`:
     (a) Morfin ved t=0, 4, 8, 100, 120 s → styrke 0 / 0,5 / 1 / 1 / 0; (b) Morfin×2 (Doses=2) → L 2,0 «Forgiftet»;
     (c) Morfin×3 → «Overdose», `Drain` ≥ 2,5; (d) Morfin + Alkohol×2 → overdose; (e) Morfin×3 + Nalokson (s=1) →
     under 1,2; (f) Amfetamin×2 + KoffeinPlus → Forgiftet; (g) Sigarett + Snus + Nikotin → under 1,2; (h) alle
     effekter med styrke 1 samtidig → alle verdier innenfor taket (før belastningstrinn); (i) ingen gjenstand med
     `bNoLoot` kommer ut av 20 000 `RandomLoot` i Medical/Corridor/Canteen/Crate; (j) alle Id-ene Kamp bruker
     finnes. Til slutt «gs.fx.test: N/N OK».
7. **Bygg** (`Build.bat ... -WaitMutex`) og kjør `gs.fx.test` og `gs.loot.stats` hodeløst (samme måte som i runde
   6). Lim resultatet inn i rapporten.
8. **Bestillinger** (`.claude/lag/bestillinger.md`):
   - Svar på Kamps bestilling 27. nov. (plan-10): sett den til LEVERT med Id-ene (Cigarettes/Royk, TurboPill =
     Amfetamin/Turbo, Beer/Alkohol), visual 8–11 og hvor de brukes, `Onset`/`Fade`, `UseAnim`, svar på R2 (ute av
     loot via `bNoLoot`, Id-ene beholdt) og B3 (tak i `Combine` + belastning/overdose).
   - **Ny til Kamp (K-FX, små kroker, passer inn i plan-10):** (1) `GetMods()` → `return
     GSItems::CombineActive(Effects, GetServerTime());` (2) `AddEffect`: ved samme Id `E.Doses = FMath::Min(E.Doses
     + 1, 5)`; ny effekt `Doses = 1`. (3) `bNumb` i stedet for `Id == FxPainkillers` (linje ~3284/3290). (4)
     Lemmer: `Def->Treat` i stedet for `Def->Heal / 60.f`, og `Kind == "splint"` behandler bein og arm fullt – si
     fra, så setter vi Skinne inn i loot. (5) `bCough`: hoste ved sprint. (6) Skjermen: `GSItems::ActiveVisuals`.
     (7) Død av overdose: drapsmelding «døde av overdose» (årsak = `DrugLoad ≥ 3`). Til (1) er levert gjelder taket,
     de nye tallene og årepressen, men ikke innslag/nedtur og overdose.
   - **Ny til Grafikk:** modeller i `gen_models.py` for sigarettpakke (`arch="cigarettes"`), ølboks, årepresse,
     skinne og nesespray; vi bytter `arch` i katalogen når de finnes.
9. **IMPROVEMENTS.md:** egen seksjon «Runde 7 (Spillbarhet): medisin og stoffer som i virkeligheten» med linjene
   under, øverst i fila.

## Ferdig når
- `gen_cpp.py` går uten `assert`-feil; bygget går uten feil og uten nye advarsler i våre filer.
- `gs.fx.test` viser alle linjer OK; `gs.loot.stats` viser 0 utelatte og ingen av de tolv overnaturlige i
  Helgasetr.
- I PIE (konsoll `GiTing` hvis Kamp har den, ellers finn tingene): fire stoffer samtidig gir ikke over +10 % fart;
  Amfetamin, Morfin og mat gir ingen helse; årepressen stopper blødning; Pils, Sigarett og Nalokson finnes og kan tas.
- Kampens kode bygger uendret (vi har ikke rørt `GSCharacter.*`), og effekt-Id-ene den bruker finnes.
- Bestillingene til Kamp og Grafikk er skrevet; Kamps 27. nov.-bestilling er besvart.

## Risiko
- **Replikering:** `Doses` er et nytt replikert felt i en struct i en replikert liste – lite, men begge sider må
  bygges likt (vanlig). `CombineActive` bruker bare replikerte data og `GetServerTime`, så klient og server regner
  likt; små tidsavvik gir bare litt ulik styrke i innslaget.
- **Ytelse:** `GetMods` kalles ofte (flere ganger per tick hos Kamp). `CombineActive` er en løkke over 0–6 effekter
  med `FindEffect` – hold `FindEffect` som i dag (oppslag), ingen tildelinger utenom en liten lokal tabell.
- **Header-bytte i `GSItems.h`/`GSTypes.h`** tvinger gjenbygg hos alle lag: gjør det i ett, tidlig, og bygg rett etter.
- **Balanse:** uten Kamps krok (1) gjelder bare taket og de nye tallene; stoffene blir svakere (ingen helse, ingen
  skuddsikkerhet) før overdose finnes. Det er riktig retning (realistisk), men skriv det i rapporten.
- Kamps `UpdateLocalVisuals` kjenner ikke visual 8–11 ennå; de gir bare dagens milde kantfarge/vignett til plan-10.
- Bots: bruker ikke stoffer etter Id i dag (sjekket); ingen følger.

## IMPROVEMENTS-linjer
920. Stoffer slår inn gradvis, når en topp og har en nedtur, i stedet for å slås av og på brått
921. Overdose: tar du for mye av ett stoff blir du uvel, så forgiftet, og til slutt dør du hvis ingen hjelper
922. Blandinger er farlige som i virkeligheten: morfin sammen med alkohol eller sovetablett gir pustestans mye raskere
923. Samme stoff flere ganger teller som flere doser (når Kamp kobler på kroken)
924. Tak på boosts: uansett hvor mange stoffer du tar blir du aldri mer enn 10 % raskere eller tåler mer enn 15 % mindre skade
925. Overnaturlige boosts (Spøkelse, Føniks, Øyet, Blodtørst, Berserk, Lynet, Iskald, Pansertablett m.fl.) finnes ikke lenger på kartet
926. Mat, adrenalin og morfin leger ikke lenger sår: mat metter, adrenalin og morfin demper smerte
927. Førstehjelpsskrin og stort sanitetssett gir litt helse med en gang og resten langsomt over halvannet minutt
928. Turbo heter nå Amfetamin og virker som det: mindre slitenhet, urolige hender, og et tungt krasj etterpå
929. Ny: Sigarett – litt roligere hender, men dårligere kondis og hoste etterpå
930. Ny: Pils – hver boks gjør siktet og ladingen dårligere, og alkohol blandet med morfin kan drepe
931. Ny: Årepresse (tourniquet) som stopper all blødning fra et bein eller en arm
932. Ny: Nalokson nesespray som opphever en opioid-overdose en stund – men morfinen kommer tilbake når den går ut
933. Ny: Skinne for brukne bein og armer (på kartet når Kamp leser behandlingsfeltet)
934. Sovetabletten (før «Drøm») gjør deg ikke lenger immun mot blendgranater
935. Store måltider tar realistisk tid å spise: Grandiosa 12 s, kebab 10 s
936. Alle stoffer og all medisin har nye, nøkterne beskrivelser av virkning og bivirkning
937. Skjermeffekter klare for hvert stoff: rødt ved røyk, hvitt ved amfetamin, tunnelsyn ved adrenalin og overdose, dobbeltsyn ved alkohol (tegnes av Kamp)
938. Selvtest `gs.fx.test` sjekker innslag, doser, blandinger, overdose, tak og at ingen overnaturlige ting ligger i loot
939. Katalogen stopper seg selv hvis noen legger inn et stoff som gjør deg skuddsikker, usynlig eller udødelig
