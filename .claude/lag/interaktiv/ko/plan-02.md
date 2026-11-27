# Plan 02 (Interaktiv verden): oversikt – sprenge, knuse, bære

Skrevet om etter forhåndssjekken (`kontroll/forhand-plan-02.md`) og delt i tre. Byggeren følger delplanene:

- **`plan-02a.md`** – lettvegger og dørblad som sprenges/skytes i stykker, åpning per dør fra data. IMPROVEMENTS 1000–1012.
  Bygger på 1b-rettingene F1/F2 (gjøres nå, ikke del av planen).
- **`plan-02b.md`** – faste møbler som knuses, løse ting med fysikk, dvale med nettvekking, dytting, knusing, brannslukker,
  hardt tak 24/160, nettavstand for hele bygget. IMPROVEMENTS 1013–1028.
- **`plan-02c.md`** – bære, kaste, treffskade, barrikader. Etter Kamps K6 og K7. IMPROVEMENTS 1029–1036.

Felles: Kamps grensesnitt holdes uendret (se 02a). Alt replikert gjennom `AGSMapState`; delskade bare på serveren.

# Svar til Grafikk (bestillingen om `Props()`, deres plan-05b)
1. **Løse typer (`bLos = true`, vi legger dem ut):** `trestol`, `kontorstol`, `puff`. **Legg også til** som løse:
   `brannslukker` (gulvet ved hver trappe-/branndør, ca. 10), `flaske` (kantinebordene, ca. 20), `soppelbotte` (ca. 8).
   Alt annet er fast. Regelen «ikke i dørsving» gjelder også løse ting.
2. **Format** (generert, med vakt): `#define GS_HELGASETR_HAS_PROPS 1` og
   `struct FPropM { int32 Id; const TCHAR* Type; float X, Y, Z, Yaw, Sx, Sy, Sz; bool bLos; bool bCover; const TCHAR* Room; };`
   `inline TConstArrayView<FPropM> Props()`. `Sx/Sy/Sz` = mål i meter; `bCover` = møbelet er en dekningsdel i sjekk e3 (da
   knuser vi det aldri). `Sx/Sy/Sz`, `bLos` og `bCover` står også i manifestet (append-only som id-ene).
3. **Tagg:** `GS_Int_Prop` på alle faste møbel-ISM-er + `Prop_<type>` per ISM. **Legg til**
   `bool AGSHelgasetrBuilder::FindProp(int32 Id, UInstancedStaticMeshComponent*& OutComp, int32& OutIndex) const` som
   `FindBreachWall`. Instanser fjernes aldri (vi skalerer til 0). «Skjerm» hos oss = bare veggmonitoren, aldri skjermvegger.
4. **Ødelagte modeller lager vi.** Fra dere: opphav gulv-midt, X fremover; løse modeller `SM_Mod_Helg_Trestol/Kontorstol/
   Puff` uten Nanite, med enkle kollisjonsbokser.
5. **Lettveggene:** modellen i samme ISM, tag `GS_Int_Breach`, `FindBreachWall` – riktig; hull-modellen er vår.
6. **Runde atriehjørner: ja.** Glass-id-ene er bare nøkler i kjøretid; append-only, aldri gjenbruk.
7. **Ny:** bryter `--breach-open` i `check_helgasetr.py` (sjekk c, d, e, e2 med alle lettvegger fjernet), og kjør vårt
   `Tools/Blender/interactive_door_limits.py` etter `gen_helgasetr.py` når dører eller vegger flyttes.
