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

#include "core/Engine.hpp"
#include "config/Config.hpp"
#include "core/AudioEngine.hpp"
#include "core/MidiEngine.hpp"
#include "core/ComponentManager.hpp"
#include "api/StreamingApiHandler.hpp"
#include "api/ControlApiHandler.hpp"
#include "api/DataApiHandler.hpp"
#include "types/NodeType.hpp"
#include "types/SocketType.hpp"
#include "types/Waveform.hpp"
#include "meta/ComponentRegistry.hpp"
#include "dsp/detune.hpp"
#include "platform/AppPaths.hpp"
#include "signal/SignalController.hpp"


#include <chrono>
#include <csignal>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <thread>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include <fstream>

Engine* Engine::instance(){
    static Engine engine ;
    return &engine ;
}

std::atomic<bool> Engine::stop_flag{false};

void Engine::signalHandler(int signum){
    SPDLOG_INFO("Caught signal {}, stopping threads...", signum);
    stop_flag = true;
}

// Constructor
Engine::Engine():
    // thread state flags
    engineRunning_(false),
    controlApiRunning_(false),
    dataApiRunning_(false),
    streamApiRunning_(false),
    audioRunning_(false),
    midiRunning_(false)
{
    signal(SIGINT, Engine::signalHandler);
    dsp::initializeDetuneLUT();
}

// ============================================================================
// STATE FUNCTIONS
// ============================================================================

void Engine::initialize(){
    // AudioEngine::instance()->initialize();

    // Start API threads
    controlApiRunning_ = true ;
    controlApiThread_ = std::thread([&](){
        ControlApiHandler::instance()->start();
    });

    dataApiRunning_ = true ;
    dataApiThread_ = std::thread([&](){
        DataApiHandler::instance()->start();
    });    

    streamApiRunning_ = true ;
    streamApiThread_ = std::thread([&](){
        StreamingApiHandler::instance()->start();
    });

    initializeUserResourceFolder();

    // this thread just sleeps until we're done running
    while ( !stop_flag && controlApiRunning_ ){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Engine::run(){
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    if ( engineRunning_ ){
        SPDLOG_WARN("Engine already running!");
        return;
    }

    SPDLOG_INFO("Starting engine...");
    setup();
    
    engineRunning_ = true ;
    
    MidiEngine::instance()->start();
    AudioEngine::instance()->start();
    
    SPDLOG_INFO("Engine startup complete.");
}

void Engine::stop(){
    std::unique_lock<std::mutex> lock(stateMutex_);
    
    if (!engineRunning_){
        SPDLOG_WARN("Engine not running!");
        return;
    }
    
    SPDLOG_INFO("Stopping engine...");
    
    // Signal all threads to stop
    engineRunning_ = false ;
    
    // Unlock before joining to avoid deadlock
    lock.unlock();

    MidiEngine::instance()->stop();
    AudioEngine::instance()->stop();
    
    SPDLOG_INFO("Engine stopped");
}

void Engine::shutdown(){
    SPDLOG_INFO("Shutting down engine...");
    
    // Stop engine if running
    if ( engineRunning_ ) stop();
    
    // Stop API server
    stop_flag = true ;
    controlApiRunning_ = false ;
    dataApiRunning_ = false ;
    streamApiRunning_ = false ;
    
    if ( controlApiThread_.joinable() ){
        SPDLOG_INFO("Waiting for Control API thread...");
        controlApiThread_.join();
    }

    if ( dataApiThread_.joinable() ){
        SPDLOG_INFO("Waiting for Data API thread...");
        dataApiThread_.join();
    }

    if ( streamApiThread_.joinable() ){
        SPDLOG_INFO("Waiting for Streaming API thread...");
        streamApiThread_.join();
    }
    
    SPDLOG_INFO("Engine shutdown complete");
}

bool Engine::isRunning() const {
    return engineRunning_ ;
}

// ============================================================================
// CLEANUP FUNCTIONS
// ============================================================================

// ============================================================================
// SETUP AND TEARDOWN
// ============================================================================

void Engine::setup(){
    MidiController::instance()->initialize();
    SignalController::instance()->updateProcessingGraph();
}

void Engine::destroy(){
    ComponentManager::instance()->reset();
    SignalController::instance()->reset();
}

int Engine::getSampleRate() const {
    return sampleRate_ ;
}

json Engine::serialize() const {
    json output ;

    // COMPONENTS
    output["components"] = ComponentManager::instance()->serializeComponents();

    // CONNECTIONS
    std::vector<ConnectionRequest> connections ;
    for ( auto id : ComponentManager::instance()->getints() ){
        getComponentConnections(id, connections);
    }
    std::set<ConnectionRequest> unique(connections.begin(), connections.end());

    output["connections"];
    for ( auto r : unique ){
        output["connections"].push_back(r);
    }

    // MIDI CONTROLS
    output["midi_controls"] = MidiControlRouter::instance()->serialize();

    return output ;
}

void Engine::initializeUserResourceFolder(){
    auto appPatchDir = AppPaths::getAppResourceDir() / "data" / "patches" ;
    auto userPatchDir = AppPaths::getUserDataDir() / "patches" ;

    SPDLOG_INFO("checking user resources and adding any new standard patches...");

    int numAdded = 0 ;
    int numUpdated = 0 ;
    fs::create_directories(userPatchDir);
    for ( const auto& entry : fs::directory_iterator(appPatchDir) ){
        if ( entry.path().extension() != ".json" ) continue ;
        json appJson ;
        try {
            appJson = readJsonFile(entry.path());
        } catch ( const std::exception& e ){
            SPDLOG_ERROR("error opening {}: {}", entry.path().c_str(), e.what());
            continue ;
        }

        int version = 0 ;
        if ( appJson.contains("version") && appJson.at("version").is_number() ){
            version = appJson.contains("version");
        }

        auto userFile = userPatchDir / entry.path().filename() ;
        if ( !fs::exists(userFile) ){
            fs::copy(entry.path(), userFile);
            numAdded++ ;
            continue ;
        }
        
        json userJson ;
        try {
            userJson = readJsonFile(userFile);
        } catch ( const std::exception& e ){
            SPDLOG_ERROR("error opening {}: {}", userFile.c_str(), e.what());
            continue ;
        }
        
        if ( !userJson.contains("version") || !userJson.at("version").is_number() ){
            SPDLOG_WARN(
                "existing user patch '{}' has naming conflict with incoming template. Will not overwrite.",
                userFile.filename().c_str()
            );
            continue ;
        }
        if ( version > userJson.at("version") ){
            numUpdated++ ;
            fs::copy(entry.path(), userFile, fs::copy_options::overwrite_existing);
            continue ;
        }
    }
    SPDLOG_INFO("Done. Added: {}; Updated: {}", numAdded, numUpdated);
}

json Engine::readJsonFile(const fs::path& path){
    std::ifstream file(path)      ;
    if ( !file.is_open() ){
        if ( !file.is_open() ) {
            throw std::runtime_error(fmt::format("Could not open file: {}", path.c_str()));
        }
    }

    json out ;
    file >> out ;
    return out ;
}