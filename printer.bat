echo
echo This is a sample simh script to attach a printer.  Keep in mind that
echo printer output doesn't flush until you detach it.
echo
echo You can execute this as 'do printer.bat filename' from simvh console
echo
detach LPT
attach LPT %1
echo
echo Don't forget to detach LPT to flush the buffers
echo

