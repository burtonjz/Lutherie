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

#ifndef BUFFER_PROCESSOR_HPP_
#define BUFFER_PROCESSOR_HPP_

#include "contexts/AudioContext.hpp"
#include "interfaces/BufferRW.hpp"
#include "status/BufferStatus.hpp"

struct BufferProcessor {
    virtual void process(BufferAccessor& accessor) = 0 ;

    virtual void onInputUpdated(BufferAccessor& accessor){
        return process(accessor);
    }

    virtual void onInputConnected(BufferAccessor& accessor){
        return process(accessor);
    }

    virtual void onInputDisconnected(BufferAccessor& accessor){
        return process(accessor);
    }

    // buffer trigger response
    virtual void onTrigger(BufferAccessor& accessor){
        return process(accessor);
    }

    virtual void onPrepare(AudioContext){}
    
    virtual ~BufferProcessor() = default ;
};


// if your BufferProcessor needs to arbitrarily receive an accessor
class BufferTrigger {
public:
    virtual void trigger() = 0 ;
    virtual ~BufferTrigger() = default ;
};

#endif // BUFFER_PROCESSOR_HPP_