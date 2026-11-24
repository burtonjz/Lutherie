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

#include "capabilities/BufferIO.hpp"   // IWYU pragma: keep
#include "capabilities/AudioIO.hpp"    // IWYU pragma: keep
#include "capabilities/Parameters.hpp" // IWYU pragma: keep
#include "interfaces/ParameterAccessor.hpp"

BufferStreamer::BufferStreamer(int id, [[maybe_unused]] BufferStreamerConfig cfg):
    Node(id, NodeType::COMPONENT),
    ComponentInfo(ComponentType::BufferStreamer),
    BufferObserver(),
    AudioProcessor(),
    MidiObserver(),
    ParameterObserver(),
    status_(createSlotAccessor<ParameterType::STATUS>()),
    loop_(createSlotAccessor<ParameterType::LOOP>()),
    playbackRate_(createSlotAccessor<ParameterType::PLAYBACK_RATE>())
{
    auto& buf = addCapability<Capability::BufferIO>(1);
    buf.addObserver(this);
    buf.bindObserver(this);

    addCapability<Capability::AudioIO>(*this, 0, 1);

    auto& params = addCapability<Capability::Parameters>();

    params.bindSlot<ParameterType::STATUS>(status_, 0);
    params.bindSlot<ParameterType::LOOP>(loop_, 0);
    params.bindSlot<ParameterType::PLAYBACK_RATE>(playbackRate_, 0);
    params.observe(ParameterType::STATUS);

    params.freezeParameterCreation();
}

// AUDIO PROCESSOR
void BufferStreamer::processFrame(const AudioAccessor& accessor, size_t frame) noexcept {
    BufferWrite output = accessor.output(0);
    if ( !status_.value() ){
        output[frame] = 0 ;
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
    bool exceedsAllInputs = true ;
    bool midiTriggerMode = numMidiConnections() > 0 ;
    for ( const auto& inp : viewInputs(0) ){
        size_t size = inp.size();
        if ( idx >= size ) continue ;
        
        exceedsAllInputs = false ;
        
        // clamp samples
        double y0 = inp[idx == 0 ? 0 : idx - 1];
        double y1 = inp[idx];
        double y2 = inp[(idx + 1 < size) ? idx + 1 : size - 1];
        double y3 = inp[(idx + 2 < size) ? idx + 2 : size - 1];

        // basis matrix, expanded and regrouped by power of frac
        // (which allows us to avoid pow costs)
        double c0 = y1 ;                                         // frac^0
        double c1 = -0.5 * y0 + 0.5 * y2 ;                       // frac^1
        double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3 ;        // frac^2
        double c3 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3 ; // frac^3
         
        // Horner's method to optimize polynomial evaluation
        // c3 * frac^3 + c2 * frac^2 + c1 * frac + c0 ;
        double val = ((c3 * frac + c2) * frac + c1) * frac + c0 ;
        
        // factor in midi velocity (when in midi mode)
        if ( midiTriggerMode ){
            val *= midiVelocity_ ;
        }
        value +=  val ;
    }
    
    output[frame] = value ;

    if ( 
        exceedsAllInputs && 
        !midiTriggerMode &&
        loop_.modulatedValue()
    ){
        bufferPos_ = 0.0 ; 
        return ;   
    } 
    
    if ( !exceedsAllInputs ){
        double rate = playbackRate_.modulatedValue();
        bufferPos_ += rate ;
    }
}

void BufferStreamer::onKeyPressed(const MidiNote& note){
    bufferPos_ = 0.0 ; // reset regardless of which key
    midiVelocity_ = note.velocity() / 127.0 ; // keep track of velocity
    // just let the full buffer play out on press, no need to kill it on release
}

void BufferStreamer::onParameterChanged(ParameterType, uint16_t){
    // only listening to status
    // if status toggles true (and we aren't in midi mode), reset position
    if ( status_.value() && numMidiConnections() == 0 ){
        bufferPos_ = 0.0 ;
    }
}

void BufferStreamer::onInputUpdated(size_t channel){
    bufferPos_ = 0.0 ;
}