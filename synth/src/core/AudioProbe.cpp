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

#include "core/AudioProbe.hpp"
#include "api/StreamingApiHandler.hpp"
#include "api/StreamingContext.hpp"
#include "meta/ComponentRegistry.hpp"
#include "core/AudioSignalComponent.hpp"

AudioProbe::AudioProbe():
    AudioSignalComponent(1,0),
    analysisBuffer_(),
    context_(nullptr)
{
    analysisBuffer_ = std::make_unique<double[]>(bufferSize_);
    createStreamingContext();
}

AudioProbe::~AudioProbe(){
    delete context_ ;
};

void AudioProbe::calculateSample(){
    double input ;
    if ( collecting_ ){
        input = aggregateInputs(0);
    } else {
        input = 0.0 ;
    }
    analysisBuffer_[bufferIndex_] = input ;
}

StreamingContext* AudioProbe::getStreamingContext() const {
    return context_ ;
}

void AudioProbe::createStreamingContext(){
    int size = Config::get<int>("analysis.ring_buffer_size").value_or(480000);

    std::string scratchKey = ComponentRegistry::getComponentDescriptor(type_).name ;
    std::transform(
        scratchKey.begin(), scratchKey.end(), 
        scratchKey.begin(), [](unsigned char c){
            if ( std::isspace(c) ) return '_' ;
            return static_cast<char>(std::tolower(c));
    });
    scratchKey = "analysis."  + scratchKey + ".buffer_size" ;
    auto scratchSize = Config::get<int>(scratchKey);
    if ( !scratchSize.has_value() ){
        SPDLOG_DEBUG(
            "Could not find key {} in config."
        );
    }

    context_ = new StreamingContext(size, scratchSize.value_or(4096),
        [this](const double* data, size_t size, ComponentId id){
            process(data, size, id);
        }
    );
}

void AudioProbe::flush(){
    if ( !collecting_ ) return ;
    auto* buf = analysisBuffer_.get();
    context_->buffer.push(buf, bufferSize_);
}