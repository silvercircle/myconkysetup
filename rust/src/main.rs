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

extern crate log;
extern crate simplelog;
extern crate clap;
extern crate chrono;
extern crate reqwest;
extern crate serde_json;
extern crate serde;
#[macro_use]
extern crate lazy_static;

mod context;
use chrono::{DateTime, Local};
use std::fs::{File};
use std::io::prelude::*;

fn main() {
    let ctx = context::get_instance();
    //
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
                    _myctx._data.visunit = if _val == "C" { 'C' } else { 'F' };
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
                return if _val != "hpa" && _val != "inhg" {
                    Err(format!("{} is invalid. Allowed are 'hpa' or 'inhg'.", _val))
                } else {
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
                if _val != "yes" && _val != "no" {
                    return Err(format!("{} is invalid. Allowed are yes or no", _val));
                }
                else {
                    let _myctx = context::get_instance();
                    _myctx._data.history = if _val == "yes" { true } else { false };
                    return Ok(());
                }
            })
            .help("Use cached Json result, if there is none, fetch from API nonetheless."))
        .get_matches();

    ctx._cfg._apikey = String::from(_matches.value_of("apikey").unwrap_or(ctx._cfg._apikey.as_str()));
    ctx._cfg._loc = String::from(_matches.value_of("loc").unwrap_or(ctx._cfg._loc.as_str()));
    let _foo: DateTime<Local> = Local::now();
    println!("Last run time: {}", ctx._cfg._lastrun);
    ctx._cfg._lastrun = _foo.to_rfc3339();
    let _url = format!("https://api.darksky.net/forecast/{}/{}{}", ctx._cfg._apikey, ctx._cfg._loc, "?units=si");
    log::info!("Requesting from: {}", _url);
    //let _rsp = fetch(&_url);
    ctx.inc_use_count();
    let _c = &ctx._data;
    println!("Wind from 42 is {}", _c.deg_to_bearing(42));
    println!("Wind from 271 is {}",_c.deg_to_bearing(271));
    println!("Temp 32C in C and F: {}, {}", _c.convert_temperature(32 as f32, Some('C')), _c.convert_temperature(32.0 as f32, Some('F')));
    println!("Wind 5 m/s = {:?}, {:?}, {:?}", _c.convert_windspeed(5 as f64, None), _c.convert_windspeed(5.0, Some('k')), _c.convert_windspeed(5 as f64, Some('m')));
    println!("Day condition for clear-day: {}", _c.get_condition("clear-day", false));
    println!("Night condition for clear-night: {}", _c.get_condition("clear-night", true));
    println!("Visibility: {}", _c.convert_vis(10.0));
    println!("Pressure: {}", _c.convert_pressure(1013.0, Some('i')));
    
    let _data_json = serde_json::to_string_pretty(&ctx._data).unwrap();
    println!("Config:\n{}", _data_json);
    ctx.cleanup();
}

pub fn fetch(url: &String) -> reqwest::Result<String> {
    let ctx = context::get_instance();

    let response = reqwest::blocking::get(url)?.text()?;
    let _val: serde_json::Value = serde_json::json!(response);

    let mut _file = File::create(ctx._cfg._cache_file_name.to_str().unwrap()).unwrap();
    let _result = _file.write_all(response.as_bytes());

    Ok(response)
}
