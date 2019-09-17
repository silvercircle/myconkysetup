/++
 + darksky.d
 +
 + fetch weather from darksky and crete conky-readable format
 +
 + (C) 2019, Alex Vie <silvercircle@gmail.com>
 + License: MIT
 +/

module darksky;

import std.stdio, std.getopt, core.stdc.stdlib: exit;
import std.net.curl, std.path, std.file, std.conv;
import std.datetime, std.string;
import vibe.data.json;

import context;

struct Config {
    string  key = "";
    bool    fUseCached = false;
    string  tempUnit = "C";
    string  windUnit = "ms";
    string  visUnit = "km";
    string  windSpeed;
    string  vis;
}

string[] _bearings = ["N","NNE","NE","ENE","E","ESE", "SE", "SSE","S","SSW","SW","WSW","W","WNW","NW","NNW"];


/++
 + print usage and exit
 +/
void print_usage()
{  
	writeln("\n    USAGE:");
	writef(q"[    darksky-d --key=YOUR_API_KEY --useCached=true|false --loc=lat,lon --tempUnit=C|F --windUnit=ms|km|knots|mph
                --visUnit = km|miles

    key:        Your darksky API key. Mandatory.
    useCached:  When true, use cached JSON. If none is found, print a notice and exit. Otherwise fetch
                fresh data from the darksky API.
    loc:        Your location, see darksky API docs. Must be in the format latitude,longitude
                Example: --loc=48.2082,16.3738. This will be saved in the config until you specify
                a different location.
    tempUnit:   C or F, default is C
    windUnit:   Unit for the wind speed. ms (m/s), km (km/h) knots, or mph are available. Defaults to m/s
]");
}

/++
 + take a wind bearing in degrees and return a symbolic wind direction like NNW
 +/

string degToBearing(uint wind_bearing)
{
    if(wind_bearing < 0 || wind_bearing > 360) {
        wind_bearing = 0;
    }
    const int val = cast(int)((wind_bearing / 22.5) + .5);
    return _bearings[val % 16];
}

Config cfg = Config();

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
            writeln("Error loading json data from cachefile. Please run once without --useCached.");
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
                                        "tempUnit", &cfg.tempUnit, "windUnit", &cfg.windUnit, "visUnit", &cfg.visUnit);
	if(stdargs.helpWanted) {
        print_usage();
        ctx.orderlyShutDown(-1);
    }

    if(cfg.key.length < 10) {
        writeln("You did not specify a valid API KEY");
        ctx.orderlyShutDown(-3);
    }

    if(cfg.fUseCached == false) {
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

string[string] daytimeCodes, nighttimeCodes;
/++
 + this is a module constructor, it runs before main()
 +/
static this()
{
    /+ 
     + these arrays translate condition strings to a single letter icon code which
     + directly corresponds to the character in the weather symbol font
     +
     + since there are different weather icons for day and night, 2 arrays are
     + needed
     +/
    daytimeCodes = [ "clear-day": "a", "clear-night": "a", "rain": "g",
        "snow": "o", "sleet": "x", "wind": "9", "fog": "7", "cloudy": "e",
        "partly-cloudy-day": "c", "partly-cloudy-night": "a"];


    nighttimeCodes = [ "clear-day": "A", "clear-night": "A", "rain": "G",
        "snow": "O", "sleet": "x", "wind": "9", "fog": "7", "cloudy": "f",
        "partly-cloudy-day": "C", "partly-cloudy-night": "C"];
}

void generateOutput(Json result)
{

    T convertTemperature(T)(const T temp)
    {
        if(cfg.tempUnit == "F") {
            return (temp * 9/5) + 32;
        } else {
            return temp;
        }
    }

    float convertVis(const float vis)
    {
        return cfg.visUnit == "miles" ? vis / 1.609 : vis;
    }

    float convertWindspeed(const float speed)
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

    void outputTemperature(const Json val, const bool addUnit = false, const string format = "%.1f%s\n")
    {
        try {
            writef(format, convertTemperature(val.get!float), addUnit ? "°" ~ cfg.tempUnit : "");
        } catch (JSONException e) {
            writef("%d%s\n", convertTemperature(val.get!int), addUnit ? "°" ~ cfg.tempUnit : "");
        }
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
        string dow = t.dayOfWeek.to!string;
        writeln(capitalize(cast(string)dow));
    }

    writeln("** begin data **");
    Json currently = result["currently"];
    const SysTime timestamp = SysTime.fromUnixTime(currently["time"].get!int);

    SysTime sunrise = SysTime.fromUnixTime(result["daily"]["data"][0]["sunriseTime"].get!int);
    SysTime sunset = SysTime.fromUnixTime(result["daily"]["data"][0]["sunsetTime"].get!int);

    const SysTime now = Clock.currTime;

    /+
     + show the day icon after sunrise and before sunset
     +/
    if (now > sunrise && now < sunset) {
        if(currently["icon"].get!string in daytimeCodes) {
            writeln(daytimeCodes[currently["icon"].get!string]);
        } else {
            writeln("a");
        }
    } else {
        if(currently["icon"].get!string in nighttimeCodes) {
            writeln(nighttimeCodes[currently["icon"].get!string]);
        } else {
            writeln("a");
        }
    }
    outputTemperature(currently["apparentTemperature"], true);
    outputForecast(result["daily"]["data"][1]);
    outputForecast(result["daily"]["data"][2]);
    outputForecast(result["daily"]["data"][3]);
    outputTemperature(currently["temperature"], true);
    outputTemperature(currently["dewPoint"], true);

    writef("Humidity: %d\n", cast(int)(currently["humidity"].get!float * 100));
    writeln(cast(int)currently["pressure"].get!float);
    writef("%.1f %s\n", convertWindspeed(currently["windSpeed"].get!float), cfg.windSpeed);
    writef("UV: %d\n", currently["uvIndex"].get!int);
    writef("%.1f %s\n", convertVis(currently["visibility"].get!float), cfg.visUnit);

    writef("%02d:%02d\n", sunrise.hour, sunrise.minute);
    writef("%02d:%02d\n", sunset.hour, sunset.minute);                  // 23

    writeln(degToBearing(currently["windBearing"].get!int));            // 24

    SysTime time = SysTime.fromUnixTime(currently["time"].get!int);     // 25
    writef("%02d:%02d\n", time.hour, time.minute);                      // 26
    writeln(currently["summary"].get!string);                           // 27
    writeln(result["timezone"].get!string);                             // 28

    writeln("** end data **");
}
