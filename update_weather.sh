#!/bin/sh

cd $HOME/.weather
#$HOME/.weather/fetchweather --loc=48.1222795,16.3347827 --tz="Europe/Vienna" --provider=CC >$HOME/.weather/weather.new && mv -f $HOME/.weather/weather.new $HOME/.weather/weather
$HOME/.weather/nim_fetchweather --api=VC >$HOME/.weather/weather.new && mv -f $HOME/.weather/weather.new $HOME/.weather/weather
