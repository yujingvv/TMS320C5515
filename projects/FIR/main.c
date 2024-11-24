/*****************************************************************************/
/*                                                                           */
/* FILENAME                                                                  */
/*   main.c                                                                  */
/*                                                                           */
/* DESCRIPTION                                                               */
/*   TMS320C5515 USB Stick Application.                                      */
/*   Audio Loopback from Microphone In --> to Headphones/Lineout.            */
/*   Use buttons, stereo in and stereo out.                                  */
/*   Mode of processing is controlled by push buttons.                       */
/*   Available: Low Frequency FIR, High Frequency FIR, Bandpass and Bandstop */
/*                                                                           */
/*   Author  : Areshchenkov Dmitriy                                          */
/*   6th Jule 2021. Created from TMS320C5515 USB Stick code.                 */
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

// h1 - Low Frequency FIR // Fpass - 330 Hz, Fstop - 740 Hz
DATA h1[100] = {
        4,      1,      1,      0,     -1,     -3,     -7,    -12,    -19,
      -28,    -39,    -52,    -68,    -86,   -106,   -129,   -152,   -177,
     -202,   -226,   -249,   -269,   -285,   -296,   -300,   -296,   -283,
     -259,   -224,   -178,   -118,    -46,     39,    136,    244,    363,
      490,    624,    762,    902,   1041,   1177,   1306,   1427,   1536,
     1631,   1709,   1770,   1811,   1832,   1832,   1811,   1770,   1709,
     1631,   1536,   1427,   1306,   1177,   1041,    902,    762,    624,
      490,    363,    244,    136,     39,    -46,   -118,   -178,   -224,
     -259,   -283,   -296,   -300,   -296,   -285,   -269,   -249,   -226,
     -202,   -177,   -152,   -129,   -106,    -86,    -68,    -52,    -39,
      -28,    -19,    -12,     -7,     -3,     -1,      0,      1,      1,
        4
};
// h2 - High Frequency FIR // Fstop - 350 Hz, Fpass - 740 Hz
DATA h2[115] = {
      387,   -249,   -176,   -119,    -75,    -40,    -10,     16,     39,
       59,     76,     89,     97,    101,     97,     88,     71,     47,
       17,    -17,    -55,    -93,   -130,   -162,   -188,   -203,   -207,
     -196,   -171,   -131,    -77,    -11,     65,    146,    229,    308,
      377,    430,    463,    469,    446,    388,    296,    167,      4,
     -192,   -415,   -659,   -918,  -1183,  -1447,  -1698,  -1929,  -2131,
    -2297,  -2420,  -2495,  30247,  -2495,  -2420,  -2297,  -2131,  -1929,
    -1698,  -1447,  -1183,   -918,   -659,   -415,   -192,      4,    167,
      296,    388,    446,    469,    463,    430,    377,    308,    229,
      146,     65,    -11,    -77,   -131,   -171,   -196,   -207,   -203,
     -188,   -162,   -130,    -93,    -55,    -17,     17,     47,     71,
       88,     97,    101,     97,     89,     76,     59,     39,     16,
      -10,    -40,    -75,   -119,   -176,   -249,    387
};
// h3 - Bandpass FIR // Fstop1 - 330 Hz, Fpass1 - 740 Hz,
                     // Fpass2 - 1600 Hz, Fstop2 - 2000 Hz
DATA h3[102] = {
       -7,    -16,    -28,    -36,    -32,     -9,     38,    102,    170,
      218,    222,    167,     54,    -95,   -242,   -342,   -365,   -303,
     -182,    -50,     40,     48,    -24,   -136,   -220,   -208,    -62,
      198,    498,    734,    811,    690,    409,     75,   -172,   -224,
      -59,    236,    481,    475,     81,   -701,  -1698,  -2602,  -3062,
    -2808,  -1761,    -87,   1831,   3502,   4473,   4473,   3502,   1831,
      -87,  -1761,  -2808,  -3062,  -2602,  -1698,   -701,     81,    475,
      481,    236,    -59,   -224,   -172,     75,    409,    690,    811,
      734,    498,    198,    -62,   -208,   -220,   -136,    -24,     48,
       40,    -50,   -182,   -303,   -365,   -342,   -242,    -95,     54,
      167,    222,    218,    170,    102,     38,     -9,    -32,    -36,
      -28,    -16,     -7
};

// h4 - Bandstop FIR // Fpass1 - 330 Hz, Fstop1 - 740 Hz
                     // Fstop2 - 1600 Hz, Fpass2 - 2000 Hz
DATA h4[93] = {
      446,   -269,   -113,     29,    137,    193,    186,    122,     29,
      -51,    -79,    -39,     48,    128,    144,     62,   -104,   -293,
     -421,   -429,   -314,   -140,    -11,    -15,   -176,   -427,   -634,
     -660,   -436,    -14,    447,    750,    758,    480,     93,   -128,
       65,    753,   1772,   2735,   3165,   2694,   1240,   -922,  -3217,
    -4963,  27153,  -4963,  -3217,   -922,   1240,   2694,   3165,   2735,
     1772,    753,     65,   -128,     93,    480,    758,    750,    447,
      -14,   -436,   -660,   -634,   -427,   -176,    -15,    -11,   -140,
     -314,   -429,   -421,   -293,   -104,     62,    144,    128,     48,
      -39,    -79,    -51,     29,    122,    186,    193,    137,     29,
     -113,   -269,    446
};

Int16 left_input;
Int16 right_input;
Int16 left_output;
Int16 right_output;

#define SAMPLES_PER_SECOND 16000 //Important for coefficients h[]
#define GAIN_IN_dB 0

unsigned long int i = 0; // index for cycles
unsigned long int j = 0; // index for cycles
Uint16 LastStep = 0xFFFF;
Uint16 CurStep = 0x0101;

#define maxnh 1000
ushort nh = 100; // number of coefficients for current FIR
DATA dbuffer1[maxnh+2];
DATA dbuffer2[maxnh+2];
DATA *h = h1; // coefficients pointer for FIR

#define NX 128 // size of array with values of input signal for FFT
DATA whiteNoise[128] = {-2511, 1710, -1768, 6250, 1722, -437, 1960, -18, 3501, -587, -3547, 779, 4936, -3785, -767, 3177, -6072, 1977, 2381, 675, -2436, -23, 294, 2957, 2098, -1108, 5122, -161, -2570, -1387, 2715, 617, 2810, 218, 7864, -5268, -1316, -267, 282, 4604, 693, -364, 216, 2376, -2181, 1425, -4950, -250, -2979, -2065, 893, -3558, -3128, -4794, -7048, -480, 3676, -576, -740, -3311, 5006, -789, 4092, 1144, -832, 3422, 3106, -353, 4098, 5632, -208, -809, 1279, 1112, 2553, -809, 2261, 3315, 561, 484, -3967, -3709, -4622, -3935, -491, -5107, 7102, 2337, -6539, -3652, -339, -5417, -1455, 3450, -519, -213, -3168, -1758, -4274, 1151, -1431, 2477, 3912, 3406, -111, -1594, -3871, 374, 3034, -1079, -549, 1948, 2731, -4481, 632, 619, -478, 2823, -1152, -2613, 872, 1454, -688, 2517, -4801, -3400, -9716, 1648};
DATA noiseResult[128];
DATA xf[2*NX]; // values of input for fft
int fr[NX/2]; // result of fft
DATA dbuffer3[maxnh+2];

void AmpFreqGraph(/*DATA *h, ushort nh*/){

    for(j = 0; j < 2*nh+2; j++){
        dbuffer3[j] = 0; // clean buffer
    }
    if(nh == 0){
        for(j = 0; j < 128; j++){
            noiseResult[j] = whiteNoise[j];
        }
    } else{
        fir(whiteNoise, h, noiseResult, dbuffer3, 128, nh);
    }
    for(j = 0; j < NX; j++){
        xf[j*2] = noiseResult[j]; // add new value of input -- Real Part
        xf[j*2+1] = 0; // Imaginary Part of number
    }
    cfft(xf, NX, SCALE); // Complex Fast Fourier transform
    cbrev(xf, xf, NX); // Complex bit reverse
    for(j = 0; j < NX/2; j++){
        fr[j] = (xf[2*j]*xf[2*j] + xf[2*j+1]*xf[2*j+1])/20; // fr[j] - amplitude of complex xf[2*j]
    }
    oled_display_bargraph(fr); // Display Amplitude Frequency Spectrum
}

void main( void ) 
{
    /* Initialize BSL */
    USBSTK5515_init( );
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
	/* Setup sampling frequency and gain */
    set_sampling_frequency_and_gain(SAMPLES_PER_SECOND, GAIN_IN_dB);
    /* Flush display buffer */
    oled_display_message_5x7("                 ", "                   ");
    // Display welcome Message
    USBSTK5515_waitusec(1000000);
    oled_display_message_5x7("Project   FIRs  ", "                ");
    USBSTK5515_waitusec(1000000);
    oled_display_message_5x7("Use buttons for ", "controlling     ");
    USBSTK5515_waitusec(2000000);

    /*descriptive text */
	puts("\n Press SW1 for DOWN, SW2 for UP, SW1 + SW2 for reset\n");
	puts(" CurStep 1   = Straight through, no signal processing");
	puts(" CurStep 2   = Low Frequency FIR");
	puts(" CurStep 3   = High Frequency FIR");
	puts(" CurStep 4   = Bandpass FIR");
	puts(" CurStep 5   = Bandstop FIR");
      
    for(i = 0; i < 2*nh+2; i++){
        dbuffer1[i] = 0; // clean buffers
        dbuffer2[i] = 0;
    }
	for ( i = 0  ; i < SAMPLES_PER_SECOND * 60000L  ;i++  )
	{
	 aic3204_codec_read(&left_input, &right_input); // Reading of inputs

	 CurStep = pushbuttons_read(5,3); // transmit number of Steps or States
     // and read current CurStep
	  if ( CurStep >> 8 == 1 )
		{
	     if ((CurStep & 0x00FF) == 1){
             if ( CurStep != LastStep )
              {
               oled_display_message_5x7("CurStep 1:         ", " No Processing  ");
               LastStep = CurStep;
               nh = 0; // no coefficients for no processing
              }
	     } else if ((CurStep & 0x00FF) == 2){
             if ( CurStep != LastStep )
              {
               LastStep = CurStep;
               nh = 0; // no coefficients for no processing
               AmpFreqGraph();
              }
         } else if ((CurStep & 0x00FF) == 3){
             if ( CurStep != LastStep )
              {
               oled_display_message_5x7("Pass any        ", "      frequency ");
               LastStep = CurStep;
               nh = 0; // no coefficients for no processing
              }
         }
         right_output = right_input; // transmit signal without processing
         left_output = left_input;
		}
	  else if ( CurStep >> 8 == 2)
		{
	      if ((CurStep & 0x00FF) == 1){
              if ( CurStep != LastStep)
              {
                 oled_display_message_5x7("CurStep 2: Low     ", " Frequency FIR  ");
                 LastStep = CurStep;
                 nh = 100; // Set coefficients for Low Frequency FIR
                 h = h1;
              }
	      } else if ((CurStep & 0x00FF) == 2){
	          if ( CurStep != LastStep)
              {
                 LastStep = CurStep;
                 nh = 100; // Set coefficients for Low Frequency FIR
                 h = h1;
                 AmpFreqGraph();
              }
	      } else if ((CurStep & 0x00FF) == 3){
	             if ( CurStep != LastStep )
	              {
	               oled_display_message_5x7(" Fpass = 330 Hz ", " Fstop = 740 Hz ");
	               LastStep = CurStep;
	               nh = 100; // Set coefficients for Low Frequency FIR
	               h = h1;
	              }
	         }
		}
	  else if ( CurStep >> 8 == 3)
		{
          if ((CurStep & 0x00FF) == 1){
              if ( CurStep != LastStep)
              {
                  oled_display_message_5x7("CurStep 3: High    ", " Frequency FIR  ");
                  LastStep = CurStep;
                  nh = 115; // Set coefficients for High Frequency FIR
                  h = h2;
              }
          } else if ((CurStep & 0x00FF) == 2){
              if ( CurStep != LastStep)
              {
                  LastStep = CurStep;
                  nh = 115; // Set coefficients for High Frequency FIR
                  h = h2;
                  AmpFreqGraph();
              }
          } else if ((CurStep & 0x00FF) == 3){
              if ( CurStep != LastStep )
               {
                oled_display_message_5x7(" Fstop = 350 Hz ", " Fpass = 740 Hz ");
                LastStep = CurStep;
                nh = 115; // Set coefficients for High Frequency FIR
                h = h2;
               }
          }
		}  
	  else if ( CurStep >> 8 == 4)
          {
            if ((CurStep & 0x00FF) == 1){
                if ( CurStep != LastStep)
                {
                    oled_display_message_5x7("CurStep 4:         ", " Bandpass FIR   ");
                    LastStep = CurStep;
                    nh = 102; // Set coefficients for Bandpass FIR
                    h = h3;
                }
            } else if ((CurStep & 0x00FF) == 2){
                if ( CurStep != LastStep)
                {
                    LastStep = CurStep;
                    nh = 102; // Set coefficients for Bandpass FIR
                    h = h3;
                    AmpFreqGraph();
                }
            } else if ((CurStep & 0x00FF) == 3){
                if ( CurStep != LastStep )
                 {
                  oled_display_message_5x7("st1=330Hz p1=740", "p2=1600 st2=2000");
                  LastStep = CurStep;
                  nh = 102; // Set coefficients for Bandpass FIR
                  h = h3;
                 }
            }
          }
      else if ( CurStep >> 8 == 5)
          {
            if ((CurStep & 0x00FF) == 1){
                if ( CurStep != LastStep)
                {
                    oled_display_message_5x7("CurStep 5:         ", " Bandstop FIR   ");
                    LastStep = CurStep;
                    nh = 93; // Set coefficients for Bandstop FIR
                    h = h4;
                }
            } else if ((CurStep & 0x00FF) == 2){
                if ( CurStep != LastStep)
                {
                    LastStep = CurStep;
                    nh = 93; // Set coefficients for Bandstop FIR
                    h = h4;
                    AmpFreqGraph();
                }
            } else if ((CurStep & 0x00FF) == 3){
                if ( CurStep != LastStep )
                 {
                  oled_display_message_5x7("p1=330Hz st1=740", "st2=1600 p2=2000");
                  LastStep = CurStep;
                  nh = 93; // Set coefficients for Bandstop FIR
                  h = h4;
                 }
            }
          }
	  if(nh != 0){
         fir(&left_input, h, &left_output, dbuffer1, 1, nh);
         fir(&right_input, h, &right_output, dbuffer2, 1, nh);
	  }
	  aic3204_codec_write(left_output, right_output); // Writing in outputs
	  bargraph_6dB(left_output, right_output); // display bargraph on LEDs
	}
   /* Disable I2S and put codec into reset */ 
	aic3204_disable();
	printf( "\n***Program has Terminated***\n" );
	oled_display_message("PROGRAM HAS        ", "TERMINATED        ");
	SW_BREAKPOINT;
}
