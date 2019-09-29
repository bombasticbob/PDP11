****************************************************************
**                                                            **
** DECTAPE - Copyright (c) 2019 by S.F.T. Inc.                **
**   github:  github.com/bombasticbob/PDP11                   **
**                                                            **
** DISCLAIMER:                                                **
** No warranty whatsoever, either implied or specific.        **
** You may use the 'dectape' application at your own risk, in **
** accordance with the copyright and license.                 **
**                                                            **
** BSD-LIKE LICENSE:                                          **
** This program may be used, copied, modified, or distributed **
** in any form so long as this copyright notice and the dis-  **
** claimer are included along with this documentation file.   **
**                                                            **
** You may also use, modify, and/or distribute the 'dectape'  **
** program using a GPLv2 (or later) license.                  **
**                                                            **
****************************************************************

=======
SUMMARY
=======

The 'dectape' program is designed to read a file that was generated using
the 'simh' or 'simhv' application (http://simh.trailing-edge.com) while
emulating a PDP11, and using an operating system such as 'RT11'.

The format on the tape is the standard OS format, unlike what you would
generate using the 'BUP' utility.  However, this program allows you to
create a tape header on an empty tape using 'BUP'.  More on that later.

The 'simh' (or 'simhv') application lets you attach a disk file to an
emulated peripheral device, such that the data is stored or read from
the disk file as if it were the peripheral.  In this case we're using the
'TM0' tape device, emulated as 'MT0' in the RT11 OS.

You can then use the 'PIP' program to copy files onto the tape device
'MT0' and read them using 'dectape', listing the directory contents,
validating the tape, and/or copying the files into a directory on the
host computer (running Linux or BSD or a POSIX compatible OS like OSX).

Additionally, you can create a new tape file from a directory, writing
all of the files from the directory onto the tape.  Then you can attach
the file to the TM0 device in 'simh' or 'simhv' and read from it via
the 'MT0' device with the 'PIP' program within the 'RT11' OS environment.

NOTE:  this was tested with RT11 V05.03 and may not work with others.
       You are welcome (and encouraged) to submit issues and/or patches
       for consideration via the github site.


=========================
HISTORY AND USAGE OF RT11
=========================

NOTE:  This section is entirely for newbies.  You can read it anyway if
you want but if you consider yourself a guru on ancient computers, you
may not need to read this and you can skip to the next section.


RT11 was a run-time system developed by Digital Equipment Corporation for
their PDP11 series (hence the '11').  I forget what the '11' means, but
the suffix of the computer model (such as PDP11/73) is often related to
something significant, like the year it was first built (or something).

The PDP11 computer may still be found operating things like nuclear power
plants and industrial systems, as it was intended to (essentially) be
a device controller.  The 'UNIBUS' (later QBUS) backplane was well
documented and there were driver boards designed for use by it for
various things that are probably STILL running today.

As such, the PDP11 still has relevance.

RT11 is the RTOS which has some interesting capabilities, such as both
foreground and background capabilities, virtual memory management, hardware
floating point, overlays (swap out some procs, swap in different ones,
on demand, as needed) and other things that you'd normally only find in a
mainframe system at that time.  As such it was really useful for schools
that teach programming, which is where I learned to use it.

The 'RT' means 'Run Time' and as such it is an RTOS by the classic
definition of one.  In short, it's NOT a time-share system.  It's a
dedicated system with foreground/background capability, so if you're
controlling a device, or doing punch card programs, it's perfect (well,
perfect at the time, being relatively low cost and so on).

For time sharing there was RSTS/E, which also ran on the PDP11, and later,
UNIX, which I'm sure you've heard of by now.

FYI do you remember 'Y2K' ?  That problem applies to the PDP11 operating
systems.  Some available OS images have patches for Y2k, but not all,
and it is my opinion that they may not work as advertised (or at all).
This means there's a bit of room to create patches for the OS for your
own use, in which you fix it.


----------------------
BASIC SIMH/SIMHV USAGE
----------------------

To start a 'simv' or 'simhv' session with RT11, you'll need an RT11 image.
These are available in several locations, and have various licenses
associated with them.  Generally you can use it for personall reasons,
experimentation, and so on.

RT11 (and other) images:

  http://simh.trailing-edge.com/software.html

A link on this page:

  http://simh.trailing-edge.com/kits/rtv53swre.tar.Z

and also, this version

  ftp://minnie.tuhs.org/pub/PDP-11/Sims/Supnik_2.3/software/rtv53swre.tar.gz

(don't forget to read the license agreement, which is in the tarball)

Once you have a disk image file (let's say it's Disks/rtv53_rl.dsk) then
you can run simhv and boot it up, similar to the following:

  bash prompt> pdp11
  PDP-11 simulator V3.9-0
  sim>attach rl0 Disks/rtv53_rl.dsk
  sim>boot rl0

At this oint you'll be running the emulated PDP11.  From here you can exit
back to the simhv by pressing CTRL+E.

While in the emulator, to mount a magnetic tape file, you can use the
attach command as follows:

  sim>attach tm0 tapefile.bin

then to go back to the PDP11 again, enter the 'cont' command

  sim>cont

you'll THEN be able to use the tape file as 'MT0'

Initializing a new tape with a header:

  .R DUP
  *MT0:/Z/Y
  *^C    <-- that's you pressing the CTRL+C key

Example of backing up everything onto a tape from the system drive:

  .R PIP
  *MT0:*.*/M:0=*.*/Y
  *^C    <-- that's you pressing the CTRL+C key again - wait for '*'

OK when you first try and do something with a magnetic tape, you will
need to pre-extend the file to an appropriate length.  The 'TM0' device
does not have a fixed size, but I like to use 32MB.  Here's how I make it
(from the bash prompt):

  bash prompt> dd if=/dev/zero of=tapefile.bin bs=512 count=65536

This will create a 32Mb file (64k 512-byte blocks) filled with zeros on
a POSIX-compatible system (like Linux or FreeBSD).  You can then attach
it to the 'TM0' device and use it as if it were a tape.

Additionally, the 'dectape' program can create and initialize a tape file
for you, from the contents of the directory.  For more on that, read on.

When you have mounted a tape, you can use the PIP program to one or more
files to a disk drive.  Example, copy the file "YOU.TXT" from the tape onto
the system disk:

  .R PIP
  *SY:YOU.TXT=MT0:YOU.TXT/M:0
  *^C  --> the CTRL+C again

The /M:0 tells PIP to rewind the tape first.  Back in the day, this sort of
thing was important.

Keep in mind taht the line ending for RT11 is <CR><LF> and the line ending
for POSIX systems is <LF>.  There are utilities that you can use to convert
between them, such as my 'crlf' utility:

  https://github.com/bombasticbob/crlf

--------------------------------------------
(end of 'HISTORY AND USAGE OF RT11' section)
--------------------------------------------



===============================
USING THE 'dectape' APPLICATION
===============================

DIRECTORY OF A TAPE FILE
------------------------

To list the directory of a tapefile, run 'dectape' with the tape file
name as the first ane only parameter, i.e.

  dectape tapefile.bin

If you only want to validate teh tape file, add '-V' to the command line:

  dectape -V tapefile.bin


To write the contents of a tape to a directory, specify the directory
name as the 2nd parameter on the command line.  If it does not exist, it
will created:

  dectape tapefile.bin dirname

The files from 'tapefile.bin' will be written to 'dirname'.

NOTE:  if 'dirname' exists, but is NOT a directory, the command will fail.

If 'dirname' exists, and contains files, matching file names will NOT be
automatically overwritten.  Three switches control the overwrite behavior:

(no switch)  Prompt for overwrite unless '-q' is present.
  -n         Do not overwrite any files (same as '-q' without '-o')
  -o         Overwrite files, but prompt first if '-q' present
  -q         Do not prompt.  If '-o' was specified, overwrite silently
             Otherwise, do not overwrite any files.

To write the contents of a directory to a (optionally new) tape file,
specify the directory name as the first parameter, and the tape file
name as the second.

NOTE:  if the output file exists as a directory, the command will fail.




=======================================
ADDITIONAL PDP11 and RT11 DOCUMENTATION
=======================================

There are other tape operations that you could do if you want to read up on
it.  The manuals can be found here:

  http://simh.trailing-edge.com/pdf/all_docs.html
  http://bitsavers.trailing-edge.com/pdf/dec/pdp11/
  http://bitsavers.trailing-edge.com/pdf/dec/pdp11/rt11/v5.6_Aug91/
  (these are the 5.6 manuals but they have worked just fine for me)

I recommend starting with the 1991 manuals, and only download the older ones
when you cannot find a newer one to tell you what you need to know.  The
newer ones were apparently done with OCR or similar and can be searched
properly.  The older ones are all a set of photocopies in PDF form.  They
are useful but not as useful as the ones that are searchable (and physically
smaller) because they contain the actual text of the document and not just
a set of images.

'PIP' and 'DUP' and other system utilities are documented in the 'System
Utilities Manual'.  The 1991 documentation is in 2 parts; both are available
at that link.  You will most likely want to look at the RT11 Commands Manual
as well.  These books are VERBOSE but contain a lot of information, and were
apparently written for people without a whole lot of computer skills, so that
they can learn how to use cryptic utilities on a primitive OS.


Copyrights and Trademarks:
  PDP11 and RT11 are trademarks of Digital Equipment Corporation
  UNIX® is a registered trademark of The Open Group


