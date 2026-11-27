MÅ ENDRES

Forhåndssjekk av `.claude/lag/spillbarhet/ko/plan-07.md` mot koden (27. nov.) og Kamps `.claude/loop/ko/plan-10.md`.
Retningen er god og passer Andreas' «superrealistisk». Ingen Id-kollisjoner (Cigarettes, Beer, Tourniquet, Splint,
Naloxone, Royk, Alkohol, Nalokson, Stell finnes ikke fra før), `arch`-ene `tin`, `can`, `bandage` og `dropper` finnes i
`Tools/Blender/gen_models.py`, `GSTypes.h` inkluderer ikke `GSItems.h` (ingen sirkel), og det finnes ingen lagring
(`SaveGame`) av effekter. `Effects` replikeres til alle (`GSCharacter.h:131`, `DOREPLIFETIME` linje 406); et nytt
`uint8` nederst i `FGSActiveEffect` er trygt for vert, klient og sen klient. Visual 8–11 faller i dag til standardgrenen
i `UpdateLocalVisuals` (kantfarge 1,5 og vignett 0,6), ingen indeksfeil. Disse må likevel rettes før bygging:

## Endringer

1. **Ny dose nullstiller rusen (feil i designet).** `AddEffect` (`GSCharacter.cpp:2784`) setter `E.EndTime = Now + Duration`
   når samme stoff tas igjen. Med `Start = EndTime − Duration` starter innslaget da på nytt. Styrken faller til 0, og
   `Load · Doses · styrke` faller til 0 i samme øyeblikk. Den andre morfinsprøyta gjør deg altså edru et øyeblikk, og
   overdosen kommer først 8 s senere. Rett dette: legg også `UPROPERTY() float StartTime = 0.f;` nederst i
   `FGSActiveEffect` (samme header-bytte som `Doses`). `EffectStrength` bruker `StartTime` når den er > 0, ellers
   `EndTime − Duration`. I K-FX (2) til Kamp: ved ny dose beholdes `StartTime`, `EndTime = max(EndTime, Now + Duration)`
   og `Doses + 1`. Ny selvtest: Morfin med Doses=2 og StartTime 50 s tilbake gir styrke 1, ikke 0.
2. **Test (f) feiler på tallene i planen.** To amfetamin og KoffeinPlus gir 0,8·2 + 0,3 = 1,9. Koffein har klassen
   Caffeine, ikke Stimulant, så ×1,25 slår ikke inn. 1,9 er under 2,0, altså «Uvel» og ikke «Forgiftet». Velg én av to:
   (a) regelen blir «Stimulant sammen med Stimulant **eller Caffeine** → ×1,25», som gir (1,6+0,3)·1,25 = 2,375 og
   Forgiftet; eller (b) testen sier «Uvel». Anbefaler (a), fordi koffein forsterker amfetamin i virkeligheten. Skriv
   regelen likt i `DrugLoad` og i testen.
3. **Tersklene ligger nøyaktig på eksemplene.** 3 morfin gir 3·1,0 = 3,0, og morfin + 2 pils gir (1,0+1,0)·1,5 = 3,0.
   Begge treffer `≥ 3,0` bare når styrken er nøyaktig 1, så overdosen kan blafre av og på ved fadingen og ved
   flyttallsavvik. Det samme gjelder 2 morfin = 2,0. Sett terskelen med margin, `L ≥ T − 0,01`, eller gi Morfin load
   1,05 (da blir 2 morfin 2,1, 3 morfin 3,15 og morfin + 2 pils (1,05+1,0)·1,5 = 3,08). Det blir mindre skjørt.
4. **Belastningstrinnene stables, eller avløser hverandre? Planen sier det ikke.** Hvis de stables, gir overdose fart
   0,85·0,5 = 0,425, spredning 1,15·1,4 = 1,61 og tap på 0,5 + 2,5 = 3,0 HP/s (død på 33 s, ikke 40). Skriv at bare
   **det høyeste trinnet** gjelder, slik at overdose gir fart ×0,5 og −2,5 HP/s og du dør på 40 s fra 100 HP, som
   planen sier. Test (c) skal sjekke `Drain` = 2,5 (±0,01), ikke `≥`.
5. **Stort sanitetssett blir strengt dårligere enn førstehjelpsskrinet.** SurgeryKit (Legendary, 10 s) gir 30 HP + 0,35
   HP/s i 90 s. Medkit (Rare, 6 s) gir 35 HP + det samme. Forslag: SurgeryKit heal 45, og `Kirurgi` regen 0,5 i 90 s
   (45 HP mer), eller use 8 s. Uansett skal det sjeldneste settet være best.
6. **«heal=0 på all mat» mangler mange matvarer.** Listen i steg 2 har bare 8 ting. Disse har også `heal`: Farikal 10,
   Pinnekjott 20, Risgrot 5, Tran 10, Solbaer 5, Rakfisk 15 og Sodd 20 (`catalog.py` linje 429–441). Legg inn en
   `assert` i `gen_cpp.py` som sier at alle `Food` og `Drug` har `heal == 0`, så ingen blir glemt.
7. **Medisin før Kamps krok (4).** Kamp leser i dag `Treat = Heal/60` (`GSCharacter.cpp:2588`). Medkit 75 → 35 gir
   behandling 0,58 i stedet for 1,25, så skrinet setter ikke lenger beinet helt i stand før Kamp leser `Def->Treat`.
   Det er godtatt, men skriv det i risikoen og i rapporten, sammen med at Painkillers (heal 0) slutter å behandle lemmer.
8. **Ikke inkluder `GSTypes.h` i `GSItems.h`.** Forhåndsdeklarer `struct FGSActiveEffect;` i stedet (en `const TArray<…>&`
   i en deklarasjon trenger ikke hele typen), og inkluder `GSTypes.h` bare i `GSItems.cpp`. Da slipper `GSAudio`,
   `GSPickup` og de andre filene som inkluderer `GSItems.h` å gjenbygges hver gang `GSTypes.h` endres.
9. **Klemmingen i `Combine` avhenger av rekkefølgen.** Klemming etter hver ganging gir for eksempel
   [1,05; 1,05; 1,05; 0,9] → 0,99, mens klemming til slutt gir 1,04. Rekkefølgen er lik på server og klient (replikert
   liste), så det er trygt online. Skriv likevel at `CombineActive` klemmer **én gang til slutt** (ved å kombinere
   uklemt og så kalle `ClampMods`). `Combine`-klemmingen er bare en midlertidig vakt til Kamps krok (1).
   Stamina-forbruk: gulvet på 0,4 lar en flaggbærer på amfetamin og adrenalin bruke 60 % mindre utholdenhet.
   Foreslår 0,5.
10. **Del opp i 07a og 07b.** Planen har 7 filer pluss generert kode, ny regnemotor og 10 tester. Det meste av
    innslag, nedtur og overdose synes ikke i spillet før Kamp bytter `GetMods`, og plan-10 er fortsatt en skisse som
    venter på plan-08.
    - **07a (nå, synes med en gang):** hele header-byttet i ett (alle nye felt, inkludert `Doses` og `StartTime`,
      `EGSDrugClass`, `FGSVisualLayer` og funksjonsdeklarasjonene), `bNoLoot` og `Excluded`, de nye tallene,
      heal=0, tak i `Combine`, Sigarett, Pils, Årepresse og Skinne (ute av loot), spisetider, beskrivelser,
      `gen_cpp`-assertene, `gs.fx.test` (h)(i)(j), bestillinger og IMPROVEMENTS.
      Forbedrer: B3, R1, R2. Nytt: sigarett, pils, årepresse.
    - **07b (kort, bare `GSItems.cpp`):** `EffectStrength`, `DrugLoad`, `CombineActive`, `ActiveVisuals`, Nalokson
      (gjør ingenting uten 07b, så den flyttes hit) og test (a)–(g) og (1). Bygges når Kamp er klar til K-FX (1),
      eller rett etter 07a, siden headeren da er på plass og ingen andre lag må bygge på nytt.
11. **Online: trinnskifte gir små rettinger i bevegelsen.** `Mods.Speed` går inn i `MaxWalkSpeed` på både server og
    klient (`GSCharacter.cpp:4454`). Klientens `GetServerTime` er litt forskjøvet. Under innslag og nedtur er dette
    ufarlig (5 % over 20 s), men overdosetrinnet (fart ×0,5) er et brått sprang som kan krysse terskelen på ulike
    bildetider. Gi belastningstrinnene en myk overgang: lerp over 0,2 i L rundt terskelen, slik at de ikke hopper.
12. **Små ting.** `GSHUD.cpp:403` merker en effekt som «dårlig» når `Spread > 1,01`. Da vises Amfetamin, Adrenalin,
    Morfin og Alkohol som dårlige. Send det som en linje i bestillingen til Kamp (HUD), uten å endre det selv.
    Assert-listen i `gen_cpp.py` bør også ta med `nightvision` og `flashimmune` på ting som ligger i loot.

## Balanse (utregnet, med 07-tallene)
- **Fart:** høyest 1,05 per stoff, taket er 1,10. Amfetamin og adrenalin gir 1,1025, som klemmes til 1,10.
  Med Lattergass inne gir det 1,05·1,05·0,95 = 1,047.
- **Belastning uten flere doser (før Kamps krok):** amfetamin + adrenalin + fokus gir 1,8·1,25 = 2,25 (Forgiftet).
  Morfin + sovetablett + pils gir 2,1·1,5 = 3,15 (Overdose). Sigarett + snus + plaster gir 0,5 (ingenting).
  Pils alene: 2 doser = 1,0, 3 doser = 1,5 (Uvel), 4 doser = 2,0 (Forgiftet). Maks 5 doser gir 2,5, så alkohol alene
  kan ikke gi overdose. Det er rimelig.
- **Helse i CTF:** de eneste kildene blir Plaster 15, Bandasje 30 (to i startutstyret), Medkit 35 + 31,5 og
  sanitetssettet (se punkt 5). Med respawn i CTF er det spillbart og mer realistisk.
- **Overdose:** 40 s med −2,5 HP/s fra 100 HP gir tid til Nalokson eller til at stoffet går ut. Det holder som
  realisme uten å bli uspillbart.
