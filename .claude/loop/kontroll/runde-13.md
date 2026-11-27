**MÅ FIKSES** – Kamp runde 13 (plan-13, commit `2d16a3b`): én ting mangler. Retting 12 er merket RETTET, men AG3 når fortsatt ikke grepet når man sikter 70° opp: **105 %** (grensen er 100 %). Alt annet er godkjent.

Kontrollert ved å lese commiten (`git show 2d16a3b`, hele filer med `git show 2d16a3b:<fil>`), ikke arbeidsmappen. Interaktivs filer (`GSMapState.cpp`, `GSBreakable.cpp`, `GSDoor.cpp`, `GSInteract.cpp`) er lest slik de var ved commiten. Jeg har ikke bygget (det sier instruksen). Unreal-editoren kjørte ikke, så jeg kjørte en hodeløs test med binæren som lå der (`UnrealEditor-Gloshaugen.dll`, bygd 12:13:19, 50 s før commiten):
`UnrealEditor-Cmd Gloshaugen.uproject -game -nullrhi -nosound -ExecCmds="gs.FP.ReachTest 1, gs.Test.Glasskudd"`.

## Resultat av den hodeløse testen
- `gs.Test.Glasskudd`: **OK**. En 5,56-kule mot rute 19 (glassvegg, 4. etasje) gikk gjennom 5 glassvegger på rad i grupperommene (19, 27, 35, 39 og 62 ligger på linja y = 12,6 m, uten vegger imellom). Alle 5 ble knust. Loggen: `GlassF 0.779` (3 ruter i første steg) og så `0.846` (2 ruter). Farten falt fra 927 til 604 m/s og skaden fra 30 til 19,8. Tallene stemmer med 0,92 per rute.
- `gs.FP.ReachTest` (30 stillinger): alle er ≤ 100 % unntatt **rifle (AG3) sikte +70: høyre 79 %, venstre 105 %, OVER 100**. Det er samme tall som kommentaren i koden sier det var *før* rettingen («AG3 aimed 70 degrees up was 105 %»). Andre tall ved ±70°: LMG (MG3) +70 gir 85/89, hagle (Pump) +70 gir 77/95, SVD +70 gir 79/82, og alle −70 ligger på 30–82 %.

## Funn
1. **MÅ FIKSES – Retting 12 virker ikke (AG3 +70° = 105 %).** `GSCharacter.cpp` l. 1443–1450 (`UpdateWeaponAnimation`): `Pos.X -= 6.f * Steep` gir 4,3 cm ved 70°, men målingen endret seg ikke. Venstre hånd blir da trukket inn til 97 % (`FillOwnHands`, l. ~3600), så hånden svever omtrent 5 cm bak håndvernet. Planen sier rett ut «til målingen er ≤ 100 %». Både `rettinger.md` (12 RETTET) og IMPROVEMENTS 762 sier at dette virker, og det gjør det ikke.
   Hva byggeren må gjøre: bygg på nytt og kjør `gs.FP.ReachTest 1`. Hvis det fortsatt er 105 %: logg `Steep` og `UsesBodyHands()` i stillingen for å se at grenen faktisk nås. Flytt så våpenet også ned (Z), eller mer bakover (X), til AG3 +70 er ≤ 100 % uten at andre stillinger går over. Skriv tallene før og etter i rapporten.

## Planens «Ferdig når», punkt for punkt
- **Bygger:** ikke kjørt av meg. Binæren lastet og kjørte uten feil. Testkommandoen er ny i denne commiten og finnes i binæren, så den er bygget fra denne koden.
- **Retting 10 (armer vippes):** OK. `Tick` setter ikke lenger synlighet. `UpdateArms` (l. 1763/1809) og `UpdateItemUse` (l. 1861) bestemmer synligheten ett sted, og `SetVisibility` gjør ingenting når verdien ikke endres. `UpdateWeaponAnimation` har ingen tidlig `return` før `UpdateArms`, så synligheten settes hvert bilde så lenge man lever.
- **Retting 11 (oppløsning):** OK. `ApplyGraphicsLevel` leser oppløsningen før nivået settes og legger den tilbake etterpå, likt med `Set.Graphics` i `GSMenu.cpp`. Både 597 og ini-kommentaren er skrevet om.
- **Retting 12:** **ikke ferdig**, se funn 1. Testen er utvidet riktig: 5 stillinger, riktig vinkel holdes, og blikket settes tilbake til vannrett etterpå. Klasser uten modell hopper over `NumPoses - 1`.
- **Retting 13 (beinboks):** OK. `StandExt.X` har gått fra 9 til 13. Magen går fra Z −2 til 33 og beina fra −92 til −2, så de berører hverandre uten å overlappe.
- **Retting 14–16 (lydgrupper):** OK. Våpen bruker nå 24 plasser med «fjernest først». Halene har en egen gruppe med 6, egne 2D-skudd 4 og Flag/Radio 4. Treffmarkør, øresus og hjerteslag deler ikke lenger plass med flaggradioen. Selvtesten bruker motorens eget tall og 4 skyttere. Se merknadene under.
- **K1 kuler:** OK.
  - `BulletSegment` kalles for ekte og viste kuler, og bare fram til `End` (treffpunktet).
  - `ApplyBulletHit` kalles bare for ekte kuler som ikke treffer en figur, med skaden etter avstand.
  - `AddBulletMark` kalles bare for viste kuler, før `SpawnImpact`. Flisa er fjernet, og det er riktig, for 1b lager ekte hull (`GSBreakable::AddMark`, med tak via `TakeMarkToken`).
  - Kulehull på dører og kodelåser kunne ikke sjekkes hodeløst.
- **K2 eksplosjoner:** OK. Kallet ligger bare i grenen for granat/mine/C4/rakett, kommer før `DamageAround` og er vernet med `Radius > 0`. `Explode` kjører bare på serveren. Flash, røyk og brann får ikke kallet.
- **K3 nærkamp:** OK (se merknad 4). `ApplyMelee` kalles før sveipet, og et treff bak glass gir verken skade eller støv.
- **K4 bruk-tasten:** koden er riktig (se under), men **ikke testet i PIE** av meg (Andreas spilte). Lærdommen krever at alt som har med input å gjøre testes i PIE. Det må gjøres når editoren er ledig.
- **K5 lyder:** OK. Det finnes 6 wav-filer, de står i `sources.json` og `CREDITS.md` (Kenney, CC0), og `GSAudio::Get` finner `_1.._n`. `S_GS_Door_Open/Close_*.uasset` er ikke importert ennå, fordi editoren må åpnes én gang. Fram til da spiller dørene reservelydene.
- **Bestilling og rettinger:** K1–K5 er merket LEVERT, og 10–16 er merket RETTET. Merkingen av 12 er feil, se funn 1.

## De særlige sjekkene dirigenten ba om
- **Kuler gjennom vegger via glass: kan ikke skje.**
  - Glasset (`UGSGlassSet`, glassfeltet i dørene) har typen `ECC_Destructible` og ignorerer `Visibility`. Kulesporet stopper derfor på første vegg eller dørblad og går bare gjennom glass.
  - `BulletSegment` søker bare fra `B.Pos` til `End`, og `End` er treffpunktet. Glass bak veggen blir ikke rørt.
  - Knuste ruter skaleres til 0 (`SetPaneShown`), så kroppen deres forsvinner med en gang og bremser ikke flere kuler.
  - Fast glass fra byggeren (fasadevinduer, tak) blokkerer `Visibility`. Kula stopper der som mot en vegg, med et glasshull, og ruta knuses ikke (se forslag 3).
- **Vist kule og ekte kule: ingen ulik spilltilstand, bare en usynlig forskjell.**
  - Bare ekte kuler på serveren skader glass. Viste kuler endrer ikke glasstilstanden, de lager bare lokale sprekkestjerner i laminert glass.
  - Forskjellen byggeren nevner: hos verten flyttes den ekte kula først i samme bilde. Ruta er da borte når den viste kula kommer, så den viste blir ikke bremset. Hos en klient kan det gå motsatt vei hvis knusingen ennå ikke er kommet fram.
  - Utslaget er 8 % fart for én kule, og det gir under 1 mm forskjell i kulefall over 20 m. Det påvirker bare hvor støv og hull havner, ikke skade. Det er greit og ikke Kamps ansvar å rette.
- **Drapstall gjennom én rute** (bryst, ingen vest, innenfor nullavstand, skade × 0,92). Normalt antall treff → gjennom glass:
  - Glock 22 → 20,2: **5 → 5**
  - Makarov 20 → 18,4: **5 → 6**
  - Luger 26 → 23,9: **4 → 5**
  - MP40 20: **5 → 6**
  - Uzi 18: **6 → 7**
  - Suomi 19: **6 → 6**
  - MP5 21: **5 → 6**
  - PPSh 18: **6 → 7**
  - Pistolkniv/PrintPistol 30: **4 → 4**
  - 5,56 (HK416, G36, Minimi) 30 → 27,6: **4 → 4**

  Ingen våpen får mer enn +1 treff, og det er innenfor planen. Herdet glass knuses av første kule, så i praksis er det bare én kule i en salve som blir svekket. Laminert rekkverk holder i omtrent 6 kuler (18 per kule mot 100), og de kulene blir svekket hver gang.
- **E gjør aldri to ting: riktig i koden.**
  - Enhanced Input sender `IA_Interact Started` til alle som har bundet den (`EnhancedPlayerInput.cpp`: ingen oppbruk uten `ShouldConsume`).
  - Interaktivs inndatakomponent er lagt på kontrollerens stabel og kjøres før figurens. Den oppdaterer fokuset ved trykket.
  - Kamps `Interact` spør så `HasFocus`, og begge leser samme `FocusKey` i samme bilde.
  - Interaktivs `KampFocusScore` er samme formel som `UpdateFocusPickup`, så bare den ene vinner. Kamp tømmer sitt hint når Interaktiv har fokus, så hintet vises ikke to steder (høyst 50 ms forsinkelse ved grensen).
  - Terminalen får fortsatt E. Må testes i PIE.
- **Ytelse med LMG-salve mot glassboks: ikke målt med `stat game`** (det krever skjerm, og byggeren har ikke skrevet noe tall).
  - Regnet ut: for hver ekte kule kommer ett ekstra linjespor (`LineTraceMultiByObjectType`, bare Destructible) og ett `AGSMapState::Get` (en `TActorIterator`) per steg på 1/60 s.
  - 20 spillere med LMG (13 skudd/s, kulene lever 2–4 steg innendørs) gir omtrent 15–20 ekstra spor per bilde, godt under 0,1 ms.
  - Hver knust rute sender `OnWalkableChanged` (navnett) og lager skår (felles tak `MaxFlyingBits`). En glassboks med 8 ruter gir 8 oppdateringer, og det er begrenset.
  - `UE_LOG` per ekte kule gjennom glass er greit fordi ruta knuses med en gang (se forslag 2).
- **Lydgruppene:** retting 14–16 er gjort som bestilt, med tre merknader (ingen må rettes nå):
  - **Egne skudd 4 plasser, StopOldest:** hvert skudd spiller `_FP` og `_Mech`. Da lever bare de 2 siste skuddene, og ved 800–1000 skudd/min kuttes hvert skudd etter 120–150 ms. 8 plasser hadde latt klangen i eget skudd bygge seg opp.
  - **Halene 6 plasser, StopOldest, felles for alle skyttere:** en fjern hale kan stoppe din egen nære. `StopFarthestThenOldest` er bedre også her.
  - **`Door_`, `Glass_`, `Keypad_` og `Lock_` deler Impact (12, StopQuietest) med alle kuletreff:** stille dørlyder (−14 dB) blir stoppet først midt i skuddveksling. De bør få en egen gruppe for verden (som FlagRadio).
  - Det er også en liten skjønnhetsfeil: `ActiveEstimate` tar fortsatt Weapon-taket (24) for hale og egne skudd, som nå har egne grupper. Det betyr ingenting, fordi selvtesten bruker motorens tall.

## Andre merknader (ikke krav til denne runden)
2. **Fasadevinduer er ikke knusbare.** «Kuler knuser vinduer» (764) gjelder bare de innvendige glassveggene og rekkverkene i Helgasetr. Fast glass fra byggeren blokkerer `Visibility` og knuses ikke. Andreas har sagt «vinduer kan knuse», så dette bør til Grafikk/Interaktiv.
3. **Interaktivs `ApplyMelee` ved commiten** (`GSMapState.cpp` l. 950–958): glass-sveipet sjekker ikke vegger, så en kniv kan knuse en rute bak en tynn vegg innenfor 1,6 m. Interaktiv retter allerede dette i arbeidsmappen (`WallDist`, ikke committet). Kamps del er riktig.
4. **K3, liten kant:** `bBehindGlass` sammenligner Kamps `Hit.ImpactPoint` (på overflaten av figuren) med Interaktivs `G.Location` (midten av kula på 22 cm). En figur som står innenfor ca. 22 cm *foran* ruta, sett fra den som slår, regnes som «bak glass». I praksis holder kapselen (34 cm) folk lenger unna, så det gjør nesten aldri noe. Rent: sammenlign `Hit.Distance` med `G.Distance`.
5. **Commit-hygiene i felles arbeidsmappe:** Kamps IMPROVEMENTS 760–774 kom med i Spillbarhets commit `0d7cdd1`, og Spillbarhets 7b-del (933–941) kom med i denne commiten. Det finnes ingen duplikater i HEAD, men commitene inneholder andre lags linjer.

## Andreas' faste regel
- **Forbedret (gammelt):** kuletreff (fart og skade gjennom glass, skudd i dører og låser), nærkamp (stoppes av glass), eksplosjoner (påvirker kartet), bruk-tasten (én ting per trykk), armene skjules ett sted, oppløsningen beholdes, beinboksen og lydgruppene.
- **Nytt:** glass som knuses av kuler, varige kulehull, granater som blåser opp dører, dørlyder og testen `gs.Test.Glasskudd`.
- Regelen er oppfylt.

## Forslag til neste runde
1. Rett funn 1 (AG3 +70) og kjør `gs.FP.ReachTest` med tallene i rapporten. Ta lydmerknadene samtidig: egne skudd 8 plasser, halene «fjernest først» og en egen gruppe for verden for dør, glass og lås.
2. Hold på `AGSMapState` i `UGSBallistics` (`TWeakObjectPtr`, hentes én gang) i stedet for `TActorIterator` per kulesteg, og sett glass-loggen i `Step` til `Verbose`.
3. Bestilling til Grafikk/Interaktiv: fasadevinduer som kan knuses (dagens faste glass blokkerer `Visibility`). Bestilling til Interaktiv: glass skal ikke knuses av en eksplosjon gjennom betongvegger. I dag knuses alt herdet glass innenfor 2 × radius uten linjesjekk (`ApplyBlast`, l. 903–918). Det er urealistisk for en håndgranat på 12–16 m.

## Lærdom
- En retting som skal få et målt tall under en grense er ikke ferdig før målingen *etter* endringen står i rapporten. «RETTET» uten tall etter endringen er ikke godt nok.
- I felles arbeidsmappe: legg bare til egne linjer i delte filer (`git add -p` på IMPROVEMENTS.md, bestillinger.md), ellers havner andre lags arbeid i feil commit.

---

# Eldre vurdering under samme filnavn (runde 13 i gammel nummerering, b1eb305)

Styring del 2 (c10df95): **GODKJENT**
Runde 13 (b1eb305): **GODKJENT**

Kontrollert ved å lese commitene (`git show`), ikke arbeidsmappen. Bygget er ikke kjørt på nytt (byggeren: «Result: Succeeded»).
Motorkoden er lest der det gjaldt (`PlayerController.cpp`: `Destroyed`, `PawnLeavingGame`, `OnNetCleanup`; `Controller.cpp`: `PawnPendingDestroy`, `Destroyed` → `Logout`).

---

## Del 1 – Hasteretting for styringen, del 2 (c10df95)

### Det som ble sjekket
- **Kan vakten låse styringen for godt?** Nei. `IsGameInputBlocked()` = `bMenuOpen || GSMenu::IsModal(PC)`.
  - `GSMenu::IsModal` krever at menyen faktisk er tegnet de siste 30 bildene, at tastekroken virker og at musepekeren er på
    (`IsModalState`). Hvis GSMenu ikke tegnes, slipper vakten etter 30 bilder (under et halvt sekund).
  - `bMenuOpen` (Kamp-menyen) settes bare av `ToggleMenu` og nulles av `CloseMenu`. Når Kamp-menyen er åpen, slipper GSMenu-kroken
    Esc/F10/P videre (`!PC->bMenuOpen`), så `IA_Menu` → `ToggleMenu` → `CloseMenu` lukker den alltid, også i editorens Play (F10/P).
    `SyncModal` lukker Kamp-menyen når GSMenu åpnes, så de to står aldri åpne samtidig.
  - Etter gjenoppliving: samme kontroller, `bMenuOpen` står, `AcknowledgePossession` sperrer gange og blikk igjen, og `CloseMenu`
    bruker `ResetIgnore…`, så telleren for «ignorer input» kan ikke hope seg opp. Ny kamp/ny kontroller: `bMenuOpen` starter som false.
  - Død: input-funksjonene returnerer uansett på `bDead`. PC-terminal: terminalen er ikke «modal», så vakten slår ikke inn;
    P/Esc/F10 på terminalen går til `ToggleMenu` → `CloseTerminal` (som Esc før).
  - Uten kontroller (`GetController()` null under possession) gir vakten false – aldri sperre.
- **Slipp-handlinger:** `StopFire`, `StopAim`, `StopSprint`, `StopJump` har ingen vakt. `SyncModal` kaller `FlushPressedKeys`,
  som sender slipp for tastene man holder. Ingenting henger.
- **Stjeler P en tast?** P var ikke brukt av noen handling. `IA_Menu` er en fast handling (`IsFixedAction`), så P (og F10) blir
  «reservert» i tastebyttet og kan ikke legges på noe annet; en gammel lagret binding til P faller tilbake til standard
  (`ApplySavedKeys`). I navne- og adressefeltet går P til teksten (`S.Focus == NAME_None`-sjekken), og i «trykk ny tast» får man
  bare beskjeden om at tasten er satt av. Ingen av ønskene i `andreas.md` (lene Q/E, ligge ned, ping, lommelykt) trenger P.
- **Lagpanelet og WASD:** tasten lukker panelet og går videre til spillet (`return false`); `FlushPressedKeys` i `SyncModal`
  skjer før tasten behandles, så gangen starter med en gang. Panelet åpnes ikke igjen av seg selv i samme lobby (`bTeamShownThisLobby`).
- **Skalert Esc-meny:** knapper, tekst og treffbokser bruker samme mål – klikk treffer det som tegnes.

### Funn (ingen blokkerer)
1. **Lav – `GSPlayerController::SetupInputComponent`:** `IA_Inventory` (I), `IA_Ready` (Enter) og `IA_Scoreboard` (Tab) er ikke
   sperret bak Kamp-menyen. Enter i lobbyen kan slå «klar» av/på mens Kamp-menyen står åpen. Legg samme vakt i `ToggleInventory`/`ToggleReady`.
2. **Lav – `GSCharacter.cpp`, `GSCharInputBlocked`:** `static bool bWasBlocked` deles av alle figurer i samme prosess (PIE med flere
   spillere), så loggen kan si feil om hvem som ble sperret. Bare logging, ingen feil i spillet.

---

## Del 2 – Runde 13 (b1eb305), plan `ko/plan-07.md`

### «Ferdig når», punkt for punkt
| Krav | Status |
|---|---|
| Bygger | Ja (byggerens bygg). |
| Mageskudd → «Du blør kraftig», ca. 2 HP/s, dråper hos begge, sår på kroppen | Ja i koden. AK-47 i magen 31 × 0,065 = 2,0 HP/s ≥ 1,2 → «Du blør kraftig». `BleedLevel` replikeres til alle, sporet tegnes på alle maskiner som tegner, såret legges i `MulticastWoundFX` (alle). |
| Bandasje stopper, plaster bare små sår, pille ingenting | Ja. Id-ene `Plaster`/`Bandage`/`Medkit`/`SurgeryKit` finnes i `catalog.py`. Ingen «full helse»-sperre hindrer bandasje. |
| Blør i hjel: «Skytter [AK-47 (blødning)] Offer», pøl, dødskort fra skuddstedet, ikke dunk per dråpe, hjerteslag | Ja. `KillerPawn` nulles hvis skytteren har ny kropp; `BleedFrom` brukes for avstand/sted. `OnRep_Health` hopper over dunk for fall ≤ 3 HP mens man blør. |
| Brannammo-drap til skytteren, «(brann)» | Ja (`BurnInstigator`/`BurnWeapon`). Matforgiftning heter nå matforgiftning. |
| Frakobling midt i kamp: kropp og utstyr blir liggende, skytter siste 10 s får drapet, ingen krasj; i lobbyen som før | Ja, se under. |
| Esc: «Hovedmeny», Avslutt spør først, Grafikk riktig nivå og oppløsning beholdes | Ja (med forbehold i funn 1). |
| «Annen adresse» borte, hint med egne taster | Ja. |

### Nettverk
- **Vert/klient/sen klient:** `BleedLevel` er `Replicated` til alle og endres bare når trinnet (1/32 HP/s) endres – ca. hvert 1,6 s under
  størkning. Sen klient får `BleedLevel` og ser sporet. Sår fra før han kom inn ser han ikke (multicast) – bare kosmetikk.
  Helse-teksten og hjerteslaget bruker replikert verdi hos eieren.
- **Ikke drap i lobby/etter kampslutt:** `TickBleeding` nuller `Bleeding`, `BleedAccum` og `BleedLevel` hver gang
  `DamageEnabled()` er false (bare `Playing` er true: lobby, nedtelling og `Ended` er dekket). `TickBleeding` kjøres bare med
  `HasAuthority() && !bDead`, og ikke hvis `TickSurvival` drepte samme bilde.
- **`PawnLeavingGame`:** motoren kaller den fra `APlayerController::Destroyed` bare når `Player == NULL` på serveren (forbindelsen er
  borte, `OnNetCleanup`). Verten har en lokal spiller og går aldri hit. `Die` kjøres mens kontrolleren ennå har `PlayerState`, HUD og
  kamera; `VictimPC` nulles (`bLeftGame`), så ingen dødskamera-RPC. `DetachFromControllerPendingDestroy` →
  `AController::PawnPendingDestroy` → `UnPossess` + `NAME_Inactive` (timeren den setter ryddes av `EndPlay` rett etter).
  Så `AController::Destroyed` → `Logout` med pawn = null, som Spillbarhets `Logout` (abb7780) tåler; flagget er allerede sluppet av
  `DropEverything`. Gjenopplivings-timeren i `OnPlayerKilled` bruker svak peker → ingen krasj. Under PIE-stopp (`bIsTearingDown`)
  og utenfor `Playing` brukes motorens vanlige vei.
- **Drap til riktig person:** åpent sår med gyldig `BleedInstigator` først, ellers den som skadet ham siste 10 s; egen kontroller → ingen.
  Lagkamerat → `OnPlayerKilled` gir ingen poeng (som før). Uten skytter skrives ingen «Verden»-linje, bare Spillbarhets «forlot kampen».

### Ytelse med 20 spillere
- **Blodspor:** egen liste, høyst 90 dråper per maskin (eldste forsvinner), bare innen 40 m fra eget kamera, stains tikker 2 ganger i
  sekundet når de er ferdige å spre seg. Utregnet: brystskudd fra AK (1,36 HP/s) og gange gir ca. 2 dråper/s → 90 dråper ≈ 45 s / 190 m spor.
  Verste tilfelle (4 HP/s, sprint): ca. 8 dråper/s per kilde. Taket holder også med mange som blør; sporet blir bare kortere.
- **Sår:** høyst 8 per kropp, ingen kollisjon, skygge eller tick, 30 m cull. Med 20 levende og opptil ca. 20 lik: ≤ 320 små plan.
  Hvert sår har sitt eget MID (ett tegnekall hvert) – greit, men kan deles (forslag 3).
- Ingenting nytt kjøres hver frame uten grunn: `TickBloodTrail` bare når `BleedLevel > 0`, `TickBleeding` returnerer tidlig uten blødning.

### Esc-menyen i editorens Play
I normal drift fanger GSMenu-kroken F10/P og åpner hovedmenyen selv, og den fanger også klikk på «Hovedmeny» (id `Host`) og
«Avslutt» i Kamp-menyen. Begge veier gjør det samme. Grafikk, oppløsning, strålesporing og musfølsomhet går gjennom HUD-ens
treffbokser og `MenuClick` som før.

### Funn
1. **Middels (bør rettes neste runde) – `GSPlayerController::MenuClick`, «Hovedmeny» og «Quit»:** Kamp-menyen vises nesten bare når
   GSMenu ikke kan ta over (tastekroken er ikke på). I akkurat det tilfellet lukker GSMenu seg selv igjen neste bilde
   («ingen tastekrok»). Da gjør «Hovedmeny» og «Avslutt spillet» ingenting. Før virket Avslutt alltid (`quit`).
   Rett: etter `GSMenu::AskQuit(this)`, hvis `!GSMenu::IsModal(this)`, bruk `ConsoleCommand(TEXT("quit"))` som før. Og etter `OpenMain`,
   hvis den ikke ble modal, la Kamp-menyen stå åpen (ikke `CloseMenu` først).
2. **Lav – `AGSCharacter::AddBodyWound`:** kjøres for hvert treff på alle maskiner uansett avstand (opptil 8 `GetClosestPointOnPhysicsAsset`
   + ny komponent per treff). Billig nok nå, men en grense på ca. 80 m fra kameraet (som dråpene har 40 m) ville spart arbeid i store kamper.
3. **Lav – balanse:** to brysttreff fra AK-47 (68 HP, han overlever med 32) gir 2,72 HP/s og ca. 185 HP blødning uten bandasje → død på
   ca. 13 s. Realistisk og etter planen, men bandasje er «Uncommon». Følg med i testene; `GSBleed::Clot` er knappen å skru på.
4. **Lav – prosess:** IMPROVEMENTS 500–515 ble ikke med i b1eb305, men ligger i Spillbarhets commit abb7780. Innholdet er riktig;
   dirigenten bør stage `IMPROVEMENTS.md` med Kamps commit.

### Andreas' faste regel
- **Forbedret:** bandasje/plaster/skrin stopper blødning (502), hjerteslag følger blodtap (510), blodpøl også ved sen blødningsdød (509),
  drapet og dødskortet ved blødning/brann (506–508), Esc-menyen og grafikknivået (513–514), egne taster i hint og ammoesker (515).
- **Nytt:** blødning (500, 501, 511), blodspor (503, 504), sår på kroppen (505), død ved frakobling (512).
Begge deler er oppfylt.

---

## Forslag til neste runde
1. Rett funn 1 i runde 13 (Avslutt/Hovedmeny må virke også når GSMenu ikke kan ta over) – lite og viktig for at Andreas aldri sitter fast.
2. Sperr `ToggleInventory`/`ToggleReady` bak åpne menyer (styring del 2, funn 1).
3. Del ett MID for sårene per kropp (eller ett felles) og legg en avstandsgrense på `AddBodyWound`.

## Lærdom
- En reservevei (fallback) må ikke sende brukeren tilbake til systemet som nettopp feilet: sjekk at knappene i en reservemeny virker
  også når hovedmenyen ikke kan vises.
- Når en commit fra et lag inneholder et annet lags logg (IMPROVEMENTS), sjekker kontrolløren `HEAD` i tillegg til commiten før det
  meldes som manglende.
