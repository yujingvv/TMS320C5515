/* ============================================================
 * test_transpose.c  --  Unit tests for frequency transposition
 *
 * Tests (4 total):
 *   1. mix_ratio = 0  => output equals clean spectrum (pass-through)
 *   2. mix_ratio = Q15_ONE => source band zeroed in output
 *   3. mix_ratio = Q15_ONE => target band has energy (transposition works)
 *   4. Exact bin-hit interpolation (k_s integer => no interpolation)
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../config.h"
#include "../fixed_point.h"
#include "../transpose.h"

static int tests_run = 0;
static int tests_passed = 0;

#define PASS(name, ...) do { printf("  PASS: %s", (name)); printf(" -- " __VA_ARGS__); printf("\n"); tests_passed++; tests_run++; } while(0)
#define FAIL(name, ...) do { printf("  FAIL: %s", (name)); printf(" -- " __VA_ARGS__); printf("\n"); tests_run++; } while(0)

static void test_passthrough(void)
{
    static q15_cplx_t clean[N_BINS];
    static q15_cplx_t output[N_BINS];
    int k;

    for (k = 0; k < N_BINS; k++) {
        clean[k].re = (q15_t)(k & 0x7FFF);
        clean[k].im = (q15_t)((k * 17) & 0x7FFF);
    }

    transpose_apply(clean, output, 0);

    int mismatches = 0;
    for (k = 0; k < N_BINS; k++) {
        if (abs((int)output[k].re - (int)clean[k].re) > 1 ||
            abs((int)output[k].im - (int)clean[k].im) > 1)
            mismatches++;
    }

    if (mismatches == 0)
        PASS("passthrough_mix0", "output ~= clean for all %d bins (<=1 LSB)", N_BINS);
    else
        FAIL("passthrough_mix0", "%d bins differ by >1 LSB from clean", mismatches);
}

static void test_source_zeroed(void)
{
    static q15_cplx_t clean[N_BINS];
    static q15_cplx_t output[N_BINS];
    int k;

    memset(clean, 0, sizeof(clean));
    for (k = SOURCE_LO_BIN; k < SOURCE_HI_BIN; k++) {
        clean[k].re = 10000;
        clean[k].im = 5000;
    }

    transpose_apply(clean, output, Q15_ONE);

    int nonzero = 0;
    for (k = SOURCE_LO_BIN; k < SOURCE_HI_BIN; k++) {
        if (abs(output[k].re) > 1 || abs(output[k].im) > 1)
            nonzero++;
    }

    if (nonzero == 0)
        PASS("source_band_zeroed",
             "bins [%d,%d) all zero in transposed output", SOURCE_LO_BIN, SOURCE_HI_BIN);
    else
        FAIL("source_band_zeroed",
             "%d bins in source band still nonzero", nonzero);
}

static void test_target_energy(void)
{
    static q15_cplx_t clean[N_BINS];
    static q15_cplx_t output[N_BINS];
    int k;
    long long energy_target_out = 0;

    memset(clean, 0, sizeof(clean));
    for (k = SOURCE_LO_BIN; k < SOURCE_HI_BIN; k++) {
        clean[k].re = 10000;
        clean[k].im = 0;
    }

    transpose_apply(clean, output, Q15_ONE);

    for (k = TARGET_LO_BIN; k < TARGET_HI_BIN; k++) {
        energy_target_out += (long long)output[k].re * output[k].re
                           + (long long)output[k].im * output[k].im;
    }

    if (energy_target_out > 0)
        PASS("target_band_energy",
             "target band [%d,%d) has energy=%lld",
             TARGET_LO_BIN, TARGET_HI_BIN, energy_target_out);
    else
        FAIL("target_band_energy",
             "target band has zero energy after transposition");
}

static void test_exact_bin_hit(void)
{
    static q15_cplx_t clean[N_BINS];
    static q15_cplx_t output[N_BINS];
    int k;

    memset(clean, 0, sizeof(clean));
    clean[SOURCE_LO_BIN].re = 12345;
    clean[SOURCE_LO_BIN].im = -6789;

    transpose_apply(clean, output, Q15_ONE);

    int err_re = abs((int)output[TARGET_LO_BIN].re - (int)clean[SOURCE_LO_BIN].re);
    int err_im = abs((int)output[TARGET_LO_BIN].im - (int)clean[SOURCE_LO_BIN].im);
    (void)k;

    if (err_re <= 1 && err_im <= 1)
        PASS("exact_bin_hit",
             "output[%d]=(%d,%d) matches clean[%d]=(%d,%d) within 1 LSB",
             TARGET_LO_BIN,
             (int)output[TARGET_LO_BIN].re, (int)output[TARGET_LO_BIN].im,
             SOURCE_LO_BIN,
             (int)clean[SOURCE_LO_BIN].re,  (int)clean[SOURCE_LO_BIN].im);
    else
        FAIL("exact_bin_hit",
             "output[%d]=(%d,%d) expected (%d,%d), err=(%d,%d)",
             TARGET_LO_BIN,
             (int)output[TARGET_LO_BIN].re, (int)output[TARGET_LO_BIN].im,
             (int)clean[SOURCE_LO_BIN].re,  (int)clean[SOURCE_LO_BIN].im,
             err_re, err_im);
}

int main(void)
{
    printf("=== test_transpose ===\n");
    printf("  Config: SOURCE=[%d,%d) bins, TARGET=[%d,%d) bins, GAMMA=%d/%d\n",
           SOURCE_LO_BIN, SOURCE_HI_BIN,
           TARGET_LO_BIN, TARGET_HI_BIN,
           GAMMA_NUM, GAMMA_DEN);

    test_passthrough();
    test_source_zeroed();
    test_target_energy();
    test_exact_bin_hit();

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    if (tests_passed == tests_run) {
        printf("ALL PASSED\n");
        return 0;
    }
    return 1;
}
