/* ============================================================
 * main_pc.c  --  PC simulator entry point
 *
 * Reads a 16-bit mono WAV at FS=16000 Hz, passes it through
 * the full DSP pipeline hop-by-hop, and writes the output WAV.
 * Reports peak amplitudes and estimated cycle load.
 *
 * Usage: tms320_sim in.wav out.wav
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "fixed_point.h"
#include "pipeline.h"
#include "wav_io.h"
#include "cycle_counter.h"

int main(int argc, char *argv[])
{
    wav_reader_t reader;
    wav_writer_t writer;
    q15_t  in_hop[HOP];
    q15_t  out_hop[HOP];
    int    got, total_hops = 0, i;
    q15_t  peak_in = 0, peak_out = 0;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s in.wav out.wav\n", argv[0]);
        return 1;
    }

    if (wav_read_open(&reader, argv[1]) != 0)  return 1;
    if (wav_write_open(&writer, argv[2], FS) != 0) {
        wav_read_close(&reader);
        return 1;
    }

    pipeline_init();
    cycles_reset();

    while ((got = wav_read_samples(&reader, in_hop, HOP)) > 0) {
        if (got < HOP)
            memset(in_hop + got, 0, (HOP - got) * sizeof(q15_t));

        for (i = 0; i < got; i++) {
            q15_t s = (q15_t)(in_hop[i] < 0 ? -in_hop[i] : in_hop[i]);
            if (s > peak_in) peak_in = s;
        }

        pipeline_process(in_hop, out_hop);

        for (i = 0; i < HOP; i++) {
            q15_t s = (q15_t)(out_hop[i] < 0 ? -out_hop[i] : out_hop[i]);
            if (s > peak_out) peak_out = s;
        }

        wav_write_samples(&writer, out_hop, HOP);
        total_hops++;
    }

    {
        unsigned long long total_cyc = (unsigned long long)cycles_total();
        unsigned long long cph = (total_hops > 0) ? total_cyc / total_hops : 0;

        printf("peak_in=%d  peak_out=%d\n", (int)peak_in, (int)peak_out);
        printf("total_hops=%d  total_cycles=%llu\n", total_hops, total_cyc);
        printf("cycles_per_hop=%llu\n", cph);
        printf("cpu_load=%.1f%%  (100 MHz, hop=%.1f ms)\n",
               (double)cph / 1600000.0 * 100.0,
               1000.0 * HOP / FS);
    }

    wav_read_close(&reader);
    wav_write_close(&writer);
    return 0;
}
