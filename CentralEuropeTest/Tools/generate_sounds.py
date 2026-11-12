#!/usr/bin/env python3
"""Synthesises the prototype's sound set (numpy only) into ../SourceArt/Audio as 16-bit mono WAV.

These are placeholders made from filtered noise and sine sweeps - functional, not recordings.
Content/Python/init_unreal.py imports them into /Game/CE/Audio the next time the editor starts;
UCEAudioSubsystem plays them by file name. Replace any of them by importing a better sound under
the same asset name.

Loops (Amb_*, Cart_Engine) are built in the frequency domain, so they are exactly periodic.
"""
import argparse
import os
import struct

import numpy as np

SR = 44100
rng = np.random.default_rng(7)


def t_axis(dur):
    return np.arange(int(SR * dur)) / SR


def shaped_noise(n, shape):
    """Noise whose magnitude spectrum follows shape(freq_hz). Periodic over n samples."""
    spec = np.fft.rfft(rng.standard_normal(n))
    freqs = np.fft.rfftfreq(n, 1 / SR)
    out = np.fft.irfft(spec * shape(freqs), n)
    return out / (np.abs(out).max() + 1e-9)


def band(lo, hi, slope=2.0):
    def f(freq):
        freq = np.maximum(freq, 1.0)
        return 1.0 / (1.0 + (lo / freq) ** (2 * slope)) / (1.0 + (freq / hi) ** (2 * slope))
    return f


def env_exp(n, decay, attack=0.002):
    t = np.arange(n) / SR
    return np.minimum(t / attack, 1.0) * np.exp(-t / decay)


def sweep(dur, f0, f1):
    t = t_axis(dur)
    freq = f0 * (f1 / f0) ** (t / dur)
    return np.sin(2 * np.pi * np.cumsum(freq) / SR)


def mix(*parts):
    n = max(len(p) for p in parts)
    out = np.zeros(n)
    for p in parts:
        out[:len(p)] += p
    return out


def fade_out(x, dur=0.01):
    k = min(int(SR * dur), len(x))
    x = x.copy()
    x[-k:] *= np.linspace(1, 0, k)
    return x


def normalise(x, peak):
    return x / (np.abs(x).max() + 1e-9) * peak


def click(freq=2500.0, decay=0.006, dur=0.04):
    n = int(SR * dur)
    return shaped_noise(n, band(freq * 0.5, freq * 2.5)) * env_exp(n, decay, 0.0005)


def gunshot(dur, crack_hz, body_decay, thump_f0, thump_level, tail_decay, tail_hi):
    n = int(SR * dur)
    crack = shaped_noise(n, band(crack_hz, 9000)) * env_exp(n, 0.004, 0.0003)
    body = shaped_noise(n, band(150, 4500)) * env_exp(n, body_decay, 0.0005)
    thump = sweep(dur, thump_f0, 38.0) * env_exp(n, body_decay * 1.6, 0.001) * thump_level
    tail = shaped_noise(n, band(120, tail_hi, 1.0)) * env_exp(n, tail_decay, 0.03) * 0.35   # distant slap-back
    return fade_out(mix(crack * 0.9, body, thump, tail), 0.05)


def footstep(seed_gain):
    n = int(SR * 0.16)
    heel = shaped_noise(n, band(70, 900)) * env_exp(n, 0.022, 0.002)
    grit = shaped_noise(n, band(1500, 6000)) * env_exp(n, 0.035, 0.004) * 0.35 * seed_gain
    toe = np.zeros(n)
    k = int(SR * 0.05)
    toe[k:] = (shaped_noise(n - k, band(120, 1400)) * env_exp(n - k, 0.018, 0.002)) * 0.5
    return fade_out(mix(heel, grit, toe))


def whoosh(dur=0.28):
    n = int(SR * dur)
    t = np.arange(n) / n
    # bright centre: cross-fade a low and a high band through the swing
    low = shaped_noise(n, band(250, 1200))
    high = shaped_noise(n, band(1200, 5000))
    bell = np.sin(np.pi * t) ** 2
    return (low * (1 - bell) * 0.6 + high * bell) * np.sin(np.pi * t) ** 1.5


def thud(f0=110.0, dur=0.22, grit=0.5):
    n = int(SR * dur)
    return fade_out(mix(sweep(dur, f0, 45.0) * env_exp(n, 0.05, 0.001),
                        shaped_noise(n, band(300, 3000)) * env_exp(n, 0.02, 0.0008) * grit))


def splash(dur, weight):
    n = int(SR * dur)
    hit = shaped_noise(n, band(200, 3500)) * env_exp(n, 0.05 * weight, 0.004)
    spray = shaped_noise(n, band(2500, 9000)) * env_exp(n, 0.22 * weight, 0.03) * 0.5
    bloop = sweep(dur, 380.0, 120.0) * env_exp(n, 0.06, 0.004) * 0.35
    return fade_out(mix(hit, spray, bloop), 0.08)


def slow_mod(n, rate_hz, depth, seed):
    """Smooth periodic amplitude modulation (integer number of cycles -> loop safe)."""
    r = np.random.default_rng(seed)
    t = np.arange(n) / n
    cycles = max(int(round(rate_hz * n / SR)), 1)
    m = np.zeros(n)
    for k in range(1, 4):
        m += np.sin(2 * np.pi * (cycles * k * t + r.random())) / k
    m = (m - m.min()) / (m.max() - m.min())
    return 1.0 - depth + depth * m


def loop_wind(dur=8.0):
    n = int(SR * dur)
    low = shaped_noise(n, band(60, 500, 1.0)) * slow_mod(n, 0.25, 0.6, 1)
    hiss = shaped_noise(n, band(700, 3500, 1.0)) * slow_mod(n, 0.375, 0.8, 2) * 0.35
    return low + hiss


def loop_water(dur, lo, hi, flutter, rumble):
    n = int(SR * dur)
    body = shaped_noise(n, band(lo, hi, 1.0)) * slow_mod(n, flutter, 0.25, 3)
    spray = shaped_noise(n, band(3000, 10000, 1.0)) * slow_mod(n, flutter * 2, 0.3, 4) * 0.3
    low = shaped_noise(n, band(40, 220, 1.5)) * rumble
    return body + spray + low


def loop_lake(dur=8.0):
    n = int(SR * dur)
    t = np.arange(n) / n
    wash = shaped_noise(n, band(300, 2500, 1.0))
    # four gentle wave laps per loop, each with a soft swell and a brighter break
    laps = np.clip(np.sin(2 * np.pi * 4 * t), 0, 1) ** 3
    sparkle = shaped_noise(n, band(3000, 9000, 1.0)) * np.clip(np.sin(2 * np.pi * 4 * t - 0.6), 0, 1) ** 6 * 0.4
    return wash * (0.15 + 0.85 * laps) + sparkle


def loop_birds(dur=8.0):
    n = int(SR * dur)
    out = np.zeros(n)
    r = np.random.default_rng(11)
    for _ in range(26):
        start = r.integers(0, n - int(SR * 0.9))
        base = r.uniform(2200, 5200)
        gain = r.uniform(0.25, 1.0)
        for _note in range(r.integers(2, 6)):
            d = r.uniform(0.05, 0.14)
            k = int(SR * d)
            tt = np.arange(k) / k
            f = base * (1 + r.uniform(-0.25, 0.35) * tt + 0.04 * np.sin(2 * np.pi * r.uniform(18, 40) * tt * d))
            note = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.sin(np.pi * tt) ** 2 * gain
            if start + k < n:
                out[start:start + k] += note
            start += k + int(SR * r.uniform(0.02, 0.09))
    bed = shaped_noise(n, band(400, 4000, 1.0)) * 0.02      # faint air so the gaps are not digital silence
    return out + bed * np.abs(out).max()


def loop_engine(dur=2.0, f0=55.0):
    n = int(SR * dur)
    t = np.arange(n) / SR
    out = np.zeros(n)
    for k, a in enumerate([1.0, 0.7, 0.5, 0.35, 0.25, 0.18, 0.12, 0.08], start=1):
        out += a * np.sin(2 * np.pi * f0 * k * t + k * 0.7)      # 55 Hz * 2 s = whole cycles -> seamless
    out *= 1.0 + 0.25 * np.sin(2 * np.pi * f0 / 2 * t)             # firing-order lump
    out = np.tanh(out * 1.4)
    return out + shaped_noise(n, band(800, 5000, 1.0)) * 0.12


def write_wav(path, x):
    pcm = np.clip(np.round(x * 32767), -32768, 32767).astype("<i2")
    data = pcm.tobytes()
    with open(path, "wb") as f:
        f.write(b"RIFF" + struct.pack("<I", 36 + len(data)) + b"WAVE")
        f.write(b"fmt " + struct.pack("<IHHIIHH", 16, 1, 1, SR, SR * 2, 2, 16))
        f.write(b"data" + struct.pack("<I", len(data)) + data)


def build():
    s = {}
    # --- weapons
    s["Gun_Pistol"] = (gunshot(0.7, 2500, 0.035, 220, 0.6, 0.18, 3000), 0.95)
    s["Gun_Rifle"] = (gunshot(0.8, 3000, 0.045, 170, 0.8, 0.22, 3500), 0.95)
    s["Gun_Sniper"] = (gunshot(1.6, 2000, 0.09, 130, 1.1, 0.55, 2500), 0.98)
    s["Gun_Empty"] = (fade_out(mix(click(3200, 0.004), np.r_[np.zeros(int(SR * 0.012)), click(1800, 0.006) * 0.6])), 0.5)
    mag_out = mix(click(1500, 0.012, 0.08), np.r_[np.zeros(int(SR * 0.09)), shaped_noise(int(SR * 0.12), band(600, 3000)) * env_exp(int(SR * 0.12), 0.04, 0.01) * 0.5])
    s["Reload_Start"] = (fade_out(mag_out), 0.6)
    mag_in = mix(click(1200, 0.015, 0.08), np.r_[np.zeros(int(SR * 0.16)), click(2600, 0.008, 0.06)], np.r_[np.zeros(int(SR * 0.2)), click(900, 0.02, 0.1) * 0.8])
    s["Reload_End"] = (fade_out(mag_in), 0.65)
    s["Equip"] = (fade_out(mix(shaped_noise(int(SR * 0.14), band(500, 2500)) * env_exp(int(SR * 0.14), 0.04, 0.02) * 0.6,
                                np.r_[np.zeros(int(SR * 0.1)), click(2000, 0.008, 0.06)])), 0.5)
    s["Bullet_Impact"] = (fade_out(mix(click(4000, 0.004, 0.05), thud(160, 0.15, 0.9) * 0.7,
                                       shaped_noise(int(SR * 0.25), band(1000, 6000)) * env_exp(int(SR * 0.25), 0.06, 0.01) * 0.3)), 0.7)
    s["Hit_Confirm"] = (sweep(0.06, 1400, 1900) * env_exp(int(SR * 0.06), 0.02, 0.002), 0.35)
    # --- melee
    s["Melee_Swing"] = (whoosh(0.28), 0.6)
    s["Melee_Hit"] = (thud(120, 0.24, 0.7), 0.9)
    s["Target_Down"] = (fade_out(mix(thud(80, 0.5, 0.4), np.r_[np.zeros(int(SR * 0.12)), thud(60, 0.4, 0.6) * 0.8])), 0.85)
    # --- movement
    for i, g in enumerate([0.6, 1.0, 0.8, 1.2], start=1):
        s["Footstep_%d" % i] = (footstep(g), 0.55)
    s["Jump"] = (shaped_noise(int(SR * 0.16), band(300, 2500)) * np.sin(np.pi * np.arange(int(SR * 0.16)) / int(SR * 0.16)) ** 2, 0.3)
    s["Land"] = (fade_out(mix(thud(95, 0.25, 0.6), footstep(1.2) * 0.6)), 0.75)
    s["Splash"] = (splash(0.9, 1.0), 0.8)
    s["Swim_Stroke"] = (splash(0.5, 0.5), 0.4)
    # --- loops
    s["Cart_Engine"] = (loop_engine(), 0.6)
    s["Amb_Wind"] = (loop_wind(), 0.7)
    s["Amb_Birds"] = (loop_birds(), 0.5)
    s["Amb_Lake"] = (loop_lake(), 0.5)
    s["Amb_River"] = (loop_water(8.0, 350, 3500, 0.5, 0.15), 0.6)
    s["Amb_Waterfall"] = (loop_water(8.0, 150, 5000, 0.25, 0.7), 0.8)
    # every one-shot ends on silence (no click); loops are left exactly periodic
    return {k: normalise(v if k.startswith(("Amb_", "Cart_")) else fade_out(v, 0.008), peak) for k, (v, peak) in s.items()}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SourceArt", "Audio"))
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    total = 0
    for name, x in sorted(build().items()):
        assert np.isfinite(x).all() and np.abs(x).max() <= 1.0, name
        write_wav(os.path.join(args.out, name + ".wav"), x)
        loop = name.startswith(("Amb_", "Cart_"))
        seam = abs(x[0] - x[-1]) if loop else abs(x[-1])
        typical = np.abs(np.diff(x)).mean()
        total += len(x) * 2
        print("%-14s %5.2f s  peak %.2f  rms %.3f  %s %.4f (mean step %.4f)" % (
            name, len(x) / SR, np.abs(x).max(), np.sqrt((x ** 2).mean()), "loop seam" if loop else "tail", seam, typical))
        # a loop seam must not be a click: no bigger than an ordinary sample-to-sample step x8
        if loop:
            assert seam < max(typical * 8, 0.02), "loop seam click in " + name
        else:
            assert seam < 0.01, "abrupt tail in " + name
    print("%d files, %.1f MB" % (len(build()), total / 1e6))


if __name__ == "__main__":
    main()
