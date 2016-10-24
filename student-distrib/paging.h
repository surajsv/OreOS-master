/* paging.h - Header file for the paging.c that contains the function for initializing paging
 * controller
 */
#ifndef _PAGING_H
#define _PAGING_H

// Constant quantities
#define _4KB 4096
#define NUM_ENTRIES 1024
#define MAX_NUM_PROC 8

// Initialization control bits
#define CTRL_PTE 0x01		// AV.G P0AC WSRP (4KB PTE)
 							// 0000 0000 0001
							
#define CTRL_PTE_USR 0x07	// AV.G P0AC WSRP (4KB PTE)
 							// 0000 0000 0111

#define CTRL_VIDEO 0x01		// AV.G P0AC WSRP (4KB PDE)
 							// 0000 0000 0001

#define CTRL_VIDEO_USR 0x07	// AV.G P0AC WSRP (4KB PDE)
 							// 0000 0000 0111

#define CTRL_KERNEL 0x183	// AV.G PDAC WSRP (4MB PDE)
 							// 0001 1000 0011

#define CTRL_EMPTY 0x83		// AV.G PDAC WSRP (4MB PDE)
 							// 0000 1000 0011


#define CTRL_EMPTY_USR 0x87	// AV.G PDAC WURP (4MB PDE)
 							// 0000 1000 0111


extern void init_paging();
extern int new_pde(uint32_t v_entry);
extern int change_cr3(int proc_slut);

void remove_page(int proc_id);
extern void change_pte(int i, int value);

uint32_t PDE[MAX_NUM_PROC][NUM_ENTRIES]__attribute__((aligned(_4KB)));
uint32_t PTE[NUM_ENTRIES]__attribute__((aligned(_4KB)));

#endif /* _PAGING_H */
