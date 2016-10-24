#ifndef _SCHED_H
#define _SCHED_H


volatile int cursor_pos[6][2];
unsigned char zeros[4096];


int active_term; 		// current active terminal
int term_init[3]; 		// terminal has been initialized
int proc_active[3];		// active process in terminal

int bg_term;
int fake_sti;

int proc_esp[3];		// state variable for the active process in each terminal
int proc_ebp[3];

extern void do_events();
extern int get_active_term();
extern int get_bg_term();
extern int is_active(int proc_id);
extern void switchTerm(int num);
extern void init_terminals();
extern void new_process(int proc_id);
extern void remove_process(int pProc_id, int pESP, int pEBP);

extern void run_proc(int eoi, int proc_id);

#endif /* _SCHED_H */
