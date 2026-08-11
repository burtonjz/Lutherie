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
    bufferPos_(0.0)
{
    parameters_->add<ParameterType::STATUS>(true, false);
    parameters_->add<ParameterType::LOOP>(false, false);
    parameters_->add<ParameterType::PLAYBACK_RATE>(cfg.playback,true);
    parameters_->getParameter(ParameterType::STATUS)->addListener(this);
}

void BufferStreamer::calculateSample(){
    if ( !parameters_->getParameter<ParameterType::STATUS>()->getValue() ){
        setBufferValue(0, 0);
        return ;
    }

    /*
    Catmull-Rom cubic interpolation (limits distortion compared to linear)   
    see https://www.cs.cmu.edu/~fp/courses/graphics/asst5/catmullRom.pdf 
    where τ = 0.5 per standard
    https://splines.readthedocs.io/en/latest/euclidean/hermite-uniform.html
    is also a good reference as catmull-rom is just a specific case of hermite
    */
    size_t idx = static_cast<size_t>(bufferPos_);
    double frac = bufferPos_ - static_cast<double>(idx);

    // aggregate buffer inputs
    double value = 0 ;
    bool bufferExceedsInputs = true ;
    for ( const auto& c : AudioBufferComponent::getInputs(0) ){
        if ( !c.component ) continue ;
        const auto& buf = c.component->getBuffer(c.index);
        if ( idx >= buf.size() ) continue ;
        bufferExceedsInputs = false ;
        
        size_t size = buf.size();

        // clamp samples
        double y0 = buf[ idx == 0 ? 0 : idx - 1];
        double y1 = buf[idx];
        double y2 = buf[(idx + 1 < size) ? idx + 1 : size - 1];
        double y3 = buf[(idx + 2 < size) ? idx + 2 : size - 1];

        // basis matrix, expanded and regrouped by power of frac
        // (which allows us to avoid pow costs)
        double c0 = y1 ;                                         // frac^0
        double c1 = -0.5 * y0 + 0.5 * y2 ;                       // frac^1
        double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3 ;        // frac^2
        double c3 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3 ; // frac^3
         
        // Horner's method to optimize polynomial evaluation
        // c3 * frac^3 + c2 * frac^2 + c1 * frac + c0 ;
        double val = ((c3 * frac + c2) * frac + c1) * frac + c0 ;
        if ( midiTriggerMode_ ){
            val *= midiVelocity_ ;
        }
        value +=  val ;
    }
    setBufferValue(0, value);

    if ( 
        bufferExceedsInputs && !midiTriggerMode_ &&
        parameters_->getParameter<ParameterType::LOOP>()->getValue()
    ){
        bufferPos_ = 0.0 ;    
    } else {
        double rate = parameters_->getParameter<ParameterType::PLAYBACK_RATE>()->getInstantaneousValue();
        bufferPos_ += rate ;
    }
}

void BufferStreamer::onParameterChanged([[maybe_unused]] ParameterType p, [[maybe_unused]] bool isCollection){
    if ( parameters_->getParameter<ParameterType::STATUS>()->getValue() && !midiTriggerMode_ ){
        bufferPos_ = 0.0 ;
    }
}

void BufferStreamer::onKeyPressed([[maybe_unused]] const ActiveNote* note,[[maybe_unused]] bool repress){
    // irrespective of which key, reset
    bufferPos_ = 0.0 ;
    midiVelocity_ = note->note.getMidiVelocity() / 127.0 ;
}

void BufferStreamer::onHandlerAdded(){
    midiTriggerMode_ = true ;
}

void BufferStreamer::onHandlerRemoved(){
    if ( getNumHandlers() == 0 ){
        midiTriggerMode_ = false ;
        bufferPos_ = 0.0 ;
    }
}

void BufferStreamer::onInputUpdated(){
    bufferPos_ = 0.0 ;
}
