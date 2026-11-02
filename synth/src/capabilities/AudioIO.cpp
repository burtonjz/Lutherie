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

#include "core/AudioIO.hpp"
#include <spdlog/spdlog.h>


AudioIO::AudioIO(size_t in, size_t out):
    bufferIndex_(0),
    nInputs_(in),
    nOutputs_(out),
    signalInputs_(in),
    signalOutputs_(out),
    buffers_(out)
{
    sampleRate_ = Config::get<double>("audio.sample_rate").value();
    bufferSize_ = Config::get<size_t>("audio.buffer_size").value();

    for ( size_t i = 0; i < out; ++i){
        buffers_[i] = std::make_unique<double[]>(bufferSize_);
    }
}

void AudioIO::setBufferIndex(size_t index){
    bufferIndex_ = index ;
}

void AudioIO::calculateSample(){}

double AudioIO::getCurrentSample(size_t output) const {
    assert( output < nOutputs_ );
    return buffers_[output][bufferIndex_];
}

double AudioIO::getLastSample(size_t output) const {
    assert( output < nOutputs_ );
    return buffers_[output][(bufferIndex_ + bufferSize_ - 1) % bufferSize_ ];
}

void AudioIO::clearBuffer(){
    for ( auto& buf : buffers_ ){
        std::fill(buf.get(), buf.get() + bufferSize_, 0.0);
    }
}

const double* AudioIO::data(size_t output) const {
    assert( output < nOutputs_ );
    return buffers_[output].get() ;
}

std::size_t AudioIO::size() const {
    return bufferSize_ ;
}

size_t AudioIO::getNumInputs() const {
    return nInputs_ ;
}

size_t AudioIO::getNumOutputs() const {
    return nOutputs_ ;
}

bool AudioIO::connectInput(AudioIO* source, size_t input, size_t sourceOutput){
    if ( input >= nInputs_ || sourceOutput >= source->nOutputs_){
        SPDLOG_ERROR("cannot connect input: Invalid channel specified.");
        return false ;
    };
    signalInputs_[input].insert({source, sourceOutput});
    source->signalOutputs_[sourceOutput].insert({this, input});
    onInputConnect();
    return true ;
}

bool AudioIO::disconnectInput(AudioIO* source, size_t input, size_t sourceOutput){
    if ( input >= nInputs_ || sourceOutput >= source->nOutputs_){
        SPDLOG_ERROR("cannot disconnect input: Invalid channel specified.");
        return false ;
    };
    signalInputs_[input].erase({source, sourceOutput});
    source->signalOutputs_[sourceOutput].erase({this, input});
    onInputDisconnect();
    return true ;
}

const AudioConnectionSet& AudioIO::getInputs(size_t inp) const {    
    assert( inp < nInputs_ );
    return signalInputs_[inp] ;
}

const AudioConnectionSet& AudioIO::getOutputs(size_t out) const {
    assert( out < nOutputs_ );
    return signalOutputs_[out] ;
}

void AudioIO::tick(){
    bufferIndex_ = std::fmod(bufferIndex_ + 1, bufferSize_);
}

bool AudioIO::isGenerative() const { 
    return false ; 
}
bool AudioIO::isPolyphonic() const { 
    return false ; 
}

void AudioIO::onInputConnect(){
}

void AudioIO::onInputDisconnect(){
}

double AudioIO::aggregateInputs(size_t idx) const {
    assert( idx < nInputs_ );
    double sum = 0.0 ;
    for ( const auto& conn : signalInputs_[idx] ){
        sum += conn.component->getLastSample(conn.index);
    }
    return sum ;
}

void AudioIO::setBufferValue(size_t idx, double val){
    assert( idx < nOutputs_ );
    buffers_[idx][bufferIndex_] = val ;
}

inline void to_json(json& j, const AudioConnection& conn){
    j["index"] = conn.index ;
};