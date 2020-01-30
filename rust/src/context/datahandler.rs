use std::collections::HashMap;
use serde::{Serialize};

lazy_static!{
    static ref DAYTIME_CODES: HashMap<&'static str, char> = [
 	    ("clear-day",   'a'),
 	    ("clear-night", 'a'),
 	    ("rain",        'g'),
		("snow",        'o'),
		("sleet",       'x'),
		("wind",        '9'),
		("fog",         '7'),
		("cloudy",      'e'),
		("partly-cloudy-day",   'c'),
		("partly-cloudy-night", 'C')
    ].iter().copied().collect();

    static ref NIGHTTIME_CODES: HashMap<&'static str, char> = [
        ("clear-day",   'A'),
        ("clear-night", 'A'),
        ("rain",        'G'),
		("snow",        'O'),
		("sleet",       'x'),
		("wind",        '9'),
		("fog",         '7'),
		("cloudy",      'f'),
		("partly-cloudy-day",   'C'),
		("partly-cloudy-night", 'C')
	].iter().copied().collect();
}

#[derive(Serialize)]
pub struct DataHandler {
    wind_directions: [&'static str; 16],
    pub history:    bool,
    pub tempunit:   char,
    pub cached:     bool,
    pub visunit:    char,
    pub windunit:   char,
    pub pressunit:  char,
    pub running:    bool,
}

impl DataHandler {
    pub fn new() -> Self {
        Self {
            history:    true,
            tempunit:   'C',
            cached:     true,
            visunit:    'k',
            windunit:   'm',
            pressunit:  'h',
            running:    false,
            wind_directions: ["N","NNE","NE","ENE","E","ESE", "SE", "SSE","S","SSW","SW","WSW","W","WNW","NW","NNW"]
        }
    }
    pub fn deg_to_bearing(&self, mut wind_direction: i32) -> &'static str {
        wind_direction = if wind_direction < 0 || wind_direction > 360 { 0 } else { wind_direction };
        let val = (wind_direction as f64 / 22.5) + 0.5;
        self.wind_directions[(val as usize) % 16]
    }

    #[inline]
    pub fn convert_temperature(&self, temp: f32, _unit: Option<char>) -> f32 {
        let unit = if _unit == None { self.tempunit } else { _unit.unwrap() };
        if unit == 'F' { ((temp * (9/5) as f32) + 32.0) } else { temp }
    }

    // km > miles
    #[inline]
    pub fn convert_vis(&self, vis: f64) -> f64
    {
        if self.visunit == 'm' { vis / 1.609 } else { vis }
    }

    pub fn get_condition(&self, _cond: &str, _night: bool) -> char {
        return if _night {
            if NIGHTTIME_CODES.contains_key(_cond) { *NIGHTTIME_CODES.get(_cond).unwrap() } else { 'c' }
        } else {
            if DAYTIME_CODES.contains_key(_cond) { *DAYTIME_CODES.get(_cond).unwrap() } else { 'c' }
        }
    }
    // windspeed, API always returns m/s, we convert to km/h, mph or knots
    // on user's request.
    pub fn convert_windspeed(&self, speed: f64, _unit: Option<char>) -> (&'static str, f64) {
        let unit = if _unit == None { self.windunit } else { _unit.unwrap() };

        return match unit {
            'k' => { ("km/h", speed * 3.6) }            // km/h
            'm' => { ("mph", speed * 2.237) }           // mph
            'n' => { ("knots", speed * 1.944) }         // knots
            _ => { ("m/s", speed) }
        }
    }

    // hPa > InHg
    #[inline]
    pub fn convert_pressure(&self, _hpa: f64, _unit: Option<char>) -> f64
    {
        let unit = if _unit == None { self.pressunit } else { _unit.unwrap() };
        return if unit == 'i' { _hpa / 33.863886666667 } else { _hpa };
    }
}
