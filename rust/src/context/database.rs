extern crate rusqlite;

use rusqlite::NO_PARAMS;

pub struct DB {
    _path: String,
    _valid: bool,
    _conn: Option<rusqlite::Connection>
}

impl Default for DB {
    fn default() -> DB {
        DB { _path: String::from(""), _valid: false, _conn: None }
    }
}

impl DB {
    pub fn connect(&mut self, _path: &str) -> rusqlite::Result<()> {
        self._conn = Some(rusqlite::Connection::open(_path).unwrap());
        let _c = self._conn.as_ref().unwrap();
        let _dbresult = _c.execute(r#"CREATE TABLE IF NOT EXISTS history
<<<<<<< HEAD
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
                precip_intensity REAL DEFAULT 0.07,
                precip_type TEXT DEFAULT 'none',
                uvindex INTEGER DEFAULT 0,
                sunrise INTEGER DEFAULT 0,
                sunset INTEGER DEFAULT 0
            )"#, NO_PARAMS)?;
=======
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
							precip_intensity REAL DEFAULT 0.07,
							precip_type TEXT DEFAULT 'none',
							uvindex INTEGER DEFAULT 0,
							sunrise INTEGER DEFAULT 0,
							sunset INTEGER DEFAULT 0
						)"#, NO_PARAMS)?;
>>>>>>> a573a099c10390c597dac9755329c8a5902252f2
        Ok(())
    }
}