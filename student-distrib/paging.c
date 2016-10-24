/* paging.c - Contains two functions for initializing the paging and setting up the control registers 
 * 
 */
#include "lib.h"
#include "x86_desc.h"
#include "paging.h"
//Settiing all the processes to NOT in use now 
char proc[MAX_NUM_PROC] = {0,0,0,0,0,0,0,0};


/*
* void remove_page();
*   Inputs: int proc_id
*   Return Value: none
*	Function: Clears proc[] list upon halt()
*/
void remove_page(int proc_id) {
	proc[proc_id] = 0;
}

/*
* void init_paging();
*   Inputs: none
*   Return Value: none
*	Function: Prepares the Page Descriptor Entry and Page Table Entry by setting the appropriate control bits 
*/
void init_paging() {
	uint32_t i,j;

	// Video memory
	PDE[0][0] = (unsigned int)&PTE;			// Set address of PTE
	PDE[0][0] &= 0xFFFFF000; 				// Take 20MSB and put it in PDE
	PDE[0][0] |= CTRL_VIDEO;				// Then apply control bits at bits 11-0 

	// Kernel memory
	PDE[0][1] = 1 << 22;					// Set 10 MSB of Kernel at bits 31-22
	PDE[0][1] |= CTRL_KERNEL;				// Set appropriate control bits at 12-0

	// fill the remaining PDE directly to physical memory
	for(i = 2; i < NUM_ENTRIES; i++) {
		PDE[0][i] = i << 22;
		PDE[0][i] |= CTRL_EMPTY;
	}

	// initialize rest of page table entries
	for(i = 0; i < NUM_ENTRIES; i++) {
		PTE[i] = i << 12;
		PTE[i] |= CTRL_PTE;
	}

	// for each PROCESS create new pde
	for (j = 1; j < MAX_NUM_PROC; j++) {
		for (i = 0; i<NUM_ENTRIES; i++) {
			PDE[j][i] = CTRL_EMPTY_USR;
		}
	}

	// Clear present bit (LSB) on M[0] to prevent dereferencing null pointer
	PTE[0] &= 0xFFFFFFE; // 1111 1111 .... 1111 1110

	// Fill CR3 with PDE
	asm volatile ("			\
	movl $PDE, %eax;		\n\
	movl %eax, %cr3;"
	);

	//Set bit 31 and 1 of the CR0 (PG and PE flag)
	asm volatile ("			\n\
	movl %cr4, %eax;		\n\
	orl $0x00000090, %eax;	\n\
	movl %eax, %cr4;");

	//Set the bits 4 and 7 of the CR4 (Page Global Enable and Page Size Extensions)
	asm volatile ("			\n\
	movl %cr0, %eax;		\n\
	orl $0x80000000, %eax;	\n\
	movl %eax, %cr0;");
}

/*
* int new_pde(uint32_t v_entry) 
*   Inputs:v_entry 
*   Return Value: none
*	Function: creates a new PDE for the given v_entry and returns the proc_id for the newly allocated pde 
*/
int new_pde(uint32_t v_entry) {
	uint32_t temp;
	

	// find available proc slot
	int i;
	for (i=0; i<MAX_NUM_PROC; i++) {
		if(proc[i] == 0) break;
	}
	if(i == 7) return -1;

	// mark as being used
	proc[i] = 1;
	
		// Video memory
	PDE[i+1][0] = (unsigned int)&PTE;			// Set address of PTE
	PDE[i+1][0] &= 0xFFFFF000; 				// Take 20MSB and put it in PDE
	PDE[i+1][0] |= CTRL_VIDEO;				// Then apply control bits at bits 11-0 

	// Kernel memory
	PDE[i+1][1] = 1 << 22;					// Set 10 MSB of Kernel at bits 31-22
	PDE[i+1][1] |= CTRL_KERNEL;				// Set appropriate control bits at 12-0
	
	//Set bit 31 and 1 of the CR0 (PG and PE flag)
	asm volatile ("			\n\
	movl %cr4, %eax;		\n\
	orl $0x00000090, %eax;	\n\
	movl %eax, %cr4;");

	//Set the bits 4 and 7 of the CR4 (Page Global Enable and Page Size Extensions)
	asm volatile ("			\n\
	movl %cr0, %eax;		\n\
	orl $0x80000000, %eax;	\n\
	movl %eax, %cr0;");
	
	// find physical address and obtain 10 MSB
	temp = (0x400000*i + 0x800000);
	temp &= 0xFFC00000;

	// fill in pde
	v_entry >>= 22;
	//PDE[i+1][v_entry] = 0x83;
	PDE[i+1][v_entry] |= (temp | 0x87);
	//PDE[i+1][v_entry] = 0x800083;
	//PDE[i+1][v_entry] |= temp;
	// return slot
	return i;
}



/*
* int change_cr3(int process_id) 
*   Inputs:process id 
*   Return Value: 0
*	Function: Given the process_id the CR3 register is changed with the correct PDE entry for that process 
*/
int change_cr3(int process_id) {

	if (process_id < 0 || process_id > 7) return -1;

	// check if pde entry actually exists too perhaps later

	uint32_t* temp = &PDE[process_id+1][0];
	asm volatile (
		"movl %0, %%eax \n"
		"movl %%eax, %%cr3 \n"
		:
		: "r" (temp)
	);
	
	return 0;
}


/*
* void change_pte(int i, int value) 
*   Inputs: i and value  
*   Return Value: 0
*	Function: Given the value and i, the PTE is set accordingly 
*/
void change_pte(int i, int value) {
	PTE[i] = value << 12;
	PTE[i] |= CTRL_PTE;
}
