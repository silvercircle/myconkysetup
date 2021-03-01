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
 */

#include "utils.h"

DataHandler::DataHandler() : m_options{ProgramOptions::getInstance()},
                             m_DataPoint { .valid = false }
{
    this->db_path.assign(m_options.getConfig().data_dir_path);
    this->db_path.append("/history.sqlite3");
    LOG_F(INFO, "Database path: %s", this->db_path.c_str());
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
    retval.assign(DataHandler_ImplClimaCell::wind_directions[_val % 16]);
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
    char unit[5] = "\xc2\xB0X";    // UTF-8!! c2b0 is the utf8 sequence for ° (degree symbol)
    char res[129];

    auto result = this->convertTemperature(val, this->m_options.getConfig().temp_unit).first;

    unit[2] = this->m_options.getConfig().temp_unit;

    printf("%.1f%s\n", result, addUnit ? unit : "");
}

void DataHandler::doOutput()
{
    const CFG& cfg = this->m_options.getConfig();

    if(cfg.silent)
        return;         // no stdout output.

    std::cout << "** Begin output **" << std::endl;

    printf("%c\n", m_DataPoint.weatherSymbol);
    this->outputTemperature(m_DataPoint.temperature, true);

    /*
     * The 3 days of forecast
     */
    this->outputDailyForecast(false);
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
    printf("%s\n", m_DataPoint.timeRecordedAsText);                                 // 26
    printf("%s\n", m_DataPoint.conditionAsString);                                  // 27
    printf("%s\n", cfg.timezone.c_str());                                           // 28
    outputTemperature(m_DataPoint.temperatureMin, true);		                    // 29
    outputTemperature(m_DataPoint.temperatureMax, true);		                    // 30
    printf("** end data **\n");                                          		    // 31
}

/**
 * output low/high temperature and condition "icon" for one day in the
 * forecast
 *
 * @param day           the JSON data for the forecast day to process
 * param  is_daylight   whether we should use the day or night weather symbol
 */
void DataHandler::outputDailyForecast(const bool is_day)

{
    for (int i = 0; i < 3; i++) {
        printf("%c\n", this->m_daily[i].code);
        outputTemperature(this->m_daily[i].temperatureMin, false, "%.0f%s\n");
        outputTemperature(this->m_daily[i].temperatureMax, false, "%.0f%s\n");
        printf("%s\n", this->m_daily[i].weekDay);
    }
}

// TODO - this is incomplete
void DataHandler::dumpSnapshot()
{
    DataPoint& p = this->m_DataPoint;

    printf("Temp: %f - Feels: %f\n", p.temperature, p.temperatureApparent);
    printf("Sunrise: %s - Sunset: %s\n", p.sunriseTimeAsString, p.sunsetTimeAsString);
    printf("Pressure: %f - Humidity: %f\n", p.pressureSeaLevel, p.humidity);
    printf("Code: %d, Symbol: %c, Condition: %s\n", p.weatherCode, p.weatherSymbol, p.conditionAsString);
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

    rc = sqlite3_exec(the_db, sql, utils::sqlite_callback, 0, &err);
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
    sqlite3_close(the_db);
}
