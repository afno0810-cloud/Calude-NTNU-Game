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

## Tre lag (ni agenter)
Les `.claude/lag/LAG.md`. Kjør samlebåndet under for hvert av de tre lagene samtidig (Kamp, Grafikk og
kart, Spillbarhet). Si alltid til agenten hvilket lag den er og hvilken lagmappe den bruker. Del ut
nummerserie i `IMPROVEMENTS.md` til hver plan. Commit hvert lags filer for seg. Sjekk
`.claude/lag/bestillinger.md` og gi åpne bestillinger til eierlagets planlegger. Felles lærdom som gjelder
alle går i `.claude/loop/laerdom.md`; lærdom som bare gjelder ett lag går i lagets `laerdom.md`.

## Samtidig drift (standard): alle tre jobber hele tiden
Andreas vil at alle tre agentene skal kjøre samtidig, ikke etter tur. Kjør dem som et samlebånd i
bakgrunnen, og start en ny oppgave for en agent straks den er ferdig:

- **Planleggeren** ligger foran: den skriver planer til køen `.claude/loop/ko/plan-NN.md` og holder
  alltid 1–2 ferdige planer klare. Den leser bare kode, og planlegger ut fra koden *slik den blir* etter
  rundene som er i arbeid (les planene foran i køen). Når køen er full: oppdater `retning.md`.
- **Byggeren** tar neste plan fra køen. Når runden bygger uten feil, committer dirigenten den straks som
  en kandidat (`Runde NN: ...`), og byggeren går rett videre til neste plan. Rettinger fra kontrolløren
  har alltid førsteprioritet og tas før neste plan.
- **Kontrolløren** går gjennom forrige rundes commit (`git show <commit>`), ikke arbeidsmappen, og
  skriver `.claude/loop/kontroll/runde-NN.md`. Når det ikke er noe å kontrollere, gjør den en
  **feiljakt** i ett område av den eksisterende koden og skriver funn til `.claude/loop/funn.md`, som
  planleggeren leser.
- **Bare byggeren kompilerer.** Aldri to bygg samtidig (PC-en har 16 GB RAM). Kontrolløren og
  planleggeren kjører ikke `Build.bat` i samtidig drift.
- **Bare byggeren endrer filer i `Source/`, `Tools/` og `IMPROVEMENTS.md`.** De to andre skriver kun i
  `.claude/loop/`. Slik kolliderer de aldri.
- Sier kontrolløren **MÅ FIKSES** på en commit som alt er lagret: byggeren retter i en ny commit
  (`Runde NN fix: ...`). Er feilen alvorlig og ikke lar seg rette på to forsøk: `git revert` commiten.
- Lærdom, logg og selvforbedring av agentene skjer som beskrevet under, for hver runde som er ferdig
  kontrollert.

Reglene under («Hver runde») beskriver innholdet i hvert steg og gjelder fortsatt.

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
