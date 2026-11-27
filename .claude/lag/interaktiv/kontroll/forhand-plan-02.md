**MÅ ENDRES** (planen er god i grunnen, men har 6 feil som må rettes før bygging, og 02b er for stor. Punkt 1–14 er ting
som må inn i planen; 15–22 er mindre ting byggeren bør vite.)

Forhåndssjekk av `ko/plan-02.md` (commit `9bf067c`). Lest mot: `1adda33` (1b), `kontroll/runde-01b.md`, Kamps K1–K3 slik de
står i arbeidsmappen nå (`GSBallistics.cpp` l. 328/373/380 og testkommandoen `gs.Test.Glasskudd` l. 400–470,
`GSProjectile.cpp` l. 302, `GSCharacter.cpp` l. 1175), `AGSCharacter::TakeDamage`/`Die`, `catalog.py` (skade/radius),
`GSHelgasetr.gen.inl` (`BreachWallsM`), `helgasetr.md` (spawn, siktregler, ødeleggelse), `check_helgasetr.py` (sjekk c–g),
Grafikks `plan-05.md` (05b) og motorens `UpdateInstanceTransform`.

## Må endres

### Begge
1. **1b-rettingene F1 og F2 må inn først** (planen nevner dem ikke). 02a generaliserer `AGSDebris::Init` til `InitBits` og
   legger gipsbiter, splinter og hele dørblad oppå samme tak. Med dagens tak (de eldste fryses i lufta) blir en granat ved
   en lettvegg og to dører enda verre enn i 1b. Skriv inn i steg 4/5: **taket tas når bitene lages** (plass = 150 − i
   lufta, høyst ca. 40 nye per bilde, resten bare som rester på gulvet). Og `ApplyMelee` utvides i 02a med lettvegger:
   gjør først F2 (glass bare foran første vegg), så får lettveggen slaget når den er første treff.
2. **Ikke rør det Kamp leser.** Kamps `gs.Test.Glasskudd` bruker `AGSMapState::GetGlass()`, `FGlassPane` (`State`,
   `Def.Id/Type/CenterCm/YawDeg/LengthCm/HeightCm`), `GetPart`, `NumGlassBroken` og `IsRegistryBuilt`. `ApplyMelee`s
   returverdi betyr for Kamp «avstand til det som stoppet slaget» (figurer lenger unna forkastes). Skriv i planen at disse
   holdes uendret, og at en lettvegg/et bord som stopper slaget også gir avstanden sin. Plan-13 bygges nå i
   `GSBallistics/GSProjectile/GSCharacter`; 02 endrer ingen av dem.

### 02a
3. **C4 river aldri veggen ned med ett smell.** Steg 3 krever `Strength ≥ 250`, men `Strength` er våpnets `Damage`:
   C4 200, Carl Gustaf 150, M72 130, splintgranat 120, M79 110. Ingen når 250, så IMPROVEMENTS 1001 («eller én C4») blir
   aldri sant. Bruk `≥ 180` (bare C4), eller send typen med.
4. **Største åpning per dør (steg 7) må ikke regnes med fysikkspor i kjøretid.** Byggeren fant i 1b at statiske spor ikke
   treffer noe hodeløst. Det gjelder trolig også en dedikert server med `-nullrhi`: da får serveren 95° og klientene 85°,
   og `Open`-prosenten betyr forskjellige vinkler på maskinene. Regn det **fra dataene** (`DoorSlots` + `BoxesM`, rent
   geometrisk i C++ eller i et skript som skriver en tabell), likt på alle maskiner. Kontrollen av 1a gjorde akkurat det i
   Python.
5. **Sprengte lettvegger mot spawn og siktreglene.** `check_helgasetr.py` sjekk c (ingen spawn synlig) og e2 (ingen fri
   linje > 42 m) regner lettveggene som **tette**. 02a åpner dem midt i kampen. De 6 veggene står på X ±15,55,
   |Y| ≤ 11,4 (P3/P4), mens spawnrommene ligger på |Y| ≥ 20,3. Det er trolig ufarlig, men ingen har regnet det. Legg til i
   «Ferdig når»: kjør sjekk c, d, e og e2 med alle lettvegger borte (bestill en bryter `--breach-open` hos Grafikk, eller
   kjør vår egen kopi). Dører trengs ikke: sjekken ser allerede gjennom åpne dører.
6. **«Skjerm» er tvetydig.** Grafikks siktregler (e2, «L-skjermene») bruker `screen` om skjermvegger som stenger sikt, og
   05b har «skjerm på vegg» (monitor). Skriv at 02a bare knuser **veggmonitoren**, aldri skjermveggene, ellers bryter ett
   skudd en siktregel. Sjekk også om noen av de knusbare bordene er `COVER`-deler i e3 (dekning hver 8. m). Da må e3 tåle at
   de knuses, eller så tar de bare kulehull.
7. **`Damage` trenger ikke replikeres.** Ingen klient bruker vegg- eller dørbladets delskade til noe som synes. Med
   `Damage` i lista blir hver kule mot en vegg eller dør en endring til 20 klienter (MG3 mot en vegg: ca. 20 endringer/s ×
   15 B × 19 ≈ 6 KB/s). Hold delskaden på serveren (`TMap<int32, uint16>` eller `NotReplicated`) og repliker bare
   `State`/`LeafGone`. Det er samtidig billigere og vanskeligere å lese ut med juks.

### 02b
8. **Del 02b i to.** 12 steg, 7 filer, en ny replikert aktør med egen bevegelse, bæring med lokal plassering, kasting,
   skade, brannslukker, barrikadejobb og en avhengighet av Kamps K6. Det blir ikke ferdig og kontrollert i én runde.
   Forslag: **02b** = sovende ting, vekking, fysikk, dvale, dytt/velt, knusing, flaske, brannslukker. **02c** = bære,
   kaste, treffskade og barrikader, når K6 er levert.
9. **Ting som bæres må ikke stoppe kuler (skjold-juks).** Steg 5: «kuler treffer den». Kamps kulespor er `Visibility` og
   ignorerer bare skytteren, så en stol eller puff 75 cm foran ansiktet stopper alle kuler mot hode og bryst: et skjold.
   Før K6 stopper den også bærerens egne skudd. Samme for løse ting generelt: «blokkerer kuler» gjør en puff skuddsikker,
   og det strider mot realismen og mot gjennomslag senere. **Endre:** løse ting ignorerer `Visibility`, og kuler finner dem
   som glasset, i `BulletSegment` (objektspor mot prop-settene og prop-aktørene på samme strekning). Der får tingen skade
   og dytt, og kula en faktor (stol/puff ≈ 0,95, brannslukker stopper: faktor 0). Kamp trenger ingen endring, for kallet
   finnes.
10. **Dvalende ting som dyttes, må våkne på nettet.** En figur som går inn i en stol i dvale, eller et dørblad som
    svinger mot den, dytter den på serveren (Chaos vekker kroppen), men aktøren er `DormantAll`, så klientene ser den aldri
    flytte seg. Bind `OnComponentWake` (eller sjekk fart når kroppen våkner) → `FlushNetDormancy` + `DORM_Awake`. Den ene
    sjekken hvert 0,25 s må gjelde våkne kropper, ikke bare aktører som er merket våkne.
11. **50 m nettavstand gir spøkelsesting.** Helgasetr er ca. 52 × 64 m med flere plan (diagonalt ca. 85 m). En klient
    lenger unna enn 50 m får aldri aktøren, og etter steg 3 står den sovende instansen da fortsatt **med kollisjon** der
    tingen sto. Klienten går inn i en stol serveren ikke har (retting), og viste kuler stopper i den. **Endre:** sett
    `NetCullDistanceSquared` så hele bygget dekkes (f.eks. (150 m)²; dvale gjør det billig). Ved `State ≥ 1` slås
    instansens **kollisjon** av med en gang, og bare synligheten venter på aktøren (høyst ca. 1 s).
12. **Taket på 160 er ikke et tak.** «Over taket vekker kuler ikke flere; bruk og smell gjør det fortsatt» gir ingen øvre
    grense (alle stoler i kartet kan bli aktører). Og et smell i kantina vekker 40–60 ting i samme bilde, men bare 24 får
    simulere: resten blir aktører som ikke rører seg (halve stolene flyr, halve står) og koster 60 nye nettkanaler × 20
    klienter i ett bilde. **Endre:** et smell vekker høyst de 24 nærmeste (resten får skade/knuses uten å bli aktører), og
    taket på 160 er hardt. Over taket tas den eldste rolige aktøren tilbake til instans: legg sted og yaw kvantisert i lista
    (ca. 10 B), aktøren slettes, og instansen flyttes dit.
13. **Drapskreditt virker, men navn og treffmarkør gjør det ikke.** `ApplyDamage(…, kasterens kontroller, prop)` gir riktig
    drap: `Die(EventInstigator)` og `IsSparedFrom` (eget lag) gjelder. Men `TakeDamage` setter `WeaponName = "Verden"` når
    `DamageCauser` verken er `AGSCharacter` eller `AGSProjectile`, gir ingen treffmarkør (`Shooter` er null) og ingen
    retningsvisning (`bHasFrom` usann). Bestill hos Kamp en liten krok (f.eks. et grensesnitt `IGSDamageSource` med
    `GetDamageName()` og `GetThrower()`), eller la skaden vente til 02c. Planens «sjekk og dropp skaden» er ikke nok:
    drapet går, men drapsfeeden sier «Verden».
14. **Kastefarten treffer ikke skadegrensen.** Grensen er «fart > 6 m/s», men kastefartene er stol 6, kontorstol 4,
    brannslukker 5, søppelbøtte 6 og puff 7 (pluss figurens fart). En kastet brannslukker gjør aldri skade, og en stol bare
    når man løper (IMPROVEMENTS 1022). Bruk f.eks. 4 m/s relativt til den som treffes, eller regn skaden fra impulsen.

## Mindre (byggeren bør vite)
15. **Sperre for spawnutgangene:** 5 kg-stoler som blokkerer `Pawn` kan stables i en spawnsluse uten dør (L-sving), og
    figurer kan ikke dytte dem. Da sitter et lag fast. La løse ting under ca. 15 kg dyttes av figurer på serveren
    (dytt ved overlapp), eller la dem ignorere `Pawn` innenfor spawnrommene.
16. **Rubberbanding** når figurer går inn i ting serveren simulerer og klienten bare interpolerer: prøv i PIE med
    `NetEmulation PktLag=150` gjennom en klynge stoler. Interpoler hvert bilde mens tingen beveger seg, ikke 12 ganger i
    sekundet (ser hakkete ut). Høyst 24 er i bevegelse, så det er billig.
17. **Barrikade og forutsigelse:** `AGSDoor::PredictUse` åpner døra lokalt. Står det ting i svingen, spretter den tilbake
    når serveren stopper den 5° før. Ikke forutsi når en våken prop-aktør står innen svingradien.
18. **E-tasten mot Kamps gjenstander:** i 1a viker vi når Kamps `FocusPickup` er satt. Det må gjelde løse ting også, ellers
    kan man ikke ta opp et våpen som ligger ved en stol eller flaske. Skriv regelen i steg 5.
19. **Kulehull i lettvegger:** `PerKeyCap` er 12 per nøkkel (laget for dører). En vegg som tar et magasin, viser bare de 12
    siste. Gi lettvegger egen grense (ca. 40), innenfor taket på 400.
20. **Byggerens lettvegg-ISM er `Static`.** `UpdateInstanceTransform` har ingen sjekk på mobilitet i 5.8, og skala 0
    fjerner kroppen. Men det er første gang vi skalerer en `Static` ISM i kjøretid: bekreft i PIE at veggen faktisk
    forsvinner fra bildet (hodeløst beviser det ingenting).
21. **Kulesvar på bladet:** steg 5 sier at glassdørens ramme og branndøra ikke tar kuleskade. Skriv hvilke komponenter
    som teller som blad (`Blockers` minus ruta, håndtak og kodelås), så ikke et skudd i karmen eller veggen ved siden av
    teller. Tallene stemmer: AG3 38 → 9,5 enheter → 13 skudd for en vegg (magasin 20), MP5 23 skudd, 5,56 16 skudd,
    hagle 5 skudd. Akkumuler som flyttall eller rund av til minst 1, ellers gir svake treff 0.
22. **Svar til Grafikk (05b):** formatet (`FPropM`, `GS_HELGASETR_HAS_PROPS`, `FindProp`) passer til 05b-bestillingen.
    Legg til at `Sx/Sy/Sz` og `bLos` også står i manifestet (append-only som id-ene), og at 05b sin plasseringsregel
    («ikke i dørsving») også gjelder løse ting med `bLos`.

## Størrelse
- **02a** er stor nok med 1b-rettingene: 9 steg, 3 kodefiler (+ én Blender), 13 modeller. Steg 6 (faste møbler) kan bare
  prøves i testrommet til Grafikks `Props()` finnes. Flytt det gjerne til 02b, så 02a blir vegger + dører + åpningstabell
  + F1/F2.
- **02b** må deles (punkt 8).
