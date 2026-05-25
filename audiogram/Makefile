# Makefile for TMS320C5515 audiogram PC simulator + tests
# Run from the audiogram/ directory

CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall -Wextra -I. -D_INT64_T_DECLARED -D_GNU_SOURCE
LDFLAGS = -lm
SRCDIR  = .
TESTDIR = test

ALGO_SRC = pipeline.c stft.c wiener.c vad_decision.c transpose.c
HAL_SRC  = fft_hwa.c cycle_counter.c
UTIL_SRC = window_table.c synthesis_window_table.c twiddle_table.c wav_io.c

ALL_SRC  = $(ALGO_SRC) $(HAL_SRC) $(UTIL_SRC)

.PHONY: all tests clean run_tests

all: tms320_sim test_fft test_stft test_transpose gen_test_signal

tms320_sim: main_pc.c $(ALL_SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test_fft: $(TESTDIR)/test_fft.c fft_hwa.c cycle_counter.c twiddle_table.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test_stft: $(TESTDIR)/test_stft.c stft.c fft_hwa.c cycle_counter.c \
           window_table.c synthesis_window_table.c twiddle_table.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test_transpose: $(TESTDIR)/test_transpose.c transpose.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

gen_test_signal: $(TESTDIR)/gen_test_signal.c wav_io.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

run_tests: all
	@echo "=== Running test_fft ==="
	./test_fft
	@echo ""
	@echo "=== Running test_stft ==="
	./test_stft
	@echo ""
	@echo "=== Running test_transpose ==="
	./test_transpose
	@echo ""
	@echo "=== Generating test signal ==="
	./gen_test_signal synth.wav
	@echo ""
	@echo "=== Running end-to-end simulator ==="
	./tms320_sim synth.wav out.wav
	@echo ""
	@echo "=== Spectral comparison ==="
	python3 $(TESTDIR)/compare.py synth.wav out.wav compare.png

clean:
	rm -f tms320_sim test_fft test_stft test_transpose gen_test_signal
	rm -f synth.wav out.wav compare.png
