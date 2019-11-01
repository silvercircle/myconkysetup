/++
 + darksky-d fetches weather data from darksky.net and builds 
 + a plain text file which conky will read to construct a nice weather 
 + widget. 
 +  
 + This module handles sqlite3 access to build a history of recorded 
 + weather data. 
 +  
 + © 2019, Alex Vie <silvercircle@gmail.com>
 + License: MIT
 +/

module darkskyd.database;

import d2sqlite3.database, d2sqlite3.statement;
import std.stdio, std.path;
import vibe.data.json;
import darkskyd.context;

// values for temperature, pressure, windspeed etc. can be either
// float or int in the Json. We always want float.
float getFloatValue(const Json val)
{
	try {
		return val.get!float;
	} catch(JSONException e) {
		return cast(float)val.get!int;
	}
}

final class DB
{
	this(const string dbname = null)
	{
		try {
			if(dbname != null) {
				this.db = d2sqlite3.database.Database(dbname);
				this.db.run(q"[
					CREATE TABLE IF NOT EXISTS history 
						(
							id INTEGER PRIMARY KEY AUTOINCREMENT,
							timestamp INTEGER DEFAULT 0,
							summary TEXT NOT NULL DEFAULT 'unknown',
							icon TEXT NOT NULL DEFAULT 'unknown',
							temperature REAL NOT NULL DEFAULT 0.0,
							feelslike REAL NOT NULL DEFAULT 0.0,
							dewpoint REAL DEFAULT 0.0,
							windbearing INTEGER DEFAULT 0,
							windspeed REAL DEFAULT 0.0,
							windgust REAL DEFAULT 0.0,
							humidity REAL DEFAULT 0.0,
							visibility REAL DEFAULT 0.0,
							pressure REAL DEFAULT 1013.0,
							precip_probability REAL DEFAULT 0.0,
							precip_intensity REAL DEFAULT 0.0,
							precip_type TEXT DEFAULT 'none',
							uvindex INTEGER DEFAULT 0,
							sunrise INTEGER DEFAULT 0,
							sunset INTEGER DEFAULT 0
						)]");
				this.valid = true;
			}
		} catch(SqliteException e) {
			this.valid = false;
		}
	}

	void addDataPoint(ref Json data)
	{
		if(this.valid == false) {
			return;
		}
		Json currently = data["currently"];

		Statement st = this.db.prepare(q"[
				INSERT INTO history
					(
						timestamp, summary, icon, temperature,
						feelslike, dewpoint, windbearing, windspeed,
						windgust, humidity, visibility, pressure,
						precip_probability, precip_intensity, precip_type,
						uvindex, sunrise, sunset
					) 
				  VALUES
					(
						:timestamp, :summary, :icon, :temperature,
						:feelslike, :dewpoint, :windbearing, :windspeed,
						:windgust, :humidity, :visibility, :pressure,
						:precip_probability, :precip_intensity, :precip_type,
						:uvindex, :sunrise, :sunset
			)]");

		st.bind(":timestamp", currently["time"].get!int);
		st.bind(":summary", currently["summary"].get!string);
		st.bind(":icon", currently["icon"].get!string);
		st.bind(":temperature", currently["temperature"].getFloatValue());
		st.bind(":feelslike", currently["apparentTemperature"].getFloatValue());
		st.bind(":dewpoint", currently["dewPoint"].getFloatValue());
		st.bind(":windbearing", currently["windBearing"].get!int);
		st.bind(":windspeed", currently["windSpeed"].getFloatValue());
		st.bind(":windgust", currently["windGust"].getFloatValue());
		st.bind(":humidity", currently["humidity"].getFloatValue());
		st.bind(":visibility", currently["visibility"].getFloatValue());
		st.bind(":pressure", currently["pressure"].get!float);
		st.bind(":precip_intensity", currently["precipIntensity"].getFloatValue());
		st.bind(":precip_probability", currently["precipProbability"].getFloatValue());
		st.bind(":precip_type", "precipType" in currently ? currently["precipType"].get!string : "none");
		st.bind(":uvindex", currently["uvIndex"].get!int);

		st.bind(":sunrise", data["daily"]["data"][0]["sunriseTime"].get!int);
		st.bind(":sunset", data["daily"]["data"][0]["sunsetTime"].get!int);
		st.execute();
		st.reset();
	}

	void close()
	{
		try {
			this.db.close();
		} catch (SqliteException e) {}
	}

public:
	static DB getInstance(const string dbname = null)
	{
		if (!isInstantiated) {
			synchronized (DB.classinfo) {
				if (!instance_) {
					instance_ = new DB(dbname);
				}
				isInstantiated = true;
			}
		}
		return instance_;
	}

private:
	static bool isInstantiated = 0;
	__gshared DB instance_;
	d2sqlite3.database.Database db;
	bool valid = false;
}

static ~this()
{
	DB db = DB.getInstance();
	db.close();
}

