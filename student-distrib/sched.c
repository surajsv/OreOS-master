#include "sched.h"
#include "lib.h"
#include "sys_call.h"
#include "handlers.h"


/*
* 	void do_events() 
*   Inputs:none
*   Return Value: none
* 	This function is called for 10ms - 50ms i.e everytime a periodic interrupt happens. The background processes are run for the given CPU cycles 
*/
void do_events() {
	if(fake_sti) return;

	int i, count=0;
	for(i=0; i<3; i++)
		if(term_init[i]) count++;

	// only 1 active terminal exit
	if(count == 1)
		return;

	// save state (esp and ebp) of current process
	asm volatile (
		"movl %%esp, %0 \n"
		"movl %%ebp, %1 \n"
		: "=r" (proc_esp[bg_term]), "=r" (proc_ebp[bg_term])
	);	

	// save cursor position
	getXY((int*) &cursor_pos[bg_term][0]);

	// get the next event(process) in queue
	do {
		bg_term++;
		if (bg_term == 3) bg_term = 0;
	} while(term_init[bg_term] != 1);



	// if the the event is active term write to video
	if(active_term == bg_term) {
		change_coord(cursor_pos[active_term][0], cursor_pos[active_term][1]);
		change_pte(0xb8, 0xb8);

	// if not, store it
	} else {
		change_coord_silent(cursor_pos[bg_term][0], cursor_pos[bg_term][1]);
		change_pte(0xb8, bg_term);
	}

	run_proc(8, bg_term);


}


/*
* 	void switchTerm(int num) 
*   Inputs:num - The number of the terminal the user wishes to switch to 
*   Return Value: none
* 	This function is called whenever the user wishes to view/start a new terminal  
*/
void switchTerm(int num) {
	fake_sti = 1;

	// save current video memory and cursor pos
	if(active_term == bg_term) 
		memcpy((void*)(_4KB + active_term * _4KB), (void*)(0xb8000), _4KB);
	getXY((int*) &cursor_pos[bg_term][0]);

	// save state (esp and ebp) of current process
	asm volatile (
		"movl %%esp, %0 \n"
		"movl %%ebp, %1 \n"
		: "=r" (proc_esp[bg_term]), "=r" (proc_ebp[bg_term])
	);

	// restore pte
	change_pte(0xb8, 0xb8);

	// switch active terminal
	active_term = num;

	// if the new terminal has been initialized
	if(term_init[num]) {
		// restore video and cursor
		memcpy((void*)(0xb8000), (void*)(_4KB + active_term * _4KB), _4KB);
		change_coord(cursor_pos[active_term][0], cursor_pos[active_term][1]);

		bg_term = num;
		fake_sti = 0;
		run_proc(1, active_term);

	// if new terminal has not been intialized before
	} else {
		bg_term = num;
		// run shell
		//printf(".\n.\nNew terminal: active_term: (%d) | proc_active: (%d, %d, %d)\n", active_term, proc_active[0], proc_active[1], proc_active[2]);

		// set as initialized
		term_init[active_term] = 1;
		clear();

		// remnants of done_kbd_handle
		sti();
		send_eoi(1);
		fake_sti = 0;

		// execute new shell
		execute((unsigned char*) "shell");
	}
}

/*
* 	void init_terminals() 
*   Inputs:num - The number of the terminal the user wishes to switch to 
*   Return Value: none
* 	This function is called in the beginning of the OS. The settings for the different terminals is initialized here 
*/
void init_terminals() {
	// current active terminal
	active_term = 0;

	// has terminal ever been used
	term_init[0] = 1;	// yes
	term_init[1] = 0;	// no
	term_init[2] = 0;	// nay

	// active processes in each terminal
	proc_active[0] = -1; 	// nothing
	proc_active[1] = -1;	// nothing
	proc_active[2] = -1;	// nothing

	fake_sti=0;

	// doevents scheduler
	bg_term = 0;

	int i;
	for(i=0; i<4096; i++)
		zeros[i] = 0;

	for(i=1; i<5; i++)
		memcpy((void*)(0x1000 * i), (void*)&zeros[0], 0x1000);
}


/*
* 	void run_proc(int eoi, int proc_id) 
*   Inputs:num - eoi and the process id of process that needs to be run 
*   Return Value: none
* 	This function is called with a specific process id. The process environment is set up and the settings for it is loaded 
*/
void run_proc(int eoi, int proc_id) {
	// grab pde of new process
	uint32_t* temp = get_PDE(proc_active[proc_id]);


	// update cr3 to new pde
	asm volatile (
		"movl %0, %%eax \n"
		"movl %%eax, %%cr3 \n"
		:
		: "r" (temp)
	);


	// return to kernel and point to kernel stack
	tss.ss0 = KERNEL_DS;
	tss.esp0 = proc_esp[proc_id];
	
	// restore ebp and kernel stack
	tss.ebp = proc_ebp[proc_id];
	tss.esp = proc_esp[proc_id];

	sti();
	if (eoi > 0)
		send_eoi(eoi);

	asm volatile(
		"movl %0, %%ebp \n"
		"movl %1, %%esp \n"
		:
		: "r" (proc_ebp[proc_id]), "r" (proc_esp[proc_id])
	);
}

/*
* 	int get_active_term() 
*   Inputs:none
*   Return Value: none
* 	Helper function that returns the terminal number that user is viewing
*/
int get_active_term() { return active_term; }


/*
* 	int get_bg_term()
*   Inputs:none
*   Return Value: none
* 	Helper function that returns the background terminal number that is using CPU cycles
*/
int get_bg_term() { return bg_term; }


/*
* 	int is_active(int proc_id) 
*   Inputs:process id 
*   Return Value: none
* 	Helper function that returns a boolean value on whether the given process is active or not
*/
int is_active(int proc_id) {
	if (proc_active[active_term] == proc_id) 
		return 1;
	return 0;
}

/*
* 	void new_process(int proc_id) 
*   Inputs:process id 
*   Return Value: none
* 	Helper function that is called when a new process is started. The correct settings are loaded 
*/
void new_process(int proc_id) {
	proc_active[active_term] = proc_id;
	bg_term = proc_id;
}

/*
* 	void remove_process(int pProc_id, int pESP, int pEBP)
*   Inputs:int pProc_id, int pESP, int pEBP
*   Return Value: none
* 	Helper function that is called when a  process needs to be removed after it has been sucesfully completed 
*/
void remove_process(int pProc_id, int pESP, int pEBP) {
	proc_active[active_term] = pProc_id;
	proc_esp[active_term] = pESP;
	proc_ebp[active_term] = pEBP;
}


/*
* 	void gbg4()
*   Inputs:none
*   Return Value: none
* 	Helper function that is garbage
*/
void gbg4(){
	fotp* gbg = otp_terminal; gbg = otp_dir; gbg = otp_rtc; gbg = otp_file;
}
