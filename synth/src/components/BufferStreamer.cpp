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

#include "components/BufferStreamer.hpp"
#include "params/ParameterMap.hpp"

BufferStreamer::BufferStreamer(ComponentId id, [[maybe_unused]] BufferStreamerConfig cfg):
    BaseComponent(id, ComponentType::BufferStreamer),
    AudioBufferComponent(1,0),
    AudioSignalComponent(0,1),
    bufferPos_(0)
{
    parameters_->add<ParameterType::STATUS>(true, false);
    parameters_->getParameter(ParameterType::STATUS)->addListener(this);
}

void BufferStreamer::calculateSample(){
    if ( !parameters_->getParameter<ParameterType::STATUS>()->getValue() ){
        setBufferValue(0, 0);
        return ;
    }
    
    // aggregate buffer inputs
    double value = 0 ;
    for ( const auto& c : AudioBufferComponent::getInputs(0) ){
        if ( !c.component ) continue ;
        const auto& buf = c.component->getBuffer(c.index);
        if ( bufferPos_ >= buf.size() ) continue ;
        value += buf[bufferPos_];
    }
    
    setBufferValue(0, value);
    bufferPos_ += 1 ;
}

void BufferStreamer::onParameterChanged([[maybe_unused]] ParameterType p){
    // whenever status is toggled on, reset to beginning of buffer
    if ( parameters_->getParameter<ParameterType::STATUS>()->getValue() ){
        bufferPos_ = 0 ;
    }
}