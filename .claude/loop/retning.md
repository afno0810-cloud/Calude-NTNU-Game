# Retning for spillet (planleggerens egen visjon)

Denne filen eies av planleggeren. Her bestemmer løkken selv hvilken retning spillet skal utvikles i:
hva som mangler, hva som vil gjøre spillet mye bedre, og i hvilken rekkefølge. Den skal revideres
når løkken lærer noe nytt (minst hver 3. runde). Andreas' ønsker i `andreas.md` er rammen.

## Visjon (kort)
Et **superrealistisk og grotesk** online skytespill på den ekte Gløshaugen. Våpen, kropper, lyd og lys skal
oppføre seg som i virkeligheten: kuler har fart, fall og gjennomslag; smellet kommer etter glimtet; den som blir
truffet blør, halter og dør stygt; kroppen blir liggende i sitt eget blod. Vold skal ha *konsekvens*, ikke belønning.

Prøvestein for hvert valg: **«Ville dette skjedd i virkeligheten?»** Hvis nei, og det ikke trengs for at spillet
skal kunne spilles, skal det ikke inn. Skjermen skal vise verden, ikke et spillbrett.

Ikke arkade: ingen medaljer, drapsserier, poengtekster, dusører, lysstråler på loot, glødende omriss eller
jubellyder. Tilbakemelding skal komme fra verden selv (blod, lyd, kropp, støv), ikke fra tekst og ikoner.

Andreas har veldig høye standarder: heller én ting som ser ekte ut enn fem halve.

## Store spor
1. **Skade og gore**: blod i verden, sår, blødning, skade etter kroppsdel, kropper som blir liggende.
2. **Våpen og ballistikk**: kulefart, kulefall, gjennomslag, rekyl, lading og funksjonsfeil som på ekte.
3. **Lyd**: smell forsinket med avstand, overlydsknall, rom og demping, kropp og pust.
4. **Kropp og bevegelse**: tyngde, utholdenhet, fall, halting, lene seg – ingen «skøyting».
5. **Minimal, ærlig HUD**: mindre tekst og færre markører; informasjon fra verden.
6. **Campus, lys og vær**: ekte Gløshaugen, mørke netter, våte flater, bygg med dører og rom.
7. **Stabilitet, rettferdighet og ytelse** med 4–20 spillere (kontrollørens funn i `funn.md`).

## Gjort
- Runde 1: dødskamera, drapskort, treffmarkører.
- Runde 2 (committet `94f1370`, hos kontrolløren): å bli truffet – blodkant, retningspiler, rustningslyder, hjerteslag.

## I arbeid / planlagt
- **Runde 3** (`ko/plan-03.md`): blod i verden – sky og dråper ved såret, sprut på veggen bak, dråper på gulvet,
  pøl som vokser under kroppen, kropper ligger i 90 s og faller slik kula traff (spor 1).

## Neste ideer (backlog, viktigst øverst)
1. **[Spor 1] Blødning og blodspor** (kandidat runde 4). Nytt: kjøttsår gir blødning (replikert `Bleeding`-nivå) som
   tapper liv til den stoppes med bandasje/plaster; den som blør etterlater dråpespor man kan følge. Gammelt: bandasje
   og plaster får en ekte oppgave (i dag bare +liv), hjerteslaget fra runde 2 følger blodtapet. Bruker `SpawnBloodStain`
   fra runde 3. Rører `TakeDamage` og `CompleteUse`.
2. **[Spor 1] Skade etter kroppsdel**. Nytt: egne treffsoner for hode, overkropp, mage, armer og bein (i dag bare én
   sylinder og én kule). Bein = halter og kan ikke sprinte, arm = dårligere sikte og tregere lading, mage = kraftig
   blødning. Gammelt: skademodellen og hodeskuddregelen. Skinne/smertestillende lindrer.
3. **[Spor 2] Ekte ballistikk**. Nytt: kuler med fart, flytid og kulefall (rask simulering i steg, ikke hitscan) og
   gjennomslag gjennom tynne vegger, dører, glass og kropper etter kaliber. Gammelt: all skyting, kikkertsiktets
   mil-dots får mening, lyssporene følger kulebanen.
4. **[Spor 3] Ekte lyd på avstand**. Nytt: smellet forsinkes etter avstand (343 m/s), overlydsknallet fra kula kommer
   før smellet, kuler som går nær smeller og hviner. Gammelt: skuddlydene dempes og får romklang inne, og dempes
   gjennom vegger.
5. **[Spor 1] Sår som synes**. Nytt: blodflekker på kroppen der kula traff (festet til nærmeste bein), blod på egne
   hender/våpen når du er skadet, blod som renner nedover veggen fra store flekker. Gammelt: blodmaterialet og
   treffene fra runde 3.
6. **[Spor 1+7] Lik med innhold**. Nytt: ransake kroppen (utstyret blir i liket i stedet for å sveve som pickups
   rundt), kroppene ligger hele kampen (billig «frosset» ragdoll, lik posisjon hos alle). Gammelt: `DropEverything`,
   og rustning husker slitasjen sin (funn 1: gratis reparasjon ved bytte).
7. **[Spor 5] Slank HUD**. Gammelt: treffmarkører tones ned til et knapt synlig hakk eller fjernes (blodet viser
   treffet nå), skadepiler blir svakere, ammo-tallet vises bare når du sjekker magasinet. Nytt: «sjekk magasin»
   (hold R: «nesten fullt / halvt / nesten tomt»), valg i innstillinger for HUD-mengde. Gjøres gradvis, og først
   etter at verden selv gir nok tilbakemelding.
8. **[Spor 2] Våpenhåndtering**. Nytt: magasiner som egne ting (bytter du halvfullt mag, beholder du det), kammer
   +1, funksjonsfeil på slitte/våte våpen, våpenet må løftes før sikting etter sprint. Gammelt: rekyl og svai.
9. **[Spor 3] Kroppen høres**. Nytt: våt trefflyd, stønn/skrik når man treffes, gisping ved lavt liv, dødsralling,
   pust etter sprint. Gammelt: `Hit_Body`/`Hit_Head` brukes ikke lenger som 3D-lyd for andre.
10. **[Spor 1] Undertrykkelse uten skjermtriks**. Nytt: støv/fliser/murbiter spruter der kuler slår ned nær deg, og
    rykk i siktet. Gammelt: kulehull får form etter materialet (mur: krater og støv, tre: fliser, metall: gnist og
    bulk, glass: sprekker), og varer lenger.
11. **[Spor 4] Tyngde i bevegelsen**. Nytt: utholdenhet (sprint tar slutt, tung sekk gjør deg treg), fallskade med
    brukne bein, lene seg rundt hjørner. Gammelt: akselerasjon/stopp og hopp blir mindre «svevende».
12. **[Spor 7] Rettferdig spawn uten magi**. Gammelt: `PickSpawnTransform` velger aldri et punkt en fiende ser eller
    står nær, flere spawnpunkter (funn 12). Nytt: man kommer inn fra kanten av campus (gående/dør), ikke «ut av
    lufta» – ingen usårlighets-skjær.
13. **[Spor 6] Dører og glass**. Nytt: dører som åpnes, lukkes og sparkes inn (replikert, med lyd); vinduer som
    knuses av kuler. Gammelt: byggene blir taktiske, kuler går gjennom dørblad (henger sammen med punkt 3).
14. **[Spor 6] Mørke og vær som betyr noe**. Gammelt: nettene blir virkelig mørke (lommelykt/nattbriller trengs),
    regn gir våte, blanke flater. Nytt: regn vasker bort blod fortere, munningsflammen lyser opp rommet om natta,
    pusten synes i kulde.
15. **[Spor 1] Eksplosjoner som ødelegger**. Nytt: svimerker og sot på bakken, trykkbølge-støv, øresus og dempet
    hørsel nær smellet, splinter som egne treff. Gammelt: granatskaden (i dag ren avstandsformel) og funn 2 (drap
    telles ikke hvis kasteren dør først) og funn 4 (granater hakker hos klienter).
16. **[Spor 7] Feilrunde fra `funn.md`**: loot-vekter (funn 3, nesten ingen medisin – viktig når blødning kommer),
    loot gjennom vegger (8), skudd som starter bak vegg (9), miner (5, 14), kampstart (6, 7).
17. **[Spor 5] Nøktern kampinfo** (det brukbare fra den gamle runde 3-planen). Gammelt: drapsfeeden blir en liten,
    dempet logg med riktig tekst («Ola døde [Sult]», ikke «Verden [Sult] Ola»); poengtavla sorterer likt drap etter
    færrest døde og heter «Drap». Nytt: valg for å skjule feeden helt.
18. **[Spor 7] Ytelse**: pickups som ticker (funn 11), måling med 20 spillere, grenser for effekter.
19. **[Spor 6] Innstillinger**: FOV, lyd, mus – lagres mellom økter.
20. **[Spor 6] Hendelser som finnes på ekte**: strømbrudd i et bygg, brannalarm, forsyninger som leveres med varebil.

### Strøket (arkade – skal ikke bygges)
Medaljer («DOBBELTDRAP» osv.), drapsserier og serie-teller, medaljelyder, drapsbekreftelse i stor tekst, dusør på
lederen, lysstråle i sjeldenhetsfarge på loot, glødende omriss rundt drapsmannen, spawnbeskyttelse med synlig skjær,
rundeslutt med «høydepunkter». Statistikk kan vises nøkternt på resultattavla etter kampen, ikke underveis.

## Endringer i retningen
- 2026-11-26 (runde 3, revidert): **Ny kurs.** Andreas la inn to overordnede mål: superrealistisk og veldig groteskt,
  uten arkade-preg. Visjon, spor og backlog er skrevet om etter det. Den første planen for runde 3 (drapsserier,
  medaljer, medaljelyder) er strøket og erstattet med «blod i verden». Det nøkterne fra den (riktig tekst i
  drapsfeeden, sortering i poengtavla) ligger som punkt 17. Rekkefølge videre: først det spilleren *ser* av skade
  (blod → blødning → kroppsdeler), så ballistikk og lyd, så HUD-slanking når verden gir nok tilbakemelding alene.
- 2026-11-26 (runde 3): Valgte drapsserier/medaljer foran spawnbeskyttelse (samlebånd uten kollisjoner). *Opphevet
  samme dag, se over.*
- 2026-11-26 (runde 2): Fylt ut backlog. Runde 2 tok «å bli truffet» (kontrollørens forslag).
- 2026-11-26: startversjon.
