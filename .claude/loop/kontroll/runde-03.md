**GODKJENT**

Runde 3 – blod (commit `f19d059`). Kontrollert mot commiten (`git show f19d059`, hele filer med `git show f19d059:<fil>`),
ikke arbeidsmappen. Alt er vurdert ved å lese koden. Ingenting er sett i spillet.

## Bygg
- Ikke bygget av kontrolløren, fordi byggeren jobber samtidig. Byggeren meldte `Result: Succeeded`.
- Commiten inneholder bare filene planen nevner: `GSTypes.h`, `GSEffects.*`, `GSCharacter.*`, `gs_materials.py` og
  `IMPROVEMENTS.md`. Ingen .uasset, ingen `Config/` og ingen filer fra andre lag er med.
- `IMPROVEMENTS.md`: linje 318–331 under «Runde 8 (Kamp)». Planen sa 313–326, men feilrettingen tok 313–317 først,
  så forskyvningen er riktig.

## Fast regel (gammelt forbedret + nytt)
- **Forbedret:**
  - Ragdollen kastes ikke lenger fast bakover og opp. Den får et lite støt i treffpunktet og i kulas retning.
    Eksplosjoner kaster hardere, og død uten treff gir ikke noe støt.
  - Kropper blir liggende i 90 s i stedet for 15 s, med tak på 12.
  - Tilskuere hører nå treffet i kropp, hode eller vest (før hørte bare offeret det).
- **Nytt:** blodsky og dråper ved såret, sprut på vegg eller bakke bak offeret, dråper på gulvet, en blodpøl som vokser
  under døde kropper, og et eget vått blodmateriale som tørker.
- Regelen er oppfylt.

## Sjekk mot «Ferdig når»
- [x] Bygget gikk gjennom (byggerens rapport). Python-koden følger samme mønster som `_fx_master`, er pakket i
      `try/except` og har en reserve i C++.
- [x] `MulticastWoundFX` kalles bare i `TakeDamage`, og den returnerer straks `!HasAuthority()`. Den kalles bare når
      `DamageCauser` finnes og det ikke er brann, høyst 3 ganger per bilde per offer (`GFrameCounter`). Den kalles før
      `Die()`, og det ligger ingen `return` imellom: Føniks-greina setter bare `bKilled = false`.
- [x] Ingen blodkode kjører på dedikert server:
      - `MulticastWoundFX_Implementation`, `SpawnBloodHit`, `SpawnBloodStain`, `SpawnBloodPool` og `SpawnDeathPool`
        returnerer alle tidlig på dedikert server.
      - Timeren i `OnRep_Dead` er beskyttet.
      - `ApplyDeathImpulse` gjør ingenting på dedikert server, fordi `LastWoundTime` aldri settes der.
- [x] Det faste støtet er fjernet fra `OnRep_Dead`. `ApplyDeathImpulse` virker i begge rekkefølger:
      - Er såret først, kommer støtet fra `OnRep_Dead`.
      - Er døden først, kommer støtet fra `MulticastWoundFX` når `bDead` er satt.
      - Støtet gis bare én gang (`bDeathImpulseDone`) og bare innen 0,8 s etter såret.
      - På listen-serveren kjører multicasten lokalt før `Die → OnRep_Dead`, så verten får alltid riktig støt.
- [x] Død av sult, tørst eller gift (`Die(nullptr, ...)` i `TickSurvival`) går ikke gjennom `TakeDamage`. Molotov-ild
      (`Proj->Kind == "fire"`) er filtrert bort. Det gir ingen sår, og `SpawnDeathPool` krever et sår de siste 8 s,
      så det blir heller ingen pøl.
- [x] Hjelm eller vest som tar treffet (`bArmorHit && Lost < 12`) gir bare støv.
- [ ] Testes i spillet av Andreas (se under).

## Vurdering av de ni avvikene fra planen
1. **Brann gir ikke blod.** Godtatt. Det er riktig og realistisk. Brannkuler (`Incendiary`) blør fortsatt, siden de er
   kuler. Det er også riktig.
2. **Færre blodpartikler under last.** Godtatt, og det er et godt grep.
   - Hvordan det virker: et vindu på 0,3 s i ekte tid. Mengden halveres over 5 treff og igjen over 12. Over 24 treff
     lages bare sky og én dråpe. Over 40 m lages ingen småprikker.
   - Gjelder hele maskinen: vinduet er `static` og felles for alle verdener i PIE. Det er greit, siden dette bare er
     kosmetisk.
3. **Sprut uten vegg bak faller på bakken 1,5–4 m bortenfor.** Godtatt, og det er mer realistisk enn ingenting. Se
   funn 1 om ett hull.
4. **`GSFitStain` krymper flekker over hjørner, maks 40/90 cm.** Godtatt.
   - Gjelder bare store flekker (≥ 10 cm) og pølen.
   - Koster høyst 16 sporinger per flekk. Løkken stopper ved første punkt som mangler gulv.
5. **Eldste flekk eroderer på 1 s ved taket på 220.** Godtatt. Det er penere enn at flekken blinker bort.
   - `RetireStain` setter `Life`, `ErodeTime`, levetid og tick-intervall riktig.
   - En flekk som ikke har landet ennå, ødelegges straks.
   - Flekker som er på vei ut, telles ikke med i taket. Det gir litt over 220 aktører et kort øyeblikk, og det er greit.
6. **Dødsstøt via `FindClosestBone`.** Godtatt.
   - Delen som ble truffet får høyst `bonemasse × 400 cm/s`, og resten går inn i kroppen.
   - Et hode på ca. 5 kg får ca. 2000. Det gir et tydelig rykk bakover uten at hodet slynges.
   - Realistisk.
7. **Materialmaske: radial + støy (`Add`, −0,55…0,12).** Godtatt. Regnet etter:
   - Midten holder seg alltid over grensen (≥ 0,45 > 0,3).
   - Kanten flytter seg mellom ca. 18 % og 85 % av radien, så omrisset blir ujevnt.
   - Når `Opacity` går mot 0, spises flekken fra kanten og innover.
8. **Trefflyd høyst én per 0,06 s per kropp.** Godtatt. Hagl gir ett smell i stedet for tre lag.
9. **Gulvdråper sjekker vegg, pølen krever gulv, flekker er tatt ut av Lumen/distance fields.** Godtatt. Det hindrer
   dråper gjennom vegger og pøler på vegger, og flekkene koster ikke noe i lysberegningen.

## Nettverk
- `MulticastWoundFX` er upålitelig og sender ca. 12 byte: kvantisert punkt og retning og to `uint8`. Den går bare til
  maskiner som har kroppen som relevant. Med høyst 3 per bilde per offer koster selv hagl lite båndbredde.
- **Når pakken mistes:**
  - På den maskinen blir det ikke noe blod for det treffet.
  - Kommer bare drapstreffet bort, blir det heller ikke noe støt, og kroppen segner rett ned.
  - Pølen kommer likevel hvis et tidligere sår kom fram innen 8 s.
  - Ingen spilltilstand avhenger av dette, så det er akseptabelt.
- **Rekkefølge:**
  - Kommer `OnRep_Dead` før drapssåret, og et tidligere sår er yngre enn 0,8 s (automatvåpen), brukes støtet fra det
    tidligere såret. Det er nesten samme retning, så det er greit.
  - En kropp som blir relevant for en maskin etter døden (spilleren kommer rundt hjørnet), får verken støt eller pøl.
    Den blir heller ikke kastet slik som før. Godtatt.
- Taket på 12 kropper håndheves på serveren (`SetLifeSpan(0.5f)`), og klientene får fjerningen gjennom replikeringen.
  Det er riktig.

## Ytelse (20 spillere, automatvåpen og hagl)
- **Per treff, i verste fall (hodeskudd, full styrke, rolig situasjon):**
  - Ca. 17 sprites, som lever 0,3–0,6 s.
  - Ca. 12 sporinger for spruten og opptil 5 × 16 sporinger i `GSFitStain`.
  - Ca. 14 sporinger for gulvdråpene og ca. 21 flekker.
  - Sporinger er billige. Det som koster, er å lage aktører og MID-er.
- **Under last:** halveringen virker fra treff nr. 6 i vinduet. I en stor skuddveksling blir det grovt anslått
  10–15 aktører per treff. Med 30 treff i sekundet er det ca. 0,3 ms per bilde ved 60 bps. Det er akseptabelt.
- **Flekker:** taket på 220 virker. Hver flekk har sin egen MID og er sitt eget tegnekall (ingen instansiering), uten
  skygge og uten kollisjon. Små flekker forsvinner etter 25 m, store etter 70 m. Akseptabelt på RTX 3050, men se
  forslag 3.
- **Tick i `GSEffects`:** flekker som har vokst ferdig, ticker to ganger i sekundet. Ved fullt tak blir det ca. 440
  tick i sekundet. Hvert tick kaller `SetWorldScale3D` og tre MID-parametre (se funn 2). Det er småpenger, men unødvendig.
- **Kropper i 90 s:** `Tick` hopper over det tunge for døde kropper (`!bDead`-vakter). Ragdollene sovner når de ligger i ro.
- **Hagl:** taket på 3 per bilde virker på serveren, og vinduet på klientene demper resten.

## Realisme og groteskhet (Andreas' mål)
- **Bra:**
  - Inngangs- og utgangssår gir ulik mengde blod.
  - Hodeskudd gir mer blod.
  - Spruten blir avlang på skrå og mindre jo lenger bort den lander.
  - Dråpene lander med riktig forsinkelse (`sqrt(2h/g)`).
  - Pølen vokser sakte over 16 s med ujevne lapper.
  - Blodet tørker mørkere og mattere.
  - Vesten gir støv i stedet for blod.
  - Ikke noe arkade-preg.
  - Dette er et stort steg fra «null blod i verden».
- **Mangler fortsatt for «veldig groteskt»:**
  - Ingen sår eller blod på selve kroppen eller klærne.
  - Ingen blodspor etter sårede.
  - Skudd i lik gjør ingenting (`TakeDamage` returnerer på `bDead`).
  - Hagl, som er det mest brutale våpenet på kort hold, gir bare små `Flesh`-sår uten utgangssprut. Hvert hagl tar
    under 18 i skade og regnes for seg.
  - Dette er ikke feil i denne runden, men det bør komme neste runde (se forslag).

## Funn (ingen blokkerer)
1. **`GSEffects.cpp`, `SpawnBloodHit`, sprut-løkka (ca. linje 545–579 i `f19d059`), lav alvorlighet.**
   - Problemet: `Trace` gir `false` både når den ikke treffer noe og når den treffer en annen spiller. Står en annen
     spiller bak offeret, går koden derfor inn i «faller på bakken»-greina. Da kan flekken havne på gulvet på andre
     siden av en vegg som står bak den andre spilleren.
   - Rettelse: skill mellom «traff en pawn» og «traff ingenting», og hopp over flekken når en pawn er truffet. Eller
     sjekk at det er fri vei fram til `Fall`.
2. **`GSEffects.cpp`, `Tick` i flekk-greina, lav alvorlighet.**
   - Problemet: `SetWorldScale3D` kalles hvert tick også etter at flekken har vokst ferdig (`T >= 1`). Det markerer
     transformen som endret to ganger i sekundet for opptil 220 aktører.
   - Rettelse: sett skalaen bare mens `T < 1` (og når reserve-skiva krymper). Oppdater ellers bare MID-en, og gjerne
     sjeldnere (tørkingen tar 60 s).
3. **`GSCharacter.cpp`, `TakeDamage`, hagl (realisme, ikke feil).**
   - Problemet: hvert hagl blir et eget lite `Flesh`-sår med svak styrke, og bare de tre første i bildet sendes. Et
     haglskudd på kort hold ser derfor svakere ut enn ett riflekuletreff.
   - Rettelse: legg sammen skaden fra samme bilde og send ett sterkt sår (`Through`/`Blast`-mønster med bred kjegle)
     i stedet for tre små.

## Hva Andreas skal teste
Velg «Number of Players: 2» og vent til kampen har startet, siden ingen tar skade i lobbyen. Sjekk dette:
- Skyt den andre foran en vegg. Du skal se en rød sky, mørkerøde flekker på veggen bak og dråper på gulvet.
- Skyt skrått mot veggen. Flekkene skal bli avlange.
- Skyt ute på åpen plass. Spruten skal lande på bakken noen meter bak.
- Skyt på en spiller med vest. Du skal se støv, ikke blod.
- Drep spilleren. Kroppen skal segne i skuddretningen, ikke bli kastet. En pøl skal vokse under kroppen i ca. 15 s,
  og kroppen skal ligge i 90 s.
- Dø av sult eller i molotov-ild. Det skal ikke bli noen pøl.
- Første gang editoren åpnes etter runden skal Output Log vise «bygger blodmateriale». Uten materialet vises flate,
  mørkerøde skiver.

## Forslag til neste runde
1. **Sår og blod på kroppen, og blodspor.**
   - Sår der kula traff: mørke inngangshull og blod som brer seg på klærne. Lag det som dekal- eller maskeparametre på
     kroppen, styrt av det samme `MulticastWoundFX`.
   - En såret spiller drypper blod mens han går (blodspor), og det henger sammen med «blødning som må bandasjeres».
   - Dette er det Andreas uttrykkelig ba om («sår der kula traff», «blodspor»), og det som mangler mest nå.
2. **Treff i lik og hagl.**
   - Lik som blir skutt, rykker til og blør. Det trenger en egen vei forbi `bDead`-returen, rent kosmetisk og med
     samme tak.
   - Hagl på kort hold blir ett samlet, kraftig sår (funn 3).
3. **Ekte dekaler og gjenbruk.**
   - Flytt flekkene fra plan-aktører til `UDecalComponent` med et deferred decal-materiale. Da legger blodet seg over
     ujevn geometri, trapper og hjørner, og pølen kan få en normal som ser ut som tykk væske.
   - Gjenbruk aktører i en pool i stedet for å lage og ødelegge dem hele tiden.

## Lærdom
- Upålitelige multicaster som setter lokal tilstand (støt, pøl) bør ha tidsvindu + én-gangs-flagg og kalles fra begge
  sider (RPC og OnRep), da tåler de både tap og vilkårlig rekkefølge. (runde 3)
- Når en sporing filtrerer bort noen treff (f.eks. pawns), må «filtrert treff» og «ingen treff» behandles ulikt, ellers
  går effekter gjennom vegger. (runde 3)
- Kosmetiske effekter som kan utløses av mange spillere samtidig trenger både et tak per kilde (per bilde) og et felles
  tak per maskin (tidsvindu); planen bør si begge. (runde 3)
