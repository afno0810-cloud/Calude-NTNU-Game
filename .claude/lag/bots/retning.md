# Retning for laget Bots

Eies av lagets planlegger. Andreas' ønsker i `.claude/loop/andreas.md` er rammen.

## Visjon
(skrives av planleggeren i første runde)

## Neste ideer (backlog, viktigst øverst)
- (fylles av planleggeren)

## Fra kontrollørens forhåndssjekk av plan-01 (27. nov., se `kontroll/forhand-plan-01.md`)
- Del plan-01 i 01a (navnett, bots som spiller: syn, sikte, skudd, roller) og 01b (dekning, medisin, hørsel, deling, ytelse).
- Krav før 01a: Kamps K1 (utvidet med `SetupLocalBody`/Tick-grenen og friend for `Camera`) og Grafikks Helgasetr 4a.
- Bare egne filer: bots lages fra en `UGSBotsSubsystem` i `GSBots.*` og `AGSGameMode::RestartPlayer`; små bestillinger til
  Spillbarhet (`bBotsAvailable`, lagbytte, hjelpetekst, «Vis navnett»).
- Botene må selv sette kamerarotasjonen (ellers skyter de vannrett), trekke via-punkter fra gulv (ikke atrielufta), og sjekke
  skuddlinja mot fast glass.
- Dører, fysikkmøbler, knusbart glass og navnettet: bestillingene går nå til Interaktiv verden.

## Endringer i retningen
- 2026-11-27: laget opprettet (Andreas: «legg til 2 sett til med agenter»).
