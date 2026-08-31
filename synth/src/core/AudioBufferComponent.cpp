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

#include "core/AudioBufferComponent.hpp"
#include "api/DataApiHandler.hpp"

 AudioBufferComponent::AudioBufferComponent(size_t in, size_t out):
    nInputs_(in),
    nOutputs_(out),
    inboundConnections_(in),
    outboundConnections_(out),
    buffers_(out)
{
    Config::load();
    sampleRate_ = Config::get<double>("audio.sample_rate").value();
}

void AudioBufferComponent::connectInput(AudioBufferComponent* source, size_t input, size_t sourceOutput){
    assert( input < nInputs_ );
    assert( sourceOutput < source->nOutputs_ );

    inboundConnections_[input].insert({source, sourceOutput});
    source->outboundConnections_[sourceOutput].insert({this, input});
    SPDLOG_DEBUG("input from source {} outbound channel {} connected to inbound channel {}. Current number of inputs: {}", 
        fmt::ptr(source), sourceOutput, input, inboundConnections_[input].size());
    onInputConnect();
}

void AudioBufferComponent::disconnectInput(AudioBufferComponent* source, size_t input, size_t sourceOutput){
    assert ( input < nInputs_ );
    assert ( sourceOutput < source->nOutputs_ );

    inboundConnections_[input].erase({source, sourceOutput});
    source->outboundConnections_[sourceOutput].erase({this, input});
    SPDLOG_DEBUG("input from source {} outbound channel {} disconnected from inbound channel {}. Current number of inputs: {}", 
        fmt::ptr(source), sourceOutput, input, inboundConnections_[input].size());
    onInputDisconnect();
}

const std::vector<double>& AudioBufferComponent::getBuffer(size_t idx) const {
    if ( idx >= nOutputs_ ){
        std::string err = fmt::format("Cannot get buffer of index {}. Out of range (size={})", idx, nOutputs_);
        SPDLOG_CRITICAL(err);
        throw std::runtime_error(err);
    }

    return buffers_.at(idx);
}

size_t AudioBufferComponent::getNumInputs() const {
    return nInputs_ ; 
}

size_t AudioBufferComponent::getNumOutputs() const {
    return nOutputs_ ;
}

bool AudioBufferComponent::saveBuffer(std::string savePath){
    int saveFormat ;
    if ( savePath.ends_with(".wav") ) saveFormat = SF_FORMAT_WAV | SF_FORMAT_DOUBLE ;
    else if ( savePath.ends_with(".mp3") ) saveFormat = SF_FORMAT_MPEG | SF_FORMAT_MPEG_LAYER_III ;
    else if ( savePath.ends_with(".aiff") ) saveFormat = SF_FORMAT_AIFF | SF_FORMAT_DOUBLE ;
    else {
        SPDLOG_DEBUG("save path does not end with an expected file type. setting to wav");
        savePath.append(".wav");
        saveFormat = SF_FORMAT_WAV ;
    }
    
    SF_INFO sfinfo{};
    sfinfo.samplerate = static_cast<int>(sampleRate_);
    sfinfo.channels   = static_cast<int>(nOutputs_);
    sfinfo.format     = saveFormat ;

    bool validFormat = sf_format_check(&sfinfo);
    if ( !validFormat ){
        SPDLOG_ERROR("sndfile formatting error. please investigate.");
        return false ;
    }

    SNDFILE* file = sf_open(savePath.c_str(), SFM_WRITE, &sfinfo);
    
    if ( !file ){
        SPDLOG_ERROR("Error opening file for write: {}", sf_strerror(nullptr));
        return false ;
    }

    sfinfo.frames = static_cast<sf_count_t>(getMaxBufferFrames());
    auto output = getInterleavedBuffers(sfinfo);

    sf_count_t count = sf_write_double(
        file, output.data(), 
        sfinfo.frames * sfinfo.channels
    );

    sf_close(file);

    SPDLOG_INFO("wrote {} samples to file {}", count, savePath);
    return true ;
}

const BufferConnectionSet& AudioBufferComponent::getInputs(size_t inp) const {    
    assert( inp < nInputs_ );
    return inboundConnections_[inp] ;
}

const BufferConnectionSet& AudioBufferComponent::getOutputs(size_t out) const {
    assert( out < nOutputs_ );
    return outboundConnections_[out] ;
}

void AudioBufferComponent::onInputConnect(){};
void AudioBufferComponent::onInputDisconnect(){};
void AudioBufferComponent::onInputUpdated(){};

void AudioBufferComponent::notifyDownstream(size_t channel){
    for ( size_t i = 0 ; i < nOutputs_ ; ++i ){
        for ( const auto& conn : getOutputs(i) ){
            conn.component->onInputUpdated();
        }
    }
    DataApiHeader header = {
        .componentId = static_cast<uint32_t>(getId()),
        .channel = static_cast<uint32_t>(channel)
    };
    const auto& buf = getBuffer(channel);
    DataApiHandler::instance()->sendApiData(header, buf.data(), buf.size());
}

size_t AudioBufferComponent::getMaxBufferFrames() const {
    size_t numFrames = 0 ;
    for ( const auto& buf : buffers_ ){
        if ( buf.size() > numFrames ){
            numFrames = buf.size();
        }
    }
    return numFrames ;
}

std::vector<double> AudioBufferComponent::getInterleavedBuffers(const SF_INFO& info) const {
    std::vector<double> interleaved(info.frames * info.channels);
    for ( sf_count_t frame = 0 ; frame < info.frames ; ++frame ){
        for ( int channel = 0 ; channel < info.channels; ++channel ){
            interleaved[frame * info.channels + channel] = buffers_[channel][frame];
        }
    }
    return interleaved ;
}
