# Plan 07a – lag Spillbarhet: realistiske stoffer og medisin, tak på boosts, nye ting (header for 07b)

Erstatter `plan-07.md` sammen med `plan-07b.md`. Rettet etter forhåndssjekken `kontroll/forhand-plan-07.md` (punkt 1–12).
07a synes i spillet med en gang. 07b (regnemotoren: innslag, nedtur, belastning, overdose, Nalokson) bygges rett
etter, bare i `GSItems.cpp`, fordi headeren da allerede er på plass.

Andreas' punkt 3: «alle ting virker, også medisin og boosts». Kamp lager bevegelser og skjerm (`.claude/loop/ko/plan-10.md`).
Vi eier katalogen og `GSItems` og leverer mekanikken.

**Ingen kollisjon med Kamp:** vi rører ikke `GSCharacter.*` eller `GSHUD.*`. Effekt-Id-ene Kamps kode bruker beholdes:
`Foniks`, `Glide`, `Avslort`, `Blendet`, `Matforgiftning`, `Smertestillende`, `Brennende`, `Sjokk`, `Adrenalin`, `Turbo`.
Gjenstands-Id-ene `Plaster`, `Bandage`, `Medkit` og `SurgeryKit` beholdes, siden `GSBleed::StopFor` bruker dem. Ingen ny
Id får «Bandage» i Id-en eller «Bandasje» i navnet, fordi `GSGameMode` velger startbandasjen ut fra det.

## Mål
Ingen pille eller matbit gjør deg skuddsikker, usynlig eller lynrask lenger. Medisin og stoffer har nøkterne, ekte
virkninger og bivirkninger, og du finner sigaretter, pils og årepresse i Helgasetr.

## Forbedrer (gammelt)
1. **Tak på stabling (funn B3)** i `FGSEffectMods::Combine`, som virker med en gang:
   - fart: maks 1,10
   - skade tatt: minst 0,85
   - stamina-forbruk: minst **0,5**
   - stamina-henting: maks 1,6
   - rekyl og spredning: minst 0,75
   - skade: maks 1,10
   - hopp: maks 1,05
   - ladetid: minst 0,85
   - regen: maks 1,0 HP/s
   - lifesteal: maks 0

   Dette er en midlertidig vakt. Når Kamp kaller `CombineActive` (07b), klemmes det én gang til slutt (punkt 9).
2. **Overnaturlige boosts ut av loot (R2)** med det nye feltet `bNoLoot`. Id-ene blir i katalogen.
3. **Ingen helse fra mat eller stoffer (R1).** `heal=0` settes på alt med kategori `Food` og `Drug`. En assert passer på det.
   Stoffer gir heller ikke `taken < 1` lenger.
4. **Medisin:**
   - Førstehjelpsskrin: 35 HP med en gang, + «Stell» (0,35 HP/s i 90 s).
   - Stort sanitetssett (før «Feltkirurgsett»): **45 HP** med en gang, + «Kirurgi» (**0,5 HP/s i 90 s**), 10 s å bruke.
     Det sjeldneste settet er nå best.
   - Smertestillende gir ingen helse.
5. **Spisetid (R7):** Grandiosa 12 s, kebab 10 s, matpakke 8 s, baguett 8 s.
6. **Nye navn og tekster:** Turbo heter nå **Amfetamin** og Drøm heter **Sovetablett**, som ikke lenger gjør deg immun
   mot blendgranat. Alle stoffer og all medisin får nøkterne beskrivelser.

## Nytt
1. **Sigarett, Pils, Årepresse og Skinne.**
   - Årepressen har `Kind="tourniquet"`, som Kamps `StopFor` allerede leser, så den stopper blødning fra dag én.
   - Skinnen er ute av loot til Kamp leser `Treat`.
2. **Hele headeren for 07b i ett bytte:**
   - `EGSDrugClass`
   - `FGSVisualLayer`
   - effektfeltene `Onset`, `Fade`, `Load` og `Class`
   - flaggene `bNumb` og `bCough`
   - tingfeltene `Treat`, `UseAnim` og `bNoLoot`
   - `FGSActiveEffect::Doses` og `StartTime`
   - funksjonsdeklarasjonene til 07b
3. **Katalogsjekker i `gen_cpp.py`**, som stopper hvis noen legger inn et stoff som gjør deg skuddsikker, usynlig,
   udødelig eller helbredet av mat.
4. **Selvtesten `gs.fx.test`**, del 1: test (h), (i) og (j).

## Hvorfor nå
Runde 6 la loot i Helgasetr, så man nå finner stoffene. Men de er overnaturlige, stables uten tak og leger
(funn B3, R1 og R2 er Høy). Kamp venter på svar fra oss før plan-10 skrives. Alt ligger i våre filer.

## Filer
- `Tools/ItemCatalog/catalog.py`
- `Tools/ItemCatalog/gen_cpp.py`
- generert `GSItemCatalog.gen.inl`
- `Public/GSItems.h`
- `Private/GSItems.cpp`
- `Public/GSTypes.h` (bare to felt nederst i `FGSActiveEffect`)
- `.claude/lag/bestillinger.md`
- `IMPROVEMENTS.md`

## Steg
1. **`EFFECTS` i `catalog.py`.**
   - Nye nøkler: `onset`, `fade`, `load`, `cls`, `numb` og `cough`. `cls` er en av "stim", "opioid", "depressant",
     "alcohol", "nicotine", "caffeine", "cannabis" eller "antidote".
   - Oppdater kommentaren øverst: visual 8 = rød/varm, 9 = hvit/overeksponert, 10 = tunnelsyn, 11 = dobbeltsyn.
   - Spilltiden er komprimert (en kamp er 15 min), men forholdene er ekte.
   - Felt som ikke står i tabellen, fjernes fra disse effektene.

   | Id | navn | cls | load | onset | fade | dur | virkning | after |
   |---|---|---|---|---|---|---|---|---|
   | Turbo | Amfetamin | stim | 0,8 | 20 | 15 | 90 | speed 1,05, stamina_drain 0,5, reload 0,92, spread 1,1, hunger 0,3, thirst 1,5, visual 9 | TurboKrasj |
   | TurboKrasj | Amfetaminkrasj | – | 0 | 0 | 10 | 60 | stamina_regen 0,6, spread 1,2, reload 1,1, visual 1 | |
   | Adrenalin | Adrenalin | stim | 0,6 | 1 | 5 | 20 | numb, speed 1,05, stamina_drain 0,6, spread 1,15, visual 10 | AdrenalinKrasj |
   | AdrenalinKrasj | Etter adrenalin | – | 0 | 0 | 5 | 20 | stamina_regen 0,5, spread 1,15 | |
   | Morfin | Morfin | opioid | **1,05** | 8 | 20 | 120 | numb, speed 0,95, spread 1,1, reload 1,15, visual 1 | |
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
   | Nalokson (ny) | Nalokson | antidote | 0 | 2 | 10 | 60 | ingen mods (virkningen kommer i 07b) | |
   | Stell (ny) | Sårstell | – | 0 | 0 | 0 | 90 | regen 0,35 | |
   | Kirurgi | Sårstell (stort) | – | 0 | 0 | 0 | 90 | regen **0,5** | |
   | Tran, Julestemning | – | – | 0 | – | – | uendret | fjern `regen` og `taken`, behold resten | |

   Disse står uendret: de overnaturlige effektene (de ligger bare på ting som går ut av loot) og spilleffektene
   (Glide, Avslort, Blendet, Brennende, Sjokk, Matforgiftning, Sukkerrush, Mett).
2. **`ITEMS` i `catalog.py`.**
   - `drug()`, `med()` og `food()` får `**kw` som sendes videre (`loot`, `kind`, `treat`, `anim`).
   - `loot=False` på disse: Bulk, Lynet, Iskald, NightEye, Berserk, Ghost, ArmorPill, Regenerator, Oyet, Phoenix,
     Bloodthirst, Defib og Splint.
   - `heal=0` på **alle** `drug` og `food`. Det gjelder blant annet:
     - Adrenaline og Morphine
     - OrangeSoda, Lunchbox, Grandiosa, Kebab, Taco, Apple, Salmon og Brunost
     - og de sju i «more food»: Farikal, Pinnekjott, Risgrot, Tran, Solbaer, Rakfisk og Sodd

     Assert-en i steg 3 fanger det som blir glemt.
   - TurboPill får navnet «Amfetamin», og Drom får navnet «Sovetablett».
   - Alle `drug` og `med` får nye beskrivelser: én nøktern setning om virkningen og om prisen du betaler.
   - Medisin (`treat` er hvor mye et skadet bein eller en arm behandles):
     - Plaster: heal 15, `treat=0.25`
     - Bandage: heal 30, `treat=0.5`
     - Painkillers: heal 0, `treat=0`
     - Medkit: heal 35, effect `Stell`, `treat=1.0`
     - SurgeryKit: navnet «Stort sanitetssett», heal **45**, effect `Kirurgi`, `treat=1.0`, use 10
   - Nye ting:
     - Sigarett: `drug("Cigarettes", "Sigarett", "Common", 6.0, "Royk", dict(arch="tin", color=(0.95, 0.95, 0.95)), "...", anim="cigarette")`
     - Pils: `food("Beer", "Pils (0,5 l)", "Common", 6.0, 0, 25, effect="Alkohol", model=dict(arch="can", color=(0.85, 0.7, 0.2), h=0.17, r=0.033))`
     - Årepresse: `med("Tourniquet", "Årepresse", "Uncommon", 5.0, 0, "", dict(arch="bandage"), kind="tourniquet")`
     - Skinne: `med("Splint", "Skinne", "Uncommon", 8.0, 0, "", dict(arch="bandage"), kind="splint", treat=1.0, loot=False)`

     Bruk bare `arch`-er som finnes allerede.
   - Nalokson nesespray kommer i 07b.
   - Spisetid: Grandiosa 12, Kebab 10, Lunchbox 8, Baguette 8.
3. **`gen_cpp.py`.**
   - Skriv `E.Onset`, `E.Fade`, `E.Load`, `E.Class = EGSDrugClass::...`, `E.Mods.bNumb` og `E.Mods.bCough`.
   - Skriv `D.Treat`, `D.UseAnim` (fra `anim`, ellers `model["arch"]`), `D.bNoLoot`, og `D.Kind` også for Medicine.
   - Assert-er som stopper genereringen:
     - (1) hver `effect` og `after` finnes i `EFFECTS`
     - (2) hver `Drug` har en effekt med `cls`
     - (3) Id-ene Kamps kode bruker (listen øverst) finnes
     - (4) **alle `Food` og `Drug` har `heal == 0`**
     - (5) ingen effekt på en ting som ligger i loot har noe av dette: `taken < 1`, `damage > 1`, `regen > 0` (unntatt
       Stell og Kirurgi), `lifesteal`, `ghost`, `wallhack`, `phoenix`, `nightvision` eller `flashimmune`. NVGoggles er en
       gadget uten effekt og påvirkes ikke.
   - Kjør `python Tools/ItemCatalog/gen_cpp.py`.
4. **`GSItems.h`.** Gjør alt i ett bytte, raskt.
   - **Ikke** inkluder `GSTypes.h`. Forhåndsdeklarer `struct FGSActiveEffect;` i stedet.
   - `enum class EGSDrugClass : uint8 { None, Stimulant, Opioid, Depressant, Alcohol, Nicotine, Caffeine, Cannabis, Antidote };`
   - `FGSEffectMods`:
     - nederst: `bool bNumb = false, bCough = false;`
     - `Combine` slår dem sammen med `|=` og kaller `ClampMods(*this)` til slutt. Tallene står i Forbedrer 1.
     - Legg også til `void CombineRaw(const FGSEffectMods&)`, som gjør det samme uten klemming. Den brukes av 07b.
   - `FGSEffectDef`, nederst: `float Onset = 0.f, Fade = 0.f, Load = 0.f; EGSDrugClass Class = EGSDrugClass::None;`
   - `FGSItemDef`, nederst: `float Treat = 0.f; FName UseAnim; bool bNoLoot = false;`
   - `struct FGSVisualLayer { int32 Visual = 0; float Strength = 0.f; };`
   - Deklarasjoner i `namespace GSItems`:
     - `void ClampMods(FGSEffectMods&);`
     - `float EffectStrength(const FGSEffectDef&, const FGSActiveEffect&, float Now);`
     - `FGSEffectMods CombineActive(const TArray<FGSActiveEffect>&, float Now);`
     - `float DrugLoad(const TArray<FGSActiveEffect>&, float Now);`
     - `void ActiveVisuals(const TArray<FGSActiveEffect>&, float Now, TArray<FGSVisualLayer>& Out);`
   - Til 07b er bygget, er de fire siste i `GSItems.cpp` enkle reserveversjoner:
     - `EffectStrength` gir 1 mens effekten varer, ellers 0.
     - `CombineActive` er dagens løkke med `Combine`.
     - `DrugLoad` gir 0.
     - `ActiveVisuals` gir én linje per `Visual`.

     Da bygger alt, og Kamp kan koble på allerede nå.
5. **`GSTypes.h`**, nederst i `FGSActiveEffect`:
   ```
   UPROPERTY(BlueprintReadOnly) uint8 Doses = 1;
   UPROPERTY(BlueprintReadOnly) float StartTime = 0.f;   // server time of the first dose; 0 = EndTime - Duration
   ```
6. **`GSItems.cpp`.**
   - `#include "GSTypes.h"`.
   - `Excluded()`: `D.bNoLoot ||` legges til det som er der (Place ≠ Campus). Fjern `IdOyet`, som nå dekkes av feltet.
   - **`gs.fx.test`** (`FAutoConsoleCommand`, samme mønster som `gs.loot.stats`), del 1:
     - (h) alle effekter med styrke 1 samtidig gjennom `Combine` holder seg innenfor taket
     - (i) ingen gjenstand med `bNoLoot` kommer ut av 20 000 `RandomLoot` i Medical, Corridor, Canteen og Crate
     - (j) alle Id-ene Kamp bruker finnes, og alle Food og Drug har `Heal == 0`

     Hver linje logges med `PASS` eller `FAIL`, og til slutt «gs.fx.test: N/N OK».
7. **Bygg** med `Build.bat ... -WaitMutex`. Kjør `gs.fx.test` og `gs.loot.stats` hodeløst og lim resultatet inn i rapporten.
8. **Bestillinger.** Bestillingen K-FX til Kamp står allerede i `bestillinger.md` (skrevet av planleggeren). Den skal
   ikke skrives på nytt. Ellers:
   - Sett Kamps bestilling fra 27. nov. (plan-10) til LEVERT med:
     - Id-ene: Cigarettes/Royk, TurboPill (Amfetamin)/Turbo, Beer/Alkohol
     - visual 8–11, `Onset`, `Fade` og `UseAnim`
     - svar på R2 (`bNoLoot`) og B3 (taket, og motoren i 07b)
   - Ny bestilling til Grafikk: modeller i `gen_models.py` for sigarettpakke, ølboks, årepresse, skinne og nesespray.
     Vi bytter `arch` når modellene finnes.
9. **IMPROVEMENTS.md:** lag seksjonen «Runde 7a (Spillbarhet): realistiske stoffer og medisin» øverst, med linjene under.

## Ferdig når
- `gen_cpp.py` går uten feil i assert-ene. Bygget går uten feil og uten nye advarsler i våre filer.
- `gs.fx.test` viser (h), (i) og (j) som OK.
- `gs.loot.stats` viser 0 utelatte, og ingen av de tretten `bNoLoot`-tingene ligger i Helgasetr.
- I PIE gir fire stoffer samtidig ikke over +10 % fart. Mat og stoffer gir ingen helse, årepressen stopper blødning, og
  Pils og Sigarett kan tas.
- Kamps kode bygger uendret, og bestillingene er skrevet eller besvart.

## Risiko
- **Header-bytte** i `GSItems.h` og `GSTypes.h` gjør at alt må bygges på nytt. Gjør det i ett, tidlig. Forhåndsdeklarasjonen
  gjør at filer som bare inkluderer `GSItems.h` slipper ny bygging når `GSTypes.h` endres.
- **Replikering:** to nye felt i en replikert liste. Det er trygt for vert, klient og sen klient (forhåndssjekken). Det
  finnes ingen lagring av effekter.
- **Bein og armer før Kamps krok K-FX (4):** Kamp regner fortsatt behandling som `Heal/60`.
  - Medkit (35) gir da 0,58 i stedet for 1,25, så skrinet setter ikke lenger et bein helt i stand.
  - Smertestillende (heal 0) slutter å behandle lemmer.
  - Godtatt. Skriv det i rapporten.
- **Balanse:** stoffene blir svakere (ingen helse, ingen skuddsikkerhet) til overdose og innslag kommer med 07b og Kamps
  krok. Det er riktig retning.
- Kamps `UpdateLocalVisuals` faller tilbake til standardgrenen for visual 8–11 (mild kantfarge), uten feil.
- Kamps HUD merker en effekt som «dårlig» når `Spread > 1,01`, så Amfetamin, Adrenalin, Morfin og Alkohol vises som dårlige.
  Dette er meldt i K-FX, og vi endrer det ikke selv.

## IMPROVEMENTS-linjer
920. Tak på boosts: uansett hvor mange stoffer du tar blir du aldri mer enn 10 % raskere eller tåler mer enn 15 % mindre skade
921. Overnaturlige boosts (Spøkelse, Føniks, Øyet, Blodtørst, Berserk, Lynet, Iskald, Pansertablett m.fl.) finnes ikke lenger på kartet
922. Mat, adrenalin og morfin leger ikke lenger sår: mat metter, adrenalin og morfin demper smerte
923. Førstehjelpsskrin og stort sanitetssett gir litt helse med en gang og resten langsomt over halvannet minutt; det sjeldneste er best
924. Turbo heter nå Amfetamin og virker som det: mindre slitenhet, urolige hender, og et tungt krasj etterpå
925. Ny: Sigarett – litt roligere hender, men dårligere kondis og hoste etterpå
926. Ny: Pils – hver boks gjør siktet og ladingen dårligere
927. Ny: Årepresse (tourniquet) som stopper all blødning fra et bein eller en arm
928. Ny: Skinne for brukne bein og armer (på kartet når Kamp leser behandlingsfeltet)
929. Sovetabletten (før «Drøm») gjør deg ikke lenger immun mot blendgranater
930. Store måltider tar realistisk tid å spise: Grandiosa 12 s, kebab 10 s
931. Alle stoffer og all medisin har nye, nøkterne beskrivelser av virkning og bivirkning
932. Katalogen stopper seg selv hvis noen legger inn mat eller stoff som leger, eller gjør deg skuddsikker, usynlig eller udødelig
