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

#ifndef OBJCTEST_OPTIONS_H
#define OBJCTEST_OPTIONS_H

#ifdef __OPTIONS_INTERNAL
#endif

typedef struct _cfg {
    int version;
    char        temp_unit;
    std::string config_dir_path;
    std::string config_file_path;
    std::string data_dir_path;
    std::string apikey;
    std::string data_url;
    std::string vis_unit;
    std::string speed_unit;
    std::string output_dir;
} CFG;

class ProgramOptions {
  public:
    ProgramOptions ();
    ProgramOptions (const ProgramOptions &) = delete;
    ProgramOptions &operator= (const ProgramOptions &) = delete;

    static ProgramOptions &getInstance ()
    {
        static ProgramOptions instance;
        volatile int x{};               // otherwise, -O3 might just optimize this away.
        return instance;
    }
    int parse (int argc, char **argv);
    void flush ();
    CFG &getConfig ()
    { return m_config; }

  private:
    CLI::App m_oCommand;
    //po::options_description m_oCommand, m_oFile;
    //po::variables_map       m_omap;
    std::string m_name;
    void _init ();
    unsigned int counter;
    CFG m_config;
};
#endif //OBJCTEST_OPTIONS_H
