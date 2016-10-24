#ifndef _FILESYS_H
#define _FILESYS_H

#include "multiboot.h"
#include "types.h"
#include "sys_call.h"

typedef struct {
    uint8_t file_name[32]; //static array allocation
    uint32_t file_type;
    uint32_t inode_num;
	uint8_t reserved[24];
} dentry_t;

void set_filesys_meta(uint32_t* boot_block_ptr);

int32_t read_dentry_by_name(const char* fname, dentry_t* dentry);

int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, 
	uint32_t length);
	
int32_t file_close();

int32_t file_write();

int32_t file_open(uint32_t fd, const char* fname);

extern void filesys_write(int32_t freq);
extern int32_t filesys_read(int32_t index, void* buf, int32_t nbytes);

int dir_read(char* buff);

#endif /* _FILESYS_H */
