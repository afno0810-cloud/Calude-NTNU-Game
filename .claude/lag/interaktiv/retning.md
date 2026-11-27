# Retning for laget Interaktiv verden

Eies av lagets planlegger. Andreas' ønsker i `.claude/loop/andreas.md` er rammen. Fra 27. nov. eier laget også
kapittel 6 («Interaktivitet» og «Ødeleggelse») i Grafikks kartfasit `.claude/lag/grafikk/helgasetr.md`: Grafikk lager
bygget og legger ut alt som kan åpnes/knuses som egne instanser med id; vi gjør dem levende.

## Visjon
Helgasetr skal føles som et ekte bygg midt i en skuddveksling. Dører går opp, smeller igjen og kan sparkes inn;
grupperommene har glassdører med kodelås som kan låses opp med adgangskort, dirkes eller brytes opp. Glass oppfører seg
som ekte glass: herdet glass i boksene og dørene smuldrer i ett smell til tusen småbiter på gulvet, laminert glass i
rekkverkene sprekker og holder til det til slutt gir etter (og da kan man hoppe ned en etasje). Kuler setter hull som
blir værende, granater blåser opp dører, river ned lettvegger og knuser alt glass i rommet. Løse ting (stoler, puffer,
flasker, brannslukkere) kan plukkes opp, bæres, kastes og stables foran en dør. Brytere slukker lyset i rommet, heisene
går, kiosken kan tømmes og pianoet kan spilles.

Fire regler for alt laget lager:
1. **Realistisk, ikke arkade.** Ekte materialer (herdet vs. laminert glass, tre vs. branndør), ekte tider (dirking tar
   sekunder, en spark er høylytt), ingen glødende omriss eller poengtekster.
2. **Alt replikert, ingenting avhenger av rekkefølge.** Én replikert tilstand (`AGSMapState`) for alle deler; den som
   kommer inn sent får riktig tilstand. Biter, støv og skår er lokale og kosmetiske.
3. **Billig.** Ingen replikerte aktører per dør eller glassfelt; instanser (ISM) og tak på alt som lages lokalt
   (fysikkbiter ≤ 150, skår på gulvet ≤ 3000, kulehull ≤ 400 per maskin). RTX 3050-budsjettet i helgasetr.md kap. 9 gjelder.
4. **Virker uten de andre lagene.** Alt kan prøves i eget testrom (`Interaktiv.Testrom`) før Helgasetr er bygget og før
   krokene hos Kamp/Spillbarhet er levert. Krokene bestilles som én linje hver.

## Spor
- **A. Grunnmur (runde 1):** `AGSMapState`, `IGSInteract`, dører, knusbart glass, varige kulehull, testrom.
- **B. Ødeleggelse og løse ting (runde 2):** sprengbare lettvegger og dører, møbler som knuses, løse ting som plukkes
  opp/bæres/kastes, barrikader.
- **C. Bygget som virker (runde 3):** lysbrytere, heiser, kiosk/kjøleskap/salatbar, skap med loot, piano, bordtennis,
  brannalarm, skjermer, hengelamper.
- **D. Finpuss:** modeller i Blender for alt (`Tools/Blender/interactive*.py`), lyder, animasjoner (Kamp), bots som
  bruker dører (Bots), ytelse målt i Helgasetr.

## Neste ideer (backlog, viktigst øverst)
1. **Runde 1 – grunnmuren** (`ko/plan-01.md`): AGSMapState, IGSInteract + AGSInteractor, AGSDoor (åpne/lukke, sparke,
   kodelås: kort/dirk/bryte opp/skyte låsen, branndører som lukker seg), knusbart glass (herdet/laminert), varige kulehull,
   testrom.
2. **Runde 2 – sprenge og bære** (skisse i `ko/plan-02.md`): lettvegger (`BreachWalls`) som sprenges, dører som blåses
   ut av karmen, møbler med hel/ødelagt modell, `AGSPhysicsProp` (plukke opp, bære, kaste, velte, ødelegge), barrikader
   foran dører, Bots-varsel når gangbare ruter endres.
3. **Runde 3 – brukbare ting** (skisse i `ko/plan-03.md`): lysbrytere per rom, heis sør/øst, kiosk og kjøleskap i
   Ryddekroken, salatbaren, garderobeskap, piano, bordtennis, brannalarm, skjermer som knuses, hengelamper som svinger.
4. Glass som knuses av granater som treffer det i flukt, og av kropper som kastes/faller mot det (eksplosjonsdytt).
5. Dirke- og brekkanimasjon i førsteperson (bestilles hos Kamp når Kamps runde 14–16 er ferdig).
6. Spark med fot synlig i førsteperson (Kamp), og at døra dytter den som står bak (ikke skade).
7. Lys som slukkes når lamper skytes (henger sammen med lysbryterne i runde 3).
8. Ekte sprekkemønster i laminert glass fra Blender (3 varianter) i stedet for stjernemerker.
9. Plan 5 og 6 åpnes med kortleser-dørene (adgangskort åpner «Kun ansatte») – høy utkikk, egen runde med Grafikk.
10. Brannslukker som kan skytes (hvit sky + dytt) og brukes til å blinde (passer Kamps røyk).
11. Tråkkelyd på glasskår og gips (fottrinn på skår – bestilles hos Kamp).
12. Måling: `stat unit` i atriet med 40 knuste felt og 150 biter i lufta; tak justeres etter målingen.

## Endringer i retningen
- 2026-11-27: Første retning skrevet. Valgt teknikk: lokale (ikke-replikerte) dør-aktører og instanser for glass,
  styrt av én replikert liste over endrede deler (`FFastArraySerializer`) – billig i nett og ytelse, og sen klient får
  riktig tilstand. Bruk-tasten fanges av lagets egen aktør (`AGSInteractor`, én per spiller, eid av spilleren), så
  runde 1 virker før Kamp har levert kroken; kroken gjør det bare penere. Glass får objekttypen `Destructible` og
  ignorerer `Visibility`, så siktlinjer (bots, HUD) ser gjennom glass, mens kulene finner glasset med et eget, billig spor.
- 2026-11-27: laget opprettet (Andreas: «legg til 2 sett til med agenter»).
