#include "rtc.h"
#include "lib.h"
#include "sched.h"

volatile int32_t interrupt_count = 0; 


/*
* void rtc_init() 
*   Inputs: none
*   Return Value: none
*	Function:// Initializes rtc clock
*/
void rtc_init() {
	// nmi protection on
	outb(inb(RTC_PORT)&NMI, RTC_PORT); 
	// Retrieve register A
	outb(REG_A, RTC_PORT);
	char prev = inb(CMOS_PORT);
	// set the frequency
	outb(REG_A, RTC_PORT);
	outb(((prev & FREQ_MASK)| 10), CMOS_PORT);
	// Retrieve register B
	outb(REG_B, RTC_PORT);
	prev = inb(CMOS_PORT);
	// set PIE (bit 6)
	outb(REG_B, RTC_PORT);
	outb(((prev & FREQ_MASK) | PIE), CMOS_PORT);
	// nmi protection off
	outb(inb(RTC_PORT|NMI_OFF), RTC_PORT);
}



/*
* void periodic_interrupt_handler() 
*   Inputs: none
*   Return Value: none
* Function for handling periodic interrupt (pin 8 on pic)
*/
void periodic_interrupt_handler() 
{
	cli();	
	interrupt_count = 1; 
	outb(C_MASK, RTC_PORT); //select register c
	inb(CMOS_PORT); // throw away content
	
	do_events();

	sti();
	send_eoi(8);
	asm volatile("leave; iret;");
}


/*
* int32_t rtc_read () 
*   Inputs: none
*   Return Value: none
* 	This call should always return 0, but only after an interrupt has occurred, a flag is set and it waits 
	 until the interrupt handler clears it, then return 0). 
*/
int32_t rtc_read () {
																																										
	while(!interrupt_count);
	interrupt_count = 0;
	return 0; 
}



/*
* void rtc_write(int32_t freq) 
*   Inputs: none
*   Return Value: none
* 	//Sets RTC clock to desired frequency
*/
void rtc_write(int32_t freq) {
	int32_t rate; 
	//The rate is set according to the right frequency 
	//cli();
	switch (freq) {
		case 2:	  rate = RATE1; break; 
		case 4:	  rate = RATE2; break; 	
		case 8:	  rate = RATE3; break; 
		case 16:  rate = RATE4; break; 
		case 32:  rate = RATE5; break; 
		case 64:  rate = RATE6; break; 
		case 128: rate = RATE7; break; 
		case 256: rate = RATE8; break; 
		case 512: rate = RATE9;	break; 
		case 1024:rate = RATE10;break; 
		default: return; 
	}

	// nmi protection on
	outb(inb(RTC_PORT)&NMI, RTC_PORT); 
	// Retrieve register A
	outb(REG_A, RTC_PORT);
	char prev = inb(CMOS_PORT);
	// set the frequency accordingly 
	outb(REG_A, RTC_PORT);
	outb(((prev & FREQ_MASK) | rate), CMOS_PORT);
	// Retrieve register B
	outb(REG_B, RTC_PORT);
	prev = inb(CMOS_PORT);
	// set PIE (bit 6)
	outb(REG_B, RTC_PORT);
	outb(((prev & FREQ_MASK) | PIE ), CMOS_PORT);
	// nmi protection off
	outb(inb(RTC_PORT|NMI_OFF), RTC_PORT);
	//sti();
}

/*
* int32_t rtc_open (const uint8_t* filename)
*   Inputs: filename
*   Return Value: none
* 	//return 0 
*/
int32_t rtc_open (const uint8_t* filename) {
	return 0; 
}


/*
* int32_t rtc_close (int32_t fd) {
*   Inputs: file descriptor
*   Return Value: none
* 	//return 0 
*/
int32_t rtc_close (int32_t fd) {
	return 0; 
}

// FOR TESTING RTC
void rtc_test() 
{
	int temp =  0; 
	int i = 2;
	int counter = 0;
	while (i != 2048)
	{	
		rtc_write(i);
		temp =  0; 
		//clear();
		while(temp < 10) 
		{
			//printf("(%d %d %d)\n", i, temp, counter);
			rtc_read(); 
			
			counter++;
			temp++;
			
			if (counter % 10 == 0)
			{
				printf("RTC %d\n", i);
			}
		}
		i = i*2;
	}
}


