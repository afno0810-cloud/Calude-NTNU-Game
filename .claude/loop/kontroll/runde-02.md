**GODKJENT**

Runde 2 – å bli truffet (commit `94f1370`, gren `claude/dodskamera`). Kontrollert mot commiten, ikke arbeidsmappen.
Alt er vurdert ved å lese koden; ingenting er sett i spillet.

## Bygg
- Ikke bygget av kontrolløren (samtidig drift). Byggeren meldte `Result: Succeeded` for denne commiten.
- Commiten inneholder bare filene planen nevner (8 kildefiler/IMPROVEMENTS, `gen_sounds.py`, fem nye wav-er og
  flyttingen `plan.md` → `ko/plan-02.md`). Ingen gamle wav-er er endret, `Config/DefaultEngine.ini` er ikke med.

## Fast regel (gammelt forbedret + nytt)
- **Forbedret:** det røde helskjermsblinket er byttet ut med blodkant som følger skaden; retningspilen er blitt opptil
  fire piler med skygge; dødskameraet snur seg mot drapsmannen også når han er for langt unna (reserveposisjon).
- **Nytt:** hjerteslag + gråere bilde ved lavt liv, egne lyder for rustning/hodeskudd/rustning som ryker, øresus,
  grønt skjær ved heling. Regelen er oppfylt.

## Sjekk mot «Ferdig når»
- [x] `ClientDamageFrom` finnes ikke lenger i `Source/`; `ClientTookHit` kalles i `TakeDamage` før `Die()`.
- [x] Helrød boks er borte; rød kant skalerer med tapt liv (`LastHurtBlood`, dybde 12–22 % av høyden).
- [x] Rustning: blågrå kant + `Hit_Armor`; ryker: `Armor_Break`; hodeskudd: hvitt blink, `Ear_Ring`, rykk ×2,2.
- [x] Opptil fire kiler, slås sammen innen 25°, skygge under, dreier med `GetControlRotation()`.
- [x] Lavt liv: hjerteslag 1,05 → 0,55 s, puls i HUD i takt, metning og vignett. Slås av igjen ved heling
      (`bOverride_* = bLow`) og ved død (`OnRep_Dead`). Ny kropp etter respawn har egne, nullstilte felter.
- [x] Heling > 4 liv gir grønt skjær.
- [x] Sult/tørst: svakere lyd (0,4) og svak kant via `OnRep_Health`.
- [x] Visual 2/5 røres ikke (vakt i `ApplyLowHealthPostProcess`).
- [~] `FGSDeathInfo.KillerLocation` finnes og kameraet bruker reserveposisjonen. Men «navn når drapsmannen døde
      først» virker **ikke i praksis** – se funn 1.
- [x] Fem nye wav-er, ingen gamle endret. IMPROVEMENTS har 301–312 øverst.

## Byggerens avvik fra planen
1. **Treffkant etter kikkertsikte, før trådkors** (`GSHUD.cpp` ~87–92): riktig valg. Før ville det svarte siktet dekket
   kanten. Trådkorset tegnes fortsatt bare når du ikke er i kikkert. OK.
2. **`LastHurtBlood`**: gir rød kant i tillegg til blå når vesten bare tok deler av kula. Logisk og uten bivirkninger. OK.
3. **Hagl innen 0,1 s summeres, lyd maks én per 0,04 s** (med rang, så «vesten røk» fortsatt høres): OK. Pekeren `Same`
   brukes før listen endres, så ingen ugyldig peker.
4. **Hodeskudd-rykk startes i `ClientTookHit`**: OK. `UpdateLocalVisuals` lar det harde rykket spille i 0,15 s før et
   vanlig treff får starte på nytt, og setter `FlinchScale` tilbake til 1. Tåler at RPC og `OnRep_Health` bytter rekkefølge.
5. **Grått bilde fjernes i `OnRep_Dead`**: nødvendig og riktig – `UpdateLocalVisuals` kjøres ikke mer når kontrolleren har
   sluppet kroppen. `Camera` er alltid satt. Kjøres også for andres kropper, men det er bare to tilordninger.
6. **`OnRep_Health()` hos verten ved sult/tørst, Føniks og livstyveri** – sjekket spesielt, ingen dobbel effekt:
   - `OnRep_Health` gjør bare noe synlig når `IsLocallyControlled()`. For andres kropper på serveren oppdateres kun
     `LastSeenHealth`, som ikke brukes til noe annet.
   - Våpentreff på verten: `ClientTookHit` kjøres lokalt først og setter `LastHurtFlashTime`, så `OnRep_Health` rett
     etter hopper over den generelle lyden (0,3 s-vakten). Én lyd, én kant.
   - `TickSurvival` kaller den bare når helsa faktisk endres med hele poeng (ikke hver frame). Sakte regen er under
     4-poengsgrensen og gir ikke grønt blink hver gang.
   - Føniks hos verten: ser 0 og så 50 → grønt skjær oppå treffkanten. Klienter ser bare sluttverdien. Liten forskjell, ufarlig.
   - Livstyveri: grønt skjær bare når ett treff gir mer enn 4 liv – likt for vert og klient.
7. **Treff uten skade viser bare pil**: OK (ingen lyd/kant når ingenting gikk gjennom og ingen rustning ble truffet).
8. **Dødskort «han er selv død»**: teksten er grei, men kan ikke vises i dag – se funn 1.

## Funn
Ingen som stopper runden. Tre ting bør inn i en kommende plan:

1. **Forbedring 311 virker ikke for granater, miner, raketter og piler** – Middels.
   `GSProjectile.cpp` linje 140 og 200 sender `GetInstigatorController()` til `TakeDamage`. Når kasteren er død har
   kroppen hans ingen kontroller, så `Killer` er null i `Die()` og offeret får fortsatt «DU DØDE / Årsak: …».
   Vanlige skudd treffer i samme øyeblikk som de avfyres, så der har drapsmannen alltid en kropp. Dermed er den nye
   grenen i `Die()` (navn uten pawn) og teksten «han er selv død» i praksis kode som aldri kjøres.
   Fiks: lagre `TWeakObjectPtr<AController> OwnerController` i prosjektilet ved `Launch` og bruk den i begge
   `TakeDamage`-kallene (samme som funn 2 i `funn.md`, som også gir riktig poeng og killfeed).
2. **`Die()` bruker `Killer->GetPawn()`, som kan være drapsmannens nye kropp** – Lav nå, Middels når funn 1 er fikset.
   `GSCharacter.cpp` `Die()` (~2660): etter at funn 1 er rettet kan en granat drepe deg etter at kasteren har respawnet
   (4 s). Da viser kortet avstand, liv og rustning til den nye kroppen, og kameraet snur seg mot spawnpunktet hans.
   Fiks sammen med funn 1: la prosjektilet lagre kasterens posisjon ved kast, send den (og «kasteren var død»-flagget)
   inn i `Die()`, og bruk `Killer->GetPawn()` bare når den er samme kropp som skjøt og ikke er død. Vis da
   «han er selv død» når den opprinnelige kroppen er død, uansett om han har fått ny.
3. **Hodeskudd som knuser hjelmen mister hodeskudd-følelsen** – Lav.
   `TakeDamage` setter `Kind = ArmorBroke` foran `Head/HeadArmor`, så et hodeskudd som ødelegger hjelmen gir verken
   hvitt blink, øresus eller hardt rykk. Send hodeskudd som eget flagg (f.eks. bit i `Kind`).

Små merknader (ingen handling nå):
- `EdgeGlow` har 8 bånd på ~30 px ved 1080p. Med alfa 0,55 er trinnene mellom båndene opptil ~0,13, og det kan synes
  som striper. Be Andreas se etter det; i så fall bytt til flere bånd eller en myk gradient-tekstur/materiale.
- `DrawHurtOverlay` tegnes ikke på PC-terminalen (HUD returnerer tidlig), men hjerteslaget høres. Greit.
- Teksten «LAVT LIV - finn dekning og heal» er arkade-preg sett mot Andreas' nye mål (se forslag 2).

## Forslag til neste runde
1. **Prosjektiler husker hvem som kastet** (funn 1 + 2, og funn 2 i `funn.md`): riktig drapsmann, poeng, killfeed og
   dødskort for granat/mine/rakett/pil – også etter at kasteren er død eller har respawnet. Liten, avgrenset, og gjør
   to allerede loggførte forbedringer ekte.
2. **Skade som synes og høres – realistisk og grotesk (Andreas' nye mål):** blodsprut der kula treffer kroppen,
   blodflekk på vegg/gulv bak offeret, blodpøl som vokser under kropper som blir liggende, og blodspor/dråper på bakken
   når du har lite liv. Samtidig tone ned HUD-en mot det realistiske: fjern teksten «LAVT LIV …», erstatt hvitt
   hodeskudd-blink med kort uskarphet/dempet lyd (lavpass på all lyd mens øresusen varer), og la tung pust/stønn erstatte
   deler av hjerteslag-lyden.
3. **Sår der kula traff:** treffpunktet (hode, bryst, arm, bein) sendes allerede som `HitInfo` – bruk det til blodmerker
   på kroppen (decals) og ulik virkning (halting ved beintreff, ustø sikting ved armtreff).

## Lærdom
- Når en plan retter en feil «på mottakersiden» (her `Die()`), må den også sjekke at alle som kaller den faktisk sender
  dataene (her prosjektilenes `GetInstigatorController()`); ellers logges en forbedring som aldri kan skje.
- Planleggeren bør lese `funn.md` før en rettelse planlegges: funn 2 der beskrev akkurat hullet som gjorde 311 virkningsløs.
- Felter som nullstilles i `UpdateLocalVisuals` må også nullstilles der kroppen slutter å være lokalt styrt (`OnRep_Dead`);
  byggeren fanget dette selv – gjør det til vane.
