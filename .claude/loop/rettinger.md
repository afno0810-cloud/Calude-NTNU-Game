# Rettinger lag Kamp skal ta før neste plan

Fra kontroll av feilretting 1 (.claude/loop/kontroll/feilretting-01.md). Byggeren tar disse først, i egen commit.

1. HØY: prosjektiler flyr i feil retning (kast mot +Y går mot −X). `Movement->bInitialVelocityInLocalSpace = false;` i konstruktøren i `GSProjectile.cpp`.
2. MIDDELS: mine/C4 som ligger stille faller til gulvet hos klienter som kommer inn senere (`BeginPlay` slår tyngdekraft på igjen etter `OnRep_Flight`).
3. LAV/MIDDELS: dødskort/dødskamera bruker drapsmannens nye kropp etter respawn; `Shooter` i `TakeDamage` er gammel kropp. Lagre kasterens posisjon ved kast, bruk pawnen bare hvis den er samme kropp.
4. LAV: velg én regel for eierens egen mine etter respawn (sammenlign mot `OwnerController`), og sett et tak på antall miner per spiller.
