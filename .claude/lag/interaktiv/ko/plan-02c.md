# Plan 02c (Interaktiv verden): bære, kaste og barrikadere

Andreas: «plukke opp ting», «kaste», «så mye som mulig interaktivt». Skrevet om etter forhåndssjekken
(`kontroll/forhand-plan-02.md`, punkt 8, 9, 13, 14, 17, 18). **Bygges etter 02b og etter Kamps K6** (hender opptatt) og
Kamps nye krok K7 (navn og treffmarkør for kastede ting). Kamps grensesnitt holdes ellers uendret.

## Mål
Ta opp en stol eller flaske, bær den, sett den ned eller kast den – og sett ting foran en dør så den ikke går opp før noen
dytter eller sparker seg gjennom.

## Forbedrer (gammelt)
- **Bruk-tasten** (1a/02b): E/hold E får mening når man bærer noe.
- **Dører** (1a): stopper mot ting i svingen; spark og ny dytte-jobb skyver dem unna; ingen lokal forutsigelse som spretter
  tilbake.
- **`AGSPhysicsProp`** (02b): kan bæres og kastes, og en kastet ting kan skade.

## Nytt
- Bære, sette ned, kaste (hold E).
- Treffskade fra kastede ting med riktig navn i drapsfeeden og treffmarkør (Kamps K7).
- Barrikader og `GSJob::Push`.

## Filer
`GSPhysicsProp.h/.cpp`, `GSInteract.h/.cpp`, `GSDoor.cpp`, `GSMapState.cpp`, `IMPROVEMENTS.md`.

## Steg
1. **Bære** (`IGSInteract` på `AGSPhysicsProp`; sovende ting vekkes først som i 02b):
   - Ikke båret: **E** = ta opp (ingen andre bærer den, du bærer ikke noe; fokus viker for Kamps `FocusPickup`).
   - Båret: **E** = sett ned foran deg (sveip ned til gulvet, fysikk på, fart = figurens). **Hold E** = kast (siktretning ×
     kastefart + figurens fart; stol 6, kontorstol 4, puff 7, flaske 14, brannslukker 5, bøtte 6 m/s). Bærer man noe, er
     fokus alltid det man bærer.
   - `UPROPERTY(ReplicatedUsing=OnRep_Carrier) TObjectPtr<AGSCharacter> Carrier`. Båret: ingen fysikk, ingen
     `ReplicateMovement`, ikke dvale, **ingen kollisjon mot `Pawn`, `PhysicsBody` eller `Visibility`**. Kuler finner den som
     før i `BulletSegment` med typens faktor (punkt 9) – **unntatt bærerens egne kuler** (ignorer båret ting når skytteren er
     bæreren; strekningen starter i hans øye). Ingen skjold-juks: stol/puff stopper ikke kuler.
   - Hver maskin legger den i `Tick`: øye + sikteretning × holdeavstand (stol 75, flaske 45 cm) − 25 cm ned; eieren
     `ControlRotation`, andre `GetBaseAimRotation()`; kulespor 15 cm mot `WorldStatic` trekker den inn. Serveren regner sitt
     eget sted ved slipp.
   - Slipp av seg selv: bæreren død/borte, `UseItemId` satt, terminal, fast > 2,5 m fra holdepunktet i 0,5 s. Tidssperre
     0,4 s per spiller.
   - For Kamp (K6, levert før 02c): `GSInteract::GetCarryMass(const AGSCharacter*)` og `GetCarried(...)`.
2. **Treffskade** (punkt 13/14): server `OnComponentHit` på kastet ting mot `AGSCharacter`, ikke kasteren de første 0,3 s.
   Fart **relativt til den som treffes** > 4 m/s → skade = min(20, kg × fart_m/s × 0,5) (stol 6 m/s → 15, brannslukker
   5 m/s → 20, flaske 14 m/s → 3,5). `ApplyDamage(..., kasterens kontroller, prop)`. `AGSPhysicsProp` implementerer Kamps
   `IGSDamageSource` (K7): `GetDamageName()` («Stol», «Flaske», «Brannslukker» …) og `GetThrower()`, så drapsfeeden viser
   tingen og kasteren får treffmarkør og retning. Eget lag spares som før (`IsSparedFrom`).
3. **Barrikader** (`GSDoor.cpp`, server, før åpning/lukking som ikke er smell eller spark-gjennom): våkne `AGSPhysicsProp`
   (avstand ≤ 1,5 × bredde fra hengselet, ikke båret) med boks i svingen mellom nåværende og ønsket vinkel; summert ≥ 8 kg →
   døra stopper 5° før første ting, brukeren får «Noe står i veien for døra». **Hold E** på blokkert dør = `GSJob::Push`
   (2 s, «Dytter døra opp»): tingene får 150 cm/s ut av svingen, så åpnes døra. Spark skyver dem 300 cm/s og åpner. Smell som
   før. Sovende instanser sjekkes ikke (Grafikk holder dem ute av dørsving).
4. **Ingen forutsigelse mot barrikade** (punkt 17): `AGSDoor::PredictUse` åpner ikke lokalt når en våken prop-aktør står
   innen svingradien.
5. **Testrom:** plass foran tredøra til å bygge barrikade.
6. **Selvtest + PIE:** ta opp → `Carrier`, ingen fysikk; sett ned → på gulvet; kast flaske mot vegg → knust + støy; kastet
   stol mot bot → 15 skade, drapsfeed «Stol»; bærerens egne skudd går gjennom stolen, en annens kule mot bæreren bremses
   bare 5 %; stol foran døra stopper den under 95 % og `Push` åpner; E-tasten prøves i PIE vert + klient (lærdom 1a).

## Ferdig når
- `Build.bat` uten feil; hodeløs selvtest OK.
- PIE vert + klient: klienten tar opp en stol, bærer den (våpen senket, saktere – K6), setter den foran døra; verten får den
  ikke opp før han holder E; en kastet brannslukker skader og drapsfeeden viser den.

## Risiko
- K6/K7 ikke levert: 02c venter (ikke bygg bæring uten K6 – da kan man skyte med stolen i hendene).
- En barrikade må aldri stenge en base for godt: dytt, spark og smell åpner alltid.

## IMPROVEMENTS-linjer
1029. Trykk E for å ta opp en ting og bære den, trykk igjen for å sette den ned
1030. Hold E for å kaste det du bærer – lette ting langt, tunge kort
1031. Mens du bærer noe, er våpenet senket og du går saktere
1032. En kastet stol eller brannslukker gjør skade, og drapsfeeden viser hva du ble truffet av
1033. Ting du bærer er ikke skjold – kuler går gjennom stoler og puffer
1034. Sett stoler eller annet foran en dør for å barrikadere den
1035. Hold E på en barrikadert dør for å dytte den opp; et spark skyver tingene unna
1036. Testrommet har en dør å barrikadere
