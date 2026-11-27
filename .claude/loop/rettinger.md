# Rettinger lag Kamp skal ta før neste plan

Fra kontroll av feilretting 1 (.claude/loop/kontroll/feilretting-01.md). Byggeren tar disse først, i egen commit.

1. **RETTET** – HØY: prosjektiler flyr i feil retning (kast mot +Y går mot −X). `Movement->bInitialVelocityInLocalSpace = false;` i konstruktøren i `GSProjectile.cpp`.
2. **RETTET** – MIDDELS: mine/C4 som ligger stille faller til gulvet hos klienter som kommer inn senere (`BeginPlay` slår tyngdekraft på igjen etter `OnRep_Flight`).
3. **RETTET** – LAV/MIDDELS: dødskort/dødskamera bruker drapsmannens nye kropp etter respawn; `Shooter` i `TakeDamage` er gammel kropp. Lagre kasterens posisjon ved kast, bruk pawnen bare hvis den er samme kropp.
4. **RETTET** – LAV: velg én regel for eierens egen mine etter respawn (sammenlign mot `OwnerController`), og sett et tak på antall miner per spiller.
5. **RETTET** – FRA ANDREAS' TESTING: musehjulet virker ikke for å bla i inventaret (og bytte våpen). Finn hvorfor i input-oppsettet (`GSCharacter`/`GSPlayerController`/Enhanced Input), og få scroll til å virke både med inventaret åpent og for våpenbytte. Høy prioritet.
6. **RETTET** – HØY (spillbarhet-feiljakt G1): granater, miner og nærkampslag fra klienter går alltid vannrett – serveren bruker kameraretning som bare oppdateres for lokal spiller. Send sikteretning til serveren.

Alle seks RETTET av bygger (Kamp) 26. nov., IMPROVEMENTS 376–385. G1 (punkt 6): kast og nærkamp bruker GetBaseAimRotation() på serveren (kontrollerens sikteretning, som serveren alt har), så ingen ny RPC trengs. Bygget: OK.

## Nye (fra kontroll av feilretting 2) – tas sammen med neste runde
7. **RETTET** – Styreflate/jevn rulling: `Scroll` må samle utslag til ±1 før ett trinn (Andreas tester på laptop).
8. **RETTET** – I terminalen skal hjul opp bla oppover (i dag sender begge retninger «Q»).
9. Blør man i hjel etter en mine, må `Die` få med prosjektilet så dødskortet ikke bruker kasterens nye kropp. (Ikke tatt i runde 4: den eneste `Die` utenom `TakeDamage` er sult/tørst/overdose med `Die(nullptr, …)`, uten drapsmann, så feilen kan ikke skje i dag. Tas når blødning legges inn.)

## Fra kontroll av runde 14b (.claude/loop/kontroll/runde-14b.md) – tas først i neste Kamp-runde
10. Gamle armer vippes synlig/usynlig hvert bilde (`UpdateArms` vs `Tick`). Send `UsesBodyHands()` inn i `UpdateArms`/`UpdateItemUse` så de bare settes ett sted.
11. Én regel for grafikknivå: `ApplyGraphicsLevel` («Grafikk N» og Esc-menyen) skal beholde spillerens egen oppløsning slik hovedmenyen gjør. Skriv IMPROVEMENTS 597 om så den stemmer.
12. Utvid `gs.FP.ReachTest` med sikte opp/ned (60–80°) og sørg for at høyre hånd ikke slipper grepet.
13. Utvid den stående beinboksen (knærne 8 cm utenfor midt i et steg).
