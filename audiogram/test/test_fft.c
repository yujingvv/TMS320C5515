/* ============================================================
 * test_fft.c  --  Unit tests for fft_hwa (radix-2 DIT, 1024-pt)
 *
 * Tests:
 *   1. Peak-bin detection: sine at bin 37 (578 Hz) -> peak at bin 37
 *   2. Conjugate symmetry: X[k] = conj(X[N-k]) for real input
 *   3. Round-trip accuracy: rel_rms = rms(ifft(fft(x))-x) / rms(x)
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../config.h"
#include "../fixed_point.h"
#include "../fft_hwa.h"

#define TEST_PASS(name)  printf("  PASS: %s\n", (name))
#define TEST_FAIL(name, ...)  do { printf("  FAIL: %s -- ", (name)); printf(__VA_ARGS__); printf("\n"); } while(0)

static int tests_run = 0;
static int tests_passed = 0;

static q31_t magnitude(q15_cplx_t c)
{
    return (q31_t)sqrt((double)c.re*c.re + (double)c.im*c.im);
}

static int test_peak_bin(void)
{
    static q15_t      time_in[N_FFT];
    static q15_cplx_t freq[N_FFT];
    int i;
    const int PEAK_BIN = 37;
    const double AMP = 18000.0;

    for (i = 0; i < N_FFT; i++) {
        double x = AMP * sin(2.0 * M_PI * PEAK_BIN * i / N_FFT);
        time_in[i] = (q15_t)(int16_t)(x >= 0 ? x + 0.5 : x - 0.5);
    }

    fft_hwa_forward(time_in, freq);

    int peak_k = 0;
    q31_t peak_mag = 0;
    for (i = 0; i < N_FFT / 2; i++) {
        q31_t m = magnitude(freq[i]);
        if (m > peak_mag) { peak_mag = m; peak_k = i; }
    }

    tests_run++;
    if (peak_k == PEAK_BIN) {
        TEST_PASS("peak_bin");
        printf("    peak_bin=%d (%.1f Hz)  mag=%d\n",
               peak_k, (double)peak_k * FS / N_FFT, (int)peak_mag);
        tests_passed++;
        return 1;
    } else {
        TEST_FAIL("peak_bin", "expected bin %d, got %d", PEAK_BIN, peak_k);
        return 0;
    }
}

static int test_conj_symmetry(void)
{
    static q15_t      time_in[N_FFT];
    static q15_cplx_t freq[N_FFT];
    int i;
    int max_err = 0;

    for (i = 0; i < N_FFT; i++) {
        double x = 12000.0 * sin(2.0 * M_PI * 100 * i / N_FFT);
        time_in[i] = (q15_t)(int16_t)(x >= 0 ? x + 0.5 : x - 0.5);
    }

    fft_hwa_forward(time_in, freq);

    for (i = 1; i < N_FFT / 2; i++) {
        int err_re = abs((int)freq[i].re - (int)freq[N_FFT - i].re);
        int err_im = abs((int)freq[i].im + (int)freq[N_FFT - i].im);
        if (err_re > max_err) max_err = err_re;
        if (err_im > max_err) max_err = err_im;
    }

    tests_run++;
    if (max_err <= 2) {
        TEST_PASS("conj_symmetry");
        printf("    worst_error=%d LSB\n", max_err);
        tests_passed++;
        return 1;
    } else {
        TEST_FAIL("conj_symmetry", "worst error = %d LSB (expected <= 2)", max_err);
        return 0;
    }
}

static int test_round_trip(void)
{
    static q15_t      x_in[N_FFT];
    static q15_cplx_t freq[N_FFT];
    static q15_t      x_out[N_FFT];
    int i;

    unsigned int lfsr = 0xACE1u;
    for (i = 0; i < N_FFT; i++) {
        lfsr ^= lfsr >> 7;
        lfsr ^= lfsr << 9;
        lfsr ^= lfsr >> 13;
        x_in[i] = (q15_t)(int16_t)((int)(lfsr & 0xFFFF) - 32768);
        x_in[i] = (q15_t)(x_in[i] >> 2);
    }

    fft_hwa_forward(x_in, freq);
    fft_hwa_inverse(freq, x_out);

    double sum_err = 0.0, sum_in = 0.0;
    for (i = 0; i < N_FFT; i++) {
        double e = (double)x_out[i] - (double)x_in[i];
        sum_err += e * e;
        sum_in  += (double)x_in[i] * x_in[i];
    }
    double rel_rms = sqrt(sum_err / sum_in);

    tests_run++;
    if (rel_rms < 0.005) {
        TEST_PASS("round_trip");
        printf("    rel_rms=%.5f\n", rel_rms);
        tests_passed++;
        return 1;
    } else {
        TEST_FAIL("round_trip", "rel_rms=%.5f (expected < 0.005)", rel_rms);
        return 0;
    }
}

int main(void)
{
    printf("=== test_fft ===\n");
    fft_hwa_init();

    test_peak_bin();
    test_conj_symmetry();
    test_round_trip();

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    if (tests_passed == tests_run) {
        printf("ALL PASSED\n");
        return 0;
    } else {
        printf("SOME FAILED\n");
        return 1;
    }
}
