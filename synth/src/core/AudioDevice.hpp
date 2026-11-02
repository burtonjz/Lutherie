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

#ifndef AUDIO_DEVICE_HPP_
#define AUDIO_DEVICE_HPP_

#include <deque>
#include <rtaudio/RtAudio.h>
#include <cstdint>
#include <optional>
#include <samplerate.h>

#include "containers/LockFreeRingBuffer.hpp"
#include "types/ConnectionEndpoint.hpp"

class AudioDevice {
public:
    enum Type { DAC, ADC, ADAC };

private:
    Type type_ ;

    uint32_t bufferSize_ ;
    uint32_t sampleRate_ ;
    uint32_t globalRate_ ;

    bool matchesGlobalRate_ = true ;
    bool running_ = false ;
    size_t connectionCount = 0 ;
    
    std::vector<ConnectionEndpoint> inbound_ ;
    std::vector<ConnectionEndpoint> outbound_ ;
    std::deque<LockFreeRingBuffer<double>> outboundRings_ ;
    std::vector<std::vector<double>> outboundScratch_ ;

    RtAudio itf_ ;
    RtAudio::DeviceInfo info_ ;
    std::optional<RtAudio::StreamParameters> inputParams_ = std::nullopt ;
    std::optional<RtAudio::StreamParameters> outputParams_ = std::nullopt ;
    RtAudio::StreamOptions options_ ;

    double resampleRatio_ ;
    std::vector<float> resampleScratchInp_ ;
    std::vector<float> resampleScratchOut_ ;
    SRC_STATE* resampleSrc_ = nullptr ;

    static constexpr double maxResampleRatioDeviation = 0.005 ; // extra room in resample output based on real time sample deviations
    static constexpr size_t numResampleSlackFrames = 16 ; // extra samples for carrying t
public:
    AudioDevice(const RtAudio::DeviceInfo& info);

    RtAudio& interface();
    const RtAudio::DeviceInfo& info() const ;

    Type type() const ;
    void setType(Type typ);
    bool isInputDevice() const ;
    bool isOutputDevice() const ;

    uint32_t inboundChannels() const ;
    uint32_t outboundChannels() const ;

    size_t bufferSize() const ;
    size_t sampleRate() const ;
    bool isGlobalSampleRate() const ;

    bool hasConnection() const ;
    void addConnection();
    void removeConnection();

    const std::vector<ConnectionEndpoint>& outboundEndpoints() const ;
    const std::vector<ConnectionEndpoint>& inboundEndpoints() const ;

    LockFreeRingBuffer<double>& outboundBuffer(size_t channel);
    std::vector<double>& outboundScratch(size_t channel);
    

    void init();
    bool start(RtAudioCallback callback);
    void stop();
    bool running() const ;

    void initResample();

    // callback helpers
    size_t resample(const double* in, size_t frames, size_t nChannels);
    void pushToRingBuffer(const auto* data, size_t frames);
    

private:
    void setType(const RtAudio::DeviceInfo& info); 
    void setDefaultInputParams();
    void setDefaultOutputParams();
    void configureEndpoints();
};

#endif // AUDIO_DEVICE_HPP_