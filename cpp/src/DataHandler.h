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

#ifndef OBJCTEST_SRC_DATAHANDLER_H_
#define OBJCTEST_SRC_DATAHANDLER_H_

#include <nlohmann/json.hpp>

typedef struct _DataPoint {
    bool            valid = false;
    bool            is_day;
    time_t          timeRecorded, sunsetTime, sunriseTime;
    char            timeRecordedAsText[10];
    char            timeZone[128];
    int             weatherCode;
    char            weatherSymbol;
    double          temperature, temperatureApparent, temperatureMin, temperatureMax;
    double          visibility;
    double          windSpeed, windGust;
    unsigned int    windDirection;
    int             precipitationType;
    char            precipitationTypeAsString[20];
    double          precipitationProbability, precipitationIntensity;
    double          pressureSeaLevel, humidity, dewPoint;
    char            sunsetTimeAsString[20], sunriseTimeAsString[20], windBearing[10], windUnit[10];
    char            conditionAsString[100];
} DataPoint;

class DataHandler {
  public:
    DataHandler();
    ~DataHandler() { writeToDB(); }

    bool readFromCache();
    bool readFromAPI();
    int  run();
    void doOutput();

    std::pair<std::string, std::string> degToBearing        (unsigned int wind_direction) const;
    std::pair<double, char>             convertTemperature  (double temp, char unit) const;
    double                              convertWindspeed    (double speed) const;
    double                              convertVis          (const double vis) const;
    double                              convertPressure     (double hPa) const;
    const char*                         getCondition        (int weatherCode);
    const char*                         getPrecipType       (int code) const;
    const DataPoint&                    getDataPoint        () const { return m_DataPoint; }

    void outputTemperature  (double val, const bool addUnit = false,
                             const char *format = "%.1f%s\n");
    void outputDailyForecast(const nlohmann::json& data, const bool is_day = true);
    char getCode            (const int weatherCode, const bool daylight = true);
    void populateSnapshot   ();

    static constexpr const char *wind_directions[] =
      {"N", "NNE", "NE",
       "ENE", "E", "ESE",
       "SE", "SSE", "S",
       "SSW", "SW", "WSW",
       "W", "WNW", "NW",
       "NNW"};

    static constexpr const char *speed_units[] = {"m/s", "kts", "km/h"};

    static constexpr const char *weekDays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
                                               "Sun", "_invalid"};

    static constexpr const char *precipType[] = { "", "(Rain)", "(Snow)", "(Freezing Rain)", "(Ice Pellets)" };
  private:
    ProgramOptions                  &m_options;
    nlohmann::json                  result_current, result_forecast;
    std::string                     db_path;
    std::map<int, const char *>     m_conditions;
    std::map<int, const char *>     m_icons;
    DataPoint                       m_DataPoint;

    void writeToDB();
};

#endif //OBJCTEST_SRC_DATAHANDLER_H_
