"""Helgasetr (NTNU, Elgeseter gate 10) - the one data source for the CTF map (plain Python, no bpy).

    make_plan()    -> the hand-written data (rooms, doors, openings, boxes, stairs, bases, spawns, places ...)
    derive(plan)   -> everything computed from it: walls with door holes, floor slabs, rails, glass panels,
                      steps, roof, ids. gen_helgasetr.py writes it to C++, check_helgasetr.py checks it.

Meters, game axes: X = east, Y = south, Z = up. Origin = axis 5 x axis H on the Plan 1 floor (kote +17.000).
Axes and levels from the architect's drawings (see .claude/lag/grafikk/helgasetr.md, chapter 1-5).
Everything here is a starting point checked by check_helgasetr.py; numbers that differ from helgasetr.md are
marked "flyttet" with the reason.
"""
import copy
import math

# ---------------------------------------------------------------- axes and levels (measured)
AX = {"1": -25.75, "2": -15.40, "3": -13.54, "4": -6.77, "5": 0.0, "6": 6.77, "7": 13.54, "8": 15.40, "9": 19.20, "10": 25.75}
AY = {"N": -32.11, "M": -26.45, "L": -20.30, "K": -18.03, "J": -12.02, "I": -6.01, "H": 0.0,
      "G": 6.01, "F": 12.02, "E": 18.03, "D": 20.30, "C": 23.85, "B": 29.80, "A": 35.66}
LEVELS = [0.0, 5.6, 9.9, 13.9, 18.2, 22.2]        # P1 ... P6
LEVEL_NAMES = ["P1", "P2", "P3", "P4", "P5", "P6"]
P1, P2, P3, P4, P5, P6 = range(6)
ROOF_Z, RIDGE_Z, SLAB, WALL_OUT, WALL_IN, CORE_WALL = 26.2, 29.0, 0.35, 0.40, 0.15, 0.25
LIGHT_WALL = 0.12
RING_IN = (13.25, 17.75)     # atrium edge |X|, |Y| (square corners in the blockout; rounded in 4b)
RING_OUT = (15.55, 20.05)    # corridor wall -> free width 2.3 m
BOX_H, BOX_ROOF = 3.0, 0.45  # hanging boxes: room height and the thick white underside
DOOR_H, RAIL_H, GLASS_T = 2.1, 1.1, 0.06
BREACH_H = 2.8
BX0, BX1 = -25.75, 25.75     # outer walls
BY0, BY1 = -32.11, 35.66
FLAG_Z_BRIDGE = 13.9         # GSCampus::Bases() Flag.Z = the Plan 4 floor. (Was 13.7 while GameMode traced from +4 m;
                             # since Spillbarhet's bfb38ef it traces with an object trace from +2 m: 15.9 < 17.85, K1 delivered.)

# material names (MI_GS_<name>, made by Content/Python/gs_materials.py)
MATS = ["PlasterWhite_In", "ConcreteSmooth_In", "WoodLight_In", "Floor_In", "Terrazzo_In", "HelgOrange_In",
        "HelgYellow_In", "Wood_In", "MetalDark_In", "Glass", "W_Asphalt", "LightPanel_In", "PaintRed_In",
        "PaintBlue_In", "PaintGreen_In", "Gunmetal_In", "Fabric_In", "Concrete_In", "PaintWhite_In"]
# collision kinds (C++ GSHelgasetr::ECollM)
C_SOLID, C_HIGH, C_NONE, C_BLOCKER = 0, 1, 2, 3
# bits (K13: the 4b model replaces the blocks bit by bit)
BITS = ["P1", "P2", "P3", "P4", "P5", "P6", "Trapper", "Tak", "Detaljer"]
B_STAIRS, B_ROOF, B_DETAIL = 6, 7, 8
DOOR_KINDS = ["tre", "glass_kode", "dobbel", "brann", "laast"]
ROOF_AREA = (-15.4, -20.3, 15.4, 20.3)   # the saw-tooth glass roof over the atrium (helgasetr.md kap. 7)
ROOF_TEETH, ROOF_STEEP = 8, 1.0          # teeth from north to south; horizontal run of the steep (glass) face


def roof_teeth():
    """(y_valley_north, y_ridge, y_valley_south) per tooth: opaque shallow face rising south to the ridge, steep glass
    face facing south down to the next valley (the low sun comes in, the truss throws its shadows)."""
    ar = ROOF_AREA
    pitch = (ar[3] - ar[1]) / ROOF_TEETH
    out = []
    for t in range(ROOF_TEETH):
        ya = ar[1] + t * pitch
        yb = ya + pitch
        out.append((ya, yb - ROOF_STEEP, yb))
    return out
PANEL_TYPES = ["rekkverk", "vegg", "dor"]


class Room:
    """A room on one level. kind: rom / lukket / korridor / lobby / trapp / heis / boks.
    doors: (x, y, width, kind); openings: (x0, y0, x1, y1) full-height gaps in the room's own walls.
    glass: sides 'NSEW' whose wall is fixed glass. allglass: every wall is breakable glass (P2 group rooms).
    nofloor: the room has no floor on this level (the climbing hall on P2)."""

    def __init__(self, lv, name, x0, x1, y0, y1, kind, doors=(), openings=(), glass="", allglass=False, nofloor=False, light=""):
        self.lv, self.name, self.kind = lv, name, kind
        self.x0, self.x1, self.y0, self.y1 = min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1)
        self.doors = list(doors)
        self.openings = list(openings)
        self.glass = glass
        self.allglass = allglass
        self.nofloor = nofloor
        self.light = light      # box sides that are lettvegg even towards the atrium

    def contains(self, x, y):
        return self.x0 < x < self.x1 and self.y0 < y < self.y1

    def rect(self):
        return (self.x0, self.y0, self.x1, self.y1)


# ================================================================ the hand-written data
def make_plan():
    R = []

    def room(lv, *a, **k):
        r = Room(lv, *a, **k)
        R.append(r)
        return r

    ri, ro = RING_IN, RING_OUT
    full = (BX0, BY0, BX1, BY1)

    # ---------------------------------------------------------- per level: footprint cuts, voids, extra floors
    cuts = {lv: [] for lv in range(6)}
    voids = {lv: [] for lv in range(6)}
    extras = {lv: [] for lv in range(6)}
    nw_cut = [(BX0, BY0, -15.55, -12.02), (BX0, BY0, -13.54, -20.05)]       # NV corner from P3 up (over the climbing hall)
    south_terrace = (-6.77, 29.8, 13.54, BY1)
    cuts[P1] = [(BX0, BY0, -15.55, 0.0)]                                      # forplass with the main entrance (outside)
    cuts[P2] = [(BX0, BY0, -15.55, -20.05)]
    cuts[P3] = nw_cut + [(BX0, 6.0, -15.55, 20.05), south_terrace]            # west terrace on P3
    for lv in (P4, P5, P6):
        cuts[lv] = nw_cut + [south_terrace]
    atrium = (-ri[0], -ri[1], ri[0], ri[1])
    # stair wells: only the entry strip is floor above P1 (the flights run over the rest)
    sor_well = (-13.54, 28.5, -6.77, 35.66)
    ost_well = (20.1, -12.5, 25.75, -7.5)
    for lv in (P2, P3, P4):
        voids[lv] += [sor_well, ost_well]
    # P2: the hole down to the canteen, the stair hole for run 1, the climbing hall (open P1-P2)
    voids[P2] += [(-9.9, -20.05, 9.0, 5.6), (-13.25, -6.5, -9.9, 5.6), (-15.55, BY0, 6.77, -20.05)]
    extras[P2] += [(-5.8, 1.0, -1.0, 5.6)]                                    # Ryddekroken gallery on top of the tower
    voids[P3] += [atrium, (-6.77, 20.05, 13.54, 29.8)]                        # atrium + open down over the student desks
    for lv in (P4, P5, P6):
        voids[lv] += [atrium]

    # ---------------------------------------------------------- the hanging boxes, shared areas and landings
    def box(lv, name, x0, x1, y0, y1, door, light=""):
        room(lv, name, x0, x1, y0, y1, "boks", doors=[door] if door else [], light=light)
        extras[lv].append((min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1)))

    def shared(lv, x0, x1, y0, y1):
        extras[lv].append((x0, y0, x1, y1))

    # P3 (helgasetr.md kap. 4, gaps under 0.3 m closed, edges snapped to the atrium edge)
    shared(P3, -6.65, 0.0, -17.75, -12.3)
    box(P3, "Grupperom 3.N1", 0.0, 6.7, -17.75, -12.3, (3.35, -17.75, 0.9, "glass_kode"))
    box(P3, "Grupperom 3.N2", 6.7, 13.25, -17.75, -12.3, (10.0, -17.75, 0.9, "glass_kode"))
    for i, (a, b) in enumerate(((-6.0, -3.025), (-3.025, -0.05), (-0.05, 2.925), (2.925, 5.9))):
        box(P3, "Grupperom 3.V%d" % (i + 1), -13.25, -8.3, a, b, (-13.25, (a + b) / 2, 0.9, "glass_kode"))
    shared(P3, -11.0, -6.65, 12.3, 17.75)
    box(P3, "Grupperom 3.S1", -6.65, 0.0, 12.3, 17.75, (-3.3, 17.75, 0.9, "glass_kode"))
    box(P3, "Grupperom 3.S2", 3.3, 6.7, 12.3, 17.75, (5.0, 17.75, 0.9, "glass_kode"))
    box(P3, "Grupperom 3.S3", 6.7, 13.25, 12.3, 17.75, (10.0, 17.75, 0.9, "glass_kode"))
    shared(P3, 9.0, 13.25, -6.0, 6.0)          # east shared area
    shared(P3, 7.5, 13.25, 6.0, 9.0)           # landing between run 2 and run 3
    # P4
    # west side whiteboard wall (lettvegg), not glass: breaks the door-to-door line between the bases (e2)
    box(P4, "Grupperom 4.N1", -6.65, 0.0, -17.75, -12.3, (-3.3, -17.75, 0.9, "glass_kode"), light="W")
    box(P4, "Grupperom 4.N2", 0.0, 6.7, -17.75, -12.3, (3.35, -17.75, 0.9, "glass_kode"))
    shared(P4, 6.7, 13.25, -17.75, -12.3)      # shared area with the environment station
    vy = [-12.0, -9.025, -6.05, -3.075, -0.1, 4.95, 10.0]
    # doors in pairs next to the shared wall (flyttet fra midten: gives long whiteboard walls = cover along R1, check e3)
    vdoor = [-9.5, -8.55, -3.55, -2.6, 4.0, 9.4]
    for i in range(6):
        box(P4, "Grupperom 4.V%d" % (i + 1), -13.25, -8.3, vy[i], vy[i + 1], (-13.25, vdoor[i], 0.9, "glass_kode"))
    shared(P4, -6.65, 0.0, 12.3, 17.75)
    shared(P4, 6.7, 13.25, 12.3, 17.75)
    box(P4, "Grupperom 4.O1", 9.0, 13.25, 6.2, 9.25, (13.25, 8.75, 0.9, "glass_kode"))
    box(P4, "Grupperom 4.O2", 9.0, 13.25, 9.25, 12.3, (13.25, 9.75, 0.9, "glass_kode"))
    shared(P4, 9.8, 13.25, -4.3, -1.8)          # top landing of run 3

    # ---------------------------------------------------------- stair cores on P1-P4 (same on every level)
    for lv in (P1, P2, P3, P4):
        room(lv, "Heis øst", 15.55, 18.5, -12.5, -9.9, "heis")
        room(lv, "Trapp øst-lobby", 15.55, 18.5, -9.9, -7.5, "lobby", openings=[(15.55, -9.9, 15.55, -7.5)])
        room(lv, "Trapp øst", 18.5, 25.75, -12.5, -7.5, "trapp", doors=[(18.5, -8.7, 1.2, "brann")])
        room(lv, "Heis sør", -10.2, -6.77, 20.05, 23.8, "heis")
        room(lv, "WC sør", -10.2, -6.77, 23.8, 26.9, "lukket")
        # K9: the alarm door to Trapp sør is locked on P4, so blue never comes up in red's lobby
        room(lv, "Trapp sør", -13.54, -6.77, 26.9, BY1, "trapp", doors=[(-12.0, 26.9, 1.2, "laast" if lv == P4 else "dobbel")])
    for lv in (P2, P3, P4):
        sl = room(lv, "Sør-lobby", -13.54, -10.2, 20.05, 26.9, "lobby", openings=[(-13.4, 20.05, -10.2, 20.05)])
    # P1 lobby is wider: the corridor past the kitchen comes in from the north-west
    room(P1, "Sør-lobby", -15.55, -10.2, 20.05, 26.9, "lobby", openings=[(-15.55, 20.05, -12.9, 20.05)])

    # ---------------------------------------------------------- P1 (Z 0): the canteen ("studenttorget")
    room(P1, "Klatrehallen", -15.55, 6.77, BY0, -20.05, "lukket", glass="S")
    room(P1, "Servicesenteret, kontor", 6.77, 15.55, BY0, -20.05, "lukket")
    room(P1, "Sit trening nord", 15.55, BX1, BY0, -12.5, "lukket", glass="W")
    room(P1, "Sit trening", 15.55, BX1, -7.5, 20.05, "lukket", glass="W")
    room(P1, "Kjøkkenet", -12.9, 15.55, 5.6, 20.05, "lukket")
    room(P1, "Studentlokalet", BX0, -15.55, 0.0, 20.05, "lukket", glass="E")
    room(P1, "Sørvest", BX0, -15.55, 20.05, BY1, "lukket")
    room(P1, "Sørvest 2", -15.55, -13.54, 26.9, BY1, "lukket")
    room(P1, "Sør", -6.77, BX1, 20.05, BY1, "lukket")

    # ---------------------------------------------------------- P2 (Z 5.6): the mezzanine
    room(P2, "Klatrehallen", -15.55, 6.77, BY0, -20.05, "lukket", glass="S", nofloor=True)
    room(P2, "Nordøst", 6.77, 15.55, BY0, -20.05, "lukket")
    room(P2, "Undervisningsrom 120", BX0, -15.55, -20.05, -1.8, "lukket")
    room(P2, "Vest", BX0, -15.55, -1.8, 20.05, "lukket")
    room(P2, "Kondisjonssal", 15.55, BX1, BY0, -12.5, "lukket", glass="W")
    room(P2, "Fysioterapi", 15.55, BX1, -7.5, 20.05, "lukket")
    room(P2, "Møterom", 9.0, 13.25, -12.0, -6.5, "boks", doors=[(13.25, -9.25, 0.9, "glass_kode")])
    room(P2, "Studentforeningen", 9.0, 13.25, -6.5, -1.0, "boks", doors=[(13.25, -3.75, 0.9, "glass_kode")])
    room(P2, "Grupperom 2.1", -8.8, -4.3, 11.9, 17.6, "boks", allglass=True, doors=[(-6.55, 11.9, 0.9, "glass_kode")])
    room(P2, "Grupperom 2.2", -4.3, 0.3, 11.9, 17.6, "boks", allglass=True, doors=[(-2.0, 11.9, 0.9, "glass_kode")])
    room(P2, "Grupperom 2.3", 8.8, 12.7, 11.9, 17.6, "boks", allglass=True, doors=[(10.75, 11.9, 0.9, "glass_kode")])
    room(P2, "Studentarbeidsplassene", -6.77, 13.54, 20.05, 29.8, "rom",
         openings=[(-4.5, 20.05, -0.5, 20.05), (5.5, 20.05, 9.5, 20.05)])
    room(P2, "Sør", -6.77, 13.54, 29.8, BY1, "lukket")
    room(P2, "Sørøst", 13.54, BX1, 20.05, BY1, "lukket")
    room(P2, "Sørvest", BX0, -15.55, 20.05, BY1, "lukket")
    room(P2, "Sørvest 2", -15.55, -13.54, 20.05, BY1, "lukket")

    # ---------------------------------------------------------- P3 (Z 9.9)
    room(P3, "Undervisningsrom 3.050", BX0, -15.55, -12.02, -1.0, "lukket")
    room(P3, "Grupperom 3.030", BX0, -15.55, -1.0, 6.0, "lukket")
    room(P3, "Sørvest", BX0, -15.55, 20.05, BY1, "lukket")
    room(P3, "Sørvest 2", -15.55, -13.54, 20.05, BY1, "lukket")
    room(P3, "Undervisningsrom 3.100 vest", -13.54, 0.0, BY0, -20.05, "lukket")
    room(P3, "Undervisningsrom 3.100 øst", 0.0, 6.77, BY0, -20.05, "lukket")
    room(P3, "Grupperom 3.022", 6.77, 15.55, BY0, -20.05, "lukket")
    room(P3, "Nordøst", 15.55, BX1, BY0, -12.5, "lukket")
    room(P3, "Ergoterapi", 15.55, BX1, -7.5, 6.0, "lukket")
    room(P3, "Barnevern", 15.55, BX1, 6.0, 20.05, "lukket")
    room(P3, "Sørøst", 13.54, BX1, 20.05, BY1, "lukket")

    # ---------------------------------------------------------- P4 (Z 13.9): the base floor (plan-04 table 1a)
    lv = P4
    room(lv, "Sørbasen", -6.77, 13.54, 20.05, 29.8, "rom", doors=[(10.0, 20.05, 1.0, "tre"), (-3.0, 20.05, 1.0, "tre")])
    room(lv, "Nordbasen", -13.54, 6.77, BY0, -20.05, "rom", doors=[(-10.0, -20.05, 1.0, "tre"), (3.0, -20.05, 1.0, "tre")])
    room(lv, "Sør-spawngang", -15.4, -13.54, 20.05, 32.11, "korridor", doors=[(-13.54, 25.9, 1.2, "tre")])
    room(lv, "S1 grupperom 4.041", BX0, -15.4, 20.3, 26.0, "rom", doors=[(-15.4, 23.0, 1.0, "tre"), (-23.6, 20.3, 1.0, "tre")])
    room(lv, "S2 grupperom 4.037", BX0, -15.4, 26.0, 32.11, "rom", doors=[(-15.4, 29.0, 1.0, "tre")])
    room(lv, "S3 garderobe", BX0, -13.54, 32.11, BY1, "lukket")
    room(lv, "Venterom", BX0, -15.4, 14.0, 20.3, "korridor", openings=[(-15.4, 14.0, -15.4, 20.05)])
    room(lv, "Audiologi 4.056", BX0, -15.55, 4.0, 14.0, "rom", doors=[(-20.5, 14.0, 1.0, "tre"), (-15.55, 9.0, 1.0, "tre"), (-20.5, 4.0, 1.0, "tre")])
    room(lv, "Psykoakustisk lab", BX0, -15.55, -3.0, 4.0, "rom", doors=[(-20.5, -3.0, 1.0, "tre")])
    room(lv, "Audiologi 4.057", BX0, -15.55, -12.02, -3.0, "rom", doors=[(-15.55, -8.0, 1.0, "tre")])
    room(lv, "Nord-lobby", 10.2, 13.54, -26.9, -20.05, "lobby", openings=[(10.2, -20.05, 13.4, -20.05)])
    room(lv, "Grupperom 4.022", 6.77, 10.2, BY0, -20.05, "lukket")
    room(lv, "Nordrom bak lobbyen", 10.2, 13.54, BY0, -26.9, "lukket")
    room(lv, "Nord-spawngang", 13.54, 15.4, BY0, -20.05, "korridor", doors=[(13.54, -25.9, 1.2, "tre")])
    room(lv, "N1 grupperom 4.035", 15.4, BX1, -26.0, -20.3, "rom", doors=[(15.4, -23.0, 1.0, "tre"), (23.6, -20.3, 1.0, "tre")])
    room(lv, "N2 grupperom 4.021", 15.4, BX1, BY0, -26.0, "rom", doors=[(15.4, -29.0, 1.0, "tre")])
    room(lv, "NØ-gang", 15.4, BX1, -20.3, -12.5, "korridor", openings=[(15.4, -20.05, 15.4, -14.0)])
    room(lv, "Garderober", 15.55, BX1, -7.5, -3.0, "lukket")
    room(lv, "Treningsleiligheten", 15.55, BX1, -3.0, 4.0, "rom", doors=[(15.55, 0.0, 1.0, "tre"), (20.5, 4.0, 1.0, "tre")])
    room(lv, "Ferdighetsrom vernepleier", 15.55, BX1, 4.0, 12.0, "rom", doors=[(15.55, 8.0, 1.0, "tre"), (20.5, 12.0, 1.0, "tre")])
    room(lv, "Ferdighetsrom radiografi", 15.55, BX1, 12.0, 20.05, "rom", doors=[(15.55, 16.0, 1.0, "tre")])
    room(lv, "Undervisningsrom SØ", 13.54, BX1, 20.05, BY1, "lukket")

    # ---------------------------------------------------------- P5 / P6: scenery (only floor, rails, closed wings)
    for lv in (P5, P6):
        room(lv, "Nordfløy", -13.54, 15.55, BY0, -20.05, "lukket")
        room(lv, "Østfløy", 15.55, BX1, BY0, BY1, "lukket")
        room(lv, "Vestfløy", BX0, -15.55, -12.02, BY1, "lukket")
        room(lv, "Sørvest", -15.55, -6.77, 20.05, BY1, "lukket")
        room(lv, "Sør", -6.77, 13.54, 20.05, 29.8, "lukket")
        room(lv, "Sørøst", 13.54, 15.55, 20.05, BY1, "lukket")

    # ---------------------------------------------------------- stairs (plan-04 1c)
    # flight: axis 'x'/'y', dir +1/-1, start coordinate, cross range, z0 -> z1, risers, landing length after `mid` risers
    STAIRS = [
        dict(name="Løp 1", axis="y", dir=1, s0=-6.5, s1=5.6, c0=-13.25, c1=-9.9, z0=0.0, z1=5.6, n=31, landing=2.5, atrium=True),
        dict(name="Sosialtrappa", axis="y", dir=1, s0=-5.0, s1=5.6, c0=-7.2, c1=-6.0, z0=0.0, z1=5.6, n=31, landing=0.0, atrium=True, solid=True),
        dict(name="Sittetrinn", axis="y", dir=1, s0=-5.0, s1=5.6, c0=-9.6, c1=-7.2, z0=0.0, z1=5.6, n=12, landing=0.0, atrium=True, solid=True, guard=False),
        dict(name="Løp 2", axis="x", dir=1, s0=-1.0, s1=7.5, c0=6.6, c1=9.0, z0=5.6, z1=9.9, n=24, landing=1.2, atrium=True),
        dict(name="Løp 3", axis="y", dir=-1, s0=6.0, s1=-1.8, c0=10.0, c1=13.25, z0=9.9, z1=13.9, n=22, landing=1.2, atrium=True),
    ]
    # stair wells: two flights per storey with a landing at the far end and a wall between them
    for lv in (P1, P2, P3):
        za, zb = LEVELS[lv], LEVELS[lv + 1]
        zm = (za + zb) / 2
        STAIRS.append(dict(name="Trapp sør %d-1" % (lv + 1), axis="y", dir=1, s0=28.5, s1=34.06, c0=-10.0, c1=-6.77, z0=za, z1=zm, n=math.ceil((zm - za) / 0.18), landing=0.0, well=True, endland=True))
        STAIRS.append(dict(name="Trapp sør %d-2" % (lv + 1), axis="y", dir=-1, s0=34.06, s1=28.5, c0=-13.54, c1=-10.3, z0=zm, z1=zb, n=math.ceil((zb - zm) / 0.18), landing=0.0, well=True))
        STAIRS.append(dict(name="Trapp øst %d-1" % (lv + 1), axis="x", dir=1, s0=20.1, s1=24.15, c0=-12.5, c1=-10.15, z0=za, z1=zm, n=math.ceil((zm - za) / 0.18), landing=0.0, well=True, endland=True))
        STAIRS.append(dict(name="Trapp øst %d-2" % (lv + 1), axis="x", dir=-1, s0=24.15, s1=20.1, c0=-9.85, c1=-7.5, z0=zm, z1=zb, n=math.ceil((zb - zm) / 0.18), landing=0.0, well=True))
    WELL_LANDINGS = []
    WELL_DIVIDERS = []
    for lv in (P1, P2, P3):
        za, zb = LEVELS[lv], LEVELS[lv + 1]
        zm = (za + zb) / 2
        WELL_LANDINGS += [(-13.54, 34.06, -6.77, BY1, zm), (24.15, -12.5, 25.75, -7.5, zm)]
        WELL_DIVIDERS += [(-10.3, 28.5, -10.0, 34.06, za - SLAB, zb + 1.0), (20.1, -10.15, 24.15, -9.85, za - SLAB, zb + 1.0)]

    # ---------------------------------------------------------- bases and spawns (plan-04 1d; S1 rows flyttet 0.6/0.3 m south: the capsule must clear the L-screen)
    SPAWN_S = [(-23.0, 22.8), (-18.5, 22.8), (-23.0, 24.9), (-18.5, 24.9), (-23.0, 27.6), (-18.5, 27.6), (-23.0, 30.6), (-18.5, 30.6)]
    SPAWN_N = [(-x, -y) for x, y in SPAWN_S]
    BASES = [dict(name="Sor", room="Sørbasen", flag=(3.4, 26.0, 13.9), yaw=-90.0, spawns=SPAWN_S, spawn_rooms=("S1 grupperom 4.041", "S2 grupperom 4.037"),
                  spawn_exits=[(-13.54, 25.9), (-23.6, 20.3)]),
             dict(name="Nord", room="Nordbasen", flag=(-3.4, -26.0, 13.9), yaw=90.0, spawns=SPAWN_N, spawn_rooms=("N1 grupperom 4.035", "N2 grupperom 4.021"),
                  spawn_exits=[(13.54, -25.9), (23.6, -20.3)])]

    # ---------------------------------------------------------- screens (L-skjerm 1.6 x 2.1, 1.5 m inside the door; K6, K8)
    SCREENS = []
    for x in (10.0, -3.0):
        SCREENS.append((x, 20.05 + 1.5, 1.6, "H"))        # Sørbasen
        SCREENS.append((-x, -20.05 - 1.5, 1.6, "H"))      # Nordbasen
    # S1 -> venterom / N1 -> NØ-gang: 2.4 m long (flyttet/forlenget: with 1.6 m the venterom saw the spawn diagonally)
    # (flyttet: door 1 m towards the middle, and the screen is a real L with a leg, so the way out turns towards the outer wall)
    SCREENS.append((-23.1, 20.3 + 1.5, 2.2, "H"))
    SCREENS.append((-22.0, 20.3 + 0.875, 1.25, "V"))
    SCREENS.append((23.1, -20.3 - 1.5, 2.2, "H"))
    SCREENS.append((22.0, -20.3 - 0.875, 1.25, "V"))

    # ---------------------------------------------------------- breach walls (lettvegger), point-symmetric pairs (plan-04 1d)
    BREACH_WALLS = [
        (P4, -15.55, 0.6, -15.55, 3.0), (P4, 15.55, -3.0, 15.55, -0.6),
        (P4, -15.55, -11.4, -15.55, -9.0), (P4, 15.55, 9.0, 15.55, 11.4),
        (P3, -15.55, -10.0, -15.55, -7.6), (P3, 15.55, 7.6, 15.55, 10.0),
    ]

    # ---------------------------------------------------------- cover (own ISM with ids; K21). (lv, name, x0, y0, x1, y1, height, mat)
    COVER = [
        # P1 canteen: planters, salad bar, booths, service desk
        (P1, "Plantekasse 1", -4.4, -10.3, -3.1, -9.7, 0.9, "Gunmetal_In"),
        (P1, "Plantekasse 2", -2.8, -10.3, -1.5, -9.7, 0.9, "Gunmetal_In"),
        (P1, "Plantekasse 3", -1.2, -10.3, 0.1, -9.7, 0.9, "Gunmetal_In"),
        (P1, "Plantekasse 4", 0.4, -10.3, 1.7, -9.7, 0.9, "Gunmetal_In"),
        (P1, "Salatbaren", -4.6, -2.6, -1.2, -1.6, 1.0, "Wood_In"),
        (P1, "Bås 1", 11.0, -15.2, 12.6, -15.0, 1.5, "MetalDark_In"),
        (P1, "Bås 2", 11.0, -11.2, 12.6, -11.0, 1.5, "MetalDark_In"),
        (P1, "Bås 3", 11.0, -7.2, 12.6, -7.0, 1.5, "MetalDark_In"),
        (P1, "Servicesenteret, disk", 7.5, -18.5, 12.5, -17.8, 1.1, "Wood_In"),
        (P1, "Serveringsdisken", 1.0, 4.6, 12.0, 5.3, 1.1, "MetalDark_In"),
        # P2
        (P2, "Skap vest", -15.4, -14.0, -15.0, -10.0, 1.9, "PaintGreen_In"),
        (P2, "Skap øst", 15.0, 10.0, 15.4, 14.0, 1.9, "PaintGreen_In"),
        # P4 corridors along the atrium: sofa groups and cabinets on the atrium side, 0.6 m from the rail (helgasetr.md kap. 5)
        (P4, "Miljøstasjon", 12.4, -17.0, 13.0, -13.0, 2.0, "Gunmetal_In"),
        (P4, "Sofagruppe R3 nord", 13.85, -11.6, 14.45, -8.4, 1.3, "Fabric_In"),
        (P4, "Sofagruppe R3 midt", 13.85, -0.9, 14.45, 2.6, 1.3, "Fabric_In"),
        (P4, "Skaprekke R3 ved trappa", 13.85, -8.0, 14.45, -4.9, 1.9, "PaintGreen_In"),
        (P4, "Sofagruppe R3 sør", 13.85, 3.2, 14.45, 6.0, 1.3, "Fabric_In"),
        (P4, "Skap R1 nord", -14.45, -16.6, -13.85, -13.4, 1.9, "PaintGreen_In"),
        (P4, "Skap R1 sør", -14.45, 11.6, -13.85, 14.8, 1.9, "PaintGreen_In"),
        (P4, "Skap R3 sør", 13.85, 13.4, 14.45, 16.6, 1.9, "PaintGreen_In"),
        # P3 corridors
        (P3, "Sofagruppe 3 vest nord", -14.45, -15.6, -13.85, -12.4, 1.3, "Fabric_In"),
        (P3, "Sofagruppe 3 vest sør", -14.45, 9.4, -13.85, 12.6, 1.3, "Fabric_In"),
        (P3, "Sofagruppe 3 øst nord", 13.85, -12.6, 14.45, -9.4, 1.3, "Fabric_In"),
        (P3, "Sofagruppe 3 øst sør", 13.85, 12.4, 14.45, 15.6, 1.3, "Fabric_In"),
    ]

    # ---------------------------------------------------------- loot (Tier 0 common / 1 better / 2 crate), terminals, cameras
    LOOT = []

    def loot(lv, pts, tier=0):
        for p in pts:
            LOOT.append((p[0], p[1], LEVELS[lv], p[2] if len(p) > 2 else tier))

    loot(P1, [(-12.0, -15.0), (12.0, -13.0), (-6.0, -17.0), (6.0, -16.0), (-8.0, -8.0), (8.0, -3.0), (-3.0, -12.5), (3.0, -12.5),
              (-14.2, 10.0), (-14.2, 16.0), (0.0, -6.0, 2), (14.0, 2.0)])
    loot(P2, [(-12.0, -12.0), (-14.4, -3.0), (12.0, -15.0), (11.0, -9.25, 1), (-6.55, 14.8, 1), (10.75, 14.8, 1), (-10.0, 10.0), (4.0, 12.0),
              (0.0, 25.0), (8.0, 26.0)])
    loot(P3, [(-14.4, -5.0), (-14.4, 13.5), (14.4, 5.0), (14.4, -13.0), (-8.0, -18.9), (8.0, -18.9), (-8.0, 18.9), (8.0, 18.9),
              (-10.8, -4.5, 1), (3.35, -15.0, 1), (-3.3, 15.0, 1), (5.0, 15.0, 1), (11.0, -3.0, 2), (9.5, 3.0), (-3.3, -15.0), (-9.0, 15.0)])
    loot(P4, [(-14.4, -10.0), (-14.4, 8.0), (14.4, 9.0), (14.9, -3.3), (-20.5, 9.0, 1), (-20.5, 0.5, 2), (-20.5, -8.0, 1),
              (20.5, 0.5, 2), (20.5, 8.0, 1), (20.5, 16.0, 1), (-10.8, -10.5, 1), (-10.8, 2.5), (11.1, 7.7, 1), (11.1, 10.8),
              (-3.3, -15.0), (3.35, -15.0), (-3.3, 15.0), (10.0, 15.0), (9.5, -15.0), (-12.0, 18.9)])
    # (flyttet runde 5a: (11.0, 3.0) on Plan 3 lay inside the fill under run 3 (Spillbarhet runde-06 F2); now (9.5, 3.0), between
    # the atrium rail and the stair, same tier.)
    # Runde 5a: one spot inside every code-locked group room that had none (risk gives reward; Spillbarhet upgrades the tier).
    # APPENDED LAST so no existing loot index moves (Spillbarhet / Interaktiv key on them). Place: the room centre moved
    # 1.0 m away from the door along the door normal, >= 0.6 m from walls and cover; none within 8 m of a flag.
    # The 13 rooms that already had a spot inside (Møterom, 2.1, 2.3, 3.N1, 3.V1, 3.S1, 3.S2, 4.N1, 4.N2, 4.V1, 4.V5,
    # 4.O1, 4.O2) get no second one. LOOT_ROOMS: (loot index, level, room) so check l can test the room side of the door.
    LOOT_ROOMS = []

    def loot_room(lv, name, x, y, tier=0):
        LOOT_ROOMS.append((len(LOOT), lv, name))
        LOOT.append((x, y, LEVELS[lv], tier))

    loot_room(P2, "Studentforeningen", 10.1, -3.75)
    loot_room(P2, "Grupperom 2.2", -2.0, 15.75)
    loot_room(P3, "Grupperom 3.N2", 10.0, -14.0)
    loot_room(P3, "Grupperom 3.V2", -9.8, -1.55)
    loot_room(P3, "Grupperom 3.V3", -9.8, 1.45)
    loot_room(P3, "Grupperom 3.V4", -9.8, 4.4)
    loot_room(P3, "Grupperom 3.S3", 10.0, 14.0)
    loot_room(P4, "Grupperom 4.V2", -9.8, -7.55)
    loot_room(P4, "Grupperom 4.V3", -9.8, -4.55)
    loot_room(P4, "Grupperom 4.V4", -9.8, -1.6)
    loot_room(P4, "Grupperom 4.V6", -9.8, 7.45)
    TERMINALS = [  # (x, y, z, yaw, kind 0 PC / 1 printer, name)
        (-24.8, 9.0, LEVELS[P4], 0.0, 0, "Audiologi 4.056, 4. etasje"),
        (24.8, 8.0, LEVELS[P4], 180.0, 0, "Ferdighetsrom vernepleier, 4. etasje"),
        (12.6, -3.75, LEVELS[P2], 180.0, 0, "Studentforeningen, 2. etasje"),
        (-8.2, 16.8, LEVELS[P2], 0.0, 0, "Grupperom 2.1, 2. etasje"),
        (6.2, -16.9, LEVELS[P3], 180.0, 1, "Grupperom 3.N1, 3. etasje"),
        (-6.1, 16.9, LEVELS[P3], 0.0, 1, "Grupperom 3.S1, 3. etasje"),
    ]
    CAMERAS = [  # (x, y, z, yaw, name)
        (0.0, -19.6, 4.6, 90.0, "Kantina, klatreveggen"),
        (15.3, -6.9, 3.6, 180.0, "Trapp øst, 1. etasje"),
        (0.0, 19.6, 9.0, -90.0, "Mesaninen"),
        (-15.3, 0.0, 13.1, 0.0, "Vestkorridoren, 3. etasje"),
        (15.3, 0.0, 13.1, 180.0, "Østkorridoren, 3. etasje"),
        (-15.3, -16.0, 17.2, 45.0, "Vestkorridoren, 4. etasje"),
        (15.3, 16.0, 17.2, -135.0, "Østkorridoren, 4. etasje"),
        (12.9, 17.4, 12.5, -135.0, "Atriet, sørøst"),
    ]
    SOUND_ZONES = [  # (name, x0, y0, z0, x1, y1, z1, rt60 s)
        ("Atriet", -13.25, -17.75, 0.0, 13.25, 17.75, ROOF_Z, 2.8),
        ("Kantina", -15.55, -20.05, 0.0, 15.55, 5.6, 5.6, 2.5),
        ("Mesaninen", -15.55, 5.6, 5.6, 15.55, 20.05, 9.9, 2.0),
        ("Studentarbeidsplassene", -6.77, 20.05, 5.6, 13.54, 29.8, 13.9, 1.6),
        ("Ringkorridoren 3. etasje", -15.55, -20.05, 9.9, 15.55, 20.05, 13.9, 1.1),
        ("Ringkorridoren 4. etasje", -15.55, -20.05, 13.9, 15.55, 20.05, 18.2, 1.1),
        ("Trapp sør", -13.54, 26.9, 0.0, -6.77, BY1, 18.2, 2.0),
        ("Trapp øst", 18.5, -12.5, 0.0, 25.75, -7.5, 18.2, 2.0),
        ("Sørbasen", -6.77, 20.05, 13.9, 13.54, 29.8, 18.2, 0.7),
        ("Nordbasen", -13.54, BY0, 13.9, 6.77, -20.05, 18.2, 0.7),
        ("Grupperom sør", BX0, 20.3, 13.9, -15.4, 32.11, 18.2, 0.5),
        ("Grupperom nord", 15.4, BY0, 13.9, BX1, -20.3, 18.2, 0.5),
        ("Laberadene", BX0, -12.02, 13.9, -15.55, 20.3, 18.2, 0.5),
        ("Østfløyens rom", 15.55, -3.0, 13.9, BX1, 20.05, 18.2, 0.6),
        ("Klatrehallen", -15.55, BY0, 0.0, 6.77, -20.05, 9.9, 1.8),
    ]
    # ---------------------------------------------------------- lights (K20): 8 big hanging rings + corridor lights <= 6 m
    LIGHTS = []  # (x, y, z, kind 0 ring lamp / 1 corridor point light / 2 ceiling spot, radius m, casts shadow)
    for p in [(-5.0, -9.0, 11.8), (1.5, -10.0, 9.2), (6.5, -8.0, 12.0), (-6.0, -3.0, 8.4), (0.0, -4.0, 10.6), (6.0, -1.5, 8.0),
              (-3.0, 2.5, 12.4), (4.0, 3.0, 10.8)]:
        LIGHTS.append((p[0], p[1], p[2], 0, 1.2))
    base_x = {P4: [(-13.54, 6.77, -1), (-6.77, 13.54, 1)]}   # base wall spans on the N (-1) / S (+1) ring side

    def near_base(lv, x, y):
        # P3 too: a light 0.7 m under the Plan 4 floor and 1.2-1.7 m from the base wall lit the base from below
        # (runde-04a funn 5: distance in 3D, the lights have no shadow)
        if lv not in (P3, P4):
            return False
        for a, b, s in base_x[P4]:
            if s * y > 17.0 and a - 3.0 < x < b + 3.0:
                return True
        return False

    for lv, z in ((P3, LEVELS[P3] + 3.3), (P4, LEVELS[P4] + 3.3)):
        mid = 14.4
        pts = [(-mid, y) for y in (-16.0, -8.0, 0.0, 8.0, 16.0)] + [(mid, y) for y in (-16.0, -8.0, 0.0, 8.0, 16.0)]
        pts += [(x, -18.9) for x in (-8.0, 0.0, 8.0, 12.0)] + [(x, 18.9) for x in (-12.0, -8.0, 0.0, 8.0)]
        for x, y in pts:
            if not near_base(lv, x, y):
                LIGHTS.append((x, y, z, 1, 6.0))
    for x, y in ((-14.4, -12.0), (12.0, -14.0), (-6.0, 14.0), (6.0, 16.0), (0.0, 23.0), (6.0, 27.0)):
        LIGHTS.append((x, y, LEVELS[P2] + 3.6, 1, 6.0))
    for x, y in ((-12.0, -16.0), (12.0, -17.0), (-13.5, -2.0), (12.5, 0.0)):
        LIGHTS.append((x, y, 4.6, 1, 6.0))
    # Runde 5a: the 4 lights that cast shadows (budget kap. 9: at most 4). The two ring lamps nearest the middle of the
    # canteen (soft, directed shadows of tables and people), and one spot (kind 2) in the ceiling of each base lobby on
    # Plan 4 (the shadow of whoever comes round the corner). Tuple element 5 = casts shadow.
    canteen_mid = (0.0, -7.2)
    rings = sorted((k for k, L in enumerate(LIGHTS) if L[3] == 0),
                   key=lambda k: math.hypot(LIGHTS[k][0] - canteen_mid[0], LIGHTS[k][1] - canteen_mid[1]))
    LIGHTS[:] = [L + (k in rings[:2],) for k, L in enumerate(LIGHTS)]
    ceil4 = LEVELS[P5] - SLAB - 0.1
    LIGHTS.append((11.87, -23.5, ceil4, 2, 7.0, True))    # Nord-lobby (blue base)
    LIGHTS.append((-11.87, 23.5, ceil4, 2, 7.0, True))    # Sør-lobby (red base)

    # ---------------------------------------------------------- team colours: stripes over the base doors and lobby openings, signs
    STRIPES = []   # (x0, y0, x1, y1, z, team 0 red / 1 blue)
    SIGNS = []     # (x, y, z, yaw facing, text, team)
    z4 = LEVELS[P4]
    for x in (10.0, -3.0):
        STRIPES.append((x - 0.7, 20.05 - 0.12, x + 0.7, 20.05 - 0.08, z4 + 2.25, 0))
        STRIPES.append((-x - 0.7, -20.05 + 0.08, -x + 0.7, -20.05 + 0.12, z4 + 2.25, 1))
    STRIPES.append((-13.4, 20.05 - 0.12, -10.2, 20.05 - 0.08, z4 + 3.2, 0))
    STRIPES.append((10.2, -20.05 + 0.08, 13.4, -20.05 + 0.12, z4 + 3.2, 1))
    SIGNS.append((3.5, 20.05 - 0.09, z4 + 2.7, -90.0, "SØR", 0))
    SIGNS.append((-3.5, -20.05 + 0.09, z4 + 2.7, 90.0, "NORD", 1))

    ZONES = [  # GSCampus::Buildings(): five zones that do not overlap (helgasetr.md kap. 5)
        ("Helgasetr nordfløy", BX0, BY0, BX1, -20.05),
        ("Helgasetr sørfløy", BX0, 20.05, BX1, BY1),
        ("Helgasetr vestfløy", BX0, -20.05, -15.55, 20.05),
        ("Helgasetr østfløy", 15.55, -20.05, BX1, 20.05),
        ("Atriet", -15.55, -20.05, 15.55, 20.05),
    ]
    CORRIDORS = [((-14.4, -18.9), (-14.4, 18.9)), ((14.4, -18.9), (14.4, 18.9)), ((-14.4, -18.9), (14.4, -18.9)), ((-14.4, 18.9), (14.4, 18.9))]

    return dict(rooms=R, cuts=cuts, voids=voids, extras=extras, stairs=STAIRS, well_landings=WELL_LANDINGS,
                well_dividers=WELL_DIVIDERS, bases=BASES, screens=SCREENS, breach=BREACH_WALLS, cover=COVER, loot=LOOT, loot_rooms=LOOT_ROOMS,
                terminals=TERMINALS, cameras=CAMERAS, sound_zones=SOUND_ZONES, lights=LIGHTS, stripes=STRIPES, signs=SIGNS,
                zones=ZONES, corridors=CORRIDORS, play_area=(-26.5, -33.0, 26.5, 36.5), full=full)


# ================================================================ geometry helpers
EPS = 1e-4


def r3(v):
    return round(v, 4)


def in_rect(x, y, r):
    return r[0] < x < r[2] and r[1] < y < r[3]


def level_grid(plan, lv):
    """Cells between every coordinate used on this level. Returns xs, ys and a dict (i, j) -> (space, floor)."""
    rooms = [r for r in plan["rooms"] if r.lv == lv]
    xs, ys = {BX0, BX1}, {BY0, BY1}
    for r in plan["cuts"][lv] + plan["voids"][lv] + plan["extras"][lv]:
        xs.update((r[0], r[2]))
        ys.update((r[1], r[3]))
    for r in rooms:
        xs.update((r.x0, r.x1))
        ys.update((r.y0, r.y1))
    xs = sorted({r3(v) for v in xs if BX0 - EPS <= v <= BX1 + EPS})
    ys = sorted({r3(v) for v in ys if BY0 - EPS <= v <= BY1 + EPS})
    cells = {}
    for i in range(len(xs) - 1):
        cx = (xs[i] + xs[i + 1]) / 2
        for j in range(len(ys) - 1):
            cy = (ys[j] + ys[j + 1]) / 2
            if any(in_rect(cx, cy, c) for c in plan["cuts"][lv]):
                cells[i, j] = ("out", False)
                continue
            fl = not any(in_rect(cx, cy, v) for v in plan["voids"][lv]) or any(in_rect(cx, cy, e) for e in plan["extras"][lv])
            rm = next((k for k, r in enumerate(rooms) if r.contains(cx, cy)), None)
            if rm is not None:
                if rooms[rm].nofloor:
                    fl = False
                cells[i, j] = (("room", rm), fl)
            else:
                cells[i, j] = ("open" if fl else "void", fl)
    return xs, ys, cells, rooms


def rects_from_cells(xs, ys, keyfn):
    """Merges grid cells with the same non-None key into rectangles: runs along X per row, then equal runs down Y."""
    nx, ny = len(xs) - 1, len(ys) - 1
    runs = []
    for j in range(ny):
        i = 0
        while i < nx:
            k = keyfn(i, j)
            if k is None:
                i += 1
                continue
            a = i
            while i < nx and keyfn(i, j) == k:
                i += 1
            runs.append([a, i, j, j + 1, k])
    merged = []
    open_runs = {}
    for a, b, j0, j1, k in runs:
        key = (a, b, k)
        if key in open_runs and open_runs[key][3] == j0:
            open_runs[key][3] = j1
        else:
            if key in open_runs:
                merged.append(open_runs[key])
            open_runs[key] = [a, b, j0, j1, k]
    merged += list(open_runs.values())
    return [(xs[a], ys[j0], xs[b], ys[j1], k) for a, b, j0, j1, k in merged]


def wall_kind(a, b, fa, fb, rooms, orient, coord, side_a_low):
    """Edge type between two cells. side_a_low: cell a has the lower coordinate."""
    if a == b:
        if a[0] == "room" and fa != fb:
            return "rail"
        return None
    if a == "out" or b == "out":
        return "ext"
    ra = rooms[a[1]] if a[0] == "room" else None
    rb = rooms[b[1]] if b[0] == "room" else None
    if ra or rb:
        if ra and rb:
            if ra.allglass or rb.allglass:
                return "glass"
            if ra.kind == "boks" or rb.kind == "boks":
                return "light"
            if ra.kind in ("trapp", "heis") or rb.kind in ("trapp", "heis"):
                return "core"
            for r, low in ((ra, side_a_low), (rb, not side_a_low)):
                side = ("E" if low else "W") if orient == "V" else ("S" if low else "N")
                if side in r.glass:
                    return "fixglass"
            return "inner"
        r, other, low = (ra, b, side_a_low) if ra else (rb, a, not side_a_low)
        if r.allglass:
            return "glass"
        side = ("E" if low else "W") if orient == "V" else ("S" if low else "N")
        if r.kind == "boks":
            return "glass" if other == "void" and side not in r.light else "light"
        if side in r.glass:
            return "fixglass"
        if r.kind in ("trapp", "heis"):
            return "core"
        return "inner"
    return "rail"


THICK = dict(ext=WALL_OUT, core=CORE_WALL, inner=WALL_IN, light=LIGHT_WALL, fixglass=0.1, glass=GLASS_T, rail=GLASS_T)
OPAQUE = ("ext", "core", "inner", "light")


def level_top(lv):
    return (LEVELS[lv + 1] if lv + 1 < len(LEVELS) else ROOF_Z) - SLAB


def stair_steps(s):
    """One box per step and landing: (x0, y0, x1, y1, ztop, kind) with kind 'step' / 'landing'."""
    run = abs(s["s1"] - s["s0"])
    n = s["n"]
    tread = (run - s["landing"]) / n
    rise = (s["z1"] - s["z0"]) / n
    mid = n // 2 if s["landing"] > 0 else -1
    out = []
    pos = s["s0"]
    d = s["dir"]
    for i in range(1, n + 1):
        a, b = pos, pos + d * tread
        out.append((a, b, s["z0"] + i * rise, "step"))
        pos = b
        if i == mid:
            out.append((pos, pos + d * s["landing"], s["z0"] + i * rise, "landing"))
            pos += d * s["landing"]
    res = []
    for a, b, z, k in out:
        lo, hi = min(a, b), max(a, b)
        if s["axis"] == "y":
            res.append((s["c0"], lo, s["c1"], hi, z, k))
        else:
            res.append((lo, s["c0"], hi, s["c1"], z, k))
    return res, rise, tread


# ================================================================ derive everything
def derive(plan):
    G = dict(boxes=[], walls=[], panels=[], doors=[], breach=[], cover=[], floors={lv: [] for lv in range(6)},
             rails={lv: [] for lv in range(6)}, opaque={lv: [] for lv in range(6)}, glass2d={lv: [] for lv in range(6)},
             steps=[], errors=[], boundary={lv: [] for lv in range(6)}, arrivals={lv: [] for lv in range(6)})

    def add_box(x0, y0, z0, x1, y1, z1, mat, coll=C_SOLID, bit=B_DETAIL, yaw=0.0, pitch=0.0, roll=0.0, tag=""):
        if x1 - x0 < 0.005 or y1 - y0 < 0.005 or z1 - z0 < 0.005:
            return
        G["boxes"].append(dict(c=((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), s=(x1 - x0, y1 - y0, z1 - z0),
                               yaw=yaw, pitch=pitch, roll=roll, mat=mat, coll=coll, bit=bit, tag=tag))

    def add_rot_box(c, s, mat, coll, bit, yaw=0.0, pitch=0.0, roll=0.0, tag=""):
        G["boxes"].append(dict(c=c, s=s, yaw=yaw, pitch=pitch, roll=roll, mat=mat, coll=coll, bit=bit, tag=tag))

    # ---- stairs first: where they arrive on or leave a floor, the rail is left out
    stair_info = []
    for s in plan["stairs"]:
        steps, rise, tread = stair_steps(s)
        stair_info.append((s, steps, rise, tread))
        for end, z in ((s["s0"], s["z0"]), (s["s1"], s["z1"])):
            lvs = [k for k, zz in enumerate(LEVELS) if abs(zz - z) < 0.01]
            for lv in lvs:
                if s["axis"] == "y":
                    G["arrivals"][lv].append(("H", end, s["c0"], s["c1"]))
                else:
                    G["arrivals"][lv].append(("V", end, s["c0"], s["c1"]))

    rooms_all = plan["rooms"]
    door_id = 0
    for lv in range(6):
        xs, ys, cells, rooms = level_grid(plan, lv)
        nx, ny = len(xs) - 1, len(ys) - 1
        z0 = LEVELS[lv]
        ztop = level_top(lv)

        def cell(i, j):
            if i < 0 or j < 0 or i >= nx or j >= ny:
                return ("out", False)
            return cells[i, j]

        # ---- floors
        def mat_of(i, j):
            sp, fl = cells[i, j]
            if not fl:
                return None
            cx, cy = (xs[i] + xs[i + 1]) / 2, (ys[j] + ys[j + 1]) / 2
            if sp[0] == "room":
                r = rooms[sp[1]]
                if r.kind in ("trapp", "heis"):
                    return "ConcreteSmooth_In"
                if r.kind == "boks":
                    return "Floor_In"
                return "WoodLight_In" if r.kind in ("korridor", "lobby") else "Floor_In"
            if lv == P1:
                return "Terrazzo_In"
            if lv == P2 and abs(cx) < 13.25 and 5.6 < cy < 20.05:
                return "HelgOrange_In"
            if lv == P2 and -5.8 < cx < -1.0 and 1.0 < cy < 5.6:
                return "HelgYellow_In"
            if lv in (P3, P4) and abs(cx) < 13.25 and abs(cy) < 17.75:
                return "Floor_In" if lv == P3 and cx > 7.4 and -6.1 < cy < 9.1 else "WoodLight_In"
            return "WoodLight_In"

        for x0, y0, x1, y1, m in rects_from_cells(xs, ys, mat_of):
            G["floors"][lv].append((x0, y0, x1, y1))
            # over P4 inside the atrium nothing may carry a player (K10); P5/P6 floors are outside the atrium
            add_box(x0, y0, z0 - SLAB, x1, y1, z0, m, C_SOLID, lv)

        # ---- edges -> wall pieces
        pieces = {}
        for i in range(-1, nx):
            for j in range(ny):
                a, fa = cell(i, j)
                b, fb = cell(i + 1, j)
                k = wall_kind(a, b, fa, fb, rooms, "V", xs[i + 1], True)
                if k:
                    pieces.setdefault(("V", xs[i + 1], k), []).append((ys[j], ys[j + 1], a, b))
        for j in range(-1, ny):
            for i in range(nx):
                a, fa = cell(i, j)
                b, fb = cell(i, j + 1)
                k = wall_kind(a, b, fa, fb, rooms, "H", ys[j + 1], True)
                if k:
                    pieces.setdefault(("H", ys[j + 1], k), []).append((xs[i], xs[i + 1], a, b))
        segs = []
        for (o, c, k), lst in pieces.items():
            lst.sort()
            cur = None
            for t0, t1, a, b in lst:
                boxside = any(isinstance(sp, tuple) and rooms[sp[1]].kind == "boks" for sp in (a, b))
                if cur and abs(cur["t1"] - t0) < EPS and cur["box"] == boxside:
                    cur["t1"] = t1
                else:
                    cur = dict(o=o, c=c, k=k, t0=t0, t1=t1, box=boxside, holes=[])
                    segs.append(cur)
        # rails: leave out where a stair arrives or leaves
        for sg in segs:
            if sg["k"] != "rail":
                continue
            for o, c, a, b in G["arrivals"][lv]:
                if o == sg["o"] and abs(c - sg["c"]) < 0.02:
                    sg["holes"].append((a, b, None, "stair"))
        # boundary between floor and hole (for the gap check)
        for sg in segs:
            if sg["k"] == "rail":
                G["boundary"][lv].append((sg["o"], sg["c"], sg["t0"], sg["t1"]))

        def find_seg(o, c, t, kinds=None):
            for sg in segs:
                if sg["o"] == o and abs(sg["c"] - c) < 0.02 and sg["t0"] - EPS <= t <= sg["t1"] + EPS and (kinds is None or sg["k"] in kinds):
                    return sg
            return None

        # ---- doors
        for r in rooms:
            for (x, y, w, kind) in r.doors:
                o = "V" if abs(x - r.x0) < 0.02 or abs(x - r.x1) < 0.02 else "H"
                c, t = (x, y) if o == "V" else (y, x)
                sg = find_seg(o, c, t, ("ext", "core", "inner", "light", "glass", "fixglass"))
                if not sg:
                    G["errors"].append("dør uten vegg: %s %s (%.2f, %.2f)" % (LEVEL_NAMES[lv], r.name, x, y))
                    continue
                if kind != "laast":   # the alarm door (Trapp sør, P4) is a sealed slot: wall, no hole (bNoOpening)
                    sg["holes"].append((t - w / 2, t + w / 2, DOOR_H, "door"))
                # the door faces out of the room it belongs to
                if o == "V":
                    yaw = 0.0 if abs(x - r.x1) < 0.02 else 180.0
                else:
                    yaw = 90.0 if abs(y - r.y1) < 0.02 else -90.0
                G["doors"].append(dict(id=door_id, x=x, y=y, z=z0, w=w, h=DOOR_H, yaw=yaw, kind=kind, lv=lv, room=r.name,
                                       hinge_left=True, locked=kind in ("laast", "glass_kode"), open=False, o=o,
                                       no_opening=kind == "laast"))
                door_id += 1
            for (x0, y0, x1, y1) in r.openings:
                o = "V" if abs(x0 - x1) < EPS else "H"
                c, a, b = (x0, y0, y1) if o == "V" else (y0, x0, x1)
                hit = False
                for sg in segs:
                    if sg["o"] == o and abs(sg["c"] - c) < 0.02 and sg["t0"] < b and sg["t1"] > a:
                        sg["holes"].append((a, b, None, "open"))
                        hit = True
                if not hit:
                    G["errors"].append("åpning uten vegg: %s %s" % (LEVEL_NAMES[lv], r.name))
        # ---- breach walls
        for (blv, x0, y0, x1, y1) in plan["breach"]:
            if blv != lv:
                continue
            o = "V" if abs(x0 - x1) < EPS else "H"
            c, a, b = (x0, y0, y1) if o == "V" else (y0, x0, x1)
            sg = find_seg(o, c, (a + b) / 2, ("inner", "core", "ext", "light"))
            if not sg:
                G["errors"].append("lettvegg uten vegg: %s (%.2f, %.2f)" % (LEVEL_NAMES[lv], x0, y0))
                continue
            sg["holes"].append((a, b, BREACH_H, "breach"))
            t = WALL_IN
            if o == "V":
                bx = (c - t / 2, a, z0, c + t / 2, b, z0 + BREACH_H)
            else:
                bx = (a, c - t / 2, z0, b, c + t / 2, z0 + BREACH_H)
            G["breach"].append(dict(id=len(G["breach"]), lv=lv, box=bx, o=o, c=c, a=a, b=b, kind=sg["k"]))

        # ---- build the walls
        for sg in segs:
            k, o, c = sg["k"], sg["o"], sg["c"]
            th = THICK[k]
            if k == "rail":
                wz0, wz1 = z0, z0 + RAIL_H
            elif sg["box"]:
                wz0, wz1 = z0, z0 + BOX_H
            else:
                wz0, wz1 = z0 - SLAB, ztop       # down through the slab: no light gap where a floor is missing
            ext = th / 2 if k in OPAQUE or k == "fixglass" else 0.0
            t0, t1 = sg["t0"] - ext, sg["t1"] + ext
            holes = sorted(sg["holes"])
            # clip holes and build solid intervals
            solid = []
            cur = t0
            for a, b, h, why in holes:
                a, b = max(a, t0), min(b, t1)
                if b <= a:
                    continue
                if a > cur:
                    solid.append((cur, a, None))
                if h is not None:
                    solid.append((a, b, h))      # lintel over a door / breach panel
                cur = max(cur, b)
            if cur < t1:
                solid.append((cur, t1, None))
            sg["solid"] = solid
            G["walls"].append(dict(lv=lv, o=o, c=c, k=k, t0=sg["t0"], t1=sg["t1"], z0=wz0, z1=wz1, holes=holes, solid=solid, th=th))

            def emit(a, b, za, zb, mat, coll=C_SOLID, bit=lv):
                if o == "V":
                    add_box(c - th / 2, a, za, c + th / 2, b, zb, mat, coll, bit)
                else:
                    add_box(a, c - th / 2, za, b, c + th / 2, zb, mat, coll, bit)

            over = lv >= P4 and sg["box"]          # P4 box walls stand on P4: they may block players (bottom < 14 m)
            if k in ("ext", "core", "inner", "light"):
                mat = {"ext": "PlasterWhite_In", "core": "ConcreteSmooth_In", "inner": "PlasterWhite_In", "light": "PlasterWhite_In"}[k]
                for a, b, h in solid:
                    if h is not None:
                        emit(a, b, z0 + h, wz1, mat)
                        continue
                    if k == "ext" and wz1 - z0 > 3.2 and lv <= P6:
                        # facade windows: 1.0 m pier + 2.0 m window (sill 0.9, head 3.0), fixed glass
                        L = b - a
                        nmod = int(L // 3.0)
                        if nmod >= 1:
                            off = a + (L - nmod * 3.0) / 2
                            emit(a, b, wz0, z0 + 0.9, mat)
                            emit(a, b, z0 + 3.0, wz1, mat)
                            emit(a, off + 0.5, z0 + 0.9, z0 + 3.0, mat)
                            for m in range(nmod):
                                wa = off + m * 3.0 + 0.5
                                emit(wa, wa + 2.0, z0 + 0.9, z0 + 3.0, "Glass", C_SOLID)
                                emit(wa + 2.0, min(b, wa + 3.0) if m < nmod - 1 else b, z0 + 0.9, z0 + 3.0, mat)
                            continue
                    emit(a, b, wz0, wz1, mat)
                # 2D sight: opaque where solid at eye height
                for a, b, h in solid:
                    if h is None:
                        G["opaque"][lv].append((o, c, a, b, k))
                if lv == P4 and sg["box"]:
                    pass
            elif k == "fixglass":
                for a, b, h in solid:
                    emit(a, b, wz0, wz1 if h is None else wz1, "Glass", C_SOLID)
                    G["glass2d"][lv].append((o, c, a, b))
            elif k in ("glass", "rail"):
                ptype = 0 if k == "rail" else 1
                hgt = RAIL_H if k == "rail" else BOX_H
                for a, b, h in solid:
                    if h is not None:   # lintel over a glass door in a glass wall
                        emit(a, b, z0 + h, wz1, "MetalDark_In")
                        continue
                    L = b - a
                    if L < 0.05:
                        continue
                    G["glass2d"][lv].append((o, c, a, b))
                    if lv >= P5:
                        # scenery rails over the atrium: fixed glass, only blocks Visibility (the hook passes, K10)
                        emit(a, b, z0, z0 + hgt, "Glass", C_HIGH)
                        emit(a, b, z0 + hgt, z0 + hgt + 0.06, "Wood_In", C_HIGH)
                        continue
                    n = max(1, math.ceil(L / 1.5 - 1e-6))
                    for m in range(n):
                        pa = a + L * m / n
                        pb = a + L * (m + 1) / n
                        cx, cy = (c, (pa + pb) / 2) if o == "V" else ((pa + pb) / 2, c)
                        G["panels"].append(dict(id=len(G["panels"]), c=(cx, cy, z0 + hgt / 2), len=pb - pa - 0.01, h=hgt,
                                                t=GLASS_T, yaw=90.0 if o == "V" else 0.0, type=ptype, lv=lv))
                    if k == "rail":
                        emit(a, b, z0 + RAIL_H, z0 + RAIL_H + 0.06, "Wood_In")      # wooden top rail
                        emit(a, b, z0, z0 + 0.05, "MetalDark_In")                  # foot profile
                    else:
                        emit(a, b, z0 + BOX_H - 0.08, z0 + BOX_H, "MetalDark_In", C_HIGH if over else C_SOLID)   # black frame
        # glass box frames (black posts) and the thick white underside (box roof)
        for r in rooms:
            if r.kind != "boks":
                continue
            coll = C_HIGH if lv >= P4 else C_SOLID
            add_box(r.x0, r.y0, z0 + BOX_H, r.x1, r.y1, z0 + BOX_H + BOX_ROOF, "PlasterWhite_In", coll, lv)
            for px, py in ((r.x0, r.y0), (r.x1, r.y0), (r.x0, r.y1), (r.x1, r.y1)):
                add_box(px - 0.05, py - 0.05, z0, px + 0.05, py + 0.05, z0 + BOX_H, "MetalDark_In", C_SOLID, lv)
        # P4: the box roofs stop bodies (Coll_High) but end on the middle of the wall under their edge. Where no other box
        # roof continues, a white cap over the wall top (2 cm above it) closes the 6 cm strip a body could perch on
        # (runde-04a funn 3; check_helgasetr_walk 5). Collinear caps are merged so no faces lie on top of each other.
        if lv == P4:
            boxes4 = [r for r in rooms if r.kind == "boks"]
            caps = {}
            for r in boxes4:
                for o, c, a, b, nx, ny in (("H", r.y0, r.x0, r.x1, 0.0, -0.2), ("H", r.y1, r.x0, r.x1, 0.0, 0.2),
                                           ("V", r.x0, r.y0, r.y1, -0.2, 0.0), ("V", r.x1, r.y0, r.y1, 0.2, 0.0)):
                    mx, my = ((a + b) / 2, c) if o == "H" else (c, (a + b) / 2)
                    if any(q is not r and q.contains(mx + nx, my + ny) for q in boxes4):
                        continue
                    caps.setdefault((o, r3(c)), []).append([a - 0.11, b + 0.11])
            for (o, c), ivs in sorted(caps.items()):
                ivs.sort()
                merged = [ivs[0]]
                for a, b in ivs[1:]:
                    if a <= merged[-1][1]:
                        merged[-1][1] = max(merged[-1][1], b)
                    else:
                        merged.append([a, b])
                for a, b in merged:
                    za, zb = z0 + BOX_H + 0.02, z0 + BOX_H + BOX_ROOF - 0.02
                    if o == "H":
                        add_box(a, c - 0.1, za, b, c + 0.1, zb, "PlasterWhite_In", C_HIGH, lv)
                    else:
                        add_box(c - 0.1, a, za, c + 0.1, b, zb, "PlasterWhite_In", C_HIGH, lv)

    # ---- columns (Ø 0.5 on axes 3 and 7 at the letter axes E-K, on P1-P4 where there is floor at the ring edge)
    for lv in (P1, P2, P3, P4):
        for x in (-13.54, 13.54):
            for y in (-18.03, -12.02, -6.01, 0.0, 6.01, 12.02, 18.03):
                if lv == P2 and x < 0 and -6.5 < y < 5.6:
                    continue
                add_box(x - 0.25, y - 0.25, LEVELS[lv], x + 0.25, y + 0.25, level_top(lv), "ConcreteSmooth_In", C_SOLID, lv)
                G["opaque"][lv].append(("BOX", x - 0.25, y - 0.25, x + 0.25, y + 0.25, "column"))

    # ---- stairs: one box per step, filled down to the floor where the space under it is lower than 2.0 m (K11)
    for s, steps, rise, tread in stair_info:
        mat = "HelgYellow_In" if s.get("atrium") else "ConcreteSmooth_In"
        thick = rise + 0.22
        zfloor = s["z0"]
        for (x0, y0, x1, y1, zt, k) in steps:
            if s.get("nofill"):
                zb = zt - thick
                filled = False
            elif s.get("solid") or zt - thick - zfloor < 2.0 and not s.get("well") or (s.get("well") and zfloor == 0.0 and zt - thick < 2.0):
                zb = zfloor - 0.01
                filled = True
            else:
                zb = zt - thick
                filled = False
            add_box(x0, y0, zb, x1, y1, zt, mat, C_SOLID, B_STAIRS)
            G["steps"].append(dict(stair=s["name"], rect=(x0, y0, x1, y1), ztop=zt, zbot=zb, filled=filled, kind=k))
        if s.get("endland"):
            pass
        # guards (black, 1.0 m over the nosing line) on both sides of the atrium runs; yellow stringer under them
        if s.get("atrium") and s.get("guard", True):
            groups = []
            cur = []
            for st in steps:
                if st[5] == "landing":
                    if cur:
                        groups.append(cur)
                    groups.append([st])
                    cur = []
                else:
                    cur.append(st)
            if cur:
                groups.append(cur)
            for gp in groups:
                xa, ya, xb, yb = min(g[0] for g in gp), min(g[1] for g in gp), max(g[2] for g in gp), max(g[3] for g in gp)
                za_, zb_ = gp[0][4] - (rise if gp[0][5] == "step" else 0.0), gp[-1][4]
                if s["dir"] < 0:
                    za_, zb_ = gp[0][4] - (rise if gp[0][5] == "step" else 0.0), gp[-1][4]
                for side in (0, 1):
                    if s["axis"] == "y":
                        cxx = (s["c0"] + 0.03) if side == 0 else (s["c1"] - 0.03)
                        L = yb - ya
                        cyy = (ya + yb) / 2
                    else:
                        cyy = (s["c0"] + 0.03) if side == 0 else (s["c1"] - 0.03)
                        L = xb - xa
                        cxx = (xa + xb) / 2
                    dz = zb_ - za_
                    ang = math.degrees(math.atan2(dz, L))
                    hz = 1.0 * math.cos(math.radians(ang))
                    zc = (za_ + zb_) / 2 + 0.5
                    # local X along the run, rising towards the top: yaw points the way the stair goes up
                    if s["axis"] == "y":
                        yaw = 90.0 if s["dir"] > 0 else -90.0
                    else:
                        yaw = 0.0 if s["dir"] > 0 else 180.0
                    length = math.hypot(L, dz)
                    add_rot_box((cxx, cyy, zc), (length, 0.04, hz + 0.25), "MetalDark_In", C_SOLID, B_STAIRS, yaw=yaw, pitch=ang)
                    add_rot_box((cxx, cyy, zc - 0.75), (length, 0.06, 0.35 * math.cos(math.radians(ang))), "HelgYellow_In", C_SOLID, B_STAIRS, yaw=yaw, pitch=ang)
    for (x0, y0, x1, y1, z) in plan["well_landings"]:
        add_box(x0, y0, z - 0.25, x1, y1, z, "ConcreteSmooth_In", C_SOLID, B_STAIRS)
        G["steps"].append(dict(stair="repos", rect=(x0, y0, x1, y1), ztop=z, zbot=z - 0.25, filled=False, kind="landing"))
    for (x0, y0, x1, y1, za, zb) in plan["well_dividers"]:
        add_box(x0, y0, za, x1, y1, zb, "ConcreteSmooth_In", C_SOLID, B_STAIRS)

    # ---- Ryddekroken (yellow tower, P1 -> P2 gallery) and the orange sitting stair on the mezzanine
    add_box(-5.8, 1.0, 0.0, -1.0, 5.6, LEVELS[P2] - SLAB, "HelgYellow_In", C_SOLID, P1)
    G["opaque"][P1].append(("BOX", -5.8, 1.0, -1.0, 5.6, "tower"))
    for k in range(3):
        add_box(8.0, 6.5 + k * 1.0, LEVELS[P2], 12.5, 9.5, LEVELS[P2] + 0.4 * (k + 1), "HelgOrange_In", C_SOLID, P2)

    # ---- screens (opaque, 2.1 m)
    for (x, y, L, o) in plan["screens"]:
        lv = P4
        if o == "H":
            add_box(x - L / 2, y - 0.05, LEVELS[lv], x + L / 2, y + 0.05, LEVELS[lv] + 2.1, "Wood_In", C_SOLID, B_DETAIL)
            G["opaque"][lv].append(("H", y, x - L / 2, x + L / 2, "screen"))
        else:
            add_box(x - 0.05, y - L / 2, LEVELS[lv], x + 0.05, y + L / 2, LEVELS[lv] + 2.1, "Wood_In", C_SOLID, B_DETAIL)
            G["opaque"][lv].append(("V", x, y - L / 2, y + L / 2, "screen"))
    # ---- breach panels are opaque too
    for b in G["breach"]:
        G["opaque"][b["lv"]].append((b["o"], b["c"], b["a"], b["b"], "breach"))
    # ---- cover
    for k, (lv, name, x0, y0, x1, y1, h, mat) in enumerate(plan["cover"]):
        G["cover"].append(dict(id=k, lv=lv, name=name, box=(x0, y0, LEVELS[lv], x1, y1, LEVELS[lv] + h), mat=mat))
        if h >= 1.2:
            G["opaque"][lv].append(("BOX", x0, y0, x1, y1, "cover"))

    # ---- team stripes (no collision)
    for (x0, y0, x1, y1, z, team) in plan["stripes"]:
        add_box(x0, y0, z, x1, y1, z + 0.14, "PaintRed_In" if team == 0 else "PaintBlue_In", C_NONE, B_DETAIL)

    # ---- roof: flat slab outside the atrium roof, saw-tooth glass over it, white truss under it (helgasetr.md kap. 7)
    ar = ROOF_AREA
    xs, ys, cells, rooms = level_grid(plan, P6)
    xs2 = sorted(set(xs) | {ar[0], ar[2]})
    ys2 = sorted(set(ys) | {ar[1], ar[3]})

    def roof_key(i, j):
        cx, cy = (xs2[i] + xs2[i + 1]) / 2, (ys2[j] + ys2[j + 1]) / 2
        if any(in_rect(cx, cy, c) for c in plan["cuts"][P6]) or in_rect(cx, cy, ar):
            return None
        return 1
    for x0, y0, x1, y1, _ in rects_from_cells(xs2, ys2, roof_key):
        add_box(x0, y0, ROOF_Z - SLAB, x1, y1, ROOF_Z, "ConcreteSmooth_In", C_SOLID, B_ROOF)
    # saw teeth (roof_teeth): shallow opaque sheet rising to the ridge, then the steep glass face down to the next valley.
    # The glass faces SOUTH, so the low Trondheim sun shines in and the white truss throws its shadows into the atrium.
    h = RIDGE_Z - ROOF_Z
    wx = ar[2] - ar[0]
    for (ya, yr, yb) in roof_teeth():
        for (p0, p1, m) in (((ya, ROOF_Z), (yr, RIDGE_Z), "PaintWhite_In"), ((yr, RIDGE_Z), (yb, ROOF_Z), "Glass")):
            dy, dz = p1[0] - p0[0], p1[1] - p0[1]
            L = math.hypot(dy, dz)
            roll = math.degrees(math.atan2(-dz, dy))
            add_rot_box((0.0, (p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2), (wx, L, 0.06), m, C_SOLID, B_ROOF, roll=roll,
                        tag="tak" if m != "Glass" else "")
        # glass gable ends, stepped to follow the slope
        for xe in (ar[0], ar[2]):
            for q in range(4):
                y0q = ya + (yr - ya) * q / 4
                y1q = ya + (yr - ya) * (q + 1) / 4
                add_box(xe - 0.03, y0q, ROOF_Z, xe + 0.03, y1q, ROOF_Z + h * (q + 1) / 4, "Glass", C_SOLID, B_ROOF)
            add_box(xe - 0.03, yr, ROOF_Z, xe + 0.03, yb, RIDGE_Z, "Glass", C_SOLID, B_ROOF)
        # truss: valley beam and ridge beam along X, posts and rafters on the axes
        add_box(ar[0], ya - 0.15, ROOF_Z - 0.45, ar[2], ya + 0.15, ROOF_Z - 0.05, "PaintWhite_In", C_HIGH, B_ROOF)
        add_box(ar[0], yr - 0.1, RIDGE_Z - 0.25, ar[2], yr + 0.1, RIDGE_Z - 0.05, "PaintWhite_In", C_HIGH, B_ROOF)
        for xt in (-13.54, -6.77, 0.0, 6.77, 13.54):
            add_box(xt - 0.06, yr - 0.06, ROOF_Z - 0.45, xt + 0.06, yr + 0.06, RIDGE_Z - 0.25, "PaintWhite_In", C_HIGH, B_ROOF)
            for (q0, q1) in (((ya, ROOF_Z - 0.1), (yr, RIDGE_Z - 0.25)), ((yr, RIDGE_Z - 0.25), (yb, ROOF_Z - 0.4))):
                dy, dz = q1[0] - q0[0], q1[1] - q0[1]
                add_rot_box((xt, (q0[0] + q1[0]) / 2, (q0[1] + q1[1]) / 2), (0.1, math.hypot(dy, dz), 0.1), "PaintWhite_In", C_HIGH, B_ROOF,
                            roll=math.degrees(math.atan2(-dz, dy)))
    add_box(ar[0], ar[3] - 0.15, ROOF_Z - 0.45, ar[2], ar[3] + 0.15, ROOF_Z - 0.05, "PaintWhite_In", C_HIGH, B_ROOF)
    for xt in (-13.54, -6.77, 0.0, 6.77, 13.54):
        add_box(xt - 0.08, ar[1], ROOF_Z - 0.6, xt + 0.08, ar[3], ROOF_Z - 0.4, "PaintWhite_In", C_HIGH, B_ROOF)

    # ---- invisible walls along the atrium edge on P5 and P6 (K10): the grapple hook finds nothing up there
    zb0, zb1 = LEVELS[P4] + 3.7, ROOF_Z - 0.06
    ri = RING_IN
    for (x0, y0, x1, y1) in ((-ri[0], -ri[1], -ri[0] + 0.1, ri[1]), (ri[0] - 0.1, -ri[1], ri[0], ri[1]),
                             (-ri[0], -ri[1], ri[0], -ri[1] + 0.1), (-ri[0], ri[1] - 0.1, ri[0], ri[1])):
        add_box(x0, y0, zb0, x1, y1, zb1, "PlasterWhite_In", C_BLOCKER, B_DETAIL, tag="blocker")

    # ---- ids for lights' wires (no collision) are in C++; ground outside is in C++
    return G


# ================================================================ round 4b: the Blender model, part by part (K13)
# Every structure box belongs to at most one model part (model_part_of). AGSHelgasetrBuilder skips the boxes of a part
# when /Game/Models/<part> exists and places the model instead, so a part that is missing (or not imported yet) is
# still there as blocks. building_helgasetr.py builds each part from exactly the same boxes (same data, same place).
# Collision kinds (C++ GSHelgasetr::EModelCollM):
MC_FULL, MC_GLASS_FULL, MC_ABOVE, MC_GLASS_ABOVE, MC_NONE = range(5)
MODEL_COLL_NAMES = ["Full", "GlassFull", "AboveAtrium", "GlassAbove", "None"]
PART_PREFIX = "SM_Bygg_Helgasetr_"
GLASS_PREFIX = "SM_Glass_Helgasetr_"
# level parts: split at X = 0 and Y = -18 / 0 / +18, so no part is much over 26 m (Lumen distance fields, K17)
PART_BANDS = [("N", -99.0, -18.0), ("MN", -18.0, 0.0), ("MS", 0.0, 18.0), ("S", 18.0, 99.0)]
# parts without blocks of their own: decoration that never collides (lamellas, rings, downlights, stair undersides)
EXTRA_PARTS = [(PART_PREFIX + "Pynt", MC_NONE)]


def part_region(x, y):
    side = "V" if x < 0.0 else "O"
    band = next(n for n, a, b in PART_BANDS if a <= y < b)
    return side + "_" + band


def model_part_of(b):
    """(part name, collision kind) of a structure box from derive(), or None when it stays a block."""
    bit, coll = b["bit"], b["coll"]
    glass = b["mat"] == "Glass"
    if bit == B_DETAIL or coll in (C_NONE, C_BLOCKER):
        return None
    if bit == B_ROOF:
        if glass:
            return GLASS_PREFIX + "Tak", MC_GLASS_FULL
        if coll == C_HIGH:
            return PART_PREFIX + "Takfagverk", MC_ABOVE
        return PART_PREFIX + "Tak", MC_FULL
    cx, cy = b["c"][0], b["c"][1]
    if bit == B_STAIRS:
        if cx > 18.0 or cy > 26.0:
            return PART_PREFIX + "Trapperom", MC_FULL
        return PART_PREFIX + "Trapper", MC_FULL
    if coll == C_HIGH:
        return (GLASS_PREFIX + "Overatriet", MC_GLASS_ABOVE) if glass else (PART_PREFIX + "Overatriet", MC_ABOVE)
    if glass:
        return GLASS_PREFIX + "Fasade", MC_GLASS_FULL
    return "%s%s_%s" % (PART_PREFIX, LEVEL_NAMES[bit], part_region(cx, cy)), MC_FULL


def model_parts(G):
    """Ordered [(name, kind)] of every part, and the part index of each box in G["boxes"] (255 = stays a block)."""
    of_box = [model_part_of(b) for b in G["boxes"]]
    names = sorted({p for p in of_box if p is not None} | set(EXTRA_PARTS))   # sorted: indices do not move with the box order
    index = {p[0]: k for k, p in enumerate(names)}
    return names, [255 if p is None else index[p[0]] for p in of_box]


def panels_by_level(G, lv):
    return [p for p in G["panels"] if p["lv"] == lv]


if __name__ == "__main__":
    P = make_plan()
    G = derive(P)
    print("rom %d, klosser %d, glassfelt %d, dører %d, lettvegger %d, dekning %d, trinn %d, feil %d" % (
        len(P["rooms"]), len(G["boxes"]), len(G["panels"]), len(G["doors"]), len(G["breach"]), len(G["cover"]), len(G["steps"]), len(G["errors"])))
    for e in G["errors"]:
        print("  " + e)
