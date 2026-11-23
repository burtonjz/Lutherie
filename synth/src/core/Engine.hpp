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

#ifndef __ENGINE_HPP_
#define __ENGINE_HPP_

#include <atomic>
#include <map>
#include <mutex>

#include <rtmidi/RtMidi.h>
#include <rtaudio/RtAudio.h>

#include <nlohmann/json.hpp> 
#include <filesystem>
#include <thread>

using json = nlohmann::json ;

namespace fs = std::filesystem ;
class Engine {
private:
    // Thread management
    std::thread controlApiThread_ ;
    std::thread dataApiThread_ ;
    std::thread streamApiThread_ ;

    // Thread control flags (atomic for thread-safe access)
    std::atomic<bool> engineRunning_ ;
    std::atomic<bool> controlApiRunning_ ;
    std::atomic<bool> dataApiRunning_ ;
    std::atomic<bool> streamApiRunning_ ;
    std::mutex stateMutex_ ;
    
    double sampleRate_ ;

    explicit Engine();

public:
    static Engine* instance();

    Engine(const Engine&) = delete ;
    Engine& operator=(const Engine&) = delete ;
    Engine(Engine&&) = delete ;
    Engine& operator=(Engine&&) = delete ;
    
    // Main state functions
    void initialize();
    void run();
    void stop();
    void shutdown();
    bool isRunning() const ;
    
    // Static functions
    static void signalHandler(int signum);
    static std::atomic<bool> stop_flag;
    
    int getSampleRate() const ;


    json serialize() const ;

private:
    // Setup/teardown
    void setup();
    void destroy();

    void initializeUserResourceFolder();
    json readJsonFile(const fs::path& path);
};

#endif // __ENGINE_HPP_