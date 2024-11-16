/*****************************************************************************/
/*                                                                           */
/* FILENAME                                                                  */
/*   main.c                                                                  */
/*                                                                           */
/* DESCRIPTION                                                               */
/*   TMS320C5515 USB Stick Application.                                      */
/*   Reading of stereo input and collecting of it's values.                  */
/*   Display graph of Amplitude Frequency Spectrum on OLED.                  */
/*   No stereo out, just stereo in.                                          */
/*                                                                           */
/*   Author  : Areshchenkov Dmitriy                                          */
/*   4th Jule 2021. Created from TMS320C5515 USB Stick code.                 */
/*                                                                           */
/*****************************************************************************/

#include "stdio.h"
#include "usbstk5515.h"
#include "usbstk5515_led.h"
#include "aic3204.h"
#include "PLL.h"
#include "bargraph.h"
#include "oled.h"
#include "pushbuttons.h"
#include "stereo.h"
#include "dsplib.h"

Int16 left_input; // inputs of Stereo IN
Int16 right_input;
Int16 mono_input;

#define SAMPLES_PER_SECOND 48000    //DEFAULT: 48000
// VALID Values: 48000, 24000, 16000, 12000, 9600, 8000, 6857
// check declaration of set_sampling_frequency_and_gain(SamplingFrequency, ADCgain)

/* Use 20 for guitar, 30 for Microphone and 0 for Line Input */
#define GAIN_IN_dB 0
#define NX 128 // size of array with values of input signal

unsigned long int i = 0; // index for cycles
unsigned long int j = 0; // index for cycles
unsigned long int count = 0; // index of array xf
DATA xf[2*NX]; // values of input for fft
int fr[NX/2]; // result of fft

void main( void ) 
{
    /* Initialize BSL */
    USBSTK5515_init( );
    printf("\n\nRunning Project Fourier Analysis\n");

    /* Initialize PLL */
    pll_frequency_setup(120);

    /* Initialise hardware interface and I2C for code */
    aic3204_hardware_init();
    
    /* Initialise the AIC3204 codec */
    aic3204_init();

    /* Turn off the 4 coloured LEDs */
    USBSTK5515_ULED_init();
    
    /* Initialise the OLED LCD display */
    oled_init();
    SAR_init();

    /* Setup sampling frequency and gain*/
    set_sampling_frequency_and_gain(SAMPLES_PER_SECOND, GAIN_IN_dB);

    /* Flush display buffer */
    oled_display_message_5x7("                 ", "                   ");
    // Display welcome Message
    USBSTK5515_waitusec(1000000);
    oled_display_message_5x7("Project         ", "Fourier Analysis");
    USBSTK5515_waitusec(2000000);

    for ( i = 0  ; i < SAMPLES_PER_SECOND * 600L  ;i++  )
    {

     aic3204_codec_read(&left_input, &right_input); // Reading of stereo inputs

     mono_input = stereo_to_mono(left_input, right_input); // mono = left + right / 2

      xf[count*2] = mono_input; // add new value of input -- Real Part
      xf[count*2+1] = 0; // Imaginary Part of number
      count++;

      if(count == NX){ // If array xf is ready to fft
       cfft(xf, NX, SCALE); // Complex Fast Fourier transform
       cbrev(xf, xf, NX); // Complex bit reverse
        for(j = 0; j < NX/2; j++){
            fr[j] = xf[2*j]*xf[2*j] + xf[2*j+1]*xf[2*j+1]; // fr[j] - amplitude of complex xf[2*j]
        }
        oled_display_bargraph(fr); // Display Amplitude Frequency Spectrum
        count = 0; // restart of measurements
      }
    }
   /* Disable I2S and put codec into reset */ 
    aic3204_disable();
    printf( "\n***Program has Terminated***\n" );
    oled_display_message("PROGRAM HAS        ", "TERMINATED        ");
    SW_BREAKPOINT;
}
