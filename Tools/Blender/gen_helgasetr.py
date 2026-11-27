"""Writes Source/Gloshaugen/Public/GSHelgasetr.gen.inl from Tools/Blender/helgasetr_plan.py (plain Python).

    python Tools/Blender/gen_helgasetr.py            writes the file
    python Tools/Blender/gen_helgasetr.py --check    writes nothing, exits 1 if the file is out of date

The file is included inside `namespace GSHelgasetr` by GSHelgasetr.h, which every file that reads GSCampus.h
pulls in. So it holds only `inline constexpr` tables (K16: no plain globals, unity builds and ODR).
Ids (doors, glass panels, breach walls, cover) are the order of the lists and stay stable as long as the
plan data is not reordered; other teams (Interaktiv verden) key their state on them.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import helgasetr_plan as H  # noqa: E402

ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(ROOT, "Source", "Gloshaugen", "Public", "GSHelgasetr.gen.inl")


def f(v):
    s = ("%.3f" % v).rstrip("0").rstrip(".")
    if s in ("-0", ""):
        s = "0"
    if "." not in s:
        s += ".0"
    return s + "f"


def t(s):
    """TEXT() literal with \\u escapes for anything outside ASCII (the source files are not always read as UTF-8)."""
    out = []
    for ch in s:
        if ord(ch) < 128 and ch not in '"\\':
            out.append(ch)
        elif ch in '"\\':
            out.append("\\" + ch)
        else:
            out.append("\\u%04X" % ord(ch))
    return 'TEXT("%s")' % "".join(out)


def generate():
    P = H.make_plan()
    G = H.derive(P)
    if G["errors"]:
        raise SystemExit("helgasetr_plan.py har feil:\n  " + "\n  ".join(G["errors"]))
    mat = {m: i for i, m in enumerate(H.MATS)}
    L = []
    w = L.append
    w("// GENERERT av Tools/Blender/gen_helgasetr.py - ikke rediger. Kilde: Tools/Blender/helgasetr_plan.py")
    w("// Inkluderes inne i namespace GSHelgasetr av GSHelgasetr.h. Meter; X = ost, Y = sor, Z = opp.")
    w("")
    w("inline constexpr int32 NumLevels = %d;" % len(H.LEVELS))
    w("inline constexpr float LevelZM[] = { %s };" % ", ".join(f(z) for z in H.LEVELS))
    w("inline constexpr float RoofZM = %s;" % f(H.ROOF_Z))
    w("inline constexpr float SlabM = %s;" % f(H.SLAB))
    w("inline constexpr float FlagZBridgeM = %s;   // GSCampus::Bases() Flag.Z until the spawn trace starts at +2 m (K1)" % f(H.FLAG_Z_BRIDGE))
    w("inline constexpr const TCHAR* MatNames[] = { %s };" % ", ".join(t(m) for m in H.MATS))
    w("inline constexpr const TCHAR* BitNames[] = { %s };" % ", ".join(t(b) for b in H.BITS))
    w("inline constexpr const TCHAR* DoorKindNames[] = { %s };" % ", ".join(t(b) for b in H.DOOR_KINDS))
    pa = P["play_area"]
    w("inline constexpr FRectM PlayAreaM = { %s, %s, %s, %s };" % tuple(f(v) for v in pa))
    w("")
    w("inline constexpr FZoneM ZonesM[] = {")
    for name, x0, y0, x1, y1 in P["zones"]:
        w("\t{ %s, %s, %s, %s, %s }," % (t(name), f(x0), f(y0), f(x1), f(y1)))
    w("};")
    w("inline constexpr FSegM CorridorsM[] = {")
    for (a, b) in P["corridors"]:
        w("\t{ %s, %s, %s, %s }," % (f(a[0]), f(a[1]), f(b[0]), f(b[1])))
    w("};")
    w("")
    w("// Index 0 = Sor (rodt lag), 1 = Nord (blatt lag).")
    w("inline constexpr FPointM SpawnsM[] = {")
    first = []
    for b in P["bases"]:
        first.append(sum(len(bb["spawns"]) for bb in P["bases"][:P["bases"].index(b)]))
        for (x, y) in b["spawns"]:
            w("\t{ %s, %s }," % (f(x), f(y)))
    w("};")
    w("inline constexpr FBaseM BasesM[] = {")
    for k, b in enumerate(P["bases"]):
        fx, fy, fz = b["flag"]
        w("\t{ %s, %s, %s, %s, %s, %s, %d, %d }," % (t(b["name"]), t(b["room"]), f(fx), f(fy), f(fz), f(b["yaw"]), first[k], len(b["spawns"])))
    w("};")
    w("")
    w("inline constexpr FLevelRectM FloorRectsM[] = {")
    for lv in range(len(H.LEVELS)):
        for (x0, y0, x1, y1) in G["floors"][lv]:
            w("\t{ %d, %s, %s, %s, %s }," % (lv, f(x0), f(y0), f(x1), f(y1)))
    w("};")
    w("")
    w("// Fixed structure: one box per entry (centre, size, yaw/pitch/roll in degrees, material index, collision, bit).")
    w("inline constexpr FBoxM BoxesM[] = {")
    for b in G["boxes"]:
        c, s = b["c"], b["s"]
        w("\t{ %s, %s, %s, %s, %s, %s, %s, %s, %s, %d, %d, %d }," % (f(c[0]), f(c[1]), f(c[2]), f(s[0]), f(s[1]), f(s[2]),
                                                               f(b["yaw"]), f(b["pitch"]), f(b["roll"]), mat[b["mat"]], b["coll"], b["bit"]))
    w("};")
    w("")
    parts, per_box = H.model_parts(G)
    w("// Round 4b: Blender model parts (/Game/Models/<Name>). A part that exists replaces its boxes (BoxPartM, 255 = always a box).")
    w("inline constexpr FModelPartM ModelPartsM[] = {")
    for name, kind in parts:
        w("	{ %s, %d },   // %s" % (t(name), kind, H.MODEL_COLL_NAMES[kind]))
    w("};")
    w("inline constexpr uint8 BoxPartM[] = {")
    for k in range(0, len(per_box), 40):
        w("	" + ", ".join(str(v) for v in per_box[k:k + 40]) + ",")
    w("};")
    w("static_assert(UE_ARRAY_COUNT(BoxPartM) == UE_ARRAY_COUNT(BoxesM), \"BoxPartM has one entry per box\");")
    w("")
    w("// Breakable glass: Id = index. Type 0 rekkverk, 1 vegg. (Glass doors are in DoorsM, K: Interaktiv verden.)")
    w("inline constexpr FPanelM PanelsM[] = {")
    for p in G["panels"]:
        c = p["c"]
        w("\t{ %d, %s, %s, %s, %s, %s, %s, %s, %d, %d }," % (p["id"], f(c[0]), f(c[1]), f(c[2]), f(p["len"]), f(p["h"]), f(p["t"]), f(p["yaw"]), p["type"], p["lv"]))
    w("};")
    w("inline constexpr FPieceM BreachWallsM[] = {")
    for b in G["breach"]:
        x0, y0, z0, x1, y1, z1 = b["box"]
        w("\t{ %d, %s, %s, %s, %s, %s, %s, %d, %d, %s }," % (b["id"], f(x0), f(y0), f(z0), f(x1), f(y1), f(z1), mat["PlasterWhite_In"], b["lv"], t("Lettvegg %d" % b["id"])))
    w("};")
    w("inline constexpr FPieceM CoverM[] = {")
    for c in G["cover"]:
        x0, y0, z0, x1, y1, z1 = c["box"]
        w("\t{ %d, %s, %s, %s, %s, %s, %s, %d, %d, %s }," % (c["id"], f(x0), f(y0), f(z0), f(x1), f(y1), f(z1), mat[c["mat"]], c["lv"], t(c["name"])))
    w("};")
    w("// Doors: floor point in the middle of the opening, Yaw = the way the front faces (out of its room).")
    w("inline constexpr FDoorM DoorsM[] = {")
    for d in G["doors"]:
        w("\t{ %d, %s, %s, %s, %s, %s, %s, %s, %d, %s, %s, %d, %s, %s }," % (
            d["id"], f(d["x"]), f(d["y"]), f(d["z"]), f(d["w"]), f(d["h"]), f(d["yaw"]), "true" if d["hinge_left"] else "false",
            H.DOOR_KINDS.index(d["kind"]), "true" if d["locked"] else "false", "true" if d["open"] else "false", d["lv"], t(d["room"]),
            "true" if d.get("no_opening") else "false"))   # runde 5a: bNoOpening was never written (only dør 31 is sealed)
    w("};")
    w("")
    w("inline constexpr FLootM LootM[] = {")
    for (x, y, z, tier) in P["loot"]:
        w("\t{ %s, %s, %s, %d }," % (f(x), f(y), f(z), tier))
    w("};")
    w("inline constexpr FTerminalM TerminalsM[] = {")
    for (x, y, z, yaw, kind, name) in P["terminals"]:
        w("\t{ %s, %s, %s, %s, %d, %s }," % (f(x), f(y), f(z), f(yaw), kind, t(name)))
    w("};")
    w("inline constexpr FCameraM CamerasM[] = {")
    for (x, y, z, yaw, name) in P["cameras"]:
        w("\t{ %s, %s, %s, %s, %s }," % (f(x), f(y), f(z), f(yaw), t(name)))
    w("};")
    w("inline constexpr FSoundZoneM SoundZonesM[] = {")
    for (name, x0, y0, z0, x1, y1, z1, rt) in P["sound_zones"]:
        w("\t{ %s, %s, %s, %s, %s, %s, %s, %s }," % (t(name), f(x0), f(y0), f(z0), f(x1), f(y1), f(z1), f(rt)))
    w("};")
    w("inline constexpr FLightM LightsM[] = {")
    for (x, y, z, kind, r, shadow) in P["lights"]:
        w("\t{ %s, %s, %s, %d, %s, %s }," % (f(x), f(y), f(z), kind, f(r), "true" if shadow else "false"))
    w("};")
    w("inline constexpr FSignM SignsM[] = {")
    for (x, y, z, yaw, text, team) in P["signs"]:
        w("\t{ %s, %s, %s, %s, %s, %d }," % (f(x), f(y), f(z), f(yaw), t(text), team))
    w("};")
    w("")
    return "\n".join(L), P, G


def main():
    text, P, G = generate()
    data = text.encode("utf-8")
    if "--check" in sys.argv:
        old = open(OUT, "rb").read() if os.path.exists(OUT) else b""
        if old.replace(b"\r\n", b"\n") != data:
            print("FEIL    GSHelgasetr.gen.inl er utdatert: kjor python Tools/Blender/gen_helgasetr.py")
            sys.exit(1)
        print("OK      GSHelgasetr.gen.inl er oppdatert")
        return
    with open(OUT, "wb") as fh:
        fh.write(data)
    print("Skrev %s: %d klosser, %d glassfelt, %d dorer, %d lettvegger, %d dekning, %d loot" % (
        os.path.relpath(OUT, ROOT), len(G["boxes"]), len(G["panels"]), len(G["doors"]), len(G["breach"]), len(G["cover"]), len(P["loot"])))


if __name__ == "__main__":
    main()
