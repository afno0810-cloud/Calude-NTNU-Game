# Plan-03 (Spillbarhet, runde 3 / «3A»): knappehjelp – H-oversikt, hint og sen innkobling

Skrevet om etter kontrollørens forhåndssjekk (`kontroll/forhand-plan-03.md`, E3 og E5 tatt inn). Bygger på `HEAD`
`ffa99ef` (hovedmenyen `GSMenu.*` er committet) + Kamps runde 11 (`GSHUD` kaller `GSHelp::Draw` selv og har radiolinjer
for flagget). **Les filene slik de står i arbeidsmappen, ikke slik eldre planer beskriver dem.**
Overtid, avslørt bærer, `Logout` og ytelse er flyttet til **plan-04 («3B»)**.

## Mål
En ny spiller ser selv hvilke taster som gjør hva og hvordan Capture the Flag spilles: **H** viser alt, og korte hint
dukker opp akkurat når de trengs – med tastene han selv har valgt.

## Forbedrer (gammelt)
- **Lobby-panelet i `GSHelp`**: får linja «H viser alle taster og regler».
- **Sen innkobling** (funn C2): den som kommer inn midt i en kamp får i dag ingen beskjed, og de andre ser ikke hvem
  som kom. Blir: egen linje med lag og stilling, og en linje i hendelseslista hos alle.
- **Kontrollsiden i hovedmenyen**: viser H-raden, H kan ikke bindes til noe annet, og får knappen «Vis hintene på nytt».

## Nytt
- **H-oversikten «Taster og regler»**: alle taster slik de er valgt, tastene på PC og 3D-printer, og reglene i CTF.
- **11 hint** som vises når de er aktuelle, høyst to ganger hver, og huskes mellom økter.

## Hvorfor nå
0a er Andreas' eldste åpne ønske. `GSMenu::KeyName`/`Controls` finnes nå, så hjelpen kan vise riktige taster.

## Filer
| Fil | Endring |
|---|---|
| `Public/GSHelp.h`, `Private/GSHelp.cpp` | H-oversikt, påminnelse, hint med lagring, sen innkobling, `ResetHints()` |
| `Private/GSMenu.cpp` | H reservert i `BuildRows`, H-rad i `Controls()`, knapp «Vis hintene på nytt» |
| `Private/GSGameMode.cpp` | beskjed i hendelseslista ved sen innkobling |
| `IMPROVEMENTS.md` | ny seksjon øverst, 460–467 |

Ikke rør `GSCharacter.*`, `GSHUD.*`, `GSPlayerController.*`, `GSTypes.h`, `GSFlag.*`, `GSGameState.*`, `catalog.py`.

## Steg

### 1. `GSHelp.h`
Oppdater kommentaren (H-oversikt og hint) og legg til:
```cpp
/** Forget which hints were shown (and that the overview was opened), so they come again. */
GLOSHAUGEN_API void ResetHints();
```

### 2. `GSHelp.cpp` – tilstand og H-tasten
Nye inkluderinger: `GSCharacter.h`, `GSPickup.h`, `GSTerminal.h`, `GSItems.h`, `Misc/ConfigCacheIni.h`.

**Tilstand per HUD** (flere PIE-vinduer deler statiske variabler):
```cpp
struct FHelpState
{
	bool bOpen = false; uint64 KeyFrame = 0;
	FName HintId; float HintStart = -100.f; bool bHintCounted = false; float NextHintAt = 0.f;
	TMap<FName, float> LastShown;      // per hint: when it was last on screen
	TSet<FName> SeenFalse;             // hints whose condition has been false since they were last shown
	float JoinedAt = -1.f; bool bFirstDrawDone = false; bool bJoinedMid = false;
};
```
i en `TMap<TWeakObjectPtr<const AHUD>, FHelpState>` (rydd ugyldige nøkler når den har over 16). Tid = `World->GetTimeSeconds()`.

**H-tasten** (i `DrawImpl`, etter at `PC` og `GS` er funnet og etter `IsFullscreen`-returen, men **før** returen for
`bMenuOpen`/`IsOnTerminal`): er `GPC->bMenuOpen || GPC->IsOnTerminal() || GPC->bShowInventory || GSMenu::IsModal(PC)` →
`bOpen = false`. Ellers:
`if (GFrameCounter != St.KeyFrame && PC->WasInputKeyJustPressed(EKeys::H)) { St.KeyFrame = GFrameCounter; St.bOpen = !St.bOpen; }`
(Ikke F1: den er «viewmode wireframe» i utviklingsbygg. Vakten trengs fordi `Draw` og reservekroken kan kjøre samme frame.)
Første og andre gang den åpnes: tell opp `OverviewOpened` (lagring som hintene, steg 4).

### 3. `GSHelp.cpp` – oversikten, påminnelsen, lobby-linja, sen innkobling
**a) Oversikten** (`bOpen`): tegnes sist, oppå alt annet i `GSHelp`; hint tegnes ikke samtidig.
Panel midt på skjermen, bredde `Min(820, SizeX − 64)`, `PanelColor` med alfa 0.9:
- Tittel «Taster og regler»; til høyre (dempet, 0.75) «H lukker».
- **Taster:** `GSMenu::Controls(PC)` i to kolonner (del på midten), rad 22 px, skala 0.8: `Label` i `TextColor`, `Key`
  høyrestilt i kolonnen i dempet rav `(0.85, 0.65, 0.25)`. (H-raden kommer fra `Controls()`, steg 5.)
- **På en PC / på en 3D-printer** (to dempede linjer): les `AGSPlayerController::TerminalInput` (og det `GSHUD`
  skriver på terminalskjermene) og skriv tastene slik de faktisk er. Utgangspunkt: PC «1 kameraer · 2 spor spillere ·
  3 tilbake · Q/F forrige/neste kamera · E logg ut»; printer «Q/1/2 velg · F eller venstre mus start».
  Stemmer det ikke med koden: rett teksten, ikke koden.
- **Capture the Flag** (overskrift + linjer, skala 0.8). Bare regler som finnes i dag (to linjer til kommer i plan-04):
  1. «Ta motstanderens flagg ved å gå helt bort til det, og bær det til din egen base.»
  2. «Du får bare poeng når ditt eget flagg står hjemme.»
  3. «Bæreren går saktere.»
  4. «Faller bæreren, blir flagget liggende. Motstanderne kan ta det opp; eget lag sender det hjem ved å gå bort til det.»
  5. «Første lag til %d erobringer vinner, ellers flest når %d minutter er gått.» (`CaptureLimit`, `MatchMinutes`)
  6. «Du kommer tilbake ved egen base. Det du bar, ligger igjen der du falt.»
  Ha linjene i én statisk funksjon `RuleLines(GS)` (plan-04 legger til der).
- Blir panelet høyere enn `SizeY − 40`: skala 0.72 og rad 19 px.

**b) Påminnelse:** dempet linje nederst til høyre (`SizeX − 24`, `SizeY − 28`, høyrestilt, skala 0.7) «H: taster og
regler», de første 45 s av lobbyen og de første 20 s av en kamp, bare mens `OverviewOpened < 2`. Les `DrawBars` og
ammo-feltet i `GSHUD.cpp` og flytt linja hvis noe ligger der.

**c) Lobby-panelet:** én dempet linje sist: «H viser alle taster og regler.»

**d) Kom inn midt i en kamp:** første gang denne HUD-en tegnes **etter at `GS` finnes** (`!bFirstDrawDone`): er fasen
`Playing` → `bJoinedMid = true`; sett `bFirstDrawDone = true`. Når `bJoinedMid && JoinedAt < 0 && PS && PS->Team != 0`:
`JoinedAt = Now` (ikke før laget er kjent – ellers står det «uten lag»). I 10 s fra `JoinedAt`, under `Playing`:
«Kampen er i gang. Du er på %s. Stillingen er %d–%d.» (lagnavn i små bokstaver; linja i lagets tekstfarge). Tegn den på
egen-linjas plass når den er ledig, ellers linja under.

### 4. `GSHelp.cpp` – hint
**Lagring:** `[Gloshaugen.Help]` i `GGameUserSettingsIni`, nøkkel = hint-id (og `OverviewOpened`), verdi = antall.
Les til en statisk `TMap<FName, int32>` første gang (ikke hver frame). `GConfig->Flush(false, GGameUserSettingsIni)` bare
når et tall endres. `ResetHints()`: `GConfig->EmptySection(...)`, tøm kartet (også `OverviewOpened`), `Flush`, og tøm
`LastShown`/`SeenFalse` i alle `FHelpState`.

**Når et hint kan vises** (alle må stemme):
- Pawnen finnes og lever; oversikten er lukket; ikke `GPC->bMenuOpen`, `IsOnTerminal()`, `bShowInventory`,
  `bShowScoreboard` eller `GSMenu::IsModal(PC)`.
- Fase: **flagghintene (`OwnTaken`, `Carry`, `EnemyFlag`) bare i `Playing`.** I `Lobby`/`Countdown` bare
  `Weapon`, `Item`, `Crate`, `Computer`, `Printer`, `Empty`. I `Playing` alle. Aldri i `Ended`.
- Lagret antall < 2.
- **Ikke mas:** samme hint kommer ikke igjen før vilkåret har vært usant siden sist (`SeenFalse` – legg id-en inn hver
  frame vilkåret er usant, fjern den når hintet vises) **og** det er gått minst 60 s (`LastShown`).
- Ett om gangen: 7 s (ton inn/ut 0,4 s), så minst 6 s pause (`NextHintAt`). Første aktuelle i rekkefølgen under.
- **Telling:** tell opp og lagre først når hintet har stått i 2 s (`bHintCounted`). Forsvinner vilkårene for visning
  (f.eks. sekken åpnes) før 2 s: avbryt uten å telle og uten 60 s-sperre.

`K(x)` = `GSMenu::KeyName(PC, TEXT(x))`; er strengen tom (før `MappingContext` finnes), bruk standardtasten i parentes.
`C = Cast<AGSCharacter>(PC->GetPawn())`. Bruk bare det som er `public` i `GSCharacter.h`; trengs noe privat: dropp
hintet og skriv det i rapporten.

| Id | Når | Tekst |
|---|---|---|
| `OwnTaken` | eget flagg `Carried` | «Flagget deres er tatt. Stopp bæreren, så blir det liggende.» |
| `Carry` | `PS->CarriedFlagTeam != 0` | «Med flagget går du saktere. Hold deg til dekning og få følge av laget.» |
| `EnemyFlag` | bærer ikke, motstanderens flagg er `Home`/`Dropped` og under 30 m unna | «Gå helt bort til flagget for å ta det.» |
| `Weapon` | `C->GetFocusPickup()` er et våpen (`GSItems::Find(ItemId)->IsWeapon()`), ikke `bLocked` | «%s tar våpenet. Du bærer fire. %s–%s eller rullehjulet bytter.» (`IA_Interact` E, `IA_Slot1` 1, `IA_Slot4` 4) |
| `Item` | fokus-pickup er noe annet, ikke låst | «%s legger tingen i sekken. %s blar, %s bruker, %s slipper.» (`IA_Interact`, `IA_NextItem` Q, `IA_UseItem` F, `IA_DropItem` G) |
| `Crate` | fokus-pickup `bLocked` | «Låst kasse. Adgangskort åpner den med en gang, dirkesett tar noen sekunder.» |
| `Computer` | `C->GetFocusTerminal()` er `EGSTerminalKind::Computer` | «%s: på PC-en ser du gjennom overvåkningskameraene.» (`IA_Interact`) |
| `Printer` | fokus-terminal er printer | «%s: printeren lager utstyr av filament du finner som loot.» |
| `Empty` | gjeldende våpen bruker ammo, magasinet er 0 og det finnes mer ammo | «Tomt magasin. %s lader.» (`IA_Reload` R) |
| `Limb` | `C->LegInjury > 0.3f` eller `C->ArmInjury > 0.3f` | «Skadd bein gjør deg treg, skadd arm gjør siktet ustøtt. Bandasje eller førstehjelpsskrin hjelper.» |
| `Hurt` | `C->Health < 60` | «Du er skadet. Velg medisin med %s og bruk den med %s. Det tar tid, så finn dekning først.» (`IA_NextItem`, `IA_UseItem`) |

- `Empty`: sjekk hvor reserven faktisk ligger (`FGSWeaponSlot::Reserve` eller felles lager per ammotype) og les den der.
- `Limb`: sjekk skalaen på `LegInjury`/`ArmInjury` i `GSCharacter.cpp` og sett grensen ved en merkbar skade.
- Ingen hint for sult/tørst (virker ikke i praksis, funn B2) og ikke for «eget flagg ligger ute» (Kamps radiolinje i
  `GSHUD::HandleFlagEvent` sier det allerede).

**Tegning:** én linje (del i to ved over 70 tegn), midtstilt, `Y = SizeY * 0.68`, skala 0.85, `TextColor` alfa 0.92,
`PanelColor`-boks bak med 10 px luft. Les `DrawPrompt`, radiolinja og hurtiglinja i `GSHUD.cpp` og flytt `Y` hvis noe
ligger der. Ingen ikon, ingen lyd, ingen blink.

### 5. `GSMenu.cpp`
1. `BuildRows`: der `Reserved` fylles (`Reserved->Add(EKeys::Escape)`), legg til `Reserved->Add(EKeys::H);`. Finn meldingen
   som vises når en reservert tast velges, og ta H med («Esc, F10, H, musa og hjulet …»).
2. `GSMenu::Controls()`: legg til `{ TEXT("Hjelp: taster og regler"), TEXT("H"), false }` blant de faste radene, og vis den
   også blant de faste linjene på kontrollsiden.
3. Kontrollsiden, ved `Key.Reset` («Tilbakestill alle taster»): ny knapp `Help.Reset` **«Vis hintene på nytt»** →
   `GSHelp::ResetHints();` og `KeyNote` = «Hintene vises på nytt.» (`#include "GSHelp.h"`).
4. Har en spiller fra før lagret en binding til H: la `ApplySavedKeys` hoppe over lagrede verdier som står i `Reserved`
   (originaltasten brukes), hvis det er enkelt; ellers noter det i rapporten.

### 6. `GSGameMode.cpp` – sen innkobling i hendelseslista
`RestartPlayer`: der `AssignTeam(PS)` kalles fordi laget er `None`, **og** fasen er `Playing`: start en timer på 2 s
(navnet fra klienten rekker fram) med svak peker til `PS`:
`GS->AddKill(PS->GetPlayerName(), FString(GSTeam::Name(PS->Team)).ToLower(), TEXT("ble med på"), false);`
→ «Kari [ble med på] rødt lag» hos alle. (`StartMatch` setter lagene før `RestartPlayer`, så kampstart gir ingen
falsk melding.) Spawn ved lagets base skjer allerede.

### 7. `IMPROVEMENTS.md`: ny seksjon øverst med linjene under. Ikke rør andres linjer.

### 8. Bygg
`Get-Process UnrealEditor` først. Bygg med kommandoen i `CLAUDE.md` (**med** `-WaitMutex`). Feiler det i andre lags
filer: vent og prøv igjen (inntil 4 ganger), ikke rett dem.

## Kontrollørens liste til byggeren (fra forhåndssjekken, delen som gjelder 3A)
1. Les `GSHelp.cpp`, `GSMenu.cpp` og `GSGameMode.*` slik de står i arbeidsmappen.
2. Lobby-lista over hint er skrevet ut: ingen flagghint i lobbyen.
3. `OwnDropped` er ute. Minst 60 s og «vilkåret har vært usant» mellom to visninger av samme hint. Tell etter 2 s.
4. `EKeys::H` i `Reserved` i `BuildRows` (ikke i `AssignKey`), og H-raden i `GSMenu::Controls()`.
5. Åpne ikke oversikten mens sekken er åpen.
6. Sen innkobling: sett «første tegning» etter at `GS` finnes, og start de 10 sekundene først når `PS` og laget er kjent.
7. `Empty`-hintet leser reserven der den faktisk ligger.
8. `ResetHints()` nullstiller også `OverviewOpened`.
9. Rapporter: hvilke taster PC-/printer-linjene ble sjekket mot, og hva som eventuelt ble tatt ut.

## Ferdig når
- Bygget går gjennom uten feil eller nye advarsler i lagets filer; `git status` viser bare filene i tabellen.
- Lest i koden: flagghint kan ikke vises utenfor `Playing`; et hint telles først etter 2 s; samme hint kan ikke komme
  to ganger innen 60 s; H kan ikke velges som tast på kontrollsiden.
- Alle taster i hinttekstene kommer fra `GSMenu::KeyName`.
- Det Andreas skal se (Play): **H** åpner og lukker «Taster og regler» med hans egne taster; se på en ting på bakken →
  ett kort hint nederst, som ikke kommer en tredje gang; «Kontroller» i menyen har «Vis hintene på nytt».
  Med to spillere: den som blir med etter kampstart ser «Kampen er i gang. Du er på …».

## Risiko
- **H** leses med `WasInputKeyJustPressed` i tegnekoden: virker bare for lokal spiller, og ikke mens `GSMenu` svelger
  innputt (ønsket). Skriver man i konsollen, går tasten dit.
- **Overlapp** med Kamps radiolinje og fokus-tekst: plasseringen må sjekkes mot `GSHUD.cpp` slik den er nå.
- **Flere PIE-vinduer** deler hint-tallene (samme ini). Greit.
- `bJoinedMid` blir sann for verten hvis HUD-en lages på nytt midt i en kamp (skjer ikke i dag).
- Ingen nye replikerte felt, ingen endring i regler: lav risiko for nett og balanse.

## IMPROVEMENTS-linjer
```
## Runde 3 (Spillbarhet): knappehjelp og hint (26. nov.)

460. H åpner en oversikt over alle taster, slik du selv har satt dem i Innstillinger. H lukker den igjen
461. Oversikten forklarer Capture the Flag i seks korte linjer, og hva tastene gjør på en PC og en 3D-printer
462. En liten påminnelse «H: taster og regler» står nede til høyre de første sekundene, til du har brukt oversikten
463. Hint første gang du ser på et våpen, en ting, en låst kasse, en PC og en 3D-printer, og når magasinet er tomt
464. Hint når du er skadet eller har fått en skade i arm eller bein, og om flagget: når du er nær det, bærer det, eller deres eget er tatt
465. Hvert hint vises høyst to ganger, aldri to ganger på rad i samme situasjon, og huskes til neste gang du spiller
466. Hintene viser tastene du faktisk har valgt. H er satt av til hjelpen, og «Vis hintene på nytt» i menyen henter hintene fram igjen
467. Den som kommer inn midt i en kamp får vite laget sitt og stillingen, og de andre ser hvem som ble med på hvilket lag
```
