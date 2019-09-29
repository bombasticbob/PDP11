echo
echo This is a sample bootup that loads the RT11 OS from Disks/rtv53_rl.dsk
echo You can invoke it using a command line similar to:  pdp11 boot.bat
echo
echo attaching "Disks/rtv53_rl.dskz'
echo
attach rl0 Disks/rtv53_rl.dsk
echo
echo booting up, use ctrl+E to bail
echo
echo ************************************************************************
boot rl0

