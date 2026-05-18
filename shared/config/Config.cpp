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
#include <fstream>
#include "filesystem"
#include <linux/limits.h>
#include <string_view>
#include <mutex>
#include <unistd.h>

#ifdef __APPLE__
    #include <mach-o/dyld.h>
#endif

std::string getExecutableDir(){ // TODO: this only works for linux currently
    char result[PATH_MAX];
#ifdef __APPLE__
    uint32_t size = sizeof(result);
    if (_NSGetExecutablePath(result, &size) != 0) {
        return {};
    }
    return std::filesystem::path(std::string(result)).parent_path().string();
#else // LINUX
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count == -1) {
        return {};
    }
    return std::filesystem::path(std::string(result, count)).parent_path().string();
#endif
}

std::string Config::configPath_ = std::filesystem::path(getExecutableDir()) / ".." / "shared" / "config.json" ;
json Config::configData_ ;
std::shared_mutex Config::mutex_ ;

void Config::load(){
    std::ifstream file(configPath_);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + configPath_ );
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
        throw std::runtime_error("Could not open config file: " + configPath_ );
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