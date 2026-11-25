"""Procedural, perfectly tileable PBR textures for Gloshaugen:  python3 Tools/Textures/gen_textures.py [Name ...]

Writes SourceArt/Textures/T_GS_<Name>_D.jpg (base colour), _N.jpg (normal, DirectX/Unreal convention)
and _ARM.jpg (R = ambient occlusion, G = roughness, B = metallic). Content/Python/init_unreal.py imports
them and builds the materials MI_GS_<Name>. Real scanned materials (Poly Haven, Megascans) still win
when they are in the project; these make sure nothing is ever a flat colour.

All noise is made in frequency space or with wrapped distances, so every texture tiles with no seams.
One texture covers 2 x 2 m unless the table below says otherwise.
"""
import os
import sys

import numpy as np
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "SourceArt", "Textures")
N = 1024


# ------------------------------------------------------------------ noise helpers (all tileable)
def fbm(rng, beta=2.0, lo=1.0, hi=None, n=N):
    """Fractal noise with a 1/f^beta spectrum, frequencies between lo and hi cycles per tile. Range 0..1."""
    fx = np.fft.fftfreq(n) * n
    fy = np.fft.rfftfreq(n) * n
    f = np.sqrt(fx[:, None] ** 2 + fy[None, :] ** 2)
    amp = np.zeros_like(f)
    mask = f >= lo
    if hi:
        mask &= f <= hi
    amp[mask] = f[mask] ** (-beta / 2.0)
    spec = amp * (rng.normal(size=f.shape) + 1j * rng.normal(size=f.shape))
    img = np.fft.irfft2(spec, s=(n, n))
    img -= img.min()
    img /= max(img.max(), 1e-9)
    return img


def grid(n=N):
    y, x = np.mgrid[0:n, 0:n].astype(np.float32)
    return x / n, y / n  # 0..1


def voronoi(rng, cells, n=N, jitter=0.9):
    """Wrapped Voronoi on a cells x cells grid. Returns (F1 distance, F2-F1 edge distance, cell id), distances in cell units."""
    pts = (np.stack(np.meshgrid(np.arange(cells), np.arange(cells)), -1).reshape(-1, 2)
           + 0.5 + (rng.random((cells * cells, 2)) - 0.5) * jitter)
    x, y = grid(n)
    x = x * cells
    y = y * cells
    f1 = np.full((n, n), 1e9, np.float32)
    f2 = np.full((n, n), 1e9, np.float32)
    cid = np.zeros((n, n), np.int32)
    for i, (px, py) in enumerate(pts):
        dx = np.abs(x - px)
        dx = np.minimum(dx, cells - dx)
        dy = np.abs(y - py)
        dy = np.minimum(dy, cells - dy)
        d = np.sqrt(dx * dx + dy * dy)
        closer = d < f1
        f2 = np.where(closer, f1, np.minimum(f2, d))
        cid = np.where(closer, i, cid)
        f1 = np.where(closer, d, f1)
    return f1, f2 - f1, cid


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def colorize(base, var, t):
    """base, var: rgb tuples (0..1), t: array -> rgb image base + var*(t-0.5)*2"""
    base = np.array(base, np.float32)
    var = np.array(var, np.float32)
    return base[None, None, :] + var[None, None, :] * (t[..., None] - 0.5) * 2.0


def normal_from_height(h, strength):
    dx = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) * 0.5 * strength * N / 256.0
    dy = (np.roll(h, -1, 0) - np.roll(h, 1, 0)) * 0.5 * strength * N / 256.0
    nx, ny, nz = -dx, -dy, np.ones_like(h)
    l = np.sqrt(nx * nx + ny * ny + nz * nz)
    return np.stack([nx / l, ny / l, nz / l], -1)


def ao_from_height(h, radius=6):
    blur = h.copy()
    for k in (radius, radius // 2 + 1):
        acc = np.zeros_like(blur)
        for sx in (-k, 0, k):
            for sy in (-k, 0, k):
                acc += np.roll(np.roll(blur, sx, 1), sy, 0)
        blur = acc / 9.0
    return np.clip(1.0 - np.clip(blur - h, 0, 1) * 3.0, 0.35, 1.0)


def save(name, color, height, rough, metal=None, nstrength=4.0, ao=None):
    os.makedirs(OUT, exist_ok=True)
    color = np.clip(color, 0, 1)
    if ao is None:
        ao = ao_from_height(height)
    if metal is None:
        metal = np.zeros_like(height)
    srgb = np.where(color <= 0.0031308, color * 12.92, 1.055 * np.power(color, 1 / 2.4) - 0.055)
    Image.fromarray((srgb * 255 + 0.5).astype(np.uint8)).save(os.path.join(OUT, f"T_GS_{name}_D.jpg"), quality=92)
    nrm = normal_from_height(height, nstrength)
    Image.fromarray(((nrm * 0.5 + 0.5) * 255 + 0.5).astype(np.uint8)).save(os.path.join(OUT, f"T_GS_{name}_N.jpg"), quality=95)
    rough = np.broadcast_to(np.asarray(rough, np.float32), height.shape)
    ao = np.broadcast_to(np.asarray(ao, np.float32), height.shape)
    arm = np.stack([ao, np.clip(rough, 0.02, 1), np.clip(metal, 0, 1)], -1)
    Image.fromarray((arm * 255 + 0.5).astype(np.uint8)).save(os.path.join(OUT, f"T_GS_{name}_ARM.jpg"), quality=92)
    print("wrote", name)


# ------------------------------------------------------------------ masonry layouts
def running_bond(rows, cols, mortar_px, rng, offset=0.5, jitter_w=0.0):
    """Returns (mortar mask 0..1 where 1 = brick face, brick id, local u, local v)."""
    x, y = grid()
    r = np.floor(y * rows).astype(np.int32)
    v = y * rows - r
    shift = (r % 2) * offset
    u_all = x * cols + shift
    c = np.floor(u_all).astype(np.int32) % cols
    u = u_all - np.floor(u_all)
    bid = r * cols + c
    mu = mortar_px / (N / cols)
    mv = mortar_px / (N / rows)
    face = smoothstep(0, mu, u) * smoothstep(0, mu, 1 - u) * smoothstep(0, mv, v) * smoothstep(0, mv, 1 - v)
    return face, bid, u, v


def ashlar(rng, course_heights, min_w, max_w):
    """Irregular stone courses: rows with given heights (fractions summing to 1), random block widths that wrap."""
    x, y = grid()
    face = np.zeros((N, N), np.float32)
    bid = np.zeros((N, N), np.int32)
    bulge = np.zeros((N, N), np.float32)
    y0 = 0.0
    k = 0
    for h in course_heights:
        rows = (y >= y0) & (y < y0 + h)
        widths = []
        tot = 0.0
        while tot < 1.0 - min_w:
            w = rng.uniform(min_w, max_w)
            widths.append(w)
            tot += w
        widths = np.array(widths) / tot
        edges = np.concatenate([[0], np.cumsum(widths)])
        start = rng.random()
        xs = (x - start) % 1.0
        idx = np.clip(np.searchsorted(edges, xs, side="right") - 1, 0, len(widths) - 1)
        u = (xs - edges[idx]) / widths[idx]
        v = (y - y0) / h
        mu = 5.0 / (N * widths[idx])
        mv = 5.0 / (N * h)
        f = smoothstep(0, mu, u) * smoothstep(0, mu, 1 - u) * smoothstep(0, mv, v) * smoothstep(0, mv, 1 - v)
        b = np.minimum(np.minimum(u, 1 - u) * widths[idx], np.minimum(v, 1 - v) * h) * 12.0
        face = np.where(rows, f, face)
        bid = np.where(rows, idx + k * 100, bid)
        bulge = np.where(rows, np.clip(b, 0, 1), bulge)
        y0 += h
        k += 1
    return face, bid, bulge


def per_id(bid, rng, lo, hi):
    ids = np.unique(bid)
    table = {i: rng.uniform(lo, hi) for i in ids}
    vec = np.vectorize(table.get, otypes=[np.float32])
    return vec(bid)


# ------------------------------------------------------------------ materials
def brick(name, base, var, seed, mortar=(0.62, 0.6, 0.56)):
    rng = np.random.default_rng(seed)
    face, bid, u, v = running_bond(27, 9, 4.5, rng)
    tone = per_id(bid, rng, 0.0, 1.0)
    burn = per_id(bid, rng, 0.0, 1.0) ** 4
    grain = fbm(rng, 1.2, 40)
    blot = fbm(rng, 2.2, 6)
    col = colorize(base, var, tone * 0.7 + grain * 0.3)
    col *= (1 - burn[..., None] * 0.45)
    col *= (0.85 + 0.3 * blot[..., None])
    mcol = colorize(mortar, (0.05, 0.05, 0.05), fbm(rng, 1.5, 30))
    dirt = fbm(rng, 2.5, 2) * smoothstep(0.3, 1.0, grid()[1])
    col = lerp(mcol, col, face[..., None]) * (1 - 0.25 * dirt[..., None])
    h = face * (0.7 + 0.08 * grain + 0.1 * fbm(rng, 2.0, 8)) + (1 - face) * 0.25 * fbm(rng, 1.0, 60)
    rough = lerp(0.95, 0.82 + 0.1 * grain, face)
    save(name, col, h, rough, nstrength=5.0)


def stone(name, base, var, seed, courses, minw, maxw, rock=1.0):
    rng = np.random.default_rng(seed)
    hs = np.array(courses, float)
    hs = hs / hs.sum()
    face, bid, bulge = ashlar(rng, hs, minw, maxw)
    tone = per_id(bid, rng, 0.0, 1.0)
    crystals = fbm(rng, 0.6, 200)
    speck = (fbm(rng, 0.2, 250) > 0.72).astype(np.float32)
    rocky = fbm(rng, 1.8, 10)
    col = colorize(base, var, tone * 0.6 + crystals * 0.4)
    col = col * (1 - 0.35 * speck[..., None]) + 0.12 * (crystals[..., None] > 0.8)
    lich = smoothstep(0.65, 0.9, fbm(rng, 2.2, 5)) * 0.4
    col = lerp(col, np.array([0.35, 0.37, 0.3]), lich[..., None])
    mcol = colorize((0.55, 0.54, 0.5), (0.05, 0.05, 0.05), fbm(rng, 1.5, 30))
    col = lerp(mcol, col, face[..., None])
    stain = fbm(rng, 2.6, 1.5) * smoothstep(0.0, 1.0, grid()[1])
    col *= (1 - 0.3 * stain[..., None])
    h = face * (0.45 + 0.35 * bulge * rock + 0.25 * rocky * rock + 0.05 * crystals)
    rough = lerp(0.95, 0.8 + 0.15 * crystals, face)
    save(name, col, h, rough, nstrength=6.0)


def slate(seed):
    rng = np.random.default_rng(seed)
    face, bid, u, v = running_bond(10, 7, 3.0, rng)
    tone = per_id(bid, rng, 0.0, 1.0)
    rot = per_id(bid, rng, -0.06, 0.06)
    layer = fbm(rng, 1.0, 30)
    col = colorize((0.16, 0.17, 0.2), (0.04, 0.04, 0.05), tone * 0.7 + layer * 0.3)
    moss = smoothstep(0.7, 0.95, fbm(rng, 2.0, 4)) * (1 - v) * 0.6
    col = lerp(col, np.array([0.18, 0.22, 0.12]), moss[..., None])
    # each slate is thicker at the bottom edge (overlap) -> height ramps with v
    h = face * (0.3 + 0.5 * (1 - v) + rot * (u - 0.5) + 0.06 * layer) + (1 - face) * 0.05
    rough = 0.55 + 0.25 * layer + 0.2 * moss
    save("Slate", col, h, rough, nstrength=6.0)


def plaster(seed, base=(0.78, 0.75, 0.68), name="Plaster"):
    rng = np.random.default_rng(seed)
    bumps = fbm(rng, 1.6, 20)
    fine = fbm(rng, 0.8, 120)
    big = fbm(rng, 2.4, 2)
    streak = fbm(rng, 2.0, 2, 30)
    x, y = grid()
    rain = np.clip(np.sin((x * 37 + streak * 3) * np.pi) * 0.5 + 0.5, 0, 1) ** 8 * smoothstep(0.2, 1, y)
    col = colorize(base, (0.04, 0.04, 0.04), big * 0.7 + fine * 0.3)
    col *= (1 - 0.07 * rain[..., None])
    h = bumps * 0.6 + fine * 0.15
    save(name, col, h, 0.88 + 0.08 * fine, nstrength=2.5)


def concrete(seed, name="Concrete", base=(0.55, 0.54, 0.52), boards=True):
    rng = np.random.default_rng(seed)
    x, y = grid()
    big = fbm(rng, 2.3, 2)
    mid = fbm(rng, 1.6, 12)
    pores = (fbm(rng, 0.3, 300) > 0.78).astype(np.float32) * (fbm(rng, 1.0, 20) > 0.5)
    col = colorize(base, (0.06, 0.06, 0.06), big * 0.6 + mid * 0.4)
    h = mid * 0.3 - pores * 0.35
    if boards:
        rows = 8
        bv = y * rows - np.floor(y * rows)
        line = 1 - smoothstep(0, 0.02, bv) * smoothstep(0, 0.02, 1 - bv)
        bid = np.floor(y * rows).astype(np.int32)
        tone = per_id(bid, rng, -0.03, 0.03)
        grainw = fbm(rng, 1.0, 4, 200)
        col = col + tone[..., None] + 0.03 * (np.sin(x * 600 * np.pi + grainw * 10)[..., None])
        h = h - line * 0.3
    col *= (1 - 0.25 * pores[..., None])
    stain = smoothstep(0.5, 1, fbm(rng, 2.5, 1.5)) * smoothstep(0.3, 1, y)
    col *= (1 - 0.3 * stain[..., None])
    save(name, col, h, 0.85 + 0.1 * mid, nstrength=3.0)


def planks(name, base, var, seed, cols=8, rows=2, herring=False, gloss=0.45):
    rng = np.random.default_rng(seed)
    x, y = grid()
    face, bid, u, v = running_bond(cols, rows, 2.0, rng, offset=0.37)
    # here rows go across: transpose so planks run along X
    face, bid, u, v = face.T, bid.T, u.T, v.T
    tone = per_id(bid, rng, 0.0, 1.0)
    warp = fbm(rng, 2.0, 3, 20)
    gr = np.sin((y * cols * 9 + warp * 6 + tone * 30) * np.pi * 2)
    rings = fbm(rng, 1.0, 50, 400)
    knots = smoothstep(0.92, 0.99, fbm(rng, 1.5, 12))
    t = tone * 0.5 + (gr * 0.5 + 0.5) * 0.3 + rings * 0.2
    col = colorize(base, var, t)
    col *= (1 - 0.5 * knots[..., None])
    col = col * face[..., None] + (1 - face[..., None]) * np.array(base) * 0.4
    h = face * (0.6 + 0.04 * gr) - knots * 0.1
    save(name, col, h, gloss + 0.2 * rings + 0.2 * (1 - face), nstrength=2.0)


def marble(seed):
    rng = np.random.default_rng(seed)
    x, y = grid()
    turb = fbm(rng, 2.0, 2, 60)
    veins = np.abs(np.sin((x * 3 + y * 2 + turb * 4) * np.pi * 2))
    veins = 1 - smoothstep(0.0, 0.12, veins)
    veins2 = 1 - smoothstep(0.0, 0.05, np.abs(np.sin((x * 7 - y * 5 + fbm(rng, 2.0, 3, 80) * 3) * np.pi * 2)))
    # large tiles 0.5 m
    face, bid, u, v = running_bond(4, 4, 1.5, rng, offset=0.0)
    tone = per_id(bid, rng, -0.04, 0.04)
    col = np.full((N, N, 3), 0.86, np.float32) + tone[..., None]
    col -= veins[..., None] * np.array([0.35, 0.35, 0.33]) + veins2[..., None] * 0.12
    col = lerp(np.array([0.5, 0.5, 0.5]), col, face[..., None])
    save("Marble", col, face * 0.5, 0.12 + 0.2 * (1 - face) + 0.05 * veins, nstrength=1.5)


def terrazzo(seed):
    rng = np.random.default_rng(seed)
    f1, edge, cid = voronoi(rng, 70, jitter=1.0)
    chips = (f1 < 0.33).astype(np.float32)
    tone = rng.random(int(cid.max()) + 1)[cid]
    base = colorize((0.7, 0.69, 0.66), (0.04, 0.04, 0.04), fbm(rng, 1.5, 20))
    chipcol = np.stack([0.3 + 0.6 * tone, 0.3 + 0.55 * tone, 0.28 + 0.5 * tone], -1)
    col = lerp(base, chipcol, chips[..., None] * 0.8)
    face, bid, u, v = running_bond(5, 5, 1.0, rng, offset=0.0)
    col = lerp(np.array([0.4, 0.4, 0.4]), col, face[..., None])
    save("Terrazzo", col, face * 0.5 + chips * 0.02, 0.25 + 0.2 * (1 - face), nstrength=1.0)


def copper(seed):
    rng = np.random.default_rng(seed)
    p = fbm(rng, 2.0, 3)
    fine = fbm(rng, 1.0, 80)
    patina = smoothstep(0.35, 0.65, p * 0.8 + fine * 0.2)
    col = lerp(np.array([0.45, 0.22, 0.1]), np.array([0.3, 0.55, 0.45]), patina[..., None])
    col *= (0.85 + 0.3 * fine[..., None])
    # standing seams every 0.5 m
    x, y = grid()
    seam = np.exp(-((x * 4 - np.round(x * 4)) ** 2) / 0.0004)
    h = 0.4 + 0.4 * seam + 0.05 * fine
    save("Copper", col, h, lerp(0.35, 0.8, patina), metal=1 - patina, nstrength=3.0)


def metal(seed, name="Metal", base=(0.56, 0.57, 0.58), painted=False):
    rng = np.random.default_rng(seed)
    x, y = grid()
    brush = fbm(rng, 1.0, 2, 400)
    brushed = np.repeat(brush[:, :1], N, 1) * 0.5 + fbm(rng, 1.0, 300) * 0.5
    scratches = np.zeros((N, N), np.float32)
    for _ in range(120):
        x0, y0, a, l = rng.random(), rng.random(), rng.uniform(0, np.pi), rng.uniform(0.02, 0.15)
        d = np.abs((x - x0) * np.sin(a) - (y - y0) * np.cos(a))
        along = (x - x0) * np.cos(a) + (y - y0) * np.sin(a)
        scratches = np.maximum(scratches, (d < 0.0012) * (np.abs(along) < l) * rng.uniform(0.3, 1))
    grime = fbm(rng, 2.2, 3)
    col = colorize(base, (0.04, 0.04, 0.04), brushed) * (1 - 0.25 * grime[..., None])
    col = col + scratches[..., None] * 0.15
    rough = 0.3 + 0.25 * grime + 0.1 * brushed - 0.15 * scratches
    save(name, col, brushed * 0.05 - scratches * 0.2, rough, metal=np.full((N, N), 0.0 if painted else 1.0) * (1 - 0.3 * grime),
         nstrength=1.0, ao=np.ones((N, N)))


def fabric(seed):
    rng = np.random.default_rng(seed)
    x, y = grid()
    k = 220
    wx = np.sin(x * k * np.pi * 2) * 0.5 + 0.5
    wy = np.sin(y * k * np.pi * 2) * 0.5 + 0.5
    checker = ((np.floor(x * k) + np.floor(y * k)) % 2)
    h = lerp(wx, wy, checker)
    fuzz = fbm(rng, 1.0, 100)
    col = colorize((0.6, 0.6, 0.6), (0.08, 0.08, 0.08), h * 0.5 + fuzz * 0.5)
    save("Fabric", col, h * 0.6, 0.95 + 0 * h, nstrength=2.0)


def asphalt(seed):
    rng = np.random.default_rng(seed)
    f1, edge, cid = voronoi(rng, 90, jitter=1.0)
    stones = smoothstep(0.42, 0.3, f1)
    tone = rng.random(int(cid.max()) + 1)[cid]
    fine = fbm(rng, 0.6, 200)
    big = fbm(rng, 2.4, 2)
    col = colorize((0.085, 0.085, 0.09), (0.03, 0.03, 0.03), fine * 0.5 + big * 0.5)
    col = lerp(col, (0.12 + 0.16 * tone)[..., None] * np.array([1, 1, 0.97]), stones[..., None] * 0.7)
    # tire wear patches and cracks
    crack = 1 - smoothstep(0.0, 0.012, edge) if False else None
    cracks = (1 - smoothstep(0.0, 0.02, voronoi(rng, 6, jitter=1.0)[1])) * smoothstep(0.55, 0.8, fbm(rng, 2.0, 3))
    col *= (1 - 0.6 * cracks[..., None])
    h = stones * 0.4 + fine * 0.2 - cracks * 0.5
    save("Asphalt", col, h, 0.85 - 0.15 * stones + 0.1 * big, nstrength=3.0)


def paving(seed):
    rng = np.random.default_rng(seed)
    face, bid, u, v = running_bond(10, 5, 3.0, rng)
    tone = per_id(bid, rng, 0.0, 1.0)
    fine = fbm(rng, 0.8, 150)
    col = colorize((0.5, 0.49, 0.47), (0.07, 0.07, 0.07), tone * 0.6 + fine * 0.4)
    gap = colorize((0.22, 0.2, 0.17), (0.04, 0.04, 0.04), fine)
    moss = smoothstep(0.6, 0.9, fbm(rng, 2.0, 6))
    gap = lerp(gap, np.array([0.12, 0.17, 0.07]), moss[..., None])
    col = lerp(gap, col, face[..., None])
    stain = smoothstep(0.55, 0.9, fbm(rng, 2.3, 3))
    col *= (1 - 0.25 * stain[..., None])
    save("Paving", col, face * (0.5 + 0.05 * fine), lerp(0.95, 0.75 + 0.15 * fine, face), nstrength=4.0)


def grass(seed):
    rng = np.random.default_rng(seed)
    big = fbm(rng, 2.3, 2)
    mid = fbm(rng, 1.5, 12)
    blades = fbm(rng, 0.4, 260)
    dry = smoothstep(0.55, 0.85, big)
    green = colorize((0.1, 0.16, 0.05), (0.05, 0.07, 0.03), mid * 0.4 + blades * 0.4 + big * 0.2)
    yellow = colorize((0.3, 0.28, 0.12), (0.06, 0.05, 0.03), blades)
    col = lerp(green, yellow, dry[..., None] * 0.6)
    dirt = smoothstep(0.82, 0.95, fbm(rng, 1.8, 8))
    col = lerp(col, np.array([0.17, 0.12, 0.08]), dirt[..., None] * 0.8)
    clover = smoothstep(0.75, 0.9, fbm(rng, 1.0, 40)) * 0.3
    col = col + clover[..., None] * np.array([0.0, 0.05, 0.0])
    h = blades * 0.7 + mid * 0.3
    save("Grass", col, h, 0.9 + 0.05 * blades, nstrength=4.0)


def gravel(seed):
    rng = np.random.default_rng(seed)
    f1, edge, cid = voronoi(rng, 60, jitter=1.0)
    tone = rng.random(int(cid.max()) + 1)[cid]
    pebble = smoothstep(0.5, 0.1, f1)
    col = (0.3 + 0.3 * tone)[..., None] * np.array([1.0, 0.97, 0.9]) * (0.6 + 0.4 * pebble[..., None])
    save("Gravel", col, pebble, 0.85 + 0.1 * (1 - pebble), nstrength=6.0)


def plastic(seed):
    rng = np.random.default_rng(seed)
    fine = fbm(rng, 0.8, 200)
    grime = fbm(rng, 2.2, 3)
    col = np.full((N, N, 3), 0.75, np.float32) * (1 - 0.12 * grime[..., None])
    save("Plastic", col, fine * 0.1, 0.4 + 0.2 * grime + 0.1 * fine, nstrength=0.6, ao=np.ones((N, N)))


def rubber(seed):
    rng = np.random.default_rng(seed)
    fine = fbm(rng, 0.8, 200)
    col = np.full((N, N, 3), 0.05, np.float32) + 0.02 * fine[..., None]
    save("Rubber", col, fine * 0.2, 0.9 + 0.05 * fine, nstrength=1.0, ao=np.ones((N, N)))


def leather(seed):
    rng = np.random.default_rng(seed)
    f1, edge, cid = voronoi(rng, 80, jitter=1.0)
    pebble = smoothstep(0.0, 0.08, edge)
    crease = smoothstep(0.6, 0.85, fbm(rng, 1.5, 6, 40))
    wear = smoothstep(0.55, 0.85, fbm(rng, 2.2, 3))
    col = colorize((0.22, 0.12, 0.06), (0.04, 0.03, 0.02), fbm(rng, 1.5, 20))
    col = col * (0.8 + 0.2 * pebble[..., None]) * (1 + 0.35 * wear[..., None])
    h = pebble * 0.4 - crease * 0.3
    save("Leather", col, h, 0.55 + 0.2 * (1 - pebble) - 0.15 * wear, nstrength=3.0)


def macro(seed):
    """Large-scale variation used by every material to hide tiling (sampled at 1/9 of the tiling)."""
    rng = np.random.default_rng(seed)
    m = fbm(rng, 2.0, 1, 40)
    os.makedirs(OUT, exist_ok=True)
    Image.fromarray((m * 255).astype(np.uint8)).save(os.path.join(OUT, "T_GS_Macro_M.jpg"), quality=92)
    print("wrote Macro")


def leaves(seed):
    """Leaf atlas with alpha (RGBA png): 4 x 4 clusters of leaves for tree cards."""
    rng = np.random.default_rng(seed)
    x, y = grid()
    alpha = np.zeros((N, N), np.float32)
    col = np.zeros((N, N, 3), np.float32)
    nrm_h = np.zeros((N, N), np.float32)
    cells = 4
    for cy in range(cells):
        for cx in range(cells):
            ox, oy = cx / cells, cy / cells
            for _ in range(75):
                lx = ox + rng.uniform(0.05, 0.2)
                ly = oy + rng.uniform(0.05, 0.2)
                a = rng.uniform(0, np.pi * 2)
                L = rng.uniform(0.018, 0.032)
                W = L * rng.uniform(0.35, 0.5)
                dx, dy = x - lx, y - ly
                ux = dx * np.cos(a) + dy * np.sin(a)
                uy = -dx * np.sin(a) + dy * np.cos(a)
                t = (ux / L + 1) * 0.5
                width = W * np.sin(np.clip(t, 0, 1) * np.pi) ** 0.8
                inside = (t > 0) & (t < 1) & (np.abs(uy) < width)
                tone = rng.uniform(0, 1)
                c = np.array([0.08 + 0.1 * tone, 0.2 + 0.12 * tone, 0.04 + 0.04 * tone])
                vein = (np.abs(uy) < W * 0.06)
                col = np.where(inside[..., None], c * np.where(vein, 1.3, 1.0)[..., None], col)
                nrm_h = np.where(inside, 0.5 + 0.5 * (1 - np.abs(uy) / np.maximum(width, 1e-6)), nrm_h)
                alpha = np.where(inside, 1.0, alpha)
            # twig
            d = np.abs((x - ox - 0.125) * 0.7 - (y - oy - 0.125) * 0.7)
            twig = (d < 0.002) & (x > ox + 0.02) & (x < ox + 0.23) & (y > oy + 0.02) & (y < oy + 0.23)
            col = np.where(twig[..., None] & (alpha[..., None] < 0.5), np.array([0.15, 0.1, 0.06]), col)
            alpha = np.where(twig, 1.0, alpha)
    srgb = np.where(col <= 0.0031308, col * 12.92, 1.055 * np.power(np.clip(col, 0, 1), 1 / 2.4) - 0.055)
    rgba = np.concatenate([srgb, alpha[..., None]], -1)
    os.makedirs(OUT, exist_ok=True)
    Image.fromarray((rgba * 255).astype(np.uint8), "RGBA").save(os.path.join(OUT, "T_GS_Leaves_D.png"))
    nrm = normal_from_height(nrm_h, 2.0)
    Image.fromarray(((nrm * 0.5 + 0.5) * 255).astype(np.uint8)).save(os.path.join(OUT, "T_GS_Leaves_N.jpg"), quality=95)
    print("wrote Leaves")


def bark(seed):
    rng = np.random.default_rng(seed)
    x, y = grid()
    warp = fbm(rng, 2.0, 2, 30)
    ridges = np.abs(np.sin((x * 14 + warp * 2.5) * np.pi))
    ridges = smoothstep(0.0, 0.6, ridges)
    fine = fbm(rng, 1.0, 60)
    col = colorize((0.2, 0.15, 0.11), (0.05, 0.04, 0.03), ridges * 0.6 + fine * 0.4)
    save("Bark", col, ridges * 0.7 + fine * 0.3, 0.9, nstrength=6.0)
    # birch bark: white with dark horizontal lenticels
    lent = smoothstep(0.75, 0.95, fbm(rng, 1.0, 8, 200) * np.abs(np.sin(y * 60 * np.pi))) 
    patches = smoothstep(0.7, 0.9, fbm(rng, 2.0, 4))
    col2 = np.full((N, N, 3), 0.78, np.float32) * (1 - 0.6 * lent[..., None]) * (1 - 0.5 * patches[..., None])
    save("BirchBark", col2, 0.5 - 0.3 * lent, 0.7, nstrength=2.0)


JOBS = {
    "Brick": lambda: brick("Brick", (0.42, 0.17, 0.11), (0.08, 0.04, 0.03), 1),
    "BrickBrown": lambda: brick("BrickBrown", (0.27, 0.15, 0.1), (0.05, 0.03, 0.02), 2, mortar=(0.45, 0.43, 0.4)),
    "BrickYellow": lambda: brick("BrickYellow", (0.62, 0.52, 0.36), (0.06, 0.05, 0.04), 3),
    "Stone": lambda: stone("Stone", (0.3, 0.31, 0.3), (0.06, 0.06, 0.06), 4, [3, 2, 3, 2, 4, 2, 3, 2, 3, 3, 2, 3], 0.12, 0.3),
    "StoneDark": lambda: stone("StoneDark", (0.26, 0.26, 0.26), (0.05, 0.05, 0.05), 5, [4, 3, 4, 3], 0.2, 0.45),
    "Trim": lambda: stone("Trim", (0.64, 0.62, 0.57), (0.03, 0.03, 0.03), 6, [1, 1], 0.4, 0.7, rock=0.15),
    "Slate": lambda: slate(7),
    "Plaster": lambda: plaster(8),
    "Concrete": lambda: concrete(9),
    "ConcreteSmooth": lambda: concrete(10, "ConcreteSmooth", (0.6, 0.59, 0.57), boards=False),
    "Floor": lambda: planks("Floor", (0.55, 0.38, 0.22), (0.08, 0.06, 0.04), 11, cols=10, rows=2),
    "Wood": lambda: planks("Wood", (0.3, 0.18, 0.1), (0.06, 0.04, 0.03), 12, cols=6, rows=1, gloss=0.5),
    "Marble": lambda: marble(13),
    "Terrazzo": lambda: terrazzo(14),
    "Copper": lambda: copper(15),
    "Metal": lambda: metal(16),
    "PaintedMetal": lambda: metal(17, "PaintedMetal", (0.8, 0.8, 0.8), painted=True),
    "Fabric": lambda: fabric(18),
    "Asphalt": lambda: asphalt(19),
    "Paving": lambda: paving(20),
    "Grass": lambda: grass(21),
    "Gravel": lambda: gravel(22),
    "Plastic": lambda: plastic(23),
    "Rubber": lambda: rubber(24),
    "Macro": lambda: macro(25),
    "Leather": lambda: leather(28),
    "Leaves": lambda: leaves(26),
    "Bark": lambda: bark(27),
}

if __name__ == "__main__":
    names = [a for a in sys.argv[1:] if a in JOBS] or list(JOBS)
    for n in names:
        JOBS[n]()
