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

#include "core/AudioEngine.hpp"
#include "core/AudioDevice.hpp"
#include "core/ComponentManager.hpp"
#include "signal/SignalController.hpp"
#include "config/Config.hpp"
#include "types/ConnectionEndpoint.hpp"
#include "types/EndpointType.hpp"

#include <spdlog/spdlog.h>


AudioEngine* AudioEngine::instance(){
    static AudioEngine engine ;
    return &engine ;
}

AudioEngine::AudioEngine():
    active_()
{}

AudioEngine::~AudioEngine(){
    stop();
    active_.clear();
}


// void AudioEngine::initialize(){
//     if ( initialized_ ){
//         SPDLOG_ERROR("audio engine already initialized");
//         return ;
//     }

//     initialized_ = true ;
// }

bool AudioEngine::prepare(){
    // set config based on primary device
    if ( !primary_ ){
        SPDLOG_ERROR("Cannot prepare audio engine: primary device not set.");
        return false ;
    } 
    Config::set("audio.sample_rate", primary_->sampleRate());
    
    for ( auto* dev : active_ ){
        dev->init();
    }

    return true ;
}

void AudioEngine::start(){
    if ( prepare() ){
        SPDLOG_ERROR("Cannot start engine. Prerequisites are not met.");
        return ;
    }

    for ( auto* dev : active_ ){
        switch(dev->type()){
        case AudioDevice::ADAC:
            dev->start(AudioEngine::duplexCallback);
            break ;
        case AudioDevice::DAC:
            dev->start(AudioEngine::playbackCallback);
            break ;
        case AudioDevice::ADC:
            dev->start(AudioEngine::playbackCallback);
            break ;
        default:
            break ;
        }
    }
}

void AudioEngine::stop(){
    for ( auto* dev : active_ ){
        dev->stop();
    }
}

bool AudioEngine::started() const {
    return running_ ;
}

bool AudioEngine::activateDevice(uint32_t id, bool preferred){
    if ( started() ){
        SPDLOG_ERROR("cannot activate new device while engine is running.");
        return false ;
    }

    if ( !available_.contains(id) ){
        SPDLOG_ERROR("cannot activate device id {}. Device unknown or unavailable.", id);
        return false ;
    }

    AudioDevice* dev = new AudioDevice(available_.at(id));

    // currently, only one output device supported
    // so that it can serve as our global sampling rate
    // (would need resampling for sinks in SignalController)
    if ( primary_ ){
        switch (dev->type()){
        case AudioDevice::DAC:
            SPDLOG_ERROR(
                "Cannot activate device '{}'. Primary output already set and device has no inputs",
                dev->info().name 
            );
            delete dev ;
            return false ;
        case AudioDevice::ADAC:
            SPDLOG_INFO(
                "Primary output already set. Device '{}' supports Duplex, but will be used for input only.",
                dev->info().name
            );
            dev->setType(AudioDevice::ADC);
            break ;
        default:
            break ;
        }
    } else {
        // register as primary device if device has inputs
        if ( dev->inboundChannels() > 0 ){
            primary_ = dev ;
        }   
    }
    
    active_.push_back(dev);

    // if ( preferred ){
    //     // TODO, let this support a list instead of single values.
    //     Config::set("audio.preferred_devices", deviceId);
    // }

    return true ;
}

bool AudioEngine::deactivateDevice(uint32_t id){
    if ( started() ){
        SPDLOG_ERROR("cannot deactivate device while engine is running.");
        return false ;
    }

    auto it = std::find_if(active_.begin(), active_.end(), [id](AudioDevice* a){
        return a->info().ID == id ;
    });

    if ( it == active_.end() ){
        SPDLOG_ERROR("cannot deactivate device id {}. Device not currently active.", id);
        return false ;
    }
    
    if ( (*it)->hasConnection() ){
        SPDLOG_DEBUG(
            "cannot remove device '{}': connections must be removed first.",
            (*it)->info().name
        );
        return false ;
    }

    if ( (*it) == primary_ ){
        primary_ = nullptr ;
    }

    active_.erase(it);
    return true ;
}

void AudioEngine::getDeviceConnections(int id, std::vector<ConnectionRequest>& requests) const {
    AudioIO* m = ComponentManager::instance()->getSignalComponent(id);
    if ( !m ) return ;

    for ( auto dev : active_ ){
        for ( auto ep : dev->inboundEndpoints() ){
            for ( const auto& s : SignalController::instance()->getSinks(ep) ){
                if ( s.component == m ){
                    auto devEp = ConnectionEndpoint::create(
                        EndpointType::COMPONENT, SocketType::SignalOutbound,
                        id, s.index
                    );
                    requests.push_back(ConnectionRequest(devEp, ep));
                }
            }
        }

        for ( auto ep : dev->outboundEndpoints() ){
            for ( const auto& s : SignalController::instance()->getSources(ep) ){
                if ( s.component == m ){
                    auto devEp = ConnectionEndpoint::create(
                        EndpointType::COMPONENT, SocketType::SignalOutbound,
                        id, s.index
                    );
                    requests.push_back(ConnectionRequest(devEp, ep));
                }
            }
        }
    }
}

auto AudioEngine::getAvailableDevices(){
    RtAudio itf ;

    available_.clear();
    for ( auto id : itf.getDeviceIds() ){
        available_[id] = itf.getDeviceInfo(id);
    }

    return std::views::values(available_);
}

AudioDevice* AudioEngine::getPrimaryDevice(){
    return primary_ ;
}

const std::vector<AudioDevice*>& AudioEngine::getInputDevices() const {
    return inputs_ ;
}

int AudioEngine::duplexCallback(
    void* outputBuffer,
    void* inputBuffer,
    uint32_t nBufferFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData
){
    AudioDevice* dev = static_cast<AudioDevice*>(userData);

    if ( !dev->running() || !dev->hasConnection() ){

    }
    if ( dev->isInputDevice() && dev->running() && dev->hasConnection() ){
        const double* bufInput = static_cast<double*>(inputBuffer);
        const size_t nChannels = dev->inputChannels();

        // resampled is float, so this allows us to push either
        auto pushToRing = [&](const auto* data, size_t frames){
            // make sure we have space on all buffers first
            for ( size_t c = 0; c < nChannels; ++c ){
                if ( dev->inputBuffers_[c].writeSpace() < frames ){
                    return ;
                }
            }

            for ( size_t c = 0; c < nChannels; ++c ){
                auto& ring = dev->inputBuffers_[c];
                ring.pushStrided(data + c, nChannels, frames);
            }
        };

        if ( dev->isGlobalSampleRate() ){
            pushToRing(bufInput, nBufferFrames);
        } else {
            pushToRing(
                dev->resampleScratchOut_.data(), 
                dev->resample(bufInput, nBufferFrames, nChannels)
            );
        }
    }
}

int playbackCallback(
    void* outputBuffer,
    void* inputBuffer,
    uint32_t nBufferFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData
){
    AudioDevice* dev = static_cast<AudioDevice*>(userData);
    double* output = static_cast<double*>(outputBuffer);
    if ( !dev->running() ){
        std::fill_n(output, nBufferFrames, 0.0);
        return 1 ; // signal loop completion
    }

    if ( dev == AudioEngine::instance()->getPrimaryDevice() ){
        processSignal()
    }

    // currently, we are configured to only support one output device
    // if this changes, we'll need to leverage ring buffers for output as well,
    // and rethink where we process midi.
    if ( dev->isOutputDevice() ){
        double* bufOutput = static_cast<double*>(outputBuffer);
        if ( !dev->running() ){
            std::fill_n(bufOutput, nBufferFrames, 0.0);
            return 1 ; // fine to return here, even if input exists, it won't get processed
        }
        

        
    }

    return 0 ;
}

int recordCallback(
    void* outputBuffer,
    void* inputBuffer,
    uint32_t nBufferFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData
){

}

void AudioEngine::processSignal(size_t frames, int sampleRate){
    auto* engine = AudioEngine::instance();
    auto* signal = SignalController::instance();

    // feed inputs into source nodes
    for ( auto& dev : engine->getInputDevices() ){
        for ( size_t i = 0; i < dev->outboundChannels(); ++i ){
            auto& ring = dev->outboundBuffer(i);
            auto& scratch = dev->outboundScratch(i);
            size_t size = ring.pop(scratch.data(), dev->bufferSize());
            if ( size < frames ){
                std::fill(scratch.begin() + size, scratch.begin() + frames, 0.0);
            }
            signal->populateSources(scratch.data(), frames);
        }
    }
    
    const float sampleDt = 1 / static_cast<float>(sampleRate);    
    
    MidiController::instance()->processEvents();
        for (unsigned int i = 0; i < nBufferFrames; ++i){
            ComponentManager::instance()->runParameterModulation();
            auto [output, outputSize] = SignalController::instance()->processFrame();
            MidiController::instance()->tick(sampleDt);
            for ( unsigned int j = 0 ; j < outputSize ; ++j ){
                *bufOutput++ = dsp::fastAtan(output[j]);
            }
        }
    ComponentManager::instance()->flushAudioProbes();
}

