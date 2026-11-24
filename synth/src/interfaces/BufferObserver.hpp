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

#ifndef BUFFER_OBSERVER_HPP_
#define BUFFER_OBSERVER_HPP_

#include "interfaces/BufferRW.hpp"

// forward declarations
class BufferIO ;

class BufferObserver {
public:
    // public event handles
    virtual void onInputConnected(size_t channel){ onInputUpdated(channel); }
    virtual void onInputDisconnected(size_t channel){ onInputUpdated(channel); }
    virtual void onInputUpdated(size_t /*channel*/){}

    virtual void onOutputChannelReady(size_t /*channel*/){};
    
    virtual ~BufferObserver() = default ;

protected:
    BufferObserver();

    // only valid if BufferObserver is bound to capability
    std::span<const BufferRead> viewInputs(size_t channel) const ;

private:
    friend BufferIO ;
    BufferIO* io_ ;
    void bind(BufferIO* io);
};

#endif // BUFFER_OBSERVER_HPP_