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

#ifndef STREAMING_CONTEXT_HPP_
#define STREAMING_CONTEXT_HPP_

#include "containers/LockFreeRingBuffer.hpp"
#include <functional>

struct StreamingContext {
    LockFreeRingBuffer<double> buffer ;
    std::function<void(const double*, size_t, int)> processFunc ;
    std::vector<double> scratch ;

    StreamingContext(
        size_t bufferSize,
        size_t scratchSize,
        std::function<void(const double*, size_t, int)> func
    ) :
        buffer(bufferSize),
        processFunc(std::move(func)),
        scratch(scratchSize)
    {}
};

#endif // STREAMING_CONTEXT_HPP_
