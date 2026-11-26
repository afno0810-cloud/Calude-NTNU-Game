# Rettinger lag Kamp skal ta før neste plan

Fra kontroll av feilretting 1 (.claude/loop/kontroll/feilretting-01.md). Byggeren tar disse først, i egen commit.

1. **RETTET** – HØY: prosjektiler flyr i feil retning (kast mot +Y går mot −X). `Movement->bInitialVelocityInLocalSpace = false;` i konstruktøren i `GSProjectile.cpp`.
2. **RETTET** – MIDDELS: mine/C4 som ligger stille faller til gulvet hos klienter som kommer inn senere (`BeginPlay` slår tyngdekraft på igjen etter `OnRep_Flight`).
3. **RETTET** – LAV/MIDDELS: dødskort/dødskamera bruker drapsmannens nye kropp etter respawn; `Shooter` i `TakeDamage` er gammel kropp. Lagre kasterens posisjon ved kast, bruk pawnen bare hvis den er samme kropp.
4. **RETTET** – LAV: velg én regel for eierens egen mine etter respawn (sammenlign mot `OwnerController`), og sett et tak på antall miner per spiller.
5. **RETTET** – FRA ANDREAS' TESTING: musehjulet virker ikke for å bla i inventaret (og bytte våpen). Finn hvorfor i input-oppsettet (`GSCharacter`/`GSPlayerController`/Enhanced Input), og få scroll til å virke både med inventaret åpent og for våpenbytte. Høy prioritet.
6. **RETTET** – HØY (spillbarhet-feiljakt G1): granater, miner og nærkampslag fra klienter går alltid vannrett – serveren bruker kameraretning som bare oppdateres for lokal spiller. Send sikteretning til serveren.

Alle seks RETTET av bygger (Kamp) 26. nov., IMPROVEMENTS 376–385. G1 (punkt 6): kast og nærkamp bruker GetBaseAimRotation() på serveren (kontrollerens sikteretning, som serveren alt har), så ingen ny RPC trengs. Bygget: OK.
