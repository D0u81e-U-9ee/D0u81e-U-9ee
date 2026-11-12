#!/usr/bin/env python3
"""Procedural terrain generator for the Central European test map.

Outputs (into ../SourceArt/Terrain by default):
  heightmap_2017.png    16-bit grayscale, 2017x2017, ready for Landscape > Import from File
  terrain_layout.json   landscape transform, river / road / lake control points in UE cm
  terrain_preview.png   8-bit preview shaded with the auto-material rules (for review only)
  road_mask.png         road carriageway mask sampled by the landscape material (white = road)

Conventions (match docs/UE5_OpenWorld_Combat_TDD.md):
  +X = North, +Y = East, map centre at the world origin, lake surface at Z = 0.
  Image column -> world X, image row -> world Y (UE landscape import convention).
  Landscape scale (100, 100, 250), actor Z = 56000 cm:  h16 = 4096 + Z_m * 51.2

Only needs numpy.  Usage:  python3 generate_terrain.py [--out DIR] [--seed N]
"""
import argparse
import json
import math
import os
import struct
import zlib

import numpy as np

N = 2017
HALF = 1008.0
LAKE_E = 608.0        # shoreline: lake where softmax(-x,-y) > LAKE_E
RIDGE_D0 = 300.0      # ridge foot: mountains where softmax(x,y) > RIDGE_D0
VALLEY_GRADE = 0.055  # valley floor rise per metre inland
MAX_ROAD_GRADE = 0.075
Z_MIN, Z_MAX = -80.0, 1199.9
H16_PER_M = 51.2      # 100 cm / 1.953125 cm
H16_SEA = 4096


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def softmax2(a, b, s=80.0):
    m = np.maximum(a, b)
    return m + s * np.log(np.exp((a - m) / s) + np.exp((b - m) / s))


def value_noise(rng, n, cell):
    g = int(math.ceil(n / cell)) + 2
    r = rng.random((g, g))
    idx = np.arange(n) / cell
    i0 = idx.astype(int)
    f = idx - i0
    f = f * f * (3 - 2 * f)
    fx, fy = f[None, :], f[:, None]
    a, b = r[i0][:, i0], r[i0][:, i0 + 1]
    c, d = r[i0 + 1][:, i0], r[i0 + 1][:, i0 + 1]
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def fbm(rng, n, cell, octaves, gain=0.5):
    out = np.zeros((n, n))
    amp, norm = 1.0, 0.0
    for _ in range(octaves):
        out += amp * (value_noise(rng, n, cell) * 2 - 1)
        norm += amp
        amp *= gain
        cell = max(cell / 2, 2)
    return out / norm


def densify(pts, closed=False, step=1.0):
    """Catmull-Rom through pts, resampled at ~step metres. Returns (M,2) array."""
    p = np.asarray(pts, float)
    if closed:
        p = np.vstack([p[-1], p, p[0], p[1]])
    else:
        p = np.vstack([2 * p[0] - p[1], p, 2 * p[-1] - p[-2]])
    out = []
    for i in range(1, len(p) - 2):
        p0, p1, p2, p3 = p[i - 1], p[i], p[i + 1], p[i + 2]
        n = max(int(np.linalg.norm(p2 - p1) / 0.25), 2)
        t = np.linspace(0, 1, n, endpoint=False)[:, None]
        out.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t ** 2
                          + (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3))
    fine = np.vstack(out + ([] if closed else [p[-2][None, :]]))
    if closed:
        fine = np.vstack([fine, fine[0]])
    seg = np.linalg.norm(np.diff(fine, axis=0), axis=1)
    s = np.concatenate([[0], np.cumsum(seg)])
    m = int(s[-1] / step)
    target = np.linspace(0, m * step, m + 1)
    res = np.stack([np.interp(target, s, fine[:, 0]), np.interp(target, s, fine[:, 1])], axis=1)
    return res[:-1] if closed else res


def sample(z, pts):
    """Bilinear sample of z at world-metre points (x, y)."""
    cx = np.clip(pts[:, 0] + HALF, 0, N - 1.001)
    cy = np.clip(pts[:, 1] + HALF, 0, N - 1.001)
    x0, y0 = cx.astype(int), cy.astype(int)
    fx, fy = cx - x0, cy - y0
    return (z[y0, x0] * (1 - fx) * (1 - fy) + z[y0, x0 + 1] * fx * (1 - fy)
            + z[y0 + 1, x0] * (1 - fx) * fy + z[y0 + 1, x0 + 1] * fx * fy)


def window(px, py, r):
    c0, c1 = max(int(px + HALF - r), 0), min(int(px + HALF + r) + 2, N)
    r0, r1 = max(int(py + HALF - r), 0), min(int(py + HALF + r) + 2, N)
    return r0, r1, c0, c1


def carve_channel(z, path, bed, half_w, tan_side, mask=None):
    """z = min(z, bed_i + max(0, dist_i - half_w) * tan_side) for every path sample."""
    ground = sample(z, path)
    for i in range(0, len(path), 2):
        px, py = path[i]
        r = max((ground[i] - bed[i]) / tan_side + half_w + 6.0, 12.0)
        r0, r1, c0, c1 = window(px, py, r)
        if r0 >= r1 or c0 >= c1:
            continue
        xs = np.arange(c0, c1) - HALF
        ys = np.arange(r0, r1) - HALF
        dist = np.hypot(xs[None, :] - px, ys[:, None] - py)
        cut = bed[i] + np.maximum(dist - half_w, 0.0) * tan_side
        sub = z[r0:r1, c0:c1]
        if mask is not None:
            cut = np.where(mask[r0:r1, c0:c1], cut, np.inf)
        np.minimum(sub, cut, out=sub)


def bake_road(z, path, road_z, half_w=4.5, falloff=11.5):
    r = half_w + falloff + 2
    best_d = np.full((N, N), np.inf, dtype=np.float32)
    best_z = np.zeros((N, N), dtype=np.float32)
    for i in range(len(path)):
        px, py = path[i]
        r0, r1, c0, c1 = window(px, py, r)
        xs = np.arange(c0, c1) - HALF
        ys = np.arange(r0, r1) - HALF
        dist = np.hypot(xs[None, :] - px, ys[:, None] - py)
        bd = best_d[r0:r1, c0:c1]
        closer = dist < bd
        bd[closer] = dist[closer]
        best_z[r0:r1, c0:c1][closer] = road_z[i]
    w = 1.0 - smoothstep(half_w, half_w + falloff, best_d)
    z += (best_z - z) * w
    # carriageway mask for the landscape material: 1 on the road, fading out over the verge
    return 1.0 - smoothstep(half_w - 0.5, half_w + 2.5, best_d)


def limit_grade(zr, g, closed, pin_start):
    zr = zr.copy()
    n = len(zr)
    for _ in range(4):
        rng_f = range(1, n) if not closed else range(1, 2 * n)
        for i in rng_f:
            a, b = (i - 1) % n, i % n
            zr[b] = min(max(zr[b], zr[a] - g), zr[a] + g)
        if not pin_start:
            rng_b = range(n - 2, -1, -1) if not closed else range(2 * n - 2, -1, -1)
            for i in rng_b:
                a, b = (i + 1) % n, i % n
                zr[b] = min(max(zr[b], zr[a] - g), zr[a] + g)
    return zr


def moving_avg(v, k, closed):
    if closed:
        ext = np.concatenate([v[-k:], v, v[:k]])
    else:
        ext = np.concatenate([np.full(k, v[0]), v, np.full(k, v[-1])])
    ker = np.ones(2 * k + 1) / (2 * k + 1)
    return np.convolve(ext, ker, mode="same")[k:-k]


def write_png(path, arr, bit16):
    """Minimal PNG writer. arr: (H,W) uint16 gray or (H,W,3) uint8 RGB."""
    h, w = arr.shape[:2]
    if bit16:
        raw = arr.astype(">u2").tobytes()
        rows = np.frombuffer(raw, dtype=np.uint8).reshape(h, w * 2)
        bpp, ctype, depth = 2, 0, 16
    else:
        rows = arr.reshape(h, w * 3)
        bpp, ctype, depth = 3, 2, 8
    filt = rows.copy()
    filt[:, bpp:] = rows[:, bpp:] - rows[:, :-bpp]          # PNG "Sub" filter (mod 256)
    data = np.concatenate([np.ones((h, 1), np.uint8), filt], axis=1).tobytes()

    def chunk(tag, payload):
        c = struct.pack(">I", len(payload)) + tag + payload
        return c + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, depth, ctype, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(data, 9)))
        f.write(chunk(b"IEND", b""))


# --------------------------------------------------------------------------- layout
# (x_north, y_east) in metres. River A drains the north ridge into the west lake arm,
# river B the east ridge into the south arm, so they never cross each other or the loop.
RIVERS = {
    "River_A": {
        "upper": [(885, -250), (840, -262), (780, -250)],
        "lower": [(760, -250), (600, -270), (430, -300), (300, -300), (190, -420),
                  (120, -540), (100, -640), (95, -700)],
        "down": (-1.0, 0.0), "head_plane": 768.0, "axis": 0,
    },
    "River_B": {
        "upper": [(-250, 885), (-262, 840), (-250, 780)],
        "lower": [(-250, 760), (-270, 600), (-300, 430), (-300, 300), (-420, 190),
                  (-540, 120), (-640, 100), (-700, 95)],
        "down": (0.0, -1.0), "head_plane": 768.0, "axis": 1,
    },
}
SOURCE_Z, LIP_Z = 600.0, 584.0

LOOP_C, LOOP_R = (-120.0, -120.0), 230.0
ROADS = {
    "Road_Loop": {"closed": True, "pts": [
        (LOOP_C[0] + LOOP_R * math.cos(a), LOOP_C[1] + LOOP_R * math.sin(a))
        for a in np.linspace(0, 2 * math.pi, 12, endpoint=False)]},
    "Road_Harbour": {"closed": False, "pts": [(-350, -120), (-470, -135), (-592, -120)]},
    "Road_RidgeFoot": {"closed": False, "pts": [(43, 43), (150, 160), (250, 240)]},
    "Road_Waterfall": {"closed": False, "pts": [(110, -120), (240, -190), (330, -245), (410, -275)]},
}


def build(seed):
    rng = np.random.default_rng(seed)
    xs = np.arange(N) - HALF
    X, Y = np.meshgrid(xs, xs)             # X[row, col] = x(col), Y[row, col] = y(row)
    d = softmax2(X, Y)                     # distance toward the N / E ridges
    e = softmax2(-X, -Y)                   # distance toward the S / W lake arms
    s = LAKE_E - e                         # metres inland from the shoreline

    # valley floor and lake basin
    land = VALLEY_GRADE * np.maximum(s, 0) + 1.5 * (1 - np.exp(-np.maximum(s, 0) / 15.0))
    lake = -75.0 * smoothstep(0.0, 250.0, -s)
    z = np.where(s > 0, land, lake)
    valley_mask = smoothstep(0, 120, s) * (1 - smoothstep(RIDGE_D0 - 80, RIDGE_D0 + 40, d))
    z += 4.0 * fbm(rng, N, 300, 3) * valley_mask

    # ridge profile: meadow foothills (~19 deg) -> rock flank (~50 deg) -> cliff band (>70 deg) -> crest
    dd = np.arange(0, 1400.0)
    prof = np.interp(dd, [RIDGE_D0, 520, 700, 900, 1008, 1150], [0, 75, 290, 1000, 1110, 1130])
    ker = np.hanning(41)
    prof = np.convolve(np.pad(prof, 20, mode="edge"), ker / ker.sum(), mode="valid")
    z += np.interp(d, dd, prof)
    z += 60.0 * fbm(rng, N, 256, 5) * smoothstep(RIDGE_D0, 850, d)
    z += 6.0 * fbm(rng, N, 48, 3) * smoothstep(RIDGE_D0, 600, d)
    np.clip(z, Z_MIN, Z_MAX, out=z)

    layout = {"rivers": {}, "roads": {}, "waterfalls": []}

    # rivers: hanging upper valley -> waterfall headwall -> gorge -> valley -> lake
    for name, R in RIVERS.items():
        up = densify(R["upper"])
        up_bed = np.linspace(SOURCE_Z, LIP_Z, len(up))
        carve_channel(z, up, up_bed, half_w=5.0, tan_side=2.5)

        lo = densify(R["lower"])
        t = np.arange(len(lo), dtype=float)
        ground = moving_avg(sample(z, lo), 10, False)
        foot = int(np.argmax(softmax2(lo[:, 0], lo[:, 1]) < RIDGE_D0 + 5))
        foot_z = ground[foot] - 3.0
        designed = np.where(t <= foot, foot_z + 0.08 * (foot - t), np.inf)
        bed = np.minimum.accumulate(np.minimum(designed, ground - 3.0))
        coord = X if R["axis"] == 0 else Y
        mask = coord < R["head_plane"]     # vertical headwall under the lip
        carve_channel(z, lo, bed, half_w=6.0, tan_side=2.2, mask=mask)

        def pts3(path, bedz, every, width0, width1, clamp):
            out = []
            idx = list(range(0, len(path), every))
            if idx[-1] != len(path) - 1:
                idx.append(len(path) - 1)
            for k, i in enumerate(idx):
                wz = bedz[i] + 1.2
                if clamp:
                    wz = max(wz, 0.0)
                f = k / max(len(idx) - 1, 1)
                out.append({"x": round(path[i, 0] * 100), "y": round(path[i, 1] * 100),
                            "z": round(wz * 100), "width": round(width0 + (width1 - width0) * f)})
            return out

        layout["rivers"][name + "_Upper"] = pts3(up, up_bed, 25, 600, 900, False)
        layout["rivers"][name + "_Lower"] = pts3(lo, bed, 40, 1000, 2200, True)
        lip, base = up[-1], lo[0]
        layout["waterfalls"].append({
            "name": name + "_Waterfall",
            "lip": {"x": round(lip[0] * 100), "y": round(lip[1] * 100), "z": round((LIP_Z + 1.2) * 100)},
            "base": {"x": round(base[0] * 100), "y": round(base[1] * 100), "z": round((bed[0] + 1.2) * 100)},
            "exclusion_box_center": {"x": round((lip[0] + base[0]) * 50), "y": round((lip[1] + base[1]) * 50),
                                     "z": round((LIP_Z + bed[0]) * 50)},
            "exclusion_box_extent": {"x": 3000, "y": 3000, "z": round((LIP_Z - bed[0]) * 50 + 1500)},
        })

    # roads: loop first, then branches pinned to the already-baked junction height
    road_paths = {}
    all_xy, all_z = [], []
    for name, R in ROADS.items():
        path = densify(R["pts"], closed=R["closed"])
        zr = moving_avg(sample(z, path), 40, R["closed"])
        if not R["closed"]:                # pin the branch start to the nearest earlier road sample
            pxy, pz = np.vstack(all_xy), np.concatenate(all_z)
            zr[0] = pz[np.argmin(np.hypot(pxy[:, 0] - path[0, 0], pxy[:, 1] - path[0, 1]))]
        zr = np.maximum(zr, 1.2)           # keep the carriageway above lake level
        zr = limit_grade(zr, MAX_ROAD_GRADE, R["closed"], pin_start=not R["closed"])
        all_xy.append(path)
        all_z.append(zr)
        road_paths[name] = (path, R["closed"])
    # one shared bake so the nearest road sample wins at junctions
    road_mask = bake_road(z, np.vstack(all_xy), np.concatenate(all_z))

    np.clip(z, Z_MIN, Z_MAX, out=z)

    for name, (path, closed) in road_paths.items():
        zs = sample(z, path)
        idx = list(range(0, len(path), 25))
        layout["roads"][name] = {"closed": closed, "points": [
            {"x": round(path[i, 0] * 100), "y": round(path[i, 1] * 100), "z": round(zs[i] * 100)} for i in idx]}

    return z, layout, road_paths, road_mask


def analyse(z, road_paths):
    gy, gx = np.gradient(z)
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    xs = np.arange(N) - HALF
    X, Y = np.meshgrid(xs, xs)
    d = softmax2(X, Y)
    rep = {
        "z_min_m": round(float(z.min()), 2), "z_max_m": round(float(z.max()), 2),
        "lake_area_pct": round(float((z < 0).mean() * 100), 1),
        "cliff_band_pct_over_60deg": round(float((slope[(d > 700) & (d < 900)] > 60).mean() * 100), 1),
        "area_pct_over_45deg": round(float((slope > 45).mean() * 100), 1),
        "area_pct_snow_above_850m": round(float((z > 850).mean() * 100), 1),
        "ridge_crest_min_m_north_edge": round(float(z[:, -1][xs > -500].min()), 1),
        "ridge_crest_min_m_east_edge": round(float(z[-1, :][xs > -500].min()), 1),
        "roads": {},
    }
    ok = True
    for name, (path, closed) in road_paths.items():
        zs = sample(z, path)
        if closed:
            zs = np.concatenate([zs, zs[:6]])
        g = np.abs(zs[5:] - zs[:-5]) / 5.0          # 5 m baseline, 1 m samples
        rep["roads"][name] = {"length_m": len(path), "max_grade_pct": round(float(g.max() * 100), 2),
                              "z_range_m": [round(float(zs.min()), 1), round(float(zs.max()), 1)]}
        ok &= g.max() <= 0.10
    rep["roads_within_10pct"] = bool(ok)
    return rep, slope


def material_preview(z, slope, layout):
    """Same weights as SourceArt/Materials/LandscapeAutoWeights.ush."""
    zc = z * 100.0
    w_cliff = smoothstep(42, 48, slope)
    w_scree = np.clip(np.maximum(smoothstep(27, 33, slope), smoothstep(65000, 70000, zc)), 0, 1) * (1 - w_cliff)
    rest = 1 - w_cliff - w_scree
    w_forest = rest * (1 - smoothstep(12, 18, slope)) * (1 - smoothstep(45000, 50000, zc))
    w_meadow = rest - w_forest
    nz = np.cos(np.radians(slope))
    snow = np.clip((zc - 85000) / 15000, 0, 1) * np.clip((nz - 0.5) / 0.3, 0, 1)
    cols = {"forest": (58, 74, 40), "meadow": (112, 140, 62), "scree": (138, 130, 118), "cliff": (92, 90, 92)}
    rgb = sum(w[..., None] * np.array(cols[k], float) for k, w in
              (("forest", w_forest), ("meadow", w_meadow), ("scree", w_scree), ("cliff", w_cliff)))
    rgb = rgb * (1 - snow[..., None]) + np.array((240, 244, 250.0)) * snow[..., None]
    gy, gx = np.gradient(z)
    shade = np.clip(0.75 + 0.6 * (gx * 0.6 + gy * 0.6) / np.sqrt(1 + gx * gx + gy * gy), 0.35, 1.25)
    rgb *= shade[..., None]
    water = z < 0
    depth = np.clip(-z / 60.0, 0, 1)[..., None]
    rgb = np.where(water[..., None], np.array((70, 130, 160.0)) * (1 - depth) + np.array((18, 50, 90.0)) * depth, rgb)

    def draw(points, colour, rad):
        for p in points:
            c, r = int(p["x"] / 100 + HALF), int(p["y"] / 100 + HALF)
            rgb[max(r - rad, 0):r + rad + 1, max(c - rad, 0):c + rad + 1] = colour

    for pts in layout["rivers"].values():
        draw(pts, (60, 150, 230), 3)
    for rd in layout["roads"].values():
        draw(rd["points"], (235, 215, 120), 2)
    return np.clip(rgb[::2, ::2], 0, 255).astype(np.uint8)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                                  "..", "SourceArt", "Terrain"))
    ap.add_argument("--seed", type=int, default=1847)
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)

    z, layout, road_paths, road_mask = build(args.seed)
    report, slope = analyse(z, road_paths)

    h16 = np.clip(np.round(H16_SEA + z * H16_PER_M), 0, 65535).astype(np.uint16)
    write_png(os.path.join(args.out, "heightmap_2017.png"), h16, bit16=True)
    write_png(os.path.join(args.out, "terrain_preview.png"), material_preview(z, slope, layout), bit16=False)
    # same orientation as the heightmap: column -> world X, row -> world Y
    mask8 = np.clip(np.round(road_mask * 255), 0, 255).astype(np.uint8)
    write_png(os.path.join(args.out, "road_mask.png"), np.repeat(mask8[..., None], 3, axis=2), bit16=False)

    layout.update({
        "units": "cm", "axes": "+X north, +Y east",
        "landscape": {"resolution": N, "section_quads": 63, "sections_per_component": 1, "components": 32,
                      "location": [-100800, -100800, 56000], "scale": [100, 100, 250]},
        "lake": {"water_z": 0, "shoreline_softmax_distance_cm": int(LAKE_E * 100)},
        "player_start": layout["roads"]["Road_Loop"]["points"][0],
        "report": report,
    })
    with open(os.path.join(args.out, "terrain_layout.json"), "w") as f:
        json.dump(layout, f, indent=1)
    print(json.dumps(report, indent=1))
    if not report["roads_within_10pct"]:
        raise SystemExit("FAIL: a road exceeds 10 % grade")


if __name__ == "__main__":
    main()
