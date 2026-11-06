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

#include "capabilities/BufferIO.hpp"
#include "interfaces/BufferRW.hpp"
#include "routing/BufferConnection.hpp"
#include "status/BufferStatus.hpp"
#include <spdlog/spdlog.h>

BufferIO::BufferIO(AudioContext audio, BufferProcessor& processor, size_t inputs, size_t outputs):
    processor_(processor),
    observers_(),
    numInputs_(inputs),
    numOutputs_(outputs),
    outputs_(outputs, std::vector<double>()),
    status_(outputs, BufferStatus::NotReady),
    inputViews_(inputs),
    outputViews_(outputs),
    inputEdges_(inputs),
    outputEdges_(outputs),
    sampleRate_(audio.sampleRate)
{}

void BufferIO::addInputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel){
    inputEdges_.at(toChannel).insert({edge, edgeChannel});
    process(Hook::Connected, toChannel);
}

void BufferIO::removeInputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel){
    inputEdges_.at(toChannel).erase({edge, edgeChannel});
    process(Hook::Disconnected, toChannel);
}

void BufferIO::addOutputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel){
    outputEdges_.at(toChannel).insert({edge, edgeChannel});
}

void BufferIO::removeOutputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel){
    outputEdges_.at(toChannel).erase({edge, edgeChannel});
}

void BufferIO::addObserver(BufferObserver* observer){
    if ( !observer ){
        SPDLOG_ERROR("cannot add nullptr observer.");
        return ;
    }
    auto it = std::find(observers_.begin(), observers_.end(), observer);
    if ( it != observers_.end() ){
        SPDLOG_ERROR("observer already added.");
        return ;
    }
    observers_.push_back(observer);
}

void BufferIO::removeObserver(BufferObserver* observer){
    if ( !observer ){
        SPDLOG_ERROR("cannot remove nullptr observer.");
        return ;
    }
    auto it = std::find(observers_.begin(), observers_.end(), observer);
    if ( it == observers_.end() ){
        SPDLOG_ERROR("observer not currently added. Nothing to do.");
        return ;
    }
    observers_.erase(it);
}

size_t BufferIO::getNumInputs() const {
    return numInputs_ ;
}

size_t BufferIO::getNumOutputs() const {
    return numOutputs_ ;
}

FileStatus BufferIO::exportBuffer(std::string savePath){
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
    sfinfo.channels   = static_cast<int>(numOutputs_);
    sfinfo.format     = saveFormat ;

    bool validFormat = sf_format_check(&sfinfo);
    if ( !validFormat ){
        SPDLOG_ERROR("sndfile formatting error. please investigate.");
        return FileStatus::LoadFailed ;
    }

    SNDFILE* file = sf_open(savePath.c_str(), SFM_WRITE, &sfinfo);
    
    if ( !file ){
        SPDLOG_ERROR("Error opening file for write: {}", sf_strerror(nullptr));
        return FileStatus::LoadFailed ;
    }

    sfinfo.frames = static_cast<sf_count_t>(getMaxBufferFrames());
    auto output = getInterleavedBuffers(sfinfo);

    sf_count_t count = sf_write_double(
        file, output.data(), 
        sfinfo.frames * sfinfo.channels
    );

    sf_close(file);

    SPDLOG_INFO("wrote {} samples to file {}", count, savePath);
    return FileStatus::Ok ;
}

std::span<const BufferRead> BufferIO::getInputView(size_t channel) const {
    return inputViews_[channel];
}

void BufferIO::rebuildInputViews(size_t channel){
    auto& v = inputViews_[channel];
    v.clear();
    v.reserve(inputEdges_[channel].size());
    for ( const BufferConnection& c : inputEdges_.at(channel)){
        v.push_back(c.buffer->getOutputView(c.channel));
    }
}

const BufferRead BufferIO::getOutputView(size_t channel) const {
    return outputViews_[channel];
}

void BufferIO::rebuildOutputView(size_t channel){
    auto& v = outputViews_[channel];
    if ( status_.at(channel) == BufferStatus::Ready ){
        v = BufferRead(std::span<const double>(outputs_.at(channel)));
        return ;
    }
    v = BufferRead();
    return ;
}

BufferResize BufferIO::getOutputResize(size_t channel){
    return BufferResize(outputs_.at(channel));
}

void BufferIO::process(Hook h, size_t changedInput){
    BufferAccessor accessor(*this, changedInput);

    rebuildInputViews(changedInput);

    switch (h){
    case Hook::Connected:
        processor_.onInputConnected(accessor);
        break ;
    case Hook::Disconnected:
        processor_.onInputDisconnected(accessor);
        break ;
    case Hook::Updated:
        processor_.onInputUpdated(accessor);
        break ;
    }

    // processor has full write access to all output channels
    // so rebuild them all.
    for ( size_t ch = 0 ; ch < numOutputs_; ++ch ){
        rebuildOutputView(ch);
    }

    notifyDownstream();
}

void BufferIO::notifyDownstream(){
    for ( size_t ch = 0; ch < numOutputs_; ++ch ){
        const bool ready = status_.at(ch) == BufferStatus::Ready ;

        if ( ready ){
            for ( auto& obs : observers_ ){
                obs->onOutputChannelReady(getOutputView(ch), ch);
            }
        }
        
        for ( const auto& conn : outputEdges_.at(ch) ){
            conn.buffer->process(Hook::Updated, ch);
        }
    }
}

size_t BufferIO::getMaxBufferFrames() const {
    size_t numFrames = 0 ;
    for ( const auto& out : outputs_ ){
        if ( out.size() > numFrames ){
            numFrames = out.size();
        }
    }
    return numFrames ;
}

std::vector<double> BufferIO::getInterleavedBuffers(const SF_INFO& info) const {
    std::vector<double> interleaved(info.frames * info.channels);
    for ( sf_count_t frame = 0 ; frame < info.frames ; ++frame ){
        for ( int channel = 0 ; channel < info.channels; ++channel ){
            const auto& buf = outputs_[channel];
            interleaved[frame * info.channels + channel] = 
                frame < static_cast<sf_count_t>(buf.size())
                    ? buf[frame]
                    : 0.0 ;
        }
    }
    return interleaved ;
}

void BufferIO::setStatus(size_t channel, BufferStatus s){
    status_[channel] = s ;
}

void BufferIO::setAllStatus(BufferStatus s){
    for ( auto& st : status_ ) st = s ;
}