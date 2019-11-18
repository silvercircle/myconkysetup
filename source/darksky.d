/++
 + darksky-d fetches weather data from darksky.net and builds 
 + a plain text file which conky will read to construct a nice weather 
 + widget. 
 + + 
 + © 2019, Alex Vie <silvercircle@gmail.com>
 + License: MIT 
 +  
 + How to build: 
 +  
 + install DMD (D Compiler) from http://dlang.org 
 +  
 + > dub build --build=release --compiler=dmd --arch=x86_64 
 +  
 + replace release with debug to get a debug build, arch with x86 for 
 + a 32bit build. 
 +  
 + IMPORTANT NOTE: 
 + On 32bit Linux (at least, debian-based distributions) make sure to link 
 + with ld.gold or else you'll run into troubles including crashes.
 +/
 
module main;

import std.stdio, std.getopt, core.stdc.stdlib: exit;
import std.net.curl, std.path, std.file, std.conv;
import std.datetime, std.string;
import vibe.data.json;

import context, database;

struct Config {
	string  key = "";               // darksky api key. Mandatory, the application
									// will not save it anywhere.
	bool    fUseCached = false;     // use cached Json (for testing mainly, to reducee 
									// API calls.
	string  tempUnit = "C";         // C or F
	string  windUnit = "ms";        // ms, km, knots or mph
	string  visUnit = "km";         // km or miles
	string  pressureUnit = "hpa";   // hpa or inhg
	string  windSpeed;              // temporary save wind unit 
	bool    noHistory = false;      // don't record fetched data in database
}

Config cfg = Config();

// the wind directions
string[] _windDirections = ["N","NNE","NE","ENE","E","ESE", "SE", "SSE","S","SSW","SW","WSW","W","WNW","NW","NNW"];

// translate textual condition strings into icon codes
string[string] daytimeCodes, nighttimeCodes;

/++
 + this is a module constructor, it runs before any other code in this module 
 + is executed. 
 +  
 + shared means, it runs only once (in the main thread) 
 +/
shared static this()
{
	/+ 
	 + these arrays translate condition strings to a single letter icon code which
	 + directly corresponds to the character in the weather symbol font
	 +
	 + Nighttime icons are different, so we need 2 arrays.
	 +/
	daytimeCodes = [ "clear-day": "a", "clear-night": "a", "rain": "g",
		"snow": "o", "sleet": "x", "wind": "9", "fog": "7", "cloudy": "e",
		"partly-cloudy-day": "c", "partly-cloudy-night": "C"];


	nighttimeCodes = [ "clear-day": "A", "clear-night": "A", "rain": "G",
		"snow": "O", "sleet": "x", "wind": "9", "fog": "7", "cloudy": "f",
		"partly-cloudy-day": "C", "partly-cloudy-night": "C"];
}

/++
 + print usage
 +/
void print_usage()
{  
	writeln("\n    darksky-d   Version 0.1 USAGE:");
	writef(q"[
	darksky-d   --key=YOUR_API_KEY --useCached=true|false --loc=lat,lon
				--tempUnit=C|F --windUnit=ms|km|knots|mph --visUnit = km|miles
				--pressureUnit=hpa|inhg --noHistory

	key:          Your darksky API key. Mandatory.
	useCached:    When true, use cached JSON. If none is found, print a notice 
				  and exit. Otherwise fetch fresh data from the darksky API.
	loc:          Your location, see darksky API docs. Must be in the format 
				  latitude,longitude.
				  Example: --loc=48.2082,16.3738. This will be saved in the
				  config until you specify different location.
	tempUnit:     C or F, default is C
	windUnit:     Unit for the wind speed. ms (m/s), km (km/h) knots, or 
				  mph are available. Defaults to m/s
	visUnit:      Unit for visibility. Either km or miles, defaults to km.
	pressureUnit: Unit for pressure, either hpa or inhg. Defaults to hpa.
	noHistory:    Set to true if you don't want the fetched data be written
				  to the database.

	Output will go to stdout to verify basic functionality.
	(C) 2019 by silvercircle@gmail.com
	License: MIT
]");
}

/++
 + take a wind bearing in degrees and return a symbolic wind direction like NNW
 +/

string degToBearing(uint wind_direction)
{
	if(wind_direction < 0 || wind_direction > 360) {
		wind_direction = 0;
	}
	const int val = cast(int)((wind_direction / 22.5) + .5);
	return _windDirections[val % 16];
}
@safe nothrow unittest
{
	assert(degToBearing(45) == "NE");
	assert(degToBearing(400) == "N");
}
/++
 + returns 0 on success, any other value means failure and a possibly incomplete
 + output
 +/
void main(string[] args)
{
	GlobalContext ctx = GlobalContext.getInstance(args);
	Json returnError(uint code = 0)
	{
		return Json(["success": Json("FAIL"), "code": Json(code)]);
	}

	Json readFromCache(ref int result)
	{
		try {
			std.file.isFile(ctx.cachefile);
			string rawJson = std.file.readText(ctx.cachefile);
			Json j = parseJson(rawJson);
			result = 0;
			return j;
		} catch(FileException e) {
			writeln("Error loading json data from cachefile.\nPlease run once without --useCached.");
			result = -2;
			return returnError(-2);
		}
	}
	void fetchFromApi()
	{
		const string url = "https://api.darksky.net/forecast/" ~ cfg.key ~ "/" ~ ctx.cfg.location ~ "?units=si";
		ctx.apiJson = std.net.curl.get(url);
		try {
			File f = File(ctx.cachefile, "w");
			f.write(ctx.apiJson);
			f.close();
		} catch (FileException e) {
			writeln("Error saving json response to cache\n", e);
			ctx.orderlyShutDown(-2);
		}
		ctx.cfg.lastFetched = Clock.currTime;
		ctx.cfg.numUpdates++;
	}

	const GetoptResult stdargs = getopt(args, "key", &cfg.key, "useCached", &cfg.fUseCached,
										"tempUnit", &cfg.tempUnit, "windUnit", &cfg.windUnit,
										"visUnit", &cfg.visUnit, "pressureUnit", &cfg.pressureUnit,
										"noHistory", &cfg.noHistory);
	if(stdargs.helpWanted) {
		print_usage();
		ctx.orderlyShutDown(-1);
	}
	if(cfg.key.length < 10) {
		writeln("You did not specify a valid API KEY.\nThis is mandatory, use --key=YOUR_KEY to fetch data from darksky.net.");
		ctx.orderlyShutDown(-3);
	}

	if(cfg.fUseCached == false) {
		DB db = DB.getInstance();

		int resultcode = 0;
		fetchFromApi();
		Json result = readFromCache(resultcode);
		try {
			File f = File(ctx.prettycachefile, "w");
			f.write(result.toPrettyString);
			f.close();
		} catch (FileException e) {
			writeln("Error saving PRETTY json response to cache\n", e);
			ctx.orderlyShutDown(-2);
		}
		if(resultcode != 0) {
			ctx.orderlyShutDown(resultcode);
		}
		generateOutput(result);
		if(cfg.noHistory == false) {
			db.addDataPoint(result);
		}
		ctx.orderlyShutDown(0);
	} else {
		int resultcode = 0;
		Json result = readFromCache(resultcode);
		if(resultcode != 0) {
			ctx.orderlyShutDown(resultcode);
		}
		generateOutput(result);
		ctx.orderlyShutDown(0);
	}
	ctx.orderlyShutDown(-1);
}

void generateOutput(Json result)
{
	// C to F when --tempUnit=F was specified
	T convertTemperature(T)(const T temp) const
	{
		return cfg.tempUnit == "F" ? ((temp * 9/5) + 32) : temp;
		/*
		if(cfg.tempUnit == "F") {
			return (temp * 9/5) + 32;
		} else {
			return temp;
		}
		*/
	}

	// km > miles
	float convertVis(const float vis)
	{
		return cfg.visUnit == "miles" ? vis / 1.609 : vis;
	}

	// windspeed, API always returns m/s, we convert to km/h, mph or knots
	// on user's request.
	float convertWindspeed(float speed) const
	{
		switch(cfg.windUnit) {
			case "km":
				cfg.windSpeed = "km/h".dup;
				return speed * 3.6;
			case "mph":
				cfg.windSpeed = "mph".dup;
				return speed * 2.237;
			case "knots":
				cfg.windSpeed = "knots".dup;
				return speed * 1.944;
			default:
				cfg.windSpeed = "m/s".dup;
				return speed;
			}
	}

	// hPa > InHg
	float convertPressure(float hPa)
	{
		return cfg.pressureUnit == "inhg" ? hPa / 33.863886666667 : hPa;
	}

	// output a single temperature value, assume float, but
	// the Json can also return int.
	void outputTemperature(const Json val, const bool addUnit = false, const string format = "%.1f%s\n")
	{
		writef(format, convertTemperature(getFloatValue(val)), addUnit ? "°" ~ cfg.tempUnit : "");
	}

	/++
	 + output low/high temperature and condition "icon" for one day in the
	 + forecast
	 +/
	void outputForecast(Json day)
	{
		if(day["icon"].get!string in daytimeCodes) {
			writeln(daytimeCodes[day["icon"].get!string]);
		} else {
			writeln("a");
		}
		outputTemperature(day["apparentTemperatureLow"], false, "%.0f%s\n");
		outputTemperature(day["apparentTemperatureHigh"], false, "%.0f%s\n");
		const SysTime t = SysTime.fromUnixTime(day["time"].get!int);
		writeln(capitalize(cast(string)t.dayOfWeek.to!string));
	}

	writeln("** begin data **");
	Json currently = result["currently"];
	const SysTime timestamp = SysTime.fromUnixTime(currently["time"].get!int);

	SysTime sunrise = SysTime.fromUnixTime(result["daily"]["data"][0]["sunriseTime"].get!int);
	SysTime sunset = SysTime.fromUnixTime(result["daily"]["data"][0]["sunsetTime"].get!int);

	const SysTime now = Clock.currTime;

	// determine icon type (day or night). 
	if (now > sunrise && now < sunset) {
		if(currently["icon"].get!string in daytimeCodes) {
			writeln(daytimeCodes[currently["icon"].get!string]);
		} else {
			writeln("a");
		}
	// otherwise, it's night. Show the night icon.
	} else {
		if(currently["icon"].get!string in nighttimeCodes) {
			writeln(nighttimeCodes[currently["icon"].get!string]);
		} else {
			writeln("a");
		}
	}
	
	outputTemperature(currently["apparentTemperature"], true);
	// 3 days of forecast
	outputForecast(result["daily"]["data"][1]);
	outputForecast(result["daily"]["data"][2]);
	outputForecast(result["daily"]["data"][3]);
																		// these comments are the
																		// line numbers in the weather
																		// file.
	outputTemperature(currently["temperature"], true);                  // 16
	outputTemperature(currently["dewPoint"], true);                     // 17
	writef("Humidity: %d\n", cast(int)(currently["humidity"].getFloatValue() * 100));  // 18
	writef(cfg.pressureUnit == "hpa" ? "%.1f hPa\n" : "%.2f InHg\n", convertPressure(getFloatValue(currently["pressure"])));
	writef("%.1f %s\n", convertWindspeed(getFloatValue(currently["windSpeed"])), cfg.windSpeed); // 20
	writef("UV: %d\n", currently["uvIndex"].get!int);                   // 21
	writef("%.1f %s\n", convertVis(getFloatValue(currently["visibility"])), cfg.visUnit); // 22

	writef("%02d:%02d\n", sunrise.hour, sunrise.minute);                // 23
	writef("%02d:%02d\n", sunset.hour, sunset.minute);                  // 24

	writeln((currently["windBearing"].get!int).degToBearing());         // 25

	SysTime time = SysTime.fromUnixTime(currently["time"].get!int);  
	writef("%02d:%02d\n", time.hour, time.minute);                      // 26
	writeln(currently["summary"].get!string);                           // 27
	writeln(result["timezone"].get!string);                             // 28
	writeln("** end data **");                                          // 29
}
