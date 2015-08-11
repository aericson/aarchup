#!/usr/bin/sh

# based on script from https://aur.archlinux.org/packages/batterylife
call_aarchup(){
    dbus=$(grep -z '^DBUS_SESSION_BUS_ADDRESS=' $1 | sed 's/DBUS_SESSION_BUS_ADDRESS=//')
    user=$(grep -m 1 -z '^USER=' $1 | sed 's/USER=//')
    dply=$(grep -z '^DISPLAY=' $1 | sed 's/DISPLAY=//')
    sudo -u $user sh -c "DBUS_SESSION_BUS_ADDRESS=\"$dbus\" DISPLAY=\"$dply\" /usr/bin/aarchup"   
}

wrapper(){
    called=false
    for p in $(pgrep gconf-helper); do
        call_aarchup /proc/$p/environ
        called=true
    done
    if [ $called = false ]; then
        for p in $(pgrep dconf-service); do
            call_aarchup /proc/$p/environ
            called=true
        done
    fi
    if [ $called = false ]; then
        call_aarchup /proc/self/environ
    fi
}

pacman -Sy || exit 1
wrapper || exit 1
