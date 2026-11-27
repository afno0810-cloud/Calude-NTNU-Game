# Forhåndssjekk av plan-01 (Interaktiv verden)

**BØR ENDRES.** Grunnideen holder. Den ene replikerte lista, de lokale dørene og ISM-glasset er riktig valgt, og
motorkoden bekrefter at skala 0 fjerner kollisjonen. Men 7 ting vil gi feil i spillet om de bygges som planen står
nå (punkt 1–7 under). I tillegg er runden for stor, og nesten all glass-delen kan ikke prøves før Kamp har levert
K1–K3. Ingen av Kamps planer i `.claude/loop/ko/` tar med K1–K5 ennå.

Sjekket mot HEAD `fccde97` og arbeidsmappen (det finnes ingen `GSHelgasetr.h` ennå, og ingen K-kroker hos Kamp), og mot
motorkoden i `UE_5.8` (EnhancedPlayerInput.cpp, InstancedStaticMesh.cpp, InstancedMeshComponentBodies.cpp).

## Motorsjekk (svar på planens risikopunkter)
- **(d) Skala 0 på en ISM-instans:** ja, det virker i 5.8. `UpdateInstanceTransform` kaller `UpdateInstanceBodyTransform`, og
  `FInstancedMeshComponentBodies::UpdateTransform` (InstancedMeshComponentBodies.cpp l. 152) kjører `TermBody()` og sletter
  legemet når `GetScale3D().IsNearlyZero()`. Når skalaen settes tilbake, lager l. 169–171 legemet på nytt. Ingen grunn til å
  flytte instansen 100 km ned. Pass på at `bPhysicsStateCreated` er sann (kollisjon på før `RegisterComponent`).
- **(a) Dobbel input i Enhanced Input:** en ekstra `UEnhancedInputComponent` som legges på med `PushInputComponent`, sluker
  ikke E. Bindinger konsumerer ingenting som standard (`bConsumes = false`), og nøkkelsjekken er av, fordi
  `bShouldCheckForLegacyKeyConsumption = false`. Både `AGSCharacter::Interact` og interaktoren får derfor `Started`. Den
  pålagte komponenten behandles først i samme bilde. Når GSMenu er åpen (modal), sluker `OnKey` alle taster før de når
  Enhanced Input, så E kommer ikke fram dit.

## Må endres i planen
1. **Dobbel handling ved slipp (a).** Kamps `Interact` virker når tasten trykkes ned (`Started`). Interaktoren bestemmer seg
   først når tasten slippes (`Tap`). Eksempel: E trykkes ned, og Kamp plukker opp en ting (eller åpner en PC). Når E slippes,
   er fokuset på tingen borte, og døra bak får fokus. Slippet åpner da døra, og E har gjort to ting. **Endring:** avgjør
   *ved trykket* hvem trykket tilhører, og lagre det (`bPressMine`). Er det ikke vårt (Kamp har pickup/terminal i fokus,
   eller vi har ikke fokus), ignorerer vi hele trykket helt til E er sluppet. Det samme gjelder etter K4.
2. **Sperrene må være de samme som Kamps.** `bShowMouseCursor` er ikke nok, fordi inventaret (`bShowInventory`) ikke viser
   musepekeren. Bruk `PC->IsGameInputBlocked() || PC->bShowInventory || PC->IsOnTerminal()`, og at pawnen er død eller
   mangler. Bruk også `PC->IsLocalController()` når interaktoren skal binde input og tegne (verten har interaktorer for
   klientene sine). Kall `PopInputComponent` i `EndPlay`.
3. **Sen klient kan få «ferske» effekter for gamle hendelser (b).** Regelen `bFresh = (ServerTime − ChangedAt) < 2 s` stoler
   på `GetServerWorldTimeSeconds()`. Hos en nykommer er den feil til `GameState` har replikert tidsavviket. Kommer
   `AGSMapState` før `GameState`, blir differansen negativ, og *alle* gamle knuste ruter spiller biter og lyd. **Endring:**
   alt som kommer i den første replikeringen av lista hos en klient, eller før registeret er bygd (`PendingKeys` og
   `ApplyAllLocal`), er aldri ferskt. Krev også at `0 ≤ differansen < 2 s`.
4. **Låst grupperom fra innsiden.** Tabellen sier ikke hva som skjer fra innsiden av en låst glassdør. Kodelåsen og
   `UsePoint` sitter på utsiden. Fokussporet fra innsiden treffer da veggen, så den som er inne blir sperret inne. Det
   samme gjelder alarmdøra. **Endring:** `GetUsePoint`/`GetUsePrompt`/`ServerUse` regner ut fra hvilken side brukeren står
   på. Fra innsiden åpner `Tap` alltid (dørvrider eller panikkstang, slik ekte rømningsdører er), uten kode og uten å endre
   låsen. Alarmdøra åpnes fra innsiden av trapperommet med panikkstanga.
5. **Fokus og server-sjekk ser gjennom glass.** Glass ignorerer `Visibility`. Da kan fokussporet og serverens avstandssjekk
   (bare 250 cm) åpne eller dirke en dør på andre siden av en glassvegg. **Endring:** gjør fokussporet som
   `LineTraceSingleByObjectType` mot `WorldStatic | WorldDynamic | Destructible`, og la døra selv være unntatt. Serveren
   kjører det samme sporet fra pawnens øye (`GetPawnViewLocation`) før `ServerUse`.
6. **Glass midt i en vegg (c, K1).** Bestillingen sier «etter at `End` er regnet ut»: `BulletSegment(B.Pos, End)`. Treffer
   Kamps eget spor en vegg før `End`, telles glass *bak* veggen med (kula bremses, og ruta knuses gjennom en betongvegg).
   **Endring i bestilling K1:** bruk `bHit ? Hit.ImpactPoint : End` som `To`.
7. **Slag gjennom glass (c, K3).** `DoMeleeHit` sveiper med `ECC_Visibility` (GSCharacter.cpp l. 1163) og går gjennom
   hele ruter. Kniv og kolbe treffer da en spiller som står bak en hel glassvegg. **Endring i K3:** `ApplyMelee` returnerer
   avstanden til første hele rute (eller `bool bBlocked`), og Kamp kaster treffet på figuren hvis ruta er nærmere. Legg det
   til i bestillingen.

## Bør endres (mindre, men ekte)
- **Hint og framdrift overlapper Kamps `DrawUseProgress`.** Planen sier den står «venstre side, y-liste». Det stemmer ikke:
  GSHUD.cpp l. 449 tegner den midt på (`SizeY*0,5 + 40`, tekst på +18). Når noen bandasjerer ved en dør, havner den oppå
  hintet vårt (+60). **Endring:** ikke tegn hint eller framdrift når `C->UseItemId` er satt, og la serveren avvise dørjobber
  mens en gjenstand brukes.
- **Glassruta i glassdøra.** Ruta har samme nøkkel som døra (kategori `Door`) og ligger i et eget `UGSGlassSet` med én
  instans som svinger med bladet. Det gir et nøkkelsammenstøt i `TMap`-registeret, og ISM-instanslegemer som oppdateres
  hvert bilde mens døra går. **Endring:** gjør ruta til en vanlig `UStaticMeshComponent` med objekttypen `Destructible` på
  bladet. `BulletSegment` slår opp `Hit.GetActor()` som `AGSDoor` og gir skaden til døra (bit 2). Ruta skal, som `LeafBox`,
  ignorere `Pawn` mens døra går.
- **Skjule Grafikks glass i 4a.** I plan-04 lager 4a rutene med `GSUtils::Cube()`, ikke med `SM_Mod_Helg_GlassPanel`, og de
  får bare taggen `Mat_Glass`. Fast glass (tak og vinduer) har *også* `Mat_Glass`. Å skjule etter mesh-navn treffer ingen
  ting, og å skjule etter `Mat_Glass` tar bort glasstaket. **Endring:** skjul bare ISM-er på byggeren med taggen
  `GS_Int_Glass`, eller med `Mat_Glass` *og* `GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore` (det er rutene).
  Det faste glasset blokkerer `Visibility`. Logg hvor mange som ble skjult.
- **`UpdateInstanceTransform(..., bMarkRenderStateDirty=true)`** på en ISM med 1 200 instanser bygger hele proxyen på nytt.
  En granat (40 ruter), og hver kule mot et merke (30 per sekund), gir da hakk. **Endring:** `false` overalt. I 5.8 går
  oppdateringen gjennom `PrimitiveInstanceDataManager`. Rutene får uansett `bTeleport = true`.
- **Branndør som lukker seg** oppå en spiller som står i døråpningen. **Endring:** serveren venter (prøver igjen hvert 1 s)
  så lenge en `Pawn` overlapper svingboksen.
- **`PredictUse`** bare når den replikerte tilstanden sier at døra er ulåst og ikke har en jobb. Ellers går døra opp og så
  igjen hos eieren.
- **Kulehull: taket er uklart.** «400 per maskin (ringbuffer per ISM)» kan leses som 400 per ISM, som blir 8 × 400.
  **Endring:** 400 merker i alt, med én felles ringbuffer av merke-id-er. Hvert merke har 2–8 instanser. `MarksISM` på
  dører lages først ved det første merket (ikke 120 tomme ISM-er).
- **Kodelåsens lysdiode:** bruk et emissivt materiale, ikke `UPointLightComponent`. Det er 30–40 lys i grupperomsgangene
  uten at det synes bedre.
- **Bots ved dører:** bruk `!Pawn->IsPlayerControlled()` (lærdom 14a), og la serveren åpne for boten bare når døra er
  ulåst og lukket.

## Svar på de andre spørsmålene
- **(b) Båndbredde:** et element er ca. 20 B (nøkkel 4, fem byte, normal 6, tid 4). Selv om alle 1 300 deler er endret, er
  den første pakken til en sen klient ca. 26 kB. Det er godt innenfor det `FFastArraySerializer` tåler
  (`MaxNumberOfAllowedChangesPerUpdate` 2048). En granat med 40 ruter er under 1 kB. 20 Hz er greit.
  **Rekkefølge mot kartbygging:** byggeren bygger synkront i `GSWorldSubsystem::OnWorldBeginPlay` på alle maskiner, så
  timeren på 0,5 s holder. `PendingKeys` og `ApplyAllLocal` dekker resten. **Lokale dører:** alle maskiner kommer til samme
  sluttilstand fra den samme `Open`-verdien. Mens bladet går, ignorerer det `Pawn` på begge sider, så det blir ikke
  rubberbanding da. En liten retting er mulig når en dør *lukkes* (serverens blad stopper og blokkerer ca. RTT/2 før
  klientens). Det godtar vi, og det er det samme som med en replikert dør.
- **(c) `ECC_Destructible` som ignorerer Visibility,** gått gjennom spor for spor: kuler (`GSBallistics`, Visibility) går
  gjennom, og det er meningen (K1). Slag går gjennom (punkt 7). Pil- og rakett-sporet for treff (GSProjectile l. 200,
  Visibility) går gjennom, men selve kulekollisjonen `BlockAllDynamic` blokkeres av glasset (glass blokkerer
  `WorldDynamic`), så granater spretter av hele ruter. Det er greit. Dødskameraet (`ECC_Camera`) går gjennom glass, og det
  er greit. `DamageAround` (WorldStatic) skader gjennom en hel rute, men `ApplyBlast` knuser den først (K2 før
  `DamageAround`). Gripekroken (WorldStatic l. 2480) fester seg bak glass, og det er greit og et lite funn. Klatring
  (WorldStatic) ser ikke glassrekkverk, men kapselprøven (`ECC_Pawn`) stopper. Spawn (`ECC_Pawn`-overlapp) tar med glass,
  og det er bra. HUD-navn, flagglinjer, blod og nedbør ser gjennom glass, og det er riktig.
- **(e) Ytelse:** 400 merker × 2–8 instanser i ca. 8 ISM-er, uten kollisjon og skygge og med cull på 30 m, er billig.
  150 fysikkbiter som egne komponenter gir opptil 150 tegnekall i 3,5 s, og det er akseptabelt med taket. Den eneste
  ekte fellen er `bMarkRenderStateDirty` (over).
- **(f) Avhengighet:** `GSHelgasetr.h` finnes ikke ennå. `__has_include` er riktig vei. Navnene i plan-04 er `GlassPanels`,
  `DoorSlots`, `FPanelM` og `FDoorM`. Felt som yaw, hengsel og låst er bestilt, men ikke med i plan-04, så gjettingen må
  være med. K1–K5 er ikke tatt med i noen av Kamps planer. Uten dem kan ingen knuse glass i runde 1 (bare `ApplyBlast` og
  `DamageGlass` via egne kommandoer). Da kan ikke «Ferdig når: alle tre glasstypene …» prøves.
  **Endring:** legg til konsollkommandoene `Interaktiv.Knus [skade]` (gjør `DamageGlass` på ruta du ser på, som en kule)
  og `Interaktiv.Smell` (`ApplyBlast` der du ser, 3 m). Sett «Ferdig når» for glass til å gjelde disse. Dirigenten bør be
  Kamp ta K1 (6 linjer) og K4 tidlig.
- **(g) Størrelse:** 8 nye kildefiler, et Blender-skript med ca. 10 modeller, testrom, Bots-støtte, 4 konsollkommandoer og
  16 IMPROVEMENTS-linjer er for mye for én runde. **Del i to:**
  - **1a:** `GSMapState` (liste, register, tilbakestilling, subsystem), `GSInteract` (interaktor, hint), `GSDoor` (alle
    dørtypene og låser), testrommet med bare dører, og bots ved dører.
  - **1b:** `GSBreakable` (glass, biter, skår, kulehull), glass i testrommet, `interactive_doors.py` og modellene.
  1a bygger og kan prøves helt uten Kamp. 1b får da K1-tidspunktet fra Kamp. Byggerens forbedret/nytt-regel holder i
  begge: 1a forbedrer dirkesett og adgangskort og legger til dører, og 1b forbedrer kulenedslaget og legger til glass.

## Liste til byggeren
1. Lagre ved trykket om E er vårt (`bPressMine`). Er det ikke vårt, ignorer trykket helt til tasten slippes.
2. Sperrer: `IsGameInputBlocked() || bShowInventory || IsOnTerminal()`, og at pawnen er død eller mangler. Input og tegning
   bare når `IsLocalController()`. `PopInputComponent` i `EndPlay`.
3. Ikke ferskt ved første replikering, ved `PendingKeys` eller ved `ApplyAllLocal`. Krev `0 ≤ dt < 2 s`.
4. Fra innsiden åpner `Tap` alltid. `UsePoint` og hint følger brukerens side.
5. Fokus- og serverspor: objektspor mot WorldStatic, WorldDynamic og Destructible, med døra unntatt.
6. Ruta i glassdøra er en `UStaticMeshComponent` (Destructible) på bladet, og skaden går til døra.
7. Skjul 4a-glass bare med `GS_Int_Glass`, eller `Mat_Glass` + ignorerer Visibility, og slå av kollisjonen deres.
8. `bMarkRenderStateDirty = false` på alle ISM-oppdateringer.
9. Ikke hint eller dørjobb mens `UseItemId` er satt. Branndøra venter på en tom døråpning.
10. Kulehull: 400 i alt (én felles ringbuffer). `MarksISM` på dører lages først ved behov. Emissiv lysdiode, ikke punktlys.
11. Konsollen `Interaktiv.Knus` og `Interaktiv.Smell`, så glasset kan prøves uten K1/K2.
12. Ingen flytting av instanser 100 km ned (motoren fjerner legemet ved skala 0, sjekket).
13. Bestillingen til Kamp oppdateres: K1 `To = bHit ? ImpactPoint : End`, og K3 `ApplyMelee` stopper slaget ved glass.
14. Prøv input i PIE (Selected Viewport): pickup med dør bak (trykk og slipp), inventaret åpent, P/F10-menyen åpen, og
    bandasjering ved døra.
