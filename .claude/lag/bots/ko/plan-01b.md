# Plan 01b (lag Bots): bots som overlever – dekning, medisin, hørsel, deling, ytelse

Andre halvdel av den arvede plan-01, skrevet om etter kontrollørens forhåndssjekk (`kontroll/forhand-plan-01.md`).
Bygges **etter at 01a er godkjent**. Bygger på `AGSBotController`, `AGSBotTeam`, `GSNavSetup` og ferdighetstabellen fra 01a
(feltene `HearM`, `CoverUse`, `RetreatHealth` står der allerede).

## Krav før byggeren starter
1. 01a er godkjent av kontrolløren (`kontroll/runde-01a.md`), og funnene derfra er rettet eller tatt inn her.
2. Kamps K1 er i HEAD (sjekket i 01a). `friend class AGSBotController` gir lesing av `ServerLastFireTime` (hørsel) og kall til
   `ServerUseItem`/`ServerCancelUse`/`ToggleCrouch`.
3. Les 01a-loggen: «sitter fast ved»-linjer, via-punkter per etasje og tenketiden. Over 1,0 ms tenking i 01a → kutt budsjettene
   i steg 6 før nye sanser legges til.

## Mål
Botene oppfører seg som soldater som vil overleve: de går i dekning når de blir beskutt, skyter fra huk bak lave ting,
bandasjerer seg når de blør, trekker seg unna hardt såret, snur seg mot skudd de hører og roper til hverandre hva de ser –
og 19 bots holder bildefrekvensen på RTX 3050-laptopen.

## Forbedrer (gammelt)
- 01a-botene: sikte og skudd beholdes, men nå lader de i dekning, bytter til pistol når geværet er tomt på nært hold, og sikter
  mot stedet de mistet deg.
- 01a-lagets hjerne (`AGSBotTeam`): bæreren får en eskorte, og laget deler det én bot ser med lagkamerater innen 25 m.
- Kamps blødning og Spillbarhets bandasjer/medisin (`GetBleedRate`, `ServerUseItem`, `UseItemId`) brukes nå av botene som av
  spillere, med samme tid og samme sårbarhet.

## Nytt
- Dekning og skuddstilling funnet fra navnettet, uten koordinater i koden.
- Medisin og tilbaketrekking.
- Hørsel: skudd (kortere for lyddempet) og løping i nærheten.
- «Roping» innen 25 m og enkel eskorte av flaggbæreren.
- Ytelsesmåling med 19 bots etter kontrollørens krav (tenketid, spilltråd med og uten tenking, absolutt `stat unit`).

## Hvorfor nå
01a gir bots som går og skyter. Uten dekning og medisin står de i åpne korridorer og dør – det føles som sikteroboter, ikke
mennesker. Ytelsen må måles før 02 legger på taktikk.

## Eierskap
Bare `GSBotController.*`, `GSBotTeam.*`, `GSBots.*`, `GSNavSetup.*` og `IMPROVEMENTS.md` (694–709). Ingen endring i andre
lags filer. Medisin, våpen og blødning brukes bare gjennom eksisterende offentlige/`friend`-handlinger.

## Steg

### 1. Dekning og skuddstilling (`GSNavSetup::FindCover` + `AGSBotController`)
1. `bool GSNavSetup::FindCover(UWorld*, FVector From, FVector ThreatEye, float RadiusCm, FVector& OutSpot, bool& bOutFirePos)`:
   12 nåbare punkter rundt `From` (`GetRandomReachablePointInRadius`, radius 8 m). Et punkt er **dekning** når objektsporet
   (`ECC_WorldStatic`, **ikke** hopp over glass – glass er ingen dekning) fra `ThreatEye` til punktet + 150 cm er blokkert, og
   **skuddstilling** når + 100 cm er blokkert men + 150 cm er fritt. Nærmeste skuddstilling foran nærmeste dekning.
   Punkter nærmere fienden enn boten forkastes.
2. Utløses når boten blir truffet (`Health` synker), ser en fiende den ikke straks kan skyte (sjanse `CoverUse`), eller må
   lade med fiende synlig over 10 m. Løp dit (sprint hvis > 4 m).
3. I skuddstilling: huk (`ToggleCrouch`) mens den lader eller venter, reis seg og skyt i 1–2,5 s (skuddlinja fra 01a gjelder),
   huk igjen. I ren dekning: vent til fienden er usett i 2 s, så videre mot rollens mål.
4. Felles budsjett i `AGSBotTeam`: høyst 2 dekningssøk per bilde for alle bots; den som ikke rekker, prøver neste bilde.

### 2. Medisin og tilbaketrekking
1. Blør (`GetBleedRate() > 0,3`) og ingen fiende sett siste 3 s: gå i dekning (eller stå stille), velg i sekken den første
   gjenstanden som stopper blødning (katalogens `Kind`/bandasje-Id-er) og `ServerUseItem(Index)`. Ikke skyt eller løp før
   `UseItemId` er `None`. Fiende innen 15 m dukker opp → `ServerCancelUse()` og slåss.
2. Helse under 60 og trygg (ingen fiende sett siste 5 s): bruk det som gir mest helse.
3. Helse under `RetreatHealth` med fiende synlig: trekk mot egen base eller nærmeste lagkamerat til helsa er over igjen eller
   fienden har vært usett i 6 s. Flaggbæreren trekker seg aldri bort fra hjemveien (bytter bare til et annet via-punkt).

### 3. Hørsel (5 Hz, sammen med synet)
1. Fiende innen `HearM` som har skutt siste 0,3 s (`ServerLastFireTime` via `friend`; lyddempet våpen: 40 % av `HearM`):
   husk retning og omtrentlig sted (±3 m tilfeldig), snu blikket dit og gi målet en ny, full startfeil når det blir sett.
2. Fiende som løper (fart > 450 cm/s) innen 10 m: husk som «hørt fottrinn» (±2 m).
3. Hørsel gir aldri skuddlinje eller sikte gjennom vegg – bare hvor boten ser og går.

### 4. Deling og eskorte (`AGSBotTeam`)
1. En fiende en bot har **sett** siste 3 s deles med lagkamerat-bots innen 25 m («roper»), som husket sted (ikke synlig nå).
   Aldri lengre enn 25 m, aldri gjennom radio – realistisk, ikke radar.
2. En bot bærer flagget: én angriper nærmest ham blir eskorte (går 6 m bak, sikter mot sidegangene, stopper når bæreren
   stopper). Full eskorte med to og flanke er plan-02.

### 5. Våpen og sikte etter tap av mål
1. Geværet tomt og fienden innen 15 m: `SelectSlot` til pistolen (raskere enn å lade); tilbake til geværet og `Reload()` når
   fienden er borte.
2. Mistet mål: sikt mot sist sette sted i 2 s (fra 01a), og gå dit hvis rollen er forsvar og stedet er innen 12 m av flagget.

### 6. Ytelse (kontrollørens nye krav, punkt 4 i forhåndssjekken)
Ikke «høyst 10 % fall» – 19 ekstra figurer koster det samme som 19 mennesker. Mål i stedet, og logg tallene med `UE_LOG`:
1. **Botenes tenking** ≤ 1,5 ms per bilde med 19 bots (`STAT_GSBotThink` fra 01a, snitt og topp over 30 s).
2. **Spilltråden med 19 bots som tenker** mot **19 bots med `gs.bots.think 0`** (står stille, samme kropper) – forskjellen er
   botenes egen pris. Skriv begge `stat unit`-tallene (Game, Draw, GPU, Frame).
3. **Absolutt `stat unit`** i atriet (P4, ser over atriet), 1080p, Middels, du + 9 mot 10.
4. Tallene i `logg.md` og rapporten. Holder ikke (1): skru ned syn til 4 Hz og sporbudsjettet til 32 per bilde, mål på nytt.
5. Sjekk at bots ikke har førstepersonskropp eller dobbel animasjon (K1): `stat anim` – antall animerte meshes ≈ antall figurer.

## Ferdig når
- [ ] Bygger uten feil; ingen endring i andre lags filer.
- [ ] **PIE, du + 4 mot 5, Middels:** en bot som blir skutt i en korridor går i dekning eller skuddstilling (huk/reis) innen 2 s;
      en bot som blør og er trygg bandasjerer seg (animasjon/tid som spillere) og avbryter når du kommer nær.
- [ ] En bot som hører deg skyte bak seg innen 30 m snur seg mot lyden; med lyddemper skjer det bare innen ca. 14 m.
- [ ] En bot som ser deg roper det til lagkamerater innen 25 m (de snur seg mot deg uten å ha sett deg); bots lenger unna gjør det ikke.
- [ ] Bæreren har en bot 6 m bak seg på vei hjem.
- [ ] Hardt såret bot (Vanskelig) trekker seg mot egen base; Lett gjør det ikke.
- [ ] Ytelsestallene (6.1–6.3) står i loggen; tenking ≤ 1,5 ms.

## Risiko
- **Dekningssøk er dyre** (12 punkter × 2 spor): budsjettet på 2 per bilde holder det nede; mål i 6.1.
- **Bots som gjemmer seg for alltid:** ren dekning slippes etter 2 s usett; forsvarere går tilbake til punktet sitt.
- **Medisin som låser boten:** avbryt ved fiende innen 15 m; tidsgrense = gjenstandens brukstid + 1 s.
- **Hørsel som veggsyn:** gir bare retning og omtrentlig sted, aldri sikte – samme som et menneske med hodetelefoner.
- **Huk via `ToggleCrouch`:** er en veksler; boten holder egen tilstand og sjekker `bIsCrouched` før kallet.

## IMPROVEMENTS-linjer (seksjon «Runde 1b (Bots): bots som overlever»)
694. Botene søker dekning bak vegger og faste ting når de blir beskutt, i stedet for å bli stående i åpne korridorer
695. Botene skyter fra skuddstillinger: de huker bak lave ting, reiser seg for å skyte og dukker ned igjen
696. Botene lader om i dekning når fienden ser dem, ikke midt i gangen
697. Botene bandasjerer seg når de blør og er trygge, og bruker like lang tid som du
698. Botene tar medisin for å få tilbake helse når det er rolig, og avbryter hvis en fiende dukker opp
699. Hardt sårede bots trekker seg tilbake mot egen base eller lagkamerater, og kommer tilbake når de er lappet sammen
700. Botene hører skudd i nærheten og snur seg mot lyden
701. Lyddempede våpen høres på mye kortere avstand, også for botene
702. Botene hører deg løpe like ved, men ikke når du går rolig
703. Botene roper det de ser til lagkamerater innen 25 meter, slik spillere gjør, uten å se gjennom vegger
704. En bot følger etter flaggbæreren som eskorte på vei hjem
705. Botene bytter til pistolen når geværet er tomt og fienden er nær, i stedet for å lade midt i skuddvekslingen
706. Botene sikter mot stedet de mistet deg i et par sekunder, og forsvarere sjekker det når det er nær flagget
707. Vanskelighetsgradene skiller også på hørsel, hvor ofte botene bruker dekning og når de trekker seg tilbake
708. Botenes tenking er spredt over flere bilder og har tak på antall sjekker, så 19 bots holder bildefrekvensen
709. Ytelsen med bots måles i atriet: botenes egen tenketid, spilltråden med og uten tenking, og bildetid, skrevet i loggen
