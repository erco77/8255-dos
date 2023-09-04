#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>

#include "utype.h"

#undef outp		// TC 3.0 needs this

// TODO: Bring over neat stuff from parallel.c
//       Use kbhit() + getch() instead of inp(0x60), DISABLEINTS
//       Use Pin struct to keep track of last value, redraw prevention
//       Use Plot(x,y,c) instead of printf() to keep down cursor noise
//       Add use of sound(3000) if input under cursor is SET.
//       Add HelpAndExit() docs
//

#define VERSION "2.00"
#define DISABLEINTS()  outp(0x21, inp(0x21)|2)
#define ENABLEINTS()   outp(0x21, (inp(0x21)|2)^2)

/* MONITOR THE BITS ON THE 8255 I/O BOARD AT SPECIFIED PORT ADDRESS
 *    1.00 erco 05/29/00 - (yes, memorial day weekend)
 *    2.00 erco 03/15/02 - added output control
 */

/* WRITE STRING TO VIDEO MEMORY
 * Supply Y position, array of characters, array of attributes for each
 * char, and 'overwrite' to end of line flag. Used exclusivly by RUNBAR().
 */
void bar(int x, int y,		/* y location on screen */
	 char *string)		/* the string to put on screen */
{
    int t;
    uchar far *mono = MK_FP(0xb000, (y-1)*160+(x*2));
    uchar far *cga  = MK_FP(0xb800, (y-1)*160+(x*2));

    for (t=0; string[t]; t++) {
        *mono = string[t]; mono += 2;
	*cga  = string[t]; cga  += 2;
    }
}

/* SCROLL SINGLE LINE RIGHT STARTING AT x,y */
void scrollright(int x,int y)
{
    uchar far *mono = MK_FP(0xb000, (y-1)*160+160-1-2); // (x*2));
    uchar far *cga  = MK_FP(0xb800, (y-1)*160+160-1-2); // (x*2));
    for ( ; x < 79; x++ ) {
        // ATTRIB                      // CHAR
	*(mono+2) = *mono; mono--;     *(mono+2) = *mono; mono--;
	*(cga+2)  = *cga;  cga--;      *(cga+2)  = *cga;  cga--;
    }
}

/* RETURN INFO FOR A GIVEN POSITION */
void PositionInfo(int portbase,
		  int pos,
		  char *name,
		  int *port,
		  int *bitmask,
		  int *connpin)
{
    switch ( pos )
    {
        /* PORT A */
        case  0: *name='A'; *port=portbase+0; *bitmask=0x01;*connpin=37; break;
        case  1: *name='A'; *port=portbase+0; *bitmask=0x02;*connpin=36; break;
        case  2: *name='A'; *port=portbase+0; *bitmask=0x04;*connpin=35; break;
        case  3: *name='A'; *port=portbase+0; *bitmask=0x08;*connpin=34; break;
        case  4: *name='A'; *port=portbase+0; *bitmask=0x10;*connpin=33; break;
        case  5: *name='A'; *port=portbase+0; *bitmask=0x20;*connpin=32; break;
        case  6: *name='A'; *port=portbase+0; *bitmask=0x40;*connpin=31; break;
        case  7: *name='A'; *port=portbase+0; *bitmask=0x80;*connpin=30; break;

	/* PORT B */
        case  8: *name='B'; *port=portbase+1; *bitmask=0x01;*connpin=10; break;
        case  9: *name='B'; *port=portbase+1; *bitmask=0x02;*connpin= 9; break;
        case 10: *name='B'; *port=portbase+1; *bitmask=0x04;*connpin= 8; break;
        case 11: *name='B'; *port=portbase+1; *bitmask=0x08;*connpin= 7; break;
        case 12: *name='B'; *port=portbase+1; *bitmask=0x10;*connpin= 6; break;
        case 13: *name='B'; *port=portbase+1; *bitmask=0x20;*connpin= 5; break;
        case 14: *name='B'; *port=portbase+1; *bitmask=0x40;*connpin= 4; break;
        case 15: *name='B'; *port=portbase+1; *bitmask=0x80;*connpin= 3; break;

	/* PORT C */
        case 16: *name='C'; *port=portbase+2; *bitmask=0x01;*connpin=29; break;
        case 17: *name='C'; *port=portbase+2; *bitmask=0x02;*connpin=28; break;
        case 18: *name='C'; *port=portbase+2; *bitmask=0x04;*connpin=27; break;
        case 19: *name='C'; *port=portbase+2; *bitmask=0x08;*connpin=26; break;
        case 20: *name='C'; *port=portbase+2; *bitmask=0x10;*connpin=25; break;
        case 21: *name='C'; *port=portbase+2; *bitmask=0x20;*connpin=24; break;
        case 22: *name='C'; *port=portbase+2; *bitmask=0x40;*connpin=23; break;
        case 23: *name='C'; *port=portbase+2; *bitmask=0x80;*connpin=22; break;
    }
}

void UpdateLine(int portbase,
		int pos,		// 0..24
		int cursor_pos)
{
    int port,
        bitmask,
        onoff,
        connpin;
    char name[2] = "X";

    /* GET POSITION INFO */
    PositionInfo(portbase, pos, name, &port, &bitmask, &connpin);
    onoff = inp(port) & bitmask;

    /* SEEK POSITION ON SCREEN */
    printf("\033[%d;1H\033[0m\r", pos+2);

    /* HANDLE PRINTING VERTICAL BRACKETS */
         if ( bitmask == 0x01 ) printf("  %c%c ", 0xda, 0xc4);
    else if ( bitmask == 0x10 ) printf("%c %c  ", name[0], 0xb3); 
    else if ( bitmask == 0x80 ) printf("  %c%c ", 0xc0, 0xc4);
    else                        printf("  %c  ", 0xb3);

    /* SCROLL PREVIOUS BITSTATE RIGHT ONE CHAR */
    scrollright(26, pos+2);

    /* PRINT DATA AT PIN */
    {
	char *color;
	     if ( cursor_pos == pos ) { color = "\033[7m"; }
	else if ( pos & 0x01 )        { color = "\033[1m"; }
	else                          { color = "\033[0m"; }

	printf("%s%02d  0x%04x 0x%02x - %s\033[0m %c",
	   color, connpin, port, bitmask,
	   (onoff) ? "SET" : "clr",
	   (onoff) ? '-' : '_');
    }
}

void InitScreen(int portbase, int cursor_pos)
{
    int t;

    printf("\033[2J\033[1H\r     Pin Port   Mask   State   (PORTBASE=0x%04x)",
        portbase);

    for ( t=0; t<24; t++ )
	UpdateLine(portbase, t, cursor_pos);
}

int main(int argc, char **argv)
{
    int cursor_pos = 0;		/* 0 thru 23 */
    int portbase = 0x0300;	/* whatever the 8255 port address is */
    int done = 0;
    int t;

    /* PARSE COMMAND LINE */
    if ( argc >= 2 ) {
	if ( argv[1][0] == '-' || 
	     sscanf(argv[1], "%x", &portbase) != 1 ) {
	    fprintf(stderr,"8255 - monitor 3 ports on an 8255 I/O board\n");
	    fprintf(stderr,"VERSION %s\n", VERSION);
	    fprintf(stderr,"Greg Ercolano 05/29/00. Public domain software.\n"
	                   "\n"
	                   "  usage: 8255 [port]\n"
	                   "example: 8255 0300   # monitor ports 0x300-302\n"
	                   "\n"
	                   "If [port] unspecified, default port is 0x300\n"
	                   "\n"
	                   "KEYS\n"
	                   "    ESC to quit\n"
	                   "    Up/Down arrow keys to move to different pin\n"
	                   "    Space or Enter: Toggle output under cursor.\n"
	                   "\n");
	    exit(1);
	}
    }

    /* SETUP INITIAL SCREEN LAYOUT */
    InitScreen(portbase, cursor_pos);

    /* DISABLE INTERRUPTS SO WE CAN READ THE KEYBOARD */
    // intflags = inp(0x21);
    DISABLEINTS();

    while (inp(0x60)<0x80) 	/* wait until key released */
	{ }

    /* UPDATE LOOP */
    while ( ! done ) {
        int key = 0;

	/* KEY HIT? */
	if ( ( key = inp(0x60) ) < 0x80 ) {
	    while (inp(0x60)<0x80) 	/* wait until key released */
		{ }
	    /* printf("\033[1;70H--%02x--", key); */
	    switch ( key ) {

		case 0x01:	/*ESC: QUIT*/
		    done = 1;
		    continue;

		case 0x48: 	/* UP */
		    if ( --cursor_pos < 0 ) cursor_pos = 23;
		    break;

		case 0x50:	/* DOWN */
		    if ( ++cursor_pos > 23 ) cursor_pos = 0;
		    break;

		case 0x39:	/* SPACE */
		case 0x1c:	/* ENTER */
		{
		    /* TOGGLE THE OUTPUT BIT CURSOR IS ON */
		    char name[2] = "x";
		    int port, bitmask, connpin, byte;
		    PositionInfo(portbase, cursor_pos, 
		                 name, &port, &bitmask, &connpin);

		    byte = inp(port);	/* read byte */
		    byte ^= bitmask;	/* toggle bit */
		    outp(port, byte);	/* write byte */
		    break;
		}

		default:
		    break;
	    }
	}

	for ( t=0; t<24; t++ )
	    UpdateLine(portbase, t, cursor_pos);

	delay(25);	// not too fast
    }

    ENABLEINTS();
    printf("\033[24H\n");
    return 0;
}
