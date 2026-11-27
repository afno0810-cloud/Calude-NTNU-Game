"""Checks the Helgasetr map data (plain Python + numpy/scipy, no Blender, no Unreal).

    python Tools/Blender/check_helgasetr.py

Reads Tools/Blender/helgasetr_plan.py (make_plan / derive) and runs gen_helgasetr.py --check.
Every check has a counter-test: a changed copy of the data that MUST fail, otherwise the check proves nothing.
  a) axis sums 51.50 / 67.77 and storey heights                        (counter: P2 at 5.5)
  b) bases: same Z, one in each end, >= 45 m apart, flag >= 2 m from walls, two corridor doors each,
     point-symmetric flags/spawns/doors                                (counter: north flag 1 m to the side)
  c) spawns inside their own rooms, >= 0.6 m from walls, >= 1.5 m apart, and no spawn visible (2D: walls, screens,
     breach walls and cover block; open doors and glass do not) from the ring corridor, lobby openings,
     waiting room / NE corridor, stair exits and the R2/R4 rooms       (counter: a spawn right behind the S1 door)
  d) route graph on a 0.25 m walk grid with stairs: >= 4 of the routes R1-R6 walkable, >= 1 through Plan 1, and the
     shortest spawn -> enemy flag equal for both teams +-20 %          (counter: west ring blocked, wing doors locked)
  d2) every stair exit on Plan 4: walk to the enemy flag equal for both teams +-20 %; no stair exit closer than 12 m
     to, or in sight of, a spawn exit                                  (counter: Trapp sør door open on Plan 4)
  e) free 2D sight along every route (middle and +-1.0 m) <= 42 m      (counter: spawn corridor open to the ring)
  e2) no free line > 42 m from one base (flag +-2 m and doors) to the other  (counter: screens and box walls removed)
  e3) along R1/R3 a cover pocket at least every 8 m: a 1.5 m stretch with room for a body, hidden from the opposite
     ring side (2D) and from the Plan 3 ring (3D rays through slabs, walls and cover)  (counter: no cover, no box walls)
  f) loot, terminals, cameras, spawns and flags over a floor of their level  (counter: loot in the middle of the atrium)
  g) breach walls not in bases, stair cores or outer walls, point-symmetric  (counter: one in the Sørbasen wall)
  h) the whole building inside the play area
  i) under stairs: free height >= 2.0 m or solid; no 0.3-0.9 m gap between a stair and a slab edge  (counter: no fill under run 2)
  j) nothing that blocks Pawn above the Plan 4 floor inside the atrium, except the glass roof and the blockers
     (counter: a box roof on Plan 5 that blocks Pawn)
  k) id manifest (helgasetr_ids.json): no existing id of a door, glass panel, breach wall, cover piece or loot spot moves
     (loot: by at most 0.1 m)                                          (counter: a new door first on Plan 1)
  l) every loot spot over a floor and clear of every Coll_Solid box (sphere r 0.15 m, 0.1-0.4 m over the floor); the
     spots in the code-locked rooms on the room side of their door    (counters: the old spot (11.0, 3.0) in the stair
     fill on Plan 3; a room spot moved out in front of its door)
Prints one line per check, exits 1 when anything is wrong.
"""
import copy
import heapq
import math
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import helgasetr_plan as H  # noqa: E402

errors = 0


def report(ok, text):
    global errors
    print(("OK      " if ok else "FEIL    ") + text)
    if not ok:
        errors += 1


def counter(failed, text):
    """A counter-test passes when the check FAILED on the broken data."""
    report(failed, "motprove: " + text + (" blir avvist" if failed else " blir IKKE avvist"))


P4 = H.P4
Z4 = H.LEVELS[P4]


# ================================================================ 2D sight
def opaque_segments(G, lv, skip=()):
    segs = []
    for e in G["opaque"][lv]:
        tag = e[-1]
        if tag in skip:
            continue
        if e[0] == "BOX":
            x0, y0, x1, y1 = e[1:5]
            segs += [(x0, y0, x1, y0), (x1, y0, x1, y1), (x1, y1, x0, y1), (x0, y1, x0, y0)]
        else:
            o, c, a, b = e[:4]
            segs.append((c, a, c, b) if o == "V" else (a, c, b, c))
    return np.array(segs, dtype=float).reshape(-1, 4)


def blocked(S, P, Q):
    """For lines P[i] -> Q[i] (arrays (M, 2)): True where a segment in S crosses the line (strictly)."""
    out = np.zeros(len(P), dtype=bool)
    if len(S) == 0:
        return out
    ax, ay, bx, by = S[:, 0], S[:, 1], S[:, 2], S[:, 3]
    for k in range(0, len(P), 1500):
        p = P[k:k + 1500, None, :]
        q = Q[k:k + 1500, None, :]
        d1 = (q[..., 0] - p[..., 0]) * (ay - p[..., 1]) - (q[..., 1] - p[..., 1]) * (ax - p[..., 0])
        d2 = (q[..., 0] - p[..., 0]) * (by - p[..., 1]) - (q[..., 1] - p[..., 1]) * (bx - p[..., 0])
        d3 = (bx - ax) * (p[..., 1] - ay) - (by - ay) * (p[..., 0] - ax)
        d4 = (bx - ax) * (q[..., 1] - ay) - (by - ay) * (q[..., 0] - ax)
        hit = (d1 * d2 < -1e-12) & (d3 * d4 < -1e-12)
        out[k:k + 1500] = hit.any(axis=1)
    return out


def visible_pairs(S, A, B):
    """All pairs A x B; returns a boolean matrix (len A, len B) of free lines."""
    A = np.asarray(A, float).reshape(-1, 2)
    B = np.asarray(B, float).reshape(-1, 2)
    P = np.repeat(A, len(B), axis=0)
    Q = np.tile(B, (len(A), 1))
    return (~blocked(S, P, Q)).reshape(len(A), len(B))


def ray_length(S, P, D, maxlen=200.0):
    """Distance from P along unit direction D to the first segment (2D)."""
    P = np.asarray(P, float).reshape(-1, 2)
    D = np.asarray(D, float).reshape(-1, 2)
    out = np.full(len(P), maxlen)
    if len(S) == 0:
        return out
    ax, ay = S[:, 0], S[:, 1]
    ex, ey = S[:, 2] - ax, S[:, 3] - ay
    for i in range(len(P)):
        px, py = P[i]
        dx, dy = D[i]
        den = dx * ey - dy * ex
        with np.errstate(divide="ignore", invalid="ignore"):
            t = ((ax - px) * ey - (ay - py) * ex) / den
            u = ((ax - px) * dy - (ay - py) * dx) / den
        ok = (np.abs(den) > 1e-12) & (t > 1e-6) & (u >= 0) & (u <= 1)
        if ok.any():
            out[i] = min(maxlen, t[ok].min())
    return out


def seg_dist(S, x, y):
    if len(S) == 0:
        return 1e9
    ax, ay, bx, by = S[:, 0], S[:, 1], S[:, 2], S[:, 3]
    dx, dy = bx - ax, by - ay
    L2 = np.maximum(dx * dx + dy * dy, 1e-12)
    t = np.clip(((x - ax) * dx + (y - ay) * dy) / L2, 0, 1)
    return float(np.min(np.hypot(ax + t * dx - x, ay + t * dy - y)))


# ================================================================ 3D rays against boxes
def opaque_boxes3d(G, zmin, zmax):
    rows = []
    for b in G["boxes"]:
        if b["mat"] == "Glass" or b["coll"] in (H.C_NONE, H.C_BLOCKER) or b["pitch"] or b["roll"]:
            continue
        (cx, cy, cz), (sx, sy, sz) = b["c"], b["s"]
        if b["yaw"] not in (0.0,):
            continue
        z0, z1 = cz - sz / 2, cz + sz / 2
        if z1 < zmin or z0 > zmax:
            continue
        rows.append((cx - sx / 2, cy - sy / 2, z0, cx + sx / 2, cy + sy / 2, z1))
    for c in G["cover"]:
        rows.append(c["box"])
    for b in G["breach"]:
        rows.append(b["box"])
    return np.array(rows, float)


def blocked3d(Bx, P, Q):
    out = np.zeros(len(P), dtype=bool)
    lo, hi = Bx[:, :3], Bx[:, 3:]
    for k in range(0, len(P), 400):
        p = P[k:k + 400, None, :]
        d = Q[k:k + 400, None, :] - p
        with np.errstate(divide="ignore", invalid="ignore"):
            inv = 1.0 / np.where(np.abs(d) < 1e-12, 1e-12, d)
            t1 = (lo - p) * inv
            t2 = (hi - p) * inv
        tmin = np.max(np.minimum(t1, t2), axis=2)
        tmax = np.min(np.maximum(t1, t2), axis=2)
        hit = (tmax >= tmin) & (tmax > 0.02) & (tmin < 0.98)
        out[k:k + 400] = hit.any(axis=1)
    return out


# ================================================================ walk grid
CELL = 0.25
NXG = int(round((H.BX1 - H.BX0) / CELL))
NYG = int(round((H.BY1 - H.BY0) / CELL))
WALK_LEVELS = (H.P1, H.P2, H.P3, H.P4)


def cell_of(x, y):
    return int((x - H.BX0) / CELL), int((y - H.BY0) / CELL)


def walk_grid(P, G):
    """Per walkable level: boolean grid of cells a body (r 0.3 m) can stand in, and the stair links between levels."""
    xs = H.BX0 + (np.arange(NXG) + 0.5) * CELL
    ys = H.BY0 + (np.arange(NYG) + 0.5) * CELL
    X, Y = np.meshgrid(xs, ys, indexing="ij")
    grids = {}
    for lv in WALK_LEVELS:
        g = np.zeros((NXG, NYG), dtype=bool)
        for (x0, y0, x1, y1) in G["floors"][lv]:
            g |= (X > x0) & (X < x1) & (Y > y0) & (Y < y1)
        r = 0.3
        # every wall, rail and glass piece blocks (solid intervals); screens, breach walls, cover, columns
        segs = []
        for w in G["walls"]:
            if w["lv"] != lv:
                continue
            for a, b, h in w["solid"]:
                if h is not None and h >= 2.0:
                    continue   # door hole with lintel
                segs.append((w["o"], w["c"], a, b, w["th"]))
        for e in G["opaque"][lv]:
            if e[0] == "BOX":
                x0, y0, x1, y1 = e[1:5]
                g &= ~((X > x0 - r) & (X < x1 + r) & (Y > y0 - r) & (Y < y1 + r))
            elif e[-1] in ("screen", "breach"):
                segs.append((e[0], e[1], e[2], e[3], 0.1))
        for c in G["cover"]:
            if c["lv"] == lv:
                x0, y0, z0, x1, y1, z1 = c["box"]
                g &= ~((X > x0 - r) & (X < x1 + r) & (Y > y0 - r) & (Y < y1 + r))
        for (o, c, a, b, th) in segs:
            if o == "V":
                g &= ~((np.abs(X - c) < th / 2 + r) & (Y > a - r) & (Y < b + r))
            else:
                g &= ~((np.abs(Y - c) < th / 2 + r) & (X > a - r) & (X < b + r))
        grids[lv] = g
    # stairs: block their footprint on the level they stand on, and link bottom -> top
    links = []
    flights = {}
    for s in P["stairs"]:
        flights[s["name"]] = s
    for s in P["stairs"]:
        x0 = min(s["s0"], s["s1"]) if s["axis"] == "x" else s["c0"]
        x1 = max(s["s0"], s["s1"]) if s["axis"] == "x" else s["c1"]
        y0 = min(s["s0"], s["s1"]) if s["axis"] == "y" else s["c0"]
        y1 = max(s["s0"], s["s1"]) if s["axis"] == "y" else s["c1"]
        lv0 = min(range(6), key=lambda k: abs(H.LEVELS[k] - s["z0"]))
        if abs(H.LEVELS[lv0] - s["z0"]) < 0.01 and lv0 in grids:
            grids[lv0] &= ~((X > x0 - 0.05) & (X < x1 + 0.05) & (Y > y0 - 0.05) & (Y < y1 + 0.05))
    cm = lambda s: (s["c0"] + s["c1"]) / 2  # noqa: E731

    def endpoint(s, at_start, out=0.45):
        d = s["dir"]
        if at_start:
            v = s["s0"] - d * out
        else:
            v = s["s1"] + d * out
        return (cm(s), v) if s["axis"] == "y" else (v, cm(s))

    for s in P["stairs"]:
        if s.get("well"):
            continue
        lv0 = H.LEVELS.index(s["z0"]) if s["z0"] in H.LEVELS else None
        lv1 = H.LEVELS.index(s["z1"]) if s["z1"] in H.LEVELS else None
        if lv0 is None or lv1 is None:
            continue
        a, b = endpoint(s, True), endpoint(s, False)
        links.append((lv0, a, lv1, b, abs(s["s1"] - s["s0"]) + abs(s["z1"] - s["z0"]), s["name"]))
    for lv in (H.P1, H.P2, H.P3):
        for well in ("Trapp sør", "Trapp øst"):
            f1 = flights["%s %d-1" % (well, lv + 1)]
            f2 = flights["%s %d-2" % (well, lv + 1)]
            a, b = endpoint(f1, True), endpoint(f2, False)
            cost = 2 * abs(f1["s1"] - f1["s0"]) + 2.0 + (H.LEVELS[lv + 1] - H.LEVELS[lv])
            links.append((lv, a, lv + 1, b, cost, well))
    return grids, links


class Walker:
    def __init__(self, P, G):
        self.grids, self.links = walk_grid(P, G)
        self.levels = list(self.grids.keys())
        self.n = NXG * NYG

    def node(self, lv, i, j):
        return self.levels.index(lv) * self.n + i * NYG + j

    def nearest(self, lv, x, y, mask=None):
        g = self.grids[lv] if mask is None else self.grids[lv] & mask[lv]
        i, j = cell_of(x, y)
        for rad in range(0, 12):
            best = None
            for di in range(-rad, rad + 1):
                for dj in range(-rad, rad + 1):
                    ii, jj = i + di, j + dj
                    if 0 <= ii < NXG and 0 <= jj < NYG and g[ii, jj]:
                        dd = di * di + dj * dj
                        if best is None or dd < best[0]:
                            best = (dd, ii, jj)
            if best:
                return best[1], best[2]
        return None

    def dijkstra(self, sources, mask=None):
        """sources: [(lv, x, y)]. Returns {lv: dist grid}. mask: {lv: bool grid} of allowed cells (None = all)."""
        dist = {lv: np.full((NXG, NYG), np.inf) for lv in self.levels}
        ok = {lv: (self.grids[lv] if mask is None or lv not in mask else self.grids[lv] & mask[lv]) for lv in self.levels}
        heap = []
        for lv, x, y in sources:
            c = self.nearest(lv, x, y, mask)
            if c:
                dist[lv][c] = 0.0
                heapq.heappush(heap, (0.0, lv, c[0], c[1]))
        linkmap = {}
        for (la, a, lb, b, cost, name) in self.links:
            ca, cb = self.nearest(la, *a, mask), self.nearest(lb, *b, mask)
            if ca and cb:
                linkmap.setdefault((la, ca), []).append((lb, cb, cost))
                linkmap.setdefault((lb, cb), []).append((la, ca, cost))
        nb = [(-1, 0, 1.0), (1, 0, 1.0), (0, -1, 1.0), (0, 1, 1.0), (-1, -1, 1.4142), (1, 1, 1.4142), (-1, 1, 1.4142), (1, -1, 1.4142)]
        while heap:
            d, lv, i, j = heapq.heappop(heap)
            if d > dist[lv][i, j]:
                continue
            g = ok[lv]
            for di, dj, w in nb:
                ii, jj = i + di, j + dj
                if 0 <= ii < NXG and 0 <= jj < NYG and g[ii, jj]:
                    if di and dj and not (g[i + di, j] and g[i, j + dj]):
                        continue
                    nd = d + w * CELL
                    if nd < dist[lv][ii, jj]:
                        dist[lv][ii, jj] = nd
                        heapq.heappush(heap, (nd, lv, ii, jj))
            for (lb, cb, cost) in linkmap.get((lv, (i, j)), ()):
                nd = d + cost
                if nd < dist[lb][cb]:
                    dist[lb][cb] = nd
                    heapq.heappush(heap, (nd, lb, cb[0], cb[1]))
        return dist

    def at(self, dist, lv, x, y):
        c = self.nearest(lv, x, y)
        return dist[lv][c] if c else np.inf


# ================================================================ helpers on the plan
def room(P, name, lv):
    return next(r for r in P["rooms"] if r.name == name and r.lv == lv)


def rooms_at(P, lv, x, y):
    return [r for r in P["rooms"] if r.lv == lv and r.contains(x, y)]


def ring_side(x, y):
    ri, ro = H.RING_IN, H.RING_OUT
    if abs(x) > ro[0] or abs(y) > ro[1]:
        return None
    if y <= -ri[1]:
        return "N"
    if y >= ri[1]:
        return "S"
    if x <= -ri[0]:
        return "W"
    if x >= ri[0]:
        return "E"
    return None


def ring_points(step=0.5, offset=0.0):
    """Midline of the Plan 4 ring corridor (and +offset towards the outer wall)."""
    m = (H.RING_IN[0] + H.RING_OUT[0]) / 2 + offset
    my = (H.RING_IN[1] + H.RING_OUT[1]) / 2 + offset
    pts = []
    for y in np.arange(-my, my + 1e-6, step):
        pts += [(-m, y), (m, y)]
    for x in np.arange(-m, m + 1e-6, step):
        pts += [(x, -my), (x, my)]
    return pts


def grid_in(r, step=1.0, margin=0.5):
    pts = []
    for x in np.arange(r.x0 + margin, r.x1 - margin + 1e-6, step):
        for y in np.arange(r.y0 + margin, r.y1 - margin + 1e-6, step):
            pts.append((x, y))
    return pts


# ================================================================ the checks
def check_a(P, G, quiet=False):
    ok = True
    sx = H.AX["10"] - H.AX["1"]
    sy = H.AY["A"] - H.AY["N"]
    ok &= abs(sx - 51.50) < 0.01 and abs(sy - 67.77) < 0.01
    hs = [round(b - a, 2) for a, b in zip(H.LEVELS, H.LEVELS[1:] + [H.ROOF_Z])]
    ok &= hs == [5.6, 4.3, 4.0, 4.3, 4.0, 4.0]
    if not quiet:
        report(ok, "a) aksesummer %.2f / %.2f, etasjehøyder %s" % (sx, sy, hs))
    return ok


def check_b(P, G, quiet=False):
    S, N = P["bases"]
    msgs = []
    ok = abs(S["flag"][2] - N["flag"][2]) < 0.01
    rs, rn = room(P, S["room"], P4), room(P, N["room"], P4)
    ok &= rn.y0 <= -30.0 and rs.y1 >= 29.0
    dy = abs(S["flag"][1] - N["flag"][1])
    ok &= dy >= 45.0
    for b, r in ((S, rs), (N, rn)):
        fx, fy, _ = b["flag"]
        dmin = min(fx - r.x0, r.x1 - fx, fy - r.y0, r.y1 - fy)
        if dmin < 2.0:
            ok = False
            msgs.append("flagget %s %.1f m fra veggen" % (b["name"], dmin))
        doors = [d for d in r.doors if abs(d[1] - (r.y0 if b is S else r.y1)) < 0.02]
        if len(doors) != 2:
            ok = False
            msgs.append("%s har %d dører mot korridoren" % (b["name"], len(doors)))
    sym = lambda p, q: abs(p[0] + q[0]) <= 0.1 and abs(p[1] + q[1]) <= 0.1  # noqa: E731
    ok &= sym(S["flag"], N["flag"])
    ok &= all(any(sym(a, b) for b in N["spawns"]) for a in S["spawns"])
    ok &= all(any(sym(a[:2], b[:2]) for b in rn.doors) for a in rs.doors)
    if not quiet:
        report(ok, "b) basene på samme Z i hver sin ende, %.1f m mellom flaggene, flagg >= 2 m fra veggen, to dører hver, punktsymmetriske %s" % (dy, "; ".join(msgs)))
    return ok


def spawn_observers(P, G):
    pts = ring_points(0.5) + ring_points(0.5, 0.8) + ring_points(0.5, -0.8)
    for r in P["rooms"]:
        if r.lv != P4:
            continue
        for (x0, y0, x1, y1) in r.openings:   # lobby openings and open sides: along them and 1 m out on both sides
            for t in np.linspace(0, 1, 9):
                x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
                if abs(x0 - x1) < 1e-6:
                    pts += [(x - 1.0, y), (x + 1.0, y), (x, y)]
                else:
                    pts += [(x, y - 1.0), (x, y + 1.0), (x, y)]
        if r.name in ("Venterom", "NØ-gang", "Audiologi 4.056", "Psykoakustisk lab", "Audiologi 4.057", "Treningsleiligheten",
                      "Ferdighetsrom vernepleier", "Ferdighetsrom radiografi", "Trapp øst-lobby", "Sør-lobby", "Nord-lobby"):
            pts += grid_in(r, 0.7, 0.4)
    for e in stair_exits(P, G):
        pts.append(e[1])
    return pts


def check_c(P, G, quiet=False):
    ok = True
    msgs = []
    S = opaque_segments(G, P4)
    for b in P["bases"]:
        rs = [room(P, n, P4) for n in b["spawn_rooms"]]
        for (x, y) in b["spawns"]:
            r = next((r for r in rs if r.contains(x, y)), None)
            if not r or min(x - r.x0, r.x1 - x, y - r.y0, r.y1 - y) < 0.6:
                ok = False
                msgs.append("spawn (%.1f, %.1f) ikke inne i eget rom" % (x, y))
        Sc = np.array([e for e in opaque_segments(G, P4)], float)
        for (x, y) in b["spawns"]:
            if seg_dist(Sc, x, y) < 0.6:
                ok = False
                msgs.append("spawn (%.1f, %.1f) under 0,6 m fra en vegg eller skjerm" % (x, y))
        for i, p in enumerate(b["spawns"]):
            for q in b["spawns"][i + 1:]:
                if math.dist(p, q) < 1.5:
                    ok = False
                    msgs.append("spawn %s og %s for tett" % (p, q))
    obs = spawn_observers(P, G)
    allsp = [s for b in P["bases"] for s in b["spawns"]]
    V = visible_pairs(S, obs, allsp)
    seen = V.any(axis=0)
    if seen.any():
        ok = False
        for k in np.nonzero(seen)[0][:4]:
            o = obs[int(np.nonzero(V[:, k])[0][0])]
            msgs.append("spawn (%.1f, %.1f) ses fra (%.1f, %.1f)" % (allsp[k][0], allsp[k][1], o[0], o[1]))
    if not quiet:
        report(ok, "c) spawn i eget rom, >= 0,6 m fra vegg og skjerm, >= 1,5 m mellom, usett fra %d punkter i ringen, lobbyene, sluser, trapper og R2/R4 %s"
               % (len(obs), "; ".join(msgs)))
    return ok


def side_mask(P, sides, extra_rooms=(), ring_bands=None, allow_levels=(P4,), all_other_levels=False):
    """Allowed cells: the two bases, the N and S ring sides, the given ring sides (optionally only in Y bands),
    and the named rooms on Plan 4. Other levels: all or none."""
    xs = H.BX0 + (np.arange(NXG) + 0.5) * CELL
    ys = H.BY0 + (np.arange(NYG) + 0.5) * CELL
    X, Y = np.meshgrid(xs, ys, indexing="ij")
    m = np.zeros((NXG, NYG), dtype=bool)
    for name in ("Sørbasen", "Nordbasen"):
        r = room(P, name, P4)
        m |= (X > r.x0) & (X < r.x1) & (Y > r.y0) & (Y < r.y1)
    ri, ro = H.RING_IN, H.RING_OUT
    inring = (np.abs(X) < ro[0]) & (np.abs(Y) < ro[1]) & ~((np.abs(X) < ri[0]) & (np.abs(Y) < ri[1]))
    m |= inring & (np.abs(Y) >= ri[1])
    for sd in sides:
        part = inring & (np.abs(Y) < ri[1]) & ((X < 0) if sd == "W" else (X > 0))
        if ring_bands and sd in ring_bands:
            band = np.zeros_like(part)
            for (a, b) in ring_bands[sd]:
                band |= (Y >= a) & (Y <= b)
            part &= band
        m |= part
    for name in extra_rooms:
        if isinstance(name, tuple):
            x0, y0, x1, y1 = name
            m |= (X > x0) & (X < x1) & (Y > y0) & (Y < y1)
        else:
            r = room(P, name, P4)
            m |= (X > r.x0 - 0.3) & (X < r.x1 + 0.3) & (Y > r.y0 - 0.3) & (Y < r.y1 + 0.3)
    full = np.ones((NXG, NYG), dtype=bool)
    none = np.zeros((NXG, NYG), dtype=bool)
    mask = {P4: m}
    for lv in (H.P1, H.P2, H.P3):
        mask[lv] = full if all_other_levels else none
    return mask


def routes(P, G, W):
    S, N = P["bases"]
    sf, nf = S["flag"], N["flag"]
    out = {}
    defs = {
        "R1 vestkorridoren": side_mask(P, "W"),
        "R2 laberadene": side_mask(P, "W", ("Venterom", "Audiologi 4.056", "Psykoakustisk lab", "Audiologi 4.057"),
                                   ring_bands={"W": [(-20.05, -4.0), (14.0, 20.05)]}),
        "R3 østkorridoren": side_mask(P, "E"),
        "R4 østfløyens rom": side_mask(P, "E", ("Treningsleiligheten", "Ferdighetsrom vernepleier", "Ferdighetsrom radiografi"),
                                       ring_bands={"E": [(-20.05, 0.6), (15.0, 20.05)]}),
        "R5 gjennom atriet": side_mask(P, "E", ((9.8, -4.3, 13.25, -1.8), "Trapp øst-lobby"),
                                       ring_bands={"E": [(-20.05, -7.0), (-4.4, 20.05)]}, all_other_levels=True),
    }
    for name, mask in defs.items():
        if name.startswith("R5"):
            mask[H.P1] = np.zeros_like(mask[H.P1])
        d = W.dijkstra([(P4, sf[0], sf[1])], mask)
        out[name] = W.at(d, P4, nf[0], nf[1])
    # R6: the shortest way that touches Plan 1
    dS = W.dijkstra([(P4, sf[0], sf[1])])
    dN = W.dijkstra([(P4, nf[0], nf[1])])
    out["R6 kantina"] = float(np.min(dS[H.P1] + dN[H.P1]))
    return out, dS, dN


def check_d(P, G, quiet=False, W=None):
    W = W or Walker(P, G)
    rt, dS, dN = routes(P, G, W)
    walk = [k for k, v in rt.items() if np.isfinite(v)]
    ok = len(walk) >= 4 and np.isfinite(rt["R6 kantina"])
    S, N = P["bases"]
    red = min(W.at(dN, P4, x, y) for x, y in S["spawns"])     # red spawn -> blue flag
    blue = min(W.at(dS, P4, x, y) for x, y in N["spawns"])
    bal = abs(red - blue) / max(red, blue) if np.isfinite(red) and np.isfinite(blue) else 9
    ok &= bal <= 0.2
    if not quiet:
        report(ok, "d) %d ruter kan gås (%s), via kantina: %s; spawn -> fiendens flagg rødt %.0f m / blått %.0f m (%.0f %%)" % (
            len(walk), ", ".join("%s %.0f m" % (k, v) for k, v in rt.items() if np.isfinite(v)),
            "ja" if np.isfinite(rt["R6 kantina"]) else "nei", red, blue, bal * 100))
    return ok


def stair_exits(P, G):
    """(name, point on Plan 4 where you come out) for every stair or stair well that lands on Plan 4 with an open door."""
    ex = []
    for d in G["doors"]:
        if d["lv"] == P4 and d["room"].startswith("Trapp") and not d["locked"]:
            r = room(P, d["room"], P4)
            # one step out of the stair room
            if d["o"] == "V":
                x = d["x"] - 0.6 if abs(d["x"] - r.x0) < 0.02 else d["x"] + 0.6
                ex.append((d["room"], (x, d["y"])))
            else:
                y = d["y"] - 0.6 if abs(d["y"] - r.y0) < 0.02 else d["y"] + 0.6
                ex.append((d["room"], (d["x"], y)))
    for s in P["stairs"]:
        if abs(s["z1"] - Z4) < 0.01 and not s.get("well"):
            c = (s["c0"] + s["c1"]) / 2
            v = s["s1"] + s["dir"] * 0.6
            ex.append((s["name"], (c, v) if s["axis"] == "y" else (v, c)))
    return ex


def check_d2(P, G, quiet=False, W=None):
    W = W or Walker(P, G)
    S, N = P["bases"]
    exits = stair_exits(P, G)
    dS = W.dijkstra([(P4, S["flag"][0], S["flag"][1])])
    dN = W.dijkstra([(P4, N["flag"][0], N["flag"][1])])
    red = min(W.at(dN, P4, *e[1]) for e in exits)    # red attacks the north flag from the best exit
    blue = min(W.at(dS, P4, *e[1]) for e in exits)
    ok = np.isfinite(red) and np.isfinite(blue) and abs(red - blue) / max(red, blue) <= 0.2
    msgs = []
    Sg = opaque_segments(G, P4)
    for b in P["bases"]:
        for sx, sy in b["spawn_exits"]:
            for name, (x, y) in exits:
                dd = math.dist((sx, sy), (x, y))
                pts = [(x + dx, y + dy) for dx in (-0.8, 0, 0.8) for dy in (-0.8, 0, 0.8)]
                seen = visible_pairs(Sg, pts, [(sx, sy)]).any()
                if dd < 12.0 or seen:
                    ok = False
                    msgs.append("%s %.1f m fra spawnutgangen (%.1f, %.1f)%s" % (name, dd, sx, sy, ", i sikt" if seen else ""))
    if not quiet:
        report(ok, "d2) trappeutganger på 4. etasje (%s): beste vei til fiendens flagg rødt %.0f m / blått %.0f m; ingen nærmere enn 12 m eller i sikt av en spawnutgang %s" % (
            ", ".join(e[0] for e in exits), red, blue, "; ".join(msgs)))
    return ok


def route_rays(P):
    """(points, directions) along R1-R4 and the N/S ring: middle and +-1.0 m."""
    pts, dirs = [], []
    m = (H.RING_IN[0] + H.RING_OUT[0]) / 2
    my = (H.RING_IN[1] + H.RING_OUT[1]) / 2
    for off in (-1.0, 0.0, 1.0):
        for y in np.arange(-my, my + 1e-6, 2.0):
            for x in (-m + off, m + off):
                pts += [(x, y), (x, y)]
                dirs += [(0, 1), (0, -1)]
        for x in np.arange(-m, m + 1e-6, 2.0):
            for y in (-my + off, my + off):
                pts += [(x, y), (x, y)]
                dirs += [(1, 0), (-1, 0)]
    for xx in (-20.5, 20.5, -22.0, 22.0):   # R2 / R4 room chains
        for y in np.arange(-11.0, 20.0, 2.0):
            pts += [(xx, y), (xx, y)]
            dirs += [(0, 1), (0, -1)]
    return pts, dirs


def check_e(P, G, quiet=False):
    S = opaque_segments(G, P4)
    pts, dirs = route_rays(P)
    L = ray_length(S, pts, dirs)
    worst = int(np.argmax(L))
    ok = L.max() <= 42.0
    if not quiet:
        report(ok, "e) lengste frie sikt langs rutene (midt og +-1,0 m, %d stråler): %.1f m fra (%.1f, %.1f) <= 42 m" % (
            len(pts), L.max(), pts[worst][0], pts[worst][1]))
    return ok


def base_points(P, b):
    r = room(P, b["room"], P4)
    fx, fy, _ = b["flag"]
    pts = [(fx + dx, fy + dy) for dx in np.arange(-2, 2.01, 0.5) for dy in np.arange(-2, 2.01, 0.5)]
    for d in r.doors:
        inward = 0.3 if abs(d[1] - r.y0) < 0.02 else -0.3
        for dx in (-0.4, 0.0, 0.4):
            pts.append((d[0] + dx, d[1] + inward))
    return pts


def check_e2(P, G, quiet=False, skip=()):
    S = opaque_segments(G, P4, skip)
    A = base_points(P, P["bases"][0])
    B = base_points(P, P["bases"][1])
    V = visible_pairs(S, A, B)
    D = np.array([[math.dist(a, b) for b in B] for a in A])
    bad = V & (D > 42.0)
    ok = not bad.any()
    if not quiet:
        msg = ""
        if bad.any():
            i, j = np.argwhere(bad)[0]
            msg = ": fri linje %.1f m fra (%.1f, %.1f) til (%.1f, %.1f)" % (D[i, j], A[i][0], A[i][1], B[j][0], B[j][1])
        report(ok, "e2) ingen fri linje over 42 m fra base til base (%d x %d punkter)%s" % (len(A), len(B), msg))
    return ok, bad.any()


def pockets(P, G, side, skip=(), cover=True):
    """Y positions on R1 (W) / R3 (E) with a cover pocket (see the top of the file)."""
    S = opaque_segments(G, P4, skip)
    sgn = -1 if side == "W" else 1
    m = (H.RING_IN[0] + H.RING_OUT[0]) / 2
    opp = [(-sgn * m, y) for y in np.arange(-17.5, 17.6, 1.0)]
    # Plan 3 ring observers (3D), eye 1.6 m; targets crouched 1.0 m over the Plan 4 floor
    p3 = [(x, y) for (x, y) in ring_points(2.0)]
    Bx = opaque_boxes3d(G if cover else G, H.LEVELS[H.P3], Z4 + 2.0)
    found = []
    for y in np.arange(-17.5, 17.51, 0.5):
        good = False
        for x in sgn * np.arange(13.7, 15.21, 0.25):
            trip = [(x, y - 0.75), (x, y), (x, y + 0.75)]
            if any(seg_dist(S, px, py) < 0.4 for px, py in trip):
                continue
            if visible_pairs(S, opp, trip).any():
                continue
            Pp = np.array([(ox, oy, H.LEVELS[H.P3] + 1.6) for ox, oy in p3 for _ in trip])
            Qq = np.array([(tx, ty, Z4 + 1.0) for _ in p3 for tx, ty in trip])
            if (~blocked3d(Bx, Pp, Qq)).any():
                continue
            good = True
            break
        if good:
            found.append(y)
    return found


def gaps_ok(found, lo=-17.75, hi=17.75, gap=8.0):
    if not found:
        return False, hi - lo
    pts = [lo] + sorted(found) + [hi]
    g = max(b - a for a, b in zip(pts, pts[1:]))
    return g <= gap, g


def check_e3(P, G, quiet=False, skip=(), Gc=None):
    ok = True
    parts = []
    for side, name in (("W", "R1"), ("E", "R3")):
        f = pockets(P, Gc or G, side, skip)
        g_ok, g = gaps_ok(f)
        ok &= g_ok
        parts.append("%s: %d lommer, største hull %.1f m" % (name, len(f), g))
    if not quiet:
        report(ok, "e3) dekningslomme minst hver 8. m langs R1/R3 (skjult for motsatt ringside og for 3. etasje): " + "; ".join(parts))
    return ok


def on_floor(G, lv, x, y):
    return any(x0 < x < x1 and y0 < y < y1 for (x0, y0, x1, y1) in G["floors"][lv])


def lv_of(z):
    return max(k for k in range(6) if H.LEVELS[k] <= z + 0.3)


def check_f(P, G, quiet=False):
    bad = []
    items = [("loot", x, y, z) for (x, y, z, t) in P["loot"]] + [("terminal " + n, x, y, z) for (x, y, z, yaw, k, n) in P["terminals"]]
    items += [("kamera " + n, x, y, z) for (x, y, z, yaw, n) in P["cameras"]]
    for b in P["bases"]:
        items.append(("flagg " + b["name"], b["flag"][0], b["flag"][1], b["flag"][2]))
        items += [("spawn", x, y, b["flag"][2]) for (x, y) in b["spawns"]]
    for name, x, y, z in items:
        if not on_floor(G, lv_of(z), x, y):
            bad.append("%s (%.1f, %.1f, %.1f)" % (name, x, y, z))
    # no loot in base / spawn rooms, lobbies or sluices
    forbidden = {"Sørbasen", "Nordbasen", "Sør-spawngang", "Nord-spawngang", "Venterom", "NØ-gang"}
    for b in P["bases"]:
        forbidden.update(b["spawn_rooms"])
    for (x, y, z, t) in P["loot"]:
        for r in rooms_at(P, lv_of(z), x, y):
            if r.name in forbidden or r.kind == "lobby":
                bad.append("loot i %s" % r.name)
    # loot and terminals not inside cover, columns or walls (0.3 m round them)
    for (x, y, z, *rest) in [(l[0], l[1], l[2]) for l in P["loot"]] + [(t[0], t[1], t[2]) for t in P["terminals"]]:
        lv = lv_of(z)
        if seg_dist(opaque_segments(G, lv), x, y) < 0.3:
            bad.append("(%.1f, %.1f, %.1f) inntil en vegg" % (x, y, z))
        for e in G["opaque"][lv]:
            if e[0] == "BOX" and e[1] - 0.3 < x < e[3] + 0.3 and e[2] - 0.3 < y < e[4] + 0.3:
                bad.append("(%.1f, %.1f, %.1f) inne i %s" % (x, y, z, e[-1]))
        for c in G["cover"]:
            bx = c["box"]
            if c["lv"] == lv and bx[0] - 0.3 < x < bx[3] + 0.3 and bx[1] - 0.3 < y < bx[4] + 0.3:
                bad.append("(%.1f, %.1f, %.1f) inne i %s" % (x, y, z, c["name"]))
    ok = not bad
    if not quiet:
        report(ok, "f) %d loot, %d terminaler, %d kameraer, flagg og spawn står over gulv på sitt nivå, ingen loot i base-/spawnrom, lobbyer, sluser, dekning eller søyler %s" % (
            len(P["loot"]), len(P["terminals"]), len(P["cameras"]), "; ".join(bad[:5])))
    return ok


def check_g(P, G, quiet=False):
    bad = []
    for b in G["breach"]:
        if b["kind"] != "inner":
            bad.append("lettvegg %d i %s-vegg" % (b["id"], b["kind"]))
        mx, my = (b["c"], (b["a"] + b["b"]) / 2) if b["o"] == "V" else ((b["a"] + b["b"]) / 2, b["c"])
        for dx, dy in ((0.3, 0), (-0.3, 0), (0, 0.3), (0, -0.3)):
            for r in rooms_at(P, b["lv"], mx + dx, my + dy):
                if r.name in ("Sørbasen", "Nordbasen") or r.kind in ("trapp", "heis"):
                    bad.append("lettvegg %d inntil %s" % (b["id"], r.name))
        if not any(o["lv"] == b["lv"] and abs(o["c"] + b["c"]) < 0.1 and abs(o["a"] + b["b"]) < 0.1 and abs(o["b"] + b["a"]) < 0.1 for o in G["breach"]):
            bad.append("lettvegg %d har ikke motstykke" % b["id"])
    ok = not bad and len(G["breach"]) > 0
    if not quiet:
        report(ok, "g) %d lettvegger, ingen i bærende vegger, trapperom eller baserom, punktsymmetriske %s" % (len(G["breach"]), "; ".join(sorted(set(bad))[:4])))
    return ok


def check_h(P, G, quiet=False):
    pa = P["play_area"]
    ok = all(pa[0] <= x0 and x1 <= pa[2] and pa[1] <= y0 and y1 <= pa[3] for lv in range(6) for (x0, y0, x1, y1) in G["floors"][lv])
    ok &= pa[0] <= H.BX0 - H.WALL_OUT / 2 and H.BX1 + H.WALL_OUT / 2 <= pa[2] and pa[1] <= H.BY0 - H.WALL_OUT / 2 and H.BY1 + H.WALL_OUT / 2 <= pa[3]
    if not quiet:
        report(ok, "h) hele bygget (med ytterveggene) ligger inne i spillområdet %s" % (pa,))
    return ok


def check_i(P, G, quiet=False):
    bad = []
    surf = []   # (x0, y0, x1, y1, top) of everything that could be under a step
    for lv in range(6):
        for (x0, y0, x1, y1) in G["floors"][lv]:
            surf.append((x0, y0, x1, y1, H.LEVELS[lv]))
    for s in G["steps"]:
        surf.append(s["rect"] + (s["ztop"],))
    for k, s in enumerate(G["steps"]):
        if s["filled"]:
            continue
        x0, y0, x1, y1 = s["rect"]
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        under = [t for (a, b, c, d, t) in surf if a < cx < c and b < cy < d and t <= s["zbot"] + 1e-3]
        top = max(under) if under else -99
        clear = s["zbot"] - top
        if 0.02 < clear < 2.0:
            bad.append("%s (%.1f, %.1f): %.2f m fri høyde" % (s["stair"], cx, cy, clear))
    # gaps between atrium runs and slab edges
    for st in P["stairs"]:
        if not st.get("atrium"):
            continue
        for lv in range(6):
            if not (st["z0"] - 0.01 <= H.LEVELS[lv] <= st["z1"] + 0.01):
                continue
            lo, hi = min(st["s0"], st["s1"]), max(st["s0"], st["s1"])
            for side in (st["c0"], st["c1"]):
                for (o, c, a, b) in G["boundary"][lv]:
                    if (o == "V") != (st["axis"] == "y"):
                        continue
                    if b <= lo or a >= hi:
                        continue
                    gap = abs(c - side)
                    if 0.3 <= gap <= 0.9:
                        bad.append("%s: %.2f m spalte mot dekkekanten på %s" % (st["name"], gap, H.LEVEL_NAMES[lv]))
    ok = not bad
    if not quiet:
        report(ok, "i) under trappene: fri høyde >= 2,0 m eller massivt (%d trinn), ingen spalte 0,3-0,9 m mot dekkekanter %s" % (
            len(G["steps"]), "; ".join(bad[:4])))
    return ok


def check_j(P, G, quiet=False):
    bad = []
    ri = H.RING_IN
    for b in G["boxes"]:
        if b["coll"] != H.C_SOLID:
            continue
        (cx, cy, cz), (sx, sy, sz) = b["c"], b["s"]
        if b["pitch"] or b["roll"]:
            ext = max(sx, sy, sz) / 2
            hx, hy, z0 = ext, ext, cz - ext
        else:
            hx, hy = (sx / 2, sy / 2) if abs(b["yaw"]) % 180 < 1 else (sy / 2, sx / 2)
            z0 = cz - sz / 2
        if z0 < Z4 + 0.1:
            continue
        if b["bit"] == P4 and z0 + sz <= Z4 + H.BOX_H + 0.01 and not (b["pitch"] or b["roll"]):
            continue   # lintels in the Plan 4 box walls: the box roof sits right on them, nothing to stand on
        if cx - hx < ri[0] - 0.05 and cx + hx > -ri[0] + 0.05 and cy - hy < ri[1] - 0.05 and cy + hy > -ri[1] + 0.05:
            if b["bit"] == H.B_ROOF and (b["mat"] == "Glass" or b["tag"] == "tak"):
                continue   # the saw-tooth roof itself (glass and the opaque sheet faces) seals the atrium
            bad.append("%s %.1f/%.1f/%.1f" % (b["mat"], cx, cy, cz))
    for p in G["panels"]:
        if p["lv"] > P4:
            bad.append("glassfelt %d på %s" % (p["id"], H.LEVEL_NAMES[p["lv"]]))
    blockers = sum(1 for b in G["boxes"] if b["coll"] == H.C_BLOCKER)
    ok = not bad and blockers == 4
    if not quiet:
        report(ok, "j) ingenting over 4. etasje inne i atriet blokkerer spillere (unntatt glasstaket); %d usynlige vegger på 5.-6. etasje %s" % (
            blockers, "; ".join(bad[:4])))
    return ok


def check_j2(P, G, quiet=False):
    """No top a body could stand on high in the atrium: every wall, post or glass top over Plan 4 + 2.2 m inside the
    atrium has something that blocks bodies (Solid or High) right on top of it, within a body height (runde-04a funn 3)."""
    ri = H.RING_IN
    tops = []
    for b in G["boxes"]:
        if b["coll"] != H.C_SOLID or b["yaw"] or b["pitch"] or b["roll"] or b["bit"] in (H.B_ROOF, H.B_DETAIL):
            continue
        (cx, cy, cz), (sx, sy, sz) = b["c"], b["s"]
        tops.append((cx, cy, cz + sz / 2, "%s %.1f/%.1f" % (b["mat"], cx, cy)))
    for pn in G["panels"]:
        tops.append((pn["c"][0], pn["c"][1], pn["c"][2] + pn["h"] / 2, "glassfelt %d" % pn["id"]))
    covers = [b for b in G["boxes"] if b["coll"] in (H.C_SOLID, H.C_HIGH) and not (b["yaw"] or b["pitch"] or b["roll"])]
    bad = []
    for x, y, zt, name in tops:
        if not (Z4 + 2.2 <= zt < H.ROOF_Z - 1.0 and abs(x) < ri[0] + 0.05 and abs(y) < ri[1] + 0.05):
            continue
        ok = False
        for c in covers:
            (cx, cy, cz), (sx, sy, sz) = c["c"], c["s"]
            zb = cz - sz / 2
            if zt - 0.01 <= zb <= zt + 1.84 and abs(x - cx) <= sx / 2 + 0.01 and abs(y - cy) <= sy / 2 + 0.01:
                ok = True
                break
        if not ok:
            bad.append("%s (topp %.2f)" % (name, zt))
    if not quiet:
        report(not bad, "j2) ingen vegg-, stolpe- eller glasstopp over 4. etasje + 2,2 m i atriet uten noe som stopper kroppen rett over %s" % "; ".join(bad[:4]))
    return not bad


# ================================================================ k) id manifest: an id that exists never moves
IDS_FILE = os.path.join(HERE, "helgasetr_ids.json")


def id_keys(G):
    """What each id stands for (other teams key their state on the ids: Interaktiv, later Spillbarhet and Bots)."""
    r2 = lambda v: round(v, 2)
    return {
        "doors": [[d["id"], d["lv"], d["room"], r2(d["x"]), r2(d["y"])] for d in G["doors"]],
        "panels": [[p["id"], p["lv"], r2(p["c"][0]), r2(p["c"][1]), r2(p["c"][2])] for p in G["panels"]],
        "breach": [[b["id"], b["lv"], r2(b["box"][0]), r2(b["box"][1])] for b in G["breach"]],
        "cover": [[c["id"], c["lv"], c["name"]] for c in G["cover"]],
    }


def loot_keys(P):
    """Loot ids = index in LOOT (Spillbarhet's GameMode and Interaktiv key on them): [id, level, x, y]."""
    return [[i, lv_of(z), round(x, 2), round(y, 2)] for i, (x, y, z, t) in enumerate(P["loot"])]


def check_k(G, quiet=False, manifest=None, P=None):
    import json
    cur = id_keys(G)
    if P is not None:
        cur["loot"] = loot_keys(P)
    if manifest is None:
        if not os.path.exists(IDS_FILE) or ("--update-ids" in sys.argv and not quiet):   # never from a counter-test
            with open(IDS_FILE, "w", encoding="utf-8") as fh:
                json.dump(cur, fh, ensure_ascii=False, indent=0)
        with open(IDS_FILE, encoding="utf-8") as fh:
            manifest = json.load(fh)
    bad = []
    for kind, rows in manifest.items():
        now = {r[0]: r for r in cur.get(kind, [])}
        for r in rows:
            n = now.get(r[0])
            if kind == "loot" and n is not None and n[1] == r[1] and math.hypot(n[2] - r[2], n[3] - r[3]) <= 0.1 + 1e-6:
                continue   # a loot spot may be nudged by at most 0.1 m
            if n != r:
                bad.append("%s %d: %s -> %s" % (kind, r[0], r[1:], (n or ["mangler"])[1:]))
    if not quiet:
        report(not bad, "k) id-manifestet (%s): ingen eksisterende id for dører, glass, lettvegger, dekning eller loot har flyttet seg %s" % (
            os.path.basename(IDS_FILE), "; ".join(bad[:3])))
    return not bad


# ================================================================ l) loot spots free of walls, stairs and furniture
def ue_axes(pitch, yaw, roll):
    """The three local axes of a box rotated like FRotator(pitch, yaw, roll) (FRotationMatrix rows)."""
    sp, cp = math.sin(math.radians(pitch)), math.cos(math.radians(pitch))
    sy, cy = math.sin(math.radians(yaw)), math.cos(math.radians(yaw))
    sr, cr = math.sin(math.radians(roll)), math.cos(math.radians(roll))
    return ((cp * cy, cp * sy, sp),
            (sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp),
            (-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp))


def sphere_in_box(b, p, r):
    ax = ue_axes(b["pitch"], b["yaw"], b["roll"])
    d = [p[k] - b["c"][k] for k in range(3)]
    dist2 = 0.0
    for i in range(3):
        u = sum(d[k] * ax[i][k] for k in range(3))
        h = b["s"][i] / 2
        e = max(0.0, abs(u) - h)
        dist2 += e * e
    return dist2 < r * r


def check_l(P, G, quiet=False):
    bad = []
    solid = [b for b in G["boxes"] if b["coll"] == H.C_SOLID]
    for i, (x, y, z, t) in enumerate(P["loot"]):
        lv = lv_of(z)
        if not on_floor(G, lv, x, y):
            bad.append("loot %d (%.2f, %.2f, %.1f) uten gulv" % (i, x, y, z))
            continue
        # sphere r 0.15 m from 0.1 to 0.4 m over the floor (centre 0.25 m): where the pickup lies
        hit = next((b for b in solid if sphere_in_box(b, (x, y, z + 0.25), 0.15)), None)
        if hit:
            bad.append("loot %d (%.2f, %.2f, %.1f) inne i en massiv kloss %s (%.2f, %.2f, %.2f)" % (
                i, x, y, z, hit.get("tag") or "", hit["c"][0], hit["c"][1], hit["c"][2]))
        for c in G["cover"]:
            bx = c["box"]
            if bx[0] - 0.15 < x < bx[3] + 0.15 and bx[1] - 0.15 < y < bx[4] + 0.15 and bx[2] < z + 0.4 and bx[5] > z + 0.1:
                bad.append("loot %d inne i %s" % (i, c["name"]))
    for (i, lv, name) in P.get("loot_rooms", []):
        x, y, z, t = P["loot"][i]
        doors = [d for d in G["doors"] if d["lv"] == lv and d["room"] == name and d["kind"] == "glass_kode"]
        r = next((r for r in P["rooms"] if r.lv == lv and r.name == name), None)
        if not doors or r is None:
            bad.append("loot %d: fant ikke %s med kodelåsdør" % (i, name))
            continue
        d = doors[0]
        a = math.radians(d["yaw"])   # the way the door front faces = out of its room
        side = (x - d["x"]) * math.cos(a) + (y - d["y"]) * math.sin(a)
        if side >= -0.3 or not r.contains(x, y):
            bad.append("loot %d (%.2f, %.2f) ikke på romsiden av døra til %s (%.2f)" % (i, x, y, name, side))
        if min(x - r.x0, r.x1 - x, y - r.y0, r.y1 - y) < 0.6:
            bad.append("loot %d nærmere enn 0,6 m fra veggen i %s" % (i, name))
        for b in P["bases"]:
            if math.dist((x, y, z), b["flag"]) < 8.0:
                bad.append("loot %d nærmere enn 8 m fra flagget %s" % (i, b["name"]))
    if not quiet:
        report(not bad, "l) %d loot over gulv og fri av alle massive klosser og dekning (kule r 0,15 m, 0,1-0,4 m over gulvet); %d i låste grupperom på romsiden av døra %s" % (
            len(P["loot"]), len(P.get("loot_rooms", [])), "; ".join(bad[:4])))
    return not bad


# ================================================================ counter-tests
def variant(mutate):
    P = H.make_plan()
    mutate(P)
    return P, H.derive(P)


def main():
    P = H.make_plan()
    G = H.derive(P)
    report(not G["errors"], "data uten feil i utledningen (%d rom, %d klosser, %d glassfelt, %d dører) %s" % (
        len(P["rooms"]), len(G["boxes"]), len(G["panels"]), len(G["doors"]), "; ".join(G["errors"][:3])))
    r = subprocess.run([sys.executable, "-I", os.path.join(HERE, "gen_helgasetr.py"), "--check"], capture_output=True, text=True)
    report(r.returncode == 0, "gen_helgasetr.py --check: " + (r.stdout.strip() or r.stderr.strip()))

    # a
    check_a(P, G)
    old = list(H.LEVELS)
    H.LEVELS[1] = 5.5
    counter(not check_a(P, G, True), "P2 på 5,5 m")
    H.LEVELS[:] = old
    # b
    check_b(P, G)
    Pb = copy.deepcopy(P)
    Pb["bases"][1]["flag"] = (Pb["bases"][1]["flag"][0] + 1.0,) + Pb["bases"][1]["flag"][1:]
    counter(not check_b(Pb, G, True), "nordflagget 1 m til siden")
    # c
    check_c(P, G)
    Pc = copy.deepcopy(P)
    Pc["bases"][0]["spawns"][0] = (-24.6, 21.0)
    counter(not check_c(Pc, G, True), "spawnpunkt (-24,6, +21,0) rett bak døra fra S1 til venterommet")
    # d, d2 (walk grid)
    W = Walker(P, G)
    check_d(P, G, W=W)

    def block_west(Px):
        Px["rooms"].append(H.Room(P4, "Sperre", -15.55, -13.25, -0.5, 0.5, "lukket"))
        for rr in Px["rooms"]:
            if rr.lv == P4 and (rr.x1 <= -15.4 or rr.x0 >= 15.4):
                rr.doors = [(d[0], d[1], d[2], "laast") for d in rr.doors]
    Pd, Gd = variant(block_west)
    counter(not check_d(Pd, Gd, True), "vestkorridoren sperret og alle dører i vest- og østfløyen på 4. etasje låst")
    check_d2(P, G, W=W)

    def open_sor(Px):
        for rr in Px["rooms"]:
            if rr.lv == P4 and rr.name == "Trapp sør":
                rr.doors = [(d[0], d[1], d[2], "dobbel") for d in rr.doors]
    Pd2, Gd2 = variant(open_sor)
    counter(not check_d2(Pd2, Gd2, True), "døra fra Sør-lobbyen til Trapp sør åpen på 4. etasje")
    # e
    check_e(P, G)

    def open_spawn(Px):
        room(Px, "Sør-spawngang", P4).openings.append((-15.4, 20.05, -13.54, 20.05))
    Pe, Ge = variant(open_spawn)
    Pe["cover"] = [c for c in Pe["cover"] if not c[1].startswith("Skap R1")]
    Ge = H.derive(Pe)
    counter(not check_e(Pe, Ge, True), "veggen mellom Sør-spawngangen og ringen og skapene i R1 fjernet")
    # e2
    check_e2(P, G)
    ok2, _ = check_e2(P, G, True, skip=("screen", "light"))
    counter(not ok2, "L-skjermene og lettveggene på boksene fjernet")
    # e3
    check_e3(P, G)
    Pn = copy.deepcopy(P)
    Pn["cover"] = []
    Gn = H.derive(Pn)
    counter(not check_e3(Pn, Gn, True, skip=("cover", "light", "breach")), "all dekning og alle lettvegger fjernet")
    # f
    check_f(P, G)
    Pf = copy.deepcopy(P)
    Pf["loot"].append((0.0, 0.0, Z4, 0))
    counter(not check_f(Pf, G, True), "loot midt i atriet på 4. etasje")
    # g
    check_g(P, G)

    def breach_base(Px):
        Px["breach"].append((P4, -6.0, 20.05, -3.6, 20.05))
    Pg, Gg = variant(breach_base)
    counter(not check_g(Pg, Gg, True), "lettvegg i Sørbasens vegg")
    # h
    check_h(P, G)
    # i
    check_i(P, G)

    def nofill(Px):
        for st in Px["stairs"]:
            if st["name"] == "Løp 2":
                st["nofill"] = True
    Pi, Gi = variant(nofill)
    counter(not check_i(Pi, Gi, True), "fyllet under løp 2 fjernet")
    # j
    check_j(P, G)
    Gj = copy.deepcopy(G)
    Gj["boxes"].append(dict(c=(0.0, -15.0, H.LEVELS[H.P5] + 3.2), s=(6.0, 5.0, 0.45), yaw=0.0, pitch=0.0, roll=0.0,
                            mat="PlasterWhite_In", coll=H.C_SOLID, bit=H.P5, tag=""))
    counter(not check_j(P, Gj, True), "boksetak på 5. etasje som blokkerer spillere")
    # j2
    check_j2(P, G)
    Gj2 = copy.deepcopy(G)
    Gj2["boxes"] = [b for b in Gj2["boxes"] if not (b["bit"] == P4 and b["coll"] == H.C_HIGH and b["mat"] == "PlasterWhite_In")]
    counter(not check_j2(P, Gj2, True), "boksetakene på 4. etasje fjernet (veggtoppene blir et gulv)")
    # k
    check_k(G, P=P)

    def door_first(Px):
        for rr in Px["rooms"]:
            if rr.lv == H.P1 and rr.name == "Sør-lobby":
                rr.doors.insert(0, (-13.0, 26.9, 0.9, "tre"))
                break
    Pk, Gk = variant(door_first)
    counter(not check_k(Gk, True), "en ny dør først på 1. etasje (alle senere dør-id-er flytter seg)")
    Pk2 = copy.deepcopy(P)
    Pk2["loot"].insert(0, (0.0, -6.0, 0.0, 0))
    counter(not check_k(G, True, P=Pk2), "en ny lootplass først i lista (alle loot-id-er flytter seg)")
    # l
    check_l(P, G)
    Pl = copy.deepcopy(P)
    Pl["loot"].append((11.0, 3.0, H.LEVELS[H.P3], 0))
    counter(not check_l(Pl, G, True), "den gamle lootplassen (11,0, 3,0) i fyllet under løp 3 på 3. etasje")
    Pl2 = copy.deepcopy(P)
    if Pl2["loot_rooms"]:
        i, lv, name = Pl2["loot_rooms"][0]
        d = next(d for d in G["doors"] if d["lv"] == lv and d["room"] == name)
        a = math.radians(d["yaw"])
        Pl2["loot"][i] = (d["x"] + 1.2 * math.cos(a), d["y"] + 1.2 * math.sin(a), Pl2["loot"][i][2], 0)
    counter(not check_l(Pl2, G, True), "en lootplass i et låst rom flyttet 1,2 m ut foran døra")

    print()
    print("Alt OK" if errors == 0 else "%d FEIL" % errors)
    sys.exit(1 if errors else 0)


if __name__ == "__main__":
    main()
