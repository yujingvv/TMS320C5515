/*****************************************************************************/
/*                                                                           */
/* FILENAME                                                                  */
/*   main.c                                                                  */
/*                                                                           */
/* DESCRIPTION                                                               */
/*   TMS320C5515 USB Stick Application.                                      */
/*   Reading of stereo input and collecting of it's values                   */
/*   Display graph of input values on LEDs and OLED.                         */
/*   Step (Mode) switching by buttons.                                       */
/*   No stereo out, just stereo in.                                          */
/*                                                                           */
/*   Author  : Areshchenkov Dmitriy                                          */
/*   2th Jule 2021. Created from TMS320C5515 USB Stick code.                 */
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

unsigned long int i = 0; // index for cycles
unsigned long int j = 0; // index for cycles
unsigned int Step = 0;     // current step or state
unsigned int LastStep = 99;// previous step
int x[200]; // values of input divided by 2
int xa[100]; // average values of two measurements

void main( void ) 
{
    /* Initialize BSL */
    USBSTK5515_init( );
    printf("\n\nRunning Project Audio Input with graphs\n");

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
    oled_display_message_5x7("Project         ", " Audio Input    ");
    USBSTK5515_waitusec(1000000);
    oled_display_message_5x7("Use buttons to  ", "switch graph    ");
    USBSTK5515_waitusec(2000000);

    /* descriptive text in console */
    puts("\n Press SW1 for DOWN, SW2 for UP, SW1 + SW2 for reset\n");

	for ( i = 0  ; i < SAMPLES_PER_SECOND * 600L  ;i++  )
	{

	 aic3204_codec_read(&left_input, &right_input); // Reading of stereo inputs
	 mono_input = stereo_to_mono(left_input, right_input); // mono = left + right / 2
	 Step = pushbuttons_read(5);// transmit number of Steps or States
     // and read current Step

      for(j = 0  ; j < 199  ; j++ ){ // shift old values of input
          x[j] = x[j+1];
      }
      x[199] = mono_input/2; // add new value of input
      for(j = 0  ; j < 100  ; j++ ){
         xa[j] = x[j*2] + x[j*2 + 1]; // xa - average of two values x - more accuracy against noise
      }

	  if (Step == 1 )
		{
		 if (Step != LastStep )
		  {
		   oled_display_message_5x7("Step 1:         ", "Bargraph on LEDs");
		   LastStep = Step;
		  }
		 bargraph_6dB(left_input, right_input); // display bargraph on LEDs
		}
	  else if (Step == 2)
		{
		  if (Step != LastStep)
		  {
		   oled_display_message_5x7("Step 2:         ", "Bargraph on OLED");
		   LastStep = Step;
		   USBSTK5515_ULED_setall(0x0F);
		  }
		  if(i % (SAMPLES_PER_SECOND/2) == 1){
		    oled_display_bargraph(xa); // display bargraph on OLED
		  }
		}
      else if (Step == 3) // Step to stop bargraph from step 2
          {
            if (Step != LastStep)
            {
             LastStep = Step;
             USBSTK5515_ULED_setall(0x00);
            }
          }
	  else if (Step == 4)
		{
          if (Step != LastStep)
          {
           oled_display_message_5x7("Step 4:         ", "Waveform on OLED");
           LastStep = Step;
           USBSTK5515_ULED_setall(0x0F);
          }
          if(i % (SAMPLES_PER_SECOND/2) == 1){
            oled_display_waveform(xa); // display graph of waveform on OLED
          }
		}  
	  else if (Step == 5) // Step to stop waveform from step 4
          {
            if (Step != LastStep)
            {
             LastStep = Step;
             USBSTK5515_ULED_setall(0x00);
            }
          }
	}
   /* Disable I2S and put codec into reset */ 
	aic3204_disable();
	printf( "\n***Program has Terminated***\n" );
	oled_display_message("PROGRAM HAS        ", "TERMINATED        ");
	SW_BREAKPOINT;
}
