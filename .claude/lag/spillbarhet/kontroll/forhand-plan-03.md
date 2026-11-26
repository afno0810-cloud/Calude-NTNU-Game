**BØR ENDRES**

# Forhåndssjekk – Spillbarhet plan-03 (knappehjelp, hint, avslørt bærer, overtid, Logout, sen innkobling, ytelse)

Lest mot `HEAD` (86ecf6c) med `git show`, mot `GSMenu.h`/`GSMenu.cpp` i arbeidsmappen (ucommittet plan-02) og mot
`kontroll/runde-01.md`. Ingenting er bygget eller spilt. Planen er ikke endret.

Planen er godt forankret i koden: signaturene den bruker finnes (`GSMenu::KeyName`, `Controls`, `IsModal`,
`AGSCharacter::GetFocusPickup/GetFocusTerminal/LegInjury/ArmInjury/Health`, `FGSWeaponSlot::Ammo/Reserve`,
`AGSPlayerController::bMenuOpen/IsOnTerminal/bShowInventory/bShowScoreboard/MappingContext`), rekkefølgen
pawn slettes → `Logout` stemmer, numrene 460–475 er ledige, og rettingene fra runde-01 (F1, F3, F5, F6) ligger
allerede i arbeidsmappen. Serverdelen av avsløringen og `Logout` kan bygges som skrevet.
Det som må endres før bygging er **tre regelhull (E1–E3), framvisningen av avslørt bærer (E4) og størrelsen (E5)**.

## Må endres i planen

**E1. Overtid som ikke kan endre noe (regel, Høy).**
`AnyFlagAway()` spør bare om et flagg er ute. Eksempel: rødt leder 2–1 og bærer det blå flagget når tiden går.
Overtiden starter, men blått kan ikke utligne (flagg som står hjemme kan ikke tas i overtid), og rødt har allerede
vunnet. Bæreren kan gjemme seg, og alle venter i inntil 2 minutter på et resultat som er gitt. Samme når laget som
angriper ligger under med 2 eller mer.
- Endring: bytt `AnyFlagAway()` med `AnyDecisiveFlagAway()`: sann hvis et flagg `F` er ute og
  `D = TeamScore(F->Team) − TeamScore(Other(F->Team))` er 0 eller 1 (en erobring gir seier fra uavgjort, eller
  uavgjort fra ett bak). Bruk den både når overtiden startes og i slutt-vilkåret
  (`bOvertime && (Now >= PhaseEndsAt || !AnyDecisiveFlagAway())`). Da slutter overtiden også straks det avgjørende
  flagget er hjemme, selv om det andre fortsatt er ute.
- Regeltekst 6 i H-oversikten: «Kan et flagg som er ute avgjøre kampen når tiden går, spilles det overtid …».

**E2. Flagg kan tas etter at tiden er ute (regel, Lav–middels).**
`AGSGameMode::Tick` går hvert 0,25 s, flagget sjekker hvert 0,1 s. I vinduet mellom `PhaseEndsAt` og neste
GameMode-tick er `bOvertime` ennå usann, så et flagg som står hjemme kan tas og selv utløse overtid.
- Endring i `GSFlag::ServerStep`: vilkåret blir
  `!(State == Home && (GS->bOvertime || GS->GetServerWorldTimeSeconds() >= GS->PhaseEndsAt))`.

**E3. Hintene: feil i lobbyen, feil i overtid, og de maser (Middels).**
- «`Lobby` for de seks første»: de seks første i tabellen er `OwnTaken, OwnDropped, Carry, EnemyFlag, Weapon,
  Item`. Flagg kan ikke tas i lobbyen (`ServerStep` kjører bare i `Playing`), så «Gå helt bort til flagget for å ta
  det» blir usant og bruker opp en visning. Skriv lista ut: i lobbyen bare `Weapon, Item, Crate, Computer, Printer,
  Empty`. Flagghint bare i `Playing`.
- `EnemyFlag` må ikke vises når flagget står hjemme og `bOvertime` er sann (kan ikke tas). Legg i stedet til en egen
  linje for den som står innen ca. 5 m av motstanderens hjemmeflagg i overtid: «Overtid: et flagg som står hjemme
  kan ikke tas.» Ellers står spilleren på plata og ingenting skjer.
- «Høyst to ganger» blir i praksis to ganger på 20 s: `Hurt` (liv under 60) og `Limb` varer lenge, så hintet
  vises 7 s, pause 6 s, vises igjen, og er brukt opp. Legg til: samme hint kommer ikke igjen før vilkåret har vært
  usant i mellomtiden **og** det er gått minst 60 s (eller: høyst én gang per kamp).
- Tell visningen først når hintet har stått i minst 2 s (åpner man sekken etter 0,3 s, er det ikke lest).
- `OwnDropped` sier nesten ordrett det samme som Kamps radiolinje i `GSHUD::HandleFlagEvent` («Flagget vårt ligger
  ute. Gå bort til det, så er det hjemme.»), og hendelsen står allerede i hendelseslista og i `GSHelp`s egen linje.
  Ta ut `OwnDropped`. `OwnTaken` kan bli (den sier noe nytt: stopp bæreren).
- Lagringen (`[Gloshaugen.Help]` i `GGameUserSettingsIni`, lest én gang, `Flush` bare når et tall endres) er riktig
  og billig. Flere PIE-vinduer deler tallene; det er greit. `ResetHints()` må også nullstille `OverviewOpened`.

**E4. Avslørt bærer: markøren er for arkade-aktig (Andreas' mål, Middels).**
Serverdelen er riktig og lekker ikke: `RevealLocation` oppdateres først etter 20 s, går til alle (flagget er
`bAlwaysRelevant`), og bærerens eget lag vet uansett hvor han er. Vert, klient og sen klient får samme verdier, og
`bRevealed`, `State` og `RevealLocation` kommer i samme oppdatering. Nullstillingen i fem overganger er riktig, og
klokka kan ikke lures ved å sende flagget videre (det finnes ingen slipp-tast, bare død). To merknader:
- Planens setning «ingen kan lese posisjonen ut av nettrafikken før 20 s» gjelder bare `RevealLocation`. Bærerens
  pawn replikeres til alle hele tiden (kartet er mindre enn relevansavstanden), så en jukser ser ham uansett. Det
  er et eget, gammelt hull (Kamps fil), ikke noe denne planen gjør verre. Stryk påstanden, før hullet i `funn.md`.
- Basene står 250 m fra hverandre og bæreren går med 85 % fart, så nesten hvert flaggløp varer over 20 s.
  Avsløringen blir altså regelen, ikke unntaket. En firkant gjennom vegger med meter-tall og pil på skjermkanten,
  oppdatert hvert 2. s, er da i praksis veggsyn på bæreren i mesteparten av hvert løp. Det er det Andreas ber oss
  unngå («velg realistisk», radiomeldinger i stedet for kommentator).
- **Nøktern løsning (anbefalt):** behold `bRevealed`/`RevealLocation` på serveren, men vis det som en
  **observasjonsmelding med stedsnavn**, ikke som markør i verden:
  «Flagget sist sett: inne i Gamle fysikk, 2. etasje» / «… ute, nord for Hovedbygningen».
  Regnes hos klienten fra `RevealLocation` og `GSCampus::Buildings()` (innenfor fotavtrykket → «inne i», etasje =
  `Z / FloorHeight + 1`; ellers nærmeste bygg og himmelretning, +X er øst og +Y er sør). Tegnes som egen linje under
  statuslinja for laget som eier flagget, i samme stil som de andre linjene, og byttes bare når teksten endrer seg.
  Sett `RevealInterval` til 5 s. Ingen meter, ingen pil, ingen firkant.
  Bestillingen til Kamp (punkt 2) endres til: vis `RevealLocation` som punkt på PC-kartet/minikartet for eierlaget.
  Da er «blir synlig for fiendene» oppfylt på to nøkterne måter (melding + kart).
- Vil dirigenten likevel ha en markør: tegn den bare når punktet er på skjermen (ingen kantpil), uten avstand, og
  ton den ut over 2 s etter hver oppdatering, så den leses som «sist sett her» og ikke som sporing.
- Linja til bæreren («Motstanderne vet hvor du er.») er god og kan stå.

**E5. Runden er for stor – del i to.**
11 filer, 16 forbedringslinjer, og `GSHelp.cpp` alene får seks nye deler (oversikt, påminnelse, statuslinjer,
sen innkobling, avsløring, hint med lagring). Løkkens egen lærdom er at slike planer ikke blir ferdige.
- **3A – hjelp:** steg 4, 5a–d, 6 og `GSMenu.cpp`, pluss sen innkobling (C2, både `RestartPlayer` og linja i
  `GSHelp`). Filer: `GSHelp.*`, `GSMenu.cpp`, `GSGameMode.*`, `IMPROVEMENTS.md`. Nytt: H-oversikt og hint.
  Forbedret: lobby-panelet og sen innkobling. Dette er Andreas' eldste ønske og bør komme først.
- **3B – CTF-regler og ytelse:** steg 1–3 (uten sen innkobling), 5e–f og 7. Filer: `GSFlag.*`, `GSGameState.*`,
  `GSGameMode.*`, `GSHelp.cpp`, `GSPickup.cpp`, `GSTerminal.cpp`. Nytt: overtid og avslørt bærer. Forbedret:
  frakobling og loot/kameraer. Regeltekstene 3 og 6 i H-oversikten legges inn i 3B, ikke før regelen finnes.
Den faste regelen (ett gammelt forbedret + ett nytt) holder i begge.

## Sjekket og funnet i orden

**CTF med overtid, alle kanter** (med E1 og E2 inne):
| Kant | Resultat |
|---|---|
| Flagg tas i siste sekund | Flagget er ute når tiden går → overtid hvis det kan avgjøre (E1). OK |
| Flagg tas rett etter tiden | Stoppes av E2 |
| Begge flagg bæres | Ingen kan erobre før ett eget flagg er hentet hjem (bærer må felles). Varer høyst 120 s, så avgjør stillingen. OK |
| Erobring i overtid | `Capture` setter `Home`, poeng økes, neste GameMode-tick avslutter. Poenggrensen sjekkes først. OK |
| Flagget hentes hjem i overtid | Ikke lenger ute → kampen slutter på stillingen. OK |
| Flagget ligger ute og ingen rører det | `SafetyReturnSeconds` 180 > 120, så taket på 2 min avgjør. OK |
| Uavgjort etter overtid | `EndMatch` gir tom vinner = uavgjort, som i dag. OK |
| Bærer kobler fra i overtid | `Logout` → `DropIfCarriedBy` → `Dropped` (fortsatt ute), overtiden går videre. Hele kjeden skjer i samme kall som pawnen slettes, så flaggets egen sjekk kommer ikke imellom; lista sier «Kari [mistet] det blå flagget» og «Kari [forlot] kampen». OK |
| Sen spiller i overtid | Får `bOvertime` og `PhaseEndsAt` replikert, spawner ved egen base. OK |
| Solokamp + venn kommer i overtid | `BackToLobby` nullstiller `bOvertime` (står i planen). OK |
| `bOvertime` henger igjen | Nullstilles i `StartMatch`, `EndMatch`, `BackToLobby`. OK |

**Logout:** `Exiting->PlayerState` lever, pawnen er borte, `Drop()` tåler det (bruker `LastCarrierLocation`, som
serveren oppdaterer hver frame; punktet ligger inne i kapselradien, så ikke i en vegg). `Super::Logout` kalles. OK.
Ikke dekket (til plan-04): et lag som blir tomt midt i kampen, og at koble av og på fyller hendelseslista (6 linjer).

**Sen innkobling:** `RestartPlayer` i `StartMatch` gir ikke falsk melding (lagene er satt i steg 2 før). OK.

**Tastenavn:** `KeyName` gir første binding for handlingen og tom streng før `MappingContext` finnes; planens reserve
(standardtast) dekker det. Handlingsnavnene i hinttabellen finnes i `GSMenu.cpp`. Lista over reserverte taster
bygges i `BuildRows(…, Reserved)`, ikke i `AssignKey` – H legges til der.

**Ytelse:** `GetPlayerViewPoint` gir kameraets sted også når spilleren ser gjennom et overvåkningskamera eller er
død, så loot rundt det man faktisk ser på snurrer. Vinkelen kommer fra klokka, så ingen hopp. Ingenting annet leser
`Mesh`-rotasjonen (ingen kollisjon, fokus bruker aktøren). `OnRep_State` slår ikke skyggen på igjen. Kameralyset
blinker på 2 Hz bare utenfor 40 m, der lyset uansett er skjult (30 m). `MaxDrawDistance` finnes på lyskomponenter.
Én svakhet: i kikkertsikte ser man loot på 60–100 m stå stille mens det nære snurrer. Bruk grensa
`3000 × 90 / FOV` (FOV fra `PlayerCameraManager`), eller godta det til loot legges på gulvet.

## Liste til byggeren
1. Les `GSHelp.cpp`, `GSMenu.cpp`, `GSFlag.cpp` og `GSGameMode.*` slik de står i arbeidsmappen (plan-02 og
   rettingene fra runde-01 ligger der, ikke i `HEAD`). Ikke rull tilbake objektsporene i `GSFlag.cpp`.
2. Overtid: `AnyDecisiveFlagAway()` (E1) begge steder i `Tick`; rekkefølgen i `Playing`-grenen ellers som planen.
3. `GSFlag::ServerStep`: hjemmeflagg kan ikke tas når `bOvertime` **eller** tiden er ute (E2).
4. Avsløring: server som planen, `RevealInterval = 5`. Framvisning etter E4 (stedsmelding), med mindre dirigenten
   velger markør-varianten. Ingen meter-tall og ingen kantpil i noen av variantene.
5. Hint: lobby-lista skrevet ut, ingen flagghint i lobbyen, `EnemyFlag` av i overtid for hjemmeflagg, `OwnDropped`
   ut, minst 60 s og «vilkåret har vært usant» mellom to visninger av samme hint, tell etter 2 s (E3).
6. Linje for den som står ved motstanderens hjemmeflagg i overtid (E3).
7. H: legg `EKeys::H` i `Reserved` i `BuildRows`, og legg raden «Hjelp: taster og regler / H» i
   `GSMenu::Controls()` (ikke bare i `GSHelp`), så menyens kontrollside viser den også. Åpne ikke oversikten mens
   sekken er åpen.
8. Sen innkobling i `GSHelp`: sett `FirstDraw` etter at `GS` finnes, og start de 10 sekundene først når `PS` og
   laget er kjent (ellers står det «Du er på uten lag»).
9. Ytelse som planen; gjerne FOV-skalert grense. Sjekk at `Empty`-hintet leser reserven der den faktisk ligger
   (slot eller felles lager per `AmmoType`).
10. Rapporter: hvilke taster PC-/printer-linjene ble sjekket mot, og hva som eventuelt ble tatt ut.

## Lærdom
- En regel som forlenger kampen må spørre «kan dette endre resultatet?», ikke bare «pågår det noe?».
- Sperrer som slås på av en treg klokke (GameMode 4 Hz) må også sjekke selve tidspunktet der hendelsen skjer.
- «Vises høyst N ganger» trenger en regel for avstand mellom visningene, ellers brukes alle opp i samme situasjon.
