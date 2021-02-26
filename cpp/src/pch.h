//
// Created by alex on 23.02.21.
//

#ifndef OBJCTEST_PCH_H
#define OBJCTEST_PCH_H

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <glib-2.0/glib-unix.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"
#include "nlohmann/json.hpp"
#include "loguru/loguru.hpp"
#include "conf.h"
#include "options.h"
#include "DataHandler.h"

namespace fs = std::filesystem;
using namespace nlohmann;

#endif //OBJCTEST_PCH_H
