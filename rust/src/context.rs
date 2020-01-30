extern crate app_dirs;
pub mod database;
pub mod datahandler;

pub mod database;

use std::sync::Once;
use std::mem;
use std::path::{PathBuf, Path};
use app_dirs::*;
use std::fs::{File, create_dir_all};
use std::io::prelude::*;
use simplelog::{WriteLogger, LevelFilter, Config as LogConfig};
use serde::{Deserialize, Serialize};

const APP_INFO: AppInfo = AppInfo{name: "darksky-r", author: "alex"};

#[derive(Serialize, Deserialize)]
pub struct Config {
    pub _config_dir: PathBuf,
    pub _data_dir: PathBuf,
    pub _log_file_name: PathBuf,
    pub _cache_file_name: PathBuf,
    pub _apikey: String,
    pub _loc: String,
    pub _is_intialiazed: bool,
    pub _updated_from_api: u32,
    pub _lastrun: String
}

pub struct Context {
    pub _cfg: Config,
    pub _use_count: i32,
    pub _exe_path: PathBuf,
<<<<<<< HEAD
    _db: database::DB,
    pub _data: datahandler::DataHandler
=======
    _db: database::DB
>>>>>>> a573a099c10390c597dac9755329c8a5902252f2
}

impl Context {
    pub fn init(&mut self) -> std::io::Result<()> {
        create_dir_all(&self._cfg._config_dir)?;
        let config_file_name = self.get_config_filename();
        let log_file_name = self.get_log_filename();
        let _logresult = WriteLogger::init(LevelFilter::Info, LogConfig::default(), File::create(log_file_name.clone())?);

        log::info!("Context::init(): Log file created at {}", log_file_name);
        debug_assert_ne!(config_file_name.len(), 0);
<<<<<<< HEAD
=======

>>>>>>> a573a099c10390c597dac9755329c8a5902252f2
        if Path::new(config_file_name.as_str()).exists() {
            log::info!("Context::init(): Found a config file at {} and using it", config_file_name);
            let mut _file = File::open(config_file_name)?;
            let mut _raw_json: String = String::new();

            _file.read_to_string(&mut _raw_json)?;
            self._cfg = serde_json::from_str(_raw_json.as_str())?;
        } else {
            log::info!("Context::init(): The config file did not exist. Creating at {}", config_file_name);
            let _l = serde_json::to_string_pretty(&self._cfg).unwrap();
            let mut _file = File::create(config_file_name)?;
            _file.write_all(_l.as_bytes())?;
        }

        let mut database_filename = self._cfg._data_dir.clone();
        database_filename.push("darksky-r.sqlite");
        let _dbresult = self._db.connect(database_filename.to_str().unwrap());
        log::info!("Context::init(): Created the database at {}", database_filename.to_str().unwrap());
        Ok(())
    }

    pub fn cleanup(&mut self) {
        let _r = self.write_config();
    }

    pub fn inc_use_count(&mut self) {
        self._use_count = self._use_count + 1;
    }

    pub fn write_config(&self) -> std::io::Result<()> {
        let _config_file_name = self.get_config_filename();
        let mut _file = File::create(_config_file_name)?;
        let _l = serde_json::to_string_pretty(&self._cfg).unwrap();
        _file.write_all(_l.as_bytes())?;
        _file.flush()?;
        Ok(())
    }

    pub fn get_config_filename(&self) -> String {
        let mut config_file_name = self._cfg._config_dir.clone();
        config_file_name.push("conf.json");
        String::from(config_file_name.to_str().unwrap().clone())
    }

    pub fn get_log_filename(&self) -> String {
        let mut log_file_name = self._cfg._data_dir.clone();
        log_file_name.push("darksky-r.log");
        String::from(log_file_name.to_str().unwrap().clone())
    }
}

/*
 * this creates exactly one instance of Context and returns a reference
 * to it.
 *
 * It is very similar to the Singleton pattern in other languages and can be used
 * for application-wide objects that only must exist once.
<<<<<<< HEAD
 * https://stackoverflow.com/questions/27791532/how-do-i-create-a-global-mutable-singleton
=======
>>>>>>> a573a099c10390c597dac9755329c8a5902252f2
 */

pub fn get_instance() -> &'static mut Context {
    static mut CTX: *mut Context = 0 as *mut Context;
    static ONCE: Once = Once::new();
    unsafe {
        ONCE.call_once(|| {
            let data_dir = get_app_dir(AppDataType::UserData, &APP_INFO, "").unwrap();
            let context = Context {
                _use_count: 0,
                _db: database::DB::default(),
                _exe_path: std::env::current_exe().unwrap(),
                _data: datahandler::DataHandler::new(),
                _cfg: Config {
                    _lastrun: chrono::Local::now().to_rfc3339(),
                    _is_intialiazed: false,
                    _apikey: "FOO".to_string(),
                    _updated_from_api: 0,
                    _loc: "0000".to_string(),
                    _config_dir: get_app_dir(AppDataType::UserConfig, &APP_INFO, "").unwrap(),
                    _data_dir: data_dir.clone(),
                    _log_file_name: [ data_dir.clone(), PathBuf::from("log.log") ].iter().collect(),
                    _cache_file_name: [ data_dir.clone(), PathBuf::from("cache.json") ].iter().collect(),
                }
            };
            CTX = mem::transmute(Box::new(context));

            let ctx = &mut *CTX;
            let _result = ctx.init();  // (*CTX).init(); does the same
            ctx._cfg._is_intialiazed = true;
        });
        (*CTX)._use_count = (*CTX)._use_count + 1;
        &mut*CTX
    }
}
