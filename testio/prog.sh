#!/bin/bash

NIXDEVNAME=`ls /dev/disk/by-id | grep INTRLGX | tr -d [:space:]`
if [ -z $NIXDEVNAME ]; then 
	echo LPC2148 not found.; 
	echo Please make sure the device is connected.; 
	echo You can force the LPC2148 into bootloader mode by; 
	echo shorting the DBG_E jumper and hitting the RESET button.; 
	exit; 
	fi
NIXDEVPATH=`readlink -m /dev/disk/by-id/$NIXDEVNAME`
sudo umount $NIXDEVPATH
echo -n "Device located at .."
echo $NIXDEVPATH
echo -e "Programming Device ...\n"
echo ab > .program_begin
echo ba > .program_end
sudo dd bs=512 seek=3 if=.program_begin of=$NIXDEVPATH
sudo dd bs=512 seek=4 if=iotest.bin of=$NIXDEVPATH
sudo dd bs=512 seek=3 if=.program_end of=$NIXDEVPATH
echo 'Done.'
