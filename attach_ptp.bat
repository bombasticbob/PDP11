echo
echo This simv script attaches a device to 'ptp' which is the
echo paper tape punch.  Useful for capturing a file or text
echo output to a host disk file.
echo
echo Call this function using "do attach_ptp.bat filename"
echo
echo attaching '%1' to ptr for I/O
echo
echo enter '"pip PC:=%1"' within PDP11 env to copy from the PDP
echo
detach ptp
attach ptp %1
echo
echo Use 'detach ptp' to close the file and commit the buffers
echo (otherwise the file won't write completely)
echo
cont

