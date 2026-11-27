**GODKJENT** – med fire små rettinger til neste Kamp-runde (én av dem, IMPROVEMENTS 597, rettes før IMPROVEMENTS.md commites).

Kontrollert: `fe41bb8` (Combat round 14b) mot `ko/plan-08b.md`, `kontroll/runde-14a.md`, bestilling K1/K2 og motorkoden i
`UE_5.8/Engine/Source/Runtime`. Ikke bygget selv. Byggeren kjørte reach-testen og positurmålingene hodeløst (scratchpad
`reach2/3.log`, `pose_*.log`).

## Bygg – de tre endringene «uten fullt bygg»
Bevis for at de kompilerer, ikke bare syntaks lest:
- Arbeidsmappen er lik commiten for `GSCharacter.cpp/.h` og `GSHUD.cpp` (`git diff HEAD` tom).
- `GSCharacter.cpp` sist endret 10:46:21, `GSCharacter.cpp.obj` skrevet 10:47:52 (etter siste endring, altså vellykket
  kompilering av gevær −7 cm og testkoden uten `UEnum`). `reach3.log` (10:48) er kjørt med denne koden: gevær i hofta 99 %.
- `GSHUD.cpp` endret 10:47:21, kompilert uten feil i `build3.log` (`[3/12]`, `.obj` 11:00:43). Feilene i build3 er bare
  `GSDoor.h`/`GSDoor.cpp` (annet lag).
- `build4.log` (11:03, etter commiten) kompilerte unity-filene på nytt og lenket: **Result: Succeeded**.
- Lest i tillegg: `Hip`-linjen (`bMeleeWeapon` er deklarert før, l. 1385), `static_cast<int32>(GS->CaptureGoal)` (`uint8`,
  `GS` null-sjekket l. 1050), `TickReachTest` med egne navnetabeller og `UE_ARRAY_COUNT` som `constexpr`. ✔

## Andreas' regel
- **Forbedret:** de løse førstepersonsarmene → kroppens egne hender; ladelomma flyttet til venstre side av beltet; huk med
  hofta bakover (knærne inne i beintreffet); hodetreffet sentrert; skyggen holder nå våpen og utstyr; «Avslutt» virker i
  Play; skarpere bilde (72 % → 85/100 %).
- **Nytt:** hånd-IK på egen kropp med rekkeviddeklemme som flytter `FPArmL` (magasinet følger), fot-IK med gulvsporing og
  senket hofte, armer skjult i kikkert, `IsLocalHuman()` for bots, `gs.FP.ReachTest`/`gs.FP.TestPose` for målinger.
- Begge deler finnes. ✔

## De fem funnene fra 14a
1. **Skygge med våpen** – ✔ `SetGearShadowType(WorldSpaceRepresentation)` på våpen, fester, armbind, vest, hjelm og sekk i
   `SetupLocalBody`; tilbake til `None` i gammel-kropp-grenen, `FallBackToOldBody` og `OnRep_Dead`. Bare på eierens maskin.
2. **Bots** – ✔ (K1, se under).
3. **Knær i huk** – ✔ `CrouchBack = 12`, `CrouchTip = 20`. Målt (`pose_0_1.log`): knær 17–18 cm *innenfor* fronten av
   den hukende beinboksen, hodet inne (fram −0,5…0,3, opp −9,5…−9,9).
4. **Hodeboks** – ✔ `Stand.X` 3 → 0. Målt: sikte +80 fram −8,4, sikte −80 fram +8,3, begge innenfor ±9,5.
5. **Avslutt i PIE** – ✔ `UKismetSystemLibrary::QuitGame`.

## K1 – `IsLocalHuman()` (sjekket mot motoren)
`IsLocallyControlled() && GetController()->IsPlayerController()`. `APawn::IsLocallyControlled` (Pawn.cpp 272) er
`GetController() && GetController()->IsLocalController()`, så kontrolleren er aldri null i andre ledd.
- **Vert (lytteserver):** egen figur – lokal PlayerController → sann. Klienters figurer – fjern PlayerController → usann. ✔
- **Klient:** egen figur – sann når kontrolleren er replikert; andres figurer har ingen kontroller → usann. ✔
- **Bots:** `AIController` er lokal på serveren og i frittstående (`IsLocalController` gir sann i `NM_Standalone`), men
  `IsPlayerController()` er usann → usann. ✔
- **PIE med flere vinduer:** hver verden har egen nettmodus og egne kontrollere; samme logikk som over. ✔
- **Avvik fra bestillingen godtatt:** `IsPlayerControlled()` (Pawn.cpp 286) leser spillertilstanden, som er en egen aktør og
  kan komme etter kontrolleren hos en klient; da ville `PawnClientRestart` hoppe over egen kropp. Kontrollerens type er
  tryggere. (Lærdommen fra 14a oppdateres, se under.)
- **Dekning:** alle stedene i bestillingens (a) og (b) bruker den: `FireOnce` (2D-lyd, egen kule, munningsflamme, kick,
  pumpe/bolt/hylser), `MulticastShot/Pellet/MeleeFX`, `bOwnView`, `OnRep_Health`, `ClientTookHit/HitMarker/Toast`,
  `SetupLocalBody`, `OwnAnim`, `UpdateFocusPickup`/`UpdateLocalVisuals`-grenen, spøkelsesflimmer, tredjepersonsvåpen.
  `IsLocallyControlled()` er beholdt der den skal: inngangen til `FireOnce` og skuddsjekken i `ServerFire` (l. 1031–1084:
  ingen forsinkelseskompensasjon og ingen ekstra rekyl for en bot, riktig). Ingen andre filer bruker
  `IsLocallyControlled`. Ingenting dobbelt: botens skudd tegnes bare av multicasten, og `FireOnce` hopper over sitt eget for
  bots. Botens tredjepersonsvåpen plasseres nå også hos verten (før: aldri, fordi boten var «lokal»).
- **Sikte i høyden:** `Camera->SetWorldRotation(GetControlRotation())` for bot i `Tick`, og `Aim = GetControlRotation()` i
  `FireOnce` – riktig; motoren snur kameraet bare i en spillers `GetCameraView`. `friend class AGSBotController` ✔.
- **K2** ✔ poengtavla leser `GS->CaptureGoal`.

## Skarphet (`Config/DefaultScalability.ini`)
- Nøkkelen er riktig: `Scalability.cpp` 651 leser `[ScalabilitySettings] PerfIndexValues_ResolutionQuality` fra
  `GScalabilityIni`, og `SetFromSingleQualityLevel` (1047) bruker den. Fem verdier som motorens. ✔
- **Spillerens valg:** lagres i `GameUserSettings.ini` (`sg.ResolutionQuality`) og overstyres ikke ved oppstart. Tabellen
  brukes bare når et nivå settes. ✔ Bonus: med 85/100 er `GetSingleQualityLevel` ikke lenger «egendefinert» (−1).
- **Første oppstart:** `!bDone` → Høy → 100 %. ✔ **Migrering:** bare når den lagrede verdien er nøyaktig 72 (gammel
  standard, `RoundToInt`), én gang (`SharpDefaultsApplied`), nivå fra skyggenivået. Sjekker bare lokal kontroller, ikke i
  editoren. ✔ (Den som selv valgte 72 % får den overskrevet én gang – ubetydelig.)
- **Funn 2** (under): esc-menyen/«Grafikk N» og hovedmenyen behandler nå oppløsningen ulikt.

## Plan-08b, steg for steg
1. **Hender på `FPBody`** ✔ Kragebeina vises når `UsesBodyHands()`; gamle armer `SetVisibility(false)` (ikke
   `SetHiddenInGame`, så `FPMag` på `FPArmL` vises). Tilbakefall: `UpdateArms` setter armenes synlighet hvert bilde, så
   `gs.FP.Hands 0` gir gamle armer straks (kragebein skjules, hofte- og lommeplass går tilbake). `FallBackToOldBody` setter
   `bTrueFP = false` → `UsesBodyHands()` usann. ✔ Men se funn 1 (synlighet vippes hvert bilde).
2. **Mål uten verdensrom** – valgt alternativet: `Camera->SetWorldRotation(GetControlRotation())` etter `TickRecoil`/
   `TickScopeSway` og før `UpdateWeaponAnimation`; målene regnes etterpå og kroppen evalueres etter aktøren (forutsetning
   fra 14a). Helning (lean) ligger i kameraets additive forskyvning og påvirkes ikke. ✔
3. **Rekkevidde** ✔ klemt til 97 %, `FPArmL` flyttes med `SetWorldLocation` (stabil: `VInterpTo` neste bilde starter fra
   det klemte punktet og konvergerer, ingen drift). Målt (`reach3.log`, pitch 0): høyeste 99 % (gevær hofte, venstre),
   ellers 52–98 %. Alle klasser ≤ 100 %. Gevær −7/−2 cm i hofta, lomma (−17, −24, −40). Se funn 3 (bratt sikte ikke målt).
4. **Hånd-IK i `UGSBodyAnim`** ✔ også for `bFirstPerson`, rask inn/ut (30/s) for egen kropp. Albuehint (ned 30, ut 25,
   bak 10). Ting: høyre hånd til `FPItemHand` (skala 1). Uten våpen og ting: ingen IK.
5. **Fot-IK** ✔ to objektspor (`WorldStatic`+`WorldDynamic`, ignorer seg selv) per kropp, bare < 200 cm/s, ikke fall/død,
   innen 25 m og sett siste 0,2 s (andre), alltid for egen. Klemt ±45, normal.Z > 0,6, død sone 1,5 cm, hofte
   `min(0, min(L, R))`, ut/inn over 0,1 s. Gulvet = mesh-origo − kapselens gap (2,15 cm som reserve for simulerte figurer,
   der `CurrentFloor` ikke regnes). Målt flatt gulv: 0,0/0,0 cm. **Kostnad:** høyst 2 spor × synlige, stillestående figurer
   innen 25 m + 4 for eieren; med 20 spillere ≤ ~44 korte vertikale spor per bilde, ingen på dedikert server. Ubetydelig. ✔
   Trapp er ikke prøvd (ingen PIE).
6. **Kikkert** ✔ kragebein skjules ved `IsScopedIn() && AimBlend > 0.85`, bare når tilstanden endres.
7. Valgfri førstepersonsskalering – hoppet over (tillatt).
8. PIE – ikke gjort (Andreas vil ikke teste nå; hodeløse målinger i stedet).

## Skudd og treffbokser
- Skuddet starter fortsatt i kameraet (`Start = Camera->GetComponentLocation()`), og serveren sjekker som før.
  `FPWeaponMesh`s plass brukes bare til munningsflamme, røyk, hylser og magasin som faller – kosmetisk. Gevær 7 cm nærmere
  og hånd-IK endrer **ikke** hvor skudd starter. ✔
- Treffboksene er faste bokser i kapselen; hofte-/fotendringene endrer bare bildet, og målingene over viser at det nå
  stemmer bedre med boksene. Fot-IK senker hofta høyst 45 cm på andres kropp, men bare i trapper/kanter og bare visuelt.

## Funn
1. **Gamle armer vippes synlig/usynlig hvert bilde** (`GSCharacter.cpp`, `Tick` l. ~4617–4620 mot `UpdateArms` l. 1734/1780 og
   `UpdateItemUse` l. 1831). `UpdateArms` setter `FPArmR/FPArmL` synlige, så setter `Tick` dem usynlige igjen. Hver endring går
   gjennom `USceneComponent::SetVisibility` → `OnVisibilityChanged` → `MarkRenderStateDirty` (SceneComponent.cpp 3576), så
   to (tre ved spising) komponenter får gjenskapt render-tilstanden hvert bilde uten grunn. Ikke synlig, men unødvendig.
   **Rett:** send `UsesBodyHands()` inn i `UpdateArms`/`UpdateItemUse` og sett `false` der (behold punktenes bevegelse), og
   fjern de tre linjene i `Tick`.
2. **Grafikknivå og oppløsning følger to regler** (`GSPlayerController.cpp` `ApplyGraphicsLevel` mot `GSMenu.cpp` 1167–1171).
   Esc-menyen/«Grafikk N» setter nå oppløsningen til nivåets verdi, mens hovedmenyen (`Set.Graphics`) beholder spillerens
   egen. Kommentarene i `DefaultScalability.ini` og `ApplyGraphicsLevel` («the same as when the main menu picks it») og
   IMPROVEMENTS **597** («også når grafikknivået velges i hovedmenyen») stemmer ikke. **Rett:** én regel. Anbefalt: behold
   spillerens oppløsning også i `ApplyGraphicsLevel` (de to fjernede linjene tilbake) – migreringen har allerede tatt bort
   72 %, og første oppstart får 100 %. Skriv 597 om til «Skarpere bilde: første oppstart og gamle 72 % blir full oppløsning
   på Høy/Episk (85 % på Lav/Middels)». **Før IMPROVEMENTS.md commites:** rett 597.
3. **Rekkevidde ved bratt sikte er ikke målt.** `FPBody` bøyer ikke ryggen (`bFirstPerson`), men førstepersonsvåpenet går
   rundt kameraet med blikket. Ser du 60–80° opp, kommer våpenet over hodet, og høyre hånd kan bli klemt til 97 % uten at
   våpenet følger (bare `FPArmL` flyttes) – hånda slipper grepet. **Forslag:** la `gs.FP.ReachTest` også gå gjennom
   sikte ±60/±80 (sett `ControlRotation.Pitch`), skriv tallene, og flytt våpenet nærmere/ned ved høy pitch eller tillat
   litt ryggbøy på egen kropp hvis noe er over 100 %.
4. **IMPROVEMENTS.md er ikke med i commiten** (582–597 ligger endret i arbeidsmappen, som i 14a). Dirigenten må få den med,
   etter rettingen av 597.

## Andreas tester (når han vil – ikke spør nå)
Se ned med hvert våpen (hofte, sikte, lading, pumpe/bolt, slag, spis/drikk): hendene skal sitte på våpenet; snu fort; se
opp/ned bratt; gå opp/ned trappene i Helgasetr og se på føttene (egne og andres); kikkert; `gs.FP.Hands 0` (gamle armer);
`gs.FP.Debug 1` viser rekkevidde og fotmål.

## Forslag til neste runde
1. Funn 1–3 (små): synlighet uten vipping, én regel for oppløsning, rekkevidde ved bratt sikte.
2. Mål bildefrekvensen med 100 % på Høy på RTX 3050-laptopen (`stat unit` / hodeløs `-benchmark` på Helgasetr); er den
   under 60, vurder 90 % på Høy i `DefaultScalability.ini`.
3. Stående beinboks: målingen viser knærne 8 cm foran/6 cm bak beinboksen midt i et steg (`pose_0_0.log`, boks ±9) – utvid
   `Stand`-beinboksens X-utstrekning litt (eller to bokser per bein), så skudd på synlige knær teller.

## Lærdom
- «Bare for spilleren bak skjermen» = `IsLocallyControlled() && GetController()->IsPlayerController()`, ikke
  `IsPlayerControlled()` (spillertilstanden kan komme etter kontrolleren hos klienten). Erstatter regelen fra 14a. (runde 14b)
- `SetVisibility` på en komponent som en annen funksjon setter tilbake samme bilde gjenskaper render-tilstanden hvert
  bilde; bestem synligheten ett sted. (runde 14b)
- Når to lag har hver sin vei til samme innstilling (meny vs. konsoll), skal planen si hvilken regel begge følger. (runde 14b)
