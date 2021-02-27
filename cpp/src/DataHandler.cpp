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
#include <codecvt>

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
      {7102, "uu"},                 {8000, "kK"} }
{
    this->db_path.assign(m_options.getConfig().data_dir_path);
    this->db_path.append("/DB.sqlite3");
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
        this->output();
        return 1;
    }
    return 0;
}

void DataHandler::output()
{
    using namespace std::chrono;

    std::time_t t = system_clock::to_time_t(system_clock::now());
    bool ec;
    time_t _t = time(0);

    std::stringstream foo;
    foo << std::put_time( std::localtime( &t ), "%FT%T%z" );

    std::cout << "** Begin output  " << foo.str() << " **" << std::endl;

    //std::cout << t1 << std::endl;

    nlohmann::json& cur = this->result_current["data"]["timelines"][0]["intervals"][0]["values"];
    nlohmann::json& forc = result_forecast["data"]["timelines"][0]["intervals"][0]["values"];

    time_t sunset_time = utils::IsoToUnixtime(forc["sunsetTime"].get<std::string>(), 0);
    time_t sunrise_time = utils::IsoToUnixtime(forc["sunriseTime"].get<std::string>(), 0);

    time_t now = time(0);
    bool is_daylight =   (now > sunrise_time && now < sunset_time);
    printf("%c\n", this->getCode(cur["weatherCode"].get<int>(), is_daylight));
    this->outputTemperature(cur["temperature"], true);

    this->outputDailyForecast(result_forecast["data"]["timelines"][0]["intervals"][1]["values"], true);
    this->outputDailyForecast(result_forecast["data"]["timelines"][0]["intervals"][2]["values"], true);
    this->outputDailyForecast(result_forecast["data"]["timelines"][0]["intervals"][3]["values"], true);
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
std::pair<std::string, std::string> DataHandler::degToBearing(unsigned int wind_direction)
{
    wind_direction = (wind_direction > 360) ? 0 : wind_direction;
    size_t _val = (size_t) (((double) wind_direction / 22.5) + 0.5);
    std::string retval;
    retval.assign(DataHandler::wind_directions[_val % 16]);
    return std::pair<std::string, std::string>(retval, std::string(DataHandler::speed_units[0]));
}

/**
 * Convert a temperature from metric to imperial, depending on the user's preference.
 *
 * @param temp      - temperature in Celsius
 * @param unit      - destination unit (F or C are allowed)
 * @return          - temperature and its unit as a std::pair
 */
std::pair<double, char> DataHandler::convertTemperature(double temp, char unit)
{
    double converted_temp = temp;
    unit = (unit == 'C' || unit == 'F') ? unit : 'C';
    if (unit == 'F')
        converted_temp = (temp * (9.0 / 5.0)) + 32.0;

    return std::pair<double, char>(converted_temp, unit);
}

double DataHandler::convertVis(const double vis)
{
    return this->m_options.getConfig().vis_unit == "miles" ? vis / 1.609 : vis;
}

double DataHandler::convertWindspeed(double speed)
{
    const std::string& _unit = m_options.getConfig().speed_unit;
    if(_unit == "km")
        return speed * 3.6;
    else if(_unit == "mph")
        return speed * 2.237;
    else if(_unit == "knots")
        return speed * 1.944;
    else
        return speed;
}

// hPa > InHg
double DataHandler::convertPressure(double hPa)
{
    return m_options.getConfig().pressure_unit == "inhg" ? hPa / 33.863886666667 : hPa;
}

// output a single temperature value, assume float, but
// the Json can also return int.
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
    current.append("precipitationType,precipitationProbability,pressureSeaLevel,");
    current.append("humidity,dewPoint&timesteps=current&units=metric");

    std::string daily(baseurl);
    daily.append("&fields=weatherCode,temperatureMax,temperatureMin,sunriseTime,sunsetTime,");
    daily.append("precipitationType,precipitationProbability&timesteps=1d&startTime=");

    /*
     * figure out the startTime parameter for the forcast. It needs to be UTC and tomorrow
     */

    GDateTime *g = g_date_time_new_now_local();                  // now
    GDateTime *g_end = g_date_time_add_days(g, 5);     // +1 day

    char cl[128];
    snprintf(cl, 40, "%04d-%02d-%02dT06:00:00Z",
             g_date_time_get_year(g),
             g_date_time_get_month(g), g_date_time_get_day_of_month(g));

    std::string _tmp(cl);
    daily.append(_tmp);

    /*
     * calculate the end date, we need 5 days at max for our forecast
     */
    snprintf(cl, 40, "%04d-%02d-%02dT06:00:00Z",
             g_date_time_get_year(g_end),
             g_date_time_get_month(g_end), g_date_time_get_day_of_month(g_end));

    g_date_time_unref(g_end);
    g_date_time_unref(g);

    daily.append("&endTime=");
    _tmp.assign(cl);
    daily.append(_tmp);

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
    return (fSuccess && !this->result_current["data"].empty() && !this->result_forecast["data"].empty());
}

/**
 * Write the database entry, unless database recording is disabled
 *
 * @author alex (25.02.21)
 */
void DataHandler::writeToDB()
{
    sqlite3 *the_db = 0;
    char    *err = 0;
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

    sqlite3_close(the_db);
}
