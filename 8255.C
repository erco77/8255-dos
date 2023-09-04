#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <dos.h>
#include <conio.h>

#undef outp		// Turbo C 3.0 needs this

#define VERSION "2.10"

//
// 8255.c - DOS 8255 DIO board monitoring program
//
// (C) Copyright Greg Ercolano 1988, 2007.
// (C) Copyright Seriss Corporation 2008, 2020.
// Available under GPL3. http://github.com/erco77/8255-dos
//
// v1.00 05/29/00 erco@3dsite.com - (yes, memorial day weekend)
// v2.00 03/15/02 erco@3dsite.com - added output control
// v2.10 09/02/23 erco@seriss.com - rewrite: ported parallel.c program to support 8255
//
// 80 //////////////////////////////////////////////////////////////////////////
//

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;

// Tone frequency (in HZ) when an input pin is logic high
#define TONE_FREQ	3000

// Pin->dir
#define OUT 1
#define IN  2
#define GND 3

// ANSI colors
#define NORMAL    "\033[0m"
#define BOLD      "\033[1m"
#define UNDERLINE "\033[4m"
#define INVERSE   "\033[7m"

// A SINGLE PORT PIN
//    It's port and mask, in|out, inverted, label, screen position..
//
typedef struct {
    int x,y;	 	// x/y position on screen
    int port;		// port offset. actual_port = (G_baseaddr+port)
    int mask;		// mask bit
    int inv;		// 1=actual hardware output is inverted
    int dir;		// IN, OUT or GND
    const char *label; 	// label for this pin
    uchar laststate;	// last state of port (to optimize redraws)
} Pin;

// GLOBALS
Pin *G_pins[24];
int G_baseaddr = -1;    // base address
int G_ctrlreg  = -1;    // control register. Data sheet says it's write only!

// CREATE AN INSTANCE OF A 'Pin' STRUCT
Pin* MakePin(int x, int y, int port, 
             int mask, int inv, int dir,
	     const char *label) {
    Pin *p = (Pin*)malloc(sizeof(Pin));
    p->x         = x;
    p->y         = y;
    p->port      = port;
    p->mask      = mask;
    p->inv       = inv;
    p->dir       = dir;
    p->label     = label;
    p->laststate = -1;
    return p;
}

// PEEK THE BYTE AT ADDRESS <segment>:<offset>
//    Used mainly to read BIOS memory at 0040:xxxx
uchar PeekByte(ushort segment, ushort offset)
{
    uchar far *addr = MK_FP(segment, offset);
    return *addr;
}

// INITIALIZE G_pins[] ARRAY
void Init(void)
{
    // Read the control port bits to determine in or out.
    uchar port_ax_io = (G_ctrlreg & 0x10) ? IN : OUT;	// port A
    uchar port_bx_io = (G_ctrlreg & 0x02) ? IN : OUT;	// port B
    uchar port_cl_io = (G_ctrlreg & 0x01) ? IN : OUT;	// port C lo
    uchar port_ch_io = (G_ctrlreg & 0x08) ? IN : OUT;	// port C hi
    int y = 2;
    int i = 0;

    //                    X   Y     PORT MASK  INV DIR  LABEL
    //                    -   -     ---- ----  --- ---  ---------
    G_pins[i++] = MakePin(5,  y++,  0,   0x01, 0,  port_ax_io, "Port A[0]");
    G_pins[i++] = MakePin(5,  y++,  0,   0x02, 0,  port_ax_io, "Port A[1]");
    G_pins[i++] = MakePin(5,  y++,  0,   0x04, 0,  port_ax_io, "Port A[2]");
    G_pins[i++] = MakePin(5,  y++,  0,   0x08, 0,  port_ax_io, "Port A[3]");
    G_pins[i++] = MakePin(5,  y++,  0,   0x10, 0,  port_ax_io, "Port A[4]");
    G_pins[i++] = MakePin(5,  y++,  0,   0x20, 0,  port_ax_io, "Port A[5]");
    G_pins[i++] = MakePin(5,  y++,  0,   0x40, 0,  port_ax_io, "Port A[6]");
    G_pins[i++] = MakePin(5,  y++,  0,   0x80, 0,  port_ax_io, "Port A[7]");

    G_pins[i++] = MakePin(5,  y++,  1,   0x01, 0,  port_bx_io, "Port B[0]");
    G_pins[i++] = MakePin(5,  y++,  1,   0x02, 0,  port_bx_io, "Port B[1]");
    G_pins[i++] = MakePin(5,  y++,  1,   0x04, 0,  port_bx_io, "Port B[2]");
    G_pins[i++] = MakePin(5,  y++,  1,   0x08, 0,  port_bx_io, "Port B[3]");
    G_pins[i++] = MakePin(5,  y++,  1,   0x10, 0,  port_bx_io, "Port B[4]");
    G_pins[i++] = MakePin(5,  y++,  1,   0x20, 0,  port_bx_io, "Port B[5]");
    G_pins[i++] = MakePin(5,  y++,  1,   0x40, 0,  port_bx_io, "Port B[6]");
    G_pins[i++] = MakePin(5,  y++,  1,   0x80, 0,  port_bx_io, "Port B[7]");

    G_pins[i++] = MakePin(5,  y++,  2,   0x01, 0,  port_cl_io, "Port C[0]");
    G_pins[i++] = MakePin(5,  y++,  2,   0x02, 0,  port_cl_io, "Port C[1]");
    G_pins[i++] = MakePin(5,  y++,  2,   0x04, 0,  port_cl_io, "Port C[2]");
    G_pins[i++] = MakePin(5,  y++,  2,   0x08, 0,  port_cl_io, "Port C[3]");

    G_pins[i++] = MakePin(5,  y++,  2,   0x10, 0,  port_ch_io, "Port C[4]");
    G_pins[i++] = MakePin(5,  y++,  2,   0x20, 0,  port_ch_io, "Port C[5]");
    G_pins[i++] = MakePin(5,  y++,  2,   0x40, 0,  port_ch_io, "Port C[6]");
    G_pins[i++] = MakePin(5,  y++,  2,   0x80, 0,  port_ch_io, "Port C[7]");
}

// SEND A TONE TO THE SPEAKER
void Tone(int onoff)
{
    static int last = 0;
    if ( onoff== 0 )  { nosound();        last = 0; return; }
    if ( last == 0 )  { sound(TONE_FREQ); last = 1; return; }
}

// SCROLL SINGLE LINE RIGHT STARTING AT x,y
void ScrollRight(int x, int y)
{
    uchar far *mono = MK_FP(0xb000, (y-1)*160+160-1-2); // (x*2));
    uchar far *cga  = MK_FP(0xb800, (y-1)*160+160-1-2); // (x*2));
    for ( ; x < 80; x++ ) {
        // ATTRIB                      // CHAR
	*(mono+2) = *mono; mono--;     *(mono+2) = *mono; mono--;
	*(cga+2)  = *cga;  cga--;      *(cga+2)  = *cga;  cga--;
    }
}

// PLOT CHAR 'c' AT POSITION x,y
void Plot(int x, int y, uchar c)
{
    uchar far *mono = MK_FP(0xb000, ((y-1)*160)+(x*2));
    uchar far *cga  = MK_FP(0xb800, ((y-1)*160)+(x*2));
    *mono = c;
    *cga  = c;
}

// BREAK (^C) HANDLER
void BreakHandler(void)
{
    signal(SIGINT, BreakHandler);
}

// SHOW HELP AND EXIT PROGRAM
void HelpAndExit(void)
{
    printf(
        "8255 - monitor/control an 8255 CIO/DIO card's 24 bit ports A,B,C\n"
	"    VERSION v%s\n"
	"    (C) Copyright Greg Ercolano 1988, 2007.\n"
	"    (C) Copyright Seriss Corporation 2008, 2023.\n"
	"    Available under GPL3. http://github.com/erco77/8255-dos\n"
        "\n"
        "USAGE\n"
	"    8255 [-h] [-q] [baseaddr] [ctrlreg]\n"
	"\n"
        "EXAMPLES\n"
	"    8255         - monitor baseaddr 300 hex (default)\n"
	"    8255 200     - monitor baseaddr 200 hex\n"
	"    8255 200 9b  - monitor baseaddr 200 hex as all inputs\n"
	"                   (9b=control register value for all inputs)\n"
	"    8255 -q      - quiet (disable tone)\n"
	"    8255 -h[elp] - this help screen\n"
	"\n"
	"KEYS\n"
	"    ESC        - quit program\n"
	"    UP/DOWN    - move edit cursor up/down\n"
	"    ENTER      - toggles state of output (when cursor on an output)\n"
	"\n"
        "    While edit cursor is on an input, speaker makes a %d HZ tone.\n",
	VERSION, TONE_FREQ);
    exit(0);
}

int main(int argc, char **argv)
{
    int i;
    int edit     = 0;	  // pin# currently being edited
    int done     = 0;	  // set to 1 when user hits ESC
    int redraw   = 1;	  // set to 1 to do a full redraw
    int quiet    = 0;     // set to 1 to disable tone
    ulong lasttime = time(NULL);

    // PARSE COMMAND LINE
    for ( i=1; i<argc; i++ ) {

	// -help?
	if ( argv[i][0] == '-' ) {
	    switch (argv[i][1]) {
	        case 'h': HelpAndExit();
		case 'q': quiet = 1; continue;
	    }
	    printf("8255: bad option '%s' (use -h for help)\n", argv[i]);
	    exit(1);
	}

	if ( G_baseaddr == -1 ) {
	    // Base address not yet specified? Parse it
	    if ( sscanf(argv[i], "%x", &G_baseaddr) != 1 ) {
		printf("8255: '%s' bad base address "
		       "(expected e.g. 0200, 0300, etc)\n", argv[1]);
		exit(1);
	    }
	    if ( G_baseaddr < 0 || G_baseaddr > 0x03ff ) {
		printf("8255: '%s' bad port# "
		       "(range 0 to 03ff hex)\n", argv[1]);
		exit(1);
	    }
	} else if ( G_ctrlreg == -1 ) {
	    // Ctrl reg value not yet specified? Parse it
	    if ( sscanf(argv[i], "%x", &G_ctrlreg) != 1 ) {
		printf("8255: '%s' bad control reg value"
		       "(expected e.g. 80, 9b, etc)\n", argv[1]);
		exit(1);
	    }
	    if ( G_ctrlreg < 0 || G_ctrlreg > 0x00ff ) {
		printf("8255: '%s' bad control register value "
		       "(range 0 to ff hex)\n", argv[1]);
		exit(1);
	    }
	}
    }

    // DEFAULTS IF NOT SPECIFIED BY USER
    if ( G_baseaddr == -1 ) {
        G_baseaddr = 0x0300;  // 300 is default
    }

    if ( G_ctrlreg  == -1 ) {
        G_ctrlreg = inp(G_baseaddr+3); // probably bad to read! (write only?)
    }

    // DISABLE ^C
    //     Prevent interrupt while sound() left on..
    //
    BreakHandler();

    // INITIALIZE G_pins[] ARRAY
    Init();

    // TOP OF SCREEN
    printf("\33[2J\33[1;1H"); // clear screen, cursor to top

    // PIN HEADING
    printf("    PIN STATE  PORT MASK SIGNAL");

    while (!done) {
        Pin *p;
        int i;
	int port;
	uchar state;
	const char *statestr;
	static int first = 1;

	for (i=0; i<24; i++) {
	    p     = G_pins[i];
	    port  = G_baseaddr + p->port;
	    state = inp(port)  & p->mask;

	    // REDRAW LINE ONLY IF STATE CHANGED
	    if ( redraw || state != p->laststate ) {
	        statestr = (p->dir==GND) ? "gnd" :
		           (state)       ? "SET" :
			   "clr";
		// HANDLE PIN COLOR
		if ( i == edit ) printf(INVERSE);
		if ( p->dir == IN) printf(BOLD);
		// SHOW PIN
		printf("\033[%d;%dH%2d   %s   %04x %c%02x  %s",
		    p->y, p->x, 
		    i+1, statestr,
		    port, (p->inv?'!':' '),
		    p->mask, p->label);
		printf(NORMAL);
		p->laststate = state;	// save state change
	    }

	    // UPDATE INPUT "OSCILLOSCOPE"
	    if ( p->dir == IN ) {
		ScrollRight(35, p->y);	                // rotate line right
		Plot(35, p->y, (state ? 0xdf : '_'));	// display state in left column
	    }

	    // TONE SOUND IF INPUT PIN UNDER EDIT CURSOR 'SET'
	    if ( (i == edit) && !quiet ) {
	        if ( p->dir == IN ) Tone(state ? 1 : 0);
		else                Tone(0);
	    }
	}

	// "OSCILLOSCOPE" SECONDS MARKERS
	{
	    ulong lt = time(NULL);
	    ScrollRight(35, 1);
	    Plot(35, 1, (lt != lasttime) ? '.' : ' ');	// c2, b3
	    lasttime = lt;
	}

	delay(25);	// not too fast

	// NO LONGER FIRST EXECUTION
	first  = 0;
	redraw = 0;

//	// KEEP CURSOR POSITIONED AT EDIT CURSOR
	printf("\033[%d;%dH", G_pins[edit]->y, G_pins[edit]->x-1);

	/* KEYBOARD HANDLER */
	if (kbhit()) {
	    uchar c = getch();
	    // printf("\033[2HKEY=%02x\n", c);
	    switch (c) {
	        case ' ':
		case 0x1b:
		case 'q':
		    done = 1;
		    break;

	        case '\r':
	        case '\n': {
		    // CHANGE VALUE OF PORT
		    p = G_pins[edit];			// pin being edited
		    if ( p->dir == IN ) break;		// early exit if input
		    port = G_baseaddr + p->port;	// port
		    state = inp(port);			// get current value
		    state ^= p->mask;			// toggle bit
		    outp(port, state);			// write modified value
		    redraw = 1;
		    break;
		}

	        case 0:
		    c = getch();
		    // printf("\033[2HKEY=%02x\n", c);
		    switch(c) {
			case 0x48:	/* UP ARROW */
			    if (--edit < 0) edit = 0;
			    redraw = 1;
			    break;
		        case 0x50: 	/* DOWN ARROW */
			    if (++edit > 23) edit = 23;
			    redraw = 1;
			    break;
		    }
		    break;
	    }
	}
    }
    nosound();	// TurboC: stop sound
    printf("\033[0m\033[25H");
    return 0;
}
