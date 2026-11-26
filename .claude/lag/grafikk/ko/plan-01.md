# Plan 01 (Grafikk og kart): CTF-kjernen rundt Hovedbygningen, og dører og ramper som virker

## Mål
Kartet blir et lite, sammenhengende Capture the Flag-kart rundt Hovedbygningen (330 × 265 m) med to baser
og tre ruter, og spilleren kommer faktisk inn dørene i Hovedbygningen og opp rampene i basene.

## Forbedrer (gammelt)
- **F1:** dørene i Hovedbygningen er ikke lenger sperret av steinsokkelen.
- **F2:** rampene i byggene ender ikke lenger i en lomme bak rekkverket.
- **F4:** sola står i sør midt på dagen (var 90° feil).
- **F6:** hekkene står ikke lenger foran dørene på kortsidene.
- **F7:** stinettet er lagt om; ingen sti går inn i et bygg, og stien til hovedinngangen når helt fram.
- Trær, steiner, plenmøbler og parkerte biler settes bare der det spilles, og aldri på en spawnplass.

## Nytt
- Spillområdet `GSCampus::PlayArea()` og basene `GSCampus::Bases()` (flagg og spawn som data).
- Kulissebygg (`bBackdrop`): Sentralbygg 1 og 2, Bergbygget og Elektrobygget står utenfor uten innhold.
- Brolagt bakgård mellom fløyene og brolagt forplass foran hovedinngangen.
- Kantstein langs alle gangstier.
- Allé langs oppkjørselen og gatetrær langs Høgskoleringen.

## Hvorfor nå
Andreas har bestemt: mindre og mye mer detaljert kart, og Capture the Flag som eneste modus. Alt videre
detaljarbeid trenger at utsnittet og rutene ligger fast først. Kontrolløren fant samtidig at to av de tre
rutene i praksis er stengt (F1: ingen kommer inn i Hovedbygningen, F2: ingen kommer opp i 2. etasje i basene).
Hovedagenten har bestemt at F1 og F2 rettes i denne runden.

## Kartet (fasit for denne og senere runder)
Meter, X = øst, Y = sør, origo midt i Hovedbygningen.

- Spillområde: X −165 … 165, Y −125 … 140.
- Base vest = Gamle fysikk (midt −125, 10). Base øst = Gamle kjemi (midt 125, 10). Flagg midt i 1. etasje.
- Midten = Hovedbygningen med Lysgården.
- Nordruta: stien langs fasaden (Y = −30). Midtruta: endedørene i Hovedbygningen (X = ±55, Y = 0) og
  korridoren gjennom Lysgården. Sørruta: passasjene X = ±78, bakgården, promenaden Y = 120.

## Filer
- `Source/Gloshaugen/Public/GSCampus.h` (kartdata; står ikke i eiertabellen i `LAG.md`, men er kartets
  geometri og leses av `Tools/Blender/buildings.py`. Spawn-tallene er Spillbarhets å endre senere, se
  bestillingen i `.claude/lag/bestillinger.md`.)
- `Source/Gloshaugen/Public/GSCampusBuilder.h`, `Source/Gloshaugen/Private/GSCampusBuilder.cpp`
- `Source/Gloshaugen/Private/GSEnvironment.cpp` (én linje)
- `Tools/Blender/gs_common.py`, `buildings.py`, `building_hovedbygningen.py`
- Ny: `Tools/Blender/check_campus.py`, `Tools/Blender/check_walkable.py`
- Genereres på nytt: `SourceArt/Models/SM_Hovedbygningen.*`, `SM_Glass_Hovedbygningen.*`, og `SM_Bygg_*` /
  `SM_Glass_*` for Gamle_fysikk, Gamle_kjemi, Kjelhuset, Hovedbygningen_vest, Hovedbygningen_oest
- `IMPROVEMENTS.md`, `README.md` (avsnittet om «17 bygg»)
- **Røres ikke:** `Content/Python/gs_materials.py`, `Config/DefaultEngine.ini`, usporede filer i `Content/`
  og `SourceArt/PolyHaven`. Ingen modellfiler slettes (de seks byggene som går ut blir liggende i `SourceArt/Models`).

Blender kjøres slik (finnes på PC-en, testet i planleggingen med `details.py`):
`"C:\Program Files\Blender Foundation\Blender 5.3\blender.exe" --background --python <skript> -- <argumenter>`
(`python` på PC-en har ikke `bpy`.)

## Steg

### Del A – Feilene i modellene (gjør denne først; den er uavhengig av resten)

1. **`gs_common.py`: felles argumentleser.** Legg til nederst:
   ```python
   def cli_args():
       """Arguments after "--" when run by Blender, or after the script name when run with python + bpy."""
       import sys
       a = sys.argv
       if "--" in a:
           return a[a.index("--") + 1:]
       if os.path.basename(a[0]).lower().startswith("blender"):
           return []
       return a[1:]
   ```
2. **`buildings.py`:**
   - `main()`: bytt `names = sys.argv[1:]` med `from gs_common import cli_args` + `names = cli_args()`.
     (I dag blir Blenders egne argumenter tolket som byggnavn, så ingenting bygges under `blender --python`.)
   - `read_buildings()`: la regex-en godta et valgfritt ellevte felt, og les det:
     slutten av mønsteret blir `r'\s*(\w+),\s*(true|false)\s*(?:,\s*(true|false)\s*)?\}'`, og
     dict-en får `backdrop=m.group(11) == "true"`.
   - `main()`: hopp over bygg der `not b["enter"] and not b["backdrop"]`. Kulisser (`backdrop`) bygges
     bare når de er nevnt ved navn (ellers ville de blitt regenerert uten grunn).
   - **F2, linje 244–245 (rekkverk på rampen):** behold rekkverket på veggsiden (`iy1 - 0.1`) i full
     lengde. Rekkverket på gulvsiden (`oy0 + 0.1`) skal stoppe 2,6 m før toppen, samme åpning som
     gulvrekkverket (`rx1 = ix1 - 2.6`). Rampen stiger mot +X (toppen ved `ix1`):
     ```python
     # wall side: full length
     k.box((cxr, iy1 - 0.1, (z0 + zt) / 2 + 0.9), (length, 0.05, 0.05), m.metal, rot=(0, -RAMP_ANGLE, 0))
     # floor side: stops 2.6 m before the top, so you can step off the ramp onto the floor (F2)
     t1 = max(0.0, (ix1 - 2.6 - ox0) / ramp_len)
     if t1 > 0.05:
         k.box((ox0 + ramp_len * t1 / 2, oy0 + 0.1, z0 + FH * t1 / 2 + 0.9), (length * t1, 0.05, 0.05), m.metal, rot=(0, -RAMP_ANGLE, 0))
     ```
3. **`building_hovedbygningen.py`, F1, linje 152** (`ob(a0, a1, 0.0, 1.0, 0.25, STONE_D)` i `facade`):
   del sokkelen rundt dørene:
   ```python
   # plinth, split around the doors so you can walk in (F1)
   edges = [a0]
   for (c, w, h) in sorted(doors):
       edges += [c - w / 2, c + w / 2]
   edges.append(a1)
   for al, ah in zip(edges[0::2], edges[1::2]):
       if ah - al > 0.01:
           ob(al, ah, 0.0, 1.0, 0.25, STONE_D)
   ```
   Sjekk også om tårnets vegger eller andre kall legger en sokkel foran døra inn til tårnet (y = 14,0);
   kontrolløren fant sperre der også. Alt som står i døråpningen under 2 m skal bort.
4. **Generer modellene på nytt** (fra prosjektmappen):
   ```
   blender --background --python Tools/Blender/building_hovedbygningen.py
   blender --background --python Tools/Blender/buildings.py -- Gamle_fysikk Gamle_kjemi Kjelhuset Hovedbygningen_vest Hovedbygningen_oest
   ```
   Sjekk i utskriften at fem bygg ble «built», og at `.fbx` og `.blend` har ny tid. (Unreal importerer
   dem selv neste gang editoren åpnes, fordi `.fbx` er nyere enn `.imported`-merket.)
5. **Ny `Tools/Blender/check_walkable.py`** (kjøres med Blender, leser bare): åpner en `.blend`, skyter
   stråler og skriver `OK`/`SPERRET` per stråle, og avslutter med kode 1 hvis noe er sperret.
   Husk at modellene er speilet i Y i Blender (Unreal y → Blender −y); bruk `obj.ray_cast` i objektets rom.
   - `SM_Hovedbygningen.blend`: for hver dør, to stråler (z = 0,3 og z = 0,9) 3 m rett gjennom åpningen,
     fra 1,5 m utenfor til 1,5 m innenfor: hoveddøra (x 0, y −18), endedørene (x ±55, y 0), sørdørene
     (x 0 og ±48, y 14), tårndøra mot sør (x 0, y 26). Ingen skal treffe noe.
   - `SM_Bygg_Kjelhuset.blend` og `SM_Bygg_Gamle_fysikk.blend`: fra et punkt 1,0 m over rampen, 1,3 m før
     toppen, en stråle 2 m sidelengs mot gulvet (−Y i Unreal-rom) i høyde 0,3 / 0,9 / 1,5 m over
     etasjegulvet. Ingen skal treffe. Og én stråle samme vei midt på rampen i 0,9 m høyde over rampen:
     den SKAL treffe rekkverket (det står fortsatt der nede).
   Mål for rampen regnes som i `buildings.py` (`ramp_len = FH / tan(25°)`, `ix1 = sx/2 − 0.4`, `oy0 = sy/2 − 0.4 − 2.6`).
6. **Render to bilder** til `.claude/lag/grafikk/kontroll/` så kontrolløren kan se resultatet:
   hovedinngangen (`render_building.py SourceArt/Models/SM_Hovedbygningen.blend .../r01-hovedinngang.png 0 45 3 0 18 3`,
   Blender-rom: Unreal y −45 = Blender y +45) og toppen av rampen i Kjelhuset (velg kamera inne i 2. etasje
   som ser mot rampetoppen).

### Del B – Kartdata (`GSCampus.h`)

7. **`FBuilding`:** legg til nederst i structen: `bool bBackdrop = false;   // scenery outside the play area: model only, no loot, furniture or props`.
8. **Bygglisten** blir (fjern fargene `Light` og `Glass`, de brukes ikke lenger):
   ```cpp
   { TEXT("Hovedbygningen"),          0.f,    0.f, 110.f, 28.f,  0.f, 22.f, 5.5f, Stone, true },
   { TEXT("Hovedbygningen vest"),   -48.f,   34.f,  14.f, 40.f,  0.f, 18.f, 4.5f, Stone, true },
   { TEXT("Hovedbygningen oest"),    48.f,   34.f,  14.f, 40.f,  0.f, 18.f, 4.5f, Stone, true },
   { TEXT("Hovedbygningen taarn"),    0.f,   20.f,  12.f, 12.f,  0.f, 42.f, 4.f,  Stone, false },
   { TEXT("Gamle fysikk"),         -125.f,   10.f,  46.f, 22.f,  0.f, 16.f, 4.f,  Brick, true },   // base vest
   { TEXT("Gamle kjemi"),           125.f,   10.f,  56.f, 22.f,  0.f, 16.f, 4.f,  Brick, true },   // base oest
   { TEXT("Kjelhuset"),             -62.f,  105.f,  22.f, 18.f,  0.f, 12.f, 4.f,  Brick, true },
   // Scenery outside the play area
   { TEXT("Sentralbygg 1"),         -20.f,  190.f,  34.f, 22.f,  0.f, 52.f, 4.f,  Concrete, false, true },
   { TEXT("Sentralbygg 2"),          60.f,  215.f,  40.f, 22.f,  0.f, 44.f, 4.f,  Concrete, false, true },
   { TEXT("Bergbygget"),           -160.f,  185.f,  60.f, 34.f,  0.f, 18.f, 4.5f, Brick, false, true },
   { TEXT("Elektrobygget"),         235.f,  160.f, 120.f, 46.f,  0.f, 20.f, 4.f,  Brown, false, true },
   ```
   IT-bygget, Stripa, Kjemiblokkene, Varmeteknisk, Realfagbygget og Idrettsbygget tas ut av listen.
   Oppdater kommentaren øverst i fila (utsnittet, CTF, hva som er kulisse).
9. **Veilisten** blir:
   ```cpp
   // Hoegskoleringen (car road, north edge)
   { FVector2D(-260.f, -110.f), FVector2D(260.f, -110.f), 9.f, false },
   // North route
   { FVector2D(0.f, -110.f),    FVector2D(0.f, -27.f),    6.f, true },  // up to the main entrance
   { FVector2D(-158.f, -30.f),  FVector2D(158.f, -30.f),  4.f, true },  // along the facade, base to base
   { FVector2D(-125.f, -30.f),  FVector2D(-125.f, -3.f),  3.f, true },  // to the north door of each base
   { FVector2D(125.f, -30.f),   FVector2D(125.f, -3.f),   3.f, true },
   // Middle route: end doors of Hovedbygningen and of the bases
   { FVector2D(-78.f, 0.f),     FVector2D(-57.f, 0.f),    3.f, true },
   { FVector2D(57.f, 0.f),      FVector2D(78.f, 0.f),     3.f, true },
   { FVector2D(-100.f, 10.f),   FVector2D(-78.f, 10.f),   3.f, true },
   { FVector2D(78.f, 10.f),     FVector2D(95.f, 10.f),    3.f, true },
   // South route
   { FVector2D(-78.f, -30.f),   FVector2D(-78.f, 120.f),  4.f, true },  // west passage
   { FVector2D(78.f, -30.f),    FVector2D(78.f, 120.f),   4.f, true },  // east passage
   { FVector2D(-200.f, 120.f),  FVector2D(200.f, 120.f),  5.f, true },  // south promenade
   { FVector2D(-125.f, 23.f),   FVector2D(-125.f, 120.f), 3.f, true },  // from the south door of each base
   { FVector2D(125.f, 23.f),    FVector2D(125.f, 120.f),  3.f, true },
   { FVector2D(0.f, 58.f),      FVector2D(0.f, 170.f),    5.f, true },  // from the courtyard towards Sentralbygg 1
   ```
   (Regnet gjennom i planleggingen: ingen av disse, med endene forlenget en halv bredde slik
   `BuildRoads` gjør, overlapper et bygg.)
10. **Nye data** (etter `Roads()`):
    ```cpp
    /** The part of campus the match is played on (m). Everything outside is scenery. */
    inline FBox2D PlayArea() { return FBox2D(FVector2D(-165.f, -125.f), FVector2D(165.f, 140.f)); }

    /** Capture the Flag bases. Index 0 = west, 1 = east. Flag and spawn positions in meters. */
    struct FBase
    {
        const TCHAR* Name;
        const TCHAR* Building;
        FVector Flag;            // on the ground floor
        float SpawnYaw;          // degrees, facing the middle of the map
        TArray<FVector2D> Spawns;
    };
    inline const TArray<FBase>& Bases()
    {
        static const TArray<FBase> List = {
            { TEXT("Vest"), TEXT("Gamle fysikk"), FVector(-125.f, 10.f, 0.f), 0.f,
              { { -157.f, 10.f }, { -140.f, -14.f }, { -125.f, -16.f }, { -110.f, -14.f }, { -140.f, 34.f }, { -125.f, 36.f }, { -110.f, 34.f } } },
            { TEXT("Oest"), TEXT("Gamle kjemi"), FVector(125.f, 10.f, 0.f), 180.f,
              { { 159.f, 10.f }, { 140.f, -14.f }, { 125.f, -16.f }, { 110.f, -14.f }, { 140.f, 34.f }, { 125.f, 36.f }, { 110.f, 34.f } } },
        };
        return List;
    }
    ```
11. **`SpawnPoints()`** (brukes av dagens spill til CTF er på plass): bytt innholdet til de 14 basepunktene
    over pluss `{ -60.f, -80.f }, { 60.f, -80.f }, { -20.f, 125.f }, { 20.f, 125.f }` (18 punkter, alle
    inne i området og minst 1,5 m fra bygg; regnet gjennom i planleggingen).
12. **Ny `Tools/Blender/check_campus.py`** (vanlig `python`, uten `bpy`): leser `GSCampus.h` med regex og
    sjekker: (a) alle bygg uten kulisse-merke ligger helt inne i `PlayArea`, (b) ingen vei (forlenget en
    halv bredde i hver ende) overlapper et bygg-fotavtrykk (ta med frontkarnappet X ±12, Y −21…−14),
    (c) alle spawnpunkter og flagg ligger inne i `PlayArea`; spawnpunkter minst 1,5 m utenfor alle
    fotavtrykk, flaggene inne i sitt eget bygg. Skriver én linje per sjekk og avslutter med kode 1 ved feil.

### Del C – Byggeren (`GSCampusBuilder.cpp/.h`) og sola

13. **`BuildBuilding`:** der det står `if (B.bEnterable)` foran innlasting av `SM_Bygg_<navn>`: bruk
    `if (B.bEnterable || B.bBackdrop)`. I klossgrenen (`if (!B.bEnterable)`): lag kjegletaket bare når
    `!B.bBackdrop`. Gi glasskomponentene (begge steder de lages, `G`) `G->ComponentTags.Add(TEXT("Mat_Glass"));`
    (første del av Kamps bestilling om materialmerking).
14. **Plasser.** Øverst i den anonyme namespacen:
    ```cpp
    struct FPlaza { float X0, Y0, X1, Y1; };
    const FPlaza Plazas[] = {
        { -16.f, -34.f, 16.f, -20.5f },   // forecourt in front of the main entrance
        { -41.f,  14.f, 41.f,  58.f },    // courtyard between the wings, around the tower
    };
    bool InPlaza(const FVector2D& P, float Margin)
    {
        for (const FPlaza& Z : Plazas)
            if (P.X > Z.X0 - Margin && P.X < Z.X1 + Margin && P.Y > Z.Y0 - Margin && P.Y < Z.Y1 + Margin) return true;
        return false;
    }
    ```
    Ny `void BuildPlazas();` (i headeren ved `BuildSidewalks`), kalt i `Build()` rett etter `BuildRoads()`:
    for hver plass `AddBoxMat(FVector(midtX, midtY, -0.02f) * CmPerM, FRotator::ZeroRotator,
    FVector(bredde, dybde, 0.1f), TEXT("W_Paving"), FLinearColor(0.42f, 0.41f, 0.39f), true);`
    (topp 3 cm over plenen, 1 cm under stiene, så ingenting flimrer). Dette lukker også gress-stripa
    mellom stien og trappeavsatsen ved hovedinngangen (F7).
15. **Kantstein.** Ny `void BuildPathEdges();`, kalt etter `BuildSidewalks()`. For hver vei med `bPath`:
    gå fra A til B i biter på 4 m. For hver side: punktet `K = Mid + Side * Sgn * (R.Width * 0.5f + 0.06f)`.
    Hopp over biten hvis `K` ligger på en annen vei eller sti (`DistToSegment(K, O.A, O.B) < O.Width * 0.5f + 0.1f`
    for en annen `O`), inne i en plass (`InPlaza(K, 0.f)`) eller ved et bygg (`Footprints[i].ExpandBy(0.2f).IsInside(K)`).
    Ellers: `AddBoxMat(FVector(K.X, K.Y, 0.f) * CmPerM, Rot, FVector(L, 0.12f, 0.14f), TEXT("W_Concrete"),
    FLinearColor(0.55f, 0.55f, 0.53f), false);` (topp 7 cm, altså 3 cm over stien; uten kollisjon så ingen
    hekter seg). Ca. 770 biter.
16. **Trær (`BuildTreesAndProps`, den første løkka med 220 trær).** Bytt den tilfeldige spredningen med:
    - Utvid `IsFree` slik at den også sier nei når punktet er i en plass (`InPlaza(P, Margin * 0.5f)`),
      nærmere enn 3,5 m fra et punkt i `GSCampus::SpawnPoints()`, eller nærmere enn 4 m fra et tre som
      allerede står (hold en `TArray<FVector2D> Placed`).
    - Lag en liten lambda `PlantTree(Kind, P)` av koden som finnes (modell, tilfeldig vinkel og skala
      0,8–1,3, stammekollisjon, legg i `Placed`). Stammekollisjonens radius: lønn 0,35, bjørk 0,2, gran
      0,3, liten gran 0,14 (× skala), ikke 0,35 for alle (del av F14).
    - **Allé:** lønn på begge sider av oppkjørselen: `X = ±6.5`, `Y = -98 + 9.5 * i` for i = 0…6 (14 trær).
    - **Gatetrær:** lønn langs Høgskoleringen: `Y = -100`, `X = -156, -143, … 156` (steg 13), hopp over `|X| < 12` (24 trær).
    - **Resten:** inntil 80 trær (maks 1500 forsøk) tilfeldig i X −230…230, Y −125…240 med `IsFree(P, 6.f)`,
      og ikke i selve aksen (`FMath::Abs(P.X) < 10.f && P.Y < -30.f` hoppes over). Samme blanding av
      sorter og samme busk-regel som før. Reservegrenen (sylinder + kule når modellene mangler) beholdes
      for de tilfeldige trærne.
    - Steiner: 70 → 10, i samme område. Blomsterbed: ingen tilfeldige; fire faste ved forplassen:
      (±22, −38) og (±32, −38), hvis `IsFree`.
17. **Hekker (F6, samme funksjon, hekk-løkka langs ±X-veggene).** Regn ut hvor døra står og hopp over
    hekken der:
    ```cpp
    // door position on the +-X walls (same formula as BuildWallSide); the Hovedbygningen model has its end doors at y = 0
    const float L = B.SizeY - 2.f * WallT;
    const int32 NW = FMath::FloorToInt((L - MinPier) / (WindowW + MinPier));
    const float Pier = NW >= 1 ? (L - NW * WindowW) / (NW + 1) : 0.f;
    const bool bMainModel = MainBuildingMesh() && FString(B.Name) == TEXT("Hovedbygningen");
    const float DoorY = bMainModel ? 0.f : -L * 0.5f + Pier + WindowW * 0.5f + (NW / 2) * (Pier + WindowW);
    ...
    if (NW >= 1 && FMath::Abs(Y - DoorY) < WindowW * 0.5f + 2.7f) continue;   // hedge piece is 4.2 m long
    if (InPlaza(P, 0.5f)) continue;
    ```
    (For vegger på 60 m eller mer har `BuildWallSide` to dører; ingen ±X-vegg i den nye listen er så lang.)
    Busk-løkka rett under: legg til `!InPlaza(P, 0.5f)`.
18. **Parkerte biler:** bare Høgskoleringen er bilvei nå. Sett maks 22 biler (var 60).
19. **Plenmøbler (`BuildCampusProps`, siste løkke):** trekk punktet i X −150…150, Y −100…130, hopp over
    `InPlaza(P, 2.f)` og punkter nærmere enn 4 m fra en spawnplass, og sett maks 8 (var 45).
    Resten av funksjonen følger veilisten og bygglisten av seg selv (kulisser har `bEnterable = false`).
20. **F4, `GSEnvironment.cpp` linje 207:** `const float Azimuth = Hours / 24.f * 360.f + 90.f;`
    med kommentar: «X = east, Y = south: the light travels west in the morning, north at noon, east in the evening».
    Ikke gjør mer med lyset i denne runden (hele lysrunden er utredet i `retning.md`).

### Del D – Bygg, sjekk og logg

21. `python Tools/Blender/check_campus.py` skal gi bare OK. `python -m py_compile` på alle endrede `.py`.
    `check_walkable.py` (steg 5) skal gi bare OK.
22. Bygg: sjekk `Get-Process UnrealEditor` (skal ikke kjøre), så
    `Build.bat GloshaugenEditor Win64 Development -Project=... -WaitMutex`. Feiler bygget i en fil laget
    ikke eier: vent og prøv igjen (inntil 4 ganger), ikke rett den.
23. `README.md`: rett punktet «Resten av Gløshaugen: 17 bygg …» til det nye kartet (kjernen rundt
    Hovedbygningen for Capture the Flag: Gamle fysikk og Gamle kjemi som baser, Kjelhuset, fløyene;
    høyblokkene som kulisse), og legg Blender-kommandoen med `--` inn under «3D-modeller».
24. `IMPROVEMENTS.md`: ny seksjon øverst med linjene under.

**Hvis tiden ikke strekker til:** Del A (F1, F2) og Del B + steg 13, 17, 20 er kjernen og skal med.
Steg 15 (kantstein) og 16 (allé) kan utsettes til runde 2; da strykes linjene 342–343 og tas neste gang.

## Ferdig når
- `check_walkable.py`: alle dørstråler i Hovedbygningen og rampestrålene i Kjelhuset og Gamle fysikk er `OK`.
- `check_campus.py`: bare `OK`.
- `Build.bat` går gjennom uten feil (eller «Target is up to date» etter byggerens bygg).
- De to renderbildene ligger i `.claude/lag/grafikk/kontroll/` og viser åpen hoveddør og åpen rampetopp.
- `git status` viser ingen endring i `gs_materials.py`, `Config/` eller usporede `Content/`-filer fra laget.
- Det Andreas skal se når han trykker Play (etter at Unreal har importert modellene på nytt ved oppstart):
  han kan gå rett inn hoveddøra i Hovedbygningen; han kan gå opp rampen i Gamle fysikk og ut på gulvet i
  2. etasje; det står ingen hekk foran endedørene; bakgården bak Hovedbygningen og plassen foran inngangen
  er steinlagt; det går en allé opp til inngangen; langt i sør står høyblokkene som kulisse, og de fjerne
  byggene er borte; midt på dagen står sola i sør (bak Hovedbygningen sett fra inngangen).

## Risiko
- **Spawn og loot (Spillbarhet):** `SpawnPoints()` får nye tall i en fil som ikke står i eiertabellen.
  Det er avtalt som «data de kan bruke» og står i bestillingen. Utendørs loot trekkes fortsatt fra det
  gamle, store området til Spillbarhet har levert: ca. 85 % av den havner utenfor spillområdet (på gress).
  Ingenting krasjer av det. Kanten stenges ikke før spawn og loot er flyttet.
- **Online:** alle maskiner bygger campus selv fra samme data og samme tilfeldig-frø, så endringene er
  like hos alle. Men modellene ligger ikke i git (F11): en klient med gamle `SM_Bygg_*` får annen
  kollisjon ved rampetoppen enn verten til den har åpnet editoren og importert på nytt.
- **Kulisser:** har `bEnterable = false`, så `GSGameMode` hopper over loot og terminaler der, men modellene
  har fortsatt åpne dører og innside. Til kanten er stengt kan man gå inn i tomme bygg.
- **Aggregat-initialisering:** `bBackdrop = false` som standardverdi i structen krever C++14 eller nyere
  (UE 5.8 bruker C++20). `GSGameMode.cpp` og `GSHUD.cpp` leser bare feltene som fantes før.
- **Ytelse:** blir bedre (11 bygg i stedet for 17, langt færre møbler, 118 trær i stedet for 220).
  Kantsteinen er ca. 770 instanser i én gruppe.
- **Blender 5.3:** skriptene er skrevet for en eldre `bpy`. `details.py` kjørte feilfritt i planleggingen;
  feiler `buildings.py` eller `building_hovedbygningen.py` på et API som er endret, rett det i skriptet
  (laget eier dem) og skriv det i lagets `laerdom.md`.
- **F1 kan ha flere årsaker:** kontrolløren fant sperre også i tårndøra. `check_walkable.py` er fasiten,
  ikke linje 152 alene.
- **Kartet i PC-terminalen (`GSHUD`, Kamp)** har faste grenser og viser det nye kartet lite til Kamp har
  tatt bestillingen.

## IMPROVEMENTS-linjer
Seksjon øverst: `## Runde 1 (Grafikk og kart): CTF-kjernen rundt Hovedbygningen (26. nov.)`

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
