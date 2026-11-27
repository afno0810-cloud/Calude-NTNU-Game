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


def flag_sounds():
    """Radio cues for Capture the Flag:  python Tools/Audio/gen_sounds.py flag  (writes only these four).

    A hand radio breaking in: squelch click, one to three plain tones through a narrow speaker, squelch tail.
    Sober on purpose: no fanfare, no arcade jingle.
    """
    def cue(name, notes, peak):
        # notes: (start s, frequency Hz, length s)
        length = max(s + d for s, _, d in notes) + 0.22
        n = int(length * SR)
        x = np.zeros(n)
        for start, f, d in notes:
            i0 = int(start * SR)
            m = min(n - i0, int(d * SR))
            tt = np.arange(m) / SR
            tone = np.sin(2 * np.pi * f * tt) + 0.25 * np.sin(2 * np.pi * 2 * f * tt)
            e = np.minimum(1.0, tt / 0.008) * np.minimum(1.0, (d - tt) / 0.03).clip(0, 1)
            x[i0:i0 + m] += tone * e
        # carrier hiss while the channel is open, squelch click at both ends
        hiss = highpass(lowpass(noise(length), 3200), 700) * 0.10
        x += hiss
        click = int(0.018 * SR)
        x[:click] += highpass(rng.standard_normal(click), 1500) * np.linspace(1, 0, click) * 0.9
        tail0 = n - int(0.12 * SR)
        m = n - tail0
        x[tail0:] += highpass(lowpass(rng.standard_normal(m), 3500), 900) * env(m, 0.0, 0.03) * 0.8
        # small radio speaker: nothing below 400 Hz or above 3 kHz, a little clipped
        x = highpass(lowpass(x, 3000), 400)
        x = np.tanh(x * 1.6)
        save(name, x, peak)

    cue("Flag_Info", [(0.03, 880, 0.11)], 0.5)                                       # something happened
    cue("Flag_Alert", [(0.03, 620, 0.13), (0.21, 620, 0.13)], 0.55)                  # your flag is in danger
    cue("Flag_Good", [(0.03, 660, 0.10), (0.16, 880, 0.14)], 0.5)                    # your flag is safe again
    cue("Flag_Score", [(0.03, 660, 0.10), (0.15, 880, 0.10), (0.27, 990, 0.20)], 0.55)   # your team captured


def bullet_sounds():
    # Bullet_Crack: the supersonic crack of a bullet passing close by. Its shock wave is an "N-wave": the pressure
    # jumps up at once, falls straight through to below zero over about a millisecond and snaps back. A little
    # bright noise on the edges and a short slap off the surroundings.
    length = 0.35
    n = int(length * SR)
    x = np.zeros(n)
    k = 45                                    # ~1 ms
    x[:k] = np.linspace(1.0, -1.0, k)
    hiss = highpass(noise(length), 3000)
    hiss /= np.abs(hiss).max() + 1e-9
    x += hiss * env(n, 0.0, 0.004) * 0.5
    x = reverb(x, 0.3, 0.25)
    save("Bullet_Crack", x, 0.9)

    # Bullet_Whiz: a slower (subsonic) bullet passing: a band of rushing air that swells and dies, with a
    # falling tone as it goes by (doppler), 1400 down to 700 Hz.
    length = 0.3
    n = int(length * SR)
    band = highpass(lowpass(noise(length), 3000), 600)
    band /= np.abs(band).max() + 1e-9
    freq = np.linspace(1400, 700, n)
    tone = np.sin(2 * np.pi * np.cumsum(freq) / SR)
    e = env(n, 0.08, 0.1)
    x = (band * 0.7 + tone * 0.3) * e
    save("Bullet_Whiz", x, 0.6)


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


def round11_sounds():
    """Sound round 1 (plan-11):  python Tools/Audio/gen_sounds.py lyd1  (writes only these; never the real ones).

    What the downloaded packs (Snake's gun sounds, Kenney impacts/interface) do not have: explosions, gear rattle
    while sprinting, body sliding on a floor, radio squelch, and more variants of the passing-bullet crack/whiz.
    Uses scipy filters (fast) instead of the per-sample loops above.
    """
    from scipy import signal as sg

    def lp(x, hz, order=4):
        return sg.sosfilt(sg.butter(order, hz, "low", fs=SR, output="sos"), x)

    def hp(x, hz, order=2):
        return sg.sosfilt(sg.butter(order, hz, "high", fs=SR, output="sos"), x)

    def bp(x, lo, hi):
        return sg.sosfilt(sg.butter(2, [lo, hi], "band", fs=SR, output="sos"), x)

    def tail(x, sec, rt60, damp, mix, pre=0.0):
        n = int(sec * SR)
        tt = np.arange(n) / SR
        ir = lp(rng.standard_normal(n) * 10 ** (-3 * tt / rt60), damp, 2)
        ir = np.concatenate([np.zeros(int(pre * SR)), ir])
        ir /= np.abs(ir).sum() / 10
        wet = sg.fftconvolve(x, ir)
        out = np.zeros(len(wet))
        out[: len(x)] += x
        return out * (1 - mix) + wet * mix

    # --- Explosions: a grenade / rocket / mine is a supersonic pressure spike (a hard N-wave with a very fast rise),
    # a heavy low "body" from the expanding gas (20-80 Hz), the crackle of fragments and debris, and the room/landscape.
    def blast(length, body_hz, crack, low_cut, rt60, damp, mix, pre, seed):
        r = np.random.default_rng(seed)
        n = int(length * SR)
        tt = np.arange(n) / SR
        x = np.zeros(n)
        k = int(0.003 * SR)
        x[:k] = np.linspace(1.0, -0.7, k) * crack                            # shock front
        body = np.sin(2 * np.pi * body_hz * tt * (1 + 1.5 * np.exp(-tt * 12))) * np.exp(-tt / 0.35)
        rumble = lp(r.standard_normal(n), 180) * np.exp(-tt / 0.9) * 3.0
        roar = lp(r.standard_normal(n), 1400) * np.exp(-tt / 0.12) * 1.4
        frag = np.zeros(n)                                                   # fragments and stones hitting things
        for _ in range(60):
            i = int(r.uniform(0.02, min(length - 0.1, 1.4)) * SR)
            m = int(0.02 * SR)
            frag[i:i + m] += hp(r.standard_normal(m), 2500) * np.exp(-np.arange(m) / SR / 0.004) * r.uniform(0.05, 0.4)
        x += body * 1.6 + rumble + roar + frag
        x = np.tanh(x * 1.8)                                                 # the microphone/ear overloads
        x = hp(x, low_cut)
        return tail(x, rt60 * 1.4, rt60, damp, mix, pre)

    for v in range(3):
        save(f"Explosion_Close_{v + 1}", blast(3.0, 42 + 6 * v, 1.0, 18, 1.6, 2200, 0.35, 0.06, 100 + v), 0.95)
        save(f"Explosion_Indoor_{v + 1}", blast(3.2, 48 + 5 * v, 0.9, 25, 1.9, 3500, 0.55, 0.015, 200 + v), 0.95)
        far = blast(4.5, 36 + 4 * v, 0.4, 20, 3.0, 600, 0.7, 0.12, 300 + v)
        save(f"Explosion_Far_{v + 1}", lp(far, 500), 0.9)

    # --- Gear rattle while sprinting: fabric swish, magazines and buckles knocking in the pouches.
    for v in range(4):
        r = np.random.default_rng(400 + v)
        length = 0.45
        n = int(length * SR)
        tt = np.arange(n) / SR
        swish = bp(r.standard_normal(n), 600, 3500) * np.sin(np.pi * tt / length) ** 2 * 0.35
        x = swish
        for _ in range(r.integers(3, 6)):
            i = int(r.uniform(0.0, length - 0.08) * SR)
            m = int(0.06 * SR)
            ct = np.arange(m) / SR
            f = r.uniform(1800, 4200)
            clink = (np.sin(2 * np.pi * f * ct) * 0.4 + hp(r.standard_normal(m), 2000) * 0.6) * np.exp(-ct / 0.008)
            x[i:i + m] += clink * r.uniform(0.3, 0.8)
            x[i:i + m] += lp(r.standard_normal(m), 400) * np.exp(-ct / 0.015) * 0.5   # dull knock of a full magazine
        save(f"Gear_Rattle_{v + 1}", x, 0.5)

    # --- Sliding on the floor: hard floors scrape (bright band noise with grit), grass/carpet hiss softly.
    for v in range(2):
        r = np.random.default_rng(500 + v)
        length = 0.9
        n = int(length * SR)
        tt = np.arange(n) / SR
        e = np.minimum(1, tt / 0.04) * np.exp(-np.maximum(0, tt - 0.15) / 0.35)
        grit = np.zeros(n)
        idx = r.integers(0, n - 200, 220)
        grit[idx] = r.uniform(-1, 1, len(idx))
        hard = bp(r.standard_normal(n), 900, 5000) * 0.5 + hp(grit, 3000) * 2.0 + lp(r.standard_normal(n), 250) * 0.4
        save(f"Slide_Hard_{v + 1}", hard * e, 0.55)
        soft = bp(r.standard_normal(n), 300, 2200) * 0.6 + lp(r.standard_normal(n), 200) * 0.3
        save(f"Slide_Soft_{v + 1}", soft * e, 0.45)

    # --- Radio squelch: the click and hiss of a hand radio opening/closing.
    for v in range(2):
        r = np.random.default_rng(600 + v)
        length = 0.16
        n = int(length * SR)
        tt = np.arange(n) / SR
        x = bp(r.standard_normal(n), 900, 3200) * np.exp(-tt / 0.05) * 0.6
        c = int(0.004 * SR)
        x[:c] += np.linspace(1, 0, c) * (1 if v == 0 else -1)
        save(f"Radio_Squelch_{v + 1}", np.tanh(x * 2), 0.5)

    # --- Passing bullets: three cracks (different N-wave lengths = different calibres/miss distances) and whizzes.
    for v in range(3):
        r = np.random.default_rng(700 + v)
        length = 0.4
        n = int(length * SR)
        tt = np.arange(n) / SR
        x = np.zeros(n)
        k = int((0.0007 + 0.0004 * v) * SR)
        x[:k] = np.linspace(1.0, -1.0, k)
        x += hp(r.standard_normal(n), 3000) * np.exp(-tt / 0.003) * 0.4
        x = tail(x, 0.5, 0.35 + 0.15 * v, 4000, 0.3, 0.008)
        save(f"Bullet_Crack_{v + 1}", x, 0.9)
        length = 0.3 + 0.05 * v
        n = int(length * SR)
        tt = np.arange(n) / SR
        band = bp(r.standard_normal(n), 500, 3200)
        band /= np.abs(band).max() + 1e-9
        freq = np.linspace(1500 - 100 * v, 650 + 50 * v, n)
        tone = np.sin(2 * np.pi * np.cumsum(freq) / SR)
        e = np.sin(np.pi * np.clip(tt / length, 0, 1)) ** 3
        save(f"Bullet_Whiz_{v + 1}", (band * 0.7 + tone * 0.3) * e, 0.6)


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "lyd1":
        round11_sounds()
        raise SystemExit
    if len(sys.argv) > 1 and sys.argv[1] == "hurt":
        hurt_sounds()
        raise SystemExit
    if len(sys.argv) > 1 and sys.argv[1] == "flag":
        flag_sounds()
        raise SystemExit
    if len(sys.argv) > 1 and sys.argv[1] == "bullet":
        bullet_sounds()
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
    flag_sounds()
    bullet_sounds()
