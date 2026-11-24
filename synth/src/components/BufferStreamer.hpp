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

#ifndef BUFFER_STREAMER_HPP_
#define BUFFER_STREAMER_HPP_


#include "configs/BufferStreamerConfig.hpp"

#include "core/Node.hpp"
#include "interfaces/AudioProcessor.hpp"
#include "interfaces/BufferObserver.hpp"
#include "interfaces/ComponentInfo.hpp"
#include "interfaces/MidiObserver.hpp"
#include "interfaces/ParameterAccessor.hpp"
#include "interfaces/ParameterObserver.hpp"

class BufferStreamer : 
    public Node,
    public ComponentInfo,
    public BufferObserver,
    public AudioProcessor,
    public MidiObserver,
    public ParameterObserver
{
private:
    SlotAccessor<ParameterType::STATUS> status_ ;
    SlotAccessor<ParameterType::LOOP> loop_ ;
    SlotAccessor<ParameterType::PLAYBACK_RATE> playbackRate_ ;

    double bufferPos_ = 0.0 ;
    double midiVelocity_ = 1.0 ;

public:
    BufferStreamer(int id, BufferStreamerConfig cfg);

    BufferStreamer(const BufferStreamer&) = delete ;
    BufferStreamer& operator=(const BufferStreamer&) = delete ;
    BufferStreamer(BufferStreamer&&) = delete ;
    BufferStreamer& operator=(BufferStreamer&&) = delete ;

    // BUFFER OBSERVER
    void onInputUpdated(size_t channel) override ;

    // AUDIO PROCESSOR
    void processFrame(const AudioAccessor& accessor, size_t frame) noexcept override ;

    // MIDI OBSERVER
    void onKeyPressed(const MidiNote& note) override ;

    // PARAMETER OBSERVER
    void onParameterChanged(ParameterType p, uint16_t slot) override ;

};

#endif // BUFFER_STREAMER_HPP_