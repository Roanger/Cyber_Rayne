#!/usr/bin/env python3
"""Synthesize Cyber Rayne's sound effects (.wav) and starter music tracks (.ogg).

All 32 sound effects and 9 music tracks listed in ASSET_LIST.md section 9 are
generated deterministically using NumPy. Any file can be replaced later with an
externally authored .wav or .ogg; pass --skip-existing to keep custom files.

Usage:
    python3 CyberRayne/tools/generate_audio.py [--skip-existing] [--sfx-only]
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

import numpy as np

SR = 44100


# ---------------------------------------------------------------------------
# DSP building blocks
# ---------------------------------------------------------------------------

def t_array(duration: float) -> np.ndarray:
    """Time array in seconds for a given duration."""
    return np.linspace(0.0, duration, int(round(SR * duration)), endpoint=False, dtype=np.float64)


def sine(freq: float | np.ndarray, t: np.ndarray, phase: float = 0.0) -> np.ndarray:
    if np.ndim(freq) == 0:
        return np.sin(2.0 * np.pi * float(freq) * t + phase)
    inst_phase = 2.0 * np.pi * np.cumsum(freq) / SR + phase
    return np.sin(inst_phase)


def saw(freq: float | np.ndarray, t: np.ndarray) -> np.ndarray:
    if np.ndim(freq) == 0:
        p = (t * float(freq)) % 1.0
    else:
        p = (np.cumsum(freq) / SR) % 1.0
    return 2.0 * p - 1.0


def square(freq: float | np.ndarray, t: np.ndarray, duty: float = 0.5) -> np.ndarray:
    if np.ndim(freq) == 0:
        p = (t * float(freq)) % 1.0
    else:
        p = (np.cumsum(freq) / SR) % 1.0
    return np.where(p < duty, 1.0, -1.0)


def triangle(freq: float | np.ndarray, t: np.ndarray) -> np.ndarray:
    if np.ndim(freq) == 0:
        p = (t * float(freq)) % 1.0
    else:
        p = (np.cumsum(freq) / SR) % 1.0
    return 4.0 * np.abs(p - 0.5) - 1.0


def white_noise(n: int, seed: int = 42) -> np.ndarray:
    rng = np.random.default_rng(seed)
    return rng.uniform(-1.0, 1.0, n)


def one_pole_lp(sig: np.ndarray, cutoff_hz: float) -> np.ndarray:
    """Simple 1-pole low-pass filter."""
    rc = 1.0 / (2.0 * np.pi * max(cutoff_hz, 20.0))
    dt = 1.0 / SR
    alpha = dt / (rc + dt)
    out = np.empty_like(sig)
    acc = 0.0
    for i in range(len(sig)):
        acc += alpha * (sig[i] - acc)
        out[i] = acc
    return out


def one_pole_hp(sig: np.ndarray, cutoff_hz: float) -> np.ndarray:
    return sig - one_pole_lp(sig, cutoff_hz)


def adsr(n: int, attack: float, decay: float, sustain: float, release: float) -> np.ndarray:
    """ADSR envelope over n samples (times in seconds)."""
    a = max(1, int(attack * SR))
    d = max(1, int(decay * SR))
    r = max(1, int(release * SR))
    if a + d + r > n:
        scale = n / float(a + d + r)
        a = max(1, int(a * scale))
        d = max(1, int(d * scale))
        r = max(1, n - a - d)
    s = max(0, n - a - d - r)
    env = np.concatenate([
        np.linspace(0.0, 1.0, a, endpoint=False),
        np.linspace(1.0, sustain, d, endpoint=False),
        np.full(s, sustain),
        np.linspace(sustain, 0.0, r, endpoint=True),
    ])
    return env[:n]


def exp_decay(t: np.ndarray, rate: float = 12.0, attack_s: float = 0.002) -> np.ndarray:
    env = np.exp(-rate * t)
    att = max(1, int(attack_s * SR))
    if att < len(env):
        env[:att] *= np.linspace(0.0, 1.0, att)
    env[-min(64, len(env)):] *= np.linspace(1.0, 0.0, min(64, len(env)))
    return env


def concat_notes(notes: list[tuple[float, float, str]], duty: float = 0.5) -> np.ndarray:
    """Render a sequence of (freq_hz, duration_s, wave_kind) notes."""
    chunks = []
    for freq, dur, kind in notes:
        t = t_array(dur)
        if freq <= 0.0:
             chunks.append(np.zeros_like(t))
             continue
        if kind == "sine":
            w = sine(freq, t) + 0.25 * sine(freq * 2.0, t)
        elif kind == "square":
            w = square(freq, t, duty=duty)
        elif kind == "saw":
            w = saw(freq, t)
        else:
            w = triangle(freq, t)
        env = adsr(len(t), 0.003, dur * 0.35, 0.55, dur * 0.35)
        chunks.append(w * env)
    return np.concatenate(chunks)


def normalize_mono(sig: np.ndarray, peak: float = 0.70) -> np.ndarray:
    """Remove DC offset, normalize to target peak (~-3 dB), and soft-clip."""
    sig = sig - np.mean(sig)
    mx = float(np.max(np.abs(sig)))
    if mx > 1e-8:
        sig = sig * (peak / mx)
    return np.clip(sig, -0.98, 0.98)


def write_wav_mono(path: Path, sig: np.ndarray, peak: float = 0.70) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    normed = normalize_mono(sig, peak=peak)
    pcm = (normed * 32767.0).astype(np.int16)
    with wave.open(str(path), "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SR)
        wf.writeframes(pcm.tobytes())


def write_music_stereo(path: Path, left: np.ndarray, right: np.ndarray, peak: float = 0.68) -> None:
    """Write stereo audio as .ogg (via ffmpeg libvorbis) or .wav."""
    path.parent.mkdir(parents=True, exist_ok=True)
    mx = max(float(np.max(np.abs(left))), float(np.max(np.abs(right))), 1e-8)
    left = np.clip(left * (peak / mx), -0.98, 0.98)
    right = np.clip(right * (peak / mx), -0.98, 0.98)
    interleaved = np.empty(left.size * 2, dtype=np.int16)
    interleaved[0::2] = (left * 32767.0).astype(np.int16)
    interleaved[1::2] = (right * 32767.0).astype(np.int16)

    if path.suffix.lower() == ".wav" or shutil.which("ffmpeg") is None:
        wav_path = path.with_suffix(".wav")
        with wave.open(str(wav_path), "wb") as wf:
            wf.setnchannels(2)
            wf.setsampwidth(2)
            wf.setframerate(SR)
            wf.writeframes(interleaved.tobytes())
        return

    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tmp:
        tmp_wav = Path(tmp.name)
    try:
        with wave.open(str(tmp_wav), "wb") as wf:
            wf.setnchannels(2)
            wf.setsampwidth(2)
            wf.setframerate(SR)
            wf.writeframes(interleaved.tobytes())
        subprocess.run(
            [
                "ffmpeg", "-y", "-v", "error",
                "-i", str(tmp_wav),
                "-c:a", "libvorbis", "-q:a", "5",
                str(path),
            ],
            check=True,
        )
    finally:
        tmp_wav.unlink(missing_ok=True)


# ---------------------------------------------------------------------------
# 32 Sound Effects (ASSET_LIST.md section 9)
# ---------------------------------------------------------------------------

def build_sfx() -> dict[str, tuple[np.ndarray, float]]:
    """Return {name: (samples, peak_amplitude)} for all 32 sound effects."""
    sfx: dict[str, tuple[np.ndarray, float]] = {}

    # --- Menus and interface (7) ---
    # ui_move: Crisp, subtle high-tech tick; heard constantly.
    t = t_array(0.036)
    sig = (sine(1450.0, t) * 0.7 + triangle(2900.0, t) * 0.3) * exp_decay(t, 95.0, 0.001)
    sfx["ui_move"] = (sig, 0.42)

    # ui_confirm: Rising two-note cyber chime (A5 -> E6).
    n1 = concat_notes([(880.0, 0.055, "triangle"), (1318.5, 0.095, "triangle")])
    t_c = t_array(len(n1) / SR)
    sig = n1 + 0.3 * sine(2637.0, t_c) * exp_decay(t_c, 18.0)
    sfx["ui_confirm"] = (sig, 0.62)

    # ui_cancel: Falling two-note tone (E5 -> A4).
    sig = concat_notes([(659.25, 0.055, "triangle"), (440.0, 0.085, "triangle")])
    sfx["ui_cancel"] = (sig, 0.58)

    # ui_error: Low gritty double buzz.
    t = t_array(0.085)
    buzz = (saw(130.0, t) * 0.6 + square(98.0, t, 0.35) * 0.4) * exp_decay(t, 18.0)
    sig = np.concatenate([buzz, np.zeros(int(0.02 * SR)), buzz * 0.85])
    sfx["ui_error"] = (one_pole_lp(sig, 1400.0), 0.65)

    # ui_pause: HUD fold-in sweep + soft high ping.
    t = t_array(0.16)
    f_sweep = np.linspace(320.0, 1280.0, len(t))
    sig = (triangle(f_sweep, t) * 0.6 + sine(1920.0, t) * 0.4) * exp_decay(t, 16.0)
    sfx["ui_pause"] = (sig, 0.58)

    # ui_save: Shimmering 4-note data-write arpeggio (D5 - F5 - A5 - D6).
    arp = concat_notes([
        (587.33, 0.07, "triangle"),
        (698.46, 0.07, "triangle"),
        (880.00, 0.07, "triangle"),
        (1174.66, 0.22, "sine"),
    ])
    sfx["ui_save"] = (arp, 0.68)

    # ui_equip: Metallic slide + latch click.
    t1 = t_array(0.07)
    slide = one_pole_hp(white_noise(len(t1), 7), 1800.0) * exp_decay(t1, 35.0)
    t2 = t_array(0.09)
    latch = (sine(740.0, t2) + 0.6 * triangle(1480.0, t2)) * exp_decay(t2, 32.0)
    sfx["ui_equip"] = (np.concatenate([slide * 0.6, latch]), 0.65)

    # --- Exploring (8) ---
    # step: Soft muted boot tap on wet pavement; heard constantly.
    t = t_array(0.055)
    thud = sine(np.linspace(145.0, 58.0, len(t)), t) * exp_decay(t, 55.0, 0.001)
    grit = one_pole_lp(white_noise(len(t), 11), 950.0) * exp_decay(t, 75.0, 0.001)
    sfx["step"] = (thud * 0.65 + grit * 0.35, 0.34)

    # travel: District transition transit whoosh + neon chord swell.
    t = t_array(0.42)
    noise_sw = one_pole_lp(white_noise(len(t), 19), 1100.0) * np.sin(np.pi * t / 0.42)
    chord = (sine(293.66, t) + sine(440.0, t) + sine(523.25, t)) / 3.0
    env = adsr(len(t), 0.08, 0.12, 0.6, 0.22)
    sfx["travel"] = (noise_sw * 0.45 + chord * env * 0.55, 0.65)

    # blocked: Heavy security barrier thud + warning beep.
    t = t_array(0.24)
    thump = sine(np.linspace(160.0, 48.0, len(t)), t) * exp_decay(t, 16.0)
    warn = square(520.0, t, 0.25) * exp_decay(t, 14.0)
    sfx["blocked"] = (one_pole_lp(thump * 0.65 + warn * 0.35, 1600.0), 0.68)

    # talk_blip: Short warm digital terminal blip.
    t = t_array(0.045)
    f = np.linspace(620.0, 790.0, len(t))
    sig = (triangle(f, t) * 0.75 + sine(f * 2.0, t) * 0.25) * exp_decay(t, 45.0, 0.001)
    sfx["talk_blip"] = (sig, 0.46)

    # quest_new: Cyberpunk uplink fanfare (D5 -> A5 -> C6 -> D6).
    sig = concat_notes([
        (587.33, 0.09, "saw"),
        (880.00, 0.09, "saw"),
        (1046.50, 0.09, "saw"),
        (1174.66, 0.28, "triangle"),
    ])
    sfx["quest_new"] = (one_pole_lp(sig, 2800.0), 0.68)

    # quest_done: Triumphant 5-note synth arpeggio (D5-F#5-A5-D6-F#6).
    sig = concat_notes([
        (587.33, 0.085, "triangle"),
        (739.99, 0.085, "triangle"),
        (880.00, 0.085, "triangle"),
        (1174.66, 0.11, "triangle"),
        (1479.98, 0.36, "sine"),
    ])
    sfx["quest_done"] = (sig, 0.70)

    # rest: Street Vendor noodle meal restore — warm rising harmonic chord.
    t = t_array(0.75)
    env = adsr(len(t), 0.10, 0.20, 0.65, 0.45)
    chord = (
        sine(261.63, t) * 0.30
        + sine(329.63, t) * 0.25
        + sine(392.00, t) * 0.25
        + sine(493.88, t) * 0.20
        + sine(783.99, t) * 0.15 * np.sin(2.0 * np.pi * 5.0 * t)
    )
    sfx["rest"] = (chord * env, 0.68)

    # encounter: Dramatic battle swoosh + alarm sweep + sub drop.
    t = t_array(0.52)
    f_up = np.linspace(180.0, 960.0, len(t))
    f_dn = np.linspace(220.0, 42.0, len(t))
    swoosh = one_pole_lp(white_noise(len(t), 23), 1800.0) * np.sin(np.pi * t / 0.52)
    siren = saw(f_up, t) * exp_decay(t, 4.5)
    sub = sine(f_dn, t) * exp_decay(t, 5.0)
    sfx["encounter"] = (one_pole_lp(swoosh * 0.35 + siren * 0.35 + sub * 0.45, 2600.0), 0.72)

    # --- Battle: general (12) ---
    # turn_ready: Crisp ATB gauge ready ping.
    t = t_array(0.13)
    sig = (sine(1567.98, t) * 0.65 + sine(2349.32, t) * 0.35) * exp_decay(t, 24.0, 0.001)
    sfx["turn_ready"] = (sig, 0.52)

    # guard: Metallic energy-shield brace clank.
    t = t_array(0.20)
    clank = (
        sine(420.0, t) * 0.45
        + square(630.0, t, 0.3) * 0.30
        + sine(1180.0, t) * 0.25
    ) * exp_decay(t, 18.0, 0.001)
    sfx["guard"] = (clank, 0.66)

    # heal: Sparkling upward harmonic shimmer.
    t = t_array(0.46)
    f = np.linspace(523.25, 1567.98, len(t))
    shimmer = (sine(f, t) * 0.6 + triangle(f * 1.5, t) * 0.4) * (0.65 + 0.35 * np.sin(2.0 * np.pi * 16.0 * t))
    sfx["heal"] = (shimmer * exp_decay(t, 5.5, 0.015), 0.66)

    # revive: Defib capacitor whine -> shock thump -> life chime.
    t1 = t_array(0.22)
    whine = sine(np.linspace(300.0, 1400.0, len(t1)), t1) * np.linspace(0.2, 0.9, len(t1))
    t2 = t_array(0.12)
    shock = (sine(np.linspace(180.0, 50.0, len(t2)), t2) + 0.5 * white_noise(len(t2), 31)) * exp_decay(t2, 22.0)
    t3 = t_array(0.28)
    chime = (sine(880.0, t3) + 0.6 * sine(1318.5, t3)) * exp_decay(t3, 8.0)
    sfx["revive"] = (np.concatenate([whine * 0.5, shock * 0.9, chime * 0.7]), 0.72)

    # buff: Rising power-up pulse sweep.
    t = t_array(0.38)
    f = np.linspace(260.0, 780.0, len(t))
    sig = (saw(f, t) * 0.5 + triangle(f * 1.5, t) * 0.5) * adsr(len(t), 0.03, 0.1, 0.7, 0.25)
    sfx["buff"] = (one_pole_lp(sig, 2400.0), 0.65)

    # debuff: Descending phase warp.
    t = t_array(0.40)
    f = np.linspace(680.0, 140.0, len(t))
    wobble = 1.0 + 0.08 * np.sin(2.0 * np.pi * 18.0 * t)
    sig = saw(f * wobble, t) * exp_decay(t, 6.0, 0.01)
    sfx["debuff"] = (one_pole_lp(sig, 1800.0), 0.65)

    # poison: Short acidic bubble/hiss tick.
    t = t_array(0.18)
    f = 340.0 + 180.0 * np.sin(2.0 * np.pi * 28.0 * t)
    bubble = sine(f, t) * exp_decay(t, 14.0)
    hiss = one_pole_hp(white_noise(len(t), 37), 2200.0) * exp_decay(t, 18.0)
    sfx["poison"] = (bubble * 0.65 + hiss * 0.35, 0.55)

    # stun: High-voltage neural disruptor crackle + ring.
    t = t_array(0.36)
    crackle = square(np.linspace(920.0, 240.0, len(t)) * (1.0 + 0.25 * np.sin(2.0 * np.pi * 45.0 * t)), t, 0.25)
    ring = sine(1850.0, t) * exp_decay(t, 9.0)
    sfx["stun"] = (crackle * exp_decay(t, 9.0) * 0.55 + ring * 0.45, 0.66)

    # enemy_down: Digital de-rez dissolve + low power-down drop.
    t = t_array(0.48)
    f = np.linspace(540.0, 55.0, len(t))
    steps = np.floor(f / 30.0) * 30.0
    derez = square(steps, t, 0.4) * exp_decay(t, 5.5)
    noise = one_pole_lp(white_noise(len(t), 41), 1200.0) * exp_decay(t, 7.0)
    sfx["enemy_down"] = (one_pole_lp(derez * 0.65 + noise * 0.35, 2000.0), 0.68)

    # ally_down: Somber falling synth thud + low minor tone.
    t = t_array(0.55)
    f = np.linspace(293.66, 110.0, len(t))
    sig = (triangle(f, t) * 0.6 + sine(f * 0.5, t) * 0.4) * exp_decay(t, 4.8, 0.005)
    sfx["ally_down"] = (sig, 0.68)

    # flee: Quick triple dash-away footsteps/whoosh.
    chunks = []
    for i in range(3):
        tc = t_array(0.11)
        w = one_pole_lp(white_noise(len(tc), 50 + i), 1500.0 + i * 400.0) * exp_decay(tc, 22.0)
        tone = sine(np.linspace(300.0 + i * 90.0, 520.0 + i * 110.0, len(tc)), tc) * exp_decay(tc, 24.0)
        chunks.append(w * 0.5 + tone * 0.5)
    sfx["flee"] = (np.concatenate(chunks), 0.65)

    # level_up: Upbeat cyberpunk JRPG level-up jingle (D5 - F#5 - A5 - B5 - D6).
    sig = concat_notes([
        (587.33, 0.08, "saw"),
        (739.99, 0.08, "saw"),
        (880.00, 0.08, "saw"),
        (987.77, 0.10, "saw"),
        (1174.66, 0.42, "triangle"),
    ])
    sfx["level_up"] = (one_pole_lp(sig, 3200.0), 0.72)

    # --- Battle: attacks (5) ---
    # hit_blunt: Heavy pipe / riot-slam impact crunch with sub-bass punch.
    t = t_array(0.24)
    punch = sine(np.linspace(190.0, 38.0, len(t)), t) * exp_decay(t, 14.0, 0.001)
    crunch = one_pole_lp(white_noise(len(t), 61), 1100.0) * exp_decay(t, 20.0, 0.001)
    sfx["hit_blunt"] = (punch * 0.65 + crunch * 0.45, 0.74)

    # hit_blade: Razor-sharp mono-katana slash + high metallic ring.
    t = t_array(0.22)
    slash = one_pole_hp(white_noise(len(t), 67), 1600.0) * exp_decay(t, 18.0, 0.001)
    ring = (
        sine(np.linspace(2200.0, 1100.0, len(t)), t) * 0.6
        + sine(3450.0, t) * 0.4
    ) * exp_decay(t, 14.0, 0.001)
    sfx["hit_blade"] = (slash * 0.55 + ring * 0.55, 0.72)

    # gunshot: Heavy cyberpunk pistol crack + low-mid punch + short tail.
    t = t_array(0.28)
    boom = sine(np.linspace(260.0, 44.0, len(t)), t) * exp_decay(t, 16.0, 0.0005)
    crack = one_pole_lp(white_noise(len(t), 73), 3600.0) * exp_decay(t, 14.0, 0.0005)
    mech = square(480.0, t, 0.3) * exp_decay(t, 38.0, 0.0005)
    sfx["gunshot"] = (boom * 0.55 + crack * 0.55 + mech * 0.20, 0.75)

    # hack: Glitchy cyberdeck / ICE-spike digital burst.
    t = t_array(0.30)
    rng = np.random.default_rng(79)
    step_freqs = rng.choice([330.0, 440.0, 587.33, 659.25, 880.0, 1174.66, 1760.0], size=12)
    f = np.repeat(step_freqs, int(np.ceil(len(t) / len(step_freqs))))[:len(t)]
    sig = (square(f, t, 0.35) * 0.6 + saw(f * 0.5, t) * 0.4) * exp_decay(t, 7.5, 0.002)
    sfx["hack"] = (one_pole_lp(sig, 3200.0), 0.68)

    # zap: High-voltage taser / System Shock electric arc.
    t = t_array(0.32)
    fm = 520.0 + 380.0 * saw(55.0, t)
    arc = (saw(fm, t) * 0.65 + one_pole_hp(white_noise(len(t), 83), 2400.0) * 0.35)
    sfx["zap"] = (arc * exp_decay(t, 8.5, 0.001), 0.70)

    return sfx


# ---------------------------------------------------------------------------
# Starter Music Tracks (ASSET_LIST.md section 9)
# ---------------------------------------------------------------------------

def midi_to_hz(note: float) -> float:
    return 440.0 * (2.0 ** ((note - 69.0) / 12.0))


def stereo_widen(mono: np.ndarray, delay_ms: float = 14.0, mix: float = 0.35) -> tuple[np.ndarray, np.ndarray]:
    """Create a smooth stereo image from a mono synth stem."""
    d = int(SR * delay_ms / 1000.0)
    delayed = np.roll(mono, d)
    left = mono + mix * delayed
    right = mono - mix * delayed
    return left, right


def render_synth_loop(
    bpm: float,
    bars: int,
    chords_midi: list[tuple[int, ...]],
    bass_pattern: list[int],
    arp_pattern: list[int],
    drums: bool = True,
    bright: float = 1800.0,
) -> tuple[np.ndarray, np.ndarray]:
    """Render a sample-accurate looping cyberpunk synth track."""
    sec_per_beat = 60.0 / bpm
    sec_per_step = sec_per_beat / 4.0  # 16th note
    total_steps = bars * 16
    total_sec = total_steps * sec_per_step
    n_samples = int(round(total_sec * SR))
    t = np.linspace(0.0, total_sec, n_samples, endpoint=False)

    # 1. Warm detuned pad chords (1 chord per bar)
    pad = np.zeros(n_samples)
    bar_samples = n_samples // bars
    for b in range(bars):
        chord = chords_midi[b % len(chords_midi)]
        s0 = b * bar_samples
        s1 = n_samples if b == bars - 1 else (b + 1) * bar_samples
        tb = t[s0:s1] - t[s0]
        env = adsr(s1 - s0, 0.18, 0.3, 0.75, 0.25)
        c_sig = np.zeros(s1 - s0)
        for m in chord:
            hz = midi_to_hz(m)
            c_sig += 0.35 * saw(hz * 0.997, tb) + 0.35 * saw(hz * 1.003, tb) + 0.30 * triangle(hz, tb)
        pad[s0:s1] = (c_sig / max(len(chord), 1)) * env
    pad = one_pole_lp(pad, bright * 0.7)

    # 2. Driving 16th-note synth bass + arpeggio
    bass = np.zeros(n_samples)
    arp = np.zeros(n_samples)
    drum_track = np.zeros(n_samples)

    for step in range(total_steps):
        s0 = int(round(step * sec_per_step * SR))
        s1 = int(round((step + 1) * sec_per_step * SR))
        if s1 <= s0 or s1 > n_samples:
            continue
        ts = t[s0:s1] - t[s0]

        # Bass
        b_note = bass_pattern[step % len(bass_pattern)]
        if b_note > 0:
            bhz = midi_to_hz(b_note)
            b_env = exp_decay(ts, 14.0, 0.002)
            bass[s0:s1] = (saw(bhz, ts) * 0.65 + square(bhz * 0.5, ts, 0.4) * 0.35) * b_env

        # Arp
        a_note = arp_pattern[step % len(arp_pattern)]
        if a_note > 0:
            ahz = midi_to_hz(a_note)
            a_env = exp_decay(ts, 11.0, 0.002)
            arp[s0:s1] = (triangle(ahz, ts) * 0.7 + sine(ahz * 2.0, ts) * 0.3) * a_env

        # Drums (kick on 0, 8; snare on 4, 12; hat on every 2nd 16th)
        if drums:
            step_in_bar = step % 16
            if step_in_bar in (0, 6, 8):
                k_env = exp_decay(ts, 18.0, 0.001)
                drum_track[s0:s1] += sine(np.linspace(135.0, 42.0, len(ts)), ts) * k_env * 0.85
            if step_in_bar in (4, 12):
                s_env = exp_decay(ts, 22.0, 0.001)
                snare = (
                    one_pole_hp(white_noise(len(ts), 100 + step), 900.0) * 0.65
                    + sine(185.0, ts) * 0.35
                )
                drum_track[s0:s1] += snare * s_env * 0.65
            if step_in_bar % 2 == 0:
                h_env = exp_decay(ts, 48.0, 0.0005)
                hat = one_pole_hp(white_noise(len(ts), 300 + step), 4500.0) * h_env * 0.22
                drum_track[s0:s1] += hat

    bass = one_pole_lp(bass, bright * 0.55)
    arp = one_pole_lp(arp, bright)

    # Add rhythmic delay to arp
    delay_samples = int(sec_per_step * 3.0 * SR)
    arp = arp + 0.32 * np.roll(arp, delay_samples)

    pad_l, pad_r = stereo_widen(pad, 17.0, 0.4)
    arp_l, arp_r = stereo_widen(arp, 11.0, 0.45)

    left = pad_l * 0.42 + bass * 0.48 + arp_l * 0.34 + drum_track * 0.45
    right = pad_r * 0.42 + bass * 0.48 + arp_r * 0.34 + drum_track * 0.45

    # Smooth 20ms boundary crossfade so loop wrap has zero click
    fade = min(int(0.02 * SR), n_samples // 16)
    w = np.linspace(0.0, 1.0, fade)
    left[:fade] = left[:fade] * w + left[-fade:] * (1.0 - w)
    right[:fade] = right[:fade] * w + right[-fade:] * (1.0 - w)
    return left, right


def build_music() -> dict[str, tuple[np.ndarray, np.ndarray]]:
    """Return {track_name: (left, right)} for the vertical slice music cues."""
    tracks: dict[str, tuple[np.ndarray, np.ndarray]] = {}

    # 1. title.ogg: Moody D-minor cyberpunk main theme (96 BPM, 4 bars)
    tracks["title"] = render_synth_loop(
        bpm=96.0,
        bars=4,
        chords_midi=[
            (50, 57, 62, 65),  # Dm
            (46, 53, 58, 62),  # Bb
            (48, 55, 60, 64),  # C
            (45, 52, 57, 61),  # A
        ],
        bass_pattern=[
            38, 0, 38, 38, 0, 38, 50, 38, 38, 0, 38, 38, 0, 38, 45, 48,
            34, 0, 34, 34, 0, 34, 46, 34, 36, 0, 36, 36, 0, 36, 43, 45,
        ],
        arp_pattern=[
            62, 65, 69, 74, 69, 65, 62, 69,
            58, 62, 65, 70, 65, 62, 58, 65,
        ],
        drums=True,
        bright=1900.0,
    )

    # 2. central.ogg: Calm, rainy Neo-Kyoto Central night exploration (84 BPM)
    tracks["central"] = render_synth_loop(
        bpm=84.0,
        bars=4,
        chords_midi=[
            (45, 52, 57, 60, 64),  # Am7
            (41, 48, 53, 57, 60),  # Fmaj7
            (43, 50, 55, 59, 62),  # G6
            (40, 47, 52, 55, 59),  # Em7
        ],
        bass_pattern=[
            33, 0, 0, 33, 0, 0, 45, 0,
            29, 0, 0, 29, 0, 0, 41, 0,
        ],
        arp_pattern=[
            64, 0, 60, 64, 69, 0, 64, 60,
            60, 0, 57, 60, 65, 0, 60, 57,
        ],
        drums=False,
        bright=1400.0,
    )

    # 3. alley.ogg: Seedier, tenser Neon Alley pulse (92 BPM, E minor)
    tracks["alley"] = render_synth_loop(
        bpm=92.0,
        bars=4,
        chords_midi=[
            (40, 47, 52, 55),  # Em
            (39, 47, 51, 54),  # B/D#
            (36, 43, 48, 52),  # C
            (35, 42, 47, 51),  # B
        ],
        bass_pattern=[
            28, 28, 0, 28, 40, 0, 28, 0,
            27, 27, 0, 27, 39, 0, 27, 0,
        ],
        arp_pattern=[
            52, 0, 55, 59, 0, 58, 55, 0,
            48, 0, 52, 55, 0, 54, 51, 0,
        ],
        drums=True,
        bright=1350.0,
    )

    # 4. plaza.ogg: Cold, clinical Corporate Plaza minimalism (88 BPM, C minor)
    tracks["plaza"] = render_synth_loop(
        bpm=88.0,
        bars=4,
        chords_midi=[
            (48, 55, 60, 63),  # Cm
            (44, 51, 56, 60),  # Ab
            (41, 48, 53, 56),  # Fm
            (43, 50, 55, 59),  # G
        ],
        bass_pattern=[
            36, 0, 0, 0, 36, 0, 48, 0,
            32, 0, 0, 0, 32, 0, 44, 0,
        ],
        arp_pattern=[
            72, 67, 63, 60, 67, 63, 60, 55,
            68, 63, 60, 56, 63, 60, 56, 51,
        ],
        drums=False,
        bright=2100.0,
    )

    # 5. battle.ogg: Driving 132 BPM ATB combat theme (E minor)
    tracks["battle"] = render_synth_loop(
        bpm=132.0,
        bars=4,
        chords_midi=[
            (40, 47, 52, 55),  # Em
            (36, 43, 48, 52),  # C
            (38, 45, 50, 54),  # D
            (35, 42, 47, 51),  # B
        ],
        bass_pattern=[
            28, 40, 28, 40, 28, 40, 28, 40,
            24, 36, 24, 36, 26, 38, 23, 35,
        ],
        arp_pattern=[
            64, 67, 71, 76, 74, 71, 67, 64,
            60, 64, 67, 72, 71, 67, 64, 59,
        ],
        drums=True,
        bright=2500.0,
    )

    # 6. boss.ogg: Intense 144 BPM story-fight / Corporate Enforcer theme
    tracks["boss"] = render_synth_loop(
        bpm=144.0,
        bars=4,
        chords_midi=[
            (38, 45, 50, 53),  # Dm
            (37, 44, 49, 52),  # C#dim/A
            (34, 41, 46, 50),  # Bb
            (33, 40, 45, 49),  # A
        ],
        bass_pattern=[
            26, 38, 26, 38, 26, 26, 38, 26,
            25, 37, 25, 37, 22, 34, 21, 33,
        ],
        arp_pattern=[
            62, 65, 69, 74, 77, 74, 69, 65,
            61, 64, 69, 73, 76, 73, 69, 64,
        ],
        drums=True,
        bright=2800.0,
    )

    # 7. victory.ogg: Short upbeat fanfare + warm tail (plays once)
    fanfare = concat_notes([
        (523.25, 0.13, "saw"),   # C5
        (523.25, 0.13, "saw"),   # C5
        (523.25, 0.13, "saw"),   # C5
        (523.25, 0.36, "saw"),   # C5
        (415.30, 0.34, "saw"),   # Ab4
        (466.16, 0.34, "saw"),   # Bb4
        (523.25, 0.22, "saw"),   # C5
        (466.16, 0.12, "saw"),   # Bb4
        (523.25, 0.95, "triangle"),  # C5
    ])
    fanfare = one_pole_lp(fanfare, 2600.0)
    tracks["victory"] = stereo_widen(fanfare, 15.0, 0.4)

    # 8. game_over.ogg: Melancholic falling resolution (plays once)
    gameover = concat_notes([
        (440.00, 0.42, "triangle"),  # A4
        (415.30, 0.42, "triangle"),  # Ab4
        (392.00, 0.42, "triangle"),  # G4
        (369.99, 0.50, "triangle"),  # F#4
        (293.66, 1.35, "sine"),      # D4
    ])
    gameover = one_pole_lp(gameover, 1500.0)
    tracks["game_over"] = stereo_widen(gameover, 18.0, 0.45)

    # 9. ending.ogg: Bittersweet neon credits theme (90 BPM, D major / B minor)
    tracks["ending"] = render_synth_loop(
        bpm=90.0,
        bars=4,
        chords_midi=[
            (47, 54, 59, 62),  # Bm
            (43, 50, 55, 59),  # G
            (50, 57, 62, 66),  # D
            (45, 52, 57, 61),  # A
        ],
        bass_pattern=[
            35, 0, 35, 47, 0, 35, 0, 0,
            31, 0, 31, 43, 0, 31, 0, 0,
        ],
        arp_pattern=[
            66, 62, 59, 62, 66, 69, 66, 62,
            66, 62, 57, 61, 64, 69, 64, 61,
        ],
        drums=True,
        bright=1750.0,
    )

    return tracks


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assets-dir", type=Path,
                        default=Path(__file__).resolve().parent.parent / "assets",
                        help="Path to CyberRayne/assets directory")
    parser.add_argument("--skip-existing", action="store_true",
                        help="Do not overwrite sound or music files that already exist")
    parser.add_argument("--sfx-only", action="store_true",
                        help="Only generate the 32 sound effects")
    args = parser.parse_args()

    sfx_dir = args.assets_dir / "audio" / "sfx"
    music_dir = args.assets_dir / "audio" / "music"

    sfx_map = build_sfx()
    sfx_written = 0
    for name, (samples, peak) in sorted(sfx_map.items()):
        out = sfx_dir / f"{name}.wav"
        if args.skip_existing and out.exists():
            continue
        write_wav_mono(out, samples, peak=peak)
        sfx_written += 1

    music_written = 0
    if not args.sfx_only:
        music_map = build_music()
        for name, (left, right) in sorted(music_map.items()):
            out = music_dir / f"{name}.ogg"
            if args.skip_existing and out.exists():
                continue
            write_music_stereo(out, left, right)
            music_written += 1

    print(f"Generated {sfx_written}/{len(sfx_map)} SFX in {sfx_dir}")
    if not args.sfx_only:
        print(f"Generated {music_written} music tracks in {music_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
