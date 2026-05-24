/* stub_bsl.c -- BSL/CSL hardware stubs for build verification.
 * Replace with real CSL driver calls when SDK is available. */

#include "fixed_point.h"

void audio_codec_init(unsigned int sample_rate) { (void)sample_rate; }
void dma_audio_start(short *in_buf, short *out_buf, int half_size) {
    (void)in_buf; (void)out_buf; (void)half_size;
}
void intc_enable(int irq_id) { (void)irq_id; }
void cpu_idle_until_irq(void) { }
