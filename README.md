# 8255-dos
![screenshot](https://github.com/erco77/8255-dos/blob/main/8255-screenshot.jpg)
*This shows the main 8255 text-based interface screen, in this case showing as inputs for Ports A,B and C.*

Builds in Turbo C 3.0 on DOS machines up to and including Windows 98; see the Makefile. Runs in DOS mode only.

This program monitors and controls the bits on any 8255 based I/O board that appears as 4 ports at a specified base address. This is useful for debugging I/O on 8255-based CIO-DIO24 and CIO-DIO48 ISA boards.

To run this program, just invoke '8255' from the DOS prompt. The default will be to monitor an 8255 I/O chip at IBM PC port 0300 (hex).
You can specify different base addresses as an optional argument on the command line, as often 8255 based boards have DIP switch settings to configure the device appear at different base addresses.
Use '8255 -help' to see the program's help screen with a list of the optional arguments (screenshot below).

The program can monitor 24 bits at a time, so it can monitor/control all 24 bits on a CIO-DIO24 board.

For 48 channel boards like the CIO/DIO48, this program can only look at 24 bits at a time. So if the board appears at base address 0300, you can control/monitor the first 24 bits by invoking '8255 0300', and the second 24 bits using '8255 0304', as the second 8255 chip's ports usually appears starting at base address + 4.

Lines shown in BOLD are "input" signals. The rest are "outputs". Which is which is determined by reading the 8255's Control Register (at base address + 3). Some 8255 chips don't allow reading the Control Register, so for those situations you can specify the Control Register byte as an optional byte argument (in hex) on the command line, after the base address. e.g. '8255.exe 0300 9b' to specify base address 0300 (hex), Control Register value 9b (hex). Other Control Register values can be used for programming different input/output combinations for the A,B and C ports.

The STATE column monitors the realtime input and output bit states on the port:

- 'SET' if the bit is set
- 'clr' if the bit is clear.

You can use the UP/DOWN arrow keys to move the inverse "cursor" (the white block) to focus on a particular I/O pins:

- When sitting on an input (bold), the speaker audibly 'beeps' like a multimeter whenever the input is SET.
- When sitting on an output (normal), hit the ENTER key to toggle the state of that pin's output.

Any 8255 ports programmed as inputs will include a scrolling 'oscilloscope' that runs along the right edge of the screen next to that pin, showing the set/clear state. The scrolling aspect allows one to more easily oscillations on the input. The little dots that scroll by on the top line are 1 second timing markers.

![Help screen](https://github.com/erco77/8255-dos/blob/main/8255-help-screen.jpg)

Hit 'Esc' or 'q' to exit the program and return to DOS.
