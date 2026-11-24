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

BufferIO::BufferIO(BufferProcessor* processor, size_t inputs, size_t outputs):
    processor_(processor),
    numInputs_(inputs),
    numOutputs_(outputs),
    outputs_(outputs, std::vector<double>()),
    status_(outputs, BufferStatus::NotReady),
    inputViews_(inputs),
    outputViews_(outputs),
    inputEdges_(inputs),
    outputEdges_(outputs),
    sampleRate_(0)
{}

BufferIO::BufferIO(BufferProcessor& processor, size_t inputs, size_t outputs):
    BufferIO(&processor, inputs, outputs)
{}

BufferIO::BufferIO(size_t inputs):
    BufferIO(nullptr, inputs, 0)
{}

void BufferIO::prepare(AudioContext audio){
    sampleRate_ = audio.sampleRate ;
    if ( processor_ ) processor_->onPrepare(audio);
}

void BufferIO::addInputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel){
    inputEdges_.at(toChannel).insert({edge, edgeChannel});
    process(Event::Connected, toChannel);
}

void BufferIO::removeInputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel){
    inputEdges_.at(toChannel).erase({edge, edgeChannel});
    process(Event::Disconnected, toChannel);
}

void BufferIO::addOutputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel){
    outputEdges_.at(toChannel).insert({edge, edgeChannel});
}

void BufferIO::removeOutputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel){
    outputEdges_.at(toChannel).erase({edge, edgeChannel});
}

void BufferIO::addObserver(BufferObserver* obs){
    if ( !obs ){
        SPDLOG_ERROR("cannot add nullptr obs.");
        return ;
    }
    auto it = std::find(observers_.begin(), observers_.end(), obs);
    if ( it != observers_.end() ){
        SPDLOG_ERROR("obs already added.");
        return ;
    }
    observers_.push_back(obs);
}

void BufferIO::removeObserver(BufferObserver* obs){
    if ( !obs ){
        SPDLOG_ERROR("cannot remove nullptr obs.");
        return ;
    }
    auto it = std::find(observers_.begin(), observers_.end(), obs);
    if ( it == observers_.end() ){
        SPDLOG_ERROR("obs not currently added. Nothing to do.");
        return ;
    }
    observers_.erase(it);
}

void BufferIO::bindObserver(BufferObserver* obs){
    if ( !obs ) return ;
    obs->bind(this);
}

size_t BufferIO::numInputs() const {
    return numInputs_ ;
}

size_t BufferIO::numOutputs() const {
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

    sfinfo.frames = static_cast<sf_count_t>(maxBufferFrames());
    auto output = interleavedBuffers(sfinfo);

    sf_count_t count = sf_write_double(
        file, output.data(), 
        sfinfo.frames * sfinfo.channels
    );

    sf_close(file);

    SPDLOG_INFO("wrote {} samples to file {}", count, savePath);
    return FileStatus::Ok ;
}

std::span<const BufferRead> BufferIO::inputView(size_t channel) const {
    return inputViews_[channel];
}

void BufferIO::rebuildInputViews(size_t channel){
    auto& v = inputViews_[channel];
    v.clear();
    v.reserve(inputEdges_[channel].size());
    for ( const BufferConnection& c : inputEdges_.at(channel)){
        v.push_back(c.buffer->outputView(c.channel));
    }
}

BufferRead BufferIO::outputView(size_t channel) const {
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

BufferResize BufferIO::outputResize(size_t channel){
    return BufferResize(outputs_.at(channel));
}

void BufferIO::process(Event e, size_t changedInput){
    rebuildInputViews(changedInput);
    notifyObservers(e, changedInput);
    if ( !processor_ ) return ;

    BufferAccessor accessor(*this, changedInput);
    switch (e){
    case Event::Connected:
        processor_->onInputConnected(accessor);
        break ;
    case Event::Disconnected:
        processor_->onInputDisconnected(accessor);
        break ;
    case Event::Updated:
        processor_->onInputUpdated(accessor);
        break ;
    }

    publishOutputs();
}

void BufferIO::notifyObservers(Event e, size_t changedInput) const {
    for ( const auto& obs : observers_ ){
        switch (e){
        case Event::Connected:
            obs->onInputConnected(changedInput);
            break ;
        case Event::Disconnected:
            obs->onInputDisconnected(changedInput);
            break ;
        case Event::Updated:
            obs->onInputUpdated(changedInput);
            break ;
        }
    }
}

void BufferIO::trigger(){
    if ( !processor_ ) return ;

    BufferAccessor accessor(*this, 0);
    processor_->onTrigger(accessor);
    publishOutputs();
}

void BufferIO::publishOutputs(){
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
                obs->onOutputChannelReady(ch);
            }
        }
        
        for ( const auto& conn : outputEdges_.at(ch) ){
            conn.buffer->process(Event::Updated, ch);
        }
    }
}

size_t BufferIO::maxBufferFrames() const {
    size_t numFrames = 0 ;
    for ( const auto& out : outputs_ ){
        if ( out.size() > numFrames ){
            numFrames = out.size();
        }
    }
    return numFrames ;
}

std::vector<double> BufferIO::interleavedBuffers(const SF_INFO& info) const {
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