# De tre lagene

Tre lag jobber samtidig på spillet. Hvert lag har sin egen planlegger, bygger og kontrollør (ni agenter),
og følger de samme instruksene (`.claude/agents/`), de samme ønskene (`.claude/loop/andreas.md`) og den
samme felles lærdommen (`.claude/loop/laerdom.md`). Dirigenten (`/forbedringsloop`) sier hvilket lag du er.

| Lag | Fokus | Lagmappe |
|---|---|---|
| **Kamp** | Våpen, skyting, skade, blod og gore, lyd, HUD i kamp, dødskamera | `.claude/loop/` |
| **Grafikk og kart** | Campus og bygg, modeller, materialer, teksturer, lys, vær, himmel, vegetasjon | `.claude/lag/grafikk/` |
| **Spillbarhet** | Kampregler, spawn, loot og balanse, crafting, PC-er, lobby, flyt, kontroller og innstillinger | `.claude/lag/spillbarhet/` |

I lagmappen ligger lagets egne filer: `lag.md` (fokus), `retning.md`, `logg.md`, `funn.md`, `laerdom.md`
(lagets egen lærdom, i tillegg til den felles), `ko/plan-NN.md` og `kontroll/runde-NN.md`.

## Hvem eier hvilke filer (bare eierlagets bygger endrer dem)
| Filer | Eier |
|---|---|
| `GSCharacter.*`, `GSHUD.*`, `GSPlayerController.*`, `GSProjectile.*`, `GSEffects.*`, `GSAudio.*`, nye `GSBallistics.*`, `Tools/Audio/`, `SourceArt/Sounds/` | Kamp |
| `GSCampusBuilder.*`, `GSCampus.h` (kartdata: bygg, veier, spillområde, baser, spawnpunkter), `GSEnvironment.*`, `GSWorldSubsystem.*`, `Tools/Blender/`, `Tools/Textures/`, `Content/Python/`, `SourceArt/Models/`, `SourceArt/Textures/` | Grafikk og kart |
| `GSGameMode.*`, `GSGameState.*`, `GSPlayerState.*`, `GSItems.*`, `GSPickup.*`, `GSTerminal.*`, `Tools/ItemCatalog/` (og den genererte `GSItemCatalog.gen.inl`), nye `GSHelp.*`, `GSMenu.*` og `GSFlag.*` | Spillbarhet |
| `GSTypes.h`, `Gloshaugen.Build.cs`, `IMPROVEMENTS.md` | Felles: bare små tillegg, aldri omskriving. Legg til nederst i en struct/enum, ikke endre andres linjer. |

## Avgjørelser om delte ting
- Bildefrekvens-taket eies av Kamp (`GSPlayerController`: `Bildefrekvens`, `t.MaxFPS`). Menyen viser og endrer det via Kamps funksjoner. (26. nov.)

## Regler for samarbeid
- **Trenger du en endring i et annet lags fil?** Ikke gjør den selv. Skriv en bestilling i
  `.claude/lag/bestillinger.md` (hvem ber, hvilken fil, nøyaktig hva, hvorfor). Eierlagets planlegger tar
  den inn i neste plan. Planlegg din egen runde slik at den virker også før bestillingen er levert.
- **Bare én kompilerer om gangen.** `Build.bat` kjøres med `-WaitMutex`, som gjør at et bygg venter på
  det forrige. Start aldri et bygg uten `-WaitMutex`.
- **Bygget feiler i en fil du ikke eier?** Da er et annet lag midt i en endring. Ikke rett den filen.
  Vent litt, bygg på nytt (inntil 4 forsøk), og meld fra til dirigenten hvis det fortsatt feiler.
- **Hold koden byggbar:** gjør endringer i headere (`.h`) ferdige raskt og i ett, så andre lags bygg
  ikke stopper lenge.
- **IMPROVEMENTS.md:** dirigenten deler ut nummerserien for runden. Bruk bare dine numre, i din egen
  seksjon med lagnavnet i overskriften.
- **Commit:** dirigenten committer hvert lags filer for seg, med lagnavnet først i meldingen
  (`Combat: ...`, `Graphics: ...`, `Gameplay: ...`). Alle lag jobber i samme mappe og på samme gren.
