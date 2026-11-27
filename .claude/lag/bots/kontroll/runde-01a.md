MÅ FIKSES

# Kontroll av Bots runde 01a (commit `c10dc9e`)

Kontrollert mot `ko/plan-01a.md` og forhåndssjekken `kontroll/forhand-plan-01.md`. Lest: hele commiten (`git show c10dc9e:…`
for alle åtte filer), K1 i `GSCharacter` på samme commit, `GSMapState`/`GSDoor` (dørsynk), `GSAudio`/`GSEffects` (lasting),
motorkoden i UE 5.8 (`DestroyComponent`, `NavigationSystem.cpp`, `BaseEngine.ini`) og loggene `bots2/5/7/8.log`. Ikke bygget
(byggeren meldte Succeeded).

Grunnmuren er god: navnettet virker (88 fliser på 3–6 s, sti fra alle 16 spawn til fiendens flagg, 36 av 54 døråpninger
gangbare, de 18 andre er låste/lukkede dører og dør 31), via-punkter på alle fire etasjer og begge sider, 6 forsvarspunkter
per base, K1 er på plass (`IsLocalHuman`, `friend`, sikte med kontrollrotasjon, kamera snudd for AI), og botene bruker
spillernes egne handlinger. Ingen andre lags filer er endret. Men det er tre ting som må rettes før «Spill mot bots» slås på:
ett hull som gir veggsyn med 20 bots, sikte som blir perfekt etter et par sekunder, og CTF-kriteriet (erobring) er ikke vist.

## Andreas' faste regel
- **Forbedret:** «Spill mot bots» fyller nå lagene etter `BotAllies`/`BotEnemies`/`BotSkill`; gjenoppliving, lag, startutstyr,
  flaggradioen og drapslista gjelder også datastyrte spillere (`RestartPlayer` uten endring i GameMode).
- **Nytt:** navnett laget i kode med selvtest, botkontroller (syn, sikte, skudd, bevegelse), lagroller, fire grader,
  `gs.bots.*`-kommandoer. Regelen er oppfylt.

## Funn – må fikses

**F1. Veggsyn når synsbudsjettet er brukt opp (skjer med 20 bots).** `GSBotController.cpp` l. 232: når
`TakeSightTraces(3)` sier nei, hopper boten over fienden og lar `M.bVisible` stå som før. Samtidig settes `NextSightAt = Now + 0,2`
(l. 163) uansett. En fiende som var synlig blir da stående «synlig» til neste vellykte sjekk, og i mellomtiden bruker
`AimPointFor` (l. 313) og `PickTarget` fiendens **levende** posisjon: boten følger deg med siktet gjennom veggen og er ferdig
siktet når du kommer fram (skuddlinja stopper skuddet, men ikke siktet). Med 9 bots skjer det nesten aldri (2 bots × 5 fiender
per synsbilde = 10 av 16 plasser), men med 19 bots og 10 fiender i synsfeltet er det ~4 bots × 10 = 40 sjekker mot 16 plasser per
bilde, og det er **de samme** fiendene (siste i `TActorIterator`) og de samme botene (sist i tick-rekkefølgen) som sultes hver
gang. Rett: (a) når budsjettet er brukt, prøv igjen neste bilde (ikke vent 0,2 s); (b) en fiende teller som synlig bare når den er
sporet de siste ~0,3 s (`Now - M.LastSeen <= 0.3f` i `PickTarget`, `UpdateAim`, `UpdateFire`), ellers sikt mot `LastLoc`;
(c) start fiendelisten på et roterende sted (f.eks. `BotIndex + teller`), så ingen sultes fast.

**F2. Siktet blir perfekt etter 2–3 s, og botene slipper siktestøyen mennesker har.** Feilen halveres hver `AimSettleHalfLife`
helt mot null (l. 348): Lett 4,0° → 0,4° etter 3 s → 0,09° etter 5 s (5 cm på 30 m); Middels 2,5° → 0,08° etter 3 s.
Samtidig får botene ingen av forstyrrelsene fra `AGSCharacter::Tick`: `TickRecoil` og `TickScopeSway` (pust i kikkert, skjelving
fra skutt arm) kjører bare for `IsLocalHuman()` (`GSCharacter.cpp` l. 4596–4605), rus-svaien i `UpdateLocalVisuals` likeså, og
`RInterpConstantTo` mot ønsket retning (200–450 °/s) trekker resten av spillets svai tilbake på noen få millisekunder. Det eneste
som gjenstår er våpenets spredning. Det strider mot planens mål («ikke som sikteroboter») og kriteriet «på 30 m bommer Lett og
Middels ofte». Rett i botens filer: (a) et gulv for siktefeilen per grad (nytt felt i `FGSBotSkill`, f.eks. Lett 1,2°, Middels 0,7°,
Vanskelig 0,4°, Ekspert 0,2°) som vandrer sakte (ny retning hvert 0,5–1 s, myk overgang), ganget opp ved bevegelse (×1,8 over
50 cm/s), med `GetArmPain()` og når boten selv er truffet; (b) litt sideveis rekyl i tillegg til pitch; (c) logg treff/skudd per
avstandsbånd (0–10, 10–25, 25+ m) og grad hvert 30. s (UE_LOG), så en hodeløs kjøring viser at Lett/Middels bommer ofte på 30 m og
Ekspert sjelden.

**F3. CTF er ikke vist: 0 erobringer i alle tre kjøringene.** «Ferdig når» krever at en bot tar flagget og bærer det hjem (erobring
telles). Loggene viser «tatt», «mistet», «levert tilbake» (`bots8.log`), men ingen «erobret» – bæreren blir skutt underveis, så
koden er ikke motbevist, men heller ikke bevist. Kjør en hodeløs test der mennesket står stille og eget flagg er uforsvart, f.eks.
`gs.bots.fill 0 5 1` (fem fiender, ingen venner), og vis «Flagg: … erobret» i loggen innen ca. 3 min. Kommer den ikke: finn
årsaken (rute hjem, `CarryHome`/`ViaThen`, krav om eget flagg hjemme) før godkjenning.

## Funn – bør fikses (stopper ikke runden)

**B1. Årsaken til tenketoppene (skudd/skuddlinje 10–17 ms, 33 ms i bots7).** Ikke botens hjerne. Delen «skudd/skuddlinje»
(`Part[3]`, l. 177) måler hele `UpdateFire`, også `C->StartFire()` → `FireOnce` med lyd, effekter, treff og eventuelt drap. Begge
toppene i `bots8.log` (bilde 365 og 11) ligger i **samme bilde** som en `LogStreaming: FlushAsyncLoading` – synkron lasting av en
pakke midt i spillet. Det er 165–182 slike per kjøring. Sannsynlig kilde: Kamps lager med `LoadObject` og **svake** pekere
(`GSAudio.cpp` `LoadExact`, l. 210–231; `GSEffects.cpp` l. 15–46): søppeltømmingen frigjør lyden/materialet, og neste skudd laster
det synkront på nytt. Selve linjesporet i `LineOfFire` er ett spor + en løkke over 20 figurer, langt under 1 ms. Snittet er godt:
0,18–0,36 ms per bilde for 9 bots (≈ 0,4–0,8 ms for 19). Gjør: (a) **bestilling til Kamp:** hold lydene og FX-materialene med
sterke referanser (`UPROPERTY` i et delsystem eller `TStrongObjectPtr`) så de ikke lastes om; (b) i boten: mål handlingene
(`StartFire`, `SelectSlot`, `Reload`, `ServerInteract`, `ServerOpenAway`) for seg, så tenketiden i 01b (≤ 1,5 ms) måler hjernen.

**B2. Stivalget på 7,7 ms** (Ingrid, `bots8.log` bilde 88) skjedde **før navnettet var klart** (klart i bilde 196): `Decide` kaller
`MoveToLocation` 4 ganger i sekundet per bot mens flisene bygges, og hver gang feiler stien. Gjør: (a) ikke kall `GoTo` før
`GSNavSetup::IsNavReady(World)`; (b) høyst én ny sti per bot per sekund og et felles tak (f.eks. 2 stisøk per bilde) – særlig når
angripere vipper mellom «stå» og «gå» rundt 20 m-grensen i kamp; (c) `bNoPathLogged` (l. 755) settes én gang og nullstilles aldri,
så den første feilen før navnettet er klart skjuler alle ekte «ingen sti» senere – nullstill den når navnettet blir klart.

**B3. Navnettets agent er mindre enn kroppen.** `DefaultEngine.ini`: `AgentRadius=25`, `AgentHeight=160`; kapselen er radius 40 og
høyde 184 (`GSCharacter.cpp` l. 207). Stiene går 25 cm fra vegger og hjørner der kroppen trenger 40, og nettet godtar steder med
1,6–1,84 m takhøyde der figuren ikke får plass. Byggerens grunn (5 cm celler så 0,9 m dører holder) er god, men med 5 cm celler gir
radius 35 fortsatt 20 cm stripe i en 0,9 m dør. Prøv `AgentRadius=35`, `AgentHeight=184` og kjør dørtesten på nytt. Loggene har 3–4
«sitter fast» per 2 min, så det haster ikke.

**B4. Botene ser gjennom røyk.** Røykgranaten lager en sky i 20 s (`GSProjectile.cpp` l. 319 → `AGSShortFX::SpawnSmokeCloud`), men
synssporene (`ECC_Visibility`) går rett gjennom. Det er et menneske ikke kan. Trenger en liste over aktive skyer (bestilling til
Kamp, f.eks. `GSEffects::ForEachSmoke(World, Fn)` med sentrum/radius/slutt; Interaktiv har allerede tilbudt `ForEachCloud` for
brannslukkere) og en sjekk av linje mot kule i `UpdateSight`. Naturlig i 01b.

**B5. Utenfor planen (ufarlig):** `Gloshaugen.Build.cs` legger til `RenderCore`/`RHI` for Grafikks `GSFoto.cpp`, som ikke er
committet. Påvirker ikke botene; nevn det i loggen så Grafikk vet at avhengigheten alt er inne.

## Det dirigenten ba meg sjekke særlig

**Krasjen i dørsynken kan ikke skje i den committede koden.** Krasjen (`bots2.log` 10:42) var `EXCEPTION_ACCESS_VIOLATION reading
0x0000008000000000` i **konstruktøren til TMap-iteratoren** for `DoorBlocks`, fra `RunSelfTest` → `SyncLockedDoors` (l. 645 i en
eldre versjon av fila). En iterator leser bare kartets eget minne; den kan bare feile når selve objektets minne er feil – typisk når
ett kompilert stykke ser en eldre header (felt lagt til, objektet laget med gammel størrelse). Det stemmer med «utdatert bygg». I
`c10dc9e`: `DoorBlockHolder` og `DoorBlocks` er `UPROPERTY` (GC nuller dem), verdien null-sjekkes før `DestroyComponent`, og
`DestroyComponent` tåler å kalles to ganger (`bIsBeingDestroyed`, `ActorComponent.cpp` l. 2156). Ingen gjeninngang endrer kartet
mens det gås gjennom: botens `ServerOpenAway(…, bBreakLock=false, …)` sender ikke `OnWalkableChanged` (`GSDoor.cpp` l. 1022), og
`RegisterComponent`/`DestroyComponent` kaller ikke tilbake. `ForEachDoor` går bare gjennom `MS->GetDoors()`, som bare endres ved
registrering. bots5/7/8 kjørte synken hvert sekund i flere minutter uten krasj. Liten ting: `static int32 LastLogged` (l. 716)
deles mellom PIE-økter – bare loggen.

**Veggsyn og juks:** Syn sporer fra kameraet til hode/bryst/hofte, hopper bare over `Mat_Glass`, ignorerer egen figur og
lagkamerater – riktig. Hukommelsen lagrer bare det som er sett; flaggbæreren bare via `bRevealed`/`RevealLocation`, sluppet flagg
via `DropLocation` (samme som HUD-en). Reaksjonstid før første skudd, skuddlinje på kulekanalen uten å hoppe over glass (så botene
skyter ikke i fast glass), lagkamerat nær linja stopper skuddet. «Kule gjennom glass» i loggene er knusbart glass som ignorerer
`Visibility` og som kulene går gjennom med tap (`GSBallistics.cpp` l. 328) – likt for mennesker. Hullene er F1 (sultet budsjett),
F2 (null siktestøy) og B4 (røyk). 360° innen 3 m er greit inntil hørselen kommer i 01b.

**Ytelse med 20 bots:** Hjernen skalerer bra (syn 5 Hz med felles tak, beslutning 4 Hz, forskjøvet; `LineOfFire` billig;
`LookForAmmo` 1 Hz). Risikoene er F1 (budsjettet sulter fast), B2 (synkrone stisøk uten tak, 5 cm celler gir mange polygoner) og
B1 (synkron lasting i skuddveien, som rammer mennesker også, men oftere med mange som skyter). Kroppene (19 × CharacterMovement og
animasjon) koster som 19 spillere, som forhåndssjekken sa.

**DefaultEngine.ini påvirker nesten ingenting utenfor bot-kamp.** `bAutoCreateNavigationData=true` og
`bAllowClientSideNavigation=false` var allerede motorens standard (`BaseEngine.ini`), og kartet hadde navigasjonssystem fra før.
Navdata får fliser bare der det finnes et grensevolum, og det lages bare av `EnsureNavMesh` i bot-kamp (eller med `gs.bots.fill`).
Online og «Test kartet» får derfor en tom RecastNavMesh som før, bare med agenten «Soldat» og `Dynamic`. `bDrawPolyEdges` virker
bare med `show Navigation`. `TilePoolSize=1024` er små hoder, ikke store minneblokker. I bot-kamp betyr `Dynamic` + `Lazy` at ting som
endres (knust glass, låste dører) bygger fliser på nytt i bakgrunnen; loggen har én ufarlig advarsel `LogNavigationDirtyArea: Skipped
… GSGlassSet` (tom boks). Oppstartsmålingen online før/etter som planen ba om, er ikke rapportert – ta den med i 01b.

## «Ferdig når», punkt for punkt
- Krav K1/4a: K1 står i `c10dc9e` (sjekket i `GSCharacter`); 4a/05a: `NavBoundsM()` og `DoorSlots()` brukes. Commitnumrene står
  ikke i lagets logg – før dem inn.
- Bygger og loggen viser navnett klart, selvtest for begge lag (0 uten sti, forskjell 1,8 m), dører 36 av 54, via-punkter på 4
  etasjer og begge sider, 6 forsvarspunkter per base, tenketid hvert 30. s: **ja**.
- Bot tar flagget og bærer det hjem: **ikke vist** (F3).
- Sikte i høyden, fast glass, lagkamerat i skuddlinja, bommer på 30 m: koden er riktig bygget opp (K1 + `LineOfFire`), men ingen
  test viser det; F2 (c) gir tallene.
- Bots dør, blir liggende, gjenoppstår, «Navn (bot)»: loggene viser drap = døde og navn med «(bot)»: **ja**.
- Ingen endring i andre lags filer: **ja** (unntatt B5, ufarlig).

## Forslag til neste runde
1. 01a-fiks: F1–F3 (alle i lagets egne filer), pluss B2 (a) og (c) – små og billige.
2. 01b: hørsel, dekning og røyk som stenger syn (B4), og ytelsesmålingen med 19 bots der hjernen og handlingene måles hver for seg (B1 b).
3. Bestilling til Kamp: sterke referanser for lyd og FX som lastes i skuddveien (B1 a) – fjerner hakk for alle, ikke bare bots.

## Lærdom
- Et felles budsjett som hopper over arbeid må ikke la gammel tilstand stå som sann: det som ikke ble sjekket er ukjent, og den
  som ble hoppet over må få første plass neste gang.
- AI som sikter selv slipper spillets siktestøy (svai, smerte, rus, rekyl), fordi den kjører bare for lokale mennesker: boten må
  lage tilsvarende støy selv, og feil som «krymper» trenger et gulv.
- Tidsmåling av AI må skille hjernen fra handlingene den utløser (skudd, våpenbytte, dører); en topp i samme bilde som
  `FlushAsyncLoading` er synkron lasting, ikke tenking.
