//main.c
#include "init.h"
#include "uart.h"
#include <math.h>

// ******************************************************************************************* //
// Configuration bits for CONFIG1 settings. 
//
// Make sure "Configuration Bits set in code." option is checked in MPLAB.
// This option can be set by selecting "Configuration Bits..." under the Configure
// menu in MPLAB.

_FOSC(OSCIOFNC_ON & FCKSM_CSDCMD & POSCMD_NONE);	//Oscillator Configuration (clock switching: disabled;
							// failsafe clock monitor: disabled; OSC2 pin function: digital IO;
							// primary oscillator mode: disabled)
_FOSCSEL(FNOSC_FRCPLL);					//Oscillator Selection PLL
_FWDT(FWDTEN_OFF);					//Turn off WatchDog Timer
_FGS(GCP_OFF);						//Turn off code protect
_FPOR(FPWRT_PWR1);					//Turn off power up timer

//holds the char sent
volatile char uart_rcvd_char;
//flag indicating char has been rcvd
volatile char uart_rcvd;

//IC1 interrupt flag
volatile char falling_edge_rcvd;

//rising & falling flag
volatile char rising_edge;
volatile unsigned int rising_time, falling_time;

//flag to start new frame
volatile char new_frame_start = 0;

//value to hold number of timer counts of echo
//Timer2 count, with presclaer @ 1:8, is 2E-7s -or-
//5 counts per 1us
volatile unsigned int timer_counts;

//echo period in us
//= timer_counts / 5.0 to convert to us
volatile float period_us;

//distance variable. units are cm
volatile float distance;


int main()
{

    //initialize everything
    uart_rcvd_char = 0;
    uart_rcvd = 0;
    falling_edge_rcvd = 0;
    new_frame_start = 0;
    rising_edge = 1;

    //initializations
    InitHC_SR04();
    InitClock();
    InitUART1();
    InitTimer1();
    InitTimer2();

    //wait for signal from PC
    while(uart_rcvd == 0);

    while(1) {

        //refresh everything
        new_frame_start = 0;

        //clear timer counter and start it up again
        TMR1 = 0;
        T1CONbits.TON = 1;

        //Set Trigger pin high for ~10us (a bit more here)
        LATBbits.LATB7 = 1;
        while (TMR1 < 10);
        LATBbits.LATB7 = 0;

        //wait for the echo pin to go low
        //while (falling_edge_rcvd == 0);

        //wait for timer1 to interrupt indicating new frame
        //do all the calcuations after this
        while(new_frame_start == 0);

        //means echo has gone low and interrupt occurred
        if(falling_edge_rcvd == 1) {

            //logic for rollover of timer2
            if (falling_time > rising_time) {
                timer_counts = falling_time - rising_time;
            }
            else {
                //PR2 resets to 0xFFFF
                timer_counts = (PR2 - rising_time) + falling_time;
            }
            //period_us = (timer_counts / 5.0);
            //distance = period_us / 58.0;
            //combining those two calculations
            distance = timer_counts / 290.0;
            
        }
        else {

            //timeout code just so something gets sent
            distance = -1.0;
        }

        //reset the flag for the next frame
        falling_edge_rcvd = 0;

        //mask timer 1 interrupt while sending uart
	//IEC0bits.T1IE = 0;
        sendFloat(distance);
        //sendShort(timer_counts);
        //sendFloat(3.1415);
	//IEC0bits.T1IE = 1;

   
    }

    return 1;
}

void __attribute__((__interrupt__, no_auto_psv)) _U1RXInterrupt(void)
{

	// Clear interrupt flag
	IFS0bits.U1RXIF = 0;
        //let the main loop know we received a char
        uart_rcvd = 1;
        //load the char
        uart_rcvd_char = U1RXREG;
}

void __attribute__((__interrupt__, no_auto_psv)) _IC1Interrupt(void)
{

    unsigned int t1;
    t1 = IC1BUF;
    //Clear IF
    IFS0bits.IC1IF = 0;
    if (rising_edge == 1) {
        rising_time = t1;
        rising_edge = 0;
    }
    else {
        falling_edge_rcvd = 1;
        falling_time = t1;
        rising_edge = 1;
    }

}

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{

	// Clear Timer 1 interrupt flag to allow another Timer 1 interrupt to occur.
	IFS0bits.T1IF = 0;
        T1CONbits.TON = 0;
        new_frame_start = 1;
}