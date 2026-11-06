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

#ifndef AUDIO_PROCESSOR_HPP_
#define AUDIO_PROCESSOR_HPP_

#include "interfaces/BufferRW.hpp"
#include <cstddef>

// forward declarations
class AudioIO ;

class AudioAccessor {
public:
    size_t numInputs() const { return inputs_.size(); }
    size_t numOutputs() const { return outputs_.size(); }
    size_t frames() const { return frames_; }
    double sampleRate() const { return sampleRate_; }

    BufferRead  input(size_t i) const  { return inputs_[i]; }   // never empty: silence if unconnected
    BufferWrite output(size_t i) const { return outputs_[i]; }  // fixed size, can't resize

private:
    friend class AudioIO ;

    AudioAccessor(
        std::span<const BufferRead> in, 
        std::span<const BufferWrite> out,
        size_t frames, double sampleRate
    ):
        inputs_(in), 
        outputs_(out), 
        frames_(frames), 
        sampleRate_(sampleRate)
    {}

    std::span<const BufferRead>  inputs_ ;
    std::span<const BufferWrite> outputs_ ;
    size_t frames_ ;
    double sampleRate_ ;
};

struct AudioProcessor {
    virtual void processFrame(const AudioAccessor& accessor, size_t frame) noexcept = 0 ;
    virtual void onTick(size_t /*newFrame*/){}
    virtual ~AudioProcessor() = default ;
};

#endif // AUDIO_PROCESSOR_HPP_