/*****************************************************************************/
/*                                                                           */
/* FILENAME                                                                  */
/* 	 main.c                                                                  */
/*                                                                           */
/* DESCRIPTION                                                               */
/*   TMS320C5515 USB Stick Application.                                      */
/*   LEDs and OLED state controlling by push buttons.                        */
/*   NO stereo in and No stereo out, just buttons and LEDs.                  */
/*                                                                           */
/*   Author  : Areshchenkov Dmitriy                                          */
/*   2th Jule 2021. Created from TMS320C5515 USB Stick code.                 */
/*                                                                           */
/*****************************************************************************/

#include "stdio.h"
#include "usbstk5515.h"
#include "usbstk5515_led.h"
#include "oled.h"
#include "pushbuttons.h"

unsigned long int i = 0;
unsigned int Step = 0; 
unsigned int LastStep = 99;
Uint16 led_state = 0x07;
Uint16 fixed_state = 0xFF;

void main( void ) 
{
    /* Initialize BSL */
    USBSTK5515_init( );

	/* Turn off the 4 coloured LEDs */
	USBSTK5515_ULED_init();
	
	/* Initialise the OLED LCD display */
	oled_init();
	SAR_init();

	/* Flush display buffer */
    oled_display_message_5x7("                 ", "                   ");
    
    printf("\n\nRunning Project LED, OLED, Button\n");

    // Display welcome Message
    USBSTK5515_waitusec(1000000);
    oled_display_message_5x7("Project LEDs    ", "                ");
    USBSTK5515_waitusec(1000000);
    oled_display_message_5x7("Use buttons for ", "controlling     ");
    USBSTK5515_waitusec(2000000);

    /* descriptive text in console */
	puts("\n Press SW1 for DOWN, SW2 for UP, SW1 + SW2 for reset\n");

	while(1)
	{

	 Step = pushbuttons_read(9); // transmit number of Steps or States
	     // and read current Step
	  if ( Step == 1 ) // if now is Step 1
		{
		 if ( Step != LastStep ) // update display if previous step is another
		  {
		   oled_display_message_5x7("Step 1:  Clear  ", "only last LED   "); // msg showing
		   LastStep = Step; // fix current step
		   USBSTK5515_ULED_setall(0x0E); // 0000 1110 - turn on last LED
		  }
		}
	  else if ( Step == 2)
		{
		  if ( Step != LastStep)
		  {
		   oled_display_message_5x7("Step 2: Turn on ", " second LED     ");
		   LastStep = Step;
		   USBSTK5515_ULED_on(1); // turn on second LED from 0 to 3
		  }
		}
 
	  else if ( Step == 3)
		{
		  if ( Step != LastStep)
		  {
		   oled_display_message_5x7("Step 3: Turn off", "fourth(last) LED");
		   LastStep = Step;
		   USBSTK5515_ULED_off(3); // turn off last (fourth) LED
		  }
		}  
	  else if ( Step == 4)
        {
          if ( Step != LastStep)
          {
           oled_display_message_5x7("Step 4:Set third", "and toggle first");
           LastStep = Step;
           USBSTK5515_ULED_setall(0x02); // 0000 0010 - turn on 1,2 and 4 LEDs
           USBSTK5515_waitusec(1000000);
           USBSTK5515_ULED_toggle(0); // invert 1 LED -> turn on 2 and 4 LEDs
          }
        }
      else if ( Step == 5)
        {
          if ( Step != LastStep)
          {
           oled_display_message_5x7("Step 5:Blink and", "off  LED DS1    ");
           LastStep = Step;
           for(i = 0; i<10; i++){  // blink LED DS1
               USBSTK5515_LED_off(0);
               USBSTK5515_waitusec(100000);
               USBSTK5515_LED_on(0);
               USBSTK5515_waitusec(100000);
           }
           USBSTK5515_LED_off(0); // the same asm(" bclr XF"); OFF DS1 - parameter doesn't matter
          }
        }
      else if ( Step == 6)
        {
          if ( Step != LastStep)
          {
           oled_display_message_5x7("Step 6: Turn on ", "separate LED DS1");
           LastStep = Step;
           USBSTK5515_LED_on(0); // asm(" bset XF"); ON DS1 - parameter doesn't matter
          }
        }
      else if ( Step == 7)
        {
          if ( Step != LastStep)
          {
           oled_display_message_5x7("Step 7: Shift   ", " state of LEDs  ");
           LastStep = Step;
           led_state = 0x07;
           i = 0;
          }
          i++;
          if(i % 100000 == 0){ // shift of state with delay
              USBSTK5515_ULED_setall(led_state);
              USBSTK5515_ULED_getall(&fixed_state); // read current pattern of LEDs DS2-DS5
              led_state = led_state << 1; // Shift
              if(((led_state >> 4) % 2) == 1) led_state += 1; // cycle shift to first bit from fifth
          }
        }
      else if ( Step == 8)
        {
          if ( Step != LastStep)
          {
           oled_display_message_5x7("Step 8: OFF all ", " DS1 and DS2-DS5");
           LastStep = Step;
           USBSTK5515_ULED_setall(0x0F); // OFF DS2-DS5
           asm(" bclr XF"); // OFF DS1
          }
        }
      else if ( Step == 9)
        {
          if ( Step != LastStep)
          {
           oled_display_message_5x7("Step 9:Set saved", " state on step 7");
           LastStep = Step;
           USBSTK5515_ULED_setall(fixed_state); // Set state saved before on step 7
          }
        }
	}
}
