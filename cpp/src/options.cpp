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

#define __OPTIONS_INTERNAL

ProgramOptions::ProgramOptions() : m_oCommand(),
                                   m_config{
                                     .temp_unit = 'C', .config_dir_path = "",
                                     .apikey = "MY_API_KEY", .data_url="http://foobar.org",
                                     .vis_unit = "km", .speed_unit = "km/h",
                                     .output_dir = ""
                                   }
{
    this->_init();
}

void ProgramOptions::_init()
{
    m_oCommand.add_flag("--version,-V", "Show program version");
    m_oCommand.add_option("--apikey,-a", this->m_config.apikey, "Set the API key");
}

/**
 * write options to configuration file
 */
void ProgramOptions::flush()
{
    std::string filename;
}
/**
 * parse command line and a config file (if it does exist)
 *
 * @param argc command line argument count (from main())
 * @param argv list of arguments
 *
 * @return  0 if --help was requested -> print help and exit
 *          1 if --version was requested -> print version and exit
 *          2 otherwise.
 */
int ProgramOptions::parse(int argc, char **argv)
{
    CLI11_PARSE(this->m_oCommand, argc, argv);

    //po::positional_options_description p;
    const gchar *datadir = g_get_user_data_dir();
    const gchar *homedir = g_get_home_dir();
    const gchar *cfgdir = g_get_user_config_dir();
    std::string tmp(cfgdir);
    tmp.append("/objctest/config.toml");
    this->m_config.config_file_path.assign(tmp);
    this->m_config.data_dir_path.assign(datadir);
    this->m_config.data_dir_path.append("/climacell");

    this->logfile_path.assign(this->m_config.data_dir_path);
    this->logfile_path.append("/log.log");
    loguru::add_file(this->logfile_path.c_str(), loguru::Append, loguru::Verbosity_MAX);

    std::string p(this->getConfig().data_dir_path);
    p.append("/cache");
    fs::path path(p);
    std::error_code ec;

    if (bool res = fs::create_directories(path, ec)) {
        std::cout << "the error code was: " << ec << std::endl;
        if (0 == ec.value()) {
            fs::permissions(path, fs::perms::owner_all, fs::perm_options::replace);
            fs::permissions(path.parent_path(), fs::perms::owner_all, fs::perm_options::replace);
        }
    } else {
        std::cout << "The data dir already exists, error code: " << ec << std::endl;
    }

    return this->m_oCommand.get_option("--version")->count() ? 1 : 2;
}
