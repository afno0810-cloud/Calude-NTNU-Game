**GODKJENT** – med tre rettinger til neste Kamp-runde. F1 (lydtaket for våpen kutter egne skudd og romhalen i store kamper) tas først; den er liten.

Kontrollert: `01c0051` (Kamp lydrunde 1) mot `ko/plan-11.md`, med `git show 01c0051:<fil>` for `GSAudio.h/.cpp`,
`GSCharacter.cpp`, `GSProjectile.cpp`, `GSEffects.*`, `GSBallistics.cpp`, `GSHUD.cpp`, `GSMenu.cpp`, `GSMapState.cpp`, lyd-
mappen, lisensene og `Tools/Audio/`. Motorkoden i `UE_5.8/Engine/Source` er slått opp for okklusjonsintervall og
samtidighetsregler. Jeg har ikke bygget selv; byggeren meldte at bygget lyktes (Succeeded) og at den hodeløse selvtesten besto.

## Bygg og kall fra andre lag
- **`SpawnImpact`:** den nye parameteren `FName Surface = NAME_None` står sist og har en standardverdi. Interaktivs kall i
  `GSMapState.cpp:927` (`Interaktiv.Knus`, tre argumenter) kompilerer uendret og gir betong. Alle kallene våre som har en
  `FHitResult` (kuler i `GSBallistics.cpp:358`, nærkamp i `GSCharacter.cpp:1184`) sender `SurfaceAt(Hit)`, og sporene har fått
  `bReturnPhysicalMaterial`. ✔
- **`GSMenu.cpp` mot `PlayUI`:** kallene `GSAudio::PlayUI(PC, TEXT("UI_Click"/"UI_Confirm"/"UI_Back"))` passer til
  signaturen `(const UObject*, const TCHAR*, float = 1)`. `UI_` havner i Radio-klassen (`bIsUISound`, høres også når spillet
  står på pause, og dempes ikke av miksen). Filene `UI_Click` ×5, `UI_Confirm` ×4 og `UI_Back` ×4 finnes. ✔
- Interaktivs `PlayPartSound` (`GSMapState.cpp:674`) kaller `GSAudio::Get(Name)` som en sjekk og deretter `PlayAt`. Det gir to
  trekninger, men aldri samme variant to ganger på rad. Det er harmløst (se forslag).

## Vert, klient, sen klient og bots
- **Egne skudd:** `FireOnce` kjører `PlayOwnShot` bare når `bHuman = IsLocalHuman()`. `MulticastShotFX` og `MulticastPelletFX`
  returnerer for `IsLocalHuman()`, så den som skyter, hører ikke skuddet sitt to ganger. Det gjelder både verten og klienten. ✔
- **Bots på verten:** `IsLocalHuman()` er usann, så multicasten spiller `PlayShotFor` (lag, hale og forsinkelse) hos verten
  og hos alle klienter. Bots får ingen lyd for pumpe eller bolt, akkurat som før. Ladelyden (`UpdateWeaponAnimation`) kjører
  fortsatt bare under `IsLocalHuman()` (l. 4596–4611), så verten hører ikke en bot lade som om det var ham selv. ✔
- **Dedikert server:** `CanPlay` og `UGSAudioWorld::Tick` hopper over `NM_DedicatedServer`, og fottrinn og gli har samme
  sjekk. ✔
- **Sen klient:** lyden har ingen tilstand som replikeres. Den bygger på multicaster og `OnRep_Exploded`. En klient som
  kobler seg til innenfor levetiden etter en eksplosjon (0,5 s, eller 1 s for blendgranat), kan høre den, og kan da få
  trykkbølgemiksen. Det er samme vindu som for bildeeffekten fra før og har ingen praktisk betydning. Romklangen måles i
  `Tick`, så den sene klienten får riktig rom etter høyst 0,25 s. ✔
- **Flaggradioen** (`GSHUD.cpp:1011`) bruker nå `PlayRadio`, som spilles per lokal HUD med squelch gjennom køen. ✔

## Fottrinn og radio dempes ikke
- `BlastMix()` legger bare inn Weapon, Impact, Body, Blast, Mech og Ambience. **Step og Radio er ikke med**, så trykkbølgen
  (−12 dB, 600 Hz) rører dem ikke. ✔
- Step og Radio har `bAlwaysPlay` og prioritet 3 og 4. Ingen annen `USoundMix` rører disse klassene. ✔
- Merk: fottrinn **okkluderes** bak vegger (−9 dB og lavpass). Det er tilsiktet realisme ifølge planen og er ikke det samme
  som å dempe.
- Liten svakhet (F2): Radio-klassen deler én samtidighetsgruppe (8, `StopOldest`) med `Hit_Marker`, `Heartbeat`, `Ear_Ring`
  og `UI_*`. Mange treffmarkører pluss øresus kan i verste fall stoppe en flaggmelding som er i gang.

## Ytelse med 20 spillere – regnet ut
Verste tilfelle er 19 andre som skyter automatvåpen (10 skudd/s), sett fra én klient:
| Kilde | Per hendelse | Per sekund |
|---|---|---|
| `Blocked()` (okklusjonssjekk når lyden velges, synkron) | 1 linjespor per skudd > 8 m unna | 19 × 10 = **190** |
| `RoomAt` for skytterens rom (bufres 0,5 s per skytter) | 10 spor (8 vannrette, opp, ned) | 19 × 2 × 10 = **380** |
| `RoomAt` for lytterens rom | 10 spor hvert 0,25 s | **40** |
| Egne skudd (`PlayOwnShot` → `RoomAt`, bufret) | 10 spor per 0,5 s | **20** |
| Fottrinn (`SurfaceBelow`, ett kort spor per steg) | ~2,5 steg/s × 20 | **50** |
| **Sum synkrone spor i spillkoden** | | **ca. 680/s ≈ 11 per ramme ved 60 fps** – billig (korte spor mot statisk verden) |
| Motorens okklusjon (asynkron, `DefaultOcclusionCheckInterval` = 0,1 s, `ActiveSound.cpp:195`) | 10/s per aktiv okkludert 3D-lyd | taket er 74 okkluderte lyder (Weapon 24 + Step 16 + Impact 12 + Body 8 + Blast 8 + Mech 6; haler okkluderes ikke) → **høyst ca. 740/s**, typisk 200–300/s |

Begrensningene er på plass: køen har tak 64, og slutten på miksen kastes aldri. Rombufferen nullstilles over 64 oppføringer,
`Ends` holder høyst 512, og `RoomCache` bruker svake pekere. Ingenting går hver ramme uten grunn: `Tick` tømmer køen og
måler rommet 4 ganger i sekundet. **Ytelsen er godkjent.**

## Lisenser
- `git show 01c0051 --name-status`: 339 nye og 7 endrede WAV-filer. **0 treff på «Sonniss»** i `sources.json` og
  `CREDITS.md`. Den genererte `.gitignore` har ingen filnavn (riktig, siden ingen Sonniss-filer er brukt). ✔
- Kildene i `CREDITS.md` er Kenney Impact (136 filer), Kenney Interface (18) og Snake (167). Hver linje har kilde,
  originalfil, lisens og hva som er gjort med filen. Ingen linje peker på en fil som mangler. ✔
- De 25 nye WAV-filene som ikke står i CREDITS (`Explosion_*` ×9, `Bullet_Crack/Whiz` ×6, `Gear_Rattle` ×4, `Slide_*` ×4, `Radio_Squelch` ×2), er alle syntetiske fra
  `gen_sounds.py round11_sounds()`. CREDITS sier selv at lyder som ikke står i tabellen, er syntetiske. ✔
- `LICENSES/`: `CC0-1.0.txt` og begge Kenney-lisensene er CC0. `Snake_itch_terms.txt` siterer sidens setning om kommersiell
  bruk, har dato og forteller at pakken ikke har noen lisensfil. Sonniss-lisensen mangler, men den trengs ikke i dag. ✔

## Mot planens «Ferdig når»
- Bygg: meldt Succeeded. ✔
- Varianter: alle skuddlag har minst 3 varianter for Pistol, SMG, Rifle, Sniper, Shotgun og LMG (Close/Mid/FP 4–6, Far 3).
  Silenced har bare Close/FP/Mech, men koden bruker bare Close for lyddempede våpen. Fottrinn har 5 per underlag for alle ni
  underlag. Nedslag har 5 per materiale for seks materialer. Det finnes også glass, eksplosjoner og 3 haler for hver av de fem
  romtypene. ✔
- Selvtesten: «ingen samme variant to ganger på rad» stemmer i koden (`Pick`: `I % N + 1`). Køtaket på 64 stemmer. Men
  kravet «maks aktive ≤ 48» kan ikke slå feil (F3).
- Eksplosjoner har lyd (`OnRep_Exploded` for granat, rakett og mine, og `flash` for blendgranat). Kallet i `OnRep_Exploded`
  når både verten (`Explode` kaller den direkte) og klientene. ✔
- Okklusjon, romklang (`ActivateReverbEffect` med `UReverbEffect` laget i C++, loggen ved bytte av rom),
  avstandsforsinkelse (`d/343`, ikke på egne skudd) og `ZoneRT60`-kroken for plan-12 er på plass. ✔ Romklangen er ikke
  lyttet på i editoren ennå.

## Andreas' regel
- **Forbedret:** `Pick` finner variantene selv og trekker aldri samme to ganger på rad. Skudd fra andre kommer i lag
  (nær/middels/fjern med myk overgang) i stedet for én fil. Fottrinn følger underlaget (ni underlag i stedet for to).
  Nedslag høres etter materialet. Lading, bolt og pumpe har lyd per våpentype. Ekte opptak fra Snake og Kenney har erstattet
  de syntetiske lydene.
- **Nytt:** okklusjon, romklang fra målte rom, lydklasser med prioritet og samtidighetstak, trykkbølgemiks med øresus,
  lydforsinkelse med avstanden, eksplosjonslyd, `PlayUI` og `PlayRadio`, utstyrsrasling ved sprint, lyd når man glir, og
  lydverktøyene. ✔

## Funn
**F1 – Våpentaket (24, `StopOldest`) kutter egne skudd og romhaler i store kamper.** `GSAudio.cpp:72`.
Alle `Shot_*`, `Tail_*` og `Bullet_*` deler én global gruppe, også det egne 2D-skuddet (`Shot_X_FP`, `Shot_X_Mech`). Hvert
skudd fra andre gir 2–3 lyder (lag og hale), og hvert eget skudd gir 3. Med 3 andre som skyter automatisk (8/s) og deg selv
(10/s) starter det ca. 85 våpenlyder i sekundet. Med tak 24 lever hver lyd da bare ca. **0,3 s** før den eldste stoppes.
Halene (1–3 s) blir borte, og det egne nærskuddet (0,7 s) kuttes, altså nettopp når romklangen skal høres (IMPROVEMENTS
663/664). Selvtesten bruker 1 skytter og ser ikke dette.
*Retting:* bruk `StopFarthestThenOldest` for Weapon, så 2D-lyder og nære skudd overlever og fjerne skudd og haler går først.
Legg i tillegg halene i en egen gruppe (f.eks. `Tail_` → maks 6, `StopOldest`) og/eller de egne `_FP`/`_Mech` i en egen
gruppe (maks 4). Det er 5–10 linjer.

**F2 – Flaggradioen deler gruppe med treffmarkører og øresus.** `GSAudio.cpp:34` og `:78`. `Hit_Marker`, `Heartbeat`,
`Ear_Ring` og `UI_` ligger i Radio (maks 8, `StopOldest`). Når mange treff og blendgranaten kommer samtidig, kan
flaggmeldingen være den eldste og bli stoppet. Det bryter «flagg alltid høres» i et sjeldent tilfelle.
*Retting:* gi `Flag_` og `Radio_` en egen samtidighetsgruppe, eller bruk `StopQuietest`/`StopLowestPriority` med lavere
prioritet for `Hit_Marker`, `Ear_` og `Heartbeat` enn for `Flag_`.

**F3 – Tallet for «maks aktive lyder» er et anslag som ikke kan slå feil.** `GSAudio.cpp:811–836`. `ActiveEstimate` teller
bare lydene GSAudio selv startet, og kapper hver kategori ved taket sitt. Med bare riflelyder blir det aldri over 24, så
«≤ 48» består alltid. Planen ba om tallet fra `stat audio`. I tillegg pakkes kategori og sluttid inn i én `float`
(`Cat * 1e6 + tid`): for kategori 4–7 er presisjonen 0,5 s, så korte lyder telles feil.
*Retting:* les `FAudioDevice::GetNumActiveSounds()` (eller `GetActiveSounds().Num()`) via `World->GetAudioDevice()` i
selvtesten og i 10-sekunders-loggen, og lagre kategori og sluttid i en `struct` i stedet for i én float.

## Forslag til neste runde
1. F1–F3 over (F1 først). Kjør så selvtesten med **4 skyttere samtidig** (automat, 10/s) og logg det ekte
   `GetNumActiveSounds()` og hvor lenge `Tail_*` faktisk lever.
2. Hopp over sporet i `SurfaceBelow` og `PlayAt` for fottrinn når karakteren er lenger unna lytteren enn rekkevidden
   (18/30 m). Det sparer et spor per steg for alle langt unna og gjør at fottrinn som ikke høres, ikke tar plass under
   Step-taket.
3. Bestilling til Interaktiv: `PlayPartSound` bør bruke `GSAudio::Has(Name)` i stedet for `Get(Name)`, og
   `Interaktiv.Smell` bør bruke `GSAudio::PlayExplosion` i stedet for `Shot_Launcher`, så testsmellet får rom, rusk og
   trykkbølge.

## Lærdom
- Samtidighetstak må regnes ut for hvor mange lyder som starter per sekund med mange spillere, ikke bare for hvor mange som
  spilles samtidig. `StopOldest` på en felles gruppe kutter de lange lydene (haler, egne skudd) først.
- En selvtest som måler noe gjennom vår egen telling med samme tak som den skal kontrollere, kan ikke slå feil. Mål med
  motorens eget tall.
