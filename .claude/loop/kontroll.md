**GODKJENT**

Runde 1 – dødskamera, drapskort og treffmarkører (gren `claude/dodskamera`, ikke committet ennå).

## Bygg
- `Build.bat GloshaugenEditor Win64 Development` kjørt av kontrolløren: «Target is up to date», `Result: Succeeded`.
  Kildekoden slik den ligger nå er altså det som ble kompilert. Ingen feil.

## Sjekk mot «Ferdig når»
- [x] Bygger.
- [x] `ClientHitMarker(bool bKill, bool bHeadshot, bool bArmor)`: eneste kall (`TakeDamage`) er oppdatert.
- [x] Fire markørtyper (hvit / blå / gull / rød med «smell» + HODESKUDD), med skygge, i både vanlig sikte og kikkert.
      Haglvern: en kropps-markør overskriver ikke en drapsmarkør innen 0,5 s.
- [x] Dødskamera: fall (0–0,35 s), trekk bakover til 2,8 m bak / 1,8 m over (til 1,6 s), deretter snu mot drapsmannen
      (+0,5 m løft) eller bane rundt kroppen. Sveip på `ECC_Camera` fra kroppen, kameraet trekkes inn straks noe er i veien
      og glir ut igjen. FOV nullstilles så kikkert-zoom ikke henger igjen.
- [x] Dødskort med navn, våpen, avstand, hodeskudd, liv- og rustningsstolper; nedtelling bruker `AGSGameMode::RespawnDelay` (4 s).
- [x] Sultedød: `Die(nullptr, ...)` gir tom `Killer` → «DU DØDE / Årsak: …» og bane rundt kroppen.
- [x] Etter respawn: FP-delene skjules bare på den døde kroppen (løkke over `bOnlyOwnerSee`-komponenter i `OnRep_Dead`);
      `ApplyWeaponVisuals`/`ApplyGearVisuals`/`ApplyAttachmentVisuals` har allerede `bDead`-vakter, og `UpdateWeaponAnimation`
      (som også styrer `UpdateItemUse` og kikkert-skjuling) kjøres ikke når død. Ny karakter har normale verdier.
- [x] Ingen endringer i `Config/DefaultEngine.ini` eller usporede filer er staget.
- [x] IMPROVEMENTS.md: «Runde 6» med 289–300 øverst.

## Nettverk (listen server + klienter)
- Dødsinfo bygges på serveren (har drapsmannens ekte liv/rustning) og sendes med én `Client, Reliable`-RPC. Riktig.
- Rekkefølgen er robust selv om pakkene kommer i ulik rekkefølge på klienten (kroppens kanal og PC-kanalen er ikke ordnet
  mot hverandre): serveren setter view target i `SetPawn`-overstyringen og etter `DetachFromControllerPendingDestroy`,
  klienten setter den i `ClientDeathCam` og igjen i `SetPawn(nullptr)` når `Pawn` replikeres, og `PlayerTick`-vakten
  fanger resten. Å nulle kroppens `PlayerState` lokalt i `ClientDeathCam` hindrer at kameramanageren «følger spilleren»
  bort fra kroppen når serverens null-PlayerState kommer – samme verdi som serveren uansett setter, så ufarlig.
- Listen-server-verten: `OnRep_Dead` kalles direkte i `Die()`, `ClientDeathCam` kjøres lokalt, `AcknowledgePossession`
  kalles ved ny pawn. Dekket.

## PlayerTick-vakten
Betingelsen `!GetPawn() && DeathCamBody.IsValid() && !ActiveTerminal.IsValid() && GetViewTarget() != DeathCamBody`
slåss ikke med noe jeg finner:
- Respawn: `SetPawn(ny)` og `AcknowledgePossession` nullstiller `DeathCamBody` før/samtidig med at view target flyttes
  (ClientRestart kaller selv `AutoManageActiveCameraTarget`). Hvis ny pawn ikke er replikert ennå, holder vakten bildet på
  kroppen til den er det – riktig oppførsel.
- PC-overvåkningskamera: vakten er av mens en terminal er aktiv, og `CloseTerminal`/`ServerViewCamera` går nå tilbake til
  kroppen (via `GetHomeViewTarget`) i stedet for `nullptr` når du dør mens du ser på et kamera. God ekstra rettelse.
- Kampstart (`GSGameMode` linje ~109–115, UnPossess + RestartPlayer): ny pawn rydder `DeathCamBody`.
- Den gjør bare `SetViewTarget` når målet faktisk er feil, så ingen RPC-er hver frame.

## Ytelse
Én kulesveip per bilde bare for den døde spilleren, ~30 HUD-bokser på dødsskjermen. Ubetydelig.

## Funn
Ingen som må fikses. To små svakheter, kan tas senere:
1. `GSCharacter.cpp` `Die()` (~linje 2515): `Info.Killer` fylles bare når drapsmannen har en pawn. Dør drapsmannen før
   granaten/kula hans dreper deg, står det «DU DØDE / Årsak: <våpen>» i stedet for navnet. Fyll navnet fra
   `Killer->PlayerState` uavhengig av pawn (bare liv/avstand trenger pawnen).
2. `ClientDeathCam(..., APawn* KillerPawn, ...)`: er drapsmannen utenfor nett-relevans for offeret (lange snikskyttertreff,
   > ~150 m), blir pekeren null på klienten og kameraet går i bane i stedet for å snu seg mot ham. Send drapsmannens
   posisjon i `FGSDeathInfo` som reserve.

## Forslag til neste runde
1. Commit denne runden (kun de 8 filene + IMPROVEMENTS.md, stage med navn) før noe nytt startes.
2. Skademarkør tilbake til offeret: retningsindikator er der, men treffet bør ha lyd/rist etter type (rustning-klink, hodeskudd),
   og drapskortet kan få en «Se drapsmannen»-markering (rød ring rundt ham i 1–2 s i dødskameraet).
3. Fyll de to små svakhetene over (navn uten pawn, posisjon som reserve).

## Lærdom
- Endringer i kamera/view target online må tåle at RPC-er og replikerte verdier kommer i vilkårlig rekkefølge på klienten;
  sett tilstanden på både server og klient, og ha en billig vakt som bare retter når noe faktisk er feil. (runde 1)
- `Build.bat` som sier «Target is up to date» betyr at byggeren allerede har kompilert nåværende kilde – greit bevis. (runde 1)
