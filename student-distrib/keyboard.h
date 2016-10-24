#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#define BREAKCC 0x80
#define KEY_INB 0x60

#define KEY_CAPS 0x3A
#define SHIFT 0x2A
#define SHIFT2 0x36
#define SHIFT_OFF 0xAA
#define SHIFT_OFF2 0xB6
#define CTRL_ON 0x1D
#define CTRL_ON2 0xE0
#define CTRL_OFF 0x9D
#define BACKSPACE 0x0E
#define ALT 0x38
#define ALT_OFF 0xB8
#define F1 0x3B
#define TAB 0x0F

#define BUF_SIZE 1024

extern volatile int keyboard_flag;
extern volatile unsigned char scancode_prev;
volatile unsigned char capson;
volatile unsigned char ctrl_on;
volatile unsigned char shift_on;
volatile unsigned char alt_on;

volatile int fflag;

unsigned char buf[6][BUF_SIZE];
static volatile int shell_init[6];
unsigned char print_buf[BUF_SIZE];
volatile unsigned int buf_index[6];
volatile unsigned int ind_entered[6];

extern void check_buf(int n, char x);
extern int curTerm();
extern int* buff_index();
extern void init_buffers();

extern void keyboard_handler();
extern int read_terminal(int32_t fd, unsigned char * buf1, int32_t nbytes) ;
extern int write_terminal(int32_t fd, const unsigned char * buf1, int32_t nbytes) ;
extern int open_terminal(const uint8_t* filename) ; //returns 0
extern int close_terminal(int32_t fd) ; //returns -1
extern void clear_buffer(int n,unsigned char * buf1) ;

#endif /* _KEYBOARD_H */
