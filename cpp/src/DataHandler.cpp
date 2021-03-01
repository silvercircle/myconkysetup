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

DataHandler::DataHandler() : m_options{ProgramOptions::getInstance()}
{
    
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
