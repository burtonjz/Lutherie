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

#include "platform/AppPaths.hpp"
#include <spdlog/spdlog.h>

#ifdef __APPLE__
    #include <mach-o/dyld.h>
#endif

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
#endif

fs::path getExecutableDir(){
#ifdef __linux__
    char result[PATH_MAX];

    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    SPDLOG_DEBUG("reading path to current executable...");

    if ( count == -1 ) {
        throw std::runtime_error("Could not determine executable path");
    }

    fs::path p(std::string(result, count));

    SPDLOG_DEBUG("executable directory is {}", p.parent_path().c_str());
    return p.parent_path();

#elif defined(__APPLE__)

    char result[PATH_MAX];
    uint32_t size = sizeof(result);

    if ( _NSGetExecutablePath(result, &size) != 0 )
        throw std::runtime_error("Could not determine executable path");

    auto p = fs::canonical(result);

    SPDLOG_DEBUG("executable directory is {}", p.parent_path());
    return p.parent_path();

#elif defined(_WIN32)

    wchar_t result[MAX_PATH];

    DWORD size = GetModuleFileNameW(
        nullptr,
        result,
        MAX_PATH
    );

    if (size == 0) {
        throw std::runtime_error("Could not determine executable path");
    }

    // Handle paths longer than MAX_PATH
    if (size == MAX_PATH) {
        std::vector<wchar_t> buffer(32768);

        size = GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size())
        );

        if (size == 0 || size == buffer.size()) {
            throw std::runtime_error("Could not determine executable path");
        }

        fs::path p(std::wstring(buffer.data(), size));

        SPDLOG_DEBUG("executable directory is {}", p.parent_path());
        return p.parent_path();
    }

    fs::path p(std::wstring(result, size));

    SPDLOG_DEBUG("executable directory is {}", p.parent_path());
    return p.parent_path();

#else

    throw std::runtime_error("Unsupported platform");

#endif
}

fs::path AppPaths::getAppResourceDir(){
#ifdef DEBUG_BUILD
    return fs::path(LUTHERIE_SOURCE_DIR) / "shared" / "resources" ;
#endif 

    auto resource = fs::path(LUTHERIE_RESOURCE_DIR);

    if ( !fs::exists(resource) ){
        throw std::runtime_error("resource directory does not exist");
    }

    return resource ;
}

fs::path AppPaths::getUserConfigDir(){
#ifdef DEBUG_BUILD
    return fs::path(LUTHERIE_SOURCE_DIR) / "build" / "dev" / "resources" / "config" ;
#endif 

#ifdef __linux__
    const char* xdg = std::getenv("XDG_AppPaths_HOME");
    if ( xdg ){
        return fs::path(xdg) / "lutherie" ;
    }

    const char* home = std::getenv("HOME");
    if ( !home ) {
        throw std::runtime_error("Could not determine home directory");
    }

    return fs::path(home) / ".AppPaths" / "lutherie" ;

#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");

    if ( !home ){
        throw std::runtime_error("Could not determine home directory");
    }

    return fs::path(home) / "Library" / "Application Support" / "Lutherie" ;

#elif defined(_WIN32)
    wchar_t* appData = nullptr ;
    if ( 
        SHGetKnownFolderPath(
            FOLDERID_RoamingAppData,
            0, nullptr, &appData
        ) != S_OK
    ){
        throw std::runtime_error("Could not determine AppData directory");
    }

    fs::path result(appData);
    CoTaskMemFree(appData);
    return result / "Lutherie" ;

#else
    throw std::runtime_error("Unsupported platform");
#endif
}

fs::path AppPaths::getUserDataDir(){
#ifdef DEBUG_BUILD
    return fs::path(LUTHERIE_SOURCE_DIR) / "build" / "dev" / "resources" / "data" ;
#endif 

#ifdef __linux__
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if ( xdg ){
        return fs::path(xdg) / "lutherie" ;
    }

    const char* home = std::getenv("HOME");

    if ( !home ){
        throw std::runtime_error("Could not determine home directory");
    }
    return fs::path(home) / ".local" / "share" / "lutherie" ;

#elif defined(__APPLE__)
    // no AppPaths/data distinction
    return getUserAppPathsDir();

#elif defined(_WIN32)
    wchar_t* localAppData = nullptr ;
    if (
        SHGetKnownFolderPath(
            FOLDERID_LocalAppData,
            0,
            nullptr,
            &localAppData
        ) != S_OK
    ){
        throw std::runtime_error("Could not determine LocalAppData directory");
    }
    fs::path result(localAppData);
    CoTaskMemFree(localAppData);
    return result / "Lutherie" ;

#else
    throw std::runtime_error("Unsupported platform");
#endif
}


fs::path AppPaths::getUserCacheDir(){
#ifdef DEBUG_BUILD
    return fs::path(LUTHERIE_SOURCE_DIR) / "build" / "dev" / "resources" / "cache" ;
#endif 

#ifdef __linux__
    const char* xdg = std::getenv("XDG_CACHE_HOME");
    if ( xdg ){
        return fs::path(xdg) / "lutherie" ;
    }

    const char* home = std::getenv("HOME");
    if ( !home ){
        throw std::runtime_error("Could not determine home directory");
    }

    return fs::path(home) / ".cache" / "lutherie" ;

#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if ( !home ){
        throw std::runtime_error("Could not determine home directory");
    }

    return fs::path(home) / "Library" / "Caches" / "Lutherie" ;

#elif defined(_WIN32)
    wchar_t* localAppData = nullptr;
    if (
        SHGetKnownFolderPath(
            FOLDERID_LocalAppData,
            0,
            nullptr,
            &localAppData
        ) != S_OK
    ){
        throw std::runtime_error("Could not determine LocalAppData directory");
    }

    fs::path result(localAppData);
    CoTaskMemFree(localAppData);
    return result / "Lutherie" / "Cache";

#else
    throw std::runtime_error("Unsupported platform");
#endif
}