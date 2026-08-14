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

#ifndef ANALYSIS_CONTEXT_HPP_
#define ANALYSIS_CONTEXT_HPP_

#include "containers/LockFreeRingBuffer.hpp"
#include <functional>

struct AnalysisContext {
    LockFreeRingBuffer<double> buffer ;
    std::function<void(const double*, size_t, int)> processFunc ;
    std::vector<double> scratch ;

    AnalysisContext(
        size_t bufferSize,
        size_t scratchSize,
        std::function<void(const double*, size_t, int)> func
    ) :
        buffer(bufferSize),
        processFunc(std::move(func)),
        scratch(scratchSize)
    {}
};

#endif // ANALYSIS_CONTEXT_HPP_
