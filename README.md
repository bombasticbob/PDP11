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

## boot.bat

This is a simple simh startup script that shows how to boot up a PDP11
system using an RT11 5.3 image.  These images are available online for
personal use at various places, including these:

  <a href="http://simh.trailing-edge.com/kits/rtv53swre.tar.Z">rtv53swre.tar.Z</a><br>

  ftp://minnie.tuhs.org/pub/PDP-11/Sims/Supnik_2.3/software/rtv53swre.tar.gz<br>

  (make sure you read the license file - I'm supposed to mention that)

## attach_ptp.bat, attach_ptr.bat

These are sample simh script files that you can run from the simh console
to attach a file as the 'paper tape reader' (ptr) or the 'paper tape
punch'. These scripts make it convenient to transfer text or single files
to and from the PDP11 environment, by using a disk file on the host as
input or output to/from the paper tape reader+punch device.

## printer.bat

This is a sample simh script that attaches a printer to a host disk file.
It will basically let you access the line printer from the PDP11
environment by writing output to the disk file.  Keep in mind that the
simh program doesn't immediately flush the buffer, so to get the output
you will need to detach the file from the simh console.


## additional information

Additional information (such as PDP11 documentation) can be found at:

  <a href="http://simh.trailing-edge.com/pdf/all_docs.html">
    http://simh.trailing-edge.com/pdf/all_docs.html</a><br>

  <a href="http://bitsavers.trailing-edge.com/pdf/dec/pdp11/rt11/v5.6_Aug91/">
    http://bitsavers.trailing-edge.com/pdf/dec/pdp11/rt11/v5.6_Aug91/</a><br>


