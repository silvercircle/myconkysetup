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

#include <sqlite3.h>
#include <curl/curl.h>

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
DataHandler::DataHandler() : m_options{ProgramOptions::getInstance()}
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
        if(!this->read_from_cache()) {
            LOG_F(INFO, "run() Reading from cache failed, giving up.");
            return -1;
        }
    } else {
        LOG_F(INFO, "DataHandler::run(): --offline not specified, attemptingn to fetch from API");
        this->read_from_api();
    }
    if(!this->result_current["data"].empty() && !this->result_forecast["data"].empty()) {
        LOG_F(INFO, "run() - valid data, beginning output");
        return 1;
    }
    return 0;
}

/**
 * convert a wind bearing in degrees into a human-readable form (i.e. "SW" for
 * a south-westerly wind).
 *
 * @param wind_direction    - wind bearing in degrees
 * @return                  - wind direction and speed unit as a std::pair
 */
std::pair<std::string, std::string> DataHandler::deg_to_bearing(unsigned int wind_direction)
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
std::pair<double, char> DataHandler::convert_temperature(double temp, char unit)
{
    double converted_temp = temp;
    unit = (unit == 'C' || unit == 'F') ? unit : 'C';
    if (unit == 'F')
        converted_temp = (temp * (9.0 / 5.0)) + 32.0;

    return std::pair<double, char>(converted_temp, unit);
}

double DataHandler::convert_vis(const double vis)
{
    return this->m_options.getConfig().vis_unit == "miles" ? vis / 1.609 : vis;
}

/**
 * attempt to read current and forecast data from cached JSON
 *
 * @return  true if successful, false otherwise.
 */
bool DataHandler::read_from_cache()
{
    std::string path(m_options.getConfig().data_dir_path);
    path.append(ProgramOptions::_current_cache_file);
    LOG_F(INFO, "Attempting to read current from cache: %s", path.c_str());
    std::ifstream current(path);
    std::stringstream current_buffer, forecast_buffer;
    current_buffer << current.rdbuf();
    current_buffer.seekg(0, std::ios::end);
    current.close();
    this->result_current = json::parse(current_buffer.str().c_str());

    path.assign(m_options.getConfig().data_dir_path);
    path.append(ProgramOptions::_forecast_cache_file);
    LOG_F(INFO, "Attempting to read forecast from cache: %s", path.c_str());

    std::ifstream forecast(path);
    forecast_buffer << forecast.rdbuf();
    forecast_buffer.seekg(0, std::ios::end);
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
 * unlike darksky, which allowed for a single-call request with all data
 * ClimaCell does not. We need to perform two requests. One detail request for the
 * current weather, and one forecast request to get data for the next 3-5 days.
 *
 * @return      - true if successful.
 */
bool DataHandler::read_from_api()
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
    daily.append("precipitationType,precipitationProbability&timesteps=1d");

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
                path.append(ProgramOptions::_forecast_cache_file);
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
void DataHandler::write_to_db()
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
