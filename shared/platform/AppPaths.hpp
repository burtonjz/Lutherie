/*
 * Copyright (C) 2026 Jared Burton
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

#ifndef APP_PATHS_HPP_
#define APP_PATHS_HPP_

#include <filesystem>

namespace fs = std::filesystem ;

class AppPaths {
public:
    static fs::path getAppResourceDir();
    static fs::path getUserConfigDir();
    static fs::path getUserDataDir();
    static fs::path getUserCacheDir();
};

#endif // APP_PATHS_HPP_