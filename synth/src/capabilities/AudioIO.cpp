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

#include "capabilities/AudioIO.hpp"
#include "interfaces/AudioProcessor.hpp"
#include <spdlog/spdlog.h>

AudioIO::AudioIO(
    AudioProcessor& processor, AudioContext audio,
    size_t inputs, size_t outputs
):
    processor_(processor),
    observers_(),
    currentFrame_(0),
    sampleRate_(audio.sampleRate),
    bufferSize_(audio.bufferSize),
    numInputs_(inputs),
    numOutputs_(outputs),
    input_(numInputs_, std::vector<double>(bufferSize_)),
    output_(numOutputs_, std::vector<double>(bufferSize_)),
    inputView_(makeAccessor<BufferRead>(input_)),
    outputView_(makeAccessor<BufferRead>(output_)),
    outputWrite_(makeAccessor<BufferWrite>(output_)),
    accessor_(inputView_, outputWrite_, bufferSize_, sampleRate_),
    inputEdges_(numInputs_),
    outputEdges_(numOutputs_)
{}

void AudioIO::prepare(AudioContext audio){
    sampleRate_ = audio.sampleRate ;
    if ( audio.bufferSize == bufferSize_ ) return ;
    bufferSize_ = audio.bufferSize ;

    for ( auto& inp : input_ ){
        inp.resize(audio.bufferSize);
    }

    for ( auto& out : output_ ){
        out.resize(audio.bufferSize);
    }

    inputView_   = makeAccessor<BufferRead>(input_);
    outputView_  = makeAccessor<BufferRead>(input_);
    outputWrite_ = makeAccessor<BufferWrite>(input_);

    accessor_ = AudioAccessor(
        inputView_, outputWrite_, 
        bufferSize_, sampleRate_
    );

    currentFrame_ = 0 ;
}

void AudioIO::addInputEdge(AudioIO* edge, size_t edgeChannel, size_t toChannel){
    inputEdges_.at(toChannel).insert({edge, edgeChannel});
}

void AudioIO::removeInputEdge(AudioIO* edge, size_t edgeChannel, size_t toChannel){
    inputEdges_.at(toChannel).erase({edge, edgeChannel});
}

void AudioIO::addOutputEdge(AudioIO* edge, size_t edgeChannel, size_t toChannel){
    outputEdges_.at(toChannel).insert({edge, edgeChannel});
}

void AudioIO::removeOutputEdge(AudioIO* edge, size_t edgeChannel, size_t toChannel){
    outputEdges_.at(toChannel).erase({edge, edgeChannel});
}

void AudioIO::addObserver(AudioObserver* observer, size_t channel){
    if ( !observer ){
        SPDLOG_ERROR("cannot add nullptr observer.");
        return ;
    }
    auto& observers = observers_[channel];
    auto it = std::find(observers.begin(), observers.end(), observer);
    if ( it != observers.end() ){
        SPDLOG_ERROR("observer already added.");
        return ;
    }
    observers.push_back(observer);
}

void AudioIO::removeObserver(AudioObserver* observer, size_t channel){
    if ( !observer ){
        SPDLOG_ERROR("cannot remove nullptr observer.");
        return ;
    }
    auto& observers = observers_[channel];
    auto it = std::find(observers.begin(), observers.end(), observer);
    if ( it == observers.end() ){
        SPDLOG_ERROR("observer not currently added. Nothing to do.");
        return ;
    }
    observers.erase(it);
}

size_t AudioIO::getNumInputs() const {
    return numInputs_ ;
}

size_t AudioIO::getNumOutputs() const {
    return numOutputs_ ;
}

double AudioIO::getCurrentSample(size_t channel) const {
    return output_[channel][currentFrame_];
}

double AudioIO::getLastSample(size_t channel) const {
    size_t last = currentFrame_ == 0
        ? bufferSize_ - 1
        : currentFrame_ - 1 ;

    return output_[channel][last];
}

BufferRead AudioIO::output(size_t channel) const {
    return outputView_[channel];
}

const AudioConnectionSet& AudioIO::getInputEdges(size_t channel) const {
    return inputEdges_[channel];
}
const AudioConnectionSet& AudioIO::getOutputEdges(size_t channel) const {
    return outputEdges_[channel];
}

void AudioIO::processFrame(){
    aggregateInputFrame();
    processor_.processFrame(accessor_, currentFrame_);
}

void AudioIO::tick(){
    bool bufferEnd = ++currentFrame_ == bufferSize_ ;
    if ( bufferEnd )  currentFrame_ = 0 ;
    
    processor_.onTick(currentFrame_);

    if ( bufferEnd ) notifyObservers();
}

void AudioIO::aggregateInputFrame(){
    for ( size_t ch = 0; ch < numInputs_; ++ch ){
        double& out = output_[ch][currentFrame_];
        for ( const auto& edge : inputEdges_[ch] ){
            out += edge.audio->getLastSample(edge.channel);
        }
    } 
}

void AudioIO::notifyObservers() const {
    for ( size_t ch = 0; ch < numOutputs_; ++ch ){
        for ( auto& obs : observers_[ch] ){
            obs->onData(outputView_[ch]);
        }
    }
}