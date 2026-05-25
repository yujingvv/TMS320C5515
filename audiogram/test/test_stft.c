/* ============================================================
 * test_stft.c  --  Unit tests for STFT framing + OLA synthesis
 *
 * Tests:
 *   1. Pass-through unity gain: feed noise through
 *      push_hop -> fwd FFT -> inv FFT -> overlap_add
 *      and verify output equals input (delayed by N_FFT-HOP samples).
 *      Expected: scale ~1.0000, rel_rms < 0.005
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../config.h"
#include "../fixed_point.h"
#include "../stft.h"
#include "../fft_hwa.h"

#define TOTAL_HOPS  64
#define WARMUP_HOPS  4

static stft_state_t g_stft;

#define OLA_DELAY  (N_FFT - HOP)

int main(void)
{
    static q15_t      input_buf[TOTAL_HOPS * HOP];
    static q15_t      output_buf[TOTAL_HOPS * HOP];
    static q15_t      frame[N_FFT];
    static q15_cplx_t spec[N_FFT];
    static q15_t      itime[N_FFT];

    int hop, i;
    unsigned int lfsr = 0x1234ABCDu;

    printf("=== test_stft ===\n");

    fft_hwa_init();

    for (i = 0; i < TOTAL_HOPS * HOP; i++) {
        lfsr ^= lfsr >> 7;
        lfsr ^= lfsr << 9;
        lfsr ^= lfsr >> 13;
        input_buf[i] = (q15_t)(int16_t)((int)(lfsr & 0xFFFF) - 32768);
        input_buf[i] = (q15_t)(input_buf[i] >> 3);
    }

    stft_init(&g_stft);

    for (hop = 0; hop < TOTAL_HOPS; hop++) {
        const q15_t *in_hop  = &input_buf[hop * HOP];
        q15_t       *out_hop = &output_buf[hop * HOP];

        stft_push_hop(&g_stft, in_hop, frame);
        fft_hwa_forward(frame, spec);
        fft_hwa_inverse(spec, itime);
        stft_overlap_add(&g_stft, itime, out_hop);
    }

    double sum_sq_err = 0.0;
    double sum_sq_in  = 0.0;
    double sum_out = 0.0, sum_in = 0.0;
    int count = 0;

    for (hop = WARMUP_HOPS; hop < TOTAL_HOPS; hop++) {
        for (i = 0; i < HOP; i++) {
            int out_idx = hop * HOP + i;
            int in_idx  = out_idx - OLA_DELAY;
            if (in_idx < 0) continue;

            double out_s = (double)output_buf[out_idx];
            double in_s  = (double)input_buf[in_idx];
            double err   = out_s - in_s;

            sum_sq_err += err * err;
            sum_sq_in  += in_s * in_s;
            sum_out    += out_s * out_s;
            sum_in     += in_s * in_s;
            count++;
        }
    }

    double scale   = (sum_in > 0) ? sqrt(sum_out / sum_in) : 0.0;
    double rel_rms = (sum_sq_in > 0) ? sqrt(sum_sq_err / sum_sq_in) : 0.0;

    printf("  scale=%.4f  rel_rms=%.4f  (samples=%d)\n", scale, rel_rms, count);

    int tests_passed = 0;
    int tests_run = 2;

    if (fabs(scale - 1.0) < 0.01) {
        printf("  PASS: unity_gain  (scale=%.4f)\n", scale);
        tests_passed++;
    } else {
        printf("  FAIL: unity_gain  (scale=%.4f, expected ~1.0000)\n", scale);
    }

    if (rel_rms < 0.005) {
        printf("  PASS: low_rms  (rel_rms=%.4f)\n", rel_rms);
        tests_passed++;
    } else {
        printf("  FAIL: low_rms  (rel_rms=%.4f, expected < 0.005)\n", rel_rms);
    }

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    if (tests_passed == tests_run) {
        printf("ALL PASSED\n");
        return 0;
    }
    return 1;
}
