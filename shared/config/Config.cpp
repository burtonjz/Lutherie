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

#include "config/Config.hpp"
#include "platform/AppPaths.hpp"

#include <fmt/format.h>
#include <fstream>
#include <linux/limits.h>
#include <string_view>
#include <mutex>
#include <unistd.h>
#include <spdlog/spdlog.h>

json Config::getTemplateConfig(){
    auto config = AppPaths::getAppResourceDir() / "config.json" ;

    std::ifstream file(config);
    if ( !file.is_open() ) {
        throw std::runtime_error(fmt::format("Could not open config file: {}", config.c_str()));
    }

    json out ;
    file >> out ;

    return out ;
}

json Config::configData_ ;
std::shared_mutex Config::mutex_ ;

void Config::load(){
    auto cfg = AppPaths::getUserConfigDir() / "config.json" ;

    // make sure user config directory exists
    if ( !fs::exists(cfg.parent_path()) ){
        fs::create_directories(cfg.parent_path());
    }

    // if the cfg file doesn't exist, let's copy the template
    if ( !fs::exists(cfg) ){
        SPDLOG_DEBUG(
            "user config path does not yet exist, loading data from template"
        );
        json temp = getTemplateConfig();
        {
            std::unique_lock lock(mutex_);
            configData_ = std::move(temp);
        }
        save(); // save immediately so we don't override pending changes with a second load call
        return ;
    }

    std::ifstream file(cfg);
    if ( !file.is_open() ) {
        throw std::runtime_error(fmt::format("Could not open config file: {}", cfg.c_str()));
    }

    json temp ;
    file >> temp ;

    std::unique_lock lock(mutex_);
    configData_ = std::move(temp);
}

void Config::save(){
    std::shared_lock readLock(mutex_);
    auto cfg = AppPaths::getUserConfigDir() / "config.json" ;
    std::ofstream file(cfg);
    if (!file.is_open()){
        throw std::runtime_error(fmt::format("Could not open config file: {}", cfg.c_str()));
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