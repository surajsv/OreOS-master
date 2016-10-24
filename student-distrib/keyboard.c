// Function for handling keyboard interrupts

#include "handlers.h"
#include "lib.h"
#include "sys_call.h"
#include "sched.h"


static unsigned char kbd[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};	

static unsigned char kbd_sym[21] = {
	'<','_','>','?',')','!','@',
	'#','$','%','^','&','*','(',
	'~','+','{','}',':','|','"'};

/*
* void keyboard_handlers 
*   Inputs: none
*   Return Value: none
*	Function: Where the different cases for the different key presses by the user is handled 
*/
void keyboard_handler() {
	unsigned char scancode = inb(KEY_INB);

	cli();

	// caps lock code
	if(scancode==KEY_CAPS) {
		capson = (capson==1) ? 0 : 1;
		goto done_kbd_handle;
	}

	// shift on
	if(scancode==SHIFT || scancode == SHIFT2) {
		shift_on= 1;
		goto done_kbd_handle;
	}

	// shift off
	if(scancode==SHIFT_OFF || scancode == SHIFT_OFF2) {
		shift_on = 0;
		goto done_kbd_handle;
	}

	// control on
	if(scancode==CTRL_ON || scancode==CTRL_ON2) {
		ctrl_on= 1;
		goto done_kbd_handle;
	}

	// control off
	if(scancode==CTRL_OFF) {
		ctrl_on = 0;
		goto done_kbd_handle;
	}

	// alt
	if(scancode==ALT) {
		alt_on = 1;
		goto done_kbd_handle;
	}

	// alt off
	if(scancode==ALT_OFF) {
		alt_on = 0;
		goto done_kbd_handle;
	}

	// backspace
	if(scancode==BACKSPACE) {
		backspacing(1);
		goto done_kbd_handle;
	}

	// tab
	if(scancode==TAB) {
		puts("    ");


		check_buf(0, ' ') ;
		goto done_kbd_handle;
	}

	// clear screen
	if(ctrl_on==1 && kbd[scancode]=='l') {
		clear();
		change_coord(0,0);
		goto done_kbd_handle;
	}

	// scancodes greater than 0x80 are key press up codes
	if(scancode <= BREAKCC) {
		unsigned char x;
		uint32_t temp;

		// regular letters with caps lock on
		if(capson==1 && ((kbd[scancode] >= 'a') && (kbd[scancode] <='z'))) {
			x = kbd[scancode] - 32;
			putc(x);
		} 
		
		if (alt_on==1) {

			int newTerm = scancode-F1;
			if (newTerm < 6 && newTerm >= 0) {
				switchTerm(newTerm);

			} else {
				goto done_kbd_handle;
			}

		// if shift is on
		} else if(shift_on==1) {

			// shift on with regular letters
			if((kbd[scancode] >= 'a') && (kbd[scancode] <='z')) {
				x = kbd[scancode] - 32;
				putc(x);

			// shift on with special cases
			} else {
				temp = (int)kbd[scancode] - 44;

				if (temp >= 0 && temp < 14) {
						x = kbd_sym[temp];
						putc(x);

				} else {
					switch(kbd[scancode]) {
						case '`': 
							x = kbd_sym[14] ;
							putc(x) ;
							break ;

						case '=':
							x = kbd_sym[15] ;
							putc(x) ;
							break ;

						case '[':
							x = kbd_sym[16] ;
							putc(x) ;
							break ;

						case ']':
							x = kbd_sym[17] ;
							putc(x) ;
							break ;

						case ';':
							x = kbd_sym[18] ;
							putc(x) ;
							break ;

						case 92: // backslash
							x = kbd_sym[19] ;
							putc(x) ;
							break ;

						case 39: // apostrophe
							x = kbd_sym[20] ;
							putc(x) ;
							break ;
						default: 
							break ;			
					}
				}
			}

		} else {
			x = kbd[scancode];
			putc(kbd[scancode]);
		}

		if (buf_index[get_active_term()] != 127) {
			buf[get_active_term()][buf_index[get_active_term()]] = x;
		 	buf_index[get_active_term()]++;
		}

		check_buf(1, x);
	}

done_kbd_handle:
	sti();
	send_eoi(1);
	asm volatile("leave;\
					iret;") ;
}


/*
* void init_buffers() 
*   Inputs: none
*   Return Value: none
*	Function: We clear buffers with 0 values for later use. 
*   
*/
void init_buffers() {
	int i=0;
	for(i=0; i<6; i++) {
		buf_index[i] = 0;
		ind_entered[i] = 0;
	}
}


/*
* void check_buf(int n, char x)
*   Inputs: none
*   Return Value: none
*	Function: It checks and clears the current buffer 
*   
*/
void check_buf(int n, char x) {
	if(n==0) {
		int j ;
		for(j=0;j<4;j++) {
			buf[get_active_term()][buf_index[get_active_term()]] = ' ';
			buf_index[get_active_term()]++;
		}

	} else {

		if( x =='\n') {
			ind_entered[get_active_term()] = buf_index[get_active_term()];
			if(ind_entered[get_active_term()]==BUF_SIZE-1 && fflag==0)
				putc('\n') ;
			fflag = 1;
		}

	}
}


/* This function is to read the buffer it stops looping once we encounter a \n character
*INPUTS: fd is the file directory
*		buf is the buffer we are checking
*		nbytes is last index
* OUTPUTs : 0 on success and -1 on failure
*/
int read_terminal(int32_t fd, unsigned char * buf1, int32_t nbytes) {
	buf_index[get_active_term()] = 0;
	ind_entered[get_active_term()] = nbytes;

	while(!fflag){}

	fflag = 0;
	clear_buffer(nbytes,buf1);
	return ind_entered[get_active_term()];
}


/* This function is to write to the buffer and echo on the screen
*INPUTS: fd is the file directory
*		buf is the buffer we are checking
*		nbytes is last index
* OUTPUTs : 0 on success and -1 on failure
*/
int write_terminal(int32_t fd, const unsigned char * buf1, int32_t nbytes) {
	int i = 0;

	for(i=0;i<=nbytes;i++)
		putc(buf1[i]);
	
	if(nbytes!=BUF_SIZE-1)
		backspacing(0);

	buf_index[get_active_term()] = 0;
	return nbytes;
}

/* This function is to read the buffer it stops looping once we encounter a \n character
*INPUTS: filename is the filename to open
*	 for now just return 0
*/
int open_terminal(const uint8_t* filename) {
	return 0;
}

/* This function is to read the buffer it stops looping once we encounter a \n character
*INPUTS: filename is the filename to open
*	 for now just return 0
*/

int close_terminal(int32_t fd) {
	return 0;
}


/* This function is to read the buffer it stops looping once we encounter a \n character
*INPUTS: filename is the filename to open
*	 for now just return 0
*/
void clear_buffer(int n,unsigned char * buf1) {

	int i= 0;

	strcpy((int8_t*)buf1,(int8_t*)&buf[get_active_term()]);

	for(i=0 ; i < n ; i++)
		buf[get_active_term()][i] = ' ';

	buf_index[get_active_term()] = 0;
}



/* This function is to return the current terminal number that the user is viewing 
*OUTPUTS: terminal number (0 - 2) that user is viewing 
*	
*/
int curTerm() { return get_active_term(); }


/* This function returns the buffer index 
*OUTPUTS: pointer to the correct buff index 
*	
*/
int* buff_index() {
	return (int *)&buf_index[get_active_term()];
}

void gbg2(){
	fotp* gbg = otp_terminal; gbg = otp_dir; gbg = otp_rtc; gbg = otp_file;
}

