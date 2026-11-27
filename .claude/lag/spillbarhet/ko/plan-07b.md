# Plan 07b – lag Spillbarhet: regnemotoren for stoffer (innslag, nedtur, belastning, overdose, Nalokson)

Bygges rett etter at 07a er godkjent. Headeren fra 07a er da på plass, så andre lag slipper å bygge alt på nytt. Bare
`GSItems.cpp` får ny kode, pluss én ny ting i katalogen (Nalokson). Rettet etter forhåndssjekken
`kontroll/forhand-plan-07.md`, punkt 1–4, 9 og 11.

Det meste av dette synes i spillet først når Kamp kobler på K-FX (1) og (2) i `bestillinger.md`, altså når `GetMods`
kaller `CombineActive` og `AddEffect` teller doser. Før det kan alt prøves med `gs.fx.test`.

## Mål
Hvert stoff slår inn gradvis og har en topp og en nedtur. Tar du for mye av ett stoff, eller blander farlige stoffer,
blir du først uvel, så forgiftet, og til slutt dør du av overdose. Nalokson kan redde deg fra opioider en stund.

## Forbedrer (gammelt)
- Reserveversjonene fra 07a byttes ut med de ekte: `EffectStrength`, `CombineActive`, `DrugLoad` og `ActiveVisuals`.
- **Taket klemmes én gang til slutt.** Effektene kombineres først uklemt med `CombineRaw`, så kalles `ClampMods`. Da
  avhenger ikke resultatet av rekkefølgen effektene står i.
- Effektene slås ikke lenger av og på brått.

## Nytt
- Innslag, topp og nedtur for hvert stoff.
- Belastning og overdose, med myke overganger mellom trinnene.
- **Nalokson nesespray** (Medicine, Rare).
- Skjermlag til Kamps plan-10 (`ActiveVisuals`).
- Testene (a)–(g) og (k) i `gs.fx.test`.

## Filer
- `Private/GSItems.cpp`
- `Tools/ItemCatalog/catalog.py` (Nalokson-tingen)
- generert `GSItemCatalog.gen.inl`
- `IMPROVEMENTS.md`
- `bestillinger.md` (bare statuslinjer)

## Steg
1. **Katalogen.** Legg til
   `med("Naloxone", "Nalokson nesespray", "Rare", 2.0, 0, "Nalokson", dict(arch="dropper", color=(0.9, 0.9, 0.95)), "...")`.
   Beskrivelsen sier at den opphever opioider en stund, men at morfinen kommer tilbake. Kjør `gen_cpp.py`.
2. **`EffectStrength(Def, Active, Now)`.**
   - Start er `Active.StartTime` når den er større enn 0, ellers `EndTime − Duration`.
   - Styrken stiger lineært fra 0 til 1 over `Onset`, er 1 på toppen, og synker lineært over `Fade` før `EndTime`.
   - Utenfor tidsrommet er styrken 0. Er `Onset` eller `Fade` 0, hoppes den rampa over.
   - **En ny dose nullstiller ikke rusen.** Kamp beholder `StartTime` og forlenger bare `EndTime` (K-FX 2). Da står
     innslaget fortsatt på full styrke og blir ikke borte et øyeblikk.
3. **`DrugLoad(Effects, Now)`.**
   - Hver klasse får summen av `Load · max(1, Doses) · styrke`.
   - Opioider teller 0 mens en Antidote har styrke ≥ 0,5.
   - Er både Opioid og (Depressant eller Alcohol) over 0, ganges de tre med **1,5**. Det er pustestans, den farligste
     blandingen i virkeligheten.
   - Er Stimulant over 0 og det finnes **en annen** Stimulant-effekt **eller Caffeine**, ganges Stimulant og Caffeine
     med **1,25**. Koffein forsterker amfetamin.
   - Svaret er summen av alle klassene.
4. **Belastningstrinn.** Bare det høyeste trinnet gjelder, og overgangen er myk.

   | Trinn | Terskel T | Fart | Spredning | Stamina-henting | Ladetid | Helse | Skjerm |
   |---|---|---|---|---|---|---|---|
   | Uvel | 1,2 | – | ×1,15 | ×0,8 | – | – | 3 skjelving |
   | Forgiftet | 2,0 | ×0,85 | ×1,4 | – | ×1,3 | −0,5 HP/s | 1 uklart |
   | Overdose | 3,0 | ×0,5 | ×1,4 | – | ×1,3 | −2,5 HP/s | 10 tunnelsyn |

   - Trinn k er det høyeste der L > T_k − 0,2.
   - Vekten er `w = clamp((L − (T_k − 0,2)) / 0,2, 0, 1)`. Full vekt kommer altså ved L ≥ T. Med morfins load på 1,05
     ligger eksemplene klart over tersklene.
   - Følgene blir `lerp(trinn k−1, trinn k, w)`. Trinn 0 betyr ingen følger.
   - Det gir ingen brå sprang i fart (viktig for nettet: `MaxWalkSpeed` regnes på både klient og server) og ingen
     blafring ved fadingen.
   - På full overdose dør man på 40 s fra 100 HP. Det gir tid til Nalokson, eller til at stoffet går ut.
5. **`CombineActive(Effects, Now)`.**
   - Ta hver effekt med `EndTime ≥ Now`.
   - Styrken s skalerer verdiene:
     - multiplikatorer blir `1 + (m − 1) · s`
     - `Regen`, `Drain` og `Lifesteal` ganges med `s`
     - flagg og `Visual` gjelder bare når s ≥ 0,5
   - Opioider hoppes over mens motgiften har styrke ≥ 0,5.
   - Kombiner alt med `CombineRaw` og kall `ClampMods` **én gang** til slutt.
   - Legg deretter på belastningstrinnet som egne ganginger og `Drain`. Overdose skal kunne gjøre deg tregere enn taket
     ellers ville tillatt.
   - Unngå heap-tildelinger. `GetMods` kalles ofte.
6. **`ActiveVisuals(Effects, Now, Out)`.**
   - Ett lag `{Visual, s}` per effekt med `Visual ≠ 0` og s > 0,05.
   - Pluss et lag for belastningstrinnet, med `w` som styrke.
   - Sorter sterkest først.
7. **`gs.fx.test`, del 2.** Legg til disse linjene. Alle tester bygger kunstige `TArray<FGSActiveEffect>`.
   - (a) Morfin ved t = 0, 4, 8, 100 og 120 s gir styrke 0 / 0,5 / 1 / 1 / 0.
   - (b) Morfin ×2 (Doses = 2) gir L = 2,1 og trinnet Forgiftet med w = 1.
   - (c) Morfin ×3 gir L = 3,15 og Overdose, med `Drain` = 2,5 ±0,01. Bare det høyeste trinnet gjelder.
   - (d) Morfin + Alkohol ×2 gir L = 3,075 og Overdose.
   - (e) Morfin ×3 + Nalokson med s = 1 gir L < 1,0 (bare motgiften er igjen).
   - (f) Amfetamin ×2 + KoffeinPlus gir L = 2,375 og Forgiftet.
   - (g) Sigarett + Snus + Nikotinplaster gir L < 1,0.
   - (k) Morfin med Doses = 2 og StartTime 50 s tilbake (EndTime forlenget) gir styrke 1, ikke 0.
   - (l) L = 2,9 gir en Overdose-vekt mellom 0 og 1, og fart mellom 0,85 og 0,5. Det er den myke overgangen.
8. **Bygg** med `-WaitMutex`. Kjør `gs.fx.test` hodeløst. Alle linjene fra 07a og 07b skal være OK. Lim dem inn i
   rapporten.
9. **Bestillinger.** Skriv i K-FX-linjen i `bestillinger.md` at motoren er levert, så Kamp kan koble på.
10. **IMPROVEMENTS.md:** lag seksjonen «Runde 7b (Spillbarhet): rus, overdose og motgift» med linjene under.

## Ferdig når
- Bygget går uten feil, og `gs.fx.test` viser alle linjer OK.
- Ingen headere er endret i 07b.
- Nalokson finnes i loot, med Rare-vekt i Medical.

## Risiko
- **Nett:** klient og server regner ut fra de samme replikerte dataene. Klientens `GetServerTime` er litt forskjøvet,
  men de myke trinnene gjør at det bare gir små avvik i fart, ingen sprang.
- **Ytelse:** løkken går over 0–8 effekter med `FindEffect`-oppslag. Bruk en liten fast tabell per klasse, ingen
  `TArray`-tildeling.
- **Før Kamps krok (1)** synes ingenting av dette i spillet, bortsett fra at Nalokson kan tas uten virkning. Skriv det
  i rapporten.
- **Før Kamps krok (2)** er Doses alltid 1. Da gir bare blandinger overdose, ikke gjentatte doser av samme stoff.

## IMPROVEMENTS-linjer
933. Stoffer slår inn gradvis, når en topp og har en nedtur, i stedet for å slås av og på brått
934. En ny dose av samme stoff forlenger rusen uten å starte innslaget på nytt
935. Overdose: tar du for mye av ett stoff blir du uvel, så forgiftet, og til slutt dør du hvis ingen hjelper
936. Blandinger er farlige som i virkeligheten: morfin med alkohol eller sovetablett gir pustestans, og koffein forsterker amfetamin
937. Overgangene mellom uvel, forgiftet og overdose er myke, så farten ikke rykker (også online)
938. Ny: Nalokson nesespray opphever en opioid-overdose en stund, men morfinen kommer tilbake når den går ut
939. Taket på boosts regnes én gang til slutt, så rekkefølgen du tar stoffene i ikke betyr noe
940. Skjermeffekter klare for hvert stoff: rødt ved røyk, hvitt ved amfetamin, tunnelsyn ved adrenalin og overdose, dobbeltsyn ved alkohol (tegnes av Kamp)
941. Selvtest `gs.fx.test` sjekker innslag, doser, blandinger, overdose, motgift og myke overganger
