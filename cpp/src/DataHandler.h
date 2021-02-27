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

class DataHandler {
  public:
    DataHandler();
    ~DataHandler() { writeToDB(this->result_current); }

    bool readFromCache();
    bool readFromAPI();
    int run();
    void output();

    /**
     * TODO: fix unit stuff
     */

    std::pair<std::string, std::string> degToBearing        (unsigned int wind_direction);
    std::pair<double, char>             convertTemperature  (double temp, char unit);
    double                              convertWindspeed    (double speed);
    double                              convertVis          (const double vis);
    double                              convertPressure     (double hPa);
    const char*                         getCondition        (int weatherCode);

    void outputTemperature  (double val, const bool addUnit = false,
                             const char *format = "%.1f%s\n");
    void outputDailyForecast(const nlohmann::json& data, const bool is_day = true);
    char getCode            (const int weatherCode, const bool daylight = true);
    void populateSnapshot   (const nlohmann::json& data);

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

  private:
    ProgramOptions                  &m_options;
    nlohmann::json                  result_current, result_forecast;
    std::string                     db_path;
    std::map<int, const char *>     m_conditions;
    std::map<int, const char *>     m_icons;

    void writeToDB(nlohmann::json& data);
};

#endif //OBJCTEST_SRC_DATAHANDLER_H_
