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

#include "core/AudioSignalComponent.hpp"


AudioSignalComponent::AudioSignalComponent(size_t in, size_t out):
    bufferIndex_(0),
    nInputs_(in),
    nOutputs_(out),
    signalInputs_(in),
    signalOutputs_(out),
    buffers_(out)
{
    Config::load();
    sampleRate_ = Config::get<double>("audio.sample_rate").value();
    bufferSize_ = Config::get<size_t>("audio.buffer_size").value();

    for ( size_t i = 0; i < out; ++i){
        buffers_[i] = std::make_unique<double[]>(bufferSize_);
    }
}

void AudioSignalComponent::setBufferIndex(size_t index){
    bufferIndex_ = index ;
}

void AudioSignalComponent::calculateSample(){}

double AudioSignalComponent::getCurrentSample(size_t output) const {
    assert( output < nOutputs_ );
    return buffers_[output][bufferIndex_];
}

double AudioSignalComponent::getLastSample(size_t output) const {
    assert( output < nOutputs_ );
    return buffers_[output][(bufferIndex_ + bufferSize_ - 1) % bufferSize_ ];
}

void AudioSignalComponent::clearBuffer(){
    for ( auto& buf : buffers_ ){
        std::fill(buf.get(), buf.get() + bufferSize_, 0.0);
    }
}

const double* AudioSignalComponent::data(size_t output) const {
    assert( output < nOutputs_ );
    return buffers_[output].get() ;
}

std::size_t AudioSignalComponent::size() const {
    return bufferSize_ ;
}

size_t AudioSignalComponent::getNumInputs() const {
    return nInputs_ ;
}

size_t AudioSignalComponent::getNumOutputs() const {
    return nOutputs_ ;
}

void AudioSignalComponent::connectInput(AudioSignalComponent* source, size_t input, size_t sourceOutput){
    assert( input < nInputs_ );
    assert( sourceOutput < source->nOutputs_ );
    signalInputs_[input].insert({source, sourceOutput});
    source->signalOutputs_[sourceOutput].insert({this, input});
    onInputConnect();
}

void AudioSignalComponent::disconnectInput(AudioSignalComponent* source, size_t input, size_t sourceOutput){
    assert ( input < nInputs_ );
    assert ( sourceOutput < source->nOutputs_ );
    signalInputs_[input].erase({source, sourceOutput});
    source->signalOutputs_[sourceOutput].erase({this, input});
    onInputDisconnect();
}

const SignalConnectionSet& AudioSignalComponent::getInputs(size_t inp) const {    
    assert( inp < nInputs_ );
    return signalInputs_[inp] ;
}

const SignalConnectionSet& AudioSignalComponent::getOutputs(size_t out) const {
    assert( out < nOutputs_ );
    return signalOutputs_[out] ;
}

void AudioSignalComponent::tick(){
    bufferIndex_ = std::fmod(bufferIndex_ + 1, bufferSize_);
}

bool AudioSignalComponent::isGenerative() const { 
    return false ; 
}
bool AudioSignalComponent::isPolyphonic() const { 
    return false ; 
}

void AudioSignalComponent::onInputConnect(){
}

void AudioSignalComponent::onInputDisconnect(){
}

double AudioSignalComponent::aggregateInputs(size_t idx) const {
    assert( idx < nInputs_ );
    double sum = 0.0 ;
    for ( const auto& conn : signalInputs_[idx] ){
        sum += conn.component->getLastSample(conn.index);
    }
    return sum ;
}

void AudioSignalComponent::setBufferValue(size_t idx, double val){
    assert( idx < nOutputs_ );
    buffers_[idx][bufferIndex_] = val ;
}

inline void to_json(json& j, const SignalConnection& conn){
    j["componentId"] = conn.component->getId() ;
    j["index"] = conn.index ;
};