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
