#!/bin/bash
if [ -z "$(pgrep 'aarchup$')" ];then
	/usr/bin/aarchup --loop-time 60 --icon /usr/share/aarchup/archlogo.svg &
fi
