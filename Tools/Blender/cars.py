"""Parked cars for Gloshaugen (generic, unbranded):  python Tools/Blender/cars.py

SM_Car_Sedan, SM_Car_Hatch, SM_Car_SUV, SM_Car_Van, SM_Car_Estate. Front faces +X, origin on the ground in the
middle. The body uses the material slot "CarPaint", which GSCampusBuilder swaps per car for a colour
(MI_GS_CarPaint_<Colour>), so one mesh gives cars in many colours.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from meshkit import MeshKit  # noqa: E402


def car(name, L, W, body, cabin, wheel_r=0.33, wheelbase=None, clearance=0.22, roof_rails=False, van=False):
    """body/cabin: side profiles [(x, z), ...] counter-clockwise, x from rear (-) to front (+)."""
    k = MeshKit(name)
    paint = k.mat("CarPaint", (0.5, 0.05, 0.05), metallic=0.3, rough=0.3)
    glass = k.mat("GlassDark", (0.03, 0.04, 0.05), rough=0.05)
    trim = k.mat("Plastic", (0.05, 0.05, 0.05))
    rubber = k.mat("Rubber", (0.03, 0.03, 0.03))
    rim = k.mat("Steel", (0.7, 0.7, 0.72), metallic=1.0, rough=0.25)
    light = k.mat("PlasticWhite", (0.95, 0.95, 0.95))
    red = k.mat("PaintRed", (0.8, 0.05, 0.03))
    amber = k.mat("PaintOrange", (1.0, 0.5, 0.05))
    plate = k.mat("Plate", (0.95, 0.95, 0.95))
    wb = wheelbase or L * 0.6
    hw = W / 2
    # body and cabin
    k.prism_y(-hw, hw, body, paint)
    cw = hw - 0.12
    k.prism_y(-cw, cw, cabin, paint)
    # glass: side windows (shrunk cabin profile, a little wider than the cabin), windscreen, rear window
    cx = sum(p[0] for p in cabin) / len(cabin)
    cz = sum(p[1] for p in cabin) / len(cabin)
    shrunk = [(cx + (x - cx) * 0.86, cz + (z - cz) * 0.8) for x, z in cabin]
    k.prism_y(-cw - 0.015, cw + 0.015, shrunk, glass)
    # front and rear glass follow the sloped edges of the cabin
    pts = cabin
    zmin = min(p[1] for p in pts)
    front = [p for p in pts if p[0] > cx]
    rear = [p for p in pts if p[0] < cx]
    for grp, sgn in ((front, 1), (rear, -1)):
        lo = max((p for p in grp if abs(p[1] - zmin) < 0.05), key=lambda p: sgn * p[0], default=None)
        hi = max((p for p in grp if p[1] > zmin + 0.1), key=lambda p: sgn * p[0], default=None)
        if lo and hi:
            mx, mz = (lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2
            length = math.hypot(hi[0] - lo[0], hi[1] - lo[1])
            pitch = math.degrees(math.atan2(hi[1] - lo[1], hi[0] - lo[0]))
            k.box((mx + sgn * 0.012, 0, mz + 0.012), (length * 0.92, cw * 2 - 0.1, 0.02), glass, rot=(0, -pitch, 0))
    # wheels with rims and wheel-arch shadows
    for x in (-wb / 2, wb / 2):
        for y in (-hw + 0.13, hw - 0.13):
            k.cyl_axis((x, y, wheel_r), wheel_r, 0.24, rubber, axis="Y", verts=22)
            s = 1 if y > 0 else -1
            k.cyl_axis((x, y + s * 0.11, wheel_r), wheel_r * 0.66, 0.04, rim, axis="Y", verts=22)
            for i in range(5):
                a = i * 72
                k.box((x, y + s * 0.13, wheel_r), (wheel_r * 1.2, 0.02, 0.05), rim, rot=(0, a, 0))
            k.cyl_axis((x, y + s * 0.135, wheel_r), 0.05, 0.02, trim, axis="Y", verts=10)
            k.cyl_axis((x, y - s * 0.02, wheel_r + 0.02), wheel_r + 0.06, 0.2, trim, axis="Y", verts=22)
    # underbody shadow
    k.box((0, 0, clearance + 0.02), (L - 0.4, W - 0.1, 0.04), trim)
    # lights, grille, bumpers, plates, mirrors, handles, door seams
    xf = max(p[0] for p in body)
    xr = min(p[0] for p in body)
    zl = clearance + 0.45 if not van else clearance + 0.6
    for y in (-hw + 0.22, hw - 0.22):
        k.box((xf - 0.02, y, zl), (0.06, 0.32, 0.12), light)
        k.box((xr + 0.02, y, zl + 0.08), (0.06, 0.28, 0.14), red)
        k.box((xf - 0.06, y * 1.04, zl - 0.2), (0.06, 0.12, 0.05), amber)
    k.box((xf - 0.01, 0, zl - 0.04), (0.04, W * 0.42, 0.16), trim)
    k.box((xf - 0.04, 0, clearance + 0.12), (0.12, W + 0.02, 0.16), trim)
    k.box((xr + 0.04, 0, clearance + 0.12), (0.12, W + 0.02, 0.16), trim)
    k.box((xf + 0.0, 0, clearance + 0.25), (0.02, 0.52, 0.11), plate)
    k.box((xr - 0.0, 0, clearance + 0.32), (0.02, 0.52, 0.11), plate)
    mx = max(p[0] for p in cabin) - 0.15
    for y in (-hw - 0.08, hw + 0.08):
        k.box((mx, y, zmin + 0.08), (0.12, 0.14, 0.1), paint)
    # door seams and handles
    x_doors = [xf - L * 0.33, xf - L * 0.57] if not van else [xf - L * 0.3]
    for y in (-hw - 0.004, hw + 0.004):
        for xd in x_doors:
            k.box((xd, y, (clearance + zmin) / 2 + 0.15), (0.012, 0.01, zmin - clearance - 0.1), trim)
            k.box((xd - 0.25, y, zmin - 0.12), (0.16, 0.02, 0.03), rim)
        k.box((0, y, clearance + 0.12), (L * 0.7, 0.012, 0.06), trim)
    if roof_rails:
        rz = max(p[1] for p in cabin)
        x0, x1 = cabin[3][0], cabin[2][0]
        for y in (-cw + 0.1, cw - 0.1):
            k.box(((x0 + x1) / 2, y, rz + 0.04), (abs(x1 - x0) * 0.9, 0.04, 0.04), rim)
    k.box((xr + 0.1, 0, clearance + 0.05), (0.1, 0.06, 0.06), rim)  # exhaust
    k.finish()


def main():
    c = 0.22
    car("SM_Car_Sedan", 4.7, 1.82,
        [(-2.35, c + 0.08), (2.35, c + 0.08), (2.35, c + 0.55), (2.1, c + 0.72), (1.2, c + 0.78), (-1.6, c + 0.8),
         (-2.3, c + 0.75), (-2.35, c + 0.5)],
        [(-1.75, c + 0.79), (1.15, c + 0.77), (0.35, c + 1.22), (-1.05, c + 1.24)])
    car("SM_Car_Hatch", 4.15, 1.78,
        [(-2.05, c + 0.1), (2.05, c + 0.1), (2.05, c + 0.55), (1.8, c + 0.72), (0.95, c + 0.8), (-1.95, c + 0.84),
         (-2.05, c + 0.6)],
        [(-1.95, c + 0.83), (0.95, c + 0.79), (0.2, c + 1.25), (-1.75, c + 1.27)], wheelbase=2.6)
    c = 0.32
    car("SM_Car_SUV", 4.75, 1.92,
        [(-2.37, c + 0.05), (2.37, c + 0.05), (2.37, c + 0.65), (2.05, c + 0.88), (1.2, c + 0.95), (-2.3, c + 0.98),
         (-2.37, c + 0.65)],
        [(-2.3, c + 0.97), (1.2, c + 0.94), (0.45, c + 1.42), (-2.15, c + 1.45)], wheel_r=0.37, wheelbase=2.85,
        clearance=c, roof_rails=True)
    car("SM_Car_Estate", 4.85, 1.85,
        [(-2.42, 0.3), (2.42, 0.3), (2.42, 0.78), (2.15, 0.95), (1.25, 1.0), (-2.4, 1.02), (-2.42, 0.75)],
        [(-2.35, 1.01), (1.2, 0.99), (0.4, 1.45), (-2.2, 1.47)], wheelbase=2.9, roof_rails=True)
    c = 0.25
    car("SM_Car_Van", 5.3, 2.0,
        [(-2.65, c + 0.05), (2.65, c + 0.05), (2.65, c + 0.65), (2.3, c + 0.95), (1.75, c + 1.05), (-2.65, c + 1.1)],
        [(-2.65, c + 1.09), (1.75, c + 1.04), (1.0, c + 1.85), (-2.6, c + 1.92)], wheel_r=0.35, wheelbase=3.4,
        clearance=c, van=True)
    print("cars done")


if __name__ == "__main__":
    main()
