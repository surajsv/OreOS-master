#include "handlers.h"
#include "lib.h"


// Function for unused exceptions
// shouldn't be called
// input: none
// output: none
void unused_exception() {
	printf("unused_exception\n");
	while(1);
}

// Function for handling divide exception
// input: none
// output: none
void divide_error() {
	printf("divide_error\n");
	while(1);
}

// Function for handling debug exception
// input: none
// output: none
void debug() {
	printf("debug\n");
	while(1);
}

// Function for handling non-maskable-interrupt exception
// input: none
// output: none
void nmi() {
	printf("nmi\n");
	while(1);
}

// Function for handling int3 exception
// input: none
// output: none
void int3() {
	printf("int3\n");
	while(1);
}

// Function for handling overflow exception
// input: none
// output: none
void overflow() {
	printf("overflow\n");
	while(1);
}

// Function for handling bounds exception
// input: none
// output: none
void bounds() {
	printf("bounds\n");
	while(1);
}

// Function for handling invalid opcode exception
// input: none
// output: none
void invalid_op() {
	printf("invalid_operation\n");
	while(1);
}

// Function for handling device not available exception
// input: none
// output: none
void device_not_available() {
	printf("device_not_available\n");
	while(1);
}

// Function for handling double fault exception
// input: none
// output: none
void double_fault() {
	printf("double_fault\n");
	while(1);
}

// Function for handling the French-Ass-Sounding
// coprocesseur segment overrun exception
// input: none
// output: none
void coprocesseur_segment_overrun() {
	printf("coprocesseur_segment_overrun\n");
	while(1);
}

// Function for handling inavlid tss exception
// input: none
// output: none
void invalid_tss() {
	printf("invalid_tss\n");
	while(1);
}

// Function for handling segment not present exception
// input: none
// output: none
void segment_no_present() {
	printf("segment_no_presnt\n");
	while(1);
}

// Function for handling stack segment exception
// input: none
// output: none
void stack_segment() {
	printf("stack_segment\n");
	while(1);
}

// Function for handling general protection exception
// input: none
// output: none
void general_protection() {
	printf("general_protection\n");
	while(1);
}

// Function for handling page fault exception
// input: none
// output: none
void page_fault() {
	printf("pretty boi\n");
	while(1);
}

// Function for handling none exception
// input: none
// output: none
void none() {
	printf("none\n");
	while(1);
}

// Function for handling coprocessor error exception
// input: none
// output: none
void coprocessor_error() {
	printf("coprocessor_error()\n");
	while(1);
}

// Function for handling alignment check exception
// input: none
// output: none
void alignement_check() {
	printf("alignment_check\n");
	while(1);
}

// Function for handling machine check exception
// input: none
// output: none
void machine_check() {
	printf("machine_check\n");
	while(1);
}


// Function for handling the rtc clock interrupt (pin 0 on pic)
// input: none
// output: none
void rtc_handler() {
	cli();
	printf("rtc_handler\n");
	sti();
	send_eoi(0);
	asm volatile("leave;\
					iret;") ;
}
