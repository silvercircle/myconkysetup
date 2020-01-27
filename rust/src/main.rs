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
use std::sync::{Mutex};

pub struct LocalConfig {
    history:    bool,
    tempunit:   char,
    cached:     bool,
    visunit:    String,
    windunit:   String,
    pressunit:  String,
    running:    bool,
}

impl LocalConfig {
    pub fn new() -> Self {
        println!("Builing local config");
        Self {
            history:    true,
            tempunit:   'C',
            cached:     true,
            visunit:    "km".to_string(),
            windunit:   "ms".to_string(),
            pressunit:  "hpa".to_string(),
            running:    false,
        }
    }
}

// this makes a mutable global reference to an instance of LocalConfig
// a Mutex is required to access it
lazy_static! {
    pub static ref CONFIG: Mutex<LocalConfig> = Mutex::new(LocalConfig::new());
}
fn main() {
    let ctx = context::get_instance();
    // test global reference to LocalConfig object
        {
            let mut _c = CONFIG.lock().unwrap();
            println!("Config.running initial value: {}", _c.running);
            _c.running = true;
            println!("Config.running is now: {}", _c.running);
        }
        let mut _c = CONFIG.lock().unwrap();
        println!("Config.running is still: {}", _c.running);
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
            .short("n")
            .long("no-history")
            .value_name("NO-HISTORY")
            .help("Do not record the data set in the database"))
        .arg(clap::Arg::with_name("tempunit")
            .short("t")
            .long("tempunit")
            .value_name("TEMPUNIT")
            .help("Set the unit for temperature. Allowed are C or F, default is C"))
        .arg(clap::Arg::with_name("visunit")
            .short("v")
            .long("visunit")
            .value_name("VISUNIT")
            .help("Set the unit for visibility. Allowed are 'km' or 'miles', default is 'km"))
        .arg(clap::Arg::with_name("pressunit")
            .short("p")
            .long("pressunit")
            .value_name("PRESSUNIT")
            .help("Set the unit for pressure. Allowed are 'hpa' or 'inhg', default is 'km"))
        .arg(clap::Arg::with_name("windunit")
            .short("w")
            .long("windunit")
            .value_name("WINDUNIT")
            .help("Set the unit for wind speed. Allowed are 'ms', 'km', 'knots' or 'mph' are allowed, defaults to 'ms"))
        .arg(clap::Arg::with_name("cached")
            .short("c")
            .long("cached")
            .value_name("CACHED")
            .help("Use cached Json result, if there is none, fetch from API nonetheless."))
        .get_matches();

    ctx._cfg._apikey = String::from(_matches.value_of("apikey").unwrap_or(ctx._cfg._apikey.as_str()));
    ctx._cfg._loc = String::from(_matches.value_of("loc").unwrap_or(ctx._cfg._loc.as_str()));
    let _foo: DateTime<Local> = Local::now();
    println!("Last run time: {}", ctx._cfg._lastrun);
    ctx._cfg._lastrun = _foo.to_rfc3339();
    let _url = format!("https://api.darksky.net/forecast/{}/{}{}", ctx._cfg._apikey, ctx._cfg._loc, "?units=si");
    log::info!("Requesting from: {}", _url);
    let _rsp = fetch(&_url);
    ctx.inc_use_count();
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
