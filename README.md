# PDP11 Utilities

Utilities and programs related to 'simh' and PDP11 emulation

SIMH:  http://simh.trailing-edge.com/
( also https://github.com/simh/ )

The 'simh' minicomputer emulator allows you run ancient software on a
simulated ancient platform.  However, it can be very difficult to move
files between the host computer and the simulated on.  Additionally,
there are often difficulties in doing simple operations like editing,
searching, and so forth.

Given the power of modern computers, it makes sense to be able to easily
move things back and forth so that you can do all of the 'convenient'
things on the host system, then transfer files back and forth between
host and emulated minicomputer.

That's where this repository comes in, making some of these operations
easier, with scripts and host utilities, as well as native programs that
run on the emulated minicomputer.

## dectape

This specifically translates to/from the RT11 magnetic tape format. For
more information, see the 'README' file for that directory.

