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
#include <rtmidi/RtMidi.h>
#include <rtaudio/RtAudio.h>

#include "midi/MidiEventHandler.hpp"
#include "requests/ConnectionRequest.hpp"
#include "core/ComponentManager.hpp"
#include "core/ComponentFactory.hpp"

#include <nlohmann/json.hpp> 
#include <filesystem>

using json = nlohmann::json ;
namespace fs = std::filesystem ;

class Engine {
private:
    // Thread entry points
    void midiLoop();
    void audioLoop();
    
    // Setup/teardown
    void setup();
    void destroy();
    void stopAudio();
    void stopMidi();
    static void audioCleanup(RtAudio* dac, RtAudioErrorType error);
    
    // Thread management
    std::thread controlApiThread_ ;
    std::thread dataApiThread_ ;
    std::thread streamApiThread_ ;
    std::thread midiThread_ ;
    std::thread audioThread_ ;
    
    
    // Thread control flags (atomic for thread-safe access)
    std::atomic<bool> controlApiRunning_ ;
    std::atomic<bool> dataApiRunning_ ;
    std::atomic<bool> streamApiRunning_ ;
    
    std::atomic<bool> engineRunning_ ;
    std::atomic<bool> midiRunning_ ;
    std::atomic<bool> audioRunning_ ;
    std::mutex stateMutex_ ;
    
    RtAudio dac_ ;

    bool audioSet_ ;
    std::vector<RtAudio::DeviceInfo> availableAudioDevices_ ;
    uint32_t selectedAudioOutput_ ;
    
    bool midiSet_ ;
    RtMidiIn midiIn_ ;
    std::map<int, std::string> availableMidiDevices_ ;
    int selectedMidiPort_ ;
    
    MidiEventHandler midiDefaultHandler_ ;
    
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
    
    // Audio callback
    static int audioCallback(
        void *outputBuffer, 
        void *inputBuffer, 
        unsigned int nBufferFrames, 
        double streamTime, 
        RtAudioStreamStatus status, 
        void *userData
    );
    
    RtAudio* getDac();
    RtMidiIn* getMidiIn();
    MidiEventHandler* getDefaultMidiHandler();
    int getSampleRate() const ;
    
    const std::vector<RtAudio::DeviceInfo> getAvailableAudioDevices() const ;
    uint32_t getAudioDeviceId() const ;
    bool setAudioDeviceId(uint32_t deviceId, bool preferred = false);

    const std::map<int,std::string> getAvailableMidiDevices() const ;
    int getMidiDeviceId() const ;
    bool setMidiDeviceId(int deviceId, bool preferred = false);
    
    // Connection Management
    bool handleMidiConnection(ConnectionRequest connection);
    bool registerBaseMidiHandler(MidiEventHandler* handler);
    bool unregisterBaseMidiHandler(MidiEventHandler* handler);

    std::vector<ConnectionRequest> getComponentConnections(ComponentId id) const ;
    void getComponentConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const ;

    bool handleSignalConnection(ConnectionRequest request);
    bool handleModulationConnection(ConnectionRequest request);
    bool handleBufferConnection(ConnectionRequest request);

    json serialize() const ;

private:
    void getPeripheralConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const ;

    void initializeUserResourceFolder();
    json readJsonFile(const fs::path& path);

};

#endif // __ENGINE_HPP_