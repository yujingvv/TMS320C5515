#!/usr/bin/env python3
"""
compare.py  --  Spectral comparison of input vs. output WAV files

Usage: python3 compare.py in.wav out.wav [output.png]

Metrics reported:
  - Source band attenuation (dB):  energy change in 4-8 kHz
  - Target band boost (dB):        energy change in 2-4 kHz
  - Cosine similarity:             dot(X_in, X_out) / (|X_in| |X_out|)
"""

import sys
import struct
import math

try:
    import numpy as np
    HAS_NUMPY = True
except ImportError:
    HAS_NUMPY = False
    print("[compare.py] numpy not found")

try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[compare.py] matplotlib not found; skipping plot")


def read_wav_mono16(path):
    with open(path, 'rb') as f:
        if f.read(4) != b'RIFF': raise ValueError(f"Not RIFF: {path}")
        f.read(4)
        if f.read(4) != b'WAVE': raise ValueError(f"Not WAVE: {path}")
        audio_format = num_channels = sample_rate = bits = 0
        data_bytes = None
        while True:
            chunk_id = f.read(4)
            if len(chunk_id) < 4: break
            chunk_size = struct.unpack('<I', f.read(4))[0]
            if chunk_id == b'fmt ':
                fmt = f.read(chunk_size)
                audio_format = struct.unpack_from('<H', fmt, 0)[0]
                num_channels = struct.unpack_from('<H', fmt, 2)[0]
                sample_rate  = struct.unpack_from('<I', fmt, 4)[0]
                bits         = struct.unpack_from('<H', fmt, 14)[0]
            elif chunk_id == b'data':
                data_bytes = f.read(chunk_size)
                break
            else:
                f.read(chunk_size)
        if data_bytes is None: raise ValueError("No data chunk")
        if audio_format != 1: raise ValueError(f"Not PCM")
        if num_channels != 1: raise ValueError(f"Not mono")
        if bits != 16: raise ValueError(f"Not 16-bit")
        n = len(data_bytes) // 2
        return list(struct.unpack(f'<{n}h', data_bytes[:n*2])), sample_rate


FS        = 16000
N_FFT     = 1024
SOURCE_LO = 4000;  SOURCE_HI = 8000
TARGET_LO = 2000;  TARGET_HI = 4000

def freq_to_bin(hz): return int(round(hz * N_FFT / FS))

SOURCE_LO_BIN = freq_to_bin(SOURCE_LO)
SOURCE_HI_BIN = freq_to_bin(SOURCE_HI)
TARGET_LO_BIN = freq_to_bin(TARGET_LO)
TARGET_HI_BIN = freq_to_bin(TARGET_HI)

def db(out, inp):
    if inp <= 0: return float('nan')
    r = out / inp
    return float('-inf') if r <= 0 else 10.0 * math.log10(r)

def compute_metrics(samples_in, samples_out):
    hop  = N_FFT // 4
    n    = min(len(samples_in), len(samples_out))
    x_in  = np.array(samples_in[:n],  dtype=np.float64)
    x_out = np.array(samples_out[:n], dtype=np.float64)
    win   = np.hanning(N_FFT)
    src_e_in = src_e_out = tgt_e_in = tgt_e_out = 0.0
    psd_in  = np.zeros(N_FFT // 2 + 1)
    psd_out = np.zeros(N_FFT // 2 + 1)
    frames  = 0
    pos = 0
    while pos + N_FFT <= n:
        Si = np.abs(np.fft.rfft(x_in [pos:pos+N_FFT] * win)) ** 2
        So = np.abs(np.fft.rfft(x_out[pos:pos+N_FFT] * win)) ** 2
        psd_in  += Si;  psd_out += So
        src_e_in  += np.sum(Si[SOURCE_LO_BIN:SOURCE_HI_BIN])
        src_e_out += np.sum(So[SOURCE_LO_BIN:SOURCE_HI_BIN])
        tgt_e_in  += np.sum(Si[TARGET_LO_BIN:TARGET_HI_BIN])
        tgt_e_out += np.sum(So[TARGET_LO_BIN:TARGET_HI_BIN])
        pos += hop;  frames += 1
    dot = float(np.dot(x_in[:n], x_out[:n]))
    ni  = float(np.linalg.norm(x_in[:n]))
    no  = float(np.linalg.norm(x_out[:n]))
    return {
        'source_db': db(src_e_out, src_e_in),
        'target_db': db(tgt_e_out, tgt_e_in),
        'cosine_sim': dot / (ni * no) if ni * no > 0 else 0.0,
        'psd_in':  psd_in  / max(frames, 1),
        'psd_out': psd_out / max(frames, 1),
        'frames': frames,
    }

def plot_spectra(psd_in, psd_out, out_png):
    freqs = np.linspace(0, FS / 2, len(psd_in))
    fig, axes = plt.subplots(2, 1, figsize=(10, 7))
    fig.suptitle('Spectral comparison: input vs. output', fontsize=13)
    eps = 1e-12
    ax = axes[0]
    ax.plot(freqs/1000, 10*np.log10(psd_in +eps), label='Input',  color='steelblue', lw=1.5)
    ax.plot(freqs/1000, 10*np.log10(psd_out+eps), label='Output', color='tomato',    lw=1.5)
    ax.axvspan(SOURCE_LO/1000, SOURCE_HI/1000, alpha=0.12, color='red',   label='Source band')
    ax.axvspan(TARGET_LO/1000, TARGET_HI/1000, alpha=0.12, color='green', label='Target band')
    ax.set_xlabel('Frequency (kHz)');  ax.set_ylabel('PSD (dB)')
    ax.legend(fontsize=9);  ax.grid(True, alpha=0.4);  ax.set_xlim(0, FS/2000)
    ax2 = axes[1]
    ax2.plot(freqs/1000, 10*np.log10((psd_out+eps)/(psd_in+eps)), color='purple', lw=1.5)
    ax2.axhline(0, color='k', lw=0.8, linestyle='--')
    ax2.axvspan(SOURCE_LO/1000, SOURCE_HI/1000, alpha=0.12, color='red',   label='Source band')
    ax2.axvspan(TARGET_LO/1000, TARGET_HI/1000, alpha=0.12, color='green', label='Target band')
    ax2.set_xlabel('Frequency (kHz)');  ax2.set_ylabel('Output - Input (dB)')
    ax2.set_title('Difference spectrum')
    ax2.legend(fontsize=9);  ax2.grid(True, alpha=0.4);  ax2.set_xlim(0, FS/2000)
    plt.tight_layout();  plt.savefig(out_png, dpi=150)
    print(f"Plot saved: {out_png}")

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} in.wav out.wav [compare.png]");  sys.exit(1)
    in_path  = sys.argv[1]
    out_path = sys.argv[2]
    png_path = sys.argv[3] if len(sys.argv) >= 4 else 'compare.png'
    samples_in,  _ = read_wav_mono16(in_path)
    samples_out, _ = read_wav_mono16(out_path)
    print(f"  input : {len(samples_in)} samples")
    print(f"  output: {len(samples_out)} samples")
    if not HAS_NUMPY:
        print("numpy required"); sys.exit(1)
    m = compute_metrics(samples_in, samples_out)
    print(f"\n=== Spectral metrics ===")
    print(f"  Source band attenuation ({SOURCE_LO}-{SOURCE_HI} Hz): {m['source_db']:+.2f} dB")
    print(f"  Target band boost       ({TARGET_LO}-{TARGET_HI} Hz): {m['target_db']:+.2f} dB")
    print(f"  Cosine similarity:  {m['cosine_sim']:.4f}")
    print(f"  Frames analysed:    {m['frames']}")
    if HAS_MATPLOTLIB:
        plot_spectra(m['psd_in'], m['psd_out'], png_path)
    src_ok = m['source_db'] < 0
    tgt_ok = m['target_db'] > 0
    cos_ok = 0.3 < m['cosine_sim'] < 1.0
    if src_ok and tgt_ok and cos_ok:
        print("\nRESULT: PASS")
    else:
        print("\nRESULT: CHECK")
        if not src_ok: print(f"  WARNING: source not attenuated ({m['source_db']:+.2f} dB)")
        if not tgt_ok: print(f"  WARNING: target not boosted ({m['target_db']:+.2f} dB)")

if __name__ == '__main__':
    main()
