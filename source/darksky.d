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
}

string[] _bearings = ["N","NNE","NE","ENE","E","ESE", "SE", "SSE","S","SSW","SW","WSW","W","WNW","NW","NNW"];


/++
 + print usage and exit
 +/
void print_usage()
{  
	writeln("\nUSAGE:");
	writef(q"[darksky-d --key=YOUR_API_KEY --useCached=true|false --loc=lat,lon
 
    key:        Your darksky API key. Mandatory.
    useCached:  When true, use cached JSON. If none is found, print a notice and exit. Otherwise fetch
                data from the darksky API.
    loc:        Your location, see darksky API docs. Must be in the format latitude,longitude
                Example: --loc=48.2082,16.3738. This will be saved in the config until you specify
                a different location.
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

/++
 + returns 0 on success, any other value means failure and a possibly incomplete
 + output
 +/
void main(string[] args)
{
    Config cfg = Config();
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
            writeln("Error loading json data from cachefile.");
            result = -2;
            return returnError(-2);
        }
    }

    void fetchFromApi()
    {
        const string url = "https://api.darksky.net/forecast/" ~ cfg.key ~ "/" ~ ctx.cfg.location ~ "?units=" ~ ctx.cfg.units;
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

	const GetoptResult stdargs = getopt(args, "key", &cfg.key, "useCached", &cfg.fUseCached);
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
    daytimeCodes = [ "clear-day": "a", "clear-night": "a", "rain": "g",
        "snow": "o", "sleet": "x", "wind": "9", "fog": "7", "cloudy": "e",
        "partly-cloudy-day": "c", "partly-cloudy-night": "a"];


    nighttimeCodes = [ "clear-day": "A", "clear-night": "A", "rain": "G",
        "snow": "O", "sleet": "x", "wind": "9", "fog": "7", "cloudy": "f",
        "partly-cloudy-day": "C", "partly-cloudy-night": "C"];
}

void generateOutput(Json result)
{
    void outputForecast(const Json day)
    {
        if(day["icon"].get!string in daytimeCodes) {
            writeln(daytimeCodes[day["icon"].get!string]);
        } else {
            writeln("a");
        }
        writeln(cast(int)day["apparentTemperatureLow"].get!float);
        writeln(cast(int)day["apparentTemperatureHigh"].get!float);
        const SysTime t = SysTime.fromUnixTime(day["time"].get!int);
        string dow = t.dayOfWeek.to!string;
        writeln(capitalize(cast(string)dow));
    }

    writeln();
    Json currently = result["currently"];
    const SysTime timestamp = SysTime.fromUnixTime(currently["time"].get!int);
    //writeln(timestamp.hour);

    if (timestamp.hour > 6 && timestamp.hour < 18) {
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
    writeln(cast(int)currently["apparentTemperature"].get!float);
    outputForecast(result["daily"]["data"][1]);
    outputForecast(result["daily"]["data"][2]);
    outputForecast(result["daily"]["data"][3]);
    writeln(cast(int)currently["temperature"].get!float);
    writef("Dew point: %.1f\n", currently["dewPoint"].get!float);
    writef("Humidity: %d\n", cast(int)(currently["humidity"].get!float * 100));
    writeln(cast(int)currently["pressure"].get!float);
    writef("%.1f\n", currently["windSpeed"].get!float);
    writef("UV: %d\n", currently["uvIndex"].get!int);
    writef("%.0f\n", currently["visibility"].get!float);

    SysTime sunrise = SysTime.fromUnixTime(result["daily"]["data"][0]["sunriseTime"].get!int);
    SysTime sunset = SysTime.fromUnixTime(result["daily"]["data"][0]["sunsetTime"].get!int);
    writef("%02d:%02d\n", sunrise.hour, sunrise.minute);
    writef("%02d:%02d\n", sunset.hour, sunset.minute);

    writeln(degToBearing(currently["windBearing"].get!int));

    SysTime time = SysTime.fromUnixTime(currently["time"].get!int);
    writef("%02d:%02d\n", time.hour, time.minute);
    writeln(currently["summary"].get!string);   
}
