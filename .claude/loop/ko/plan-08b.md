# Plan 08b – runde 14b (lag Kamp): ekte førsteperson, del 2 – hendene og føttene

Bygges etter at 14a (`plan-08.md`) er bygget og kontrollert. Bygger på `UGSBodyAnim`, `FPBody`, `SetupLocalBody`,
`FillBodyAnimInput` og grep-/debugvalgene fra 14a. Funn fra forhåndssjekken (`kontroll/forhand-plan-08.md`) står i
hakeparentes. Planleggeren skriver de siste detaljene når kontrollen av 14a er lest; stegene under er bindende.

## Mål
Hendene som holder våpenet er kroppens egne hender: de følger alle dagens bevegelser (sikting, lading med magasin,
pumpe, bolt, slag, veggstilling, å spise og drikke), føttene står på trappetrinnene, og armene synes ikke i kikkerten.

## Forbedrer (gammelt)
- De løse førstepersonsarmene (`SM_FPArm_R/L`) erstattes av `FPBody`s armer (14a skjulte `clavicle_l/r`).
- Ladingen: magasinet sitter i kroppens venstre hånd hele veien.
- Bruk av ting: kroppens høyre hånd holder tingen (`FPItemMesh`).
- Føtter i trapper og på ujevnt gulv (14a: bare knebøy ved huk).

## Nytt
- Hånd-IK på eierens `FPBody` mot de usynlige `FPArmR`/`FPArmL`/`FPItemHand`-punktene.
- Rekkevidde-klemming som flytter selve `FPArmL` (magasinet følger hånda).
- Fot-IK med gulvsporing.
- Armer skjult i kikkert.
- (Valgfritt, eget konsollvalg) førstepersonsskalering med delt kropp.

## Steg
1. **Hendene på `FPBody`:** fjern `HideBoneByName` på `clavicle_l/r` når `bTrueFP`. Skjul `FPArmR`, `FPArmL`, `FPItemHand`
   med `SetVisibility(false)` (ikke `SetHiddenInGame` på `FPArmL` – da forsvinner `FPMag` som henger på den), men behold
   dem som punkter; all dagens bevegelse i `UpdateWeaponAnimation`/`UpdateArms`/`UpdateItemUse` flytter dem fortsatt.
   Fallback (`!bTrueFP` eller manglende bein): synlige gamle armer.
2. **Mål uten verdensrom** [F6]: regn hånd-målene i kapselrom: våpenets plass relativt til kameraet × kameraets relative
   plass med rotasjon fra `GetControlRotation()` (relativt til aktørens rotasjon) × invers av `FPBody`s relative plass.
   (Kameraet får rotasjonen først i `GetCameraView`, etter alle tick; verdensplassen i `Tick` er fra forrige frame.)
   Alternativ: `Camera->SetWorldRotation(GetControlRotation())` rett før målene regnes. Skriv valget.
3. **Rekkevidde** [F5]: på spilltråden, klem målet for hver hånd til 97 % av armlengden (fra referansestillingen, skulder
   → hånd) og **flytt selve `FPArmL`** (og `FPArmR`) til det klemte målet, så `FPMag` følger hånda. Skytterstillingen fra
   14a (brystet vridd 15°) gjelder også `FPBody` for gevær/hagle. Logg rekkevidde i prosent per våpenklasse (hofte,
   sikte, lading) med `gs.FP.Debug 1`. Er noen over 100 % etter vridningen, flyttes våpenplasseringen (`FPHip`/`Pouch`-
   nøkkelpunkter i `UpdateWeaponAnimation`/`UpdateArms`), ikke katalogen. Kjente anslag fra forhåndssjekken: gevær i
   hofta (venstre hånd ved X≈42) ca. 58 cm og lading til lomma ca. 63 cm – begge utenfor 55 cm uten tiltak.
4. `UGSBodyAnim`: hånd-IK også når `bFirstPerson` (samme løser og skriving som 14a: `LocalBlendCSBoneTransforms`,
   NaN-vakt). Albuehint: skulder + (ned 30, ut 25, bak 10) cm, akser fra `CompFwd/Right/Up`.
   Bruk av ting: høyre hånd til `FPItemHand`s plass; venstre fri. Uten våpen og ting: ingen hånd-IK.
5. **Fot-IK** [F4b/B7]: `FillBodyAnimInput` sporer hver fot med `LineTraceSingleByObjectType` (`WorldStatic` +
   `WorldDynamic`, ignorer seg selv) fra hoftehøyde til 90 cm under `foot_l/r`, bare når fart < 200 cm/s, ikke fall,
   innen 25 m fra lokalt kamera og `GetMesh()->WasRecentlyRendered(0.2f)` (eller eierens egen). `FootOffsetL/R` klemt
   ±45 cm, `PelvisOffset = min(0, min(L, R))`. Proxyen senker hofta og løser beina (bygger på knebøy-IK fra 14a, som
   alltid er på ved huk). Bland ut på 0,1 s over 200 cm/s.
6. **Kikkert:** mens `IsScopedIn() && AimBlend > 0.85`: `HideBoneByName` på `clavicle_l/r` på `FPBody`, `UnHideBoneByName`
   etterpå.
7. **Førstepersonsskalering (bare hvis 1–6 er ferdige og testet)** [F8]: `FPBody` deles i to: armer og overkropp som
   egen komponent med `FirstPersonPrimitiveType::FirstPerson` og `thigh_l/r` skjult, beina som `None` med
   `SetLeaderPoseComponent`; våpen og `FPAtt*`/`FPMag`/`FPItemMesh` som `FirstPerson`; `gs.FP.Scale` standard 1,0 (av),
   prøv 0,5. Ikke hele kroppen som `FirstPerson` (røntgen-bein gjennom rekkverk og bord). Hopp over hvis tiden ikke rekker.
8. Test i editorens Play, 2 spillere: se ned og sikt/lad/pump/bolt/slå/spis/drikk med hvert våpen; snu fort (hendene
   skal ikke henge etter); gå opp og ned en trapp i Helgasetr og se på føttene; kikkert; `gs.TrueFP 0`.

## Ferdig når
- Ingen løse armer: hendene på våpenet er kroppens; rekkevidde ≤ 100 % for alle våpenklasser (tall i rapporten).
- Magasinet er i kroppens venstre hånd gjennom hele ladingen; hendene henger ikke etter ved rask snuing.
- Føttene står på trappetrinnene; armene synes ikke i kikkerten; `gs.TrueFP 0` gir gammel løsning.

## Risiko
Rekkevidde (F5) er det største: kan kreve at våpenplasseringen flyttes litt nærmere kroppen. Fot-sporing koster to spor
per kropp per frame (begrenset). Førstepersonsskalering kan påvirke lys på armene – derfor valgfritt og av som standard.

## IMPROVEMENTS-linjer (582–597)
582. Hendene som holder våpenet er nå kroppens egne hender, ikke løse armer foran kameraet
583. Ved lading tar kroppens venstre hånd magasinet ut og setter det nye inn
584. Magasinet følger hånda hele veien, også når armen er strukket langt
585. Ved pumpe og bolt er det kroppens hånd som drar
586. Når du spiser, drikker eller bruker noe, er det kroppens høyre hånd som holder tingen
587. Hendene henger ikke etter når du snur deg fort
588. Albuene peker ned og ut som når man holder et ekte våpen
589. Gevær og hagle holdes i skytterstilling også i din egen kropp
590. Føttene står på trappetrinn i stedet for å henge i lufta eller gå inni dem
591. Hofta senkes når den ene foten står lavere, så du står støtt på ujevnt gulv
592. Andre spilleres føtter står også på trinnene når du er nær dem
593. Armene skjules når du ser gjennom en kikkert, så de ikke synes inne i siktebildet
594. Våpenets plass er justert så hendene når fram ved hofta, i sikte og under lading
595. Fot-sporingen slås av for figurer langt unna eller utenfor synet, så den koster lite
596. Grepene for hendene kan justeres og vises i spillet (`gs.FP.Debug`) for finjustering
597. Hvis kroppen ikke kan brukes, kommer de gamle armene tilbake av seg selv
