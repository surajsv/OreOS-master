/* defining_handlers.c - the C part of the kernel
 */
#ifndef _HANDLERS_H
#define _HANDLERS_H

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "paging.h"
#include "keyboard.h"

extern void exception_handler() ;
extern void unused_exception() ;
extern void divide_error() ;
extern void debug() ;
extern void nmi() ;
extern void int3() ;
extern void overflow() ;
extern void bounds()  ;
extern void invalid_op() ;
extern void device_not_available() ;
extern void double_fault() ;
extern void coprocesseur_segment_overrun();
extern void invalid_tss() ;
extern void segment_no_present() ;
extern void stack_segment() ;
extern void general_protection() ;
extern void page_fault() ;
extern void none()  ;
extern void coprocessor_error() ;
extern void alignement_check() ;
extern void machine_check() ;
extern void rtc_handler();

#endif /* _HANDLERS_H */
