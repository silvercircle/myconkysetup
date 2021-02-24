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

int main(int argc, char **argv)
{
    ProgramOptions& opt = ProgramOptions::getInstance();
    auto result = opt.parse(argc, argv);
    std::cout << "The result was " << result << std::endl;

    // catch the help
    if(0 == result)
      exit(result);

    if(1 == result) {
        exit(0);
    }

    std::cout << "Config file: " << opt.getConfig().config_file_path << std::endl;
    std::cout << "Data dir:" << opt.getConfig().data_dir_path << std::endl;
    std::cout << "APIKEY:" << opt.getConfig().apikey << std::endl;

    DataHandler dh;
    std::cout << "280 degrees wind is: " << dh.deg_to_bearing(280).first
              << " wind direction" << std::endl;

    std::cout << "38 degrees Celsius are " << dh.convert_temperature(38.0, 'F').first
              << " Fahrenheit" << std::endl;
    //smp::run();
}
