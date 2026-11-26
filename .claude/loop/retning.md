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
8. **Capture the Flag** (eneste spillmodus, Andreas 26. nov.): to lag. Kamp eier lagtilhørighet i treff og HUD,
   synlig flaggbærer, og lyd/retningsvarsel når flagget tas. Reglene og flagget eier lag Spillbarhet.

## Gjort
- Runde 1: dødskamera, drapskort, treffmarkører.
- Runde 2 (`94f1370`, godkjent): å bli truffet – blodkant, retningspiler, rustningslyder, hjerteslag.
- Feilretting 1 (`57493cb`): drap med prosjektil telles, granater flyr riktig hos klienter, miner blir liggende.

## I arbeid / planlagt
- **Runde 3** (`ko/plan-03.md`, bygges): blod i verden – sky og dråper ved såret, sprut bak, pøl under kroppen,
  kropper ligger i 90 s og faller slik kula traff.
- **Rettinger** (`rettinger.md`, byggeren tar dem før runde 4): prosjektilretning, mine hos sene klienter,
  dødskort etter respawn, mineregel, musehjul.
- **Runde 4** (`ko/plan-04.md`): kroppssoner – sju treffbokser med ekte mål, skade etter kroppsdel, beinskade
  (halter, ingen sprint) og armskade (ustøtt sikte, treg lading), vest bare på overkroppen, hjelm ikke i ansiktet.
  Samtidig V1 (sikting til serveren), V3 (skuddtakt), V5 (rakettkasterlyd), kroppen andre ser står på bakken,
  og HUD mister «LAVT LIV», hvitt blink og «HODESKUDD».
- **Runde 5** (`ko/plan-05.md`): ballistikk del 1 – kuler som raske prosjektiler på serveren (ny fil `GSBallistics.*`)
  med kulefart, luftmotstand og fall per kaliber (reservetabell til Spillbarhet leverer våpendata), skadefall over
  avstand, innskutte sikter, holdemerker i kikkerten, overlydssmell når kuler passerer, lysspor bare for sporlys og
  hver femte MG-kule. Samtidig V2 (serveren bestemmer spredningen), V7 (`ServerFire` upålitelig) og funn 9 i `ServerFire`.

## Neste ideer (backlog, viktigst øverst)
1. **[Spor 8] Lag i kamp (CTF)** – kandidat runde 6, så snart lag Spillbarhet har levert lagfeltet (bestilling
   ligger inne). Gammelt: `TakeDamage` får én lagsjekk øverst (venneild av = ingen skade, blod, lemskade eller
   treffmarkør på lagkamerater), eksplosjoner og miner følger samme regel, drapskort og drapslogg viser lagfarge.
   Nytt: lagkamerater vises med et lite, dempet navn over hodet på nært hold (ingen markør gjennom vegger),
   flagget henger synlig på ryggen til bæreren og bæreren går tregere, egen lyd og retningskile i HUD når flagget
   tas, mistes og erobres. Kan ikke planlegges i detalj før feltene finnes; er de ikke levert, går punkt 2 først.
   (Ikke levert ved runde 5; derfor ble runde 5 ballistikk.)
2. **[Spor 2] Ballistikk del 2: gjennomslag og ødeleggelse**. Nytt: kuler går gjennom glass, tre, gips og kropper
   etter kaliber, med tapt fart (bygger på `UGSBallistics::Step` fra runde 5: kula fortsetter med lavere fart i stedet
   for å stoppe); kulehull som blir værende, splinter, dører som kan skytes opp (Andreas liker
   glass som knuser). Gammelt: nedslagseffekten får form etter materialet. Trenger materialmerking fra Grafikk
   (bestilt); uten den er alt betong. Ta samtidig V8 (forsinkelseskompensasjon: spol treffboksene tilbake til
   skytterens tid når kula skytes) og la klientens nedslag følge serverens treff (fjerner restjukset med `SpreadKey`).
3. **[Spor 1] Blødning og blodspor** (sammen med Spillbarhet: Kamp eier skadelogikken, de eier bandasje/
   tourniquet). Nytt: replikert blødning som tapper liv til den stoppes, mest ved mageskudd; dråpespor man kan
   følge. Gammelt: bandasje får sin egentlige oppgave, hjerteslaget følger blodtapet. Bygger på sonene fra runde 4.
4. **[Spor 4] Bruke ting med animasjon** (Andreas' ønske): sprøyte, pille, bandasje, spise/drikke, ta på vest og
   hjelm – realistisk lengde, kan avbrytes, sårbar imens. Gammelt: `UpdateItemUse`. Og mer detaljert lading per
   våpentype (taktisk mot tom lading, magasin som faller, +1 i kammer, skudd-for-skudd som kan avbrytes – R8).
5. **[Spor 4] Lene seg (taster avtales med Spillbarhet) og ligge ned**. Nytt: lene rundt hjørner, ligge. Gammelt:
   treffsonene får en tredje stilling (ligge), tofot/huk demper rekyl (R10).
6. **[Spor 4] Kropp med ordentlige animasjoner**. Gammelt: mannekengen andre ser har ingen huke-, sikte- eller
   haltebevegelse og holder ikke våpenet. Nytt: animasjoner fra Game Animation Sample/Lyra, treffbokser festet til
   beina på skjelettet (da stemmer løpende bein og armer også), halting som synes for andre.
7. **[Spor 3] Ekte lyd på avstand** (R11). Nytt: smellet forsinkes med avstanden (343 m/s), lengre rekkevidde med
   demping (overlydssmell og sus når en kule passerer kom i runde 5). Gammelt: lyddempet våpen høres lenger enn 40 m, demper
   trekker ikke skade, hagle med demper (V6).
8. **[Spor 1] Sår som synes**. Nytt: blodflekker på kroppen der kula traff (per sone), blod på egne hender når du
   er skadet, lik kan treffes og blør (R15). Gammelt: blodmaterialet fra runde 3.
9. **[Spor 7] Bestilling fra Spillbarhet** (`lag/spillbarhet/funn.md`): S3 brann-drap går til skytteren og får
    riktig dødsårsak, S5 serveren vet om du sitter ved PC-en og sporing stopper når du går, S10 sluttskjermen
    overlapper poengtavla, F2 spillernavn, B2 sult/tørst med tidlige følger. Tas som små tillegg i rundene over
    der de rører samme funksjon (S3 passer i blødningsrunden, S10/F2 i CTF-runden).
10. **[Spor 4] Klatring med vekt** (Andreas liker klatring og sliding): vinduer, tak, høyere murer med animasjon;
    skli med lyd og kamerabevegelse.
11. **[Spor 2] Rekyl og spredning som på ekte** (R9, R10, V10): liten mekanisk spredning, resten er synlig
    våpenbevegelse; rekyl fra impuls, ingen fast mønster. Ildvelger (R14). Våpenbytte tar tid (V11).
12. **[Spor 1] Drapskamera fra drapsmannens side** (Andreas' godkjente forslag), bygger på dødskameraet.
13. **[Spor 5] Slank HUD videre**: treffmarkør tones ned eller fjernes (blodet viser treffet), ammo vises ved
    «sjekk magasin», valg for HUD-mengde.
14. **[Spor 3] Kroppen høres**: våt trefflyd, stønn, gisping ved lavt liv, pust etter sprint.
15. **[Spor 1] Eksplosjoner som ødelegger**: sot, trykkbølge, øresus, splinter som egne treff per sone.
16. **[Spor 2] Lommelykt og nattsikte montert på våpen; vind på lange skudd** (leser vær fra Grafikk; vinden legges
    som en ekstra kraft i `UGSBallistics::Step`).
17. **[Spor 7] Åpne funn i egne filer**: kast som starter bak vegg (funn 9, `ThrowItem`; skudd ble rettet i runde 5), spesialammo som varer evig (V4),
    lyddemper på alt (V12), røyk/ild for sene spillere (funn 10).

### Strøket (arkade – skal ikke bygges)
Medaljer («DOBBELTDRAP» osv.), drapsserier og serie-teller, medaljelyder, drapsbekreftelse i stor tekst, dusør på
lederen, lysstråle i sjeldenhetsfarge på loot, glødende omriss rundt drapsmannen, spawnbeskyttelse med synlig skjær,
rundeslutt med «høydepunkter». Statistikk kan vises nøkternt på resultattavla etter kampen, ikke underveis.

## Endringer i retningen
- 2026-11-26 (runde 5): **Ballistikk før lag i kamp.** Lagfeltene fra Spillbarhet er ikke levert, så backlogens
  punkt 2 (ballistikk del 1) ble rundens arbeid, med V2 og V7 fra `funn.md`. Kulene flyr i en egen, ny fil
  (`GSBallistics.*`, lag Kamp eier den) slik at gjennomslag (del 2) og vind senere bare endrer ett steg i kulas bane.
  Kulefart ligger i en reservetabell per kaliber til våpendataene kommer. Lysspor er gjort sjeldne (bare sporlys og
  hver femte MG-kule) fordi «ville dette skjedd i virkeligheten?» – vanlige kuler synes ikke. Del 2 får nå også
  V8 (forsinkelseskompensasjon), som blir viktigere når kuler har flytid.
- 2026-11-26 (runde 4): **Capture the Flag er eneste modus** (Andreas). Nytt spor 8, og lag i kamp ligger øverst i
  backlogen; den kan først bygges når lag Spillbarhet har lagfeltet (bestilt). Runde 4 ble kroppssoner
  (hovedagentens prioritering) fordi den ikke trenger noe fra andre lag, og er skrevet slik at lagsjekken senere
  blir én linje. Rekkefølgen etterpå: lag i kamp → ballistikk (fart/fall, så gjennomslag) → blødning. Blødning er
  flyttet bak ballistikk fordi den må samordnes med Spillbarhets gjenstander. Andreas' godkjente forslag (lene,
  ligge, animasjoner for bruk av ting, klatring, drapskamera, våpenlykt, vind) er lagt inn. Fire bestillinger
  sendt (lagfelt, våpendata for ballistikk, medisin, materialmerking av flater).
- 2026-11-26 (runde 3, revidert): **Ny kurs.** Andreas la inn to overordnede mål: superrealistisk og veldig groteskt,
  uten arkade-preg. Visjon, spor og backlog er skrevet om etter det. Den første planen for runde 3 (drapsserier,
  medaljer, medaljelyder) er strøket og erstattet med «blod i verden». Det nøkterne fra den (riktig tekst i
  drapsfeeden, sortering i poengtavla) ligger som punkt 17. Rekkefølge videre: først det spilleren *ser* av skade
  (blod → blødning → kroppsdeler), så ballistikk og lyd, så HUD-slanking når verden gir nok tilbakemelding alene.
- 2026-11-26 (runde 3): Valgte drapsserier/medaljer foran spawnbeskyttelse (samlebånd uten kollisjoner). *Opphevet
  samme dag, se over.*
- 2026-11-26 (runde 2): Fylt ut backlog. Runde 2 tok «å bli truffet» (kontrollørens forslag).
- 2026-11-26: startversjon.
