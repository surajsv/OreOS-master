#include "filesys.h"
#include "lib.h"

int num_de;
int N; // number of inodes
int D; // number of data blocks

uint32_t* bbp; // boot block pointer
dentry_t* de_ptr; // pointer to the directory entries
uint32_t* inode_ptr; // pointer to the inodes
uint8_t* db_ptr; // pointer to the data blocks
int dir_index;

static int num_64B_entries = 64;
static int size_block = 4096;
static int max_str_length = 32;

// Function for setting the meta data of the filesystem.
// It is written as a function in order to facilitate easy changes
// of filesystem meta data structure.
// input: a pointer to the boot_block of the filesystem
// output: none
void set_filesys_meta(uint32_t* boot_block_ptr) {
	// Check documentation for explanation of how our filesystem is set up
	//     courses.engr.illinois.edu/ece391/
	//	   assignments->mp3->section 7.1
	num_de = boot_block_ptr[0];
	N = boot_block_ptr[1];
	D = boot_block_ptr[2];
	dir_index = 0;

	bbp = boot_block_ptr;
	de_ptr = (dentry_t*)((uint8_t*)boot_block_ptr + num_64B_entries);
	inode_ptr = (uint32_t*) ((uint8_t*) boot_block_ptr + size_block);
	db_ptr = (uint8_t*)((uint8_t*)boot_block_ptr + size_block
		+ N*size_block);
}

// Function for finding a dentry by name.
// input: fname, the name to be looked for
// 		  dentry, pointer to the dentry to be filled in
// returns: -1 if the file was not found, 0 if it does
int32_t read_dentry_by_name(const char* fname, dentry_t* dentry) {

	int i; // counter for dir entry 
	int check; // will be zero if strings match
	int length = strlen(fname); // length of given file name

	// if length is greater than 32, cannot possibly match, so return
	if (length > max_str_length-1)
		return -1;

	// Go through all the directory entries and see if the name matches
	for (i=0; i<num_de; i++) {
		check = strncmp((char*)fname, (char*)de_ptr[i].file_name, 
			max_str_length);

		// if equal copy the data into the given dentry
		if (check == 0) {
			strncpy((char*)dentry->file_name, (char*)de_ptr[i].file_name,
				max_str_length);
			dentry->file_type = de_ptr[i].file_type;
			dentry->inode_num = de_ptr[i].inode_num;
			check = 0;
			break;
		}
	}
	if (check != 0) return -1;

	// return size of the file
	uint32_t* curr_inode_ptr = (uint32_t*)((uint8_t*)inode_ptr
		+ size_block*dentry->inode_num);
	int B = curr_inode_ptr[0]; // the length in Bytes

	return B;
}

// Function for reading dentry given an index in to the directory entry
// block system.
// input: index, the index into the directory entry system (63 64B entries
//		  		 after the boot_block).
//		  dentry, pointer to the dentry to be filled in
// returns: -1 if index is invalid, 0 else
int32_t read_dentry_by_index(const uint32_t index, dentry_t* dentry) {

	int i; // counter for character array

	if (index < 0 || index >= num_de)
		return -1;
	else {
		for(i=0; i<32; i++)
			dentry->file_name[i] = de_ptr[index].file_name[i];

		dentry->file_type = de_ptr[index].file_type;
		dentry->inode_num = de_ptr[index].inode_num;
	}

	return 0;
}

// Function for reading data from an arbitrary file. 
// inputs: inode,   the inode to read from (this can be gotten from the
//				    above functions).
//		   offset,  the offset into the file to start reading from in bytes
//		   buf,     pointer to a buffer to fill in with the data
//		   length,  the number of bytes to be read
// returns: -1 for invalid inode number, otherwise, the number of bytes 
// 			copied
// NOTE: The caller should not pass in a buffer of smaller length than
//			 the amount of space allocated for the buffer.
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf_rd,
	uint32_t length) { 

	int block_offset; // the number of blocks into the data_block system
					  // to read from
	int offset_in_block; // the offset within the block to read from
	uint32_t* curr_inode_ptr = (uint32_t*)((uint8_t*)inode_ptr
		+ size_block*inode); // the pointer to the current inode
	int B = curr_inode_ptr[0]; // the length in Bytes

	// not enough bytes
	if (inode >= N)
		return -1;

	int i;
	for(i=0; i<length; i++) {
		
		// get offset into block data
		block_offset = (offset+i)/size_block;
		offset_in_block = (offset+i)%size_block;

		if (i+offset >= B)
			break; // => we have read to the end of the file

		// fill in buffer
		buf_rd[i] = db_ptr[size_block*curr_inode_ptr[block_offset+1] + 
			offset_in_block];
	}

	return i; // return number of bytes read
}

// Function for opening or closing a file
// input: fname, the name of the file/directory to open
// returns: -1 on unsuccessful file open, 0 else
int32_t file_open(uint32_t fd, const char* fname) {
	dentry_t myDentry;
	if (0 > read_dentry_by_name(fname, &myDentry))
		return -1;

	get_curr_pcb()->fd[fd].inode = myDentry.inode_num;
	return 0;
}

// Function to eventually be used for writing to the filesystem
// input: none
// returns: -1 on unsucessful file write, 0 else
int32_t file_write() {
	return -1;
}

// Function to eventually be used for closing a file
// input: none (eventually will be index into our open files
// returns: -1 on unsucessful file close, 0 else
int32_t file_close() {
	return 0;
}

void filesys_write(int32_t freq) {
	return; 
}

int dir_read(char* buff) {
	dentry_t dentry;
	int retval = read_dentry_by_index(dir_index, &dentry);

	dir_index++;

	if (retval == -1){
		dir_index = 0;
		return 0;
	}
	else {
		strncpy((char*) buff, (char*)dentry.file_name, max_str_length);
		return max_str_length;
	}

}

void garbage() {
	// garbage to get rid of double inclusion error for fotp
	fotp* gbg = otp_terminal; gbg = otp_dir; gbg = otp_rtc; gbg = otp_file;
}

/*
//NO NEED TO UNCOMMENT FOR THIS CHECKPOINT
int32_t filesys_read (int32_t index, void* buf, int32_t nbytes)
{
	dentry_t dentry; 
	//data should be read to the end of the file or the end of the buffer provided, whichever occurs sooner 
	read_dentry_by_index(fd,&dentry);
	dentry.inode_num; 
	read_data(dentry.inode_num,0,buf,4096);
	return num_of_bytes_read; 
}
*/
