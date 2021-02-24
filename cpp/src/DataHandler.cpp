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

#include "DataHandler.h"

DataHandler::DataHandler() : m_options{ ProgramOptions::getInstance() }
{
    std::cout << "DataHandler: " << m_options.getConfig().apikey << std::endl;
}

std::pair<std::string, std::string> DataHandler::deg_to_bearing (unsigned int wind_direction)
{
    wind_direction = (wind_direction > 360) ? 0 : wind_direction;
    size_t val = (size_t) (((double) wind_direction / 22.5) + 0.5);
    std::string retval (DataHandler::wind_directions[val % 16]);
    return std::pair<std::string, std::string> (retval, std::string (DataHandler::speed_units[0]));
}

std::pair<double, char> DataHandler::convert_temperature (double temp, char unit)
{
    double converted_temp = temp;
    unit = (unit == 'C' || unit == 'F') ? unit : 'C';
    if (unit == 'F')
        converted_temp = (temp * (9.0 / 5.0)) + 32.0;

    return std::pair<double, char> (converted_temp, unit);
}

double DataHandler::convert_vis(const double vis)
{
    return this->m_options.getConfig().vis_unit == "miles" ? vis / 1.609 : vis;
}
