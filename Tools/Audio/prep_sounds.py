"""Turns library recordings into game sounds:  python -I Tools/Audio/prep_sounds.py [--only PREFIX] [--dry]

Reads Tools/Audio/sources.json (the answer sheet: which original file becomes which S_GS_<Name>.wav and what is
done to it) and writes SourceArt/Sounds/S_GS_<Name>.wav, SourceArt/Sounds/CREDITS.md and SourceArt/Sounds/.gitignore.

Per sound: cut (start/len), optional speed change (pitch + length, like a tape), extra layers mixed in, 48 kHz,
high/low-pass, effect ("mid", "far", "tail:<room>"), mono for 3D sounds / stereo for 2D, silence trimmed below
-60 dBFS, 5 ms fades, peak normalised per category, 16-bit WAV.

Library files are data: WAVs are read with `wave`, OGGs are decoded by Blender's audio library
(Tools/Audio/blender_decode.py, run headless) into a cache under %TEMP%. Nothing from the library is executed.
Never deletes anything in SourceArt/Sounds that sources.json does not name (the synthetic sounds stay as backup).
Sounds whose license forbids sharing them as files (Sonniss) are listed in .gitignore so they never reach GitHub.
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import wave
from fractions import Fraction

import numpy as np
from scipy import signal

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(ROOT, "SourceArt", "Sounds")
LIBRARY = os.environ.get("GS_SOUND_LIBRARY", r"C:\Users\afno0\Documents\Lydbibliotek")
BLENDER = os.environ.get("GS_BLENDER", r"C:\Program Files\Blender Foundation\Blender 5.3\blender.exe")
CACHE = os.path.join(tempfile.gettempdir(), "gs_sound_decode")
SR = 48000
NO_SHARE = ("Sonniss",)   # license ids whose files must not be committed

# Peak (dBFS) per category; Ambience is normalised by loudness (RMS) instead.
PEAK_DB = {"Weapon": -1.0, "Step": -6.0, "Impact": -4.0, "Body": -3.0, "Blast": -1.0, "Mech": -4.0,
           "UI": -10.0, "Radio": -6.0, "World": -3.0}
AMB_RMS_DB = -24.0
rng = np.random.default_rng(11)


# ---------------------------------------------------------------------------------------------------------- reading
def read_wav(path):
    with wave.open(path, "rb") as w:
        ch, width, sr, n = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        raw = w.readframes(n)
    if width == 2:
        x = np.frombuffer(raw, dtype="<i2").astype(np.float64) / 32768.0
    elif width == 3:
        b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        v = b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)
        v = np.where(v >= 1 << 23, v - (1 << 24), v)
        x = v.astype(np.float64) / float(1 << 23)
    elif width == 4:
        x = np.frombuffer(raw, dtype="<i4").astype(np.float64) / 2147483648.0
    else:
        x = (np.frombuffer(raw, dtype=np.uint8).astype(np.float64) - 128.0) / 128.0
    x = x.reshape(-1, ch)
    if ch == 1:
        x = np.repeat(x, 2, axis=1)
    return x[:, :2], sr


def lib_path(src):
    folder, inner = src.split("/", 1)
    return os.path.join(LIBRARY, folder, "extracted", *inner.split("/"))


def cache_name(path):
    return os.path.join(CACHE, hashlib.sha1(path.encode("utf-8")).hexdigest()[:16] + ".wav")


def decode_compressed(paths):
    """Decode every OGG/MP3/FLAC not in the cache yet with one headless Blender run."""
    todo = [p for p in paths if not os.path.exists(cache_name(p))]
    if not todo:
        return
    os.makedirs(CACHE, exist_ok=True)
    lst = os.path.join(CACHE, "decode_list.txt")
    with open(lst, "w", encoding="utf-8") as f:
        for p in todo:
            f.write(f"{p}|{cache_name(p)}\n")
    print(f"decoding {len(todo)} file(s) with Blender ...")
    subprocess.run([BLENDER, "-b", "--factory-startup", "--python", os.path.join(HERE, "blender_decode.py"), "--", lst],
                   cwd=CACHE, check=False, stdout=subprocess.DEVNULL)


def load(src):
    path = lib_path(src)
    if not os.path.exists(path):
        raise FileNotFoundError(f"{src} (run fetch_library.py --extract)")
    if os.path.splitext(path)[1].lower() != ".wav":
        path = cache_name(path)
        if not os.path.exists(path):
            raise FileNotFoundError(f"{src}: not decoded")
    return read_wav(path)


# ---------------------------------------------------------------------------------------------------------- dsp
def resample(x, sr_in, sr_out):
    if sr_in == sr_out:
        return x
    fr = Fraction(sr_out, sr_in).limit_denominator(1000)
    return signal.resample_poly(x, fr.numerator, fr.denominator, axis=0)


def apply_speed(x, sr, speed):
    """Tape-style speed change: faster = higher and shorter."""
    if abs(speed - 1.0) < 1e-3:
        return x
    return resample(x, int(round(sr * speed)), sr)


def lowpass(x, hz, order=4):
    sos = signal.butter(order, min(hz, SR * 0.45), "low", fs=SR, output="sos")
    return signal.sosfilt(sos, x, axis=0)


def highpass(x, hz, order=2):
    sos = signal.butter(order, hz, "high", fs=SR, output="sos")
    return signal.sosfilt(sos, x, axis=0)


def decay_noise(sec, rt60, damp_hz, predelay=0.0):
    n = int(sec * SR)
    t = np.arange(n) / SR
    ir = rng.standard_normal(n) * 10 ** (-3.0 * t / rt60)
    ir = lowpass(ir, damp_hz, 2)
    pd = int(predelay * SR)
    return np.concatenate([np.zeros(pd), ir])


def room_ir(kind):
    """Synthetic impulse responses for the room tails (wet only, no direct sound)."""
    if kind == "small":       # group room / office: dense, short, soft walls eat the top
        ir = decay_noise(0.7, 0.5, 4500, 0.004)
    elif kind == "corridor":  # long hard corridor: flutter between the walls (2.5 m -> ~15 ms) and a 1 s decay
        ir = decay_noise(1.4, 1.0, 5000, 0.006)
        for k in range(1, 24):
            i = int((0.015 * k + rng.uniform(-0.001, 0.001)) * SR)
            if i < len(ir):
                ir[i] += 0.9 * 10 ** (-3.0 * 0.015 * k / 1.0) * (1 if k % 2 else -1) * 6
    elif kind == "large":     # atrium / hall: late, long, darker
        ir = decay_noise(3.2, 2.4, 3200, 0.028)
    elif kind == "stair":     # concrete stairwell: hard, ringing, distinct echoes up and down
        ir = decay_noise(2.6, 2.0, 6000, 0.012)
        for d, a in ((0.042, 8), (0.088, 6), (0.131, 4), (0.177, 3)):
            ir[int(d * SR)] += a
        d = int(0.011 * SR)   # ringing between the parallel concrete walls: a feedback comb at ~90 Hz
        ir = signal.lfilter([1.0], np.r_[1.0, np.zeros(d - 1), -0.5], ir)
    elif kind == "out":       # outside between buildings: a few late slaps off facades, little diffuse sound
        ir = decay_noise(2.2, 1.6, 2200, 0.09) * 0.25
        for d, a in ((0.12, 7), (0.26, 5), (0.45, 4), (0.71, 2.5)):
            i = int(d * SR)
            ir[i:i + 40] += a * np.hanning(40)
        ir = lowpass(ir, 2500, 2)
    else:
        raise ValueError(kind)
    return ir / (np.abs(ir).sum() / 12.0)


def convolve(x, ir):
    return np.stack([signal.fftconvolve(x[:, c], ir) for c in range(x.shape[1])], axis=1)


def effect(x, fx):
    if fx == "mid":           # 25-60 m: the crack is gone, some reflections
        dry = lowpass(x, 3200)
        wet = convolve(dry, decay_noise(0.9, 0.7, 2500, 0.02))
        wet[: len(dry)] += dry * 2.5
        return wet
    if fx == "far":           # far away / behind walls: a dull boom with a long roll
        dry = lowpass(highpass(x, 60), 750)
        wet = convolve(dry, decay_noise(2.4, 1.8, 900, 0.05))
        wet[: len(dry)] += dry * 3.0
        return wet
    if fx.startswith("tail:"):
        return convolve(x, room_ir(fx[5:]))
    raise ValueError(fx)


def trim(x, keep_start=False, db=-60.0):
    a = np.abs(x).max(axis=1)
    thr = a.max() * 10 ** (db / 20)
    idx = np.nonzero(a > thr)[0]
    if not len(idx):
        return x
    s = 0 if keep_start else max(0, idx[0] - int(0.002 * SR))
    return x[s: idx[-1] + int(0.01 * SR)]


def fades(x, fade_in=0.005, fade_out=0.005):
    n = len(x)
    i = min(n // 2, int(fade_in * SR))
    o = min(n // 2, int(fade_out * SR))
    if i:
        x[:i] *= np.linspace(0, 1, i)[:, None]
    if o:
        x[-o:] *= np.linspace(1, 0, o)[:, None]
    return x


def piece(spec, default_len=None):
    x, sr = load(spec["src"])
    start = float(spec.get("start", 0.0))
    ln = spec.get("len", default_len)
    a = int(start * sr)
    b = len(x) if ln is None else min(len(x), a + int(float(ln) * sr))
    x = x[a:b].copy()
    if spec.get("fade_cut"):   # the slice was cut off by the next shot: fade it out over the last bit
        o = min(len(x) // 2, int(float(spec["fade_cut"]) * sr))
        x[-o:] *= np.linspace(1, 0, o)[:, None]
    x = apply_speed(x, sr, float(spec.get("speed", 1.0)))
    x = resample(x, sr, SR)
    return x * 10 ** (float(spec.get("gain_db", 0.0)) / 20)


def build(e):
    x = piece(e)
    for layer in e.get("layers", []):
        y = piece(layer)
        d = int(float(layer.get("delay", 0.0)) * SR)
        n = max(len(x), d + len(y))
        out = np.zeros((n, 2))
        out[: len(x)] += x
        out[d: d + len(y)] += y
        x = out
    if e.get("highpass"):
        x = highpass(x, float(e["highpass"]))
    if e.get("lowpass"):
        x = lowpass(x, float(e["lowpass"]))
    if e.get("fx"):
        x = effect(x, e["fx"])
    if e.get("channels", "mono") == "mono":
        x = np.repeat(x.mean(axis=1, keepdims=True), 2, axis=1)
    x = trim(x, keep_start=bool(e.get("fx", "").startswith("tail:")))
    if e.get("max_len"):
        x = x[: int(float(e["max_len"]) * SR)]
    x = fades(x, 0.005, float(e.get("fade_out", 0.005)))
    cat = e.get("category", "World")
    if cat == "Ambience":
        rms = np.sqrt(np.mean(x ** 2)) + 1e-12
        x *= 10 ** (AMB_RMS_DB / 20) / rms
    else:
        peak = np.abs(x).max() + 1e-12
        x *= 10 ** (float(e.get("peak_db", PEAK_DB.get(cat, -3.0))) / 20) / peak
    return np.clip(x, -1.0, 1.0)


def write(name, x, stereo):
    path = os.path.join(OUT, f"S_GS_{name}.wav")
    data = x if stereo else x[:, :1]
    with wave.open(path, "wb") as w:
        w.setnchannels(2 if stereo else 1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes((data * 32767).astype("<i2").tobytes())
    return path


# ---------------------------------------------------------------------------------------------------------- credits
def describe(e):
    parts = [f"klipt {e.get('start', 0)}–{'slutt' if e.get('len') is None else round(float(e.get('start', 0)) + float(e['len']), 3)} s"]
    if e.get("speed", 1.0) != 1.0:
        parts.append(f"fart ×{e['speed']}")
    if e.get("layers"):
        parts.append(f"{len(e['layers'])} lag blandet inn")
    if e.get("highpass") or e.get("lowpass"):
        parts.append("filtrert")
    if e.get("fx"):
        parts.append({"mid": "middels avstand (lavpass + refleksjoner)", "far": "langt unna (lavpass + lang rull)"}.get(e["fx"], f"romhale {e['fx'][5:]} (syntetisk rom)"))
    parts.append(e.get("channels", "mono"))
    parts.append("48 kHz, normalisert")
    return ", ".join(parts)


def write_credits(entries, libs):
    lines = ["# Lydkreditering (generert av Tools/Audio/prep_sounds.py – ikke rediger for hånd)", "",
             "Vi krediterer alle kilder, også der lisensen ikke krever det. Lisenstekstene ligger i `LICENSES/`.",
             "Lyder som ikke står i tabellen er våre egne, syntetisert av `Tools/Audio/gen_sounds.py` (ingen kilde).", "",
             "## Kilder", ""]
    used = sorted({s["src"].split("/", 1)[0] for e in entries for s in [e] + e.get("layers", [])})
    for folder in used:
        L = libs[folder]
        lines.append(f"- **{L['library']}** – {L['publisher']} – {L['license']} – {L['url']} (lastet ned {L['downloaded']})")
    lines += ["", "## Filer", "", "| Fil | Kilde (bibliotek / utgiver) | Originalfil | Lisens | Behandling |", "|---|---|---|---|---|"]
    for e in entries:
        L = libs[e["src"].split("/", 1)[0]]
        orig = e["src"].split("/")[-1]
        extra = "".join(f" + {s['src'].split('/')[-1]}" for s in e.get("layers", []))
        lines.append(f"| S_GS_{e['name']}.wav | {L['library']} / {L['publisher']} | {orig}{extra} | {L['license']} | {describe(e)} |")
    with open(os.path.join(OUT, "CREDITS.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def write_gitignore(entries, libs):
    names = []
    for e in entries:
        lic = {libs[s["src"].split("/", 1)[0]]["license_id"] for s in [e] + e.get("layers", [])}
        if any(l.startswith(NO_SHARE) for l in lic):
            names.append(f"S_GS_{e['name']}.wav")
    lines = ["# Generert av Tools/Audio/prep_sounds.py: lyder laget av filer som ikke kan deles videre (Sonniss-lisensen).",
             "# Disse ligger bare lokalt; andre som kloner prosjektet får de syntetiske lydene."]
    lines += [n for n in names] + [f"{n}.imported" for n in names]
    with open(os.path.join(OUT, ".gitignore"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", help="only names starting with this")
    ap.add_argument("--dry", action="store_true", help="check sources only")
    a = ap.parse_args()
    with open(os.path.join(HERE, "sources.json"), encoding="utf-8") as f:
        data = json.load(f)
    libs, entries = data["libraries"], data["sounds"]
    names = [e["name"] for e in entries]
    dup = {n for n in names if names.count(n) > 1}
    if dup:
        sys.exit(f"duplicate names in sources.json: {sorted(dup)}")
    todo = [e for e in entries if not a.only or e["name"].startswith(a.only)]
    compressed = sorted({lib_path(s["src"]) for e in todo for s in [e] + e.get("layers", [])
                         if os.path.splitext(s["src"])[1].lower() != ".wav"})
    decode_compressed(compressed)
    os.makedirs(OUT, exist_ok=True)
    ok = 0
    for e in todo:
        try:
            x = build(e)
        except Exception as ex:
            print(f"SKIP {e['name']}: {ex}")
            continue
        if not a.dry:
            write(e["name"], x, e.get("channels", "mono") == "stereo")
        ok += 1
        print(f"{'checked' if a.dry else 'wrote'} S_GS_{e['name']}  {len(x) / SR:.2f}s")
    if not a.dry and not a.only:
        write_credits(entries, libs)
        write_gitignore(entries, libs)
    print(f"{ok}/{len(todo)} sounds")


if __name__ == "__main__":
    main()
