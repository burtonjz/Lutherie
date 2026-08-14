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

#ifndef STREAM_RECORDER_HPP_
#define STREAM_RECORDER_HPP_

#include "core/AudioBufferComponent.hpp"
#include "dsp/Analyzer.hpp"
#include "configs/StreamRecorderConfig.hpp"

class StreamRecorder : public AudioBufferComponent, public Analyzer {
public:
    StreamRecorder(ComponentId id, StreamRecorderConfig cfg);

    void process(const double* data, size_t size, ComponentId id) override ;
    void onParameterChanged(ParameterType p, bool isCollection = false) override ;
};

#endif // STREAM_RECORDER_HPP_