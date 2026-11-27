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
- Runde 3 (`f19d059`): blod i verden – sky, sprut, dråper, pøl, kropper ligger i 90 s og faller slik kula traff.
- Feilretting 2 (`2f04300`): musehjul, prosjektilretning, miner hos sene klienter, dødskort, kast fra klient.
- Runde 4 (`60a516c`): kroppssoner – sju treffbokser, bein- og armskade, vest bare på overkroppen, V1 og V3.
- Lag i kamp, første del (`2f6f46b`): ingen venneild, flaggbæreren går på 85 % fart.
- Runde 11 (`86ecf6c`): lagfarger og armbind, CTF-poengtavle og sluttskjerm, navn på lagkamerater, flagg-radio,
  PC-kart for det nye området, 60 fps-tak.
- Runde 5a (`a28c23e`): ballistikk-kjernen – kulefart, luftmotstand og fall, skadefall, serverens spredning, framspoling.
- Runde 12 = 5b (`4d74e7c`, kontrolleres): upålitelig ild for automatvåpen, flygende lysspor, overlydssmell og sus,
  holdemerker.

## I arbeid / planlagt
- **Runde 14a** (`ko/plan-08.md`, revidert etter forhåndssjekk): kroppen – `UGSBodyAnim` i C++ (sekvenser fra
  Mannequin-pakken, rotlås, ryggbøy klemt ±15°, huk/glid med knebøy-IK, hånd-IK for andre spillere), `FPBody` uten
  hode og armer (dagens førstepersonsarmer beholdes), hel egen skygge, tilbakefall. + rettingene fra runde 13.
- **Runde 14b** (`ko/plan-08b.md`): hånd-IK på egen kropp (armene erstatter `SM_FPArm`), rekkevidde, fot-IK i trapper,
  armer skjult i kikkert, ev. delt førstepersonsskalering.
- **Lydrunde 1** (`ko/plan-11.md`, **bygges rett etter 14b, før plan-09**): ekte innspilte lyder (nedlastingsliste til Andreas:
  Sonniss GDC 2019/2020/2024/2026, Snake's Authentic Gun Sounds, Kenney – venter på hans ja) og en lydmotor i C++: varianter,
  skudd i lag med romhale, okklusjon, romklang målt automatisk, miks der fottrinn og flagg alltid høres, eksplosjonslyd
  (i dag stumme), avstandsforsinkelse. Virker med de syntetiske lydene til filene kommer.
- **Runde 15** (`ko/plan-09.md`, skisse): animasjoner på kroppen – hopp/landing, glid, klatring med hender på kanten,
  slag, rekyl og lading på kroppen, treffrykk, hodebevegelse – uten GASP/Lyra (hovedagenten 27. nov.).
- **Runde 16** (`ko/plan-10.md`, skisse): bruke ting med kroppen (andre ser det) og gradvise skjermeffekter per stoff
  (rød røyk, hvit amfetamin, tunnelsyn …). Venter på katalogsvar fra Spillbarhet (bestilt 27. nov.).
- **Runde 17: V8, tilbakespoling av treffbokser** til skytterens tid, og «sist sett»-punktet for avslørt bærer.
- **Runde 18: N1, nettrelevans** (`IsNetRelevantFor`).
- **Lydrunde 2** (`ko/plan-12.md`, skisse): romklang og romtone per sone i Helgasetr, vegg-for-vegg-okklusjon etter materiale,
  glass/dører/ødeleggelse med ekte lyd (Interaktiv spiller, Kamp leverer), kroppslyder, radio. Etter plan-09, når Grafikks
  `SoundZones()`/`PM_GS_*` og Interaktivs `OnBroken` finnes. Rekkefølge nå: 14b → lyd 1 → 15 (plan-09) → 16 (plan-10) → lyd 2 → V8 → N1.
- Runde 13 (`ko/plan-07.md`): **godkjent** (`b1eb305`) – blødning, blodspor, sår, frakobling, Esc-meny.

## Neste ideer (backlog, viktigst øverst)
1. **[Spor 8] Lag i kamp (CTF), resten** – lagfeltene er levert; venneild og bærerfart er inne, lagfarger og
   CTF-HUD bygges nå. Gjenstår det som står under og ikke er tatt der. Gammelt: `TakeDamage` får én lagsjekk øverst (venneild av = ingen skade, blod, lemskade eller
   treffmarkør på lagkamerater), eksplosjoner og miner følger samme regel, drapskort og drapslogg viser lagfarge.
   Nytt: lagkamerater vises med et lite, dempet navn over hodet på nært hold (ingen markør gjennom vegger),
   flagget henger synlig på ryggen til bæreren og bæreren går tregere, egen lyd og retningskile i HUD når flagget
   tas, mistes og erobres. Kan ikke planlegges i detalj før feltene finnes; er de ikke levert, går punkt 2 først.
   (Ikke levert ved runde 5; derfor ble runde 5 ballistikk.)
2. **[Spor 2] Ballistikk del 2: gjennomslag og ødeleggelse**. Nytt: kuler går gjennom glass, tre, gips og kropper
   etter kaliber, med tapt fart (bygger på `UGSBallistics::Step` fra runde 5: kula fortsetter med lavere fart i stedet
   for å stoppe); kulehull som blir værende, splinter, dører som kan skytes opp (Andreas liker
   glass som knuser). Gammelt: nedslagseffekten får form etter materialet. Trenger materialmerking fra Grafikk
   (bestilt); uten den er alt betong. V8 er tatt ut herfra og er egen runde rett etter 5b (se over). La klientens
   nedslag følge serverens treff (fjerner restjukset: beste av tre mønstre og 15 % kjegle).
3. ~~[Spor 1] Blødning og blodspor~~ – tatt i runde 13 (plan-07). Gjenstår etterpå: tourniquet/trykkbandasje som egen
   gjenstand (Spillbarhet, `Kind = "tourniquet"` er avtalt), blod på egne hender, blødning som gjør deg svakere (utholdenhet).
4. **[Spor 4] Bruke ting med animasjon** (Andreas' ønske): sprøyte, pille, bandasje, spise/drikke, ta på vest og
   hjelm – realistisk lengde, kan avbrytes, sårbar imens. Gammelt: `UpdateItemUse`. Og mer detaljert lading per
   våpentype (taktisk mot tom lading, magasin som faller, +1 i kammer, skudd-for-skudd som kan avbrytes – R8).
5. **[Spor 4] Lene seg (taster avtales med Spillbarhet) og ligge ned**. Nytt: lene rundt hjørner, ligge. Gammelt:
   treffsonene får en tredje stilling (ligge), tofot/huk demper rekyl (R10).
6. **[Spor 4] Kropp med ordentlige animasjoner**. Gammelt: mannekengen andre ser har ingen huke-, sikte- eller
   haltebevegelse og holder ikke våpenet. Nytt: animasjoner fra Game Animation Sample/Lyra, treffbokser festet til
   beina på skjelettet (da stemmer løpende bein og armer også), halting som synes for andre.
7. **[Spor 3] Ekte lyd på avstand** (R11) – smell forsinket og fjernlyd tas i lydrunde 1 (plan-11); demper-delen (V6) står. Nytt: smellet forsinkes med avstanden (343 m/s), lengre rekkevidde med
   demping (overlydssmell og sus når en kule passerer kommer i 5b). Gammelt: lyddempet våpen høres lenger enn 40 m, demper
   trekker ikke skade, hagle med demper (V6).
8. **[Spor 1] Sår som synes, resten**. Blodflekk på kroppen der kula traff kommer i runde 13. Gjenstår: blod på egne
   hender når du er skadet, lik kan treffes og blør (R15), skitt og blod som setter seg på klærne (Andreas del 5).
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
14. **[Spor 3] Kroppen høres** (tas i lydrunde 2, plan-12): våt trefflyd, stønn, gisping ved lavt liv, pust etter sprint.
15. **[Spor 1] Eksplosjoner som ødelegger**: sot, trykkbølge, øresus, splinter som egne treff per sone.
16. **[Spor 2] Lommelykt og nattsikte montert på våpen; vind på lange skudd** (leser vær fra Grafikk; vinden legges
    som en ekstra kraft i `UGSBallistics::Step`).
17. **[Spor 7] Åpne funn i egne filer**: kast som starter bak vegg (funn 9, `ThrowItem`; skudd rettes i 5a), spesialammo som varer evig (V4),
    lyddemper på alt (V12), røyk/ild for sene spillere (funn 10).

### Strøket (arkade – skal ikke bygges)
Medaljer («DOBBELTDRAP» osv.), drapsserier og serie-teller, medaljelyder, drapsbekreftelse i stor tekst, dusør på
lederen, lysstråle i sjeldenhetsfarge på loot, glødende omriss rundt drapsmannen, spawnbeskyttelse med synlig skjær,
rundeslutt med «høydepunkter». Statistikk kan vises nøkternt på resultattavla etter kampen, ikke underveis.

## Endringer i retningen
- 2026-11-27 (plan-11/12): **Lyd før animasjonene i plan-09.** Andreas vil ha både «mye bedre lydeffekter» og «bedre/flere
  animasjoner». Lyd går først fordi (1) den er det svakeste i spillet målt mot «superrealistisk»: alle 48 lydene er syntetisert
  med numpy, og eksplosjoner har ingen lyd i det hele tatt; (2) den merkes i hvert sekund av spillet (hvert skudd og fottrinn);
  (3) plan-09 kan uansett ikke skrives ferdig før 14b er kontrollert, og plan-10 venter på katalogsvar; (4) lydrunden trenger
  ingenting fra andre lag (motoren måler rommene selv) og rører andre funksjoner enn plan-09 (bare lydlinjene i glid/landing/lading);
  (5) nedlastingen må godkjennes av Andreas, så spørsmålet bør gå nå mens 14b bygges – planen virker også om svaret er nei.
  Lydrunde 2 (soner, materialer, glass/dører) venter på Grafikk og Interaktiv og kommer etter plan-09. Backlog 7 (ekte lyd på
  avstand) og 14 (kroppen høres) er tatt inn i lydrundene. Ekte filer: bare kilder der lisensen tillater spill uten krav (CC0,
  Sonniss GDC-lisensen, Snake), Sonniss-filer holdes utenfor GitHub.
- 2026-11-27 (runde 14–16): **Kroppen før V8.** Andreas' liste etter det nye Helgasetr-kartet har «bedre førsteperson:
  hele kroppen synlig, bedre animasjoner» øverst og «ting med animasjoner og skjermeffekter» som punkt 3. Alt det
  trenger en kropp med hender som kan styres fra kode, så runde 14 er grunnmuren (ekte førsteperson, `UGSBodyAnim`,
  IK; delt i 14a/14b etter forhåndssjekken), 15 animasjonene og 16 bruk av ting/skjermeffekter. Funnet som avgjorde metoden: prosjektet har bare
  `ABP_Unarmed`, men 100+ sekvenser for gevær/pistol/ubevæpnet ligger ubrukt i `Content/Characters` – de kan spilles
  fra C++ uten Blueprint. Det som mangler (huk, glid, klatring) lages i kode; GASP/Lyra hentes ikke foreløpig
  (hovedagenten), så plan-09 bygges uten dem. V8 flyttes til runde 17, N1 til 18. Backlog 6 (kropp med animasjoner), 4 og 10 er
  dermed tatt inn i rundene 14–16.
- 2026-11-26 (runde 13): **Blødning før V8.** Hovedagenten foreslo blødning + de små åpne bestillingene; det er det
  Andreas merker mest («veldig groteskt», ønsket om skader som må bandasjeres) og trenger ingenting fra andre lag
  (bandasjen finnes; tourniquet kan komme senere uten mer kode). V8 blir runde 14 sammen med «sist sett»-punktet på
  kartet (venter på Spillbarhets plan-04), N1 (nettrelevans) runde 15. Blødningstallene er satt så ett treff aldri
  dreper raskere enn før, men en ubandasjert mageskade dreper på under ett minutt.
- 2026-11-26 (plan 5 delt): **Ballistikk del 1 er delt i 5a og 5b** etter kontrollørens forhåndssjekk (BØR ENDRES).
  5a er kjernen med rettelsene: skuddnummeret klienten velger kan bare hoppe 3 (ellers trekker serveren selv),
  klientens kjegle sendes med og klemmes til ±15 %, skadefallet begynner først etter innskytingsavstanden (drap-
  tersklene i katalogen står), kuler ryddes når de forlater banen, serverens kule spoles fram med halve pingen, og
  `ServerFire` forblir pålitelig. 5b tar det spilleren ser og hører (lysspor, smell, holdemerker) og upålitelig ild
  bare for automatvåpen – ett dyrt skudd skal aldri forsvinne. V8 (tilbakespoling) er løftet fra backlog til runden
  rett etter 5b, fordi flytid + ping + små treffbokser ellers gir verten for stor fordel.
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
