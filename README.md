My conky setup

This is my personal conky configuration  It is largely based on the work of others, particularly [this 
remake](https://github.com/rsheasby/Conky-Lililo-2018) of a fairly popular conky setup. I changed 
quite a few things though. 

## Requirements

* A D Compiler to compile the darksky client. I recommend DMD or LDC, but both are ok. If you don't 
want to deal with D at all, there is a PHP version of the darksky data fetcher included. 

### Conky

Obviously, you need that and you need a recent version (1.10 or later) with lua and cairo support. On 
Ubuntu-based distros, this is just a matter of installing `conky-all`and you should be all set.

### Fonts

* The [OpenLogos Font](https://www.dafont.com/openlogos.font) 
* The [Style Bats Font](https://www.dafont.com/style-bats.font) 
* The [Conkyweather Font](https://github.com/altinukshini/conky_blue/blob/master/fonts/conkyweather.ttf) 
* The [Hack font](https://github.com/source-foundry/Hack), which should be available for most distros 
via package management.


## Tweaking

There are quite a few things to change, 

* In the system section you may want to comment out the individual CPU load graphs. Since I do not 
care about these, I prefer to have only one load bar.

* Also, I have commented out the 


