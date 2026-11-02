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

#include "interfaces/BufferRW.hpp"
#include "status/BufferStatus.hpp"

// forward declarations
class BufferIO ;

class BufferContext {
public:
    size_t changedInput() const ;
    size_t numInputs() const ;
    size_t numOutputs() const ;

    BufferReads inputs(size_t channel) const ;
    BufferWrite output(size_t channel) const ;

    void setStatus(size_t channel, BufferStatus s);
    void setAllStatus(BufferStatus s);

private:
    friend class BufferIO ;
    BufferContext(BufferIO& io, size_t changed);

    BufferIO& io_ ;
    size_t changed_ ;
    std::vector<BufferStatus> status_ ;
};

struct BufferProcessor {
    virtual void onInputUpdated(BufferContext& ctx) = 0 ;

    virtual void onInputConnected(BufferContext& ctx){
        return onInputUpdated(ctx);
    }

    virtual void onInputDisconnected(BufferContext& ctx){
        return onInputUpdated(ctx);
    }

    virtual ~BufferProcessor() = default ;
};

#endif // BUFFER_PROCESSOR_HPP_