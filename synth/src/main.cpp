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

#include "core/Engine.hpp"
#include "config/Config.hpp"

#include <rtaudio/RtAudio.h>
#include <rtmidi/RtMidi.h>
#include <spdlog/spdlog.h>

// Program Entry Point
int main() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
#ifdef DEBUG_BUILD
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
#endif
    SPDLOG_INFO("RtAudio version: " + RtAudio::getVersion());
    SPDLOG_INFO("RtMidi version: " + RtMidi::getVersion());
    Config::load();
    Engine::instance()->initialize();
    
    // cleanup
    Engine::instance()->shutdown();
}