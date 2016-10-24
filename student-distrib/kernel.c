/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "paging.h"
#include "handlers.h"
#include "filesys.h"
#include "rtc.h"
#include "sys_call.h"
#include "sched.h"

volatile int keyboard_flag;
volatile unsigned char scancode_prev;

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

/*
* void init_idt() 
*   Inputs: none
*   Return Value: none
*	Function: helper function to initialize the idt
*/
void init_idt() {

	// fill in IDT
	int i; //counter
	for(i=0; i<256; i++) {

		// default idt values
		idt[i].seg_selector = KERNEL_CS;
		idt[i].reserved3 = 0;
		idt[i].reserved2 = 1;
		idt[i].reserved1 = 1;
		idt[i].reserved0 = 0;
		idt[i].size = 1;
		idt[i].dpl = 0;
		idt[i].present = 1;

		switch(i) {
			case 0:
				SET_IDT_ENTRY(idt[i], divide_error);
				break;
			case 1:
				SET_IDT_ENTRY(idt[i], debug);
				break;
			case 2:
				SET_IDT_ENTRY(idt[i], nmi);
				break;
			case 3:
				SET_IDT_ENTRY(idt[i], int3);
				break;
			case 4:
				SET_IDT_ENTRY(idt[i], overflow);
				break;
			case 5:
				SET_IDT_ENTRY(idt[i], bounds);
				break;
			case 6:
				SET_IDT_ENTRY(idt[i], invalid_op);
				break;
			case 7:
				SET_IDT_ENTRY(idt[i], device_not_available);
				break;
			case 8:
				SET_IDT_ENTRY(idt[i], double_fault);
				break;
			case 9:
				SET_IDT_ENTRY(idt[i], coprocesseur_segment_overrun);
				break;
			case 10:
				SET_IDT_ENTRY(idt[i], invalid_tss);
				break;
			case 11:
				SET_IDT_ENTRY(idt[i], segment_no_present);
				break;
			case 12:
				SET_IDT_ENTRY(idt[i], stack_segment);
				break;
			case 13:
				SET_IDT_ENTRY(idt[i], general_protection);
				break;
			case 14:
				SET_IDT_ENTRY(idt[i], page_fault);
				break;
			case 15:
				SET_IDT_ENTRY(idt[i], none);
				break;
			case 16:
				SET_IDT_ENTRY(idt[i], coprocessor_error);
				break;
			case 17:
				SET_IDT_ENTRY(idt[i], alignement_check);
				break;
			case 18:
				SET_IDT_ENTRY(idt[i], machine_check);
				break;
			case 32:
				SET_IDT_ENTRY(idt[i], rtc_handler);
				break;
			case 33:
				SET_IDT_ENTRY(idt[i], keyboard_handler);
				break;
			case 40:
				SET_IDT_ENTRY(idt[i], periodic_interrupt_handler);
				break;
			case 128:
				SET_IDT_ENTRY(idt[i], sys_call);
				idt[i].dpl = 3;
				break;
			default:
				SET_IDT_ENTRY(idt[i], unused_exception);
				break;
		}
	}

	lidt(idt_desc_ptr);
}

/*
* void entry (unsigned long magic, unsigned long addr)
*   Inputs: none
*   Return Value: none
*	Function: Check if MAGIC is valid and print the Multiboot information structure
*   pointed by ADDR.
*/
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;

	/* Clear the screen. */
	clear();

	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;

		set_filesys_meta((uint32_t*) mod->mod_start);

		while(mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}

	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}
	
	// garbage to get rid of double inclusion error for fotp
	fotp* gbg = otp_terminal; gbg = otp_dir; gbg = otp_rtc; gbg = otp_file;

	/*
	dentry_t my_dentry;
	int j;
	int retval;
	
	// TEST FUNCTION FOR READ_DENTRY_BY_INDEX (i.e. READ DIRECTORY)
	// This function is the same as calling "ls" or "dir"
	printf("\ntest read_dentry_by_index:\n");
	for (j=0; j<20; j++) {
		retval = read_dentry_by_index(j,&my_dentry);
		if (retval != -1) {
			if (my_dentry.file_type == 0)
				printf("%s                                   RTC\n",
					my_dentry.file_name);
			else if (my_dentry.file_type == 1)
				printf("%s                                     Dir\n",
					my_dentry.file_name);
			else if (my_dentry.file_type == 2)
				printf("%s\n", my_dentry.file_name);
		}
		else
			printf("index out of bounds\n");
	}
	*/

	init_buffers();
	init_terminals();

	capson=0 ;
	keyboard_flag = 0 ;
	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */
	init_idt();
	i8259_init(); // Init the PIC
	rtc_init();
	init_paging();

	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	printf("Enabling Interrupts\n");
	sti();
	
	// TESTING SYSTEM CALL
	/*
	asm volatile (
		"movl $1, %%eax \n"
		"movl $2, %%ebx \n"
		"movl $3, %%ecx \n"
		"movl $4, %%edx \n"
		"int $0x80 \n"
		:
	); */

	//enable_irq(0); // enable rtc
	enable_irq(1); // enable keyboard
	enable_irq(2); // enable slave, then
	enable_irq(8); // enable periodic interrupt
	clear();
	fflag = 0;
	/*rtc_test() ;

	clear_buffer(BUF_SIZE,buf);
	fflag = 0 ;
	read_terminal(0,buf,BUF_SIZE-1);
	//printf("%d",r) ;
	int a =ind_entered ;
	write_terminal(1,print_buf,a);
	//printf("\n %d",r) ;
*/

	/* Execute the first program (`shell') ... */
	//uint8_t blahp[] = "testprint ";
	uint8_t blahp[] = "shell ";
	//uint8_t blahp[] = "hello ";
	//uint8_t blahp[] = "ls ";

	
	asm volatile (
		"movl %0, %%ebx; \n"
		"movl $0x2, %%eax; \n"
		"int $0x80; \n"
		:
		:"r" (&blahp)
	);



	asm volatile("exit_shell :");


	puts("Exited from shell. Nicely spinning\n");

	/* Spin (nicely, so we don't chew up cycles) */
	asm volatile(".1: hlt; jmp .1;");
}

/*
* void shutdown 
*   Inputs: none
*   Return Value: none
*	Function: This function is called when exit is called on the last shell running. It exits shell and shuts down 
*   pointed by ADDR.
*/
void shutdown() {
	asm volatile("jmp exit_shell");
}
