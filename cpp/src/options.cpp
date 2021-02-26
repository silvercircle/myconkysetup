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
                                     .output_dir = "", .location="", .timezone="",
                                     .offline = false, .nocache = false, .skipcache = false
                                   }
{
    this->_init();
}

void ProgramOptions::_init()
{
    m_oCommand.add_flag("--version,-V", "Show program version");
    m_oCommand.add_flag("--offline", this->m_config.offline, "No API request, used cached result only");
    m_oCommand.add_flag("--nocache", this->m_config.nocache, "Do not cache the result");
    m_oCommand.add_flag("--skipcache", this->m_config.skipcache, "Do not read from cached results, even when online request fails");

    m_oCommand.add_option("--apikey,-a", this->m_config.apikey, "Set the API key");

    // TODO location
    /*
     * locationi can be either a registered location id or a lat,lon format
     * e.g. 48.1222795,16.3347827
     */
    m_oCommand.add_option("--loc,-l", this->m_config.location, "Set the location");
    m_oCommand.add_option("--tz", this->m_config.timezone, "Set the time zone");
    m_oCommand.add_option("--ouput,-o", this->m_config.output_dir,
                          R"("Write result to this file instead of stdout. Must have write access)");
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
        LOG_F(INFO, "ProgramOptions::parse(): create_directories result: %d : %s", ec.value(), ec.message().c_str());
        if (0 == ec.value()) {
            fs::permissions(path, fs::perms::owner_all, fs::perm_options::replace);
            fs::permissions(path.parent_path(), fs::perms::owner_all, fs::perm_options::replace);
        }
    } else {
        LOG_F(INFO, "ProgramOptions::parse(): Could not create the data directories. Maybe no permission or they exist?");
        LOG_F(INFO, "ProgramOptions::parse(): Attempted to create: %s", path.c_str());
        LOG_F(INFO, "ProgramOptions::parse(): Error code: %d : %s", ec.value(), ec.message().c_str());
    }

    return this->m_oCommand.get_option("--version")->count() ? 1 : 2;
}
