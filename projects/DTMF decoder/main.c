/*****************************************************************************/
/*                                                                           */
/* FILENAME                                                                  */
/*   main.c                                                                  */
/*                                                                           */
/* DESCRIPTION                                                               */
/*   TMS320C5515 USB Stick Application.                                      */
/*   DTMF decoder of 16 values by detecting 8 frequencies.                   */
/*   Just stereo in.                                                         */
/*   Available: values of '0'...'9', '*', '#', 'A'...'D'.                    */
/*                                                                           */
/*   Author  : Areshchenkov Dmitriy                                          */
/*   9th Jule 2021. Created from TMS320C5515 USB Stick code.                 */
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
#include "math.h"

Int16 left_input;
Int16 right_input;

#define SAMPLES_PER_SECOND 8000 //Important for coefficients ind[]
#define GAIN_IN_dB 0

unsigned long int i = 0; // index for cycles
unsigned long int j = 0; // index for cycles
unsigned int Step = 0; 
unsigned int LastStep = 99;

#define nx 205
#define nk 8 // number of frequencies
double x[nx];
int ind[nk] = {18, 20, 22, 24, 31, 34, 38, 42}; // indexes of freq.
double res[2*nk];
#define M_2PI 6.2831853072
char num_string[16]; // output string
short count = 0; // current index of output string
short state = 1; // states: 1 - analyzing input signal and selection of output symbol
                 //         2 - waiting for no signals and then display output string

/* Алгоритм Герцеля
\param[in]  x
Указатель на вектор вещественного входного сигнала.
Размер вектора `[n x 1]`.
\param[in]  n
Размер вектора входного сигнала.
\param[in]  ind
Указатель на вектор индексов спектральных отсчетов для расчета которых
будет использоваться алгоритм Герцеля.
Размер вектора `[k x 1]`.
\param[in]  k
Размер вектора индексов спектральных отсчетов `ind`.
\param[out]  y
Указатель на вектор спектральных отсчетов, соответствующих номерам `ind`.
Размер вектора `[k x 1]` */
int goertzel(double *x, int n, int *ind, int k, double *y)
{
    int m, p;
    double wR, wI;
    double alpha;
    double v[3];
    if(!x || !y || !ind)
        return 0;
    if(n < 1 || k < 1)
        return 0;
    for(p = 0; p < k; p++)
    {
        wR = cos(M_2PI * (double)ind[p] / (double)n);
        wI = sin(M_2PI * (double)ind[p] / (double)n);
        alpha = 2.0 * wR;
        v[0] = v[1] = v[2] = 0.0;
        for(m = 0; m < n; m++)
        {
            v[2] = v[1];
            v[1] = v[0];
            v[0] = x[m]+alpha*v[1] - v[2];
        }
        y[2*p] = wR * v[0] - v[1];
        y[2*p+1] = wI * v[0];
    }
    return 1;
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
    oled_display_message_5x7("Project         ", "  DTMF decoder  ");
    USBSTK5515_waitusec(1000000);
    oled_display_message_5x7("Use buttons for ", "controlling     ");
    USBSTK5515_waitusec(2000000);
    oled_display_message_5x7("                 ", "                   ");
    for(j = 0; j < 16; j++){
        num_string[j] = ' ';
    }
    /*descriptive text */
	puts("\n Press buttons on phone\n");
      
	for ( i = 0  ; i < SAMPLES_PER_SECOND * 60000L  ;i++  )
	{
	 for(j = 0; j < nx; j++){
	     aic3204_codec_read(&left_input, &right_input); // Reading of inputs
	     x[j] = stereo_to_mono(left_input, right_input);
	 }
	 goertzel(x, nx, ind, nk, res); // x - input signal, res - frequencies info
	 for(j = 0; j < nk; j++){
	     res[j] = res[2*j] + res[2*j+1]; // amplitudes of frequencies
	 }
	 if(state == 1){
	     state = 2;
	     if(count > 15){ // oled is overflowed
              count = 0; // next position of symbol - 0
              for(j = 0; j < 16; j++) // cleaning of output string
                  num_string[j] = ' ';
          }
	     short button1 = 0;
	     double max = res[0];
	     for(j = 1; j < 4; j++){ // searching of main frequency in column
	         if(res[j]>max){
	              max = res[j];
	              button1 = j;
	          }
	     }
	     if(max < 5000){button1 = 16;}
	     short button2 = 0;
	     max = res[4];
         for(j = 5; j < 8; j++){ // searching of main frequency in row
             if(res[j]>max){
                  max = res[j];
                  button2 = j-4;
              }
         }
         if(max < 5000){button2 = 16;}
         switch(button1*4 + button2) // index of detected button
         {
             case 0:
                 num_string[count] = '1';
                 break;
             case 1:
                 num_string[count] = '2';
                 break;
             case 2:
                 num_string[count] = '3';
                 break;
             case 3:
                 num_string[count] = 'A';
                 break;
             case 4:
                 num_string[count] = '4';
                 break;
             case 5:
                 num_string[count] = '5';
                 break;
             case 6:
                 num_string[count] = '6';
                 break;
             case 7:
                 num_string[count] = 'B';
                 break;
             case 8:
                 num_string[count] = '7';
                 break;
             case 9:
                 num_string[count] = '8';
                 break;
             case 10:
                 num_string[count] = '9';
                 break;
             case 11:
                 num_string[count] = 'C';
                 break;
             case 12:
                 num_string[count] = '*';
                 break;
             case 13:
                 num_string[count] = '0';
                 break;
             case 14:
                 num_string[count] = '#';
                 break;
             case 15:
                 num_string[count] = 'D';
                 break;
             default :
                 count--;  // wrong input signal, no output symbols
                 state = 1; // reading next input
         }
	     count++;
	 }
	 else if(state == 2){
         short flag = 0;
         for(j = 0; j < nk; j++){ // waiting for ending of input signal
             if(res[j] > 1000)
                 flag = 1;
         }
         if(flag == 0){ // display output string on oled
             state = 1;
             oled_display_message_5x7(num_string, "                ");
         }
     }
	}
   /* Disable I2S and put codec into reset */ 
	aic3204_disable();
	printf( "\n***Program has Terminated***\n" );
	oled_display_message("PROGRAM HAS        ", "TERMINATED        ");
	SW_BREAKPOINT;
}
