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
