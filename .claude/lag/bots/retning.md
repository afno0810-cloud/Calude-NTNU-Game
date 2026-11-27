# Retning for laget Bots

Eies av lagets planlegger. Andreas' ønsker i `.claude/loop/andreas.md` er rammen.

## Visjon
«Spill mot bots» skal føles som en CTF-kamp mot folk i Helgasetr: botene går i ekte ruter gjennom korridorer, rom, trapper
og atriet, forsvarer baserommet, tar flagget og bærer det hjem en annen vei. De ser bare det de faktisk ser, trenger
reaksjonstid, bommer som mennesker, går i dekning, bandasjerer seg og roper til hverandre. Ingen bot får mer helse, skade
eller informasjon enn en spiller. Fire regler:
1. **Samme kropp, samme handlinger.** Botene styrer `AGSCharacter` bare gjennom spillerens handlinger (via Kamps `friend`).
2. **Ingen koordinater i koden.** Navnett, ruter og forsvarspunkter regnes fra kartdataene, så kartendringer virker av seg selv.
3. **Bare egne filer.** `GSBotController.*`, `GSBotTeam.*`, `GSBots.*`, `GSNavSetup.*`; alt annet som små bestillinger.
4. **Billig.** Tenking spredt over bilder med tak på spor; kroppene koster som spillere (K1 fjerner førstepersonskroppen).

## Spor
- **A. Bots som spiller (01a):** navnett, subsystem, syn, sikte, skudd, skuddlinje, roller.
- **B. Bots som overlever (01b):** dekning, medisin, hørsel, roping, eskorte, ytelsesmåling.
- **C. Bots med taktikk (02):** dører, eskorte og flanke, undertrykkende ild, granater, loot, nærkamp.
- **D. Bots online:** fylle opp lag i online-kamper.

## Neste ideer (backlog, viktigst øverst)
1. **01a** (`ko/plan-01a.md`) – krav: Kamps K1 levert (bygges nå), Grafikks Helgasetr 4a bygget. IMPROVEMENTS 678–693.
2. **01b** (`ko/plan-01b.md`) – etter 01a. IMPROVEMENTS 694–709.
3. **02** (`ko/plan-02.md`, skisse) – dører (Interaktivs `AGSDoor`), eskorte/flanke, undertrykkende ild, granater (Kamps
   kast-API), loot, nærkamp/ta flagget tilbake. Skrives ferdig etter 01b med tallene fra loggen.
4. Bots i online-kamper («fyll opp lagene»): ett valg i Spillbarhets meny + telling i `GSGameMode` (bestilling).
5. Bots som bruker Helgasetr: lysbrytere (mørke rom gir kortere sikt – erstatter nattfaktoren), heis, knust rekkverk.
6. Finjustering per vanskelighetsgrad fra Andreas' testing (tabellen logges ved kampstart).

## Avgjørelser fra forhåndssjekken av plan-01 (27. nov., `kontroll/forhand-plan-01.md`)
- Plan-01 er delt i 01a og 01b (for stor for én runde).
- Bots lages fra `UGSBotsSubsystem` i `GSBots.*` med `AGSGameMode::RestartPlayer` (public) – ingen endring i Spillbarhets filer.
  Små bestillinger til Spillbarhet: `bBotsAvailable` i `AGSGameState`, lagbytte mot bot, hjelpetekst, «Vis navnett».
- Kamps K1 (`IsLocalHuman`, `friend`, sikteretning for AI via kontrollrotasjonen, ingen førstepersonskropp for AI) er
  bestilt og bygges nå av Kamp. 01a sjekker at den er levert før bygging og legger ikke inn egen kamerafiks.
- Via-punkter trekkes fra flaggene og spawnpunktene projisert til nettet, aldri fra midtpunktet (lufta i atriet).
- Agentradius 30 (40 cm stripe i 1,0 m dører); reservegrense `PlayArea` × Z −1…+17,5 m; selvtest per døråpning.
- Ingen nattfaktor innendørs.
- Skuddlinja sjekkes mot fast glass og lagkamerater før hvert skudd.
- Dører, fysikkmøbler og knusbart glass hører til Interaktiv verden: bestilling der; flyttbare møbler står utenfor navnettet.
- Ytelse måles som botenes tenketid, spilltråd med/uten tenking og absolutt `stat unit` – ikke «høyst 10 % fall».

## Endringer i retningen
- 2026-11-27: plan-01 delt i 01a/01b etter forhåndssjekken; eierskap rettet (bare lagets fire filpar), K1 som krav før bygging.
- 2026-11-27: laget opprettet (Andreas: «legg til 2 sett til med agenter»).
