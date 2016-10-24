#include "sys_call.h"
#include "lib.h"
#include "filesys.h"
#include "paging.h"
#include "handlers.h"
#include "keyboard.h"
#include "rtc.h"
#include "sched.h"

uint8_t args[1024] ;


/*
* uint32_t halt (uint8_t status) 
*   Inputs: uint8_t status
*   Return Value: Success/Fail in a uint32_t 
*	Function: Function that is called when a process is halted. Manages the process id and other settings to switch 
*   back to parent process. Returns 0 on success
*				
*/
uint32_t halt (uint8_t status) {
	// grab the current pcb and get proc_id
	pcb_t* pcb = get_curr_pcb();
	pcb_t* parent_pcb = (pcb_t*)pcb->old_pcb;

	int proc_id = pcb->proc_id;

	// if we are running the first shell 
	if (proc_id == 0) shutdown();

	// reset page
	remove_page(proc_id);

	// return to kernel and point to kernel stack
	tss.ss0 = KERNEL_DS;
	tss.esp0 = (int) (pcb->kstack);
	
	// restore ebp and kernel stack
	tss.ebp = (int)(pcb->old_ebp);
	tss.esp = (int)(pcb->kstack);
	
	// reset cr3 (paging)
	if(is_active(proc_id)) {

		// remove process from schedule
		remove_process(parent_pcb->proc_id, (int)pcb->kstack, (int)pcb->old_ebp);

		
		asm volatile("sti");

		uint32_t* temp = &(PDE[(parent_pcb->proc_id)+1][0]);
		asm volatile (
			"movl %0, %%eax \n"
			"movl %%eax, %%cr3 \n"
			:
			: "r" (temp)
		);
		
		// reset ebp and esp
		asm volatile(
			"movl %0, %%ebp \n"
			"movl %1, %%esp \n"
			:
			: "r" (pcb->old_ebp), "r" (pcb->kstack)
		);

		// end program
		asm volatile("jmp usr_end");
	}
	
	return 0;
}
/*
* int32_t getargs(uint8_t* buf1, int32_t nbytes) 
*   Inputs: (uint8_t* buf1, int32_t nbytes)
*   Return Value: Success/Fail in a int32_t 
*	Function: Function that is called when a program needs args provided by the user. Parses the args from the buffer of the command
*   given by the user  
*   Returns 0 on success		
*/
int32_t getargs(uint8_t* buf1, int32_t nbytes) {

	int i= 0 ; 
	if(args[0]=='\n' || args[0]==NULL)
		return -1;

	for (i = 0; args[i] != ' ' && args[i] != NULL && args[i] != '\n' && i < nbytes ; i++) {
		buf1[i] = args[i]; 
	
		// Checking if the arg is too big for the buffer 
		if (i >= 30) return -1;  //error checking... check what the actual limit should be 
	}

	strcpy((int8_t*)buf1,(int8_t*)&args);
	buf1[i]=NULL ;
	return 0 ;
}

/*
*  	uint32_t execute (const uint8_t* command) 
*   Inputs: const uint8_t* command  - buffer contains users input string
*   Return Value: Success/Fail in a uint32_t 
*	Function: Function that is called when a new program has to be executed. It finds the code in the file system and sets up paging 
* 	to get the program started 
*   given by the user  
*   Returns 0 on success		
*/
uint32_t execute (const uint8_t* command) {

	int8_t file_name[namesize/2] = {0};
	uint32_t i = 0;	
	uint32_t j = 0;	
	uint32_t k = 0;	
	int file_size;
	int bytes_read;
	int proc_id;
	uint32_t addr;
	uint8_t new_addr[4];
	uint32_t v_addr;
	uint32_t* curr_pcb_ptr;
	uint32_t old_ESP, old_EBP;
	
	uint8_t temp2[namesize]; 
	
	for (i = 0; i < 20; i++) {
		temp2[i] = command[i];
	}

	asm volatile (
		"movl %%esp, %0 \n"
		"movl %%ebp, %1 \n"
		: "=r" (old_ESP), "=r" (old_EBP)
	);
	
	curr_pcb_ptr = (uint32_t*) (old_ESP & _10MSB);

	//Count number of the chars in string 
	// also end on eos
	for (i = 0; temp2[i] != ' ' && temp2[i] != NULL && temp2[i] != '\n' ; i++);

	
	//file_name = malloc(i*sizeof(uint8_t));
	strncpy(&file_name[0],(int8_t *) temp2,i); 

	j = i ;
	k = 0 ;
	
	while(temp2[j] == ' ') j++; 
	
	while(temp2[j]!='\n' && temp2[j]!=NULL)
	{
		args[k] = temp2[j] ;
		j++;
		k++;
	}
	args[k] = NULL ;
	
	
	//Error check if file even exists  
	dentry_t dentry;
	file_size = read_dentry_by_name(file_name,&dentry); 
	if (file_size == -1) return -1;

	// let's just read_data since we're in kernel
	uint8_t file_buf[file_size];
	bytes_read = read_data(dentry.inode_num, 0, file_buf, file_size);
	if (bytes_read == -1) return -1;

	// executable check
	if (file_buf[0] != ELF1) return -1;
	if (file_buf[1] != ELF2) return -1;
	if (file_buf[2] != ELF3) return -1;
	if (file_buf[3] != ELF4) return -1;

	bytes_read = read_data(dentry.inode_num, PROC_HEADER_EIP, &new_addr[0], 4);
	addr = new_addr[0] | (new_addr[1] << 8) | (new_addr[2] << 16) | (new_addr[3] << 24);
	v_addr = addr;

	proc_id = new_pde(addr);
	if (proc_id == -1) return -1;
	
	new_process(proc_id);

	uint32_t* temp = &(PDE[proc_id+1][0]);
	asm volatile (
		"movl %0, %%eax \n"
		"movl %%eax, %%cr3 \n"
		:
		: "r" (temp)
	);

	tss.ss0 = KERNEL_DS;
	tss.esp0 = _128MB - _8KB * proc_id - 4;
	
	addr = _PROC_STORAGE;

	bytes_read = read_data(dentry.inode_num, 0, (void*)addr, file_size);

	pcb_t* new_pcb;
	new_pcb = (pcb_t*) (_128MB - ((proc_id + 1) * _8KB));

	new_pcb->fd[0].fotp = otp_terminal;
	new_pcb->fd[0].inode = NULL;
	new_pcb->fd[0].fp = NULL;
	new_pcb->fd[0].flags = PCB_FLAG_TERM_PRESENT;

	new_pcb->fd[1].fotp = otp_terminal;
	new_pcb->fd[1].inode = NULL;
	new_pcb->fd[1].fp = NULL;
	new_pcb->fd[1].flags = PCB_FLAG_TERM_PRESENT;

	new_pcb->old_pcb = curr_pcb_ptr;
	new_pcb->kstack = (uint32_t*) old_ESP;
	new_pcb->old_ebp = (uint32_t*) old_EBP;
	new_pcb->proc_id = proc_id;
	
	//Saving the pointer to command to later fetch args 
	//new_pcb->command_2 = (uint8_t*) command;

	if (proc_id < 0 || proc_id > 7) return -1;

	change_pte(0xb8, 0xb8);
	if(get_active_term() == get_bg_term()) 
		memcpy((void*)(_4KB + get_active_term() * _4KB), (void*)(0xb8000), _4KB);

	change_cr3(proc_id);

	// setup stack for iret 
	asm volatile(
		//"cli \n"
		"pushl $0x002B	\n" // #define USER_DS 0x002B
		"pushl %0 		\n"
		"pushf		 	\n"
		//"popl %%eax		\n"
		//"orl $0x200, %%eax \n"
		//"pushl %%eax	\n"
		"pushl $0x0023 	\n" // #define USER_CS 0x0023
		"pushl %1 		\n"
		:
		:"r" (_128MB + _4MB - 4), "r" (v_addr)
	);

	
	asm volatile("sti");

	// go into the user program
	asm volatile("iret");
	
	// return address after user program halts
	asm volatile(
		"usr_end: \n"
	);

	return 0;
}

/*
*  	uint32_t sys_open (const char* filename) 
*   Inputs: (const char* filename) Name of file that needs to be opened
*   Return Value: Success/Fail in a uint32_t 
*	Function: Function that is called when the given file needs to opened
*   given by the user  
*   Returns 0 on success		
*/
uint32_t sys_open (const char* filename) {
	pcb_t* pcbEntry = get_curr_pcb();
	dentry_t myDentry;

	// look for available slot in pcb
	int i, fd = -1;
	for(i=2; i<8; i++) {
		if (!(pcbEntry->fd[i].flags & 0x1)) { // check the last bit
			fd = i;
			break;
		}
	}

	// if no slots available, return -1
	if (fd == -1) return fd;
	
	// check file name is valid
	if (filename == "") return -1;

	// get dentry from filename
	if (-1 == read_dentry_by_name((char*)filename, &myDentry))
		return -1;

	// set to occupied
	pcbEntry->fd[fd].flags = 0x1;
	
	switch(myDentry.file_type) {
		case 0: //RTC
			pcbEntry->fd[fd].flags |= 0x0; // 0000 (bit 0)
			pcbEntry->fd[fd].inode = NULL;
			pcbEntry->fd[fd].fotp = otp_rtc;
			break;
		case 1: //DIR
			pcbEntry->fd[fd].flags |= 0x2; // 0010 (bit 1)
			pcbEntry->fd[fd].inode = NULL;
			pcbEntry->fd[fd].fotp = otp_dir;

			break;
		case 2: //FILE
			pcbEntry->fd[fd].flags |= 0x4; // 0100 (bit 2)
			pcbEntry->fd[fd].fotp = otp_file;
			pcbEntry->fd[fd].fp = 0;
			break;
	}

	// call appropriate open function and check that it opened successfully
	if (0 > pcbEntry->fd[fd].fotp[0](fd, (void*)filename, 0)) {
		// fnc_open for this particular file failed.
	
		pcbEntry->fd[fd].flags = 0; // clear present bit
		return -1;
	}

	
	asm volatile("sti");
	return fd;
}

/*
*  	sys_read (uint32_t fd, void* my_buf, uint32_t nbytes) 
*   Inputs: uint32_t fd, void* my_buf, uint32_t nbytes)
*   Return Value: Success/Fail in a uint32_t 
*	Function: Function that is called when the given fd needs to read
*   Returns 0 on success		
*/
uint32_t sys_read (uint32_t fd, void* my_buf, uint32_t nbytes) {
	// check if fd is valid
	if(fd < 0 || fd > 7 || fd == 1)
		return -1;

	// check if given fd is open (check present bit)
	if ((get_curr_pcb()->fd[fd].flags & 0x1) == 0) 
		return -1;

	
	asm volatile("sti");
	// call appropriate function
	return get_curr_pcb()->fd[fd].fotp[1](fd, my_buf, nbytes);
}

/*
*  uint32_t sys_write (uint32_t fd, void* my_buf, uint32_t nbytes)
*   Inputs: uint32_t fd, void* my_buf, uint32_t nbytes)
*   Return Value: Success/Fail in a uint32_t 
*	Function: Function that is called when the given fd needs to wrote 
*   Returns 0 on success		
*/
uint32_t sys_write (uint32_t fd, void* my_buf, uint32_t nbytes) {
	// check if fd is valid
	if(fd <= 0 || fd > 7)
		return -1;

	// check if given fd is open (check present bit)
	if ((get_curr_pcb()->fd[fd].flags & 0x1) == 0) 
		return -1;

	// call appropriate function
	return get_curr_pcb()->fd[fd].fotp[2](fd, my_buf, nbytes);
}


/*
*  uint32_t sys_close (uint32_t fd) 
*   Inputs:sys_close (uint32_t fd) 
*   Return Value: Success/Fail in a uint32_t 
*	Function: Function that is called when the given fd needs to close
*   Returns 0 on success		
*/
uint32_t sys_close (uint32_t fd) {
	// check if fd is valid
	// for sys_close it can't be lower than 2 since stdin/stdout
	if(fd < 2 || fd > 7)
		return -1;

	// if its not present, can't close
	if (((get_curr_pcb()->fd[fd].flags) & 0x1) == 0)
		return -1;

	get_curr_pcb()->fd[fd].flags = 0;
	return 0;
}


/*
*  pcb_t* get_curr_pcb() {
*   Inputs:none
*   Return Value: Returns a pointer to the current PCB struct of the current process that is running
*	Function: Function that returns a pointer to the PCB struct of the current process that is running
*   Returns 0 on success		
*/
pcb_t* get_curr_pcb() {

	uint32_t esp;

	asm volatile (
		"movl %%esp, %0 \n"
		: "=r" (esp)
	);

	// if esp points to kernel stack,
	// there are no processes running and return error
	// this, in theory, should never happen
	esp &= ESP2PCB;
	//if(esp == 0x7fe000 ) return -1;

	return (pcb_t*) esp;
}

uint32_t * get_PDE(int proc_id){
return &(PDE[proc_id+1][0]);
}


																																																		void value_check() {}
																																																void reg_save() {}
																																																void reg_restore() {}

/*
* void sys_vidmap();
*   Inputs: (uint8_t)**addr
*   Return Value: Success/Fail
*	Function: Maps video memory for user space program and fills addr
*				with the prepared address. Returns 0 on success,
*				and returns -1 on failure.
*/
int32_t sys_vidmap(uint8_t** addr) {
	// check validity of addr
	if (addr == ((uint8_t **) CHECK_MASK)) return -1;
	if(addr == NULL)
		return -1;

	// make sure addr is in current process's user space
	//if(((uint32_t)addr & ESP2PCB) != (uint32_t)get_curr_pcb())
	//	return -1;

	// retrieve current process id from pcb
	int proc_id = (int)(get_curr_pcb()->proc_id);
	uint8_t* vid_addr;

	// map video address 
	PTE[1] = VIDEO_ADDR;
	PTE[1] |= CTRL_PTE_USR;
	
	// IDX12 is completely arbitary
	PDE[proc_id+1][12] = (unsigned int)&PTE; 
	PDE[proc_id+1][12] &= _20MSB;
	PDE[proc_id+1][12] |= CTRL_VIDEO_USR;
	
	// Fill *addr to be the address of the mapped video memory
	// (12 << 22) represents PDE index of 12
	// (1 << 12) represent PTE index of 1
	// Those two OR'd together specifies location of user video memory
	vid_addr = (uint8_t*)((12 << _SHIFT_PDE)| 1 << _SHIFT_PTE);
	*addr = vid_addr;

	// return 0 on success
	if(*addr == vid_addr)
		return 0;
	else
		return -1;
}

/*
* void sys_vidmap();
*   Inputs: (uint8_t)**addr
*   Return Value: Success/Fail
*	Function: Maps video memory for user space program and fills addr
*				with the prepared address. Returns 0 on success,
*				and returns -1 on failure.
*/
int32_t sys_set_handler() {
	return -1;
}

int32_t sys_sigreturn() {
	return -1;
}

/*
*int32_t sys_error() 
*   Inputs: none
*   Return Value: -1
*	Function: Helper function that is called when a sys_error is encountered 
*				with the prepared address. Returns 0 on success,
*				and returns -1 on failure.
*/
int32_t sys_error() {
	//puts("sys_call_error");
	return -1;
}


/*
*  These functions are used as wrappers inorder to join the syscalls with the appropriate helper function written by us
* 
*/ 
int32_t fnc_fopen(uint32_t fd, void* my_buf, uint32_t nbytes) { 
	return file_open(fd, my_buf);
}
int32_t fnc_fread(uint32_t fd, void* my_buf, uint32_t nbytes){
	pcb_t * pcb = get_curr_pcb();

	int bytes_read = read_data(pcb->fd[fd].inode, pcb->fd[fd].fp, (uint8_t*)my_buf, nbytes);
	pcb->fd[fd].fp += bytes_read;
	return bytes_read;
}
int32_t fnc_fwrite(uint32_t fd, void* my_buf, uint32_t nbytes) {
	return file_write();
}


int32_t fnc_term_open(uint32_t fd, void* my_buf, uint32_t nbytes) {
	return open_terminal(my_buf);
}
int32_t fnc_term_read(uint32_t fd, void* my_buf, uint32_t nbytes) {
	return read_terminal(fd, my_buf, nbytes);
}
int32_t fnc_term_write(uint32_t fd, void* my_buf, uint32_t nbytes) {
	return write_terminal(fd, my_buf, nbytes);
}


int32_t fnc_rtc_open(uint32_t fd, void* my_buf, uint32_t nbytes) {
	return rtc_open(my_buf);
}
int32_t fnc_rtc_read(uint32_t fd, void* my_buf, uint32_t nbytes) {
	return rtc_read();
}
int32_t fnc_rtc_write(uint32_t fd, void* my_buf, uint32_t nbytes) {
	rtc_write(*((int32_t *)my_buf));
	return 0; // RTC_WRITE RETURNS VOID???
}

int32_t fnc_dir_open(uint32_t fd, void* my_buf, uint32_t nbytes) {
	return 0;
}
int32_t fnc_dir_read(uint32_t fd, void* my_buf, uint32_t nbytes) {
	return dir_read(my_buf);
}
int32_t fnc_dir_write(uint32_t fd, void* my_buf, uint32_t nbytes) {
	return 0;
}

void gbg(){
	fotp* gbg = otp_terminal; gbg = otp_dir; gbg = otp_rtc; gbg = otp_file;
}
