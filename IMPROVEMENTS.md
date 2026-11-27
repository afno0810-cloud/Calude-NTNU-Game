# Forbedringer i Helgasetr Shootout

Andreas' mål: minst 1000 forbedringer. Hver linje under er én konkret ting som er lagt til eller gjort bedre.
Nyeste runde øverst.

## Runde 5a (Grafikk og kart): lys i Helgasetr og bevis i Unreal (27. nov.)

Gammelt som er forbedret: importen tåler låste filer og skriver rapport (961), kollisjonen på modelldelene følger tegningen og rettes ved hver oppstart (960), sola og lampene i ekte enheter (962), eksponeringen har faste grenser (963), lootplassen i trappa er flyttet (972).
Nytt: fysiske materialer per flate, solstråler i dis, skyggelys, Lumen for innendørs, `Grafikk.Foto`, hodeløs import og fotorunde, lootplasser i de låste grupperommene.

960. Helgasetr-modellen sjekkes i Unreal uten at noen åpner editoren: alle 58 modelldeler skal være importert, ha presis kollisjon og riktige materialer, og trinn og karmer kolliderer der de er tegnet (Nanite-reserven har alle trekanter)
961. Importen av modeller og materialer tåler at en fil er låst av en annen prosess, prøver igjen og skriver en rapport; alle flater har fått fysiske materialer (stein, betong, gips, tre, stål, stoff, glass, linoleum), så kulehull, nedslagslyd og fottrinn følger flaten du treffer
962. Sola er like sterk som ekte sol, så dagslyset fyller atriet og lampene lyser som ekte lamper
963. Kameraet tilpasser seg lyset som et øye: korridorer og atrium er riktig opplyst, og natta er mørk
964. Solstråler står ned gjennom glasstaket i en lett dis, med skyggene av fagverket i lufta
965. Lumen er stilt inn for innendørs: mindre lekkasje i hjørner og bedre lys på små ting som rekkverk og søyleringer
966. De to midterste hengelampene over kantina kaster myke skygger av bord og folk
967. Basene har et skyggekastende taklys i lobbyen, så du ser skyggen av den som kommer rundt hjørnet
968. Innelysene betaler ikke for tåka, så solstrålene ikke koster bildefrekvens
969. Ny konsollkommando Grafikk.Foto: fast kamerarute med bilder og tidsmåling, som lukker spillet selv
970. Hodeløs import og fotorunde med én kommando hver, som nekter å starte hvis Unreal allerede kjører
971. Målinger av bildetid og GPU-tid fra Helgasetr før og etter, med og uten maskinvare-strålesporing
972. Lootplassen som lå inne i den gule trappa i 3. etasje er flyttet ut
973. Nye lootplasser inne i de låste grupperommene: bryt opp døra og finn bedre utstyr
974. Kartsjekken stopper lootplasser som ligger inne i vegger, trapper eller møbler
975. Lootplassene har faste id-er i manifestet, så de ikke flytter seg mellom runder

## Runde 7b (Spillbarhet): rus, overdose og motgift (27. nov.)

Gammelt som er forbedret: stoffene slås ikke lenger av og på brått (933), en ny dose starter ikke rusen på nytt (934), og taket på boosts regnes én gang til slutt (939). Lattergass regnes nå som et dempende stoff, så den teller med i farlige blandinger.
Nytt: overdose med myke trinn (935, 937), farlige blandinger (936), Nalokson nesespray (938), skjermlag for hvert stoff (940) og flere selvtester (941).
Merk: det meste synes i spillet først når Kamp kobler på krokene K-FX (1) og (2). Til da kan Nalokson tas uten virkning, og alt kan prøves med `gs.fx.test`.

933. Stoffer slår inn gradvis, når en topp og har en nedtur, i stedet for å slås av og på brått
934. En ny dose av samme stoff forlenger rusen uten å starte innslaget på nytt
935. Overdose: tar du for mye av ett stoff blir du uvel, så forgiftet, og til slutt dør du hvis ingen hjelper
936. Blandinger er farlige som i virkeligheten: morfin med alkohol eller sovetablett gir pustestans, og koffein forsterker amfetamin
937. Overgangene mellom uvel, forgiftet og overdose er myke, så farten ikke rykker (også online)
938. Ny: Nalokson nesespray opphever en opioid-overdose en stund, men morfinen kommer tilbake når den går ut
939. Taket på boosts regnes én gang til slutt, så rekkefølgen du tar stoffene i ikke betyr noe
940. Skjermeffekter klare for hvert stoff: rødt ved røyk, hvitt ved amfetamin, tunnelsyn ved adrenalin og overdose, dobbeltsyn ved alkohol (tegnes av Kamp)
941. Selvtest `gs.fx.test` sjekker innslag, doser, blandinger, overdose, motgift og myke overganger

## Runde 1b-retting (Interaktiv verden): glass etter kontrollen (27. nov.)

Gammelt som er forbedret: glassbitene fra en granat (734, 735), slag mot glass (736), sprekkelyden ved hagl (737) og kulehullene på dører (738).

734. En granat midt i mye glass lager høyst 40 glassbiter i samme øyeblikk og aldri mer enn 150 i lufta, så bildet ikke hakker
735. Glassbiter som ikke får plass, blir aldri hengende i lufta: ruta legger bare skårene på gulvet
736. Kniv og kolbe knuser ikke glass bak en vegg; veggen stopper slaget
737. Hagl mot et glassrekkverk gir én sprekkelyd, ikke ti oppå hverandre
738. Kulehull følger dørbladet når døra går; andre ting lager ingen ekstra tegnekall for kulehull

## Runde 7a (Spillbarhet): realistiske stoffer og medisin (27. nov.)

Gammelt som er forbedret: boosts har tak og stables ikke lenger til overmenneskelige verdier (920), de overnaturlige er ute av kartet (921), mat og stoffer leger ikke (922), medisinen er ny og det sjeldneste settet er best (923), Turbo og Drøm er blitt ekte stoffer (924, 929), spisetider (930) og beskrivelser (931).
Nytt: sigarett, pils, årepresse og skinne (925–928), og katalogen sjekker seg selv (932).
Merk: Kamp regner fortsatt behandling av bein og armer som helse/60 til de leser det nye feltet `Treat`. Til da setter førstehjelpsskrinet ikke lenger et bein helt i stand, og smertestillende behandler ikke lemmer. Skinnen kommer på kartet når Kamp leser feltet.

920. Tak på boosts: uansett hvor mange stoffer du tar blir du aldri mer enn 10 % raskere eller tåler mer enn 15 % mindre skade
921. Overnaturlige boosts (Spøkelse, Føniks, Øyet, Blodtørst, Berserk, Lynet, Iskald, Pansertablett m.fl.) finnes ikke lenger på kartet
922. Mat, adrenalin og morfin leger ikke lenger sår: mat metter, adrenalin og morfin demper smerte
923. Førstehjelpsskrin og stort sanitetssett gir litt helse med en gang og resten langsomt over halvannet minutt; det sjeldneste er best
924. Turbo heter nå Amfetamin og virker som det: mindre slitenhet, urolige hender, og et tungt krasj etterpå
925. Ny: Sigarett – litt roligere hender, men dårligere kondis og hoste etterpå
926. Ny: Pils – hver boks gjør siktet og ladingen dårligere
927. Ny: Årepresse (tourniquet) som stopper all blødning fra et bein eller en arm
928. Ny: Skinne for brukne bein og armer (på kartet når Kamp leser behandlingsfeltet)
929. Sovetabletten (før «Drøm») gjør deg ikke lenger immun mot blendgranater
930. Store måltider tar realistisk tid å spise: Grandiosa 12 s, kebab 10 s
931. Alle stoffer og all medisin har nye, nøkterne beskrivelser av virkning og bivirkning
932. Katalogen stopper seg selv hvis noen legger inn mat eller stoff som leger, eller gjør deg skuddsikker, usynlig eller udødelig

## Runde 13 (Kamp): verden som tar imot kuler, granater og slag, og rettinger (27. nov.)

Gammelt som er forbedret: kuler, granater, kniv/kolbe og bruk-tasten virker nå på dørene og glasset fra Interaktiv verden
(764–769), og lydtaket kutter ikke lenger dine egne skudd og romklangen i store kamper (771). Nytt: lyder for dører som
åpnes og lukkes (770) og en hodeløs glasstest (774).

760. Førstepersonsarmene skjules ett sted i stedet for å vippes synlig/usynlig hvert bilde (mindre arbeid for grafikken)
761. Grafikknivået i Esc-menyen og «Grafikk N» beholder din egen oppløsning, som i hovedmenyen
762. Hendene holder grepet også når du sikter bratt opp eller ned (målt med `gs.FP.ReachTest` ved ±70°)
763. Skudd på knærne til en som går teller som beinskudd (beinboksen dekker hele steget)
764. Kuler knuser vinduer og glassvegger og blir bremset av dem (litt mindre fart og skade per rute)
765. Kulehull blir værende i vegger, gulv og tak (etter underlag) i stedet for å forsvinne etter 40 s
766. Du kan skyte opp kodelåser og låsen på låste tredører
767. Granater, miner og C4 blåser opp dører, bryter låser og knuser glass i nærheten
768. Kniv og kolbe knuser glassruter, slår løs kodelåser og sparker dører – og treffer ikke lenger noen bak en hel glassvegg
769. Bruk-tasten gjør aldri to ting: det som er nærmest midt i blikket (dør, gjenstand eller terminal) vinner
770. Nye lyder for dører som åpnes og lukkes (håndtak, smekklås), fra Kenney (CC0)
771. Dine egne skudd og romklangen etter skudd kuttes ikke lenger når mange skyter samtidig: fjerne skudd viker først, og halene og egne skudd har egne plasser
772. Flaggmeldinger på radioen har egen plass og kan ikke stoppes av treffmarkører, øresus eller hjerteslag
773. Lydtesten måler hvor mange lyder motoren faktisk spiller, med fire skyttere samtidig
774. Ny hodeløs test `gs.Test.Glasskudd`: en ekte kule mot en glassvegg i Helgasetr, og loggen sier om den knuses

## Runde 6c (Spillbarhet): rettinger etter kontrollen av runde 6 (27. nov.)

Gammelt som er forbedret: bedre loot ved kodelåsdører havner bare inne i de låste grupperommene, ikke i korridoren utenfor (899), og lootplasseringen oppdager ting som ville ligget inne i en trapp eller vegg (900, 901).
Nytt: loggen sier hvor mange plasser som ble bedre bak en låst dør (902).

899. Bare lootplasser på innsiden av en låst kodelåsdør blir én tier bedre – korridoren utenfor gir ingen gratis belønning
900. En lootplass der gulvsporet starter inne i noe (trapp, vegg) blir hoppet over og skrevet i loggen
901. Hver lootplass sjekkes for plass rundt seg før tingen legges der; tingen i den gule trappa i 3. etasje dukker ikke lenger opp inne i trinnene
902. Loot-loggen teller hvor mange plasser som ble bedre bak en låst kodelås, og «uten gulv» teller også plasser som ligger inne i noe

## Runde 1b (Interaktiv verden): glass som knuses og kulehull (27. nov.)

Gammelt som er forbedret: glassfeltene i Helgasetr er nå ekte knusbart glass i stedet for klosser kulene gikk gjennom (718), glassdørene fra 1a kan knuses (724), og dørene har fått ekte modeller med håndtak og kodelås (732).
Nytt: herdet og laminert glass (719, 720), lyd, biter og skår (721–723), kulehull etter underlag (728, 729), tak på alt (730), sen klient (731) og testrom med glass (733).
Merk: kuler, granater og kolbeslag knuser glass og lager kulehull først når Kamp kobler på krokene K1–K3 (Kamps plan-13). Til da kan alt prøves med konsollen: Interaktiv.Knus, Interaktiv.Smell og Interaktiv.Merke.

718. Glass i rekkverk, glassbokser, glassvegger og glassdører kan knuses (343 glassfelt i Helgasetr)
719. Herdet glass smuldrer helt i ett treff, slik ekte herdet glass gjør
720. Laminert glass i rekkverkene sprekker der det treffes og gir etter til slutt – da kan du hoppe ned en etasje
721. Knust glass gir lyd: knus, sprekk og skår som faller
722. Glassbiter flyr i retningen treffet kom fra og legger seg på gulvet
723. Glasskår blir liggende på gulvet til neste kamp
724. Glassdører kan knuses, og da kan du gå gjennom rammen eller gripe inn og åpne, uten å bry deg om låsen
725. Klart for granater: de knuser glass i vid radius, også utenfor den dødelige radien (virker når Kamp kobler på K2; prøv nå med Interaktiv.Smell)
726. Klart for kuler: de knuser glasset de går gjennom og bremses litt av hver rute (virker når Kamp kobler på K1; prøv nå med Interaktiv.Knus)
727. Klart for slag med kolbe eller kniv: de knuser glass, men treffer ikke noen som står bak en hel rute (virker når Kamp kobler på K3)
728. Kulehull blir sittende i vegger, dører, metall og glass til neste kamp (virker for kuler når Kamp kobler på K1; prøv nå med Interaktiv.Merke)
729. Kulehull ser forskjellige ut i gips, betong, tre, metall og glass
730. Tak på kulehull (400), biter i lufta (150) og skår (3000), så bildefrekvensen holder også etter lange kamper
731. Den som kommer inn sent ser knust glass, sprekker og skår, men ingen gamle lyder eller biter i lufta
732. Nye modeller for tredør, branndør med panikkstang og trådglass, glassdørramme, håndtak, kodelås og glassbiter
733. Testrommet har glassvegg, galleri med glassrekkverk og plater av gips, tre og stål for kulehull (konsoll: Interaktiv.Knus, Interaktiv.Merke)

## Runde 4b (Grafikk og kart): Helgasetr som ekte modell (27. nov.)

Gammelt som er forbedret: klossbygget fra 4a byttes bit for bit mot Blender-modeller (800), glasset (810, 811), hengelampene (812, 822), flaggets høyde og sporprøven (817), dørdataene (818, 819), boksetakene på 4. etasje (820) og lyset under basene (821).
Nytt: sagtak og stålfagverk med skygger (801, 802), trelameller og downlights på dekkekantene (803, 804), runde søyler (805), gule trapper med vanger (806), løp og bro over atriet (807, 808), tre nye materialer (809), avstandsfelt (813), import bare ved endring (814) og nye sjekker med motprøver (815, 816, 823, 824).

800. Helgasetr er nå ekte 3D-modeller fra Blender, delt i 58 biter; klossene fra 4a brukes bare der en bit mangler
801. Sagtaket over atriet har hvite takflater og glass i de bratte flatene, som vender mot sør så sola skinner inn
802. Hvitt stålfagverk av runde rør under glasstaket (store Warren-fagverk med diagonaler, mønebjelker og sperrer) kaster skygger ned i atriet
803. Dekkekantene mot atriet har mørke, liggende trelameller og hvit underkant, som på bildene
804. Runde downlights i undersiden av dekkekantene rundt atriet
805. Betongsøylene er runde (Ø 0,5 m) med to svarte ringer
806. De gule trappeløpene har heldekkende gule vanger, svart håndløper og gul bunnplate under løpene
807. Gule trappeløp og en bro krysser atriet høyt oppe, fra 5. til 6. etasje, som på bildene (kulisse, ingen kommer dit)
808. Glassboksene i 4. etasje henger i hvite stenger fra takfagverket
809. Nye materialer: Helgasetr-gult på trappene og Ryddekroken, oransje mesaningulv og hvitmalt stål
810. Glasset er billigere og penere: nesten klart når man ser rett gjennom, grønnlig og speilende på skrå, uten refraksjon og tunge refleksjoner
811. Fast glass kaster ikke skygge, så sollyset slipper gjennom glasstaket
812. De store runde hengelampene (Ø 2,4 m) er ekte modeller med lysende underside
813. Bygningsbitene får dobbel oppløsning på avstandsfeltet, så lyset ikke lekker gjennom vegger og dekker
814. Modellene skrives bare på nytt når de faktisk er endret, så Unreal ikke importerer dem igjen for ingenting
815. Nytt sjekkskript som går gjennom selve modellen med stråler: spawn og flagg, åpne dører, trinn, rekkverk langs hele atriekanten, ingenting å stå på over 4. etasje og kroppsprøve, med motprøver
816. Rendringer av hver etasje ovenfra og fem utsnitt inne i atriet, til kontroll mot bildene
817. Flagget står nå på gulvet (13,9 m), og sporprøven i spillet kjører de samme sporene som GameMode
818. Alarmdøra til Trapp sør i 4. etasje er merket forseglet (ingen åpning), så ingen dør dukker opp i en hel vegg
819. Kodelåsdørene er merket låst i kartdataene, likt med det som skjer i spillet
820. Taket på glassboksene i 4. etasje stopper kropper og kan ikke stås på, og veggtoppene under har en hvit kappe
821. Korridorlysene i 3. etasje rett under basene er fjernet, så de ikke lyser opp basene gjennom gulvet
822. Hengelampenes lys rekker 13 m i stedet for 26 m, så det ikke skinner gjennom dekkene
823. Id-manifest: sjekken feiler hvis en eksisterende dør, glassrute, lettvegg eller dekning får ny id
824. Ny sjekk: ingen vegg-, stolpe- eller glasstopp over 4. etasje i atriet kan brukes som gulv

## Runde 1a (Bots): bots som spiller Capture the Flag (27. nov.)

Gammelt som er forbedret: «Spill mot bots» fyller nå lagene etter valgene på bot-siden (841), og gjenoppliving, lag, startutstyr, flaggradioen og drapslista gjelder også datastyrte spillere (845, 851).
Nytt: navnett laget i kode med selvtest (842, 855), ruter, forsvar og flaggroller (843–845), syn uten veggsyn, reaksjon, sikte og rekyl (846–851), fire vanskelighetsgrader (852) og hjelp når de står fast (854).

840. Bots: datastyrte soldater spiller Capture the Flag med deg og mot deg i Helgasetr, i samme kropp og med samme våpen, skade og blod som spillere
841. «Spill mot bots» fyller nå lagene med så mange bots du valgte på bot-siden, med vanskelighetsgraden du valgte
842. Botene finner veien gjennom ganger, rom, trapper og etasjer med et navigasjonsnett som lages når kartet bygges; låste dører skjæres ut av nettet til de låses opp eller brytes
843. Botene velger ulike ruter mot flagget: vest- og østsiden, rommene i fløyene, gjennom atriet og nede i kantina
844. Noen bots blir igjen og forsvarer eget flagg fra steder med utsyn mot dørene inn til baserommet, og bytter plass etter en stund
845. Botene tar flagget ditt og bærer det hjem en annen vei enn de kom, og henter sitt eget flagg tilbake når det er mistet
846. Botene ser bare det de faktisk ser: synsfelt, synsvidde og vegger teller, og de ser gjennom glass
847. Botene husker bare fiender de har sett, og vet om flaggbæreren bare når laget får den samme radiomeldingen som du
848. Botene trenger reaksjonstid før første skudd, og siktet blir bedre jo lenger de har sett deg
849. Botene sikter opp og ned: de treffer i trapper, mellom etasjer og tvers over atriet
850. Botene skyter salver på avstand, holder avtrekkeren inne på nært hold og må holde rekylen nede selv
851. Botene skyter ikke i fast glass de ikke kan skyte gjennom, og ikke når en lagkamerat står i skuddlinja – de flytter seg i stedet
852. Fire vanskelighetsgrader – Lett, Middels, Vanskelig og Ekspert – skiller bare på ferdighet, aldri på helse eller skade
853. Botene heter vanlige norske navn merket «(bot)» i drapslista og på poengtavla
854. Bots som står fast åpner døra foran seg, hopper eller velger en annen rute, og plukker opp ammo til våpenet sitt når den ligger rett ved
855. Navnettet sjekker seg selv ved kampstart: veien mellom basene og gjennom hver døråpning skrives i loggen, og `gs.bots.debug` tegner botenes ruter

## Runde 6b (Spillbarhet): PC-er, kameraer og stedsnavn i Helgasetr (27. nov.)

Gammelt som er forbedret: radiomeldingen om flaggbæreren sier romnavn og etasje i stedet for fløy (894), en ny kamp setter også kartet tilbake (895), og nøkkelkortet og dirkesettet har riktige tekster (896).
Nytt: PC-er, 3D-printere og kameraer i ekte Helgasetr-rom (890–893), testrommet i menyen (897) og menylyder (898).

890. Fire PC-er i ekte Helgasetr-rom: Audiologi 4.056, ferdighetsrommet for vernepleiere, Studentforeningen og grupperom 2.1
891. To 3D-printere i grupperommene i 3. etasje
892. Åtte overvåkingskameraer med navn, fra klatreveggen i kantina til korridorene i 4. etasje
893. Kameraene over atriet og kantina dekker en større vinkel enn kameraene i korridorene
894. Radiomeldingen om flaggbæreren sier rom og etasje («i Laberadene, 4. etasje»)
895. Dører, låser og knust glass settes tilbake når en ny kamp starter
896. Nøkkelkortet og dirkesettet forteller at de også åpner kodelåser på dører
897. «Test kartet» → Gå til → «Testrom for dører og glass»
898. Menyknappene gir et nøkternt klikk, og bekreftelse og tilbake har egne lyder

## Runde 6a (Spillbarhet): loot i Helgasetr (27. nov.)

Gammelt som er forbedret: ammo kommer etter hvor mange våpen som bruker kaliberet (885), loot leses fra kartdataene og legges på gulvet (880, 887), og den mytiske kassa er borte fra Helgasetr (888).
Nytt: lootsteder etter rom (882, 883), sjeldnere ting lenger fra basene (881), låste kasser (884), automatisk balanse mellom lagene (886) og konsollkommandoen `gs.loot.stats` (889).

880. Helgasetr har loot: 58 plasser i alle fire etasjer, med våpen, ammo, medisin, vester og granater
881. Det vanlige ligger nær basene og det sjeldne lenger ut mot midten av bygget
882. Ferdighetsrommene og laberadene har mest bandasjer, førstehjelpsskrin og smertestillende
883. Kantina og studentarbeidsplassene har mat og drikke i tillegg til våpen og ammo
884. Låste kasser med gode våpen i laberadene, i østfløyen og i kantina
885. Ammo kommer etter hvor mange våpen som bruker kaliberet: mye 9 mm og 5,56, ingen rakettammo uten rakettkaster
886. Begge lag har like mye og like god loot innen rekkevidde (automatisk balanse)
887. Tingene legges på gulvet eller bordet der de står, aldri i lufta over atriet
888. Ingen mytiske ting, fallskjermer, droner eller veggsyn i Helgasetr
889. Konsollen `gs.loot.stats` viser hvordan loot fordeler seg per rom og sjeldenhet

## Lydrunde 1 (Kamp): ekte lyd og rom (27. nov.)

Gammelt som er forbedret: skudd, fottrinn, kulenedslag, lading og treff har fått ekte innspilte lyder og flere varianter (662, 664, 670, 671, 673, 674, 676), og flagg-radioen kan ikke lenger druknes (675).
Nytt: romklang som måles der du står (663, 666), okklusjon bak vegger (665), eksplosjonslyd med trykkbølge (667, 668), lydens forsinkelse (669), utstyrsrasling (672) og et verktøy for lydbiblioteket (677).
Ekte lyder: Snake's Authentic Gun Sounds (skudd og våpenmekanikk), Kenney Impact Sounds og Kenney Interface Sounds (CC0). Sonniss er ikke brukt; glass, eksplosjoner og rusk er laget av Kenney-lyder eller syntetisert.

662. Skudd fra andre høres i lag: nært smell med mekanikk, dempet på middels hold og et dumpt drønn langt unna, med myk overgang
663. Skuddet får etterklang etter rommet skytteren står i: kort i grupperom, lang i atriet, hard og ekkoende i trapperommet
664. Egne skudd har en egen nærlyd med trykk og et tydelig klikk fra mekanismen, og romklangen kommer fra rommet du står i
665. Lyd bak vegger blir dumpere og lavere i stedet for å høres like klart gjennom betong
666. Romklangen skifter jevnt når du går fra en korridor til atriet eller inn i et trapperom, uten at kartet trenger egne lydsoner
667. Eksplosjoner fra granater, raketter og miner har lyd: et kraftig smell nært, et drønn innendørs og rusk som faller etterpå
668. En eksplosjon nær deg demper alt annet et par sekunder og gir øresus, men fottrinn og flagg-radio høres fortsatt
669. Smellet fra skudd og eksplosjoner kommer litt etter glimtet når det er langt unna (lydens fart, 343 m/s)
670. Alle lyder har flere varianter, og den samme varianten spilles aldri to ganger på rad
671. Fottrinn høres forskjellig på terrazzo, parkett, linoleum, stål, betong og teppe, og landing og glid følger underlaget
672. Når du sprinter, rasler utstyret ditt hørbart for dem rundt deg
673. Kulenedslag høres etter materialet: betong, gips, tre, metall, glass og jord
674. Lading, pumpe og bolt har egne lyder per våpentype
675. Lyder har prioritet: mange hylser og nedslag kan aldri drukne fottrinn eller flagg-radioen
676. Ekte innspilte lyder (Snake's Authentic Gun Sounds, Kenney) erstatter de syntetiske der de finnes, med kreditering av hver fil
677. Et nytt verktøy henter, klipper og normaliserer lydfiler fra lydbiblioteket, så nye ekte lyder bare trenger én linje i en liste

## Runde 1a (Interaktiv verden): dører og bruk-tasten (27. nov.)

Gammelt som er forbedret: dirkesett og NTNU-adgangskort åpner nå også kodelåser på dører (654, 655), bruk-tasten E virker på dører med hint i samme stil (649, 650, 659), og botene blir ikke stående foran lukkede dører (660).
Nytt: felles replikert tilstand for kartet (646–648), alle dørtypene i Helgasetr med lås, spark, rømning og branndører (651–658), og testrom med konsollkommandoer (661).

646. Ny felles tilstand for alt som kan åpnes og ødelegges i kartet, lik for alle spillere online
647. Den som kommer inn midt i kampen ser åpne dører og brutte låser slik de er, uten gamle lyder
648. Alle dører og låser settes tilbake når en ny kamp starter
649. Bruk-tasten (E) virker på dører: trykk for å åpne eller lukke, hold for å sparke
650. E gjør aldri to ting: den tingen du ser på når du trykker, er den som brukes
651. Dører går opp vekk fra deg, med lyd når de åpnes og smekker igjen
652. Dører kan sparkes opp med et høyt smell som andre hører
653. Låste tredører kan sparkes inn (tre spark); klart for å skyte opp låsen (virker når Kamp kobler på K1; prøv nå med Interaktiv.Knus)
654. Glassdørene til grupperommene har kodelås: NTNU-adgangskort åpner med et pip
655. Kodelåsen kan dirkes opp med dirkesett (6 s) eller brytes opp med makt (4 s, høylytt); klart for å skyte den i stykker (virker når Kamp kobler på K1; prøv nå med Interaktiv.Knus)
656. Låste dører og alarmdører kan alltid åpnes innenfra, som ekte rømningsdører
657. Branndører i trapperommene lukker seg selv, men aldri oppå noen som står i døråpningen
658. Klart for granater: de blåser opp dører i nærheten og ødelegger låsene (virker når Kamp kobler på K2; prøv nå med Interaktiv.Smell)
659. Kort hint midt på skjermen ved dører med framdriftslinje ved dirking og brekking
660. Klart for botene: de åpner ulåste dører de går mot (virker når Bots-laget har botene i spillet)
661. Testrom for dørene (konsoll: Interaktiv.Testrom, Interaktiv.Smell, Interaktiv.Knus)

## Runde 4a (Grafikk og kart): Helgasetr (27. nov.)

Gammelt som er forbedret: kartdataene som baser, spawn, flagg, minikart og «Flagget sist sett» leser (549, 558, 559), etasjenavnene i et bygg med ulike etasjehøyder (550), regn og snø inne (551).
Nytt: hele Helgasetr som spillbart kart (549–557), glassbokser, ruter og dekning (557, 560, 561), lagfarger (562), sperre mot gripekroken (563) og egne deler med nummer for knusing og sprenging (564).

549. Nytt kart: Capture the Flag spilles nå inne i Helgasetr (NTNU, Elgeseter gate 10); campuskartet bygges ikke lenger
550. Helgasetr har målene fra arkitektens tegninger: 51,5 × 67,8 m, søyleaksene 1–10 og A–N, og etasjehøyder på 5,6, 4,3 og 4,0 m
551. Atriet går gjennom alle seks etasjene opp til et sagtak av glass med hvitt fagverk, og regn og snø stopper ved taket
552. Ringkorridor rundt atriet i 3. og 4. etasje (og som U i 2. etasje) med betongsøyler og glassrekkverk med treoverkant
553. Kantina i 1. etasje med lyst gulv, Ryddekroken-tårnet, salatbaren, plantekassene og båsene der de står i virkeligheten
554. Mesaninen i 2. etasje med oransje gulv, sittetrapp og grupperom i glass, og studentarbeidsplassene med dobbel takhøyde
555. Atriumtrappa i tre gule løp med ekte trinn: langs vestsiden, tvers over atriet i lufta og langs østsiden
556. Trapp øst går fra 1. til 4. etasje og Trapp sør fra 1. til 3. etasje (alarmdøra på 4. er låst)
557. Glassboksene henger ut i atriet i 3. og 4. etasje, med lettvegg og whiteboard mot korridoren og glass mot atriet
558. Sørbasen (rødt lag) i undervisningsrommet i sørenden av 4. etasje og Nordbasen (blått lag) i nordenden, med to dører og skjermer innenfor som hindrer sikt tvers over atriet
559. Spawn i grupperom bak hver base, med sluse og L-sving ut, så ingen ser inn fra korridorene eller trappene
560. Seks veier mellom basene: vest- og østkorridoren, laberadene og østfløyens rom, gjennom atriet via trappene, og ned gjennom kantina
561. Dekning minst hver 8. meter i korridorene langs atriet: søyler, skap, sofagrupper og glassboksenes vegger
562. Røde og blå striper og skilt «SØR»/«NORD» viser hvilken base du er ved
563. Gripekroken fester ikke i noe over 4. etasje i atriet, og ingen kan stå på glassboksene eller fagverket
564. Glass, lettvegger og dekning er egne deler med nummer, klare til å bli knust, sprengt og flyttet i neste runder

## Runde 14b (Kamp): hendene og føttene (27. nov.)

Gammelt som er forbedret: de løse armene foran kameraet (582–587), egen skygge (590), huk og hodetreff (591–592), «Avslutt» i reservemenyen (593), bildeskarphet (597).
Nytt: kroppens egne hender på våpenet med lading, spising og kikkert (582–587), føtter i trapper (588–589), alt som er «bare for spilleren bak skjermen» holdes unna botene, og botene sikter i høyden (594–596).

582. Hendene som holder våpenet er nå kroppens egne hender, ikke løse armer foran kameraet (`gs.FP.Hands 0` eller en kropp som mangler gir de gamle armene)
583. Ved lading henter kroppens venstre hånd magasinet fra beltet på venstre side og setter det inn, og magasinet følger hånda hele veien
584. Når du spiser, drikker eller bruker noe, er det kroppens høyre hånd som holder tingen
585. Hendene henger ikke etter når du snur deg fort: våpenet og hendene regnes ut fra der du ser i dette bildet, ikke det forrige
586. Albuene peker ned, ut og litt bak, og gevær holdes litt nærmere kroppen i hofta, så hendene når fram med alle våpen (målt under 100 % av armlengden i hofta, i sikte og under lading)
587. Armene skjules når du ser gjennom en kikkert, så de ikke synes i siktebildet
588. Føttene står på trappetrinn og ujevnt gulv, og hofta senkes når den ene foten står lavere, både for deg og for andre nær deg
589. Fot-sporingen slås av for figurer som er langt unna, ikke synes eller beveger seg fort, så den koster lite
590. Skyggen din holder nå våpenet, og vest, hjelm, sekk og armbind er også med i skyggen
591. Hukende figurer skyver hofta bakover som i en ekte huk, så knærne er inne i beintreffet i stedet for 10–15 cm foran det
592. Hodetreffet er flyttet 3 cm bakover, så det dekker hodet også når andre sikter rett opp eller ned (målt: innenfor ved ±80 grader og hukende)
593. «Avslutt» i reservemenyen avslutter nå også Play i editoren
594. Datastyrte spillere gir ikke verten hjerteslag, treffmarkører, skjermeffekter, meldinger eller en førstepersonskropp
595. Verten hører botenes skudd og slag fra der boten står, og ser kulene deres, i stedet for å høre dem som sine egne
596. Bots sikter opp og ned også med selve skuddet, så de kan treffe mellom etasjer og i trapper
597. Skarpere bilde: første oppstart og gamle 72 % blir full oppløsning på Høy/Episk (85 % på Lav/Middels)

## Rettinger (Spillbarhet): test kartet og spawn (27. nov.)

Gammelt som er forbedret: fri flyging (710–713), spawn og flagg innendørs (714–716), teksten til lysspor (717).

710. Fri flyging ned virker med alle huk-tastene, både C og Ctrl (før bare den første), og opp og fort med alle tastene sine
711. Tekstene i test kartet viser alle tastene som virker i lufta, f.eks. «C / V. Ctrl ned»
712. Mens du flyr, hopper, klatrer, huker eller sklir ikke figuren i tillegg når du trykker Mellomrom eller C
713. Slår du av flygingen uten ledig plass og uten å ha stått på bakken før, settes du ned ved egen base, aldri inne i en vegg
714. Startplassene finner gulvet fra 2 m over punktet, så i Helgasetr havner du i rommet og ikke oppå dekket over
715. Flaggene settes på gulvet fra 2 m over basepunktet, så de aldri treffer taket over en innendørs base
716. En spiller som står på en startplass gjør ikke at neste spiller spawner oppå hodet hans
717. Lysspor-ammo sier nå riktig: kulene lyser og du ser hvor de går, men fienden ser hvor du skyter fra (ikke lenger «bedre treff»)

## Runde 5 (Spillbarhet): ny hovedmeny og test kartet (27. nov.)

Gammelt som er forbedret: hovedmenyens valg (598), vert og bli med (599), faste regler for kamplengde og erobringer (600, 605), menyen inne i spillet (611), «Til hovedmenyen» og tastelista (613).
Nytt: bot-siden og bot-kamp uten lobby (601–604), test kartet med fri flyging, flytting og lys/vær (606–610), pause (612). Selve botene kommer i neste runde; til da sier menyen og skjermen tydelig at du spiller alene.

598. Hovedmenyen har tre tydelige valg øverst: Spill mot bots, Spill online og Test kartet, og under dem Innstillinger, Kontroller og Avslutt
599. «Spill online» samler vert og bli med på én side, med adressene vennene skal skrive inn og feltet for å koble til
600. Som vert velger du kamplengde, antall erobringer og venneild før kampen starter, og alle som kobler til får de samme reglene
601. «Spill mot bots» har egen side: ditt lag, antall bots på hvert lag, vanskelighetsgrad, kamplengde, erobringer, venneild og startutstyr (til botene er ferdige står det tydelig at du spiller alene)
602. Valgene på bot-siden og vertssiden huskes til neste gang du starter spillet
603. En kamp mot bots starter med én knapp: kartet lastes, fem sekunders nedtelling, og du står på valgt lag uten å vente i lobbyen
604. Med startutstyr får alle i en bot-kamp det samme geværet, en pistol og to bandasjer ved hver start
605. Kampens lengde og antall erobringer for å vinne følger det du valgte, ikke faste 15 minutter og 3 erobringer
606. «Test kartet»: gå fritt rundt i hele kartet uten lobby, uten kamp og uten å ta skade eller bli sulten
607. Fri flyging i test kartet: N slår det av og på, Mellomrom flyr opp og C ned, og du går gjennom vegger
608. Når du slår av flygingen, settes du alltid ned på et gulv, aldri fast inne i en vegg
609. I test kartet kan du flytte deg rett til flagget og startplassen til hvert lag fra menyen
610. I test kartet kan du stille klokka og været fra menyen for å se kartet i morgenlys, kveld, regn eller tåke
611. Menyen inne i spillet viser knapper som passer det du gjør: start kampen på nytt og endre kampen i bot-kamp, flyging, flytting og lys i test
612. Spillet står stille på pause mens menyen er åpen i bot-kamp og test kartet (aldri i online-kamp)
613. «Til hovedmenyen» tar deg tilbake til startmenyen, også i editorens Play, og tasten for fri flyging kan endres i Kontroller

## Runde 14 (Kamp): hel kropp i førsteperson (27. nov.)

Gammelt som er forbedret: beina du ser når du ser ned (566), andre spilleres armer og gange (569, 570), huking (573), våpenet på andre (576), reservemenyen (579), inventar og «klar» under menyen (580), sår langt unna (581).
Nytt: egen kroppsanimasjon i C++ med åtte retninger og rotlås (567, 568), ryggbøy etter sikte (571), skytterstilling (572), knebøy i glid (574), hel skygge (575), hodet i hodeboksen (577), fallanimasjon (578).

566. Ser du ned, ser du din egen kropp – bryst, mage, bein og føtter – ikke bare beina
567. Din egen kropp går, jogger og stopper i takt med farten din, i åtte retninger
568. Kroppen glir ikke fram og hopper tilbake mellom skrittene (rotbevegelsen er låst)
569. Andre spillere holder våpenet med begge hender i stedet for å la armene henge
570. Andre spillere går og jogger med egne animasjoner for gevær og pistol, i åtte retninger
571. Andre spillere bøyer overkroppen litt etter hvor de sikter, uten at hodet flytter seg ut av treffet
572. Gevær og hagle holdes i skytterstilling med venstre skulder fram
573. Hukende figurer bøyer knærne og senker hoftene i stedet for å stå rett opp
574. Under glid bøyes knærne, og beina går ikke gjennom gulvet
575. Du har en hel skygge av deg selv på gulvet (våpenet kom med i skyggen i runde 14b, se 590)
576. Andre spilleres våpen følger skulderen når de huker
577. Hodet på andre spillere ligger der hode-treffet er, også når de huker (ryggbøyen er begrenset til 10 grader)
578. Fallanimasjon når du hopper eller faller, for deg og for andre
579. «Avslutt» og «Hovedmeny» i reservemenyen virker også når hovedmenyen ikke kan vises
580. Inventar og «klar» kan ikke slås av og på mens menyen er åpen
581. Sår på kropper langt unna (over 80 m) lages ikke, så store kamper går lettere

## Navn: Helgasetr Shootout (27. nov.)

565. Spillet heter nå «Helgasetr Shootout»: i hovedmenyens tittel, i vindustittelen og i README

## Runde 13 (Kamp): blødning, blodspor og sår på kroppen; frakobling og Esc-menyen (27. nov.)

Gammelt som er forbedret: bandasje og plaster (502), drap og dødsårsak ved blødning og brann (506–508), blodpølen (509), hjerteslaget og skadeblinket (510), Esc-menyen (513, 514), hinttekster og ammoesker (515).
Nytt: blødning (500, 501, 511), blodspor (503, 504), sår på kroppen (505), død ved frakobling (512).

500. Skudd- og knivsår blør: helsa renner sakte ut etter treffet, mest ved mageskudd, minst i armen
501. Små sår slutter å blø av seg selv etter en stund; et mageskudd gjør det ikke, og dreper på under ett minutt uten bandasje
502. Bandasje stopper blødningen, plaster stopper små sår, førstehjelpsskrin og feltkirurgsett stopper alt; piller hjelper ikke
503. Den som blør drypper blod på bakken der han går, så andre kan følge sporet; jo kraftigere blødning, jo tettere dråper
504. Blodsporet har sitt eget tak per maskin, så det aldri fjerner blodsprut på vegger eller pøler under lik
505. Blodflekk på kroppen der kula traff (hode, bryst, mage, arm eller bein), som følger kroppen når den går og faller
506. Den som blør i hjel gir drapet til den som skjøt ham, med våpenet og «(blødning)» i drapslisten
507. Dødskortet etter å ha blødd i hjel viser hvor skuddet kom fra, aldri skytterens nye kropp eller helse
508. Drap med brannammo går til skytteren med årsaken «brann», og matforgiftning heter matforgiftning, ikke «Overdose»
509. Den som har blødd i hjel blir liggende i en blodpøl, også når skuddet kom lenge før
510. Hjerteslaget begynner når du blør kraftig og slår fortere jo mer du blør; blødningen gir ikke dunk og rødt blink for hver dråpe
511. «Du blør» eller «Du blør kraftig» står nøkternt over helsestolpen
512. Den som kobler fra midt i en kamp dør der han står: utstyret og flagget blir liggende, og den som skadet ham de siste 10 sekundene får drapet
513. Esc-menyen (F10 eller P i editoren) har én knapp «Hovedmeny» i stedet for «Start server» og «Bli med», Avslutt spør før du forlater kampen, og konsoll-linja er borte
514. Grafikk i Esc-menyen viser riktig nivå og beholder oppløsningen din når du bytter; bildefrekvensen kan settes fra menyen uten chatlinje
515. Hinttekstene ved loot, PC, printer og lading viser tastene du selv har valgt, og en påbegynt ammoeske regnes som brukt (ingen gratis ammo)

## Runde 4 (Spillbarhet): overtid og avsløring (27. nov.)

Gammelt som er forbedret: kampslutt på tid (468, 469), frakobling av flaggbæreren (472), loot, kameraer og lys (473–475).
Nytt: overtid (468) og melding om hvor flaggbæreren sist ble sett (470, 471).

468. Overtid: kan et flagg som er ute avgjøre kampen når tiden går, spilles kampen ferdig til det er erobret eller hentet hjem, høyst 2 minutter
469. Når tiden er ute kan ingen ta et flagg som står hjemme, så overtiden kan ikke holdes i gang
470. Etter 20 sekunder med flagget får motstanderne en melding om hvor bæreren sist ble sett, med bygg og etasje, oppdatert hvert femte sekund
471. Bæreren får selv beskjed når motstanderne vet hvor han er
472. Kobler flaggbæreren fra, blir flagget liggende der han sto, og hendelseslista sier at han forlot kampen
473. Loot snurrer bare når du er nær det (lenger unna i kikkertsikte) og kaster ikke skygge mens det svever, så bildet flyter jevnere
474. Overvåkningskameraene svinger bare fort når noen er nær eller ser gjennom dem, og kaster ikke skygge
475. Lysene på PC-er, 3D-printere og kameraer tegnes ikke lenger på lang avstand

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
