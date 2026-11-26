"""Procedural sound effects:  python3 Tools/Audio/gen_sounds.py  ->  SourceArt/Sounds/S_GS_*.wav

Synthesized (numpy only, no downloads): layered noise bursts, filtered body thumps and reverb tails.
Scanned/recorded sounds can replace any of these later by importing a SoundWave with the same name
into /Game/Sounds (the game only looks up names).
"""
import os
import sys
import wave

import numpy as np

SR = 44100
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "SourceArt", "Sounds")
rng = np.random.default_rng(7)


def t(sec):
    return np.arange(int(sec * SR)) / SR


def noise(sec):
    return rng.standard_normal(int(sec * SR))


def lowpass(x, cutoff):
    # one-pole, run twice for 12 dB/oct
    a = np.exp(-2 * np.pi * cutoff / SR)
    for _ in range(2):
        y = np.empty_like(x)
        acc = 0.0
        b = 1 - a
        # vectorised IIR via cumulative trick is unstable for long signals; plain loop in chunks is fine here
        for i in range(len(x)):
            acc = b * x[i] + a * acc
            y[i] = acc
        x = y
    return x


def highpass(x, cutoff):
    return x - lowpass(x, cutoff)


def env(n, attack, decay):
    tt = np.arange(n) / SR
    e = np.exp(-tt / decay)
    a = int(attack * SR)
    if a > 0:
        e[:a] *= np.linspace(0, 1, a)
    return e


def reverb(x, length=1.2, mix=0.35, damp=2500):
    ir = noise(length) * np.exp(-t(length) / (length / 5))
    ir = lowpass(ir, damp)
    ir /= np.abs(ir).sum() / 8
    wet = np.convolve(x, ir)[: len(x) + len(ir) - 1]
    out = np.zeros(len(wet))
    out[: len(x)] += x
    return out * (1 - mix) + wet * mix


def save(name, x, peak=0.9):
    x = np.asarray(x, dtype=np.float64)
    x = x / (np.abs(x).max() + 1e-9) * peak
    fade = min(len(x), int(0.01 * SR))
    x[-fade:] *= np.linspace(1, 0, fade)
    os.makedirs(OUT, exist_ok=True)
    with wave.open(os.path.join(OUT, f"S_GS_{name}.wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes((x * 32767).astype("<i2").tobytes())
    print("wrote", name, f"{len(x) / SR:.2f}s")


def gunshot(name, body_hz, crack, length, tail, mix):
    n = int(length * SR)
    tt = t(length)
    crack_n = highpass(noise(length), 2500) * env(n, 0.0005, 0.006) * crack
    blast = lowpass(noise(length), 1800) * env(n, 0.001, 0.05)
    thump = np.sin(2 * np.pi * body_hz * tt * (1 + 0.6 * np.exp(-tt * 40))) * env(n, 0.001, 0.08)
    mech = highpass(noise(length), 4000) * env(n, 0.0, 0.002) * 0.3
    x = crack_n + blast * 1.2 + thump * 1.5 + mech
    x = np.tanh(x * 2.5)
    save(name, reverb(x, tail, mix, 2000))


def suppressed():
    length = 0.35
    n = int(length * SR)
    tt = t(length)
    x = lowpass(noise(length), 1200) * env(n, 0.001, 0.025) + np.sin(2 * np.pi * 90 * tt) * env(n, 0.001, 0.03) * 0.6
    x += highpass(noise(length), 3000) * env(n, 0.0, 0.004) * 0.4  # action clack
    save("Shot_Silenced", reverb(x, 0.4, 0.2), 0.6)


def distant():
    length = 2.5
    n = int(length * SR)
    x = lowpass(noise(length), 500) * env(n, 0.01, 0.15)
    save("Shot_Distant", reverb(x, 2.2, 0.6, 800), 0.7)


def step(name, bright, decay, grit):
    for v in range(4):
        length = 0.25
        n = int(length * SR)
        heel = lowpass(noise(length), 400 + 200 * v) * env(n, 0.002, decay)
        scuff = highpass(noise(length), bright) * env(n, 0.01, decay * 1.5) * grit
        save(f"{name}_{v + 1}", heel + scuff, 0.5)


def metal_click(name, freqs, length=0.25, noise_amt=0.6):
    n = int(length * SR)
    tt = t(length)
    x = highpass(noise(length), 3000) * env(n, 0.0, 0.004) * noise_amt
    for f in freqs:
        x += np.sin(2 * np.pi * f * tt) * env(n, 0.0, 0.03) * 0.3
    save(name, x, 0.6)


def whoosh():
    length = 0.35
    n = int(length * SR)
    w = np.sin(np.linspace(0, np.pi, n)) ** 2
    save("Melee_Swing", lowpass(noise(length), 900) * w, 0.6)


def hit_body():
    length = 0.2
    n = int(length * SR)
    x = lowpass(noise(length), 350) * env(n, 0.001, 0.03) + np.sin(2 * np.pi * 70 * t(length)) * env(n, 0, 0.04)
    save("Hit_Body", x, 0.8)


def hitmarker():
    length = 0.08
    n = int(length * SR)
    x = np.sin(2 * np.pi * 2400 * t(length)) * env(n, 0, 0.02) + highpass(noise(length), 5000) * env(n, 0, 0.004) * 0.5
    save("Hit_Marker", x, 0.5)


def impact():
    for v in range(3):
        length = 0.3
        n = int(length * SR)
        x = highpass(noise(length), 1500 + v * 800) * env(n, 0, 0.02) + lowpass(noise(length), 600) * env(n, 0, 0.04) * 0.5
        save(f"Impact_{v + 1}", x, 0.5)


def land():
    length = 0.4
    n = int(length * SR)
    x = lowpass(noise(length), 250) * env(n, 0.002, 0.06) + highpass(noise(length), 2000) * env(n, 0.005, 0.05) * 0.2
    save("Land", x, 0.7)


def pickup():
    length = 0.25
    n = int(length * SR)
    tt = t(length)
    x = highpass(noise(length), 2500) * env(n, 0, 0.01) * 0.5 + np.sin(2 * np.pi * 900 * tt) * env(n, 0, 0.02) * 0.3
    save("Pickup", x, 0.5)


def hurt_sounds():
    """What you hear when YOU are hit:  python Tools/Audio/gen_sounds.py hurt  (writes only these five)."""
    # Bullet on your vest/helmet: metallic plink/clonk with a short dull knock
    length = 0.35
    n = int(length * SR)
    tt = t(length)
    x = lowpass(noise(length), 600) * env(n, 0.0, 0.02) * 0.8
    for f, d, a in ((1900, 0.12, 0.5), (3100, 0.08, 0.35), (4700, 0.05, 0.25)):
        x += np.sin(2 * np.pi * f * tt) * env(n, 0.0, d) * a
    save("Hit_Armor", x, 0.7)

    # Headshot you survive: hard crack and a deep thud
    length = 0.4
    n = int(length * SR)
    tt = t(length)
    x = highpass(noise(length), 3000) * env(n, 0.0, 0.006) + np.sin(2 * np.pi * 55 * tt) * env(n, 0.001, 0.06) * 1.2
    save("Hit_Head", x, 0.85)

    # Vest/helmet breaks: a few metallic clicks and a crunch
    length = 0.6
    n = int(length * SR)
    tt = t(length)
    x = lowpass(noise(length), 1200) * env(n, 0.002, 0.15) * 0.6
    for start, fs in ((0.0, (1700, 2900)), (0.06, (2300, 3600)), (0.13, (1500, 4100))):
        i0 = int(start * SR)
        m = n - i0
        ct = tt[:m]
        click = highpass(rng.standard_normal(m), 3000) * env(m, 0.0, 0.004) * 0.5
        for f in fs:
            click += np.sin(2 * np.pi * f * ct) * env(m, 0.0, 0.04) * 0.35
        x[i0:] += click
    save("Armor_Break", x, 0.8)

    # One heartbeat ("lub-dub"), played once per beat by the game
    length = 0.55
    n = int(length * SR)
    tt = t(length)
    x = np.zeros(n)
    for start, f, a in ((0.0, 48, 1.0), (0.17, 40, 0.75)):
        i0 = int(start * SR)
        m = n - i0
        x[i0:] += np.sin(2 * np.pi * f * tt[:m]) * env(m, 0.005, 0.07) * a
    save("Heartbeat", lowpass(x, 180), 0.9)

    # Ringing ears after a headshot: 3600 Hz with a faint beat
    length = 1.6
    n = int(length * SR)
    tt = t(length)
    x = (np.sin(2 * np.pi * 3600 * tt) + 0.5 * np.sin(2 * np.pi * 3610 * tt)) * env(n, 0.02, 0.6)
    save("Ear_Ring", x, 0.35)


def loop_save(name, x, peak):
    # crossfade the ends so it loops without a click
    f = int(0.5 * SR)
    x[:f] = x[:f] * np.linspace(0, 1, f) + x[-f:] * np.linspace(1, 0, f)
    save(name, x[:-f], peak)


def wind():
    length = 12
    base = lowpass(noise(length), 300)
    gust = 0.6 + 0.4 * np.sin(2 * np.pi * t(length) / 6) * np.sin(2 * np.pi * t(length) / 2.3 + 1)
    loop_save("Amb_Wind", base * gust + highpass(lowpass(noise(length), 1500), 600) * 0.05 * gust, 0.5)


def rain():
    length = 8
    x = highpass(lowpass(noise(length), 6000), 800) * 0.4
    drops = np.zeros(int(length * SR))
    idx = rng.integers(0, len(drops) - 2000, 3000)
    for i in idx:
        d = highpass(noise(0.03), 3000) * env(int(0.03 * SR), 0, 0.004) * rng.uniform(0.2, 1)
        drops[i:i + len(d)] += d
    loop_save("Amb_Rain", x + drops * 0.6, 0.5)


def birds():
    length = 15
    x = lowpass(noise(length), 200) * 0.02
    for _ in range(22):
        start = rng.uniform(0, length - 2)
        f0 = rng.uniform(2500, 5000)
        notes = rng.integers(2, 7)
        for k in range(notes):
            d = rng.uniform(0.05, 0.15)
            tt = t(d)
            f = f0 * (1 + 0.3 * np.sin(2 * np.pi * rng.uniform(5, 20) * tt)) * (1 + rng.uniform(-0.2, 0.3) * tt / d)
            chirp = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.sin(np.linspace(0, np.pi, len(tt)))
            i0 = int((start + k * (d + 0.05)) * SR)
            x[i0:i0 + len(chirp)] += chirp * rng.uniform(0.05, 0.2)
    loop_save("Amb_Birds", x, 0.35)


def city_night():
    length = 12
    x = lowpass(noise(length), 120) * 0.5 + np.sin(2 * np.pi * 60 * t(length)) * 0.02
    loop_save("Amb_Night", x, 0.3)


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "hurt":
        hurt_sounds()
        raise SystemExit
    gunshot("Shot_Pistol", 160, 0.8, 0.9, 0.9, 0.3)
    gunshot("Shot_SMG", 180, 0.7, 0.7, 0.8, 0.3)
    gunshot("Shot_Rifle", 120, 1.2, 1.3, 1.4, 0.35)
    gunshot("Shot_Sniper", 80, 1.6, 2.0, 2.2, 0.45)
    gunshot("Shot_Shotgun", 70, 0.9, 1.4, 1.6, 0.4)
    gunshot("Shot_LMG", 110, 1.1, 1.1, 1.2, 0.35)
    gunshot("Shot_Launcher", 50, 0.3, 2.0, 2.4, 0.5)
    suppressed()
    distant()
    step("Step_Hard", 3000, 0.03, 0.4)
    step("Step_Grass", 1500, 0.06, 0.8)
    metal_click("Reload_Out", [1800, 2600])
    metal_click("Reload_In", [1200, 2100, 3100], 0.3, 0.9)
    metal_click("Slide", [900, 1600], 0.35, 1.0)
    metal_click("Dry_Fire", [3500], 0.1, 0.5)
    for v, fs in enumerate(([4200, 6100, 8300], [3800, 5600, 7900], [4600, 6900, 9100])):
        metal_click(f"Shell_{v + 1}", fs, 0.3, 0.2)
    metal_click("Pump_Back", [700, 1300], 0.3, 1.0)
    metal_click("Pump_Fwd", [900, 1700], 0.25, 1.0)
    whoosh()
    hit_body()
    hitmarker()
    impact()
    land()
    pickup()
    wind()
    rain()
    birds()
    city_night()
    hurt_sounds()
