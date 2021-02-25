# ClimaCell fetching app 
                
** this is work in progress **

This is the C++ Version of the app, that fetches JSON weather data from the service API. For years, I've been using darksky for this purpose, but since it's been acquired by Apple and will be closing at the end of 2021, a new provider must be used. I decided to give [ClimaCell](https://climacell.co) a try. Unfortunately, their data format is quite a bit different from Darksky's, so some work to adjust was necessary. 

This works pretty much like the D and Rust versions of the program, the command line parameters for supplying the API Key and other data are similar.

## Built instructions

Basically, everything is included. You follow the usual CMake process. A C++14 compiler is required, most modern versions of GCC, Clang or MSVC should work. Except for GLib 2 and pthreads, no external libraries are required to build.

Cmake 3.17 or later is required, the default configuration uses precompiled headers.

## Acknowledgements

This is free software governed by the MIT License. It uses the following 3rd party open source libraries and/or components:

* [CLI11](https://github.com/CLIUtils/CLI11/blob/master/LICENSE) A command line parser by Henry Schreiner.
* [nlohmann:json](https://github.com/nlohmann/json) An excellent C++11 Json library by Niels Lohmann. License: MIT.
* [Loguru](https://github.com/emilk/loguru) A lightweight C++ logging facility by Emil Ernerfeldt. License: Public Domain
