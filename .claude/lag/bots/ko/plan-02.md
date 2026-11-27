# Plan 07 – runde 7 (lag Spillbarhet) – SKISSE: bots med taktikk

**Status: skisse.** Skrives ferdig når plan-06 er bygget og kontrollert (logg fra selvtesten, «sitter fast»-linjer og
ytelsestall leses først). IMPROVEMENTS-numre deles ut av dirigenten da. Bygger på `AGSBotController`, `AGSBotTeam`,
via- og forsvarspunktene og ferdighetstabellen fra plan-06.

## Mål
Botene spiller som et lag: de dekker flaggbæreren, flankerer, holder dører, kaster granater og tar opp utstyr – så en
bot-kamp på Vanskelig/Ekspert føles som mot dyktige folk.

## Innhold (rekkefølge etter hva spilleren merker)
1. **Dører** (krever Grafikks `AGSDoor` fra Helgasetr runde 2): stien krysser en lukket dør → gå til den, bruk den (samme
   handling som spilleren), vent til den er åpen; låst kodelås → velg annen rute (dørens navnområde `NavArea_Null` når låst,
   eller boten markerer den for seg selv i 60 s). Forsvarere lukker dører inn til baserommet bak seg (Vanskelig+).
2. **Eskorte og flagglek:** 1–2 eskorter går foran og bak bæreren, sikter mot sidegangene; bæreren venter i dekning når
   eskorten kjemper. Angripere går i par (én dekker, én går) i rom med flere innganger.
3. **Flanke:** når to angrep på rad har dødd på samme via-punkt, velg det motsatte (side/etasje); Ekspert koordinerer
   to grupper på ulike ruter som kommer fram samtidig (vent på hverandre ved siste via-punkt, maks 10 s).
4. **Undertrykkende ild og forhånds-sikte:** skyt 1–2 s mot stedet en fiende forsvant (Vanskelig+), sikt mot dører og
   hjørner man vet fienden kom fra; «klarere» rom ved å sjekke hjørner (blikkpunkter fra navnettets kanter).
5. **Granater og utstyr** (Kamps kast-API, bestilles da): splint mot fiender bak dekning i rom, røyk for flaggbæreren over
   åpne strekk (atriet, kantina), blend før inngang til baserommet. Med tak per bot og kamp.
6. **Loot:** i rolige perioder og på vei: plukk opp bedre våpen, ammo, vest/hjelm og medisin innen 8 m av stien
   (`AGSPickup`-innhold vurderes med katalogverdiene); aldri lenger omvei enn 10 % av stien.
7. **Nærkamp og «ta flagget tilbake i nærkamp»** (Andreas' ønske, regel fra vår backlog) når fienden er innen 2 m og ladingen pågår.
8. **Bots i online-kamper:** «Fyll opp lagene med bots» som vertsvalg (lagene fylles til valgt størrelse, en bot går ut
   når et menneske kommer inn). Alt i plan-06 er serverbasert, så det er ett valg i `GSMenu` og telling i `GSGameMode`.
9. **Finjustering per vanskelighetsgrad** fra Andreas' testing: tallene i ferdighetstabellen ligger i én fil og logges ved
   kampstart, så de kan justeres uten å lete.

## Avhengigheter
- Kamp: K1 (`IsLocalHuman`, `friend`) må være levert; kast-API for granater (`ThrowItem` er beskyttet – friend dekker det).
- Grafikk: `AGSDoor` (runde 2), G2 (dører påvirker ikke navnettet), G3 (`NotifyMapChanged` ved sprengte vegger).
