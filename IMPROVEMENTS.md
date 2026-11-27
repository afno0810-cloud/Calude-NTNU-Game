# Forbedringer i Gløshaugen Strike

Andreas' mål: minst 1000 forbedringer. Hver linje under er én konkret ting som er lagt til eller gjort bedre.
Nyeste runde øverst.

## Retting: meny og styring del 2 (26. nov.)

Gammelt som er rettet: menyen i editorens Play (527), lagvalget (529), åpen meny (530, 531), Esc-menyen i små vinduer (532).
Nytt: P som menytast (528).

527. I editorens Play (der Esc stopper hele Play) gjør F10 alt Esc gjør: åpner og lukker hovedmenyen, lukker lagvalget og dialoger, går tilbake og avbryter tastebytte, og alle tekster sier «F10» i stedet for «Esc» der
528. P åpner og lukker menyen overalt (også i Kamps meny), så en laptop uten F-taster uten Fn også har en menytast; P kan ikke tas av andre handlinger, og skriver fortsatt bokstaven i navne- og adressefeltet
529. Lagvalget som åpner seg selv i lobbyen lukkes når du begynner å gå (WASD), og går du, starter gangen med en gang; teksten nederst sier hvilke taster som lukker det
530. En åpen meny, et åpent lagvalg eller en dialog stopper nå all spillinput: hopp, huk, løp, lading, våpenbytte, E, F, G, Q, V, T og hjulet, ikke bare gange, sikting og skyting (å slippe en tast virker fortsatt, så ingenting henger)
531. Får du ny figur (gjenoppliving eller kampstart) mens Kamps meny er åpen, er gange og blikk fortsatt sperret bak menyen
532. Kamps Esc-meny krymper med vinduet når det er lavere enn ca. 680 punkter, så tittelen og knappen «Avslutt spillet» ikke havner utenfor skjermen

## Rettinger (Grafikk og kart): kanten og startplassene (26. nov.)

Gammelt som er forbedret: kanten av banen (521–523), riggområdene (524), README (526).
Nytt: en sjekk som regner ut siktlinjer og gripekrok-høyde (525).

521. Den usynlige veggen langs kanten er nå 100 m høy og har tak, så heller ikke gripekroken (selv flere skudd etter hverandre) kan få deg ut av banen
522. Gripekroken fester seg ikke lenger i bygg utenfor banen, som Elektrobygget: veggen stopper den først
523. Kuler, kulehull og gnister treffer ikke lenger den usynlige veggen i lufta; et skudd over gjerdet flyr videre ut av banen
524. Åpningen ut av riggområdet bak hver base er en sluse: et ekstra presenningsgjerde utenfor og et kort gjerde inntil veggen, så du går ut sidelengs og ingen på stiene kan se inn der du starter
525. Sjekkskriptet regner siktlinjer fra nordstien, sørpromenaden, dørstiene og passasjene til hvert startpunkt, at slusene er brede nok å gå i, og at veggen er høyere enn gripekroken når, med motprøver som skal feile
526. README nevner ikke lenger tårnet, scenen i Lysgården og robotene, som er borte, men forteller om byggegjerdene, gravefeltene og riggområdene

## Retting (Spillbarhet): meny og styring (26. nov.)

Gammelt som er rettet: menyen i editorens Play (516, 517), Esc (518), åpning av menyen (519), logg for neste test (520).

516. Klikk og taster når fram til menyen også når du spiller i editorens eget vindu (Play i valgt visning): editorens egen tastekrok stengte menyen ute
517. Menyen kan aldri holde på mus og tastatur uten at den faktisk vises, musepekeren er på og den selv tar imot Esc og klikk
518. Esc lukker alltid hele menyen, også fra en underside eller en dialog, og gir styringen tilbake (bare «trykk ny tast» bruker Esc til å avbryte)
519. I editorens Play starter du rett i spillet; Esc eller F10 åpner hovedmenyen, også mens du øver alene, uten at kameraet ditt dreies
520. Menyen skriver i loggen når den åpnes og lukkes, hvert klikk med punkt og knapp, nye taster og når styringen går tilbake til spillet

## Runde 3 (Spillbarhet): knappehjelp og hint (26. nov.)

Gammelt som er forbedret: lobby-panelet og påminnelsen om H (462), Kontroller-siden i menyen (466), sen innkobling (467).
Nytt: H-oversikten «Taster og regler» (460, 461), hint (463–465).

460. H åpner en oversikt over alle taster, slik du selv har satt dem i Innstillinger. H lukker den igjen
461. Oversikten forklarer Capture the Flag i seks korte linjer, og hva tastene gjør på en PC og en 3D-printer
462. En liten påminnelse «H: taster og regler» står nederst på skjermen de første sekundene, til du har brukt oversikten
463. Hint første gang du ser på et våpen, en ting, en låst kasse, en PC og en 3D-printer, og når magasinet er tomt
464. Hint når du er skadet eller har fått en skade i arm eller bein, og om flagget: når du er nær det, bærer det, eller deres eget er tatt
465. Hvert hint vises høyst to ganger, aldri to ganger på rad i samme situasjon, og huskes til neste gang du spiller
466. Hintene viser tastene du faktisk har valgt. H er satt av til hjelpen, og «Vis hintene på nytt» i menyen henter hintene fram igjen
467. Den som kommer inn midt i en kamp får vite laget sitt og stillingen, og de andre ser hvem som ble med på hvilket lag

## Runde 12 (Kamp): ballistikk del 2 (26. nov.)

Gammelt som er forbedret: lysspor (484–488), skuddmeldingene til serveren (493, 494), kikkertsiktet (496), spredningen som sendes med skuddet (497), startpunktet for skudd (498), venneild (499).
Nytt: overlydssmell og sus (489–491), holdemerker (495), flygende lysspor med felles tak (492).

484. Vanlige kuler er usynlige i lufta, som i virkeligheten; bare lysspor-ammo og hver femte kule fra maskingevær lyser
485. Lysspor flyr med kula i dens egen fart og bue, i stedet for en strek som er der i samme øyeblikk
486. Lyssporet er så langt som kula rekker på et øyeblikk: en rask geværkule gir en lang strek, en pistolkule en kort
487. Lyssporet tennes ti meter foran løpet, så det ikke dekker skjermen til den som skyter
488. Lysspor-ammo gir ikke lenger bedre treff, bare synlige kuler
489. Kuler som passerer nær hodet ditt gir et skarpt overlydssmell rett ved deg
490. Langsomme kuler (.45) suser forbi i stedet for å smelle
491. En haglskur som går forbi deg er ett smell, ikke ni, og du hører aldri smell fra dine egne kuler
492. Hvert lysspor er én strek som flyttes, og høyst 40 tegnes samtidig, så det ikke hakker når flere skyter maskingevær
493. Automatvåpen sender skuddene som raske meldinger, så maskingevær med dårlig nett ikke lenger kan koble deg fra serveren
494. Enkeltskudd, boltrifler, hagler og rakettkastere sendes fortsatt sikkert: et skudd du har hørt smelle forsvinner aldri på veien
495. Sterke kikkerter har holdemerker for 150, 200, 250 og 300 m, regnet ut fra patronen i våpenet
496. Prikkene under midten i den sterke kikkerten, som ikke betydde noe, er borte
497. Spredningen sendes til serveren i hundredels grader, så de mest presise geværene (L96 i kikkert) får nøyaktig sin egen treffkjegle, også på serveren
498. Bare vegger og dører kan flytte startpunktet for skuddet ditt; en lagkamerat som står tett ved deg gjør det ikke lenger
499. En kule som er i lufta når skytteren forlater spillet, skader ikke lagkameratene hans (laget huskes fra skuddet)

## Rettinger (Spillbarhet): hovedmeny (26. nov.)

Gammelt som er forbedret: grafikk og bildefrekvens i innstillingene (476, 477), menyen mens man kobler til (478), feilmeldinger når man starter som vert eller kobler til (479, 480), lagvalget (481, 482), Kontroller-siden i et lite vindu (483).

476. Grafikk-raden i innstillingene viser riktig nivå og kan settes helt opp til Film; den hoppet før tilbake til «Lav» og kom aldri over Middels
477. Maks bildefrekvens i hovedmenyen er den samme som spillet bruker: valget (30–240) huskes når kampen starter, når du blir med og når du går ut igjen, også i Play i editoren
478. Mens du kobler til fra hovedmenyen viser menyen fortsatt hovedmenyen, ikke de gråe «i kamp»-knappene, og Avslutt sier ikke lenger at du forlater en kamp
479. «Kunne ikke starte som vert» vises når spillet ikke får åpnet nettverket, i stedet for at du bare havner i menyen igjen; får du en annen port enn 7777, viser lagvalget adressen med riktig port
480. Prøver du å bli med rett etter at du forlot en kamp, vises feilen med en gang i stedet for «Kobler til …» i 20 sekunder; en adresse uten punktum får beskjeden «Sjekk adressen» i stedet for en misvisende kartfeil
481. Lagknappene sier «Vent litt» i to sekunder etter et bytte, i stedet for et klikk som ikke gjør noe
482. Klar-tasten i lagvalget er den du har valgt selv under Kontroller (Enter virker fortsatt)
483. Kontroller-siden får plass i et lite vindu: reglene for Capture the Flag tas bort når det blir trangt, og tastene fordeles på tre kolonner om nødvendig; dessuten: navnet kan bare byttes hvert andre sekund og bare ha bokstaver, tall, mellomrom og - _ ., navnefeltet står bare ett sted, konsollen i editoren kan skrives i mens menyen er åpen, og en glidebryter blir ikke hengende når du slipper musa utenfor vinduet

## Runde 2 (Grafikk og kart): jevnere bilde, stengt bane og dekning (26. nov.)

Gammelt som er forbedret: ytelsen (416–418), Hovedbygningen og Lysgården (419, 428), basene og spawn (424, 426, 427), kantstein, synsavstand og sjekkskript (431), vær inne (430).
Nytt: byggegjerder rundt hele banen og stengte veier (421–423), gravefelt og parkerte biler som dekning (425), møblert bakgård (429).

416. Jevnere bildefrekvens: sola flyttes i små trinn, og trærne tvinger ikke lenger skyggene til å tegnes på nytt hver eneste frame
417. Bakken, veiene, oppmerkingen, fortauene og kantsteinen kaster ikke lenger skygge (de kunne ikke skygge for noe), så skjermkortet får mindre å gjøre
418. Kortere frys når spillet starter og når noen kobler til: campus settes ut gruppe for gruppe i stedet for én ting om gangen, og lønnetrærne har halvparten så mange bladflater
419. Klokketårnet bak Hovedbygningen er borte (det finnes ikke i virkeligheten); bakveggen har vinduer hele veien og en bakdør fra Lysgården ut i bakgården
420. Robotene som sto som pynt i Lysgården, på stiene og på plenene er fjernet
421. Banen er stengt hele veien rundt med byggegjerder, noen med presenning og noen du ser gjennom; ingen kan gå ut av kartet, og det står alltid et gjerde der du blir stoppet
422. Høgskoleringen er sperret i begge ender med betongsperrer, sperrebukk og en parkert varebil; promenaden og stien mot Sentralbygget er stengt med sperrebukk
423. Nye modeller: byggegjerde, gjerde med presenning, varselskilt, sperrebukk og to containere; containerne står langs kanten som dekning
424. Du starter skjermet i et riggområde bak din egen base, ikke på åpen plen ved motstanderens rute
425. Nordstien og sørpromenaden er ikke lenger åpne skytebaner: inngjerdede gravefelt og parkerte biler bryter sikten og gir dekning
426. Flagget kan ikke lenger beskytes utenfra: en skjermvegg står mellom ytterdørene og flagget i begge basene
427. De to basene er like store og speilvendt om midten av banen, og stiene til endedørene går rett på døra
428. Midtruta gjennom Lysgården er åpen: sceneteppet, scenen og stolradene er borte, og kafébord og sofaer står ikke lenger i gangveien
429. Bakgården der tårnet sto er møblert med varebil, sykkelstativ, avfallscontainere, paller og bord som dekning i sørruta
430. Snø og regn legger seg ikke lenger inne: gulv, vegger og møbler holder seg tørre i all slags vær
431. Småfeil: kantsteinen ligger ikke lenger tvers over stier, lyktestolper og busskur synes på lang avstand (før 90 meter), og sjekkskriptene har motprøver så de ikke sier OK av feil grunn

## Runde 10 (Kamp): ballistikk 1a – kuler med fart og fall (26. nov.)

Gammelt som er forbedret: skytingen på serveren (393–396), haglene (391), drapsloggen og hodeskudd (398), retningskilen (399).
Nytt: kuler som ekte prosjektiler (386–387), skadefall og innskyting (388, 390), framspoling med ping (397).

386. Kuler er ekte prosjektiler med flytid og sin egen fart per kaliber (9 mm ca. 360 m/s, 5,56 mm ca. 940 m/s, .50 BMG ca. 890 m/s), i stedet for å treffe i samme øyeblikk
387. Kuler faller med tyngdekraften: på lang avstand må du holde over målet, mest med pistol og hagle
388. Luftmotstand bremser kula, og skaden faller med avstanden – men først etter innskytingsavstanden, så våpnene dreper på like mange treff som før på vanlig hold
389. Kula flyr til den treffer noe, i stedet for å forsvinne ved en fast «rekkevidde» (en Makarov er ikke lenger helt ufarlig på 61 m)
390. Siktene er innskutt som på ekte våpen: pistol, maskinpistol og hagle på 25 m, P90 og muskett på 50 m, rifler på 100 m
391. Hvert hagl er en egen liten kule som bremses fort, så haglen er dødelig på nært hold og svak på avstand
392. Støvet i veggen kommer når kula kommer fram, ikke i det du trykker
393. Serveren trekker spredningen selv, så en juksende klient kan ikke lenger skyte helt uten spredning eller velge seg et heldig skudd
394. Du treffer der du ser kula gå også som klient: spredningen du ser og den serveren bruker er den samme
395. Serveren avviser skudd i en retning spilleren ikke ser
396. Skudd, raketter og granater kan ikke lenger starte på andre siden av en vegg eller flere meter fra der skytteren står
397. Serverens kule spoles fram med halve pingen, så den flyr i takt med kula du ser hos deg selv
398. Drapsloggen og hodeskuddskaden følger våpenet kula ble skutt med, også når skytteren rakk å bytte våpen eller døde før kula traff
399. Retningskilen når du blir truffet peker dit skuddet ble avfyrt, også om skytteren har flyttet seg eller koblet fra
400. Blodtørst gir ikke lenger liv til en skytter som døde før kula hans traff
401. Kuler som forlater banen ryddes bort med en gang, så serveren holder følge når mange skyter maskingevær samtidig

Små rettinger etter kontrollen av runde 11:

446. Radiolinja ved flagghendelser sier «En lagkamerat har flagget deres» når navnet ikke er kommet fram ennå, i stedet for en linje som starter med et tomt navn
447. Lobbyboksen gjentar ikke lenger regelen og «Trykk Enter» som alt står i hjelpepanelet til venstre; den viser bare «Du er klar.» når du er det
448. «Bildefrekvens 60» i konsollen virker også når du har skrevet t.MaxFPS selv tidligere (før ble den stille oversett)
449. Taket på bildefrekvens tas av igjen når du stopper Play i Unreal, så editorvinduet ikke blir stående låst til 60

## Runde 2 (Spillbarhet): hovedmeny (26. nov.)

402. Hovedmeny når spillet starter: Start spill, Bli med, Øv alene, Innstillinger, Kontroller og Avslutt, med campus som rolig, sakte dreiende bakgrunn
403. Bli med: skriv vertens adresse med tastaturet eller lim den inn med Ctrl+V, og siste adresse huskes. Konsollen trengs ikke lenger, så det virker også i et ferdig pakket spill
404. Start spill viser adressen vennene dine skal skrive inn, og forklarer brannmur og port
405. Melding på skjermen når tilkoblingen feiler (ingen svar, avvist, ulik versjon, port i bruk) eller verten avslutter, og du kommer tilbake til hovedmenyen
406. «Kobler til …» med sekunder og Avbryt, og forsøket gis opp etter 20 sekunder i stedet for å henge
407. Bekreftelse før du forlater en kamp (ny kamp som vert, bli med i en annen, forlat kampen, avslutt), også fra Esc-menyen. Verten får vite at kampen avsluttes for alle
408. Velg ditt eget spillernavn, også med æ, ø og å. Det huskes, og serveren rydder navnet og skiller like navn
409. Innstillinger samlet på én side: grafikknivå, oppløsning, strålesporing, skjermmodus, V-sync og maks bildefrekvens, lagret mellom øktene
410. Musfølsomhet med glidebryter i fine steg, og strålesporing huskes nå til neste gang du starter spillet
411. Lydstyrke for hele spillet, lagret
412. Tastene kan endres: klikk en handling og trykk ny tast. Tasten byttes med den som hadde den fra før, alt lagres, og «Tilbakestill» gir standardtastene
413. Kontroller-oversikt i menyen med alle taster slik de faktisk er nå, og en kort forklaring av Capture the Flag
414. Lagvalg med mus i lobbyen: begge lagene med navn og klar-merke, bytt lag med ett klikk, Klar-knapp og nedtelling. Åpnes av seg selv når du kommer inn, og med L
415. Mens menyen, lagvalget eller en dialog er åpen, når ingen taster eller klikk spillet, så du ikke skyter, hopper eller slipper ting ved et uhell

## Runde 11 (Kamp): Capture the Flag i HUD og på kroppen (26. nov.)

Gammelt som er forbedret: kroppsfargen (432), poengtavla (435), sluttskjermen (436), kartet på PC-ene (442).
Nytt: armbind (433), navn over lagkamerater (434), radiolyd og radiolinje ved flagghendelser (438, 439).

432. Kroppen andre ser har lagets farge (dempet rød eller blå jakke, ikke lysende), og den er lik etter hver respawn (før: tilfeldig farge)
433. Armbind i lagfarge på begge overarmer, som følger armen og blir med kroppen når den faller
434. Lite navn over lagkamerater du faktisk ser innen 25 meter. Aldri noe over fiender, og ingen navn gjennom vegger
435. Poengtavla (Tab) er delt i rødt og blått lag med lagets poeng, og viser erobringer, returer, drap, død og hvem som bærer flagget
436. Sluttskjermen sier hvilket lag som vant (eller uavgjort) og om ditt lag vant, og poengtavla står under boksen i stedet for oppå den
437. Lobbyen viser lagfarge foran hvert navn og Capture the Flag-regelen i stedet for «Først til 25 drap»
438. Kort radiolyd når et flagg tas, mistes, hentes hjem eller erobres: fire nøkterne lyder, dypere når det er dårlig nytt for laget ditt
439. Radiolinje under hendelseslista som sier hva flagghendelsen betyr for ditt lag («Flagget vårt er tatt av ...», «Dekk bæreren»)
440. Statusstripa for Capture the Flag tegnes nå sammen med resten av skjermen, og den gamle hjelpeteksten som lå midt i bildet i 30 sekunder er borte
441. Kompasset er flyttet ned over sekkelinja, så det ikke lenger ligger under statusstripa øverst
442. Kartet på PC-ene fyller ruta med det nye, mindre kartet: bygg med navn, veier klippet i kanten, lagkamerater i grønt og fiender i rødt
443. Flaggbæreren blir fortere sliten av å løpe og kan ikke sprinte lenge
444. Tak på 60 bilder i sekundet, så laptopen ikke blir varm og klokker seg ned. Kan endres med «Bildefrekvens 30–240» i konsollen
445. «Straaling 0» (og knappen i menyen) slår av hele strålesporingen, ikke bare Lumen sin del, så den faktisk frigjør ytelse

## Runde 1 (Grafikk og kart): CTF-kjernen rundt Hovedbygningen (26. nov.)

332. Kartet er strammet inn til kjernen rundt Hovedbygningen (330 × 265 m), laget for Capture the Flag
333. To baser som står mot hverandre: Gamle fysikk i vest og Gamle kjemi i øst, med flagg- og spawnplasser lagt inn som kartdata
334. Tre ruter mellom basene med nytt stinett: langs fasaden i nord, gjennom Hovedbygningen og Lysgården, og gjennom bakgården i sør
335. Sentralbygg 1 og 2, Bergbygget og Elektrobygget står som kulisser utenfor banen; seks fjerne bygg er tatt ut, så spillet laster mindre
336. Dørene i Hovedbygningen er åpne: steinsokkelen går ikke lenger tvers over døråpningene
337. Rampene i byggene ender ikke lenger bak et rekkverk: man kan gå av rampen og ut på gulvet i neste etasje
338. Sola står i sør midt på dagen, i øst om morgenen og i vest om kvelden (var 90 grader feil)
339. Hekkene står ikke lenger foran dørene på kortsidene av byggene
340. Brolagt bakgård mellom fløyene, rundt tårnet
341. Brolagt forplass foran hovedinngangen, og stien går helt fram til trappa
342. Kantstein langs alle gangstier
343. Allé av lønnetrær opp til hovedinngangen og gatetrær langs Høgskoleringen
344. Trær, steiner, benker og parkerte biler settes bare der det spilles, aldri oppå hverandre eller på en spawnplass
345. Modellskriptene kjører rett i Blender 5.3, og to nye sjekker (dører/ramper og kartdata) finner sperrede dører før noen spiller

## Runde 1 (Spillbarhet): Capture the Flag, grunnmuren (26. nov.)

346. Ny spillmodus: Capture the Flag. To lag, rødt og blått, med hver sin base og hvert sitt flagg
347. Du settes automatisk på laget med færrest spillere, og lagene jevnes ut når kampen starter
348. I lobbyen bytter du lag med L, så lenge det ikke gjør lagene skjevere
349. Flagget tas ved å gå bort til det, og bæres synlig på ryggen så alle ser hvem som har det
350. Dør bæreren, blir flagget liggende der han falt. Motstanderne kan plukke det opp igjen, og eget lag henter det hjem ved å gå bort til det
351. Du erobrer ved å bære flagget til din egen base, men bare når ditt eget flagg står hjemme
352. Første lag til 3 erobringer vinner, ellers laget med flest når tiden er ute. Lik stilling gir uavgjort
353. Du starter og kommer tilbake ved din egen base, på en ledig plass, vendt mot motstanderen (før: tilfeldig sted på hele campus)
354. Stripe øverst på skjermen viser stillingen og om hvert flagg står hjemme, bæres (og av hvem) eller ligger ute
355. Alle får beskjed når et flagg tas, mistes, hentes hjem eller erobres, også i hendelseslista til venstre
356. Lobbyen forklarer Capture the Flag i korte linjer, viser hvilket lag du er på og når kampen starter av seg selv
357. Kampstart rydder campus: miner, ild, røyk, slupne ting og lik fjernes, og all loot og alle låste kasser fylles på nytt
358. Nedtelling som starter fordi tiden er ute kan ikke stoppes av én spiller, og fristen på 90 sekunder starter først når spiller nummer to er inne
359. En kamp du startet alene avbrytes når en venn kobler til, så dere kan starte sammen. Drap på lagkamerat gir ikke poeng

## Runde 9 (Kamp): kroppssoner (26. nov.)

360. Kroppen har sju treffsoner med mål som en ekte kropp (hode, bryst, mage, to armer, to bein) i stedet for én sylinder og en kule; hodet er like stort som et ekte hode, før var treffområdet nesten dobbelt så stort. Sonene følger med når du huker eller sklir
361. Styreflate og mus med jevn rulling blar ett trinn per hele hakk i stedet for å rase gjennom alle våpnene, og ved 3D-printeren og kameraene på PC-en blar hjul opp oppover (før gikk begge veier nedover)
362. Skudd i arm eller bein gjør mindre skade enn skudd i brystet, og mageskudd litt mindre
363. Vesten beskytter bare overkroppen, ikke armer og bein, og hjelmen bare skallen, ikke ansiktet når skuddet kommer forfra
364. Du kan ikke skade lagkamerater: kuler, granater, miner, ild, nærkamp og blendgranater fra eget lag gjør ingenting (ingen blod, treffmerke eller retningspil), men dine egne granater skader fortsatt deg selv
365. Skudd i beinet: du går saktere, hopper lavere, halter i bildet og kan ikke løpe når skaden er stor
366. Skudd i armen: siktet vandrer og skjelver, rekylen blir verre og lading tar lengre tid
367. Plaster, bandasje og førstehjelpsskrin behandler skadde armer og bein, og smertestillende demper følgene mens de virker
368. Du kjenner hvor du ble truffet: kneet svikter ved beinskudd, våpenet rykker i hendene ved armskudd, og en liten tekst over helsa sier hva som er skadd
369. Den som bærer flagget beveger seg litt saktere (85 % fart)
370. Hodeskudd som knuser hjelmen gir fortsatt øresus og hardt rykk
371. Sikting sendes til serveren: slutt på rykking når du går og sikter online, haglene treffer der du ser dem, og andre ser at du løfter våpenet til øyet
372. Serveren godtar ikke lenger raskere skuddtakt enn våpenet har (før opptil 43 % for fort)
373. Rakettkastere og granatkasteren smeller høyt for alle og vises på walkie-talkien, i stedet for å høres lyddempet ut
374. Kroppen andre ser står med føttene på bakken hos alle spillere, også etter huking
375. Mindre arkade i bildet: «LAVT LIV»-teksten, det hvite blinket ved hodeskudd og «HODESKUDD» under siktet er fjernet

## Rettinger (Kamp) (26. nov.)

376. Granater, miner, C4, raketter, 40 mm og piler flyr dit du sikter; før ble retningen dreid en gang til, så et kast mot øst gikk mot sør
377. En mine eller C4 som ligger stille på en vegg eller i en skråning blir liggende der også hos spillere som kommer nærmere eller blir med senere, i stedet for å falle til gulvet
378. Blir du drept av en granat eller mine fra en som har dødd og fått ny kropp, viser dødskortet avstanden fra der den ble kastet og «han er selv død», uten liv og rustning fra den nye kroppen
379. Dødskameraet snur seg da mot stedet granaten eller minen ble kastet fra, ikke mot drapsmannens nye kropp et annet sted på campus
380. Retningspilen når du blir truffet av en eksplosjon peker mot eksplosjonen (pil og rakett: mot der de ble skutt fra), ikke mot kasterens lik; treffmerket går til kasterens nye kropp, og Blodtørst heler bare kroppen som faktisk kastet
381. Én regel for egne miner: den som la minen utløser den aldri selv, heller ikke i en ny kropp etter at han har dødd
382. Hver spiller kan ha høyst 5 snubletrådminer ute; legger du en sjette, fjernes den eldste
383. Musehjulet virker igjen: det leses som én akse i stedet for to «knapper» som spillet nesten aldri fikk med seg
384. Med inventaret åpent blar musehjulet opp og ned i sekken (opp = forrige, ned = neste), og inventaret sier det; ellers bytter hjulet våpen begge veier
385. Granater og miner kastes, og nærkampslag treffer, der spillere som er koblet til verten sikter opp eller ned, ikke alltid rett frem i vannrett

## Runde 8 (Kamp): blod og kropper (26. nov.)

318. Skudd som treffer en spiller gir en sky av blod ved såret, synlig for alle i nærheten
319. Kuler som går gjennom kroppen spruter blod på veggen bak, eller på bakken noen meter bortenfor når det ikke er noen vegg
320. Blodspruten blir avlang når den treffer veggen på skrå, og mindre jo lenger unna veggen er
321. Bloddråper faller og lager flekker på gulvet under den som blir truffet
322. Hodeskudd gir mer blod i lufta og større sprut bak
323. Stopper vesten eller hjelmen kula, spruter det støv og fiber i stedet for blod
324. Eksplosjoner og nærkampvåpen gir egne blodmønstre
325. En blodpøl vokser sakte under døde kropper, under hodet ved hodeskudd, og renner ikke utover kanter og trapper
326. Blodet er blankt og vått med ujevn kant, tørker inn og blir mørkere, og forsvinner til slutt
327. Dør du av sult, tørst, gift eller brann blir det ikke noe blod
328. Kropper blir liggende i 90 sekunder i stedet for 15
329. Kroppen segner sammen den veien kula traff i stedet for å bli kastet bakover og opp i lufta
330. Alle i nærheten hører når en kule treffer en kropp eller en vest, ikke bare den som blir truffet
331. Tak på antall blodflekker og kropper, og mindre blod per treff når mange treffes samtidig, så spillet ikke hakker i store skuddvekslinger

## Feilrettinger etter feiljakt (26. nov.)

313. Drap med granat, mine, molotov, rakett og pil telles nå også når den som kastet eller skjøt er død før det smeller
314. Tilfeldig loot trekkes nå per type først og sjeldenhet etterpå, så det finnes nok medisin, vester, hjelmer og granater til 20 spillere
315. Ammo er en egen gruppe i lootfordelingen (omtrent hver åttende lootplass), i stedet for å drukne blant annet utstyr
316. Granater, raketter og piler flyr jevnt og i riktig bue hos alle spillere, ikke bare hos verten
317. Snubletrådminen blir liggende til noen går i den, i stedet for å forsvinne etter ett minutt

## Runde 7: å bli truffet (26. nov.)

301. Når du blir truffet blør kanten av skjermen i stedet for at hele bildet blinker rødt, sterkere jo mer liv du mistet
302. Opptil fire røde piler viser samtidig hvor skuddene kommer fra, også når to skyter på deg fra ulike kanter
303. Pilene har skygge så de synes på snø og himmel, og blir større ved harde treff
304. Eget metallisk «klonk» og blågrå kant når vesten eller hjelmen tar kula
305. Egen lyd når vesten eller hjelmen ryker
306. Hodeskudd du overlever gir hvitt blink, øresus og et kraftigere rykk i kameraet
307. Hagl som treffer samtidig gir én lyd, ikke ni oppå hverandre
308. Under 35 % liv hører du hjertet slå, raskere jo lavere livet er
309. Bildet blir gråere og mørkere i kantene når du har lite liv, med rød puls i takt med hjertet
310. Kort grønt skjær i kanten når du heles
311. Dødskortet viser navnet på drapsmannen også når han selv døde før kula eller granaten hans traff deg
312. Dødskameraet snur seg mot drapsmannen også når han står for langt unna til at spillet ditt ser ham

## Runde 6: kamp-tilbakemelding (26. nov.)

289. Dødskamera: når du dør faller bildet, trekker seg bakover og viser kroppen din i stedet for å fryse i øyehøyde
290. Dødskameraet snur seg mot den som drepte deg, så du ser hvor skuddet kom fra
291. Dødskameraet går aldri gjennom vegger, tak eller bakken, heller ikke inne i byggene
292. Uten drapsmann (sult, tørst, egen granat) går kameraet sakte i bane rundt kroppen
293. Dødskort: hvem som drepte deg, med hvilket våpen og fra hvor mange meter
294. Dødskortet viser hvor mye liv og rustning drapsmannen hadde igjen
295. Dødskortet merker hodeskudd, og viser årsaken når du døde av noe annet enn en spiller
296. Nedtelling med stolpe til du kommer tilbake
297. Treffmarkøren er nå en tydelig X med skygge som synes på snø og himmel
298. Egen gullfarget markør og lysere lyd for hodeskudd
299. Blå markør og «klink»-lyd når kula tas av vest eller hjelm
300. Rød, større drapsmarkør med et lite «smell» og «HODESKUDD» når drapet var et hodeskudd, også i kikkertsikte

## Runde 4: grafikk (25. nov. kveld)

### Materialer og teksturer
1. Egen PBR-tekstur for rød murstein (farge, relieff, ruhet, skygger i fugene)
2. Brun murstein (Elektrobygget)
3. Gul murstein (Realfagbygget)
4. Grå granitt i kvader med ujevne skift (Hovedbygningen)
5. Mørk sokkelstein
6. Lys sandstein til gesimser, vindusbrett og bånd
7. Skifertak med overlappende skifer og litt mose
8. Puss med fine ujevnheter og regnstriper
9. Forskalingsbetong med bordstruktur og porer
10. Glatt betong
11. Eikeparkett
12. Mørkt treverk
13. Marmorfliser med årer
14. Terrazzogulv
15. Kobber med irr
16. Børstet metall med riper
17. Lakkert metall
18. Vevd stoff
19. Asfalt med steinkorn og sprekker
20. Belegningsstein med mose i fugene
21. Gress med tørre flekker og jord
22. Grus
23. Plast
24. Gummi
25. Løvatlas med gjennomskinnelige blader
26. Furu-/lønnebark
27. Bjørkebark
28. Stor variasjonstekstur som skjuler at teksturene gjentar seg
29. Alle teksturene er sømløse
30. Unreal lager hovedmaterialet M_GS_Surface automatisk (farge × tone × variasjon, normal, AO, ruhet, metall)
31. Verdensprojisert materiale for bakke og veier, så teksturene ikke strekkes
32. Ekte vindusglass: gjennomsiktig, reflekterende og tosidig
33. Løvmateriale med maskering, tosidig lys gjennom bladene og vind
34. 60 ferdige materialinstanser (MI_GS_*)
35. Materialer legges automatisk på alle modeller etter navn på materialsporet
36. Skannede materialer (Poly Haven, Megascans, Fab) vinner automatisk over våre når navnet passer
37. Søkeord for Poly Haven-navn (red_brick, castle_wall, asphalt, aerial_grass ...)
38. 7 billakkfarger (sølv, sort, hvit, rød, blå, grå, grønn)

### Lys og rendering
39. Hardware ray tracing i Lumen (RTX), både lys og refleksjoner
40. Treffbasert lys i refleksjoner, så speilinger viser riktige farger
41. Refleksjoner i glass (front layer)
42. Konsollkommando `Straaling 0/1` for å slå ray tracing av og på
43. Kartet `Gloshaugen` lages automatisk første gang
44. Kartet har sol, himmel, skylys, tåke og skyer, så det ser riktig ut i editoren også

### Bygg
45. Gamle fysikk som ekte 3D-modell
46. Gamle kjemi som ekte 3D-modell
47. Kjelhuset som ekte 3D-modell
48. Kjelhuspipa: 38 m teglpipe med jernbånd
49. Sentralbygg 1 som ekte 3D-modell
50. Sentralbygg 2 som ekte 3D-modell
51. Bergbygget som ekte 3D-modell
52. Elektrobygget som ekte 3D-modell
53. IT-bygget som ekte 3D-modell
54. Stripa med glassfasade
55. Kjemiblokkene som ekte 3D-modell
56. Varmeteknisk som ekte 3D-modell
57. Realfagbygget som ekte 3D-modell
58. Idrettsbygget med glassfasade
59. Hovedbygningen vest som ekte 3D-modell
60. Hovedbygningen øst som ekte 3D-modell
61. Vindusrammer med midtpost og tverrpost i alle vinduer
62. Glassruter i alle vinduer, uten kollisjon, så du kan skyte og hoppe gjennom
63. Vindusbrett under alle vinduer
64. Overligger over vinduene på de klassiske byggene
65. Sokkel langs bunnen av alle bygg
66. Etasjebånd på de klassiske byggene og på betongbyggene
67. Gesims øverst på de klassiske byggene
68. Saltak i skifer på de gamle byggene
69. Kobberkam på mønet
70. Takarker med vinduer
71. Piper på takene
72. Flate tak med takpapp og grus
73. Brystning med beslag rundt flate tak
74. Maskinrom på taket over trappa
75. Ventilasjonsaggregater med vifter på takene
76. Takvinduer
77. Lufterør
78. Antenner og rødt flylys på de høye byggene
79. Vertikale aluminiumsribber på glassbyggene
80. Takutstikk på glassbyggene
81. Dørkarmer og glass over dørene
82. Baldakin over alle innganger
83. Lampe over hver inngang
84. Dørmatte ved hver inngang
85. Skilt over hovedinngangen
86. Puss på innsiden av alle yttervegger
87. Terrazzogulv i alle etasjer
88. Takplater med lys i taket i alle etasjer
89. Rekkverk langs rampene
90. Rekkverk rundt trappeåpningen
91. Hovedbygningen: vindusrammer i alle vinduer
92. Hovedbygningen: glass i alle vinduer
93. Hovedbygningen: glasstak over Lysgården
94. Mørkere og mer riktig stein på Hovedbygningen

### Ute
95. Gress med ekte tekstur på hele campus
96. Asfalt på veiene
97. Belegningsstein på gangstiene
98. Fortau langs alle bilveier
99. Kantstein
100. Stiplet midtlinje
101. Heltrukne kantlinjer
102. Fortau og linjer stopper i kryss og ved byggvegger
103. Lønnetre
104. Bjørk
105. Stor gran
106. Liten gran
107. Busker
108. Klippede hekker langs byggene
109. 220 trær i stedet for 160
110. Trær har usynlig stammekollisjon, så kuler går gjennom løvet, men ikke stammen
111. Sedan
112. Kombi
113. SUV med takrails
114. Liten bil
115. Varebil
116. Bilene har hjul med felger, lys, skilt, speil, dørhåndtak og støtfangere
117. Bilene står parkert ved fortauskanten, ikke oppå den
118. 60 biler i stedet for 36, uten at de står oppå hverandre eller i kryss

### Mennesker og lys
119. Andre spillere er ekte animerte mennesker (Unreal-mannekengen Manny/Quinn) i stedet for sylindre, når Third Person-pakken er i prosjektet
120. Manny og Quinn fordeles mellom spillerne
121. Ekte ragdoll når noen dør
122. Sylinder og kule er igjen som usynlige treffbokser, så treff virker som før
123. Lyspaneler i taket i alle etasjer i alle bygg som lyser opp rommene med Lumen (og om natta)
124. Lys over hver inngang
125. PC-skjermer lyser
126. Lysekronene i Lysgården lyser
127. Ekte gatelykter med stolpe, arm og lampehode over veien
128. Gatelyset kommer fra lampehodet, ikke fra stolpen

### Vær på overflatene
129. Våte gater og vegger når det regner: alt blir mørkere og blankt, og ray tracing gir speilinger i asfalten
130. Det tar omtrent ett minutt med regn før alt er vått, og noen minutter å tørke opp igjen
131. Snø legger seg på alt som vender opp (tak, bakke, benker, biler) når det snør
132. Snøen smelter sakte bort etter at det har sluttet å snø

### Ytelse
133. Møbler og ting ute bruker hierarkisk instansiering (HISM), så de som ikke synes tegnes ikke
134. Trær, busker og biler bruker også HISM med avstandsgrense
135. Ingen .glb-forhåndsvisninger av store modeller i repoet (mindre nedlasting)

## Runde 3 (25. nov. ettermiddag)
136. Hovedbygningen som ekte 3D-modell med Lysgården, trapp, scene og gallerier
137. Tårnet med spiralrampe opp til klokketårnet
138. 43 møbler, utendørsting og roboter
139. Møbler i alle rom: klasserom, kontor, lab, kantine, bibliotek, salong, serverrom, lager
140. Benker, sykkelstativ, sparkesykler, busskur, søppelkasser, containere, piknikbord, byggegjerder, NTNU-skilt
141. Robothund, vaktrobot og leveringsrobot
142. PC-er med overvåkningskamera
143. Sporing av alle spillere på kart fra PC
144. 3D-printere som lager 13 ting av filament
145. 38 overvåkningskameraer
146. Ammo per kaliber (15 kalibre)
147. Panserbrytende ammo
148. Hulspissammo
149. Brannammo
150. Sporlysammo
151. Sjokkammo
152. Holo-sikte
153. 8x kikkertsikte
154. Trommelmagasin
155. Kompensator
156. Grep
157. Modifikasjoner som erstatter hverandre i samme spor
158. Over/under-hagle
159. M72 LAW (engangs)
160. Mosin
161. Remington 700
162. M14
163. G36
164. SCAR
165. UMP
166. P90
167. Vector
168. Signalpistol
169. Langbue
170. Printet pistol
171. Tobladet vikingøks
172. Spyd
173. Printet kniv
174. Printet hjelm
175. Ladeanimasjon med magasin ut, magasin inn og sleide
176. Patron-for-patron lading på hagler
177. Magasinet faller ut og blir liggende
178. Inspisering av våpenet (T)
179. Pust, skritt og sprint-stilling
180. Rekyl med fjær
181. Nærkamp med oppsving og slag
182. Lefse, fårikål, pinnekjøtt, julebrus, risgrøt, tran, Kvikk Lunsj, solbærtoddy, rakfisk og sodd
183. Volumetriske skyer
184. Fargekorrigering etter vær og tid på døgnet
185. Konsollkommandoene `Grafikk` og `Opplosning`
186. Treteksturer på kolber og skjefter
187. Stål-, krom- og blåneringsmaterialer på løp og sleider
188. Messing og gull på patroner og pyntevåpen
189. Polymer, bakelitt og gummi på grep
190. Oliven, khaki og tan på militærutstyr
191. Ny skinntekstur for hylstre, remmer og vester
192. Viklet håndtak på katanaen
193. Horisont: 420 hus og blokker rundt campus
194. Skogkledde åser i en ring 1,3 til 4 km ute
195. Fjorden i nord
196. Fjell og jorder som skifter etter høyde
197. Kumlokk i støpejern i kjørebanene
198. Sluk med rist langs fortauskanten
199. Røde brannhydranter langs fortauene
200. Grå koblingsskap for strøm
201. 70 morenesteiner på plenene (gir dekning)
202. Tre steinstørrelser med flate bruddflater
203. 30 runde blomsterbed med steinkant
204. Blomster i rødt, gult, hvitt og blått
205. Kameraet lener seg inn i sidesteg
206. Kameraet rykker til når du blir truffet
207. Kameraet dukker ned når du lander, mer etter høye fall
208. Pusting i kameraet, tyngre når du er skadet eller sliten
209. Roligere pust når du sikter
210. Lyd! Ny lydmotor (GSAudio) med 3D-lyd, avstandsdemping og luftdemping
211. Egne skuddlyder for pistol, maskinpistol, gevær, snikskytter, hagle, maskingevær og rakettkaster
212. Lyddemper gir dempet skudd som bare høres 40 m
213. Fjerne skudd ruller som ekko fra åsene
214. Snikskytteren høres 450 m unna
215. Litt tilfeldig tonehøyde på hvert skudd
216. Klikk når magasinet er tomt
217. Fotsteg som følger skrittlengden (gå, snike, sprinte)
218. Myke fotsteg på gress, harde på asfalt og gulv
219. Fire varianter av hvert fotsteg
220. Andre spillere høres når de går
221. Landingslyd, sterkere etter høye fall
222. Magasin ut, magasin inn og sleide under lading
223. Treffmarkør-lyd, dypere ved drap
224. Lyd når du selv blir truffet
225. Kulenedslag høres der de treffer
226. Svisj ved nærkamp
227. Vind som blåser i kast, sterkere i dårlig vær
228. Regn, som blir dump tromming på taket når du er inne
229. Fuglekvitter om dagen
230. Stille nattsus om natten
231. Innendørs demper vind og fugler
232. Alle lyder kan byttes ut med ekte opptak med samme navn
233. Kameraet rykker bare til ved skade, ikke når du heles
234. Kulehull som ligger flatt i veggen og blir værende i 40 sekunder
235. Gnist og støvsky der kula treffer
236. Poly Haven-fototeksturer tas inn i våre egne materialer (stein, skifer, puss, marmor, terrazzo, parkett, betong, asfalt, belegningsstein, gress)
237. Skannede teksturer beholder vått føre, snø og riktig fliselegging på bakken
238. Skannede steinblokker (Poly Haven) erstatter våre steiner når de finnes

## Runde 5: grafikk, animasjoner, lobby og minimap

239. Ny valnøtt-tekstur på alle trekolber; mørkere stål og matt polymer på våpen
240. Riflene har delelinje, boltflate, forward assist, pinner, ladehåndtak, sikring, magasinutløser, M-LOK-spor og munningskrone
241. Pistolene har utkastport med messing, fremre riller, sleidestopp, skinne, beavertail og gummigrep
242. Buede magasiner er nå én glatt bue med ribber og bunnplate
243. Fem ulike skyteanimasjoner per våpen (rett bakover, vri høyre, vri venstre, munningsrykk, inn i skulderen), aldri samme to ganger på rad
244. Ekte rekyl med eget mønster per våpen som klatrer i salver og trekker seg delvis tilbake
245. Pumpehagle pumper og boltrifle sykler bolten etter hvert skudd, med lyd og hylse
246. Hylser spretter ut og klirrer mot bakken
247. Egne armer og hansker i førsteperson, høyre hånd på grepet og venstre på håndvernet
248. Reload med hånd: magasin ut, hånd til lomma, nytt magasin inn, ladehåndtak
249. Hagle og boltrifle lades ett skudd om gangen
250. Animasjoner for å bruke ting: granat (sikring, sving, kast), drikke, spise, piller, bandasje, sprøyte
251. Du ser dine egne bein når du ser ned
252. Minimap som roterer med deg, med veier, bygg med navn, loot og nord
253. Lobby: spillerliste, klar-knapp (Enter), nedtelling, kampklokke og vinnerskjerm med resultattavle
254. Kikkertsikte med svarte kanter, mil-dots og rødt punkt; våpenet skjules når du ser gjennom
255. Rød pil viser hvor skaden kom fra
256. Andre spillere bærer våpenet ved skulderen, sikter opp/ned og senker det når de løper
257. Nye glatte modeller for pilleglass med riflet kork, glasskrukke, kaffekopp og vannflaske med grepsringer
258. Ny kebab, pølse i lompe med ketchup og sennep, buet banan, baguett, hjertevaffel, sjokoladekjeks, eple, plaster og bandasje
259. Snusboks, sprøyter med skala, blisterpakker, pipette, bokser med kant og fliplokk, ampuller, lattergassbeholder og nikotinplaster
260. Matpakke i matpapir, Grandiosa, taco, nudelbeger, røkelaks, brunost med ostehøvel, skolebolle og kanelbolle
261. Nye granater (splint, røyk, blend) med bøyle, splint og ring; molotov med fille; retningsmine; kikkert; nattsyn; walkie-talkie; ekstra magasin; pokal; kompass
262. Taktisk reload (ingen lading) og tom reload (lader våpenet)
263. Klikk og liten rykk når du trykker på et tomt våpen
264. Våpenet løftes opp når du står nær en vegg, så det aldri går gjennom
265. Våpenet henger etter når du hopper, tilter når du går sidelengs og dupper når du huker deg
266. Står du stille lenge, ser figuren på våpenet sitt
267. Myke røyk- og flamme-effekter i stedet for kuler: munningsflamme, gnister og støv ved treff, eksplosjoner med ildkule og røyk
268. Røykgranaten lager en ekte sky, blendgranaten et hvitt glimt, brann har flammer og svart røyk, raketter har røykspor
269. Lysende lysspor (tracere)
270. Varm løp ryker etter lange salver
271. Vindbevegelse i blader uten feilmelding i UE 5.8
272. Nye modeller: nøkkelkort med snor, dirkesett, fallskjermrigg, gripekrok, eksoskjelett, buet panserplate
273. Nærkampvåpen på nytt: øks og slegge med formede hoder, dreid baseballkølle, bøyd brekkjern, ekte stekepanne, spyd
274. Luger, MP40, Uzi og Suomi bygd på nytt med dreide løp, riflede deler og grepspaneler
275. MG3, Bren og PKM med patronbelter og tofot, buet katana med egg
276. Armbrøst, langbue, signalpistol, tobladet øks, C4 med detonator, M79, 3D-printet pistol og kniver bygd på nytt
277. Andre spillere ser sikte, lyddemper, laser og grep på våpenet ditt
278. Avrundede kolber og forskjefter på alle rifler, hagler og repetergevær
279. Trommemagasin og kompass med detaljer
280. Tre ulike nærkampslag etter hverandre (hogg, bakhånd, ovenfra), kniver stikker
281. Andre spillere ser at du slår og lader om
282. Våpenet senkes og trekkes inn når du klatrer over kanter
283. Siktet svaier i kikkertsikte; hold Shift for å holde pusten i 4 sekunder (pustemåler i siktet)
284. Avrundede pistolgrep, håndvern og mottakere; ekte rakett- og pileammo
285. Ny sykkel med eiker, ramme og kurv, og skap med ventiler og hengelås
286. Avrundede biler med glasstak, takstolper og dekk med sidevegger
287. Nye møbler: skolestol på rørramme, sofa med puter, kontorstol, auditoriumseter, pult, labkrakk og kafébord
288. Robotene bygd på nytt: menneskerobot med kuleledd og visir, robothund med kneledd og kamerahode

*(Rundene før dette: 43 våpen, sjeldenhet, overlevelse, boosts, rustning, granater og utstyr, loot, vær og dag/natt; se README.)*
