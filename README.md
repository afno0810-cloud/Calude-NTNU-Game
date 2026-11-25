# Gløshaugen Strike

Online førstepersons skytespill på NTNU Gløshaugen, laget i Unreal Engine 5.8 (C++).

## Første gang: slik åpner du spillet

**Du trenger:** Unreal Engine 5.8, Visual Studio 2022 med «Game development with C++» og «Desktop development with C++», og GitHub Desktop.

1. **Hent koden:** I GitHub Desktop velger du *File → Clone repository* og så `Calude-NTNU-Game`. Legg den f.eks. i `Dokumenter\Unreal Projects`.
2. **Åpne prosjektet:** Dobbeltklikk `Gloshaugen.uproject`. Unreal spør *«The following modules are missing or built with a different engine version… Would you like to rebuild them now?»*. Trykk **Yes**. Første bygging tar 2–10 minutter.
   - Hvis det kommer en feilmelding, ta skjermbilde og send det til Claude. Loggen ligger i `Saved/Logs/Gloshaugen.log`.
3. **Lag kartet (bare én gang):** *File → New Level → Empty Level*. Trykk så *Ctrl+S* og lagre det i mappen `Content/Maps` med navnet **`Gloshaugen`**. Hele campus, himmel, vær og lys bygges automatisk når spillet starter, så kartet skal være tomt.
4. **Spill:** Trykk den grønne ▶ **Play**-knappen. Klikk i vinduet for å styre, og trykk **Esc** for å stoppe.

## Teste online med flere spillere på samme PC

Klikk på **⋮** ved siden av Play-knappen og velg:
- *Number of Players*: **2** (eller flere)
- *Net Mode*: **Play As Listen Server**

Trykk Play. Da får du ett vindu per spiller, koblet sammen som et ekte online-spill.

## Spille med en venn på samme nett (LAN)

- Den som er vert åpner konsollen med **§** (eller **~**) og skriver `Host`.
- Den andre skriver `Join 192.168.x.x`, altså vertens IP-adresse (finnes med `ipconfig` i Windows).
- Over internett kommer via Steam/EOS i en senere versjon.

## Kontroller

| Tast | Hva |
|---|---|
| WASD | Gå |
| Shift | Løp (bruker stamina) |
| Space | Hopp, eller klatre opp på kanter og murer |
| Ctrl / C | Huk. Mens du løper: **glid** |
| Venstre mus | Skyt |
| Høyre mus | Sikt |
| R | Lad om |
| 1 / 2 / scroll | Bytt våpen |
| Tab | Poengtavle |

## Hva som er med i denne versjonen (M1 + M2 + M3)

- Førsteperson-bevegelse: gå, løpe, hoppe, huke, gli og klatre opp på kanter
- Pistol (M1911) og AG3, med rekyl, spredning, sikting, omlading og hodeskudd (dobbel skade)
- Server-styrt treff og skade, død, respawn langt unna fiender, kill-feed og poengtavle
- Hele Gløshaugen som blokker: 17 bygg med etasjer, vinduer, dører og ramper mellom etasjene (Hovedbygningen med Lysgården, Sentralbygg 1 og 2, Realfagbygget, Elektro, Stripa osv.), veier, trær og parkerte biler
- AG3-er og pistoler å finne rundt på campus, blå ring = AG3
- Tilfeldig vær (klart, overskyet, tåke, regn, snø) som skifter underveis, og dag/natt-syklus (ett døgn = 48 minutter) med gatelys om natta
- Opptil 20 spillere

Grafikken er bevisst enkel (blokker) til vi har kartdata og modeller på plass, se planen.

## For utviklere

- `Source/Gloshaugen/Public/GSCampus.h`: layout av campus (bygg, veier, spawn, våpen)
- `GSTypes.h`: våpenstatistikk
- `GSCharacter`: bevegelse, våpen, helse
- `GSCampusBuilder`: bygger campus fra dataene ved oppstart
- `GSEnvironment`: sol, måne, himmel, tåke, regn og snø
- `GSGameMode` / `GSGameState`: respawn, kills, vær og klokke
