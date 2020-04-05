/*
+ darksky-r fetches weather data from darksky.net and builds
+ a plain text file which conky will read to construct a nice weather
+ widget.
+ +
+ © 2020, Alex Vie <silvercircle@gmail.com>
+ License: MIT
+
+ How to build:
+
+ install a rust toolchain
+
+ > cargo build --release
*/

extern crate chrono;
extern crate clap;
extern crate handlebars;
extern crate log;
extern crate reqwest;
extern crate serde;
extern crate serde_json;
extern crate simplelog;

#[macro_use]
extern crate lazy_static;

mod context;
use chrono::{DateTime, Datelike, Local, NaiveDateTime, TimeZone, Timelike};
use context::Context;
use serde_json::json;
use std::fs::File;
use std::io::prelude::*;
use std::path::Path;

fn main() {
    let ctx = context::get_instance();
    let _var: String;
    let mut jsonresult: serde_json::Value = json!({});
    let _matches = clap::App::new("darksky-r")
        .version("0.1")
        .arg(clap::Arg::with_name("apikey")
            .short("a")
            .long("apikey")
            .value_name("APIKEY")
            .help("Set the Api key"))
        .arg(clap::Arg::with_name("loc")
            .short("l")
            .long("loc")
            .value_name("LOC")
            .help("Set your latitude/longitude, e.g. --loc=48.2082,16.3738"))
        .arg(clap::Arg::with_name("no-history")
            .default_value("false")
            .possible_values(&["true", "false"])
            .short("n")
            .long("no-history")
            .value_name("NO-HISTORY")
            .validator(|_val| {
                let _myctx = context::get_instance();
                if _val == "true" {
                    _myctx._data.history = false;
                }
                Ok(())
            })
            .help("Do not record the data set in the database"))
        .arg(clap::Arg::with_name("tempunit")
            .short("t")
            .long("tempunit")
            .value_name("TEMPUNIT")
            .validator(|_val| {
                let _myctx = context::get_instance();
                return if _val != "C" && _val != "F" {
                    Err(format!("{} is invalid. Allowed are 'C' or 'F'.", _val))
                } else {
                    _myctx._data.tempunit = if _val == "C" { 'C' } else { 'F' };
                    Ok(())
                }
            })
            .help("Set the unit for temperature. Allowed are C or F, default is C"))
        .arg(clap::Arg::with_name("visunit")
            .short("v")
            .long("visunit")
            .value_name("VISUNIT")
            .validator(|_val| {
                let _myctx = context::get_instance();
                return if _val != "km" && _val != "miles" {
                    Err(format!("{} is invalid. Allowed are 'km' or 'miles'.", _val))
                } else {
                    _myctx._data.visunit = if _val == "km" { 'k' } else { 'm' };
                    Ok(())
                }
            })
            .help("Set the unit for visibility. Allowed are 'km' or 'miles', default is 'km"))
        .arg(clap::Arg::with_name("pressunit")
            .short("p")
            .long("pressunit")
            .value_name("PRESSUNIT")
            .validator(|_val| {
                let _myctx = context::get_instance();
                return if _val != "hpa" && _val != "inhg" {
                    Err(format!("{} is invalid. Allowed are 'hpa' or 'inhg'.", _val))
                } else {
                    _myctx._data.pressunit = if _val == "inhg" { 'i' } else { 'h' };
                    Ok(())
                }
            })
            .help("Set the unit for pressure. Allowed are 'hpa' or 'inhg', default is 'km"))
        .arg(clap::Arg::with_name("windunit")
            .short("w")
            .long("windunit")
            .value_name("WINDUNIT")
            .validator(|_val| {
                return if _val != "m/s" && _val != "mph" && _val != "knots" && _val != "km/h" {
                    Err(format!("{} is invalid. Allowed are 'm/s', 'km/h', 'mph' or 'knots'.", _val))
                } else {
                    let _v = _val.as_str();
                    let _myctx = context::get_instance();
                    match _v {
                        "m/s" => _myctx._data.windunit = 's',
                        "km/h" => _myctx._data.windunit = 'k',
                        "mph" => _myctx._data.windunit = 'm',
                        "knots" => _myctx._data.windunit = 'n',
                        _ => _myctx._data.windunit = 's'
                    }
                    Ok(())
                }
            })
            .help("Set the unit wind speed unit. Allowed are 'mph', 'km/h', 'knots' or 'm/s'. Default is 'm/s'"))
        .arg(clap::Arg::with_name("cached")
            .short("c")
            .long("cached")
            .value_name("CACHED")
            .validator(|_val| {
                return if _val != "yes" && _val != "no" {
                    Err(format!("{} is invalid. Allowed are yes or no", _val))
                } else {
                    let _myctx = context::get_instance();
                    _myctx._data.cached = if _val == "yes" { true } else { false };
                    Ok(())
                }
            })
            .help("Use cached Json result, if there is none, fetch from API nonetheless."))
        .get_matches();

    ctx._cfg._apikey = String::from(
        _matches
            .value_of("apikey")
            .unwrap_or(ctx._cfg._apikey.as_str()),
    );
    ctx._cfg._loc = String::from(_matches.value_of("loc").unwrap_or(ctx._cfg._loc.as_str()));
    let _url = format!(
        "https://api.darksky.net/forecast/{}/{}{}",
        ctx._cfg._apikey, ctx._cfg._loc, "?units=si"
    );

    if !ctx._data.cached {
        log::info!("Using cached json disabled, fetching new data");
        log::info!("Requesting from: {}", _url);
        let stringresult = fetch_from_api(&_url);
        if stringresult.as_str() != "ERROR" {
            jsonresult =
                serde_json::from_str(stringresult.as_str()).unwrap_or(json!( {"result": "error"} ));
            if !jsonresult["currently"]["time"].is_null() {
                ctx._json_valid = true;
                log::info!(
                    "Successfully fetched from API, timestamp = {}",
                    jsonresult["currently"]["time"]
                );
            }
        }
    }
    if ctx._data.cached || ctx._json_valid == false {
        // try using cache
        log::info!("Trying cached data...");
        if Path::new(&ctx._cfg._cache_file_name.to_str().unwrap()).exists() {
            log::info!(
                "Trying cache, data found at {}",
                ctx._cfg._cache_file_name.to_str().unwrap()
            );
            let mut _file = File::open(&ctx._cfg._cache_file_name).unwrap();
            let mut _raw_json: String = String::new();
            _file.read_to_string(&mut _raw_json).unwrap();
            jsonresult =
                serde_json::from_str(_raw_json.as_str()).unwrap_or(json!( {"result": "error"} ));
        } else {
            log::info!("Trying cached, but cache file not found");
            log::info!("Giving up");
            ctx.cleanup();
            std::process::exit(-100);
        }
    }

    //let _data_json = serde_json::to_string_pretty(&ctx._data).unwrap();
    //println!("Config:\n{}", _data_json);
    output(&mut jsonresult);
    if ctx._data.history {
        let _result = ctx._db.insert_data_point(&jsonresult);
    }
    ctx.cleanup();
    std::process::exit(0);
}

pub fn fetch_from_api(url: &String) -> String {
    let _ctx = context::get_instance();

    let response = reqwest::blocking::get(url).unwrap().text();
    let string = response.unwrap_or("ERR".to_string());
    if string != "ERR" {
        let mut _file = File::create(_ctx._cfg._cache_file_name.to_str().unwrap()).unwrap();
        let _result = _file.write_all(string.as_bytes());
        return string;
    }
    String::from("ERROR")
}

pub fn output(json: &mut serde_json::Value) {
    let ctx = context::get_instance();
    let _now: DateTime<Local> = Local::now();
    ctx._cfg._lastrun = _now.to_rfc3339();
    let _dh = &ctx._data;
    log::info!("output(): Begin");

    /// Output a temperature value using a handlebars template
    ///
    /// [`output_temperature_with_template`]: #output_temperature_with_template
    ///
    ///
    fn output_temperature_with_template(temp: f64, ctx: &Context, template_name: Option<&str>) {
        let temp = ctx
            ._data
            .convert_temperature(temp, Some(ctx._data.tempunit));
        let template = template_name.unwrap_or("one_decimal_and_unit");

        println!(
            "{}",
            ctx._templates
                .render(
                    template,
                    &serde_json::json!( {"value": temp.0, "unit": temp.1} )
                )
                .unwrap()
        );
    }

    /// Output a temperature with exactly one decimal digit and with or
    /// without the unit.
    ///
    /// [`output_temperature`]: #output_temperature
    ///
    ///
    #[inline]
    fn output_temperature(temp: f64, ctx: &Context, print_unit: bool) {
        let temp = ctx
            ._data
            .convert_temperature(temp, Some(ctx._data.tempunit));
        if print_unit {
            println!("{:.1}{}", temp.0, temp.1);
        } else {
            println!("{:.1}", temp.0);
        }
    }

    /// Output short forecast for one day.
    /// prints the condition symbol, low and high temps to expect for a
    /// single day.
    ///
    /// [`output_forecast`]: #output_forecast
    ///
    ///
    fn output_forecast(day: &serde_json::Value, ctx: &context::Context) {
        println!(
            "{}",
            ctx._data
                .get_condition(day["icon"].as_str().unwrap_or("clear"), false)
        );

        output_temperature_with_template(
            day["apparentTemperatureLow"].as_f64().unwrap_or(0.0),
            &ctx,
            Some("zero_decimal_no_unit"),
        );

        output_temperature_with_template(
            day["apparentTemperatureHigh"].as_f64().unwrap_or(0.0),
            &ctx,
            Some("zero_decimal_no_unit"),
        );

        let _time = Local.from_utc_datetime(&NaiveDateTime::from_timestamp(
            day["time"].as_i64().unwrap_or(0),
            0,
        ));
        println!("{}", _time.weekday());
    }

    println!("** begin data **");
    let currently = &json["currently"];

    let _local = Local.from_utc_datetime(&NaiveDateTime::from_timestamp(
        json["currently"]["time"].as_i64().unwrap_or(0),
        0,
    ));
    let _sunrise = Local.from_utc_datetime(&NaiveDateTime::from_timestamp(
        json["daily"]["data"][0]["sunriseTime"]
            .as_i64()
            .unwrap_or(0),
        0,
    ));
    let _sunset = Local.from_utc_datetime(&NaiveDateTime::from_timestamp(
        json["daily"]["data"][0]["sunsetTime"].as_i64().unwrap_or(0),
        0,
    ));

    if _now > _sunrise && _now < _sunset {
        println!(
            "{}",
            _dh.get_condition(currently["icon"].as_str().unwrap_or("clear"), false)
        );
    } else {
        println!(
            "{}",
            _dh.get_condition(currently["icon"].as_str().unwrap_or("clear"), true)
        );
    }

    let mut _t = _dh.convert_temperature(
        currently["apparentTemperature"].as_f64().unwrap(),
        Some(_dh.tempunit),
    );
    println!("{:.1}{}", _t.0, _t.1);
    output_forecast(&json["daily"]["data"][1], &ctx);
    output_forecast(&json["daily"]["data"][2], &ctx);
    output_forecast(&json["daily"]["data"][3], &ctx);

    // these comments are the
    // line numbers in the weather
    // file.
    output_temperature(currently["temperature"].as_f64().unwrap_or(0.0), &ctx, true); // 16
    output_temperature(currently["dewPoint"].as_f64().unwrap_or(0.0), &ctx, true); // 17

    println!(
        "Humidity: {:.0}",
        currently["humidity"].as_f64().unwrap_or(0.0) * 100.0
    ); // 18
       // 19
    if _dh.pressunit == 'h' {
        println!(
            "{:.1} hPa",
            _dh.convert_pressure(
                currently["pressure"].as_f64().unwrap_or(0.0),
                Some(_dh.pressunit)
            )
        );
    } else {
        println!(
            "{:.2} InHg",
            _dh.convert_pressure(
                currently["pressure"].as_f64().unwrap_or(0.0),
                Some(_dh.pressunit)
            )
        );
    }

    let wind = _dh.convert_windspeed(
        currently["windSpeed"].as_f64().unwrap_or(0.0),
        Some(_dh.windunit),
    );
    println!("{:.1} {}", wind.0, wind.1); // 20
    println!("UV: {}", currently["uvIndex"].as_i64().unwrap_or(0)); // 21
    let vis = _dh.convert_vis(
        currently["visibility"].as_f64().unwrap_or(0.0),
        Some(_dh.visunit),
    );
    println!("{:.1} {}", vis.0, vis.1); // 22

    println!("{:02}:{:02}", _sunrise.hour(), _sunrise.minute()); // 23
    println!("{:02}:{:02}", _sunset.hour(), _sunset.minute()); // 24

    println!(
        "{}",
        _dh.deg_to_bearing(currently["windBearing"].as_i64().unwrap_or(0))
    ); // 25

    println!("{:02}:{:02}", _local.hour(), _local.minute()); // 26
    println!("{}", currently["summary"].as_str().unwrap_or("")); // 27

    println!("{}", json["timezone"].as_str().unwrap_or("")); // 28
    output_temperature(
        json["daily"]["data"][0]["temperatureLow"]
            .as_f64()
            .unwrap_or(0.0),
        &ctx,
        true,
    ); // 29
    output_temperature(
        json["daily"]["data"][0]["temperatureHigh"]
            .as_f64()
            .unwrap_or(0.0),
        &ctx,
        true,
    ); // 30
    println!("** end data **"); // 31
    log::info!("output(): End");
}
