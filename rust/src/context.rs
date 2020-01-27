extern crate app_dirs;
extern crate serde_json;
extern crate serde;

use std::sync::Once;
use std::mem;
use std::path::{PathBuf, Path};
use std::fs;
use app_dirs::*;
use serde::{Deserialize, Serialize};
use serde_json::{Result};

const APP_INFO: AppInfo = AppInfo{name: "darksky-r", author: "alex"};

#[derive(Serialize, Deserialize)]
pub struct Config {
    pub _home_dir: String,
    pub _config_dir: PathBuf,
    pub _data_dir: PathBuf,
    pub _log_file_name: PathBuf,
    pub _apikey: String,
    pub _is_intialiazed: bool
}

pub struct Context {
    pub _cfg: Config,
    pub _use_count: i32,
    pub _exe_path: PathBuf
}

impl Context {
    pub fn init(&mut self) -> std::io::Result<()> {
        fs::create_dir_all(&self._cfg._config_dir)?;
        let mut config_file_name = self._cfg._config_dir.clone();
        config_file_name.push("conf.json");

        if Path::new(config_file_name.as_os_str()).exists() {
            println!("The config file name is: {:?}", config_file_name.as_os_str());
        } else {
            let l = serde_json::to_string_pretty(&self._cfg).unwrap();
            println!("{}", l);
        }
        println!("Singleton initialized in {:#?}", self._exe_path);
        Ok(())
    }

    pub fn cleanup(&mut self) {
        println!("Cleaning up Context");
    }

    pub fn inc_use_count(&mut self) {
        self._use_count = self._use_count + 1;
    }

    pub fn write_config(&self) {
    
    }

    pub fn get_config_filename(&self) -> String {
        let mut config_file_name = self._cfg._config_dir.clone();
        config_file_name.push("conf.json");
        String::from(config_file_name.to_str().unwrap().clone())
    }
}

/*
 * this creates exactly one instance of Context and returns a reference
 * to it.
 * it is a Singleton implementation
 */

pub fn get_instance() -> &'static mut Context {
    static mut CTX: *mut Context = 0 as *mut Context;
    static ONCE: Once = Once::new();
    unsafe {
        ONCE.call_once(|| {
            let singleton = Context {
                _use_count: 0,
                _exe_path: std::env::current_exe().unwrap(),
                _cfg: Config { _home_dir: "Foo".to_string(),
                    _is_intialiazed: false,
                    _apikey: "FOO".to_string(),
                    _config_dir: get_app_dir(AppDataType::UserConfig, &APP_INFO, "").unwrap(),
                    _data_dir: get_app_dir(AppDataType::UserData, &APP_INFO, "").unwrap(),
                    _log_file_name: [ get_app_dir(AppDataType::UserData, &APP_INFO, "").unwrap(), PathBuf::from("log.log") ].iter().collect()
                }
            };
            CTX = mem::transmute(Box::new(singleton));

            let ctx = &mut *CTX;
            let _result = ctx.init();  // (*CTX).init(); does the same
            ctx._cfg._is_intialiazed = true;
        });
        (*CTX)._use_count = (*CTX)._use_count + 1;
        &mut*CTX
    }
}


