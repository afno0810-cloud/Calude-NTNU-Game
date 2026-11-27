## 2026-11-27 kl. 10.10: rekkefølge for Spillbarhet
Hovedagentens avgjørelse: plan-05 (ny meny med bots/online/test kartet, uten selve botene) bygges nå. Rett etter
at Grafikk har levert råbygget av Helgasetr kommer plan-08 (loot, PC-er, kameraer, dører i Helgasetr) – det følger
Andreas' rekkefølge (kartet komplett med våpen og helse) og går foran botene (plan-06/07).

## 2026-11-27 kl. 09.00: NYTT KART – HELGASETR
Andreas: kartet skal bare være inne i Helgasetr, nytt kart, det gamle skrotes. Lag Grafikk og kart stopper alt
annet og planlegger et innendørs CTF-kart i Helgasetr: to baser i hver sin ende/fløy, flere ruter (korridorer,
trapper, rom), superrealistisk og svært detaljert. Lag Spillbarhet tilpasser spawn, flagg, loot, PC-er/printere og
kamplengde til et lite innendørskart. Lag Kamp sjekker lyd og ballistikk innendørs (romklang, gjennomslag i
innervegger). Kartdata eies fortsatt av Grafikk (`GSCampus.h` e.l.).

## 2026-11-27 kl. 08.50: styring og meny rettet
Menyen virket da Andreas testet som eget spillvindu, og del 2 (F10/P i editoren, input sperret bak menyer) er bygget. Pausen oppheves: lagene fortsetter med planene sine, men alt som fanger input skal prøves i editorens Play før det leveres, og spillbarhet går fortsatt foran nye ting.

## 2026-11-26 kl. 22.55: SPILLBART FØRST
Andreas: «spillet er ikke spillbart i det hele tatt». Ingen nye funksjoner før dette er løst. Alle lag:
feilretting og stabilitet først. Lag Spillbarhet retter menyen og styringen (hastesak). Lag Grafikk tar
bare rettingene fra kontrollen. Lag Kamp bygger ikke runde 13 før menyen og styringen virker; kontrollør
og planlegger kan lese koden og lete etter ting som hindrer at man kan spille (input, spawn, krasj).

# Prioriteringer fra hovedagenten

Alle planleggere leser denne før hver plan. Nyeste vurdering øverst. Innenfor prioriteringen velger
laget selv.

## 2026-11-26 kl. 17.20: Capture the Flag
**Andreas har bestemt: spillet har foreløpig én modus, Capture the Flag.** Dette går foran alt annet.
To lag (f.eks. rødt og blått, 2–10 per lag), hver med en base og et flagg på det mindre, detaljerte
kartet. Ta motstanderens flagg og bær det til egen base mens ditt eget flagg står hjemme. Realistisk:
flagget bæres synlig, bæreren går litt tregere, et mistet flagg blir liggende til det returneres eller
plukkes opp igjen. Første lag til 3 erobringer, eller flest når tiden går ut.

- **Spillbarhet:** CTF-reglene er ny hovedoppgave, rett etter (eller sammen med) hovedmenyen: lag og
  lagvalg i lobbyen, flagg som aktør, erobring/retur, poeng og kampslutt, spawn ved egen base, venneild
  av/på, lagfarger på poengtavla. Knappehjelpen må forklare CTF.
- **Grafikk og kart:** det mindre utsnittet skal velges for CTF: to baser i hver ende (f.eks. to bygg
  som står mot hverandre), flere ruter mellom dem (ute, gjennom bygg, under tak), og et midtområde.
  Basene må være lette å kjenne igjen og forsvarbare. Detaljnivået legges der det spilles.
- **Spillbarhet (tillegg):** tastene skal kunne endres i Innstillinger i hovedmenyen (keybinds), lagres,
  og vises riktig i knappehjelpen. Bruk Enhanced Input sine brukerinnstillinger; trenger du kall fra
  `GSCharacter`/`GSPlayerController`, bestill dem.
- **Andreas liker (fra testing): glass som knuser og klatring.** Behold og bygg videre.
  Kamp: mer ødeleggelse (kulehull som blir værende, splinter, dører som kan skytes/sparkes opp) og
  bedre klatring (vinduer, tak, høyere murer med animasjon). Andreas liker også ladeanimasjonene og
  sliding, og vil ha animasjoner når man bruker ting: sprøyte/dop, pille, bandasje, spise/drikke, ta
  på vest og hjelm (realistisk lengde, kan avbrytes, du er sårbar mens det pågår). Gjør lading mer detaljert og realistisk per våpentype (taktisk vs. tom lading, magasin som
  faller på bakken), og la glidningen ha vekt, lyd og kamerabevegelse. Grafikk og kart: lag kartet med vinduer som
  kan knuses og klatreruter inn i og over bygg, gjerne som alternative ruter til flagget.
- **Andreas' godkjente forslag (26. nov.), fordelt:**
  - Kamp: lene (Q/E) og ligge ned; pust/svaie i kikkert, våpensving i løp, kulefall og gjennomslag
    (henger sammen med ballistikk-sporet); fottrinn etter underlag (Grafikk gir flatene riktig fysisk
    materiale); drapskamera fra drapsmannens side (bygger på dødskameraet).
  - Spillbarhet: markere fiender og ping for laget, nærradio/stemmechat (Unreal VoIP), flaggbæreren
    blir synlig for fiendene etter 20 s; replay etter runden (vurder Unreals replay-system, kan bli stort –
    del opp).
  - Grafikk og kart: fysiske materialer (stein, tre, metall, glass, gress) på alle flater, så fottrinn og
    kulegjennomslag kan bruke dem; tynne vegger/dører som kan skytes gjennom.
- **Andreas' godkjente forslag, del 2 (26. nov.), fordelt:**
  - Grafikk og kart: vær som påvirker spillet (regn, tåke, vind – laget eier `GSEnvironment` og
    replikerer verdiene; Kamp bruker dem til lyddemping, sikt og vindavdrift); bygg man kan bruke:
    lysbrytere, heiser, brannalarm (lag dem i kart-koden, bestill lyd/samspill fra Kamp ved behov).
  - Kamp: skade per kroppsdel (halting, sving ved armskudd, blødning som må bandasjeres – overlapper med
    Spillbarhets overlevelse; Kamp eier skadelogikken, Spillbarhet eier bandasje-gjenstanden);
    lommelykt og nattsikte montert på våpen; vind på lange skudd.
  - Spillbarhet: valg av utstyr/klasse før hver runde (passer med CTF-lobbyen); statistikk og nivåer
    som bare låser opp utseende (våpendrakter), aldri styrke – lagres lokalt per spiller.
- **Andreas' godkjente forslag, del 3 (26. nov.), fordelt:**
  - Kamp: ekte våpenmodeller fra Fab når Andreas har lagt dem inn (se etter nye mapper i `Content/`,
    ikke vent på dem); Lyra/Game Animation Sample-animasjoner på andre spillere (tredjeperson); våpenlyd
    med ekko og romklang ute og inne; inventar med dra-og-slipp, tydelige ikoner og raskere bytte
    (inventar-HUD er Kamps fil – samordne med Spillbarhet om gjenstandsdata).
  - Grafikk og kart: ytelse på RTX 3050 laptop (jevn bildefrekvens, smartere lys og skygger, LOD,
    mindre hakking – mål og logg før/etter); mer liv innvendig (rot, papirer, oppslag, skilt som på NTNU).
  - Spillbarhet: minikart/kart med flagg, baser og lagkamerater (bestill tegnekall i HUD fra Kamp, eller
    lag det i egne filer og bestill ett kall).
- **Andreas' godkjente forslag, del 4 (26. nov.), fordelt:**
  - Kamp: røyk og blend som fyller korridorer (følger rom og dører); nærkamp med kolbeslag og kniv;
    taktisk sprint (kort fart, mer lyd); snubletråd/miner (finnes – gjør dem gode rundt flagget).
  - Grafikk og kart: dører og vegger som kan sprenges og åpner nye ruter (bygg kartet med slike steder);
    barrikader – flyttbare pulter og stoler (fysikk-møbler); sikkerhetskameraer plassert på campus.
  - Spillbarhet: drone styrt fra PC-ene; hacking av kameraene (PC-ene og `GSTerminal` er lagets);
    ta flagget tilbake i nærkamp (CTF-regel); radiomeldinger på norsk fra eget lag ved flaggendringer
    (realistisk stil, ikke arkade-kommentator – lyd bestilles fra Kamp); tilskuermodus for døde.
- **Andreas' godkjente forslag, del 5 (26. nov.), fordelt:**
  - Kamp: treffrespons (rykk der kula treffer, fysisk fall – bygger på blodrunden); mykere
    bevegelsesoverganger uten fotgli; eget, lærbart rekylmønster per våpen; ekte linseeffekt og riktig
    forstørrelse i sikter; spillermodeller der skitt og blod setter seg; lydmiks der fottrinn og flagg
    alltid høres.
  - Grafikk og kart: lys inne (flimrende lysrør, sollys gjennom vinduer); våte flater og sølepytter,
    slitasje; trær og gress i vind; raskere oppstart og ingen frys ved lasting (henger sammen med
    ytelsesarbeidet).
- **Ønskelisten med 50 punkter** (`.claude/lag/onskeliste.md`) er godkjent og fordelt. Den kommer etter
  CTF-grunnmuren, ytelse og listene over. Planleggerne henter fra den når backlogen fylles.
  Q/E brukes i dag til annet (Q = neste ting i sekken, E = bruk). Spillbarhet foreslår ny tastefordeling
  i knappehjelp/keybinds; Kamp legger inn lene på de tastene som velges.
- **Kamp:** lagtilhørighet i treff og HUD (ikke treffmarkør/blod-varsel på lagkamerater hvis venneild er
  av), flaggbærer synlig, lyd og retningsvarsel når flagget tas. Blod-runden fortsetter som planlagt.

## 2026-11-26 (første vurdering)
**Hovedagentens syn:** Spillet har mye innhold, men to ting skiller det fra målet «superrealistisk og
veldig groteskt»: (1) våpen og skade oppfører seg ikke som i virkeligheten (ingen kulefall, flytid eller
gjennomslag, skade følger ikke kaliber, kroppen har bare «hode» og «resten»), og (2) det finnes nesten
ikke synlig skade: blod, sår og kropper. Samtidig har feiljakten vist ekte feil og juksehull som må bort
før noe føles ferdig.

**Lag Kamp**
1. Blod og gore som synes: blodsprut, blod på vegg og gulv, blodpøl, sår der kula traff (runde 3).
2. Kroppssoner: skade per kroppsdel med følger (bein = halter, arm = ustø sikting), ordentlige treffbokser.
3. Ballistikk: kulefall, flytid, gjennomslag, skadefall over avstand. Rett samtidig juksehullene V1–V3 i
   `funn.md` (sikting sendes ikke til serveren, klienten bestemmer spredning, for rask skuddtakt godtas).
4. Tone ned arkade-preg i HUD («LAVT LIV»-tekst, hvitt blink).

**Lag Grafikk og kart**
00. **Andreas har bestemt (26. nov.):** fjern klokketårnet bak Hovedbygningen (R5) og robotene som står
   som pynt (R11). Tas i runde 2.
0. **Ny retning fra Andreas (26. nov.): mindre kart, mye mer detaljert.** Velg et mindre, sammenhengende
   utsnitt av campus (kjernen rundt Hovedbygningen er et naturlig valg, men laget velger), stram inn
   spillområdet til det, og legg all innsatsen i detaljer der: fasader, innganger, rom innvendig, møbler,
   skilt, bakke, vegetasjon, lys. Det som faller utenfor blir kulisse eller fjernes. Dette går foran
   punktene under. Spawnpunkter og lootplasser må følge med (bestilling til lag Spillbarhet).
1. Finn det som ser minst ekte ut når man står midt på campus, og gjør det ordentlig. Ett bygg eller ett
   uteområde om gangen, helt ferdig, heller enn litt overalt.
2. Lys og vær som i en ekte novemberkveld i Trondheim.
3. Flater som kan ta imot blod og kulehull fra lag Kamp (bestillinger kommer i `bestillinger.md`).

**Lag Spillbarhet**
0c. **Fra Andreas (26. nov.): invitere venner fra menyen.** Gjør det i steg, etter hovedmenyen (0b):
   (1) uten kontoer: «Finn spill på nettverket» (LAN-søk som lister åpne spill) og en invitasjonskode/IP
   man kan kopiere og lime inn; (2) ekte venneliste og invitasjoner over internett via Steam eller Epic
   Online Services. Steg 2 krever at Andreas oppretter en utviklerkonto/app-ID – hovedagenten avklarer
   det med ham; planlegg og bygg steg 1 først, og gjør koden klar for steg 2 (OnlineSubsystem-sesjoner).
0b. **Fra Andreas (26. nov.): en ordentlig hovedmeny.** Når spillet starter kommer man til en meny med
   Start spill (vert), Bli med (skriv inn IP), Innstillinger (grafikk, følsomhet, lyd – lagres),
   Kontroller (samme oversikt som 0a) og Avslutt. Skal kunne brukes med mus, se nøktern og ferdig ut,
   og erstatte konsollkommandoene `Host`/`Join` for vanlige spillere. Lag den i egne nye filer laget eier
   (f.eks. `GSMenu.*`), i C++ uten .uasset. Henger sammen med 0a; planlegg dem som to runder etter
   hverandre eller én hvis det er lite nok.
0a. **Fra Andreas (26. nov.): vis spilleren hvilke knapper som gjør hva og hvordan man spiller.** En
   kontrolloversikt man kan slå av og på (f.eks. F1), korte hint som dukker opp når de er relevante
   (ved loot: «E plukk opp», ved PC, ved printer, første gang man er skadet/sulten), og en kort innføring
   i lobbyen om hva kampen går ut på. Nøkternt og lite, ikke arkade. Lag det i egne nye filer som laget
   eier (f.eks. `GSHelp.*`), og bestill bare ett lite kall fra `GSHUD`/`GSPlayerController` hos lag Kamp.
   Ta dette tidlig (runde 1 eller 2).
0. Kartet blir mindre (se lag Grafikk og kart): spawn, loot-tetthet og kamplengde må passe et mindre
   område når det kommer. Følg med i `bestillinger.md`.
1. De åpne funnene i `.claude/loop/funn.md` i egne filer: kampstart som ikke rydder, griefing i lobbyen,
   loot gjennom vegger, spawn, ammoeske-feil, ytelse for pickups.
2. Våpendata som følger virkeligheten: riktig kaliber, skade etter patron, beskyttelsesklasser for vester
   (R5–R7 i `funn.md`), i `catalog.py`. Samordn med lag Kamp via bestillinger.
3. Realistisk overlevelse: blødning som må stoppes, bandasjer, vekt.
