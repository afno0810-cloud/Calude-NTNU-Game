# Plan 11 – lydrunde 1 (lag Kamp): ekte innspilte lyder og en lydmotor i C++

**Rekkefølge:** bygges etter 14b (`plan-08b.md`) og **før** plan-09 (se `retning.md`, 27. nov.). Planen virker også
uten at Andreas har sagt ja til nedlastingen: da får motoren (varianter, rom, okklusjon, miks, eksplosjonslyd) de
syntetiske lydene vi har, og de ekte lydene glir inn senere bare ved at filene kommer i `SourceArt/Sounds/`.
Andreas' ord 27. nov.: «mye bedre lydeffekter». Rammen: superrealistisk, ikke arkade.

---

## 0. Nedlastingslista – ÉN liste Andreas sier ja eller nei til

Ingenting lastes ned før Andreas har sagt ja i chatten. Hovedagenten viser ham denne lista (kort) og spør én gang.
Lisensene er sjekket på nett 27. nov. 2026 (kilder nederst i avsnittet).

| # | Hva | Lisens (kort) | Størrelse | Lenke | Hva vi bruker fra den |
|---|---|---|---|---|---|
| 1 | **Sonniss #GameAudioGDC 2020** | Sonniss GDC-lisens v2.0: gratis, kommersielt, i spill, **ingen navnekrav**; filene må ikke deles videre som lydfiler; ikke til KI-trening | 48,2 GB (14 zip) | https://sonniss.com/gameaudiogdc (zip: `downloads.sonniss.com/Sonniss.com-GDC2020-GameAudioBundle1of14.zip` … `14of14`) | **FLYSOUND AK‑74** (nær, langt unna, lang avstand, salve + skru fra hverandre), **FLYSOUND Mosin‑Nagant** (1 m, 50 m, fjernt + håndtering), **PMSFX Bullet Bys & Impacts** (kule i glass, i kropp, i jord, sus forbi), **Olivier Girardot Hand Guns** (håndvåpen, kulenedslag i stein), **Bluezone Tank Explosion** (stor eksplosjon ute) |
| 2 | **Sonniss #GameAudioGDC 2024** | samme | 23,5 GB (9 zip) | https://sonniss.com/gameaudiogdc (`…GDC2024-GameAudioBundle1of9.zip` … `9of9`) | **Pole Position Glock 17** (20 m foran, 100 m skrått), **Bluezone Broken Glass** (glass knuses, små skår, skår som faller), **Bluezone Detonation – Explosion** (eksplosjon i by, granat, hylser som håndteres), **Bluezone Building Collapse** (gips/betong-rester med glass), **344 Audio Flesh, Bones & Gore** (tunge kroppsknus), **Pole Position The Grunts Library** (smerte, gisp, pust), **West Wolf Quiet School Ambiences** (romtone i skolekorridor og klasserom, gammel skoledør), **West Wolf Quiet Hotels at Night 2** (romtone i korridor og trapperom), **TheWorkRoom Human Presence in Quiet Rooms** (folk i rom langt unna), **344 UK Shops** og **Omar Alvarado Minimal Ambiences** (ventilasjon/aircondition), **Ivo Vicic Industrial Gas Boiler Room** (teknisk rom), **Justsoundeffects Doors / Metal Movements** (heisdør, glassdør, panikkstang), **Sonic Bat Videogame Foley Essentials** (dørhåndtak), **RYK-Sounds Footstep** (tregulv), **Toneglow Wind and Rain Interior**, **Pole Position IKV91** (ammunisjon som faller på tre) |
| 3 | **Sonniss #GameAudioGDC 2026** | samme | 7,5 GB (5 zip) | https://gdc.sonniss.com (`downloads.sonniss.com/Sonniss.com-GDC2026-GameAudioBundle1of5.zip` … `5of5`) | **InMotionAudio USA Hotel** (ståldør i trapperom), **Sonik Spanish Crowds** («30 personer i arbeid», fjern mumling), **Jake Fielding Interior Wind Rain and Storms** (regn/hagl på vindu, torden inne), **344 East Coast America** (summing fra lysrør), **344 Cinematic Fight** (slag mot kropp), **ESM HD Game Materials** (metall-treff) |
| 4 | **Sonniss #GameAudioGDC 2019** | samme | 26,2 GB (8 zip) | https://sonniss.com/gameaudiogdc (`…GDC2019-GameAudioBundle1of8.zip` …) | **Airborne Sound Elements Glass** (tykt planglass som knuses og velter – til glassveggene i Helgasetr), **Coll Anderson House Library** (tredør som slås i stykker, lyspære som knuses), **Coll Anderson Battle Crowd** (spark mot tredør, skritt i rusk med glass), **3maze Doors** (dør som lukkes, lås), **PMSFX BLOODBATH** (stikk, blod) |
| 5 | **Snake's Authentic Gun Sounds** (itch.io) | «All sounds can be used commercially, and credit is not needed» (sidens egen tekst; ingen formell lisens – vi lagrer siden som tekst) | 11 MB | https://f8studios.itch.io/snakes-authentic-gun-sounds («Download» → «No thanks, just take me to the downloads») | Ekte skudd **.22LR, 5,56, 7,62×39, 7,62×54R**, hvert med og uten naturlig etterklang; pumpe, bolt, AK-ladegrep, AR-sluttstykke, flere ladelyder, enkeltløps hagle |
| 6 | **Kenney Impact Sounds** | CC0 (fritt, ingen krav) | ca. 5 MB, 130 filer | https://kenney.nl/assets/impact-sounds | Fottrinn (betong, tre, teppe m.m.), treff i glass/metall/tre/plate – reserve og ekstra varianter |
| 7 | **Kenney Interface Sounds** | CC0 | ca. 5 MB | https://kenney.nl/assets/interface-sounds | Klikk og bekreftelser i menyen (nøkterne, ikke spill-jingler) |

**Valgfritt senere (ikke en del av ja-et):** Freesound.org med filteret «Creative Commons 0» (krever en gratis konto
Andreas lager selv; vi lager den ikke). Aktuelle: `acidsnowflake` «AK47 and various other gunshots at indoor shooting
range» (id 402791, CC0 – skudd innendørs), `morganpurkis` «Silenced Gunshot 4» (id 396015, CC0). Lyra fra Fab (Epic-
lisens, bare for Unreal-prosjekter) har ordentlige fottrinn per underlag, men krever Epic-launcheren og er mer «spill»
enn ekte; tas bare hvis lista over ikke holder.
**Valgt bort:** BBC Sound Effects (ikke kommersielt), Zapsplat gratis (krever navn i rulletekst), Pixabay (lov, men
ujevn kvalitet), Epic Starter Content-lyder (for svake).

**Til Andreas (forslag til tekst for hovedagenten):** «For å få ekte lyd i spillet vil jeg laste ned gratis
lydsamlinger fra Sonniss (fire år, ca. 105 GB, lov å bruke i spill uten å nevne dem), en liten gratis pakke med ekte
skudd fra itch.io og to små pakker fra Kenney (helt fri bruk). Råfilene legges utenfor prosjektet, og bare de ca. 300
lydene vi bruker kopieres inn. Har du ca. 110 GB ledig plass (du har 162 GB nå)? Er det greit at jeg laster ned?»

**Hvis Andreas sier ja bare til noe:** 5 + 6 + 7 alene (21 MB) gir ekte skudd for gevær/pistol/snikskytter/hagle og
fottrinn; 2 alene gir glass, eksplosjoner og Helgasetr-atmosfære. Rekkefølge ved nedlasting: 5, 6, 7, 2, 1, 4, 3.

### Hvor ting ligger, og lisensene
- **Råfiler (aldri i git):** `C:\Users\afno0\Documents\Lydbibliotek\<kilde>\` (f.eks. `Sonniss_GDC2024\`). Sonniss-zipene
  pakkes **ikke** ut i sin helhet: `Tools/Audio/fetch_library.py` leser zipens innholdsliste og trekker bare ut filene i
  `sources.json` (Pythons `zipfile`), så toppen er én zip-størrelse og ikke dobbel. Zipen slettes etter uttrekk først
  når Andreas har sagt ja til det (sletting av noe han har lastet ned = spør).
- **Ferdige lyder:** `SourceArt/Sounds/S_GS_<Navn>_<n>.wav` (samme mappe og import som i dag – `gs_sounds.py` trenger
  ingen endring). Lyder laget av Sonniss-filer skal ikke ut på GitHub (lisensen forbyr å dele dem videre som lydfiler):
  `prep_sounds.py` skriver `SourceArt/Sounds/.gitignore` med navnet på hver slik fil. CC0-, Kenney- og Snake-filer kan
  committes. Hos andre som kloner prosjektet faller spillet tilbake til de syntetiske lydene.
- **`SourceArt/Sounds/CREDITS.md`** (generert): én linje per ferdig fil – vårt navn, kilde, bibliotek, utgiver,
  originalfilnavn, lisens, lenke, hva som er gjort (klipt, mono, normalisert). Vi krediterer alle, selv der det ikke kreves.
- **`SourceArt/Sounds/LICENSES/`:** `Sonniss_GDC_License_v2.0.txt` (lagret fra sonniss.com/gdc-bundle-license på
  nedlastingsdagen), `CC0-1.0.txt`, `Snake_itch_terms.txt` (sidens tekst med dato).
- **Regel for agentene:** Sonniss-lyder brukes aldri som inndata til KI-verktøy som lager lyd (lisensen forbyr KI-trening).
  Å klippe og normalisere dem med vårt eget skript er vanlig bruk.

Kilder (sjekket 27. nov. 2026): https://sonniss.com/gdc-bundle-license/ · https://sonniss.com/gameaudiogdc ·
https://gdc.sonniss.com · sporlistene (Google-regneark lenket fra de to Sonniss-sidene) ·
https://f8studios.itch.io/snakes-authentic-gun-sounds · https://kenney.nl/assets/impact-sounds ·
https://freesound.org (CC0-søk).

---

## Ærlig status i dag (lest i koden 27. nov.)
- Alle 48 lydene er syntetisert med numpy (`Tools/Audio/gen_sounds.py`: støyutbrudd, filtrerte dunk, kunstig
  etterklang). Det høres ut som et spill fra 2005: skuddene er «pfff» uten mekanikk og uten ekte smell, fottrinn er
  klikk, treff er dunk. Ingen mengde etterbehandling gjør syntetisk støy til en ekte AK-74. **Dette er den største
  kvalitetsforskjellen som er igjen i spillet.**
- `GSAudio` er grei grunnmur: lyd etter navn, egen dempingskurve per rekkevidde, lavpass med avstand, «Variant» med
  fast antall. Men: ingen okklusjon (skudd bak tre betongvegger høres like klart), ingen romklang (alt låter likt i
  atriet og i et grupperom), ingen miks/prioritet (40 hylser kan drukne et fottrinn), egen skuddlyd er samme fil som
  andres, og `PlayShotFor` har campus-logikk (ekko fra «åsene» over 60 m) som ikke passer et innendørskart.
- **Eksplosjoner er stumme**: `AGSProjectile::OnRep_Exploded` → `SpawnExplosion` spiller ingen lyd (granat, rakett, mine).
- `Bullet_Crack`/`Bullet_Whiz` har ingen `.imported`-merke: de er ikke importert ennå (importeres neste gang editoren
  åpnes – sjekk at de kommer).
- Fottrinn kjenner bare «gress» og «hardt» (materialnavn inneholder `Grass`). Helgasetr har terrazzo, parkett,
  linoleum, stål-/gummitrinn og betong (helgasetr.md kap. 10).
- Ambience (`GSEnvironment`, Grafikks fil) er vind/regn/fugler/by-natt for utendørs campus.

## Mål
Hvert skudd, fottrinn og treff høres ekte ut og høres forskjellig ut i atriet, i en korridor og bak en vegg;
eksplosjoner smeller; egne skudd har mekanikk og trykk; fottrinn og flagg-radio drukner aldri.

## Forbedrer (gammelt)
- `GSAudio::Get`/`Variant`: finner selv hvor mange varianter som finnes (`_1` … `_16`), trekker aldri samme to ganger
  på rad, og faller tilbake til dagens enkeltnavn – så de syntetiske lydene virker til de ekte kommer.
- `PlayShotFor`/egen skuddlyd i `FireOnce`: lag i stedet for én fil – nær (smell + mekanikk), middels, fjern/bak vegg,
  og hale etter rommet skytteren står i (lite rom / korridor / stort rom / trapperom / ute). Campus-ekkoet fjernes.
- Fottrinn: underlag fra fysisk materiale (Grafikks `PM_GS_*`, runde 5) med materialnavn som reserve; seks underlag;
  utstyrsrasling ved sprint; landing og glid følger underlaget.
- Kulenedslag (`AGSShortFX::SpawnImpact`): lyd etter materiale (betong/stein, gips, tre, metall, glass, kropp).
- Lydstyrke-/tonehøydevariasjon på alt (i dag bare noen steder).

## Nytt
- **Okklusjon i kode:** felles dempingsobjekter får `bEnableOcclusion` (spor mot `ECC_Visibility`, lavpass ~900 Hz,
  −9 dB, 0,15 s overgang). Et skudd bak en vegg blir dumpt og lavere, ikke borte.
- **Romklang i kode:** `UReverbEffect` lages i C++ (ingen editor-fil) per romtype og slås på med
  `UGameplayStatics::ActivateReverbEffect` etter hvor lytteren står. Til Grafikks `GSHelgasetr::SoundZones()` finnes,
  måles rommet automatisk: 8 vannrette + 1 loddrett spor fra lytteren hvert 0,25 s → romstørrelse og takhøyde →
  RT60-estimat (Sabine-aktig) → nærmeste av fem forhåndsinnstillinger. Når `SoundZones()` kommer, bytter én funksjon
  til sonens RT60.
- **Miks i kode:** `USoundClass` per kategori (Våpen, Fottrinn, Treff, Kropp, Eksplosjon, Radio/UI, Atmosfære) lages i
  C++ og settes på lydene når de lastes; `USoundConcurrency` per kategori (hylser maks 6, nedslag maks 12, fottrinn maks
  16 – stopper de svakeste, skudd maks 24); `USoundMix` «Trykkbølge» (alt unntatt Radio dempes og lavpasses i 1,5–4 s
  etter en nær eksplosjon, med øresus). **Fottrinn og Radio dempes aldri av miksen og har høyest prioritet**
  (Andreas: «fottrinn og flagg alltid høres»).
- **Lydens forsinkelse med avstanden:** skudd og eksplosjoner forsinkes `avstand / 343 m/s` (over 0,03 s) med en liten kø i
  `GSAudio` (ingen timer per lyd). Innendørs er det maks ca. 0,2 s – lite, men riktig, og det gjelder når vi går ut.
- **Eksplosjonslyd** (i dag ingen): nær / innendørs / fjern + rusk som faller, med trykkbølge-miks innen 10 m.
- **Lydbibliotek-verktøy:** `Tools/Audio/sources.json` (fasit: hvilken originalfil blir hvilket navn), `fetch_library.py`
  (henter bare valgte filer ut av zipene), `prep_sounds.py` (48 kHz, mono for 3D / stereo for 2D og atmosfære, klipp
  stillhet, 5 ms inn/ut-toning, normalisering per kategori, skriver `CREDITS.md` og `.gitignore`).
- `GSAudio::PlayUI(Name)` og `GSAudio::PlayRadio(Name)` for andre lag (meny, flagg-radio), og ferdige navn for
  dører/glass/møbler som Interaktiv verden kan spille (se bestillinger).

## Hvorfor nå
Andreas ba om «mye bedre lydeffekter» (27. nov.), og lyd er det som er svakest i spillet målt mot «superrealistisk».
Det trenger ikke kroppen fra 14b, ikke katalogsvar og ikke nye kartdata (motoren måler rommet selv), og den rører
andre funksjoner enn plan-09. Nedlastingsspørsmålet kan gå til Andreas nå mens 14b bygges.

## Filer
- `Source/Gloshaugen/Public/GSAudio.h`, `Private/GSAudio.cpp` (mesteparten).
- `GSCharacter.cpp`: bare lydlinjene i `FireOnce` (egen skuddlyd), `MulticastShotFX`/`MulticastPelletFX` (uendret kall,
  ny logikk i `PlayShotFor`), fottrinn/landing i `Tick`, ladelydene (navn byttes til våpentypens varianter).
- `GSEffects.cpp` (`SpawnImpact`: materiale inn), `GSProjectile.cpp` (`OnRep_Exploded`: eksplosjonslyd),
  `GSBallistics.cpp` (crack/whiz: varianter, navn uendret).
- Nye: `Tools/Audio/sources.json`, `Tools/Audio/fetch_library.py`, `Tools/Audio/prep_sounds.py`,
  `SourceArt/Sounds/CREDITS.md`, `SourceArt/Sounds/LICENSES/*`, `SourceArt/Sounds/.gitignore` (generert).
- `Gloshaugen.Build.cs`: ingen ny modul trengs (`AudioMixer` trengs ikke for `UReverbEffect`/`USoundMix`; sjekk at
  `Engine` dekker `Sound/ReverbEffect.h`, `Sound/SoundMix.h`, `Sound/SoundConcurrency.h`, `Sound/SoundClass.h`).
- **Ikke:** `Content/Python/gs_sounds.py` (Grafikks; importen virker som den er), `GSEnvironment.*` (Grafikks), `GSMenu.*`,
  `GSHUD.cpp`s flagg-radio bare hvis navnet byttes (Kamps fil, valgfritt).

## Steg
1. **Navn og varianter (`GSAudio`).** `GSAudio::Pick(const FString& Name)`: første gang et navn spørres, prøv
   `S_GS_<Name>_1` … `_16` (stopp ved første som mangler) og husk antallet; 0 varianter → bruk `S_GS_<Name>`
   (dagens fil). Trekk tilfeldig, aldri samme indeks to ganger på rad per navn. `PlayAt`/`Play2D` bruker `Pick`.
   `Variant()` beholdes som tynn innpakning (gamle kall virker). Bare spillmaskiner (`CanPlay`) som i dag.
   Navneregel for nye filer (fasit i `sources.json`):
   - Skudd: `Shot_<Klasse>_Close_n`, `Shot_<Klasse>_Mid_n`, `Shot_<Klasse>_Far_n`, `Shot_<Klasse>_FP_n` (egen, 2D),
     `Shot_<Klasse>_Mech_n` (mekanikk-klikk, egen); klasser: `Pistol`, `SMG`, `Rifle`, `Sniper`, `Shotgun`, `LMG`,
     `Launcher`, `Silenced`. Haler: `Tail_Small_n`, `Tail_Corridor_n`, `Tail_Large_n`, `Tail_Stair_n`, `Tail_Out_n`.
   - Fottrinn: `Step_<Underlag>_n` med `Stone` (terrazzo), `Wood` (parkett), `Lino`, `Metal`, `Concrete`, `Carpet`,
     `Grass`, `Glass` (skår); `Land_<Underlag>_n`; `Gear_Rattle_n`; `Slide_<Underlag>_n`.
   - Nedslag: `Impact_<Mat>_n` med `Concrete`, `Plaster`, `Wood`, `Metal`, `Glass`, `Dirt`; `Hit_Body_n`, `Hit_Head_n`,
     `Hit_Armor_n`; `Bullet_Crack_n`, `Bullet_Whiz_n`.
   - Mekanikk: `Mag_Out_<Type>_n`, `Mag_In_<Type>_n`, `Bolt_<Type>_n` (`Rifle`, `Pistol`, `Sniper`), `Pump_Back_n`,
     `Pump_Fwd_n`, `Shell_<Mat>_n`, `Dry_Fire_n`.
   - Eksplosjon: `Explosion_Close_n`, `Explosion_Indoor_n`, `Explosion_Far_n`, `Debris_Fall_n`.
   - For andre lag: `Door_Wood_Open_n`, `Door_Wood_Close_n`, `Door_Wood_Kick_n`, `Door_Wood_Break_n`,
     `Door_Glass_Open_n`, `Door_Glass_Close_n`, `Door_Metal_Stair_n`, `Door_Pushbar_n`, `Codelock_Beep_n`,
     `Glass_Break_Large_n`, `Glass_Break_Small_n`, `Glass_Shards_n`, `Plaster_Break_n`, `Furniture_Topple_n`,
     `Furniture_Scrape_n`, `Screen_Break_n`, `UI_Click_n`, `UI_Confirm_n`, `UI_Back_n`, `Radio_Squelch_n`,
     `Amb_Corridor`, `Amb_Classroom`, `Amb_Atrium`, `Amb_Stair`, `Amb_Vent`, `Amb_Tech`, `Amb_Walla_Far`,
     `Amb_Rain_Window`, `Amb_Light_Hum`.
2. **Kategorier, miks og samtidighet.** I `GSAudio.cpp`: `enum class EGSSnd : uint8 { Weapon, Step, Impact, Body, Blast,
   Radio, Ambience, Mech }`, utledet av navnets forstavelse (`Shot_`/`Tail_` → Weapon, `Step_`/`Land_`/`Gear_`/`Slide_` →
   Step, osv.). Per kategori ett `USoundClass` og ett `USoundConcurrency` laget med `NewObject` + `AddToRoot` (som
   dempingsobjektene i dag). Sett `S->SoundClassObject` når en lyd lastes første gang (endrer bare den lastede kopien;
   kall ikke `Modify()`). Send `ConcurrencySettings` i `PlaySoundAtLocation`/`SpawnSoundAtLocation`. Prioritet:
   Radio 4, Step 3, Weapon 2, Blast 2, resten 1 (`Priority` på klassen / `PriorityToUse` hvis tilgjengelig – les
   `SoundBase.h` i UE 5.8 og bruk det som finnes). Concurrency: Step 16 (`StopQuietest`), Impact 12, Mech 6
   (hylser), Weapon 24 (`StopOldest`), Body 8.
3. **Okklusjon.** I `Attenuation(RangeM)`: `bEnableOcclusion = true`, `OcclusionTraceChannel = ECC_Visibility`,
   `OcclusionLowPassFilterFrequency = 900`, `OcclusionVolumeAttenuation = 0.35`, `OcclusionInterpolationTime = 0.15`,
   `bUseComplexCollisionForOcclusion = false`. Egen variant for Radio/2D: ingen okklusjon. Merk: motoren sporer for
   hver aktive 3D-lyd hver ramme (asynkront); hold korte lyder korte (klipp i `prep_sounds.py`) så det ikke blir mange.
   Mål `stat audio` (antall aktive lyder) i en kamp med 6 bots før/etter; logg det med `UE_LOG(LogTemp, Log, ...)`.
4. **Romklang.** `GSAudio::RoomAt(UWorld*, FVector) -> FGSRoom { float RT60; float SizeM; bool bOutside; }`:
   8 vannrette spor (45°) maks 40 m + 1 opp maks 15 m (`ECC_WorldStatic`, objektspor – se lærdom om kanalspor og
   kapsler); ingen tak eller snitt > 30 m → ute. RT60 ≈ 0,16·V / (0,1·S) begrenset til 0,3–3,0 s. Fem forhåndsvalg:
   Lite (≤ 0,6 s), Korridor (lang og smal: største/minste > 3, 0,8–1,2 s), Stort (> 1,6 s), Trapperom (takhøyde > 8 m
   og liten flate, 2 s, mer ekko), Ute. Lytterens rom måles hvert 0,25 s i en `FTSTicker`/verdens-tick i `GSAudio`
   (statisk tilstand per verden – rydd ved `OnWorldCleanup`), og ved bytte kalles
   `UGameplayStatics::ActivateReverbEffect(World, Preset, "GSRoom", 1.f, 1.f, 0.4f)` med et `UReverbEffect` per
   forhåndsvalg laget i C++ (`DecayTime` = RT60, `Density`, `Diffusion`, `GainHF` lavere i små myke rom,
   `ReflectionsDelay`/`LateDelay` etter størrelse). Dempingsobjektene: `bEnableReverbSend = true`, mer romklang på
   avstand (`ReverbWetLevelMin 0.2`, `ReverbWetLevelMax 0.8`). **Krok for plan-12:** `RoomAt` sjekker først
   `GSHelgasetr::SoundZones()` hvis den finnes (Grafikk) – en `#if`/navnesjekk er ikke mulig i C++, så legg kallet i én
   funksjon `ZoneRT60(FVector, float& Out)` som returnerer `false` til plan-12 kobler den.
5. **Skudd i lag (`PlayShotFor` og egen skuddlyd).**
   - Avstand lytter–skytter `d` (m), skytterens rom `R = RoomAt(Start)` (måles én gang per skudd, men caches per skytter
     i 0,5 s – automatild skal ikke spore 9 ganger per kule).
   - `d < 25`: `Shot_<K>_Close` (full), `25–60`: `Shot_<K>_Mid`, `> 60` eller okkludert: `Shot_<K>_Far` (okklusjonen gjør
     resten). Overgang: begge i 20–30 m og 55–65 m med lineær vekt (to lyder er greit; aldri tre).
   - Hale: `Tail_<R>` fra skytterens posisjon, volum 0,5–0,8, uten okklusjon av halen (halen fyller rommet). Ute: `Tail_Out`.
   - Egen skuddlyd (`FireOnce`, `bHuman`): `Shot_<K>_FP` (2D, stereo) + `Shot_<K>_Mech` (2D, 0,5) + hale fra eget rom
     (3D ved munningen, så romklang og retning stemmer). Mangler `_FP` → dagens `ShotName` (uendret oppførsel).
   - Lyddempet: `Shot_Silenced_*` + tydelig mekanikk (sluttstykket høres mer enn smellet). Bue: som i dag.
   - Avstandsforsinkelse: `d/343` s via køen i steg 7 (ikke på egne skudd).
   - Bort: «ekko fra åsene» over 60 m (`Shot_Distant`), fordi kartet er innendørs; `Shot_Distant` blir reserve for `_Far`.
6. **Fottrinn, landing, glid (`GSCharacter::Tick`, l. ca. 4160–4190 og 4057).** Ny `GSAudio::SurfaceAt(const FHitResult&)
   -> FName` (`Stone`/`Wood`/`Lino`/`Metal`/`Concrete`/`Carpet`/`Grass`/`Glass`): sporet får
   `Params.bReturnPhysicalMaterial = true`; navnet på `Hit.PhysMaterial` (`PM_GS_Stein` → Stone, `PM_GS_Tre` → Wood,
   `PM_GS_Stal` → Metal, `PM_GS_Betong` → Concrete, `PM_GS_Stoff` → Carpet, `PM_GS_Glass` → Glass, `PM_GS_Gips` → Concrete),
   ellers materialnavnet (`Terrazzo`, `Parkett`, `Lino`, `Steel`, `Grass` …), ellers `Concrete`. Mangler `Step_<U>` →
   `Step_Hard`/`Step_Grass` (dagens). Sprint: `Gear_Rattle` hvert 2. steg, 0,35. Huk: 0,25 som i dag. Landing og
   `Slide` følger underlaget. **Plan-09 endrer `StartSlide` og landing: rør bare lydlinjene.** Rekkevidde: gange 18 m,
   sprint 30 m (som i dag), men Step-klassen dempes aldri av miksen.
7. **Forsinkelseskø.** `GSAudio::PlayAtDelayed(Context, Name, Loc, Vol, Range, Pitch, DelayS)`: under 0,03 s → spill nå;
   ellers legg i en `TArray` per verden som tømmes i samme tick som romklangen (maks 64 i køen; eldste kastes).
8. **Eksplosjoner (`GSProjectile.cpp`, `OnRep_Exploded`, grenen som kaller `SpawnExplosion`).** Kall
   `GSAudio::PlayExplosion(World, Loc, Radius)`: `Explosion_Close` (< 15 m) / `Explosion_Indoor` (innendørs rom) /
   `Explosion_Far` + `Debris_Fall` 0,4 s etter; forsinket som skudd. Innen 10 m av lytteren: `PushSoundMixModifier`
   «Trykkbølge» (alle klasser unntatt Radio og Step: −12 dB og lavpass 600 Hz, 1,5 s – 4 s etter avstand) +
   `Ear_Ring` 2D; fjern med `PopSoundMixModifier` via køen i steg 7. Blendgranat (`flash`): kort smell + lang øresus
   (4 s) når lytteren er innenfor radius. Flagg: Radio dempes aldri.
9. **Nedslag (`AGSShortFX::SpawnImpact`).** Ny parameter `FName Surface = NAME_None` (standard → `Concrete`); kallerne
   som har `FHitResult` sender `GSAudio::SurfaceAt(Hit)`. `Impact_<Surface>`; mangler den → dagens `Impact_1..3`.
   Treff i kropp/vest/hode bruker variantene (`Hit_Body_n` osv.) – navnene er de samme som i dag.
10. **Lading og mekanikk.** Ladetimerne (l. ca. 1385–1396) og pumpe/bolt (l. ca. 960–973) bruker `Mag_Out_<Type>`,
    `Mag_In_<Type>`, `Bolt_<Type>` (Type fra klassen: Pistol/SMG → Pistol, Sniper → Sniper, ellers Rifle), med dagens
    navn som reserve. Tidspunktene endres ikke (plan-09 tar ladingen på kroppen).
11. **Funksjoner for andre lag.** `GSAudio::PlayUI(const UObject*, const TCHAR* Name)` (2D, Radio-klassen, ingen
    variasjon i tonehøyde) og `GSAudio::PlayRadio(...)` (2D, squelch før og etter: `Radio_Squelch`). Flagg-radioen i
    `GSHUD.cpp` l. ca. 1011 bruker `PlayRadio` (Kamps fil). Ingen endring i andre lags filer – de får bestillinger.
12. **Lydbibliotek-verktøyene (`Tools/Audio/`).**
    - `sources.json`: liste med `{ "name": "Shot_Rifle_Close_1", "src": "Sonniss_GDC2020/<sti i zip>", "start": 0.0,
      "len": 1.2, "channels": "mono", "gain_db": 0, "category": "Weapon", "license": "Sonniss-GDC-2.0",
      "library": "AK 74", "publisher": "FLYSOUND", "url": "https://sonniss.com/gameaudiogdc" }`. Byggeren fyller den
      etter å ha lyttet/sett på bølgeformene (lengde, topp, stillhet) – **ikke gjett filstier**: list zipenes innhold med
      `fetch_library.py --list <zip> --grep <ord>` og velg derfra.
    - `fetch_library.py`: `--list` / `--extract` (bare filene i `sources.json`) fra `GS_SOUND_LIBRARY`
      (standard `C:\Users\afno0\Documents\Lydbibliotek`). Kjør med `python -I`, og bare på zip-filer i den mappen.
    - `prep_sounds.py`: les 16/24/32-bit WAV med `wave` + numpy (24-bit pakkes ut manuelt), resample til 48 kHz med
      `scipy.signal.resample_poly` (scipy 1.17 finnes), mono = snitt av kanalene for 3D-lyder, klipp stillhet under −60
      dBFS i endene, 5 ms inn-/uttoning, normaliser topp per kategori (Weapon −1 dBFS, Step −6, Impact −4, Ambience
      RMS −24 dBFS, UI −10), skriv 16-bit WAV. Skriv `CREDITS.md` og `.gitignore` (alle navn med lisens Sonniss).
      Sletter aldri filer i `SourceArt/Sounds/` som ikke står i `sources.json` (de syntetiske blir værende som reserve).
    - `gen_sounds.py` blir stående som reserve. Ikke kjør den på nytt over ekte filer (den skriver bare sine egne navn).
13. **Til nedlastingen er godkjent:** steg 1–11 bygges og testes med de syntetiske lydene; steg 12 lages og prøves på
    Kenney/Snake-filene hvis de er godkjent, ellers bare med en egen testfil laget av `gen_sounds.py`.
14. **Uten editor:** `gs_sounds.py` importerer nye WAV-er neste gang editoren åpnes (det skjer av seg selv). Byggeren
    skriver i rapporten at Andreas (eller en hodeløs kjøring) må åpne editoren én gang for at de nye lydene skal finnes.

## Hva som kan gjøres i C++, og hva som må via Python/editoren (vurdering)
| Ønske | Hvordan | Hvor |
|---|---|---|
| Flere varianter, tilfeldig valg, tonehøyde/volum-variasjon | `GSAudio::Pick` + `FMath::FRandRange` | C++ (ingen Sound Cue trengs) |
| Demping med avstand, lavpass i luft | `USoundAttenuation` laget med `NewObject` (finnes i dag) | C++ |
| Okklusjon (vegg mellom) | `FSoundAttenuationSettings::bEnableOcclusion` m.fl. | C++ (av/på-okklusjon; vegg-for-vegg med materialer i plan-12) |
| Romklang per rom | `UReverbEffect` laget i C++ + `ActivateReverbEffect` etter lytterens rom | C++ (Audio Volumes er børster som må bygges i editoren – brukes ikke) |
| Miks, prioritet, «alltid hørbar» | `USoundClass`, `USoundMix` (`PushSoundMixModifier`), `USoundConcurrency` laget i C++ | C++ |
| Avstandsforsinkelse | egen kø i `GSAudio` | C++ |
| Importere WAV, sette løkke på atmosfære | `gs_sounds.py` (finnes) | Python i editoren |
| Sound Cue-/MetaSound-grafer | `SoundCueFactoryNew` kan lage en tom cue fra Python, men nodene (Random, Modulator) er ikke tilgjengelige fra Python; MetaSound Builder (`UMetaSoundBuilderSubsystem`) kan bygge grafer fra C++/Python, men er tungt og gir lite her | **Ikke brukt i plan-11.** Vurderes i plan-12 bare for løkkelyd med parametre (automatild uten hull, ventilasjon som varierer) |
| Lagrede lydfiler for klasser/miks (så Andreas kan skru i editoren) | Python kan lage `SoundClass`/`SoundMix`/`ReverbEffect`-filer med fabrikker | Valgfritt senere; C++-objektene gjør jobben, og hodeløse tester kan prøve dem |

## Ferdig når
- `Build.bat … -WaitMutex` bygger uten feil (dirigentens bygg).
- Uten nye filer: spillet høres ut som i dag (alle reservenavn virker), pluss: eksplosjoner har lyd, skudd bak vegg er
  dumpe, romklang endres når du går fra en korridor ut i atriet (logg: `GSAudio: rom Korridor → Stort (RT60 2.4)`).
- Med godkjente filer: `SourceArt/Sounds/` har minst 3 varianter for hvert skudd-lag per klasse som har kilde (Pistol,
  Rifle, Sniper, Shotgun, LMG, Silenced), 4+ fottrinn per underlag (Stone, Wood, Metal, Concrete, Carpet), 3+ nedslag per
  materiale, glass, eksplosjon; `CREDITS.md` har én linje per fil; `.gitignore` dekker alle Sonniss-filer, og
  `git status` viser ingen Sonniss-avledede WAV-er.
- Hodeløs test (UE_LOG): 100 skudd fra en bot i samme rom → ingen samme variant to ganger på rad, maks aktive lyder
  ≤ 48 (`stat audio`-tallet logges), køen aldri > 64.
- Fottrinn og flagg-radio høres under en eksplosjon 5 m unna (Step/Radio ikke i trykkbølge-miksen).

## Risiko
- **Lisens:** Sonniss-filer på GitHub = deling. `.gitignore` må genereres før første `git add`; kontrolløren sjekker
  `git ls-files SourceArt/Sounds` mot `CREDITS.md`.
- **Ytelse:** okklusjon sporer per aktiv lyd; automatild med hale per skudd kan gi mange lyder. Concurrency og korte
  filer holder det nede; mål `stat audio` og ramme-tid før/etter (RTX 3050-laptop).
- **Romprøven** feiltolker åpne rom med glassvegger (glass blokkerer ikke `Visibility` i dag) – ok for v1; plan-12
  bruker Grafikks soner.
- **Online:** ingen ny replikering. Alt er lokalt per maskin og bygger på multicastene som finnes. Dedikert server
  spiller aldri lyd (`CanPlay`).
- **Andres filer:** plan-09 endrer `StartSlide`, landing, lading – rør bare lydlinjene. Grafikk eier `GSEnvironment`
  og `gs_sounds.py`; ingen endring der.
- **Tom import:** `Bullet_Crack`/`Whiz` mangler `.imported` – sjekk at importen virker etter at editoren har vært åpen.

## IMPROVEMENTS-linjer (662–677)
662. Skudd fra andre høres i lag: nært smell med mekanikk, dempet på middels hold og et dumpt drønn langt unna, med myk overgang
663. Skuddet får etterklang etter rommet skytteren står i: kort i grupperom, lang i atriet, hard og ekkoende i trapperommet
664. Egne skudd har en egen nærlyd med trykk og et tydelig klikk fra mekanismen, og romklangen kommer fra rommet du står i
665. Lyd bak vegger blir dumpere og lavere i stedet for å høres like klart gjennom betong
666. Romklangen skifter jevnt når du går fra en korridor til atriet eller inn i et trapperom, uten at kartet trenger egne lydsoner
667. Eksplosjoner fra granater, raketter og miner har lyd: et kraftig smell nært, et drønn innendørs og rusk som faller etterpå
668. En eksplosjon nær deg demper alt annet et par sekunder og gir øresus, men fottrinn og flagg-radio høres fortsatt
669. Smellet fra skudd og eksplosjoner kommer litt etter glimtet når det er langt unna (lydens fart, 343 m/s)
670. Alle lyder har flere varianter, og den samme varianten spilles aldri to ganger på rad
671. Fottrinn høres forskjellig på terrazzo, parkett, linoleum, stål, betong og teppe, og landing og glid følger underlaget
672. Når du sprinter, rasler utstyret ditt hørbart for dem rundt deg
673. Kulenedslag høres etter materialet: betong, gips, tre, metall, glass og jord
674. Lading, pumpe og bolt har egne lyder per våpentype
675. Lyder har prioritet: mange hylser og nedslag kan aldri drukne fottrinn eller flagg-radioen
676. Ekte innspilte lyder (Sonniss, Snake's Authentic Gun Sounds, Kenney) erstatter de syntetiske der de finnes, med kreditering av hver fil
677. Et nytt verktøy henter, klipper og normaliserer lydfiler fra lydbiblioteket, så nye ekte lyder bare trenger én linje i en liste
