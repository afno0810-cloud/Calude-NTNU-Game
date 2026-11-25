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
| 1 / 2 / 3 / 4 / scroll | Bytt våpen (du kan bære 4) |
| V | Slag / nærkamp med det du holder |
| E | Plukk opp / åpne låst kasse |
| F | Bruk ting i sekken (spis, drikk, sprøyt, kast granat, sett på demper ...) |
| Q | Neste ting i sekken |
| G | Slipp ting |
| I | Inventar |
| Tab | Poengtavle |

## Hva som er med i denne versjonen

- Førsteperson-bevegelse: gå, løpe, hoppe, huke, gli og klatre opp på kanter
- **43 våpen**: pistoler, maskinpistoler, geværer, snikskyttere, hagler, maskingeværer, Carl Gustaf, M79, armbrøst, og nærkampvåpen (kniv, katana, vikingsverd, øks, slegge, stekepanne ...)
- **Sjeldenhet** på alt: Vanlig (grå), Uvanlig (grønn), Sjelden (blå), Episk (lilla, lysstråle), Legendarisk (gull), Mytisk (rød)
- **Overlevelse**: sult og tørst (spis og drikk, ellers mister du helse), 22 matvarer, 5 medisiner
- **20 boosts/dop** med effekter og bivirkninger (Turbo, Grønn, Snus, Berserk, Føniks, Øyet ...)
- **Rustning**: 5 vester + 3 hjelmer som tar av skade til de går i stykker
- **Granater og utstyr**: splint, røyk, blend, molotov, C4, snubletrådmine, drone, gripekrok, fallskjerm, nattsyn, kikkert, lommelykt, walkie-talkie, demper, sikter, sekker ...
- Loot i alle etasjer i alle bygg, låste kasser med god loot (adgangskort/dirkesett), mytisk kasse i Lysgården
- Dør du, ligger tingene dine igjen der du døde
- Server-styrt treff og skade, død, respawn langt unna fiender, kill-feed og poengtavle
- Hele Gløshaugen som blokker: 17 bygg med etasjer, vinduer, dører og ramper mellom etasjene, veier, trær og parkerte biler
- Tilfeldig vær (klart, overskyet, tåke, regn, snø) og dag/natt-syklus (ett døgn = 48 minutter) med gatelys om natta
- Opptil 20 spillere

Grafikken er bevisst enkel (blokker) til vi har kartdata og modeller på plass, se planen.

## For utviklere

- `Source/Gloshaugen/Public/GSCampus.h`: layout av campus (bygg, veier, spawn, våpen)
- `Tools/ItemCatalog/catalog.py`: **alle gjenstander** (våpen, mat, dop, rustning, utstyr) med tall. Endre her, kjør `python Tools/ItemCatalog/gen_cpp.py`, så oppdateres C++ (`GSItemCatalog.gen.inl`)
- `GSItems`: oppslag i katalogen, sjeldenhet, tilfeldig loot
- `GSPickup`: ting på bakken og låste kasser
- `GSProjectile`: raketter, granater, piler, molotov, C4, miner
- `GSCharacter`: bevegelse, våpen, helse
- `GSCampusBuilder`: bygger campus fra dataene ved oppstart
- `GSEnvironment`: sol, måne, himmel, tåke, regn og snø
- `GSGameMode` / `GSGameState`: respawn, kills, vær og klokke

## 3D-modeller (Blender)

Modellene lages med Python-skript i `Tools/Blender/` og ligger ferdig eksportert i `SourceArt/Models/` (`.fbx` for Unreal, `.blend` for Blender og `.glb` for forhåndsvisning).

- **Automatisk import:** Hver gang du åpner prosjektet i Unreal, kjører `Content/Python/init_unreal.py` og importerer nye eller endrede modeller til `Content/Models`. Du trenger ikke gjøre noe.
- Finnes ikke modellen ennå, bruker spillet en enkel boks i stedet.
- Vil du se eller endre en modell: åpne `.blend`-filen i Blender.
- Lage modellene på nytt: `python Tools/Blender/gen_models.py` (alle gjenstandene i katalogen), pluss `weapon_ag3.py`, `weapon_pistol_m1911.py` og `weapons_more.py` for de håndlagde
- Oversiktsbilder: `python Tools/Blender/contact_sheet.py ut.png SM_AK47 SM_Kebab ...`

126 modeller er ferdige: alle våpen, rustninger, mat, dop, medisin, granater og utstyr. Første gang du åpner prosjektet tar importen noen minutter.
