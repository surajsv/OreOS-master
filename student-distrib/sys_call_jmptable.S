#define ASM     1

.global sys_call, jump_table

jump_table:
.long halt, execute, sys_read, sys_write, sys_open, sys_close, getargs, sys_vidmap, sys_set_handler, sys_sigreturn, sys_error

restore_esi:	.long 0x0
restore_edi:	.long 0x0

/*
* 	sys_call:
*   Inputs: none
*   Return Value: returns the return value of the helper function that was called from the jump table
*	Function: Maps video memory for user space program and fills addr
*
*/

sys_call:
	cli
	pushl %ebp
	movl %esp, %ebp

/*
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
*/
	
	movl %esi, restore_esi
	movl %edi, restore_edi

	pushl %edx 						## push args
	pushl %ecx
	pushl %ebx

	decl %eax						## decrement eax since jump_table starts with index 0

	cmpl $0, %eax					## eax must be greater than 0
	jl error_handle

	cmpl $10, %eax					## and eax must be less than 10
	jg error_handle

do_sys_call:
	call *jump_table(,%eax,4)		## call function

	sti
	addl $12, %esp 					## pop args


	movl restore_edi, %edi
	movl restore_esi, %esi

/*
	popl %ebx
	popl %ecx
	popl %edx

	popl %eax
*/
/*
	cmpl $7, %eax
	jne done

	movl $0, %eax

done:
*/
	leave
	iret

error_handle:
	movl $10, %eax
	jmp do_sys_call

