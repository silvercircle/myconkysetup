#!/bin/sh

cd $HOME/.weather
#$HOME/.weather/darksky-d --key=8b828fcdd7c329b4afee24fff3493dbb --windUnit=km >$HOME/.weather/weather.new && mv -f $HOME/.weather/weather.new $HOME/.weather/weather
#$HOME/.weather/darksky-r --apikey=8b828fcdd7c329b4afee24fff3493dbb --cached=no --windunit="km/h" >$HOME/.weather/weather.new && mv -f $HOME/.weather/weather.new $HOME/.weather/weather
#$HOME/.weather/climacell_fetch --apikey=cbDCjbhwvccAqa4FMoIkLrQGV381gXY5 --loc=48.1222795,16.3347827 --tz="Europe/Vienna" >$HOME/.weather/weather.new && mv -f $HOME/.weather/weather.new $HOME/.weather/weather
#$HOME/.weather/fetchweather --lat=48.1222795 --lon=16.3347827 --apikey=de5ab442f13c24614b4605aeaf8fcd6f --provider=OWM >$HOME/.weather/weather.new && mv -f $HOME/.weather/weather.new $HOME/.weather/weather
#$HOME/.weather/fetchweather --loc=48.1222795,16.3347827 --tz="Europe/Vienna" --provider=CC >$HOME/.weather/weather.new && mv -f $HOME/.weather/weather.new $HOME/.weather/weather
$HOME/.weather/nim_fetchweather --api=VC >$HOME/.weather/weather.new && mv -f $HOME/.weather/weather.new $HOME/.weather/weather
# pcmanfm-qt -w $(find $HOME/.config/variety/ | xargs file -i | grep "image/" | cut -d ":" -f1 | shuf -n1)
