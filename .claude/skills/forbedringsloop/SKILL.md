---
name: forbedringsloop
description: Kjører forbedringsløkken for Gløshaugen Strike – planlegger, bygger og kontrollør etter hverandre, runde etter runde, og lærer av hver runde. Bruk når Andreas ber om å forbedre, utvikle eller jobbe videre med spillet. Valgfritt: et mål og antall runder, f.eks. «/forbedringsloop 3 bedre våpenfølelse».
---

# Forbedringsløkken

Du er dirigenten. Du gjør ikke jobben selv; du kjører de tre agentene og passer på at løkken lærer.

**Argumenter:** et tall = antall runder (standard 3). Resten = mål for løkken (tomt = agentene velger
selv ut fra `.claude/loop/andreas.md`).

## Før første runde
1. Sørg for at du er på en arbeidsgren, ikke `main`: lag `claude/forbedring-<dato>` om nødvendig.
   Ta ikke med Andreas' egne, uferdige endringer i commitene dine (bare filene runden endret).
2. Sjekk om `UnrealEditor` kjører (`Get-Process UnrealEditor`), og gi agentene beskjed om det.
3. Opprett `.claude/loop/logg.md` hvis den ikke finnes.

## Hver runde (N = 1, 2, 3 …)
1. **Plan:** kall agenten `planlegger` med målet. Den skriver `.claude/loop/plan.md`.
2. **Bygg:** kall agenten `bygger`. Den gjennomfører planen og bygger.
3. **Kontroll:** kall agenten `kontrollor`. Den skriver `.claude/loop/kontroll.md`.
4. **Retting:** står det **MÅ FIKSES**, kall `bygger` igjen, så `kontrollor` igjen. Maks 2 rettinger.
   Er det fortsatt ikke godkjent: rull tilbake rundens endringer (`git restore`/`git clean` bare på
   filene runden endret), og skriv i loggen hvorfor det stoppet.
5. **Lagre:** når det er **GODKJENT**, commit rundens filer (engelsk melding, stilen i `git log`).
6. **Lær:**
   - Legg rundens «Lærdom» fra `kontroll.md` inn i `.claude/loop/laerdom.md`. Finnes regelen allerede,
     øk telletallet i stedet for å legge den inn på nytt. Fjern regler som denne runden viste var feil.
   - Skriv én linje i `.claude/loop/logg.md` (nyeste øverst): dato, runde, mål, resultat
     (godkjent / rullet tilbake), antall rettinger, commit-id.
   - Commit `laerdom.md` og `logg.md` sammen med runden.
7. Neste runde starter med kontrollørens «Forslag til neste runde» som utgangspunkt.

## Forbedre agentene selv (hver 3. runde, og når samme problem kommer igjen)
Les `logg.md` og `laerdom.md`. Ser du et mønster (samme type feil to ganger, mange rettinger, planer
som er for store), forbedre instruksene i `.claude/agents/*.md` slik at det ikke skjer igjen. Gjør små,
konkrete endringer, skriv hvorfor i `logg.md`, og commit dem som egen commit
(`Agents: <hva som er endret>`). Endre aldri bort reglene om at Andreas' ønsker går først, og at det
ikke skal bygges mens editoren er åpen.

## Når Andreas gir tilbakemelding
Skriv den inn under «Tilbakemeldinger» i `.claude/loop/andreas.md` med dato, før neste runde starter.

## Til slutt
Push grenen. Gi Andreas en kort oppsummering på norsk: hva som er forbedret, hva han skal teste i
spillet (trykk Play og se etter …), og om han må trykke Ctrl+Alt+F11 eller åpne Unreal på nytt.

## Kjøre hele tiden
For en løkke som går av seg selv: `/loop /forbedringsloop 1` (én runde av gangen, så lenge Claude Code
er åpen).
