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

#include "BufferRW.hpp"
#include "capabilities/BufferIO.hpp"

BufferAccessor::BufferAccessor(BufferIO& io, size_t changed):
    io_(io),
    changed_(changed)
{}

size_t BufferAccessor::changedInput() const {
    return changed_ ;
}

size_t BufferAccessor::numInputs() const {
    return io_.numInputs();
}

size_t BufferAccessor::numOutputs() const {
    return io_.numOutputs();
}

std::span<const BufferRead> BufferAccessor::inputs(size_t channel) const {
    return io_.inputView(channel);
}

BufferResize BufferAccessor::output(size_t channel) const {
    return io_.outputResize(channel);
}

void BufferAccessor::setStatus(size_t channel, BufferStatus s){
    io_.setStatus(channel, s);
}

void BufferAccessor::setAllStatus(BufferStatus s){
    io_.setAllStatus(s);
}