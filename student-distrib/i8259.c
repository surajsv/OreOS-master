/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/*
* void i8259_init();
*   Inputs: ignored
*   Return Value: none
*	Function: Initialize the 8259 PIC
*/
void
i8259_init(void) {
	// outb intialization control words
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW1, SLAVE_8259_PORT);

	outb(ICW2_MASTER, MASTER_8259_PORT+1);
	outb(ICW2_SLAVE, SLAVE_8259_PORT+1);

	outb(ICW3_MASTER, MASTER_8259_PORT+1);
	outb(ICW3_SLAVE, SLAVE_8259_PORT+1);

	outb(ICW4, MASTER_8259_PORT+1);
	outb(ICW4, SLAVE_8259_PORT+1);


	// mask (disable) all irq
	master_mask = 0xFF;
	slave_mask = 0xFF; 

	outb(master_mask, MASTER_8259_PORT+1);
	outb(slave_mask, SLAVE_8259_PORT+1);
}

/*
* void enable_irq();
*   Inputs: irq_num - #irq to enable
*   Return Value: none
*	Function: Enable (unmask) the specified IRQ
*/
void
enable_irq(uint32_t irq_num) {
	// irq in master
	if (irq_num <= 7)  {
		master_mask &= ~(1<<(irq_num%8));
		outb(master_mask, MASTER_8259_PORT+1);

	// irq in slave
	} else {
		slave_mask &= ~(1<<(irq_num%8));
		outb(slave_mask, SLAVE_8259_PORT+1);
	}
}

/*
* void disable_irq();
*   Inputs: irq_num - #irq to disable
*   Return Value: none
*	Function: Disable (mask) the specified IRQ
*/
void
disable_irq(uint32_t irq_num) {
	// irq in master
	if (irq_num <= 7) {
		master_mask |= (1<<(irq_num%8));
		outb(master_mask, MASTER_8259_PORT+1);

	// irq in slave
	} else {
		slave_mask &= (1<<(irq_num%8));
		outb(slave_mask, SLAVE_8259_PORT+1);
	}

	// if all slave is disabled, disable slave on master
	//if (slave_mask == 0xFF)
	//	disable_irq(2);
}

/*
* void send_eoi();
*   Inputs: irq_num - #irq to disable
*   Return Value: none
*	Function: Send end-of-interrupt signal for the specified IRQ
*/
void
send_eoi(uint32_t irq_num)
{
	int temp=0;

	//If the IRQ is on the master PIC 
	if (irq_num < 8) {
		temp = EOI | irq_num;
		outb(temp,MASTER_8259_PORT);

	//If the IRQ is on the slave PIC
	} else {
		irq_num = irq_num - 8;
		temp = EOI | irq_num;
		outb(temp,SLAVE_8259_PORT);
		send_eoi(2);
	}
}

