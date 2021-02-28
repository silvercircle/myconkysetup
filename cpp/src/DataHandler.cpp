/*
 * MIT License
 *
 * Copyright (c) 2021 Alex Vie (silvercircle@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * DataHandler does the majority of the work. It reads data, builds the json and
 * generates the formatted output.
 */

#include <time.h>
#include <utils.h>
#include "DataHandler.h"

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    int i;
    for(i = 0; i<argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

/**
 * This callback is used by curl to store the data.
 *
 * @param contents  - the data chunk read
 * @param size      - the length of the chunk
 * @param nmemb
 * @param s         - user-supplied data (std::string *)
 * @return          - the total length of data read
 */
static size_t curl_callback(void *contents, size_t size, size_t nmemb, std::string *s)
{
    s->append((char *)contents);
    return s->length();
}

/**
 * c'tor for DataHandler. Sets up database path and dispatches
 * data reading, either from cache or from the network.
 */
DataHandler::DataHandler() : m_options{ProgramOptions::getInstance()},
    /*
     * map weatherCodes to weather conditions.
     */
    m_conditions {
      {1000, "Clear"},              {1001, "Cloudy"},
      {1100, "Mostly Clear"},       {1101, "Partly Cloudy"},
      {1102, "Mostly Cloudy"},      {2000, "Fog"},
      {2100, "Light Fog"},          {3000, "Light Wind"},
      {3001, "Wind"},               {3002, "Strong Wind"},
      {4000, "Drizzle"},            {4001, "Rain"},
      {4200, "Light Rain"},         {4201, "Heavy Rain"},
      {5000, "Snow"},               {5001, "Flurries"},
      {5100, "Light Snow"},         {5101, "Heavy Snow"},
      {6000, "Freezing Drizzle"},   {6001, "Freezing Rain"},
      {6200, "Light Freezing Rain"},{6201, "Heavy Freezing Rain"},
      {7000, "Ice Pellets"},        {7001, "Heavy Ice Pellets"},
      {7102, "Light Ice Pellets"},  {8000, "Thunderstorm"} },
   /*
    * The following table maps weatherCodes from the API to characters used by
    * the conkyweather font to display weather icons. The string is always 2
    * characters, the first one for the day icon, the 2nd for the night icon.
    */
    m_icons {
      {1000, "aA"},                 {1001, "ef"},
      {1100, "bB"},                 {1101, "cC"},
      {1102, "dD"},                 {2000, "00"},
      {2100, "77"},                 {3000, "99"},
      {3001, "99"},                 {3002, "23"},
      {4000, "xx"},                 {4001, "gG"},
      {4200, "gg"},                 {4201, "jj"},
      {5000, "oO"},                 {5001, "xx"},
      {5100, "oO"},                 {5101, "ww"},
      {6000, "xx"},                 {6001, "yy"},
      {6200, "ss"},                 {6201, "yy"},
      {7000, "uu"},                 {7001, "uu"},
      {7102, "uu"},                 {8000, "kK"} },

    m_DataPoint { .valid = false }
{
    this->db_path.assign(m_options.getConfig().data_dir_path);
    this->db_path.append("/history.sqlite3");
    LOG_F(INFO, "Database path: %s", this->db_path.c_str());
}

/**
 * This performs all the work.
 *
 * @author alex (25.02.21)
 */
int DataHandler::run()
{
    if(m_options.getConfig().offline) {
        LOG_F(INFO, "DataHandler::run(): Attempting to read from cache (--offline option present");
        if(!this->readFromCache()) {
            LOG_F(INFO, "run() Reading from cache failed, giving up.");
            return -1;
        }
    } else {
        LOG_F(INFO, "DataHandler::run(): --offline not specified, attemptingn to fetch from API");
        this->readFromAPI();
    }
    if(!this->result_current["data"].empty() && !this->result_forecast["data"].empty()) {
        LOG_F(INFO, "run() - valid data, beginning output");
        this->doOutput();
        return 1;
    }
    return 0;
}

void DataHandler::doOutput()
{
    using namespace std::chrono;

    const CFG& cfg = this->m_options.getConfig();

    std::cout << "** Begin output **" << std::endl;

    printf("%c\n", m_DataPoint.weatherSymbol);
    this->outputTemperature(m_DataPoint.temperature, true);

    /*
     * The 3 days of forecast
     */
    this->outputDailyForecast(result_forecast["data"]["timelines"][0]["intervals"][1]["values"], true);
    this->outputDailyForecast(result_forecast["data"]["timelines"][0]["intervals"][2]["values"], true);
    this->outputDailyForecast(result_forecast["data"]["timelines"][0]["intervals"][3]["values"], true);

    this->outputTemperature(m_DataPoint.temperatureApparent, true);                             // 16
    this->outputTemperature(m_DataPoint.dewPoint, true);                                        // 17
    printf("Humidity: %.1f\n", m_DataPoint.humidity);                                           // 18
    printf(cfg.pressure_unit == "hpa" ? "%.1f hPa\n" : "%.2f InHg\n",m_DataPoint.pressureSeaLevel);     // 19
    printf("%.1f %s\n", m_DataPoint.windSpeed, cfg.speed_unit.c_str());                                 // 20
    //printf("UV: %d\n", 0);      // TODO UV index                                                      // 21
    printf("Prec: %.0f %s\n", m_DataPoint.precipitationProbability,
      m_DataPoint.precipitationProbability > 0 ? m_DataPoint.precipitationTypeAsString : "");           // 21
    printf("%.1f %s\n", m_DataPoint.visibility, cfg.vis_unit.c_str());                                  // 22

    printf("%s\n", m_DataPoint.sunriseTimeAsString);                                                    // 23
    printf("%s\n", m_DataPoint.sunsetTimeAsString);                                                     // 24

    printf("%s\n", m_DataPoint.windBearing);                                                            // 25
    GDateTime *g = g_date_time_new_now_local();
    printf("%s\n", m_DataPoint.timeRecordedAsText);                                 // 26
    printf("%s\n", m_DataPoint.conditionAsString);                                  // 27
    printf("%s\n", cfg.timezone.c_str());                                           // 28
    outputTemperature(m_DataPoint.temperatureMin, true);		                    // 29
    outputTemperature(m_DataPoint.temperatureMax, true);		                    // 30
    printf("** end data **\n");                                          		    // 31
}

char DataHandler::getCode(const int weatherCode, const bool daylight)
{
    if(DataHandler::m_icons.find(weatherCode) != DataHandler::m_icons.end()) {
        return DataHandler::m_icons[weatherCode][daylight ? 0 : 1];
    }
    return 'a';
}
/**
 * convert a wind bearing in degrees into a human-readable form (i.e. "SW" for
 * a south-westerly wind).
 *
 * @param wind_direction    - wind bearing in degrees
 * @return                  - wind direction and speed unit as a std::pair
 */
std::pair<std::string, std::string> DataHandler::degToBearing(unsigned int wind_direction) const
{
    wind_direction = (wind_direction > 360) ? 0 : wind_direction;
    size_t _val = (size_t) (((double) wind_direction / 22.5) + 0.5);
    std::string retval;
    retval.assign(DataHandler::wind_directions[_val % 16]);
    return std::pair<std::string, std::string>(retval, this->m_options.getConfig().speed_unit);
}

/**
 * Convert a temperature from metric to imperial, depending on the user's preference.
 *
 * @param temp      - temperature in Celsius
 * @param unit      - destination unit (F or C are allowed)
 * @return          - temperature and its unit as a std::pair
 */
std::pair<double, char> DataHandler::convertTemperature(double temp, char unit) const
{
    double converted_temp = temp;
    unit = (unit == 'C' || unit == 'F') ? unit : 'C';
    if (unit == 'F')
        converted_temp = (temp * (9.0 / 5.0)) + 32.0;

    return std::pair<double, char>(converted_temp, unit);
}

double DataHandler::convertVis(const double vis) const
{
    return this->m_options.getConfig().vis_unit == "miles" ? vis / 1.609 : vis;
}

double DataHandler::convertWindspeed(double speed) const
{
    const std::string& _unit = m_options.getConfig().speed_unit;
    if(_unit == "km/h")
        return speed * 3.6;
    else if(_unit == "mph")
        return speed * 2.237;
    else if(_unit == "knots")
        return speed * 1.944;
    else
        return speed;
}

// hPa > InHg
double DataHandler::convertPressure(double hPa) const
{
    return m_options.getConfig().pressure_unit == "inhg" ? hPa / 33.863886666667 : hPa;
}

/**
 * Output a single temperature value.
 *
 * @param val       temperature always in metric (Celsius)
 * @param addUnit   add the Unit (°C or °F)
 * @param format    use this format for output. See .h for defaults.
 */
void DataHandler::outputTemperature(double val, const bool addUnit, const char *format)
{
    char unit[5] = "\xc2\xB0X";         // UTF-8!! c2b0 is the sequence for ° (degree symbol)
    char res[129];

    auto result = this->convertTemperature(val, this->m_options.getConfig().temp_unit).first;

    unit[2] = this->m_options.getConfig().temp_unit;

    printf("%.1f%s\n", result, addUnit ? unit : "");
}

/**
 * output low/high temperature and condition "icon" for one day in the
 * forecast
 *
 * @param day           the JSON data for the forecast day to process
 * param  is_daylight   whether we should use the day or night weather symbol
 */
void DataHandler::outputDailyForecast(const nlohmann::json& data, const bool is_day)

{
    const int code = data["weatherCode"].get<int>();

    if(DataHandler::m_icons.find(code) != DataHandler::m_icons.end()) {
        printf("%c\n", DataHandler::m_icons[code][ is_day ? 0 : 1]);
    }
    else {
        printf("a\n");
    }

    outputTemperature(data["temperatureMin"].get<double>(), false, "%.0f%s\n");
    outputTemperature(data["temperatureMax"].get<double>(), false, "%.0f%s\n");

    GDateTime *g = g_date_time_new_from_iso8601(data["sunriseTime"].get<std::string>().c_str(), 0);
    gint weekday = g_date_time_get_day_of_week(g);
    g_date_time_unref(g);

    if(weekday >= 1 && weekday <= 7)
        printf("%s\n", DataHandler::weekDays[weekday - 1]);
    else
        printf("%s\n", DataHandler::weekDays[7]);       // print "invalid"
}


/**
 * attempt to read current and forecast data from cached JSON
 *
 * @return  true if successful, false otherwise.
 */
bool DataHandler::readFromCache()
{
    std::string path(m_options.getConfig().data_dir_path);
    path.append(ProgramOptions::_current_cache_file);
    LOG_F(INFO, "Attempting to read current from cache: %s", path.c_str());
    std::ifstream current(path);
    std::stringstream current_buffer, forecast_buffer;
    current_buffer << current.rdbuf();
    //current_buffer.seekg(0, std::ios::end);
    current.close();
    this->result_current = json::parse(current_buffer.str().c_str());

    path.assign(m_options.getConfig().data_dir_path);
    path.append(ProgramOptions::_forecast_cache_file);
    LOG_F(INFO, "Attempting to read forecast from cache: %s", path.c_str());

    std::ifstream forecast(path);
    forecast_buffer << forecast.rdbuf();
    //forecast_buffer.seekg(0, std::ios::end);
    forecast.close();
    this->result_forecast = json::parse(forecast_buffer.str());

    if(!result_current["data"].empty() && !result_forecast["data"].empty()) {
        LOG_F(INFO, "Cache read successful.");
        this->populateSnapshot();
        return true;
    }
    return false;
}

/**
 * fetch data from network API and populate the json object
 * unlike darksky, which allowed for a single-call request with all data,
 * ClimaCell does not. We need to perform two requests. One detail request for the
 * current weather, and one forecast request to get data for the next 3-5 days.
 *
 * @return      - true if successful.
 */
bool DataHandler::readFromAPI()
{
    bool fSuccess = true;
    std::string baseurl("https://data.climacell.co/v4/timelines?&apikey=");
    baseurl.append(m_options.getConfig().apikey);
    baseurl.append("&location=");
    baseurl.append(m_options.getConfig().location);
    if(m_options.getConfig().timezone.length() > 0) {
        baseurl.append("&timezone=");
        baseurl.append(m_options.getConfig().timezone);
    }
    std::string current(baseurl);
    current.append("&fields=weatherCode,temperature,temperatureApparent,visibility,windSpeed,windDirection,");
    current.append("precipitationType,precipitationProbability,pressureSeaLevel,windGust,");
    current.append("humidity,precipitationIntensity,dewPoint&timesteps=current&units=metric");
    //std::cout << current << std::endl;

    std::string daily(baseurl);
    daily.append("&fields=weatherCode,temperatureMax,temperatureMin,sunriseTime,sunsetTime,");
    daily.append("precipitationType,precipitationProbability&timesteps=1d&startTime=");

    /*
     * figure out the startTime parameter for the forcast. It needs to be UTC and tomorrow
     */

    GDateTime *g = g_date_time_new_now_local();                  // now

    char cl[128];
    snprintf(cl, 40, "%04d-%02d-%02dT23:00:00Z",
             g_date_time_get_year(g),
             g_date_time_get_month(g), g_date_time_get_day_of_month(g));

    std::string _tmp(cl);
    daily.append(_tmp);

    /*
     * calculate the end date, we need 5 days at max for our forecast
     */
    GDateTime *g_end = g_date_time_add_days(g, 5);     // 5 days forecast
    snprintf(cl, 40, "%04d-%02d-%02dT06:00:00Z",
             g_date_time_get_year(g_end),
             g_date_time_get_month(g_end), g_date_time_get_day_of_month(g_end));

    g_date_time_unref(g_end);
    g_date_time_unref(g);

    daily.append("&endTime=");
    _tmp.assign(cl);
    daily.append(_tmp);
    //std::cout << daily << std::endl;
    const char *url = current.c_str();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // read the current forecast first
    CURL *curl = curl_easy_init();
    if(curl) {
        std::string response;
        const char *url = current.c_str();
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        auto rc = curl_easy_perform(curl);
        if(rc != CURLE_OK) {
            LOG_F(INFO, "curl_easy_perform() failed, return = %s", curl_easy_strerror(rc));
        } else {
            this->result_current = json::parse(response.c_str());
            if(result_current["data"].empty()) {
                LOG_F(INFO, "Current forecast: Request failed, no valid data received");
            } else if(m_options.getConfig().nocache) {
                LOG_F(INFO, "Current forecast: Skipping cache refresh (--nocache option present)");
            } else {
                std::string path(m_options.getConfig().data_dir_path);
                path.append(ProgramOptions::_current_cache_file);
                std::ofstream f(path);
                f.write(response.c_str(), response.length());
                f.flush();
                f.close();
            }
        }
    }
    curl_easy_cleanup(curl);

    // now the daily forecast
    curl = curl_easy_init();
    if(curl) {
        std::string response;
        const char *url = daily.c_str();
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        auto rc = curl_easy_perform(curl);
        if(rc != CURLE_OK) {
            LOG_F(INFO, "curl_easy_perform() failed for forecast request, return = %s", curl_easy_strerror(rc));
        } else {
            this->result_forecast = json::parse(response.c_str());
            if(result_forecast["data"].empty()) {
                LOG_F(INFO, "Daily forecast: Request failed, no valid data received");
            } else if(m_options.getConfig().nocache) {
                LOG_F(INFO, "Daily forecast: Skipping cache refresh (--nocache option present)");
            } else {
                std::string path(m_options.getConfig().data_dir_path);
                path.append(ProgramOptions::_forecast_cache_file);
                std::ofstream f(path);
                f.write(response.c_str(), response.length());
                f.flush();
                f.close();
            }
        }
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    this->populateSnapshot();
    return (fSuccess && !this->result_current["data"].empty() && !this->result_forecast["data"].empty());
}

/**
 * Write the database entry, unless database recording is disabled
 *
 * @author alex (25.02.21)
 */
void DataHandler::writeToDB()
{
    sqlite3         *the_db = 0;
    sqlite3_stmt    *stmt = 0;
    char            *err = 0;
    DataPoint&      d = this->m_DataPoint;

    if(!m_DataPoint.valid)
        return;

    LOG_F(INFO, "Flushing DB, attemptint to open: %s", this->db_path.c_str());
    auto rc = sqlite3_open(this->db_path.c_str(), &the_db);
    if(rc) {
        LOG_F(INFO, "Unable to open the SQLite Database at %s. The error message was %s.",
              this->db_path.c_str(), sqlite3_errmsg(the_db));
        return;
    }
    LOG_F(INFO, "Database openend successfully");

    auto sql = R"(CREATE TABLE IF NOT EXISTS history
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
      )
    )";

    rc = sqlite3_exec(the_db, sql, callback, 0, &err);
    if(rc != SQLITE_OK) {
        LOG_F(INFO, "DB error when creating the table: %s", err);
        sqlite3_free(err);
    }

    rc = sqlite3_prepare_v2(the_db, "INSERT INTO history(timestamp, summary, icon, temperature,"
                                    "feelslike, dewpoint, windbearing, windspeed,"
                                    "windgust, humidity, visibility, pressure,"
                                    "precip_probability, precip_intensity, precip_type,"
                                    "uvindex, sunrise, sunset)"
                                    "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", -1, &stmt, 0);

    if(SQLITE_OK == rc) {
        DataPoint& p = this->m_DataPoint;       // shortcut
        char    tmp[10];
        LOG_F(INFO, "DataHandler::writeToDB(): sqlite3_prepare_v2() succeeded. Statement compiled");
        sqlite3_bind_int(stmt, 1, static_cast<int>(p.timeRecorded));
        sqlite3_bind_text(stmt, 2, p.conditionAsString, -1, 0);
        tmp[0] = p.weatherSymbol;
        sqlite3_bind_text(stmt, 3, tmp, -1, 0);
        sqlite3_bind_double(stmt, 4, p.temperature);
        sqlite3_bind_double(stmt, 5, p.temperatureApparent);
        sqlite3_bind_double(stmt, 6, p.dewPoint);
        sqlite3_bind_int(stmt, 7, p.windDirection);
        sqlite3_bind_double(stmt, 8, p.windSpeed);
        sqlite3_bind_double(stmt, 9, p.windGust);
        sqlite3_bind_double(stmt, 10, p.humidity);
        sqlite3_bind_double(stmt, 11, p.visibility);
        sqlite3_bind_double(stmt, 12, p.pressureSeaLevel);
        sqlite3_bind_double(stmt, 13, p.precipitationProbability);
        sqlite3_bind_double(stmt, 14, p.precipitationIntensity);
        sqlite3_bind_text(stmt, 15, p.precipitationTypeAsString, -1, 0);
        sqlite3_bind_int(stmt, 16, 0);              // TODO UVindex
        sqlite3_bind_int(stmt, 17, static_cast<int>(p.sunriseTime));
        sqlite3_bind_int(stmt, 18, static_cast<int>(p.sunsetTime));

        rc = sqlite3_step(stmt);
        if(SQLITE_OK == rc) {
            LOG_F(INFO, "DataHandler::writeToDB(): sqlite3_step() succeeded. Insert done.");
            rc = sqlite3_finalize(stmt);
        } else {
            LOG_F(INFO, "DataHandler::writeToDB(): sqlite3_step error: %s", sqlite3_errmsg(the_db));
        }

    }
    else {
        LOG_F(INFO, "DataHandler::writeToDB(): prepare stmt, error: %s", sqlite3_errmsg(the_db));
    }

    /*
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
     */
    //st.execute();
    //st.reset();

    sqlite3_close(the_db);
}

// TODO, safely populate a structure with all the data and strict error
// checking.
void DataHandler::populateSnapshot()
{
    // shortcuts
    nlohmann::json& d =     this->result_current["data"]["timelines"][0]["intervals"][0]["values"];
    nlohmann::json& df =    this->result_forecast["data"]["timelines"][0]["intervals"][0]["values"];
    DataPoint& p = this->m_DataPoint;
    const CFG& cfg = this->m_options.getConfig();
    char tmp[128];

    if(d["weatherCode"].empty())
        return; // datapoint likely not valid

    p.weatherCode = d["weatherCode"].is_number() ? d["weatherCode"].get<int>() : 0;
    snprintf(p.timeZone, 127, "%s", m_options.getConfig().timezone.c_str());

    p.timeRecorded = time(0);
    tm *now = localtime(&p.timeRecorded);
    snprintf(p.timeRecordedAsText, 19, "%02d:%02d", now->tm_hour, now->tm_min);

    p.dewPoint = d["dewPoint"].is_number() ?
      this->convertTemperature(d["dewPoint"].get<double>(), cfg.temp_unit).first : 0.0f;

    p.humidity = d["humidity"].is_number() ?
      d["humidity"].get<double>() : 0.0f;

    p.precipitationProbability = d["precipitationProbability"].is_number() ?
      d["precipitationProbability"].get<double>() : 0.0f;

    p.precipitationIntensity = d["precipitationIntensity"].is_number() ?
      d["precipitationIntensity"].get<double>() : 0.0f;

    p.temperature = d["temperature"].is_number() ?
      this->convertTemperature(d["temperature"].get<double>(), cfg.temp_unit).first : 0.0f;

    p.temperatureApparent = d["temperatureApparent"].is_number() ?
      this->convertTemperature(d["temperatureApparent"].get<double>(), cfg.temp_unit).first : 0;

    p.temperatureMin = df["temperatureMin"].is_number() ?
      this->convertTemperature(df["temperatureMin"].get<double>(),
        cfg.temp_unit).first : 0;

    p.temperatureMax = df["temperatureMax"].is_number() ?
      this->convertTemperature(df["temperatureMax"].get<double>(),
        cfg.temp_unit).first : 0;

    p.visibility = d["visibility"].is_number() ? this->convertVis(d["visibility"].get<double>()) : 0.0f;
    p.pressureSeaLevel = d["pressureSeaLevel"].is_number() ?
      this->convertPressure(d["pressureSeaLevel"].get<double>()) : 0.0f;

    p.windSpeed = d["windSpeed"].is_number() ? this->convertWindspeed(d["windSpeed"].get<double>()) : 0.0f;
    p.windDirection = d["windDirection"].is_number() ? d["windDirection"].get<int>() : 0;
    p.windGust = d["windGust"].is_number() ? this->convertWindspeed(d["windGust"].get<double>()) : 0.0f;

    auto wind = this->degToBearing(p.windDirection);
    snprintf(p.windBearing, 9, "%s", wind.first.c_str());
    snprintf(p.windUnit, 9, "%s", wind.second.c_str());

    p.sunsetTime = df["sunsetTime"].is_string() ?
      utils::ISOToUnixtime(df["sunsetTime"].get<std::string>(), 0) : 0;
    p.sunriseTime= df["sunriseTime"].is_string() ?
      utils::ISOToUnixtime(df["sunriseTime"].get<std::string>(), 0) : 0;

    p.is_day = (p.sunriseTime < p.timeRecorded < p.sunsetTime);

    tm *sunset = localtime(&p.sunsetTime);
    snprintf(tmp, 100, "%02d:%02d", sunset->tm_hour, sunset->tm_min);
    snprintf(p.sunsetTimeAsString, 19, "%s", tmp);
    tm *sunrise = localtime(&p.sunriseTime);
    snprintf(tmp, 100, "%02d:%02d", sunrise->tm_hour, sunrise->tm_min);
    snprintf(p.sunriseTimeAsString, 19, "%s", tmp);


    p.precipitationType = d["precipitationType"].is_number() ? d["precipitationType"].get<int>() : 0;
    snprintf(p.precipitationTypeAsString, 19, "%s", this->getPrecipType(p.precipitationType));
    snprintf(p.conditionAsString, 99, "%s", this->getCondition(p.weatherCode));

    p.weatherSymbol = this->getCode(p.weatherCode, p.is_day);

    /*
    printf("Temp: %f - Feels: %f\n", p.temperature, p.temperatureApparent);
    printf("Sunrise: %s - Sunset: %s\n", p.sunriseTimeAsString, p.sunsetTimeAsString);
    printf("Pressure: %f - Humidity: %f\n", p.pressureSeaLevel, p.humidity);
    printf("Code: %d, Symbol: %c, Condition: %s\n", p.weatherCode, p.weatherSymbol, p.conditionAsString);
     */

    p.valid = true;
    LOG_F(INFO, "DataHandler::populateSnapshot(): snapshot populated successfully.");
#if __clang_major__ >= 8
    //__builtin_dump_struct(&m_DataPoint, &printf);
#endif
}

const char *DataHandler::getCondition(int weatherCode)
{
    if(DataHandler::m_conditions.find(weatherCode) != DataHandler::m_icons.end()) {
        return DataHandler::m_conditions[weatherCode];
    }
    return "UNDEFINED";
}
const char *DataHandler::getPrecipType(int code) const
{
    code = (code > 4 || code < 0) ? 0 : code;
    return DataHandler::precipType[code];
}
