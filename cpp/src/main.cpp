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
    loguru::g_stderr_verbosity = loguru::Verbosity_OFF;
    loguru::init(argc, argv);
    ProgramOptions &opt = ProgramOptions::getInstance();
    auto result = opt.parse(argc, argv);
    LOG_F(INFO, "main(): The result from ProgramOptions::parse() was: %d", result);
    // catch the help
    if (0 == result)
        exit(result);

    if (1 == result) {
        exit(0);
    }

    LOG_F(INFO, "Config file: %s", opt.getConfig().config_file_path.c_str());
    LOG_F(INFO, "Data dir : %s", opt.getConfig().data_dir_path.c_str());
    LOG_F(INFO, "Log file : %s", opt.getLogFilePath().c_str());

    DataHandler dh;
    dh.run();
}
