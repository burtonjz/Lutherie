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

#ifndef AUDIO_ENGINE_HPP_
#define AUDIO_ENGINE_HPP_

#include "requests/ConnectionRequest.hpp"
#include <cstdint>
#include <rtaudio/RtAudio.h>

// forward declarations
class AudioDevice ;

class AudioEngine {
private:
    AudioDevice* primary_ = nullptr ; // playback device, responsible for global sample rate
    std::vector<AudioDevice*> active_ ;
    std::vector<AudioDevice*> inputs_ ;

    std::unordered_map<uint32_t, RtAudio::DeviceInfo> available_ ;

    bool initialized_ = false ;
    bool running_ = false ;

    explicit AudioEngine();
    ~AudioEngine();

public:
    static AudioEngine* instance();

    AudioEngine(const AudioEngine&) = delete ;
    AudioEngine& operator=(const AudioEngine&) = delete ;
    AudioEngine(AudioEngine&&) = delete ;
    AudioEngine& operator=(AudioEngine&&) = delete ;

    // void initialize();

    void start();
    void stop();
    bool started() const ;

    // AudioDevice* getDevice(const ConnectionEndpoint& ep);

    inline auto getAvailableDevices();
    AudioDevice* getPrimaryDevice();
    const std::vector<AudioDevice*>& getInputDevices() const ;

    bool activateDevice(uint32_t id, bool preferred = false);
    bool deactivateDevice(uint32_t id);

    void getDeviceConnections(int id, std::vector<ConnectionRequest>& requests) const ;

    // callbacks based on device type
    static int duplexCallback(
        void* outputBuffer,
        void* inputBuffer,
        uint32_t nBufferFrames,
        double streamTime,
        RtAudioStreamStatus status,
        void* userData
    );

    static int playbackCallback(
        void* outputBuffer,
        void* inputBuffer,
        uint32_t nBufferFrames,
        double streamTime,
        RtAudioStreamStatus status,
        void* userData
    );

    static int recordCallback(
        void* outputBuffer,
        void* inputBuffer,
        uint32_t nBufferFrames,
        double streamTime,
        RtAudioStreamStatus status,
        void* userData
    );

    // callback helpers
    static void processSignal(size_t frames, int sampleRate);


private:
    bool prepare();

    void startup(AudioDevice* dev);
    void cleanup(AudioDevice* dev);
};


#endif // AUDIO_ENGINE_HPP_