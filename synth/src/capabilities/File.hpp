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

 #ifndef FILE_HPP_
 #define FILE_HPP_

#include "interfaces/FileListener.hpp"

#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class File {
protected:
    std::filesystem::path path_ ;
    FileListener* listener_ ;
    
public:
    explicit File(FileListener* listener = nullptr):
        path_(""),
        listener_(listener)
    {}

    const std::filesystem::path& getPath() const {
        return path_ ;
    }

    FileResult setPath(const std::string& path){
        path_ = path ;
        if ( !std::filesystem::exists(path) ) return FileResult::NotFound ;
        if ( listener_ ) return listener_->onPathSet(path_);
        return FileResult::Ok ;
    }

    void toJson(json& out) const { 
        out["path"] = path_.string(); 
    }

    FileResult fromJson(const json& j){
        if ( !j.contains("path") ) return FileResult::Ok ;
        return setPath(j["path"].get<std::string>());
    }
};

 #endif // FILE_COMPONENT_HPP_