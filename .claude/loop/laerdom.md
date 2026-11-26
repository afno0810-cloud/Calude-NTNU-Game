# Lærdom fra forbedringsløkken

Generelle regler løkken har lært. Alle tre agentene leser denne før de starter.
Oppdateres etter hver runde av `/forbedringsloop` (fra kontrollørens «Lærdom»).
Hold listen kort: slå sammen like regler, og fjern regler som har vist seg feil.
Hver regel: én linje, med (runde N) bak, og et telletall når den bekreftes igjen.

## Regler
- Bygg aldri mens Unreal-editoren er åpen; be Andreas bruke Ctrl+Alt+F11 eller lukke den. (start)
- `GSItemCatalog.gen.inl` er generert: endre `catalog.py` og kjør `gen_cpp.py`. (start)
- Kamera/view target online må tåle at RPC-er og replikerte verdier kommer i vilkårlig rekkefølge: sett tilstanden på både server og klient, og ha en billig vakt som bare retter når noe faktisk er feil. (runde 1)
- `Build.bat` som sier «Target is up to date» etter byggerens bygg er godt nok bevis på at koden kompilerer. (runde 1)
- Planer for nettverksting må nevne motorens egne kamera-/possession-kall (SetPawn, ClientRestart); byggeren måtte finne dem selv. (runde 1)
- En rettelse på mottakersiden må sjekke at alle som kaller den faktisk sender dataene, ellers logges en forbedring som aldri kan skje. (runde 2)
- Planleggeren leser `funn.md` før en rettelse planlegges, så samme hull ikke blir stående. (runde 2)
- Lokale tilstander som nullstilles i `UpdateLocalVisuals` må også nullstilles i `OnRep_Dead`. (runde 2)
- Nye mål fra Andreas midt i en runde: dirigenten sjekker planer i køen mot `andreas.md` før de bygges, og sender dem tilbake hvis de strider mot målene. (runde 3)
- Verdier satt på en komponent før `FinishSpawning` kan bli tolket om av `InitializeComponent`: les motorens standardverdier (f.eks. `bInitialVelocityInLocalSpace`). (feilretting 1)
- Hos klienter kjøres `OnRep` for startverdier før `BeginPlay`, så `BeginPlay` må ikke overskrive replikert tilstand. (feilretting 1)
- Trekninger i flere trinn (loot) må regnes ut per gjenstand og for alle `MinRarity`-nivåer før de godkjennes. (feilretting 1)
- Upålitelige multicaster som setter lokal tilstand trenger et tidsvindu, et én-gangs-flagg og kall fra begge sider. (runde 3)
- En sporing (trace) som filtrerer bort treff må skille «filtrert treff» fra «ingen treff». (runde 3)
- Kosmetiske effekter trenger både et tak per kilde og et felles tak per maskin. (runde 3)
- Musehjul/styreflate sender brøkdeler: samle utslag til hele trinn (±1) før ett steg. Andreas tester på laptop. (feilretting 2)
- Server-kode skal aldri bruke kamerakomponenten for klienters sikteretning; bruk `GetBaseAimRotation()` eller send retningen i RPC-en. (feilretting 2)
- Kontrolløren sjekker byggerens forklaringer om motoren mot motorkoden, ikke bare selve rettelsen. (feilretting 2)
- Store planer forhåndssjekkes av kontrolløren før bygging; det er billigere å rette en plan enn kode. (plan 5)
- Alt klienten selv velger (indekser, nøkler, tidspunkter) kan velges for å jukse: serveren må begrense eller trekke selv. (plan 5)
- Endringer i skade må regnes mot katalogens drap-terskler (antall treff for å drepe) før de planlegges. (plan 5)
- Kanalspor på `ECC_WorldStatic` treffer også spillerkapsler; bruk objektspor når bare verden skal blokkere. (Spillbarhet runde 1)
- Planer som tegner på skjermen må liste hva andre lag tegner samme sted. (Spillbarhet runde 1)
- Nye server-RPC-er som lager eller flytter figurer trenger tidssperre per spiller. (Spillbarhet runde 1)
- Tidssjekker på serveren må regnes mot serverens bildetid og vanlig jitter; en konto må alltid romme minst to hendelser i samme serverbilde. (runde 4)
- Følger som ganges inn i fart må ganges inn i alle fartsverdiene (gå, huk, sikte). (runde 4)
- Effekter som utløses per treff trenger en «én gang per øyeblikk»-vakt (hagl). (runde 4)
- Unity-bygg: globale navn i anonyme namespace i én .cpp kan kollidere med navn i en annen. Bruk navngitte namespace (f.eks. `GSMenuImpl`) eller unike prefiks. (Spillbarhet runde 2)
- Avsløring av fiender skal være realistisk (radiomelding om sted), ikke veggsyn med markør og avstand. (plan Spillbarhet 3)
- Tall som pakkes for nettet må ha finere trinn enn den minste verdien de skal bære. (runde 5a)
- Det en kule trenger når den treffer (lag, våpen, startpunkt) lagres når den skytes. (runde 5a)

