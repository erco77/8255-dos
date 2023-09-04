##
## 8255.C - MAKEFILE FOR TURBO C 3.0
##
# TCC 3.0 Compiler flags
# ----------------------
#     -ml - large memory model      -f - floating point emulation
#     -K  - assume char unsigned    -w - enable warnings
#     -1  - generate 186/286 inst.  -G - generate for /speed/
#     -j8 - stop after 8 errors
#
CC       = tcc
CFLAGS	 = -ml -f -w -1 -G -j8 -K
LFLAGS   =

# Default target
default: 8255.exe

###
### 8255 program - Interactive manipulation of 8255 based I/O boards
###
parallel: 8255.exe

8255.obj: 8255.c
	$(CC) $(CFLAGS) -c 8255.c

8255.exe: 8255.obj
	$(CC) $(CFLAGS) -e8255.exe 8255.obj

clean:
	-del *.exe > nul
	-del *.obj > nul

