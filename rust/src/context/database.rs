extern crate rusqlite;

use rusqlite::named_params;
use rusqlite::NO_PARAMS;

pub struct DB {
    _path: String,
    _valid: bool,
    _conn: Option<rusqlite::Connection>,
}

impl Default for DB {
    fn default() -> DB {
        DB {
            _path: String::from(""),
            _valid: false,
            _conn: None,
        }
    }
}

impl DB {
    pub fn connect(&mut self, _path: &str) -> rusqlite::Result<()> {
        self._conn = Some(rusqlite::Connection::open(_path).unwrap());
        let _c = self._conn.as_ref().unwrap();
        let _dbresult = _c.execute(
            r#"CREATE TABLE IF NOT EXISTS history
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
                humidity REAL DEFAULT 0.0ping,
                visibility REAL DEFAULT 0.0,
                pressure REAL DEFAULT 1013.0,
                precip_probability REAL DEFAULT 0.0,
                precip_intensity REAL DEFAULT 0.07,
                precip_type TEXT DEFAULT 'none',
                uvindex INTEGER DEFAULT 0,
                sunrise INTEGER DEFAULT 0,
                sunset INTEGER DEFAULT 0
            )"#,
            NO_PARAMS,
        )?;
        Ok(())
    }

    pub fn insert_data_point(&self, _data: &serde_json::Value) -> rusqlite::Result<()> {
        let currently: &serde_json::Value = &_data["currently"];

        let _dbresult = self._conn.as_ref().unwrap().execute_named(
            r#"
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
			)"#,
            named_params! {
                ":timestamp": currently["time"].as_i64().unwrap_or(0),
                ":summary": currently["summary"].as_str().unwrap_or("clear"),
                ":icon": currently["icon"].as_str().unwrap_or("c"),
                ":temperature": currently["temperature"].as_f64().unwrap_or(0.0),
                ":feelslike": currently["apparentTemperature"].as_f64().unwrap_or(0.0),
                ":dewpoint": currently["dewPoint"].as_f64().unwrap_or(0.0),
                ":windbearing": currently["windBearing"].as_i64().unwrap_or(0),
                ":windspeed": currently["windSpeed"].as_f64().unwrap_or(0.0),
                ":windgust": currently["windGust"].as_f64().unwrap_or(0.0),
                ":humidity": currently["humidity"].as_f64().unwrap_or(0.0),
                ":visibility": currently["visibility"].as_f64().unwrap_or(0.0),
                ":pressure": currently["pressure"].as_f64().unwrap_or(0.0),
                ":precip_intensity": currently["precipIntensity"].as_f64().unwrap_or(0.0),
                ":precip_probability": currently["precipProbability"].as_f64().unwrap_or(0.0),
                ":precip_type": currently["precipType"].as_str().unwrap_or("none"),
                ":uvindex": currently["uvIndex"].as_i64().unwrap_or(0),
                ":sunrise": _data["daily"]["data"][0]["sunriseTime"].as_i64().unwrap_or(0),
                ":sunset": _data["daily"]["data"][0]["sunsetTime"].as_i64().unwrap_or(0),
            },
        )?;
        Ok(())
    }

    pub fn close(&self) {
        drop(self._conn.as_ref());
    }
}
