# Funn fra kontrollørens feiljakt (Spillbarhet)

Nyeste øverst. Planleggeren leser denne og tar de viktigste inn i planene.
Den felles listen `.claude/loop/funn.md` har funnene om loot/pickups, prosjektiler, spawn/kampregler og
våpen/skyting. De gjentas ikke her.

## Områder som er gjennomgått
- 2026-11-26 (commit f19d059), «Feiljakt 2»: **bevegelse** (gange, sprint, hopp, huk, glid, klatring:
  `Move`, `StartJump`, `TryMantle`, `StartSlide`/`EndSlide`, `ToggleCrouch`, sprint/stamina i `Tick`),
  **nærkamp** (`QuickMelee`, `ServerMelee`, `DoMeleeHit`), **gadgets** (gripekrok, fallskjerm, nattsyn,
  kikkert, drone, lommelykt: `UseSelectedItem`, `ServerGrapple`, `CompleteUse`, `TickEffects`, `DrawWallhack`),
  **tastebindinger/input** (`EnsureInputCreated`, `SetupInputComponent`, `SetupPlayerInputComponent`, meny-
  og inventar-bryterne, `DefaultInput.ini`), **til-/frakobling midt i kamp** (`GSGameMode`), og **hva som
  må endres for lagbasert CTF**.
- Ikke gjennomgått ennå: lootplassering per bygg (venter på det mindre kartet), lyd/HUD-detaljer, ytelse.
- 2026-11-26 (commit 55ca90f), «Feiljakt 1»: **PC-er, kameraer og 3D-printere** (`GSTerminal`, terminaldelen
  av `GSPlayerController`, `SpawnTerminals` i `GSGameMode`, terminalskjermene i `GSHUD`), **overlevelse**
  (sult, tørst, mat, medisin, boosts og bivirkninger: `TickSurvival`, `CompleteUse`, `AddEffect`,
  `TickEffects` i `GSCharacter`, data i `catalog.py`), **inventar/sekk/vekt** (`AddToBackpack`,
  `TakeFromPickup`, `GiveWeapon`, `DropEverything`), **lobby, klar-system og poengtavle** (`GSGameMode`,
  `GSGameState`, `GSPlayerState`, `DrawMatch`/`DrawScoreboard` i `GSHUD`), **meny og konsollkommandoer**
  (Host, Join, Grafikk, Opplosning, Straaling).

## Funn

Alvorlighet: **Høy** = ødelegger kamper, gjør en hel funksjon ubrukelig eller kan utnyttes lett.
**Middels** = merkes tydelig. **Lav** = pirk som tas når man likevel er i filen.
Linjenumrene står ved hver feiljakt. Alt er funnet ved å lese koden, ikke ved å spille.
«Eier» sier hvilket lag som eier filen (se `.claude/lag/LAG.md`). Funn i andre lags filer må gå som
bestilling i `.claude/lag/bestillinger.md`.

### Feiljakt 2 (2026-11-26, commit f19d059)

Linjenumre gjelder f19d059. Nesten alt her ligger i `GSCharacter.cpp` og `GSPlayerController.cpp`, som
lag Kamp eier: lag Spillbarhet må bestille rettelsene (`.claude/lag/bestillinger.md`).

#### A. Kontroller og input

I1. **Musehjulet blar aldri i inventaret – det er bare koblet til «neste våpen»** – Høy (Andreas har meldt den)
   - Fil: `GSPlayerController.cpp` linje 105–106 (`MapKey(NextWeaponAction, MouseScrollDown)` og
     `MapKey(NextWeaponAction, MouseScrollUp)`), linje 101 (`NextItemAction` = bare Q), `GSPlayerController.h`
     linje 134 (`ToggleInventory` snur bare `bShowInventory`); `GSCharacter.cpp` linje 345 (hjulet kaller
     `NextWeapon`), `NextWeapon` linje 540–545, `NextItem` linje 2165–2170. Eier: Kamp → bestilling.
   - Årsak: begge hjulretningene er bundet til samme handling, `NextWeaponAction`, som bytter våpen.
     Ingenting kobler hjulet til valgt gjenstand i sekken (`SelectedItem`); det endres bare av Q. At
     inventaret er åpent (I) endrer ikke hva hjulet gjør. Og `NextWeapon` gjør ingenting når man har færre
     enn 2 våpen (`if (Weapons.Num() < 2) return;`) – alle starter med bare én pistol (`BeginPlay`
     linje 296–301), så hjulet gjør **helt ingenting** til man har plukket opp et våpen til. Hjul opp og
     hjul ned går dessuten begge «fremover»; det finnes ingen «forrige» (verken våpen eller gjenstand).
   - Forslag: én akse-handling på `EKeys::MouseWheelAxis` (eller to handlinger, opp/ned) med retning.
     Er inventaret åpent → `SelectedItem` ±1; ellers våpen ±1. Legg også til «forrige gjenstand», og vis
     «hjul: bla» i inventarboksen og hurtiglinja (`GSHUD.cpp` linje 359 og 1024 sier bare «Q neste»).

I2. **Taster kan ikke endres: alt er hardkodet og spilleren-innstillinger er ikke slått på** – Middels (blokkerer keybinds-oppgaven)
   - Fil: `GSPlayerController.cpp` `EnsureInputCreated` linje 34–112 (handlinger og `MapKey` laget med
     `NewObject` i kode, uten `PlayerMappableKeySettings`), `SetupInputComponent` linje 123 (`ClearAllMappings`
     hver gang); `Config/DefaultInput.ini` (ingen `[/Script/EnhancedInput.EnhancedInputDeveloperSettings]`
     med `bEnableUserSettings=True`). Eier: Kamp (filen) → bestilling; Config er felles.
   - Hva: Enhanced Input sine brukerinnstillinger (som prioriteringene ber om) krever at handlingene har
     et navn for omkobling, at konteksten registreres i `UEnhancedInputUserSettings`, og at brukerinnstillinger
     er slått på. Ingenting av dette finnes. `ClearAllMappings` fjerner dessuten alle andre kontekster.
   - Forslag: gi hver handling `PlayerMappableKeySettings` (navn + norsk visningsnavn), slå på
     `bEnableUserSettings`, registrer konteksten, og la knappehjelpen lese tastene fra innstillingene i
     stedet for faste tekster.

I3. **Med menyen (Esc) åpen kan man fortsatt hoppe, lade, bytte våpen, slå, bruke og slippe ting** – Lav
   - Fil: `GSPlayerController.cpp` `ToggleMenu` linje 138–151 (stopper bare se og gå); `GSCharacter.cpp`
     bare `StartFire` (linje 634) og `Look` (linje 375) sjekker `bMenuOpen`. Eier: Kamp → bestilling.
   - Hva: G med menyen åpen slipper valgt gjenstand, V slår, R lader, Space hopper. Lett å gjøre ved et
     uhell når man skriver eller klikker i menyen (blir verre når hovedmenyen får tekstfelt).
   - Forslag: legg til en egen meny-kontekst med høyere prioritet som «spiser» spilltastene, eller sjekk
     `bMenuOpen` i én felles vakt.

#### B. Bevegelse (glid og klatring – Andreas liker begge)

M1. **Etter en glid står du fast i huk: Space hopper ikke, og sprinten må trykkes på nytt** – Middels
   - Fil: `GSCharacter.cpp` `StartSlide` linje 436–452 (`Crouch()`), `EndSlide` linje 459–468 (ingen
     `UnCrouch`), `StartJump` linje 383–389, `StartSprint` linje 471–475 (`!bIsCrouched`). Eier: Kamp → bestilling.
   - Hva: glidningen legger figuren i huk og avslutter uten å reise seg. Motorens `Jump()` virker ikke i
     huk (`CanJump` krever at man ikke er i huk), så Space under eller etter glid gjør ingenting med mindre
     det er en kant å klatre på (`EndSlide` kalles, men figuren er fortsatt i huk). Sprinten er slått av
     og startes bare når Shift trykkes ned på nytt, og nekter så lenge man er i huk. Resultat: sprint →
     glid → man kryper i 2 m/s til man trykker C og så Shift igjen. Glidningen koster heller ingen stamina.
   - Forslag: Space under/etter glid = reis deg og hopp (`UnCrouch` + hopp, kort «glidehopp»); holder du
     fortsatt Shift når glidningen slutter, reis deg og sprint videre; ellers bli i huk. La glidningen
     koste litt stamina.

M2. **Glid og sprint endres utenfor bevegelseskomponentens forutsigelse (rykk på nett)** – Middels
   - Fil: `GSCharacter.cpp` `StartSlide` linje 445–451 (endrer `GroundFriction`, `BrakingDecelerationWalking`,
     `MaxWalkSpeedCrouched` og legger direkte til `Velocity`, på klienten nå og på serveren når RPC-en kommer),
     `ServerSlide` linje 454–457 (serveren kjører hele `StartSlide` på nytt med egne vilkår), `EndSlide`
     via egen timer på hver maskin; `Tick` linje 3229–3231 (`MaxWalkSpeed` fra `bSprinting` som serveren
     får via RPC). Eier: Kamp → bestilling.
   - Hva: Unreal sin `CharacterMovementComponent` spiller av klientens bevegelser på serveren og retter
     klienten når resultatet avviker. Glid-verdiene settes på ulike tidspunkt på klient og server, og
     serveren kan nekte glidningen (f.eks. `IsFalling()` eller 1,2 s-sperren slår ut annerledes), mens
     klienten glir. Da blir spilleren dratt tilbake («rubber-banding»), mer jo høyere ping. `StartJump` på
     klienten avslutter glidningen bare lokalt (ingen RPC), så serveren glir videre med lav friksjon.
   - Forslag: glid og sprint som egne flagg i en egen `UGSCharacterMovementComponent` (`FSavedMove` med
     `CompressedFlags`, fart/friksjon i `GetMaxSpeed`/`CalcVelocity`), slik Lyra og ALS gjør. Det er også
     grunnmuren for tyngre, mer realistisk glid som Andreas ønsker.

M3. **Klatringen stoler på klienten: serveren godtar et «hopp» opptil 12 m/s i hvilken som helst retning** – Middels (juks)
   - Fil: `GSCharacter.cpp` `ServerMantle_Implementation` linje 430–434 (sjekker bare `bDead` og
     `LaunchVelocity.Size() > 1200`). Eier: Kamp → bestilling.
   - Hva: serveren sjekker ikke at det finnes en kant foran spilleren. En modifisert klient kan sende
     `ServerMantle` når som helst, også i lufta, og løfte seg ca. 7 m (1200² / 2g) om og om igjen: opp på
     tak, over murer, rett til flagget.
   - Forslag: serveren gjør samme kantsjekk som `TryMantle` og regner ut farten selv (klienten sender
     bare «jeg vil klatre»), med en kort sperre mellom klatringer.

M4. **Klatring kan gjøres i lufta og har verken sperre, stamina eller ekte klatrebevegelse** – Lav (men viktig for Andreas)
   - Fil: `GSCharacter.cpp` `StartJump` linje 383–389 og `TryMantle` linje 391–428 (ingen `IsFalling`-sjekk,
     ingen stamina, ingen ventetid), `LaunchCharacter` linje 424. Eier: Kamp → bestilling.
   - Hva: Space i lufta ved en kant gir et nytt «hopp» opp på kanten, så man når ca. 3,1 m (hopp 0,94 m +
     kant 2,2 m). Klatringen er et kast i en bue (`LaunchCharacter` med 2,6 m/s fremover), ikke et grep og
     en opptrekking; man kan skli av eller skyte over kanten. Den koster ingenting.
   - Forslag: ekte klatring i trinn (grep kanten, trekk deg opp på 0,6–1,2 s med kamerabevegelse, styrt
     posisjon i stedet for kast), med stamina-kost, slik prioriteringene beskriver (vinduer, tak, høye murer).

M5. **Ingen fallskade: man kan hoppe fra taket på Sentralbygg 1 uten en skramme** – Middels (realisme, gjør fallskjermen meningsløs)
   - Fil: ingen kode håndterer landing (ingen `Landed`/fallskade i `Source/`). Eier: Kamp → bestilling.
   - Hva: fall fra hvilken som helst høyde, og gripekrok-kast i opptil 26 m/s, gir null skade. Fallskjermen
     (som er laget for å «hoppe fra Sentralbygg 1») har derfor ingen nytte.
   - Forslag: fallskade fra ca. 3–4 m, dødelig fra ca. 12–15 m, med halting (bein) som kroppssone-systemet
     kan bruke; lyd og kameradump ved hard landing.

#### C. Nærkamp og gadgets

G1. **Klienters granater, miner og nærkampslag ser alltid rett frem i vannrett – serveren vet ikke hvor de ser opp/ned** – Høy
   - Fil: `GSCharacter.cpp` `ThrowItem` linje 2398–2404 og `DoMeleeHit` linje 888–896 (bruker
     `Camera->GetForwardVector()` på serveren), `Camera->bUsePawnControlRotation` linje 127. Eier: Kamp → bestilling.
   - Hva: kameraets rotasjon oppdateres av motoren bare for den lokale spilleren (i `GetCameraView`). På
     serveren har en tilkoblet spillers kamera samme retning som kroppen: riktig side-til-side, men alltid
     vannrett. Granater kastes derfor alltid flatt (+2,5 m/s opp) uansett om man sikter opp på en balkong
     eller ned en trapp, og et slag treffer bare i øyehøyde rett frem (en som ligger eller sitter i huk
     foran deg treffes ikke). Verten (listen server) merker ingenting, så feilen synes ikke når man
     tester alene.
   - Forslag: bruk `GetBaseAimRotation()`/`GetControlRotation()` på serveren (eller send retningen med
     RPC-en og sjekk den), slik `ServerFire` allerede gjør.

G2. **Fallskjermen må brukes på bakken og forsvinner etter 2 sekunder** – Middels
   - Fil: `GSCharacter.cpp` `CompleteUse` linje 2303 («Fallskjerm klar. Hopp!»), `TickEffects` linje
     2491–2495 (fjernes når man ikke faller og det er gått mer enn 2 s). Eier: Kamp → bestilling.
   - Hva: bruker du den mens du står på taket, har du 2 s på å hoppe før den er borte (og gjenstanden er
     brukt opp). Bruker du den i lufta, tar den 0,5 s (bruketid i `catalog.py` linje 298) før den virker.
     Mens den er aktiv på bakken gir vanlige hopp svev (0,9 luftkontroll).
   - Forslag: fallskjermen bæres som utstyr og utløses med Space i fritt fall (over en viss fallhøyde),
     slik ekte fallskjermer brukes. Med fallskade (M5) får den en ekte jobb.

G3. **Speiderdronen er ingen drone: den gir deg veggsyn på alle innen 40 m rundt deg selv** – Middels (realisme, CTF)
   - Fil: `GSCharacter.cpp` `CompleteUse` linje 2302 (`AddEffect(FxDrone)`), `catalog.py` linje 52
     (`Avslort` = `wallhack`) og 296; `GSHUD.cpp` `DrawWallhack` linje 431–445 (røde rammer på alle, også
     gjennom vegger). Eier: Spillbarhet (catalog) og Kamp (kode).
   - Hva: ingen drone flyr, ingen kan se eller skyte den ned, og den «ser» gjennom vegger fra der du står.
     I CTF blir den en 10-sekunders veggsynsjuks rundt flagget.
   - Forslag: en ekte drone-aktør man styrer (eller som flyr opp over deg), som bare ser det den har
     siktlinje til, lager lyd og kan skytes ned. Til den finnes: ta den ut av loot og printer.

G4. **Lommelykten lyser gjennom vegger, og andre ser den alltid peke vannrett** – Lav/Middels
   - Fil: `GSCharacter.cpp` linje 236–244 (`Flashlight->SetCastShadows(false)`, festet til `Camera`).
     Eier: Kamp → bestilling.
   - Hva: et lys uten skygger stopper ikke ved vegger: det lyser opp rommet bak veggen og avslører deg der.
     Og fordi kameraet på andre maskiner ikke følger blikket opp/ned (se G1), peker strålen andre ser
     alltid rett frem.
   - Forslag: skygger på (bare dynamiske, kort rekkevidde), og la lykten følge `GetBaseAimRotation()`.

G5. **Gripekroken: serveren godtar ethvert punkt innen 37 m, også gjennom vegger** – Lav (juks)
   - Fil: `GSCharacter.cpp` `ServerGrapple_Implementation` linje 2215–2229 (bare avstand og at du har kroken).
     Eier: Kamp → bestilling.
   - Forslag: linjespor fra øyet til punktet på serveren. I CTF: ingen gripekrok mens man bærer flagget.

G6. **Kikkerten sier «Hold F», men F slår av og på** – Lav
   - Fil: `catalog.py` linje 294 (Eier: Spillbarhet); `GSCharacter.cpp` `UseSelectedItem` (bryter).
   - Forslag: rett teksten til «Slå av og på med F» (kan gjøres av Spillbarhet alene).

#### D. Til- og frakobling midt i en kamp

C1. **Den som kobler fra midt i en skuddveksling forsvinner: ingen død, ingen drap, ingen loot** – Middels (blir Høy med flagg)
   - Fil: `GSGameMode.h`/`.cpp` (ingen `Logout`-overstyring; søk etter `Logout`/`PawnLeavingGame`/`EndPlay`
     i `Source/` gir ingen treff). Eier: Spillbarhet.
   - Hva: når en spiller lukker spillet, ødelegger motoren figuren uten å kalle `Die`. Skytteren får
     ikke drapet, ingenting slippes (`DropEverything` kjøres ikke), og killfeed sier ingenting. Å koble fra
     når man taper er en gratis flukt. I CTF forsvinner et båret flagg sammen med figuren og kommer aldri
     tilbake.
   - Forslag: overstyr `Logout` i `GSGameMode`: slipp flagget (eller send det hjem), slipp utstyret, gi
     drapet til siste som skadet (finnes som `LastDamageInstigator`), og skriv «X forlot kampen» i killfeed.

C2. **Den som kommer inn midt i en kamp settes rett inn uten lag og uten beskjed** – Middels (CTF)
   - Fil: `GSGameMode.cpp` (ingen `PostLogin`/`HandleStartingNewPlayer`; motorens standard kaller
     `RestartPlayer` straks), `PickSpawnTransform` linje 229–257. Eier: Spillbarhet.
   - Hva: i dag blir han en fiende til alle. I CTF må han få et lag (det med færrest), spawne ved lagets
     base, og få en melding (se F3 i Feiljakt 1).

#### E. Det som må endres for lagbasert Capture the Flag (dagens kode er «alle mot alle»)

T1. **Det finnes ingen lag i koden** – Høy (grunnmur)
   - Fil: `GSPlayerState.h` (bare `Kills`, `Deaths`, `bReady`), `GSGameState.h`, `GSGameMode.h` («Free roam
     deathmatch»). Søk etter `Team` i `Source/` gir ingen treff. Eier: Spillbarhet (`GSTypes.h` felles).
   - Forslag: `ETeam` i `GSTypes.h` (nederst), replikert `Team` i `GSPlayerState`, lagpoeng og erobringer i
     `GSGameState`, lagvalg og utjevning i lobbyen.

T2. **Venneild: alt skader alle, og lagkameratdrap gir poeng** – Høy
   - Fil: `GSCharacter.cpp` `TakeDamage` linje 2520–2526 (sjekker bare `DamageEnabled()`), treffmarkør og
     blodtørst linje 2637–2661 (gis for alle treff); `GSGameMode.cpp` `OnPlayerKilled` linje 265–275
     (`Kills++` for ethvert drap som ikke er selvmord). Eier: Kamp (`TakeDamage`) → bestilling; Spillbarhet
     (`OnPlayerKilled`, regelen «venneild av/på» i `GSGameMode`/`GSGameState`).
   - Forslag: `GSGameMode::CanDamage(Instigator, Victim)` (Spillbarhet) som `TakeDamage` spør; lagdrap gir
     −1 og egen tekst i killfeed. Husk granater, miner, brann og blitz (`ApplyFlash`), ikke bare kuler.

T3. **Spawn velger punktet lengst unna alle levende – også lagkamerater – blant 16 punkter over hele det gamle kartet** – Høy (CTF)
   - Fil: `GSGameMode.cpp` `PickSpawnTransform` linje 229–257, `GSCampus.h` `SpawnPoints` linje 82–91 (Eier:
     Grafikk og kart). Eier: Spillbarhet.
   - Hva: punktene ligger fra x −300 til +420 m og y −80 til +520 m, altså langt utenfor et mindre kart, og
     tilfeldig retning (`Rot` med tilfeldig yaw). Kampstart (`StartMatch` linje 108–116) sprer alle på samme måte.
   - Forslag: spawnsoner per lag ved egen base (bestill punktene fra Grafikk og kart sammen med basene),
     med retning mot midten og kort spawnbeskyttelse.

T4. **Poeng og kampslutt er drapsbasert** – Høy (CTF)
   - Fil: `GSGameMode.cpp` `Tick` linje 68–76 (`KillLimit` 25), `EndMatch` linje 119–131 (vinner = flest drap,
     lagret som spillernavn), `GSGameState.h` (`KillLimit`, `Winner` som `FString`). Eier: Spillbarhet.
   - Forslag: 3 erobringer eller flest når tiden går ut, uavgjort håndtert, `WinningTeam` i stedet for et
     navn. Poengtavla delt i to lag med lagfarger (`GSHUD`, bestilling).

T5. **Ingen kan se hvem som er på hvilket lag** – Høy (CTF)
   - Fil: `GSCharacter.cpp` `BeginPlay` linje 289–292 (kroppsfarge fra `GetName()` på figuren, altså ny
     tilfeldig farge for hver respawn); `GSHUD.cpp` `DrawWallhack` (alle røde), killfeed og poengtavle uten
     lag. Eier: Kamp → bestilling.
   - Forslag: lagfarge på kropp/armbind (replikert fra `PlayerState`), navn over lagkamerater på kort hold.

T6. **Boosts og gadgets gjør flaggløp trivielle** – Middels (CTF-balanse)
   - Fil: `GSCharacter.cpp` `Tick` linje 3231 (`MaxWalkSpeed = Speed * Mods.Speed * GearSpeed`, ingen
     plass for «bærer flagget»), `Spøkelse` (nesten usynlig, linje 3279 og utover), gripekrok 35 m, fallskjerm;
     se også B3 i Feiljakt 1 (fart over 2×). Eier: Kamp (kode), Spillbarhet (data).
   - Forslag: flaggbæreren får en egen fartsfaktor (Andreas: «går litt tregere») som ikke kan boostes over
     vanlig gange, kan ikke bruke gripekrok/fallskjerm, og Spøkelse/usynlighet fjernes når flagget tas.

T7. **PC-sporing og drone viser lagkamerater og fiender likt** – Lav (CTF)
   - Fil: `GSPlayerController.cpp` `SendTrackPulse` linje 470–485, `GSHUD.cpp` `DrawWallhack`. Eier: Kamp → bestilling.
   - Forslag: vis lag med farge, eller bare fiender.

### Feiljakt 1 (2026-11-26, commit 55ca90f)

#### A. Ekte feil (flyt som stopper opp, juks, feil poeng)

S1. **Kampen starter 10 sekunder etter at første venn kobler til** – Høy (flyt)
   - Fil: `GSGameMode.h` linje 34 (`LobbyStartedAt = 0.f`), `GSGameMode.cpp` linje 62 og 137. Eier: Spillbarhet.
   - Hva: 90-sekundersklokken for automatisk start regnes fra serveren ble startet, ikke fra noen kom inn.
     Den settes bare på nytt i `BackToLobby`. Har verten ventet mer enn 90 s på vennene sine (det har han
     nesten alltid), starter nedtellingen i samme øyeblikk som spiller nr. 2 kobler til. Resten av
     vennene kommer inn midt i kampen. HUD-en viser heller ikke at det finnes en slik klokke.
   - Forslag: start klokken når antall spillere går fra 1 til 2 (og start den på nytt hver gang en ny
     spiller kommer inn), repliker tidspunktet og vis «Starter automatisk om N sek» i lobbyen.

S2. **Én spiller alene kan starte en 15 minutters kamp som ikke kan avbrytes** – Middels (flyt)
   - Fil: `GSGameMode.cpp` linje 61 (`Total > 0 && Ready == Total`), linje 68–77. Eier: Spillbarhet.
   - Hva: trykker verten Enter mens han er alene, starter kampen. Drapsgrensen kan ikke nås alene, og
     verten har ingen måte å avslutte eller starte på nytt. Venner som kommer inn må spille ut resten av
     de 15 minuttene, eller verten må starte serveren på nytt (alle kastes ut).
   - Forslag: krev minst 2 spillere for vanlig start (alene = tydelig «øvingsrunde»), og gi verten en
     «Start på nytt / tilbake til lobby»-knapp i menyen.

S3. **Brannammo og andre «tappe»-effekter gir ikke drap til skytteren, og dødsårsaken blir «Overdose»** – Middels
   - Fil: `GSCharacter.cpp` linje 2541 (`AddEffect("Brennende")`), `TickSurvival` linje 2922–2937. Eier: Kamp.
   - Hva: «Brennende» (6 HP/s i 3 s) legges på som en vanlig effekt uten å huske hvem som skjøt. Dør
     offeret av brannen, kalles `Die(nullptr, false, "Overdose")`. Skytteren får ikke drapet, killfeed
     sier «Verden», og offerets dødskort sier «Overdose». Samme tekst brukes hvis man dør av
     matforgiftning fra kald pizza.
   - Forslag: lagre hvem som startet effekten og årsaken (`LastDamageInstigator`/`LastDamageWeapon`
     finnes alt) og bruk dem i `TickSurvival`. Egne tekster: «Brann», «Matforgiftning», «Overdose».

S4. **Man kan dø i lobbyen selv om det står «ingen skade»** – Lav/Middels
   - Fil: `GSCharacter.cpp` `Tick` linje 3096–3098 og `TickSurvival` linje 2915–2938. Eier: Kamp.
   - Hva: sult, tørst og effekter som tapper helse (Berserk, matforgiftning) går rett på `Health` og
     sjekker ikke `DamageEnabled()`. Sult og tørst tikker også i lobby, nedtelling og resultatskjerm.
     En spiller som står i lobbyen lenge, eller tar Berserk der, dør og får «Død» på tavla.
   - Forslag: hopp over `TickSurvival` (eller i det minste helsetapet) når kampen ikke er i gang.

S5. **Sporingen fra PC-en fortsetter etter at du har gått, og serveren vet aldri at noen «sitter ved PC-en»** – Middels (juks)
   - Fil: `GSPlayerController.cpp` linje 455–485 (`ServerTrack`, `SendTrackPulse`), 347–354 (`OpenTerminal`,
     bare lokalt), 443–447 (`ServerCanUse`). Eier: Kamp (filen) → bestilling.
   - Hva: (1) `SendTrackPulse` sjekker ikke at spilleren fortsatt står ved PC-en eller lever. Etter ett
     tastetrykk sender serveren alle spilleres navn og posisjon hvert 2. sekund i 20 s uansett hvor du
     løper. Vanlig HUD skjuler kartet når du går, men dataene ligger hos klienten (en modifisert klient
     har radar). (2) Å «sitte ved PC-en» finnes bare på klienten: det er klienten selv som slutter å ta
     imot bevegelse og skyting (`TerminalPC()`-sjekkene). Serveren krever bare at du er innen 3,5 m. En
     juksende klient kan spore, se kameraer og printe mens den løper og skyter. (3) `ServerViewCamera`
     (linje 449–453) flytter serverens synspunkt til kameraet og flytter det aldri tilbake av seg selv.
   - Forslag: repliker «bruker terminal» som tilstand på serveren (`ServerOpenTerminal`/`ServerCloseTerminal`),
     la serveren stoppe bevegelse/skyting mens den er satt, stopp sporing og kameravisning på serveren
     når spilleren går, dør eller tar skade, og la andre spillere se at du står bøyd over PC-en.

S6. **Ammokassen (vanlig) gir raketter, 40 mm-granater og .50 BMG til alle våpen på én gang** – Middels (balanse)
   - Fil: `GSCharacter.cpp` `CompleteUse` linje 2280–2288 (Eier: Kamp); `catalog.py` linje 304 (`AmmoBox`,
     Common. Eier: Spillbarhet).
   - Hva: én «Ammokasse» gir `max(2 magasiner, 3)` skudd til hvert våpen du bærer uansett kaliber: 3
     raketter til Carl Gustaf (mytisk våpen), 3 granater til M79, 20 skudd .50 BMG. Den riktige esken
     «84 mm raketter» er uvanlig og gir 1 rakett. Kalibersystemet blir meningsløst.
   - Forslag: fjern `AmmoBox` fra katalogen (kan gjøres av Spillbarhet alene), eller la den bare fylle
     pistol- og geværkalibre.

S7. **PC og printer kan brukes gjennom vegg og gulv** – Lav
   - Fil: `GSCharacter.cpp` `UpdateFocusPickup` linje 2041–2054 (2,4 m fra øyet, ingen siktlinje),
     `GSPlayerController.cpp` `ServerCanUse` linje 443–447 (3,5 m, ingen siktlinje). Eier: Kamp → bestilling.
   - Hva: samme hull som funn 8 i felleslisten (loot gjennom vegger). I bygg med 4 m etasjehøyde ligger
     printeren i 2. etasje ca. 2,3–2,4 m fra øyet til en som står rett under og ser opp.
   - Forslag: ett linjespor fra øyet til terminalen på både klient og server.

S8. **Printet gripekrok har 1 lading, den fra loot har 3** – Lav
   - Fil: `GSTerminal.cpp` linje 173 (`SpawnDropped(..., PrintingId, 1)`), `GSItems.cpp` `PickupCount` linje 272.
     Eier: Spillbarhet.
   - Forslag: bruk `Def->Charges` for gripekrok også når printeren lager den.

S9. **Poengtavla sorterer bare på drap, vinneren kåres på drap og færrest dødsfall** – Lav
   - Fil: `GSHUD.cpp` linje 912, `GSGameMode.cpp` linje 126. Eier: Kamp (HUD) → bestilling.
   - Hva: ved likt antall drap kan vinneren stå som nr. 2 eller 3 på tavla. Overskriftene blander også
     språk («Kills» og «Død»).

S10. **Sluttskjermen: poengtavla tegnes oppå «KAMPEN ER OVER» og vinnernavnet** – Middels
   - Fil: `GSHUD.cpp` `DrawMatch` linje 797–805 og `DrawScoreboard` linje 914–934. Eier: Kamp → bestilling.
   - Hva: banneret ligger på 30 % av skjermhøyden (130 px høyt). Poengtavla starter på 20 %, er
     70 + 26 px per spiller høy, og tegnes etterpå. På 1080p (banner fra 324 px, rader fra 280 px)
     skrives radene fra spiller nr. 3 og utover rett over «KAMPEN ER OVER», vinneren og nedtellingen.
   - Forslag: flytt banneret over tavla (for eksempel 8 % av høyden) eller skriv vinneren inni tavla.

#### B. Systemer som i praksis ikke virker (balanse)

B1. **Crafting er nesten umulig: det finnes ca. 2 filament-spoler på hele campus** – Høy
   - Fil: `GSItems.cpp` `RandomLoot` linje 215–219 (Utstyr = 8 av 100), `catalog.py` linje 423 (Filament, Common),
     `GSTerminal.cpp` linje 70–84 (oppskrifter 1–8 spoler). Eier: Spillbarhet.
   - Hva: regnet ut fra vektene gir en lootplass filament 0,53 % av gangene (utstyr 8 % × vanlig 40 % ×
     én av 6 vanlige utstyrsting). Med ca. 428 lootplasser er det i snitt 2,3 spoler på hele campus til
     20 spillere (én spole per funn). Billigste nyttige ting (printet pistol, panserplate, gripekrok)
     koster 3. Speiderdronen koster 8, altså mer enn det som finnes på kartet. 3D-printerne (ca. 10
     stykker) er i praksis pynt.
   - Forslag: faste funnsteder for filament ved hver printer og i verksteder (2–4 spoler), og/eller egen
     lootgruppe «materialer». Se også R3.

B2. **Sult og tørst betyr ingenting i en kamp, men mat er 20 % av all loot** – Høy (dødt system)
   - Fil: `GSCharacter.cpp` `TickSurvival` linje 2919–2920 (25 min / 18 min fra full til tom), `Tick` linje 3112
     (halv stamina-henting først under 20), `GSGameState.h` (`MatchMinutes = 15`), `GSItems.cpp` linje 219
     (Mat = 20). Eier: Kamp (kode), Spillbarhet (regler og data).
   - Hva: alle starter på 100 og får 100 igjen ved hver respawn. Kampen varer 15 minutter. Ingen
     virkning kommer før under 20, det vil si etter ca. 14 min (tørst) uten å dø og uten å drikke. Det
     skjer nesten aldri. Mat brukes derfor bare som små helsepakker. Hver femte lootplass er likevel
     mat, mens det bare er ca. 2,4 vannflasker på kartet.
   - Forslag: velg én vei og gjør den ferdig. Enten: start på 60–70, behold nivået gjennom respawn, og
     gi gradvise, realistiske følger fra ca. 50 (tregere stamina, mer våpensvai). Eller: kutt mat i
     loot til 5–8 % til kampene blir lengre.

B3. **Boosts kan stables uten tak: over dobbel fart og 70 % mindre skade** – Høy (balanse)
   - Fil: `GSItems.h` `FGSEffectMods::Combine` linje 27–35 (Eier: Spillbarhet), `GSCharacter.cpp` linje 3119
     (`Speed * Mods.Speed`), 2533 (`Damage *= Mods.Taken`) og `AddEffect` linje 2436–2449 (Eier: Kamp).
   - Hva: alle effekter ganges sammen uten grense. Turbo × Adrenalin × Lynet × Berserk × Lattergass ×
     Koffein+ = 2,19 ganger fart. Grønn × Adrenalin × Morfin × Panser × Drøm × Julestemning = 29 % skade
     (i tillegg til vest). Regenerator + Feltkirurgi + Smertestillende + Grønn + Tran = 6,7 HP/s. Det
     finnes ingen overdose: ti ulike stoffer samtidig koster ingenting utover hver sin lille bivirkning.
   - Forslag: tak i `Combine` (fart maks +15 %, skade tatt minst 70 %; kan gjøres av Spillbarhet alene),
     og en «belastning» som øker for hvert stoff i kroppen: over en grense skjelving, tåkesyn, oppkast,
     og til slutt ekte overdose.

B4. **Sporing på PC er gratis veggsyn for alle, hele tiden** – Middels
   - Fil: `GSPlayerController.cpp` linje 344–345 og 455–485, `GSHUD.cpp` linje 1156 og 1161–1188. Eier: Kamp → bestilling.
   - Hva: hvilken som helst PC (én i hvert bygg, 16–17 stykker) viser navn og nøyaktig posisjon på alle
     levende spillere på hele campus i 20 s, med 45 s ventetid per spiller (ikke per PC). Ingen kan
     gjemme seg, og den som spores får ikke vite det.
   - Forslag: se R4 (vis bare det kameraene faktisk ser).

#### C. Realismehull (mot målet «superrealistisk», se `andreas.md`)

R1. **Sår leges på sekunder, og mat gir helse** – Høy
   - Fil: `GSCharacter.cpp` `CompleteUse` linje 2252–2269 (Eier: Kamp); `catalog.py` linje 239–243 og
     matlinjene 251–275 (Eier: Spillbarhet).
   - Hva: plaster gir +15 HP på 2 s, førstehjelpsskrin +75 på 6 s, «feltkirurgi» på seg selv +100 på 10 s,
     adrenalinsprøyte +35 på 1 s. Røkelaks gir +25 HP, kebab +15. All helse kommer i ett hopp når tiden
     er ute. Det finnes ingen blødning. Bruken avbrytes ikke av at man blir truffet.
   - Virkelighet: et skuddsår leges ikke i felt. Det man kan gjøre er å stoppe blødning (trykkbandasje,
     tourniquet: 20–60 s), dempe smerte (virker etter minutter) og holde seg i live. Mat leger ikke.
   - Forslag: blødning som egen tilstand etter treff (helse renner ut til den stoppes). Bandasje og
     tourniquet stopper blødning; smertestillende fjerner skjelving/halting en stund, ikke skade;
     førstehjelpsskrin gir langsom bedring over 1–2 min; mat gir bare sult/tørst/stamina. Treff
     avbryter behandlingen. Dataene (`heal=0` på mat) kan Spillbarhet rette alene.

R2. **Overnaturlige boosts** – Høy
   - Fil: `catalog.py` linje 18–58 og 211–231 (Eier: Spillbarhet); brukt i `GSCharacter.cpp` linje 2585–2603
     og 3168–3177 (Eier: Kamp).
   - Hva: Føniks og hjertestarter (stå opp fra de døde; hjertestarteren «lades» på deg selv på forhånd),
     Spøkelse (nesten usynlig), Øyet (ser alle gjennom vegger), Blodtørst (helse av skaden du gjør),
     Regenerator (3 HP/s), Pansertablett (25 % mindre skade av en pille), Lynet (50 % høyere hopp),
     Berserk (+30 % kuleskade) og Bulk (+12 % kuleskade). Et stoff kan ikke gjøre en kule sterkere.
   - Virkelighet: stoffer endrer kroppen, ikke våpenet: puls, skjelving, tunnelsyn, smerte,
     utholdenhet, reaksjon. Koffein, nikotin, adrenalin, morfin, amfetamin og alkohol har kjente
     virkninger og bivirkninger som kan brukes direkte.
   - Forslag: ta de overnaturlige ut av loot (eller bygg dem om: hjertestarter = gjenopplive en spiller
     som nettopp falt), og la resten virke på svai, stamina, smerte og syn. `damage`-modifikatoren bør
     bare gjelde nærkamp.

R3. **3D-printeren lager stål, optikk, sprengstoff og elektronikk av plast på sekunder** – Høy
   - Fil: `GSTerminal.cpp` linje 70–84. Eier: Spillbarhet.
   - Hva: 6–60 sekunder og 1–8 hele spoler for alt. Listen har lyddemper, rødpunktsikte, panserplate,
     snubletrådmine (130 skade), dirkesett, kompass og en ferdig speiderdrone. Én spole er i
     virkeligheten 1 kg plast; en kniv bruker 30–50 g, en hjelm 300–500 g.
   - Virkelighet: en vanlig printer lager plastdeler: 1–3 timer for en kniv, 15–40 timer for en hjelm
     eller en pistolramme. Den kan ikke lage linser, batterier, motorer, stålplater eller sprengstoff.
     En printet pistol («Liberator») trenger i tillegg en spiker som tennstift og tåler få skudd.
   - Forslag: (1) filament i gram, ikke hele spoler. (2) Printeren lager plastdelen; resten er deler man
     må finne (elektronikk i Elektrobygget, kjemikalier i kjemiblokkene, stålrør/spiker i verksteder).
     (3) Spilltid som står i forhold til størrelsen: 1 min (kniv) til 5–6 min (hjelm, pistol), så
     printeren blir et sted man må vokte. (4) Ta ut det plast ikke kan bli: mine, panserplate, sikte, dirker.

R4. **PC-en «vet» hvor alle er** – Middels
   - Fil: `GSPlayerController.cpp` `SendTrackPulse` linje 470–485, `GSHUD.cpp` linje 1083–1159. Eier: Kamp → bestilling.
   - Hva: en vakt-PC har kamerabilder og adgangslogger, ikke GPS på folk. Innlogging tar 1,4 s uten
     passord eller kort. Kameraene kan ikke ødelegges, og ingen ser at noen bruker dem.
   - Forslag: sporingen viser bare spillere som et kamera faktisk ser akkurat nå (siktlinje fra
     kameraet), merket med kameraets navn. Kameraer kan skytes i stykker. Innlogging krever
     NTNU-adgangskort (finnes alt som gjenstand) eller tar mye lengre tid uten.

R5. **Ingen vekt: fire tunge våpen og full sekk i full sprint** – Høy
   - Fil: `GSCharacter.h` linje 64–65 (4 våpen, 6 plasser), `GSCharacter.cpp` `Tick` linje 3092–3119 (farten
     ser bare på vest og hjelm) (Eier: Kamp); `gen_cpp.py` linje 42 (stabler: mat 5, medisin 5, granater
     4) (Eier: Spillbarhet).
   - Hva: Carl Gustaf (ca. 7–10 kg) + Barrett (14 kg) + MG3 (11,5 kg) + PKM (7,5 kg) med ammunisjon er
     over 45 kg, og bæres uten at fart, stamina eller hopp endres. Uten sekk har man 6 plasser, og én
     plass kan være 5 Grandiosa-esker eller 5 førstehjelpsskrin.
   - Forslag: vekt i kg på hver gjenstand i `catalog.py`, og samlet vekt som gradvis trekker ned fart,
     stamina-henting og hopp fra ca. 15 kg. Våpenplasser etter type: to lange våpen, én pistol, ett
     nærkampvåpen. Sekkeplass som volum i stedet for «plasser».

R6. **Man dør av tørst på 18 minutter, men kjenner ingenting før de siste minuttene** – Middels
   - Fil: `GSCharacter.cpp` linje 2919–2924. Eier: Kamp.
   - Hva: fra full til tom på 18 min (tørst) og 25 min (sult), så 1 HP/s til man dør (100 s). Med
     spillets døgn på 48 min er 18 min lik 9 timer. Et menneske tåler ca. 3 døgn uten vann og uker uten
     mat, men blir merkbart dårligere lenge før.
   - Forslag: ingen død av sult/tørst i en kamp; i stedet tidlige, gradvise følger (se B2).

R7. **Mat spises for fort, og «matforgiftning» varer 20 sekunder** – Lav
   - Fil: `catalog.py` linje 251–275 og 428–441 (Eier: Spillbarhet); `GSCharacter.cpp` linje 2265 (Eier: Kamp).
   - Hva: en hel Grandiosa spises på 4 s, en kebab på 3,5 s. Matforgiftning slår til med en gang og er
     over etter 20 s; i virkeligheten kommer den etter timer. Rakfisk-teksten lover at halvparten «blir
     sterke», men ingenting skjer. Meldingen ved forgiftning sier alltid «pizzaen», også for rakfisk.
   - Forslag: lengre spisetid for store måltider (10–20 s), og rett tekstene.

#### D. Ny spiller og flyt (hva som mangler for at en hel kamp henger sammen)

F1. **Man kan ikke skrive inn en adresse i menyen, og «Start server» har ingen bekreftelse** – Høy
   - Fil: `GSHUD.cpp` `DrawMenu` linje 161–175, `GSPlayerController.cpp` `MenuClick` linje 189–190, `Host`
     linje 221–228, `Join` linje 230–236. Eier: Kamp → bestilling.
   - Hva: (1) «Bli med» bruker bare forrige adresse (første gang 127.0.0.1). For en ny adresse må
     spilleren åpne konsollen med § og skrive `Join 192.168.1.20`. Unreal har ikke konsoll i et ferdig
     pakket spill (Shipping), så der kan man ikke bli med hos noen. (2) Ett klikk på «Start server» midt
     i en kamp laster kartet på nytt: er du vert, kastes alle ut og kampen er borte; er du gjest,
     forlater du kampen. (3) Feiler tilkoblingen (feil adresse, full server), får spilleren ingen
     beskjed; koden har ingen håndtering av nettverksfeil (ingen treff på `NetworkFailure`/`TravelFailure`).
   - Forslag: tekstfelt for adresse i menyen, «Er du sikker?» på «Start server» når en kamp er i gang,
     og en melding på skjermen når tilkoblingen feiler eller verten forsvinner.

F2. **Spilleren kan ikke velge navn** – Middels
   - Fil: ingen kode setter navn (søk etter `SetPlayerName`/`ChangeName` i `Source/` gir ingen treff).
     Eier: Kamp (menyen i `GSPlayerController`/`GSHUD`) → bestilling.
   - Hva: lobby, killfeed, poengtavle, vinner, dødskort og sporingskartet viser motorens standardnavn.
     Vinneren lagres dessuten som tekst (`GSGameState::Winner`), så to like navn kan ikke skilles.
   - Forslag: navnefelt i menyen, lagret i innstillingene og sendt med ved tilkobling (`?Name=`).

F3. **Lobbyen forklarer ikke hva som skjer** – Middels
   - Fil: `GSHUD.cpp` `DrawMatch` linje 756–788 (Eier: Kamp → bestilling), `GSGameMode.cpp` `StartMatch` linje
     91–117 (Eier: Spillbarhet).
   - Hva: lobbyen sier ikke at alt du plukker opp forsvinner når kampen starter (alle får ny kropp med
     tomme lommer), ikke at kampen starter av seg selv etter 90 s, og ikke hvor mange som mangler.
     Den som kommer inn midt i en kamp får ingen beskjed om at en kamp pågår.
   - Forslag: to korte linjer i lobbyboksen, og en melding «Kampen pågår, N min igjen» ved innkomst.

F4. **Printeren forklarer ikke seg selv** – Middels
   - Fil: `GSHUD.cpp` linje 383–386 og `DrawPrinter` linje 1042–1081 (Eier: Kamp → bestilling);
     `GSTerminal.cpp` `FinishPrint` linje 170–176 (Eier: Spillbarhet).
   - Hva: printeren viser «Filament i sekken: 0 spoler» uten å si hvor filament finnes. Listen sier ikke
     hva tingene gjør. Printeren sier ikke fra når den er ferdig, og hvem som helst kan ta det som
     kommer ut (realistisk, men bør stå der). Det ferdige ligger bare 3 minutter før det forsvinner
     (`SpawnDropped` setter levetid 180 s).
   - Forslag: beskrivelse av valgt oppskrift, en linje om hvor filament finnes, lydsignal fra printeren
     når den er ferdig, og ingen tidsgrense på det som ligger i printeren.

F5. **Sekken som bæres forsvinner ved bytte og ved død** – Lav
   - Fil: `GSCharacter.cpp` `TakeFromPickup` linje 2015–2027, `DropEverything` linje 2398–2420. Eier: Kamp → bestilling.
   - Hva: bytter du liten sekk mot stor, blir den lille borte (vester og hjelmer slippes på bakken).
     Når du dør, slippes ikke sekken du har på ryggen, så den kan ikke tas fra liket.
   - Forslag: slipp den gamle sekken som pickup i begge tilfeller.

F6. **Verten forsvinner = kampen er over uten beskjed** – Lav/Middels
   - Fil: `GSPlayerController.cpp` `Host` linje 221–228 (lytteserver, ingen overtakelse). Eier: Kamp → bestilling.
   - Hva: lukker verten spillet, mister alle forbindelsen uten forklaring (se F1 punkt 3).
   - Forslag: i første omgang en tydelig melding «Verten avsluttet spillet» og retur til menyen.
