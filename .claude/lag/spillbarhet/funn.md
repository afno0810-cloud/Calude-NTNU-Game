# Funn fra kontrollørens feiljakt (Spillbarhet)

Nyeste øverst. Planleggeren leser denne og tar de viktigste inn i planene.
Den felles listen `.claude/loop/funn.md` har funnene om loot/pickups, prosjektiler, spawn/kampregler og
våpen/skyting. De gjentas ikke her.

## Områder som er gjennomgått
- 2026-11-26 (commit 55ca90f), «Feiljakt 1»: **PC-er, kameraer og 3D-printere** (`GSTerminal`, terminaldelen
  av `GSPlayerController`, `SpawnTerminals` i `GSGameMode`, terminalskjermene i `GSHUD`), **overlevelse**
  (sult, tørst, mat, medisin, boosts og bivirkninger: `TickSurvival`, `CompleteUse`, `AddEffect`,
  `TickEffects` i `GSCharacter`, data i `catalog.py`), **inventar/sekk/vekt** (`AddToBackpack`,
  `TakeFromPickup`, `GiveWeapon`, `DropEverything`), **lobby, klar-system og poengtavle** (`GSGameMode`,
  `GSGameState`, `GSPlayerState`, `DrawMatch`/`DrawScoreboard` i `GSHUD`), **meny og konsollkommandoer**
  (Host, Join, Grafikk, Opplosning, Straaling).
- Ikke gjennomgått ennå: gadgets i detalj (gripekrok, fallskjerm, drone, kikkert, nattbriller), nærkamp,
  bevegelse (mantle/skli), kontroller/tastebindinger, lootplassering per bygg, og hva som skjer når
  spillere kobler til/fra midt i en kamp.

## Funn

Alvorlighet: **Høy** = ødelegger kamper, gjør en hel funksjon ubrukelig eller kan utnyttes lett.
**Middels** = merkes tydelig. **Lav** = pirk som tas når man likevel er i filen.
Linjenumre gjelder commit 55ca90f. Alt er funnet ved å lese koden, ikke ved å spille.
«Eier» sier hvilket lag som eier filen (se `.claude/lag/LAG.md`). Funn i andre lags filer må gå som
bestilling i `.claude/lag/bestillinger.md`.

### Feiljakt 1 (2026-11-26, commit 55ca90f)

#### A. Ekte feil (flyt som stopper opp, juks, feil poeng)

S1. **Kampen starter 10 sekunder etter at første venn kobler til** – Høy (flyt)
   - Fil: `GSGameMode.h` linje 34 (`LobbyStartedAt = 0.f`), `GSGameMode.cpp` linje 62 og 137. Eier: Spillbarhet.
   - Hva: 90-sekundersklokken for automatisk start regnes fra serveren ble startet, ikke fra noen kom inn.
     Den settes bare på nytt i `BackToLobby`. Har verten ventet mer enn 90 s på vennene sine (det har han
     nesten alltid), starter nedtellingen i samme øyeblikk som spiller nr. 2 kobler til. Resten av
     vennene kommer inn midt i kampen. HUD-en viser heller ikke at det finnes en slik klokke.
   - Forslag: start klokken når antall spillere går fra 1 til 2 (og start den på nytt hver gang en ny
     spiller kommer inn), repliker tidspunktet og vis «Starter automatisk om N sek» i lobbyen.

S2. **Én spiller alene kan starte en 15 minutters kamp som ikke kan avbrytes** – Middels (flyt)
   - Fil: `GSGameMode.cpp` linje 61 (`Total > 0 && Ready == Total`), linje 68–77. Eier: Spillbarhet.
   - Hva: trykker verten Enter mens han er alene, starter kampen. Drapsgrensen kan ikke nås alene, og
     verten har ingen måte å avslutte eller starte på nytt. Venner som kommer inn må spille ut resten av
     de 15 minuttene, eller verten må starte serveren på nytt (alle kastes ut).
   - Forslag: krev minst 2 spillere for vanlig start (alene = tydelig «øvingsrunde»), og gi verten en
     «Start på nytt / tilbake til lobby»-knapp i menyen.

S3. **Brannammo og andre «tappe»-effekter gir ikke drap til skytteren, og dødsårsaken blir «Overdose»** – Middels
   - Fil: `GSCharacter.cpp` linje 2541 (`AddEffect("Brennende")`), `TickSurvival` linje 2922–2937. Eier: Kamp.
   - Hva: «Brennende» (6 HP/s i 3 s) legges på som en vanlig effekt uten å huske hvem som skjøt. Dør
     offeret av brannen, kalles `Die(nullptr, false, "Overdose")`. Skytteren får ikke drapet, killfeed
     sier «Verden», og offerets dødskort sier «Overdose». Samme tekst brukes hvis man dør av
     matforgiftning fra kald pizza.
   - Forslag: lagre hvem som startet effekten og årsaken (`LastDamageInstigator`/`LastDamageWeapon`
     finnes alt) og bruk dem i `TickSurvival`. Egne tekster: «Brann», «Matforgiftning», «Overdose».

S4. **Man kan dø i lobbyen selv om det står «ingen skade»** – Lav/Middels
   - Fil: `GSCharacter.cpp` `Tick` linje 3096–3098 og `TickSurvival` linje 2915–2938. Eier: Kamp.
   - Hva: sult, tørst og effekter som tapper helse (Berserk, matforgiftning) går rett på `Health` og
     sjekker ikke `DamageEnabled()`. Sult og tørst tikker også i lobby, nedtelling og resultatskjerm.
     En spiller som står i lobbyen lenge, eller tar Berserk der, dør og får «Død» på tavla.
   - Forslag: hopp over `TickSurvival` (eller i det minste helsetapet) når kampen ikke er i gang.

S5. **Sporingen fra PC-en fortsetter etter at du har gått, og serveren vet aldri at noen «sitter ved PC-en»** – Middels (juks)
   - Fil: `GSPlayerController.cpp` linje 455–485 (`ServerTrack`, `SendTrackPulse`), 347–354 (`OpenTerminal`,
     bare lokalt), 443–447 (`ServerCanUse`). Eier: Kamp (filen) → bestilling.
   - Hva: (1) `SendTrackPulse` sjekker ikke at spilleren fortsatt står ved PC-en eller lever. Etter ett
     tastetrykk sender serveren alle spilleres navn og posisjon hvert 2. sekund i 20 s uansett hvor du
     løper. Vanlig HUD skjuler kartet når du går, men dataene ligger hos klienten (en modifisert klient
     har radar). (2) Å «sitte ved PC-en» finnes bare på klienten: det er klienten selv som slutter å ta
     imot bevegelse og skyting (`TerminalPC()`-sjekkene). Serveren krever bare at du er innen 3,5 m. En
     juksende klient kan spore, se kameraer og printe mens den løper og skyter. (3) `ServerViewCamera`
     (linje 449–453) flytter serverens synspunkt til kameraet og flytter det aldri tilbake av seg selv.
   - Forslag: repliker «bruker terminal» som tilstand på serveren (`ServerOpenTerminal`/`ServerCloseTerminal`),
     la serveren stoppe bevegelse/skyting mens den er satt, stopp sporing og kameravisning på serveren
     når spilleren går, dør eller tar skade, og la andre spillere se at du står bøyd over PC-en.

S6. **Ammokassen (vanlig) gir raketter, 40 mm-granater og .50 BMG til alle våpen på én gang** – Middels (balanse)
   - Fil: `GSCharacter.cpp` `CompleteUse` linje 2280–2288 (Eier: Kamp); `catalog.py` linje 304 (`AmmoBox`,
     Common. Eier: Spillbarhet).
   - Hva: én «Ammokasse» gir `max(2 magasiner, 3)` skudd til hvert våpen du bærer uansett kaliber: 3
     raketter til Carl Gustaf (mytisk våpen), 3 granater til M79, 20 skudd .50 BMG. Den riktige esken
     «84 mm raketter» er uvanlig og gir 1 rakett. Kalibersystemet blir meningsløst.
   - Forslag: fjern `AmmoBox` fra katalogen (kan gjøres av Spillbarhet alene), eller la den bare fylle
     pistol- og geværkalibre.

S7. **PC og printer kan brukes gjennom vegg og gulv** – Lav
   - Fil: `GSCharacter.cpp` `UpdateFocusPickup` linje 2041–2054 (2,4 m fra øyet, ingen siktlinje),
     `GSPlayerController.cpp` `ServerCanUse` linje 443–447 (3,5 m, ingen siktlinje). Eier: Kamp → bestilling.
   - Hva: samme hull som funn 8 i felleslisten (loot gjennom vegger). I bygg med 4 m etasjehøyde ligger
     printeren i 2. etasje ca. 2,3–2,4 m fra øyet til en som står rett under og ser opp.
   - Forslag: ett linjespor fra øyet til terminalen på både klient og server.

S8. **Printet gripekrok har 1 lading, den fra loot har 3** – Lav
   - Fil: `GSTerminal.cpp` linje 173 (`SpawnDropped(..., PrintingId, 1)`), `GSItems.cpp` `PickupCount` linje 272.
     Eier: Spillbarhet.
   - Forslag: bruk `Def->Charges` for gripekrok også når printeren lager den.

S9. **Poengtavla sorterer bare på drap, vinneren kåres på drap og færrest dødsfall** – Lav
   - Fil: `GSHUD.cpp` linje 912, `GSGameMode.cpp` linje 126. Eier: Kamp (HUD) → bestilling.
   - Hva: ved likt antall drap kan vinneren stå som nr. 2 eller 3 på tavla. Overskriftene blander også
     språk («Kills» og «Død»).

S10. **Sluttskjermen: poengtavla tegnes oppå «KAMPEN ER OVER» og vinnernavnet** – Middels
   - Fil: `GSHUD.cpp` `DrawMatch` linje 797–805 og `DrawScoreboard` linje 914–934. Eier: Kamp → bestilling.
   - Hva: banneret ligger på 30 % av skjermhøyden (130 px høyt). Poengtavla starter på 20 %, er
     70 + 26 px per spiller høy, og tegnes etterpå. På 1080p (banner fra 324 px, rader fra 280 px)
     skrives radene fra spiller nr. 3 og utover rett over «KAMPEN ER OVER», vinneren og nedtellingen.
   - Forslag: flytt banneret over tavla (for eksempel 8 % av høyden) eller skriv vinneren inni tavla.

#### B. Systemer som i praksis ikke virker (balanse)

B1. **Crafting er nesten umulig: det finnes ca. 2 filament-spoler på hele campus** – Høy
   - Fil: `GSItems.cpp` `RandomLoot` linje 215–219 (Utstyr = 8 av 100), `catalog.py` linje 423 (Filament, Common),
     `GSTerminal.cpp` linje 70–84 (oppskrifter 1–8 spoler). Eier: Spillbarhet.
   - Hva: regnet ut fra vektene gir en lootplass filament 0,53 % av gangene (utstyr 8 % × vanlig 40 % ×
     én av 6 vanlige utstyrsting). Med ca. 428 lootplasser er det i snitt 2,3 spoler på hele campus til
     20 spillere (én spole per funn). Billigste nyttige ting (printet pistol, panserplate, gripekrok)
     koster 3. Speiderdronen koster 8, altså mer enn det som finnes på kartet. 3D-printerne (ca. 10
     stykker) er i praksis pynt.
   - Forslag: faste funnsteder for filament ved hver printer og i verksteder (2–4 spoler), og/eller egen
     lootgruppe «materialer». Se også R3.

B2. **Sult og tørst betyr ingenting i en kamp, men mat er 20 % av all loot** – Høy (dødt system)
   - Fil: `GSCharacter.cpp` `TickSurvival` linje 2919–2920 (25 min / 18 min fra full til tom), `Tick` linje 3112
     (halv stamina-henting først under 20), `GSGameState.h` (`MatchMinutes = 15`), `GSItems.cpp` linje 219
     (Mat = 20). Eier: Kamp (kode), Spillbarhet (regler og data).
   - Hva: alle starter på 100 og får 100 igjen ved hver respawn. Kampen varer 15 minutter. Ingen
     virkning kommer før under 20, det vil si etter ca. 14 min (tørst) uten å dø og uten å drikke. Det
     skjer nesten aldri. Mat brukes derfor bare som små helsepakker. Hver femte lootplass er likevel
     mat, mens det bare er ca. 2,4 vannflasker på kartet.
   - Forslag: velg én vei og gjør den ferdig. Enten: start på 60–70, behold nivået gjennom respawn, og
     gi gradvise, realistiske følger fra ca. 50 (tregere stamina, mer våpensvai). Eller: kutt mat i
     loot til 5–8 % til kampene blir lengre.

B3. **Boosts kan stables uten tak: over dobbel fart og 70 % mindre skade** – Høy (balanse)
   - Fil: `GSItems.h` `FGSEffectMods::Combine` linje 27–35 (Eier: Spillbarhet), `GSCharacter.cpp` linje 3119
     (`Speed * Mods.Speed`), 2533 (`Damage *= Mods.Taken`) og `AddEffect` linje 2436–2449 (Eier: Kamp).
   - Hva: alle effekter ganges sammen uten grense. Turbo × Adrenalin × Lynet × Berserk × Lattergass ×
     Koffein+ = 2,19 ganger fart. Grønn × Adrenalin × Morfin × Panser × Drøm × Julestemning = 29 % skade
     (i tillegg til vest). Regenerator + Feltkirurgi + Smertestillende + Grønn + Tran = 6,7 HP/s. Det
     finnes ingen overdose: ti ulike stoffer samtidig koster ingenting utover hver sin lille bivirkning.
   - Forslag: tak i `Combine` (fart maks +15 %, skade tatt minst 70 %; kan gjøres av Spillbarhet alene),
     og en «belastning» som øker for hvert stoff i kroppen: over en grense skjelving, tåkesyn, oppkast,
     og til slutt ekte overdose.

B4. **Sporing på PC er gratis veggsyn for alle, hele tiden** – Middels
   - Fil: `GSPlayerController.cpp` linje 344–345 og 455–485, `GSHUD.cpp` linje 1156 og 1161–1188. Eier: Kamp → bestilling.
   - Hva: hvilken som helst PC (én i hvert bygg, 16–17 stykker) viser navn og nøyaktig posisjon på alle
     levende spillere på hele campus i 20 s, med 45 s ventetid per spiller (ikke per PC). Ingen kan
     gjemme seg, og den som spores får ikke vite det.
   - Forslag: se R4 (vis bare det kameraene faktisk ser).

#### C. Realismehull (mot målet «superrealistisk», se `andreas.md`)

R1. **Sår leges på sekunder, og mat gir helse** – Høy
   - Fil: `GSCharacter.cpp` `CompleteUse` linje 2252–2269 (Eier: Kamp); `catalog.py` linje 239–243 og
     matlinjene 251–275 (Eier: Spillbarhet).
   - Hva: plaster gir +15 HP på 2 s, førstehjelpsskrin +75 på 6 s, «feltkirurgi» på seg selv +100 på 10 s,
     adrenalinsprøyte +35 på 1 s. Røkelaks gir +25 HP, kebab +15. All helse kommer i ett hopp når tiden
     er ute. Det finnes ingen blødning. Bruken avbrytes ikke av at man blir truffet.
   - Virkelighet: et skuddsår leges ikke i felt. Det man kan gjøre er å stoppe blødning (trykkbandasje,
     tourniquet: 20–60 s), dempe smerte (virker etter minutter) og holde seg i live. Mat leger ikke.
   - Forslag: blødning som egen tilstand etter treff (helse renner ut til den stoppes). Bandasje og
     tourniquet stopper blødning; smertestillende fjerner skjelving/halting en stund, ikke skade;
     førstehjelpsskrin gir langsom bedring over 1–2 min; mat gir bare sult/tørst/stamina. Treff
     avbryter behandlingen. Dataene (`heal=0` på mat) kan Spillbarhet rette alene.

R2. **Overnaturlige boosts** – Høy
   - Fil: `catalog.py` linje 18–58 og 211–231 (Eier: Spillbarhet); brukt i `GSCharacter.cpp` linje 2585–2603
     og 3168–3177 (Eier: Kamp).
   - Hva: Føniks og hjertestarter (stå opp fra de døde; hjertestarteren «lades» på deg selv på forhånd),
     Spøkelse (nesten usynlig), Øyet (ser alle gjennom vegger), Blodtørst (helse av skaden du gjør),
     Regenerator (3 HP/s), Pansertablett (25 % mindre skade av en pille), Lynet (50 % høyere hopp),
     Berserk (+30 % kuleskade) og Bulk (+12 % kuleskade). Et stoff kan ikke gjøre en kule sterkere.
   - Virkelighet: stoffer endrer kroppen, ikke våpenet: puls, skjelving, tunnelsyn, smerte,
     utholdenhet, reaksjon. Koffein, nikotin, adrenalin, morfin, amfetamin og alkohol har kjente
     virkninger og bivirkninger som kan brukes direkte.
   - Forslag: ta de overnaturlige ut av loot (eller bygg dem om: hjertestarter = gjenopplive en spiller
     som nettopp falt), og la resten virke på svai, stamina, smerte og syn. `damage`-modifikatoren bør
     bare gjelde nærkamp.

R3. **3D-printeren lager stål, optikk, sprengstoff og elektronikk av plast på sekunder** – Høy
   - Fil: `GSTerminal.cpp` linje 70–84. Eier: Spillbarhet.
   - Hva: 6–60 sekunder og 1–8 hele spoler for alt. Listen har lyddemper, rødpunktsikte, panserplate,
     snubletrådmine (130 skade), dirkesett, kompass og en ferdig speiderdrone. Én spole er i
     virkeligheten 1 kg plast; en kniv bruker 30–50 g, en hjelm 300–500 g.
   - Virkelighet: en vanlig printer lager plastdeler: 1–3 timer for en kniv, 15–40 timer for en hjelm
     eller en pistolramme. Den kan ikke lage linser, batterier, motorer, stålplater eller sprengstoff.
     En printet pistol («Liberator») trenger i tillegg en spiker som tennstift og tåler få skudd.
   - Forslag: (1) filament i gram, ikke hele spoler. (2) Printeren lager plastdelen; resten er deler man
     må finne (elektronikk i Elektrobygget, kjemikalier i kjemiblokkene, stålrør/spiker i verksteder).
     (3) Spilltid som står i forhold til størrelsen: 1 min (kniv) til 5–6 min (hjelm, pistol), så
     printeren blir et sted man må vokte. (4) Ta ut det plast ikke kan bli: mine, panserplate, sikte, dirker.

R4. **PC-en «vet» hvor alle er** – Middels
   - Fil: `GSPlayerController.cpp` `SendTrackPulse` linje 470–485, `GSHUD.cpp` linje 1083–1159. Eier: Kamp → bestilling.
   - Hva: en vakt-PC har kamerabilder og adgangslogger, ikke GPS på folk. Innlogging tar 1,4 s uten
     passord eller kort. Kameraene kan ikke ødelegges, og ingen ser at noen bruker dem.
   - Forslag: sporingen viser bare spillere som et kamera faktisk ser akkurat nå (siktlinje fra
     kameraet), merket med kameraets navn. Kameraer kan skytes i stykker. Innlogging krever
     NTNU-adgangskort (finnes alt som gjenstand) eller tar mye lengre tid uten.

R5. **Ingen vekt: fire tunge våpen og full sekk i full sprint** – Høy
   - Fil: `GSCharacter.h` linje 64–65 (4 våpen, 6 plasser), `GSCharacter.cpp` `Tick` linje 3092–3119 (farten
     ser bare på vest og hjelm) (Eier: Kamp); `gen_cpp.py` linje 42 (stabler: mat 5, medisin 5, granater
     4) (Eier: Spillbarhet).
   - Hva: Carl Gustaf (ca. 7–10 kg) + Barrett (14 kg) + MG3 (11,5 kg) + PKM (7,5 kg) med ammunisjon er
     over 45 kg, og bæres uten at fart, stamina eller hopp endres. Uten sekk har man 6 plasser, og én
     plass kan være 5 Grandiosa-esker eller 5 førstehjelpsskrin.
   - Forslag: vekt i kg på hver gjenstand i `catalog.py`, og samlet vekt som gradvis trekker ned fart,
     stamina-henting og hopp fra ca. 15 kg. Våpenplasser etter type: to lange våpen, én pistol, ett
     nærkampvåpen. Sekkeplass som volum i stedet for «plasser».

R6. **Man dør av tørst på 18 minutter, men kjenner ingenting før de siste minuttene** – Middels
   - Fil: `GSCharacter.cpp` linje 2919–2924. Eier: Kamp.
   - Hva: fra full til tom på 18 min (tørst) og 25 min (sult), så 1 HP/s til man dør (100 s). Med
     spillets døgn på 48 min er 18 min lik 9 timer. Et menneske tåler ca. 3 døgn uten vann og uker uten
     mat, men blir merkbart dårligere lenge før.
   - Forslag: ingen død av sult/tørst i en kamp; i stedet tidlige, gradvise følger (se B2).

R7. **Mat spises for fort, og «matforgiftning» varer 20 sekunder** – Lav
   - Fil: `catalog.py` linje 251–275 og 428–441 (Eier: Spillbarhet); `GSCharacter.cpp` linje 2265 (Eier: Kamp).
   - Hva: en hel Grandiosa spises på 4 s, en kebab på 3,5 s. Matforgiftning slår til med en gang og er
     over etter 20 s; i virkeligheten kommer den etter timer. Rakfisk-teksten lover at halvparten «blir
     sterke», men ingenting skjer. Meldingen ved forgiftning sier alltid «pizzaen», også for rakfisk.
   - Forslag: lengre spisetid for store måltider (10–20 s), og rett tekstene.

#### D. Ny spiller og flyt (hva som mangler for at en hel kamp henger sammen)

F1. **Man kan ikke skrive inn en adresse i menyen, og «Start server» har ingen bekreftelse** – Høy
   - Fil: `GSHUD.cpp` `DrawMenu` linje 161–175, `GSPlayerController.cpp` `MenuClick` linje 189–190, `Host`
     linje 221–228, `Join` linje 230–236. Eier: Kamp → bestilling.
   - Hva: (1) «Bli med» bruker bare forrige adresse (første gang 127.0.0.1). For en ny adresse må
     spilleren åpne konsollen med § og skrive `Join 192.168.1.20`. Unreal har ikke konsoll i et ferdig
     pakket spill (Shipping), så der kan man ikke bli med hos noen. (2) Ett klikk på «Start server» midt
     i en kamp laster kartet på nytt: er du vert, kastes alle ut og kampen er borte; er du gjest,
     forlater du kampen. (3) Feiler tilkoblingen (feil adresse, full server), får spilleren ingen
     beskjed; koden har ingen håndtering av nettverksfeil (ingen treff på `NetworkFailure`/`TravelFailure`).
   - Forslag: tekstfelt for adresse i menyen, «Er du sikker?» på «Start server» når en kamp er i gang,
     og en melding på skjermen når tilkoblingen feiler eller verten forsvinner.

F2. **Spilleren kan ikke velge navn** – Middels
   - Fil: ingen kode setter navn (søk etter `SetPlayerName`/`ChangeName` i `Source/` gir ingen treff).
     Eier: Kamp (menyen i `GSPlayerController`/`GSHUD`) → bestilling.
   - Hva: lobby, killfeed, poengtavle, vinner, dødskort og sporingskartet viser motorens standardnavn.
     Vinneren lagres dessuten som tekst (`GSGameState::Winner`), så to like navn kan ikke skilles.
   - Forslag: navnefelt i menyen, lagret i innstillingene og sendt med ved tilkobling (`?Name=`).

F3. **Lobbyen forklarer ikke hva som skjer** – Middels
   - Fil: `GSHUD.cpp` `DrawMatch` linje 756–788 (Eier: Kamp → bestilling), `GSGameMode.cpp` `StartMatch` linje
     91–117 (Eier: Spillbarhet).
   - Hva: lobbyen sier ikke at alt du plukker opp forsvinner når kampen starter (alle får ny kropp med
     tomme lommer), ikke at kampen starter av seg selv etter 90 s, og ikke hvor mange som mangler.
     Den som kommer inn midt i en kamp får ingen beskjed om at en kamp pågår.
   - Forslag: to korte linjer i lobbyboksen, og en melding «Kampen pågår, N min igjen» ved innkomst.

F4. **Printeren forklarer ikke seg selv** – Middels
   - Fil: `GSHUD.cpp` linje 383–386 og `DrawPrinter` linje 1042–1081 (Eier: Kamp → bestilling);
     `GSTerminal.cpp` `FinishPrint` linje 170–176 (Eier: Spillbarhet).
   - Hva: printeren viser «Filament i sekken: 0 spoler» uten å si hvor filament finnes. Listen sier ikke
     hva tingene gjør. Printeren sier ikke fra når den er ferdig, og hvem som helst kan ta det som
     kommer ut (realistisk, men bør stå der). Det ferdige ligger bare 3 minutter før det forsvinner
     (`SpawnDropped` setter levetid 180 s).
   - Forslag: beskrivelse av valgt oppskrift, en linje om hvor filament finnes, lydsignal fra printeren
     når den er ferdig, og ingen tidsgrense på det som ligger i printeren.

F5. **Sekken som bæres forsvinner ved bytte og ved død** – Lav
   - Fil: `GSCharacter.cpp` `TakeFromPickup` linje 2015–2027, `DropEverything` linje 2398–2420. Eier: Kamp → bestilling.
   - Hva: bytter du liten sekk mot stor, blir den lille borte (vester og hjelmer slippes på bakken).
     Når du dør, slippes ikke sekken du har på ryggen, så den kan ikke tas fra liket.
   - Forslag: slipp den gamle sekken som pickup i begge tilfeller.

F6. **Verten forsvinner = kampen er over uten beskjed** – Lav/Middels
   - Fil: `GSPlayerController.cpp` `Host` linje 221–228 (lytteserver, ingen overtakelse). Eier: Kamp → bestilling.
   - Hva: lukker verten spillet, mister alle forbindelsen uten forklaring (se F1 punkt 3).
   - Forslag: i første omgang en tydelig melding «Verten avsluttet spillet» og retur til menyen.
