#!/bin/sh
# called on resume from /lib/systemd/system-sleep/

sudo -u alex /home/alex/.weather/update_weather.sh
killall -USR2 conky
