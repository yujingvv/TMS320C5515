/* ============================================================
 * gen_test_signal.c  --  Generate multi-tone test WAV
 *
 * Generates a 2-second 16-bit mono WAV at 16 kHz containing
 * four sinusoidal tones:
 *   578 Hz, 3000 Hz (target band), 6000 Hz, 7000 Hz (source band)
 *
 * Usage: gen_test_signal [out.wav]
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../config.h"
#include "../fixed_point.h"
#include "../wav_io.h"

#define NOISE_DURATION_MS   250
#define SIGNAL_DURATION_S   2
#define NOISE_SAMPLES       ((NOISE_DURATION_MS * FS) / 1000)
#define SIGNAL_SAMPLES      (SIGNAL_DURATION_S * FS)
#define TOTAL_SAMPLES       (NOISE_SAMPLES + SIGNAL_SAMPLES)

static const double FREQS[4] = { 578.0, 3000.0, 6000.0, 7000.0 };
static const double AMPS[4]  = { 2000.0, 2500.0, 7000.0, 5000.0 };

int main(int argc, char *argv[])
{
    const char *out_path = (argc >= 2) ? argv[1] : "synth.wav";
    wav_writer_t writer;
    int n, t;
    q15_t peak = 0;
    unsigned int lfsr = 0xBEEFu;

    if (wav_write_open(&writer, out_path, FS) != 0) {
        fprintf(stderr, "gen_test_signal: cannot create %s\n", out_path);
        return 1;
    }

    for (n = 0; n < TOTAL_SAMPLES; n++) {
        double x;

        if (n < NOISE_SAMPLES) {
            lfsr ^= lfsr >> 7;
            lfsr ^= lfsr << 9;
            lfsr ^= lfsr >> 13;
            x = (double)((int)(lfsr & 0xFF) - 128) * 0.5;
        } else {
            int ns = n - NOISE_SAMPLES;
            x = 0.0;
            for (t = 0; t < 4; t++)
                x += AMPS[t] * sin(2.0 * M_PI * FREQS[t] * ns / FS);
        }

        if (x >  32767.0) x =  32767.0;
        if (x < -32768.0) x = -32768.0;

        q15_t s = (q15_t)(int16_t)(x >= 0 ? x + 0.5 : x - 0.5);
        q15_t abss = (q15_t)(s < 0 ? -s : s);
        if (abss > peak) peak = abss;

        wav_write_samples(&writer, &s, 1);
    }

    wav_write_close(&writer);

    printf("Generated '%s':\n", out_path);
    printf("  noise preamble: %d ms (%d samples)\n", NOISE_DURATION_MS, NOISE_SAMPLES);
    printf("  signal:         %d s  (%d samples)\n", SIGNAL_DURATION_S, SIGNAL_SAMPLES);
    printf("  tones: %.0f Hz (A=%.0f), %.0f Hz (A=%.0f), "
           "%.0f Hz (A=%.0f), %.0f Hz (A=%.0f)\n",
           FREQS[0], AMPS[0], FREQS[1], AMPS[1],
           FREQS[2], AMPS[2], FREQS[3], AMPS[3]);
    printf("  peak=%d\n", (int)peak);
    return 0;
}
