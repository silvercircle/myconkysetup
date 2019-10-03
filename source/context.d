/++
 + darksky-d fetches weather data from darksky.net and builds 
 + a plain text file which conky will read to construct a nice weather 
 + widget. 
 + + 
 + © 2019, Alex Vie <silvercircle@gmail.com>
 + License: MIT
 +/
 
module context;

import std.process, std.path, std.stdio, std.file, std.string, std.conv, core.stdc.stdlib: exit;
import vibe.data.json, vibe.data.serialization, std.datetime :Clock, SysTime;
import db;

/**
 * app configuration class
 * implemented as thread safe singleton
 * used for config storage via JSON, logging. 
 */
final class GlobalContext
{
private:

	/* 
	 * example of a config data object with sub-structures representable
	 * in Json
	 */
	struct Config {
		@optional @name("appname") 		string      myName = "darksky";
		@optional @name("firstrun") 	SysTime		firstRun;
		@optional @name("lastrun")		SysTime		lastRun;
		@optional @name("datadir") 		string		dataDir;
		@optional @name("is_portable")	bool		isPortable = false;
		@optional @name("last_fetched") SysTime		lastFetched;
		@optional @name("num_updates")	uint		numUpdates = 0;
		@optional @name("location")		string		location = "48.2082,16.3738";
        @optional @name("units")        string      units = "si";
	}

	this(ref string[] args)
	{
		this.exePath = std.path.dirName(std.file.thisExePath());
		version(Windows) {
			this.homeDir = std.process.environment["APPDATA"];
			homeDir = buildPath(this.homeDir, "darksky-d");
		}
		else {
			this.homeDir = std.process.environment["HOME"];
            this.configDir = buildPath(this.homeDir, ".config/darksky-d");
			this.homeDir = buildPath(this.homeDir, ".local/share/darksky-d");
		}       

        try {
			std.file.isDir(this.homeDir);
		}
		catch (FileException e) {
			std.file.mkdirRecurse(this.homeDir);
		}

        try {
            std.file.isDir(this.configDir);
        }
        catch (FileException e) {
            std.file.mkdirRecurse(this.configDir);
        }
		if(args != null && !this.isInitialized) 
			this.initContext(args);	
	}

	~this()
	{
		this.saveConfig();
	}
    // TLS flag, each thread has its own
	static bool 	isInstantiated = false;
	// "True" global
	__gshared 		GlobalContext instance_;
	string      	homeDir;
    string          configDir;
	bool        	isInitialized = false;
	bool        	isPortable = false;
	string      	portableDir;
	string      	exePath;
	string      	configFilePath;
	string			cacheFileName, prettyCacheFileName, dbFileName;
public:
	/**
	 * the configuration object
	 */
	Config      		cfg;
	char[]				apiJson;

	static GlobalContext getInstance(string[] args = null)
	{
		if (!isInstantiated) {
			synchronized (GlobalContext.classinfo) {
				if (!instance_) {
					instance_ = new GlobalContext(args);
				}
				isInstantiated = true;
			}
		}
		return instance_;
	}

	/**
	 * initialize the context, parse command line arguments, determine data
	 * directory
	 */
	void initContext(ref string[] args) {
		if(this.isInitialized)
			return;

		this.isInitialized = true;

		if(this.isPortable && this.portableDir.length == 0)
			this.portableDir = ".darksky-d.data";

		if(this.portableDir.length > 0) {
			this.isPortable = true;
			if(!std.path.isAbsolute(this.portableDir))
				this.homeDir = buildPath(this.exePath, this.portableDir);
			else
				this.homeDir = this.portableDir;

			try {
				std.file.isDir(this.homeDir);
			} catch (FileException e) {
				std.file.mkdirRecurse(this.homeDir);
			}
			
		}
		this.configFilePath = buildPath(this.configDir, "config.json");
		try {
			std.file.isFile(this.configFilePath);
			string rawJson = std.file.readText(this.configFilePath);
			try
				this.cfg = deserializeJson!Config(rawJson);
			catch (Throwable e) {
				writef("exception while deserialize\n%s\n", e);
			}
		} catch(FileException e) {
			File f = File(this.configFilePath, "w");
			this.cfg = Config();
			this.cfg.firstRun = Clock.currTime();
			Json s = cfg.serializeToJson();
			f.write(s.toPrettyString());
			f.close();
			destroy(s);
		}   
		cfg.dataDir = this.homeDir;
		this.cacheFileName = buildPath(this.homeDir, "darksky_response.json");
        this.prettyCacheFileName = buildPath(this.homeDir, "darksky_pretty_response.json");
        this.dbFileName = buildPath(this.homedir, "history.sqlite3");

        DB db = DB.getInstance(this.dbFileName);
	}

	/**
	 * preshutdown
	 */
	void orderlyShutDown(const int code = 0) {
		this.saveConfig();
		exit(code);
	}

	/**
	 * save configuration
	 */
	void saveConfig() {
		this.cfg.lastRun = Clock.currTime();
		this.cfg.isPortable = this.isPortable;
		Json s = this.cfg.serializeToJson();
		try {
			File f = File(this.configFilePath, "w");
			f.write(s.toPrettyString());
			f.close();
		} catch (FileException e) {
		}
	}

	/**
	 * the data directory
	 */
	@property homedir() const { return this.homeDir; }
    @property configdir() const { return this.configDir; }
	@property cachefile() const { return this.cacheFileName; }
    @property prettycachefile() const { return this.prettyCacheFileName; }
}
