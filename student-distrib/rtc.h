#ifndef _RTC_H
#define _RTC_H

#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "paging.h"
#include "handlers.h"

// ports for rtc
#define RTC_PORT 0x70
#define CMOS_PORT 0x71

// rtc registers
#define REG_A 0x8A
#define REG_B 0x8B
#define REG_C 0x8C

// rtc rates 
#define  RATE1  15
#define  RATE2	14	
#define  RATE3	13
#define  RATE4	12
#define  RATE5	11
#define  RATE6	10
#define  RATE7	9
#define  RATE8	8
#define  RATE9	7
#define  RATE10	6
#define  NMI 0x7F
#define  FREQ_MASK 0xF0 
#define  LAST4  0x0F
#define  NMI_OFF 0x80
#define  PIE  0x40
#define  C_MASK 0x0C 

extern int32_t rtc_read();
extern void rtc_write(int32_t freq);
extern int32_t rtc_open (const uint8_t* filename);
extern int32_t rtc_close (int32_t fd);
extern void periodic_interrupt_handler();
extern void rtc_init();
extern void rtc_test(); 

#endif /* _RTC _H */
