#!/bin/bash
if [ -z "$(pgrep 'aarchup$')" ];then
	/usr/bin/aarchup --loop-time 60 --icon /usr/share/aarchup/archlogo.png &
else
    kill -9 $(pgrep 'aarchup$')
    /usr/bin/aarchup --loop-time 60 --icon /usr/share/aarchup/archlogo.png &
fi
