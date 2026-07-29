/*
 * Copyright (C) 2025 Jared Burton
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "Config.hpp"
#include <fmt/format.h>
#include <fstream>
#include "filesystem"
#include <linux/limits.h>
#include <string_view>
#include <mutex>
#include <unistd.h>
#include <spdlog/spdlog.h>

#ifdef __APPLE__
    #include <mach-o/dyld.h>
#endif

std::filesystem::path getExecutableDir(){
#ifdef __linux__
    char result[PATH_MAX];

    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    SPDLOG_DEBUG("reading path to current executable...");

    if ( count == -1 ){
        throw std::runtime_error("Could not determine executable path");
    }
    
    std::filesystem::path p(std::string(result, count));

    SPDLOG_DEBUG("executable directory is {}", p.parent_path());
    return p.parent_path();

#elif defined(__APPLE__)

    char result[PATH_MAX];
    uint32_t size = sizeof(result);

    if (_NSGetExecutablePath(result, &size) != 0)
        throw std::runtime_error("Could not determine executable path");

    return std::filesystem::canonical(result).parent_path();

#else

    throw std::runtime_error("Unsupported platform");

#endif
}

std::filesystem::path findConfig(){
    auto exe = getExecutableDir();

    // Development tree
    auto dev = exe / ".." / "shared" / "config.json";
    if (std::filesystem::exists(dev))
        return std::filesystem::canonical(dev);

    // Installed tree (configured by CMake)
    auto installed = std::filesystem::path(LUTHERIE_DATA_DIR) / "config.json";
    if (std::filesystem::exists(installed))
        return installed;

    throw std::runtime_error("Couldn't locate config.json");
}

std::filesystem::path Config::configPath_ = findConfig();
json Config::configData_ ;
std::shared_mutex Config::mutex_ ;

void Config::load(){
    std::ifstream file(configPath_);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Could not open config file: {}", configPath_.c_str()));
    }

    json temp;
    file >> temp;

    std::unique_lock lock(mutex_);
    configData_ = std::move(temp);
}

void Config::save(){
    std::shared_lock readLock(mutex_);
    std::ofstream file(configPath_);
    if (!file.is_open()){
        throw std::runtime_error(fmt::format("Could not open config file: {}", configPath_.c_str()));
    }
    file << configData_.dump(4);
}

void Config::set(const std::string& dottedKey, const json& value){
    std::unique_lock lock(mutex_);

    // parse dotted json key, e.g., server.port -> json['server']['port']
    std::string_view keyView = dottedKey ;
    json* jsonPtr = &configData_ ;

    while (!keyView.empty()) {
        size_t dotPos = keyView.find('.');
        std::string_view segment = keyView.substr(0,dotPos);

        if (dotPos == std::string_view::npos) {
            // all dots have been parsed, assign value
            (*jsonPtr)[std::string(segment)] = value ;
        } else {
            // not at end dot, set the JSON pointer to the next level up
            std::string keyStr(segment) ;
            jsonPtr = &(*jsonPtr)[keyStr] ; 
        }   
    }
}