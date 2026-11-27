# Plan 10 – runde 16 (lag Kamp) – SKISSE: bruke ting med kroppen, og skjermeffekter per stoff

**Status: skisse.** Skrives ferdig når plan-08 (hendene er kroppens) er bygget, og når Spillbarhet har svart på
bestillingen om katalogen (se `.claude/lag/bestillinger.md`, 27. nov.). Andreas' punkt 3: «alle ting virker, også
medisin og boosts, med animasjoner når man tar dem og skjermeffekter (rødt ved røyking, hvitt ved amfetamin)».

## Slik er det i dag (lest i koden 27. nov.)
- **Animasjon:** `UpdateItemUse` beveger tingen (`FPItemMesh`) og en løs hånd i fem mønstre: kast, drikke, spise,
  pille, bandasje/medisin, og «sprøyte i armen» for alt annet. Ingen andre ser at du bruker noe (`UseItemId`,
  `UseStartTime`, `UseEndTime` er `COND_OwnerOnly`). Ingen skilnad på vest og hjelm (de tas bare på).
- **Skjermeffekter:** `FGSEffectMods::Visual` er ett tall (0–7); `Combine` lar den siste vinne, så to stoffer samtidig
  viser bare det ene. Effekten slås av og på brått (ingen innslag eller nedtur). 1 uskarp (bare kantfarge + vignett),
  2 farger, 3 skjelving (flytter siktet, `AddControllerPitchInput`), 4 grønn dis, 5 psykedelisk, 6 lyst/bloom, 7 nattsyn.
- **Effekter uten noe man merker på skjermen eller kroppen:** Snus, Nikotin, Koffein, Koffein+, Sukkerrush, Fokus,
  Bulk, Pansertablett, Regenerator, Tran, Mett, Lynet, Iskald, Smertestillende, Kirurgi (bare tall).
- **Feil/huller som er kjent:** «Iskald»/«Spøkelse» demper ikke hagleskudd (V6 i `funn.md`), sult/tørst har nesten
  ingen følger (B2 i `spillbarhet/funn.md`), boosts stables uten tak (B3, Spillbarhet), overnaturlige boosts (R2:
  Føniks, Spøkelse, Øyet, Blodtørst, +skade) strider mot «superrealistisk» – Spillbarhet eier dataene.
- **Finnes ikke:** røyk/sigarett, amfetamin (Turbo-pillen er nærmest), alkohol. Bestilt fra Spillbarhet.

## Mål
Alt du tar har en egen, realistisk bevegelse med kroppens hender (som andre ser), kan avbrytes og gjør deg sårbar, og
hvert stoff har en egen, gradvis skjermeffekt med innslag, topp og nedtur – flere kan virke samtidig.

## Forbedrer (gammelt)
- `UpdateItemUse`: hånd-IK på kroppen (plan-08) + venstre hånd med der to hender trengs (skru kork, rive bandasje,
  åpne pakke, trekke sprøytehette med tenna). Egne bevegelser per arketype fra katalogens `model.arch`
  (`syringe`, `pills`, `blister`, `vial`, `dropper`, `tin`, `patch`, `can`, `bottle`, `cup`, `bar`, `bandage`,
  `medkit`, `surgery`, ny `cigarette`): sprøyte i låret, snus under leppa, plaster på armen, inhalere.
- Vest og hjelm: ta på (vest over hodet, stropper; hjelm ned, hakereim), lengde fra katalogen.
- `UpdateLocalVisuals`: fra ett tall til en liste aktive effekter med styrke 0..1 over tid (innslag 2–10 s, topp,
  nedtur), blandet sammen i etterbehandlingen.
- Skjelving (3) flytter ikke lenger musa; den svaier våpenet (`SwayRot`) og kameraet (additiv), som er ærligere.

## Nytt
- Andre ser at du bruker noe: replikert `UseKind` (uint8) + tid til alle (`COND_SkipOwner`), kroppen spiller
  bevegelsen, lyd i 3D (knepp av pilleboks, riving av bandasje, snusboks).
- Skjermeffekter per stoff (forslag, avklares med Spillbarhet i katalogen):
  | Stoff | Skjerm/kropp |
  |---|---|
  | Røyk (ny) | rød-varm tone i kantene, litt mørkere, hoste ved sprint etterpå, røyk fra munnen andre ser |
  | Amfetamin / Turbo | hvit, overeksponert, store pupiller (lys blender mer), skarpt bilde, raskere pust, krasj = grått og tregt |
  | Adrenalin | tunnelsyn, puls høres, alt litt skarpere i 15 s |
  | Morfin | uskarpt i kantene (ekte dybdeskarphet), dempet lyd, langsomme bevegelser i våpenet |
  | Koffein/energidrikk | lett skjelving i hendene ved høy dose |
  | Nikotin/snus | lett svimmel de første sekundene |
  | Alkohol (ny, hvis Spillbarhet lager den) | doble kanter, svai, forsinket sikte |
  | Smertestillende | halting og armsving dempes (finnes) + lett dempet bilde |
  | Grønn | grønn dis (finnes), tregere |
  | Nattøye | nattsyn (finnes) |
  Alt uten arkade-preg: ingen ikoner eller tekst midt på skjermen; effekten skal kunne kjennes igjen uten tekst.
- Nye `Visual`-numre (8 rød/varm, 9 hvit/overeksponert, 10 tunnelsyn, 11 dobbeltsyn) – bestilt i katalogen.

## Avhengigheter
- Spillbarhet: katalogfelt (`visual`, ny `onset`/`peak`), nye gjenstander (sigarett, amfetamin, evt. alkohol), og
  svar på R2/B3. Til det er levert bruker Kamp en reservetabell på effekt-Id i `GSCharacter.cpp`.
- Ressurser: bevegelsene lages i kode (hånd-IK + nøkkelbaner, slik `UpdateItemUse` allerede gjør). Ingen gratispakke
  vi kjenner har gode animasjoner for å ta piller/sprøyter i førsteperson; Lyra/GASP har det ikke. Røyk fra munnen
  lages som partikkel i `GSEffects` (Kamp). Ingenting kreves av Andreas.

## Filer (forventet)
`GSCharacter.*` (`UpdateItemUse`, `CompleteUse`, `CancelUse`, `UpdateLocalVisuals`, `ApplyLowHealthPostProcess`,
`TickEffects`), `GSBodyAnim.*`, `GSEffects.*`, `GSAudio` + `Tools/Audio/` (lyder), bestilling til `catalog.py`.

## Testplan
Konsollkommando (bare i utviklerbygg) `GiEffekt <Id>` og `GiTing <Id> <antall>` for å prøve hver effekt og hver ting i
PIE; gå gjennom alle 40 effektene i katalogen og skriv i rapporten hva man ser og merker for hver.
