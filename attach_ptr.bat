echo
echo This simv script attaches a device to 'ptr' which is the
echo paper tape reader.  Useful for loading a file or text
echo input from a host disk file.
echo
echo Call this function using "do attach_ptr.bat filename"
echo
echo attaching '%1' to ptr for I/O
echo
echo enter '"pip %1=PC:"' within PDP11 env to copy to the PDP
echo
detach ptr
! if test -n "%1" ; then if test -e "%1" ; then crlf -m %1 ; fi ; fi
attach ptr %1
echo
echo Use 'detach ptr' to close the input file
echo
cont

