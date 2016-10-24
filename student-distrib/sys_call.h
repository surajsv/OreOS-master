#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "filesys.h"

#define _4KB 4096
#define _8KB 8192
#define _4MB 4194304
#define _128MB 134217728
#define _PROC_STORAGE 0x8048000
#define VIDEO_ADDR 0xB8000
#define CHECK_MASK	0x400000

// shifts
#define _SHIFT_PDE 22
#define _SHIFT_PTE 12

// masks
#define _10MSB 0xffe000
#define ESP2PCB 0xfffe000
#define _20MSB 0xFFFFF000;
#define PCB_FLAG 0x6

#define namesize 64
#define ELF1 0x7F
#define ELF2 0x45
#define ELF3 0x4c
#define ELF4 0x46

#define PROC_HEADER_EIP 24
#define PCB_FLAG_TERM_PRESENT 0x7

// file operations table functions
int32_t fnc_fread(uint32_t fd, void* my_buf, uint32_t nbytes);
int32_t fnc_fopen(uint32_t fd, void* my_buf, uint32_t nbytes);
int32_t fnc_fwrite(uint32_t fd, void* my_buf, uint32_t nbytes);

int32_t fnc_term_read(uint32_t fd, void* my_buf, uint32_t nbytes);
int32_t fnc_term_open(uint32_t fd, void* my_buf, uint32_t nbytes);
int32_t fnc_term_write(uint32_t fd, void* my_buf, uint32_t nbytes);

int32_t fnc_rtc_read(uint32_t fd, void* my_buf, uint32_t nbytes);
int32_t fnc_rtc_open(uint32_t fd, void* my_buf, uint32_t nbytes);
int32_t fnc_rtc_write(uint32_t fd, void* my_buf, uint32_t nbytes);

int32_t fnc_dir_read(uint32_t fd, void* my_buf, uint32_t nbytes);
int32_t fnc_dir_open(uint32_t fd, void* my_buf, uint32_t nbytes);
int32_t fnc_dir_write(uint32_t fd, void* my_buf, uint32_t nbytes);

typedef int32_t (* fotp)(uint32_t fd, void* my_buf, uint32_t nbytes);
static fotp otp_terminal[] = {fnc_term_open, fnc_term_read, fnc_term_write};
static fotp otp_dir[] = {fnc_dir_open, fnc_dir_read, fnc_dir_write};
static fotp otp_rtc[] = {fnc_rtc_open, fnc_rtc_read, fnc_rtc_write};
static fotp otp_file[] = {fnc_fopen, fnc_fread, fnc_fwrite};


typedef struct {
    fotp* fotp; // file operations table pointer
    uint32_t inode;
    uint32_t fp; // file position
	uint32_t flags;
} fd_t;

typedef struct {
	fd_t fd[8];
	uint32_t* old_pcb;
	uint32_t* kstack;
	uint32_t* old_ebp;
	int proc_id;
	uint8_t* command_2;
	
} pcb_t;

uint32_t halt (uint8_t status);
uint32_t execute (const uint8_t* command);
uint32_t sys_open (const char* filename);
uint32_t sys_close (uint32_t fd);
uint32_t sys_read (uint32_t fd, void* my_buf, uint32_t nbytes);
uint32_t sys_write (uint32_t fd, void* my_buf, uint32_t nbytes);
int32_t getargs (uint8_t* buf1, int32_t nbytes);
int32_t sys_vidmap();
int32_t sys_set_handler();
int32_t sys_sigreturn();
int32_t sys_error();


void sys_call();
extern pcb_t* get_curr_pcb();
extern uint32_t* get_PDE(int proc_id);
extern void shutdown();


#endif /* _SYSCALL_H */
