#!/bin/sh

cd $HOME/.weather
$HOME/.weather/darksky.php >$HOME/.weather/weather.new
mv -f $HOME/.weather/weather.new $HOME/.weather/weather
pcmanfm-qt -w $(find $HOME/.config/variety/ | xargs file -i | grep "image/" | cut -d ":" -f1 | shuf -n1)