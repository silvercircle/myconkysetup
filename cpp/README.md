# ClimaCell fetching app 

This is the C++ Version of the app, that fetches JSON weather data from the service API. For years, I've been using darksky for this purpose, but since it's been acquired by Apple and will be closing at the end of 2021, a new provider must be used. I decided to give [ClimaCell](https://climacell.co) a try. Unfortunately, their data format is quite a bit different from Darksky's, so some work to adjust was necessary. 

This works pretty much like the D and Rust versions of the program, the command line parameters for supplying the API Key and other data are similar.

## Acknowledgements

This is free software governed by the MIT License. It uses the following 3rd party open source libraries and/or components:

* [CLI11](https://github.com/CLIUtils/CLI11/blob/master/LICENSE) A command line parser by Henry Schreiner.
* [nlohmann:json](https://github.com/nlohmann/json) An excellent C++11 Json libray by Niels Lohmann. License: MIT.