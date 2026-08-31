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

#include "components/StreamRecorder.hpp"
#include "params/ParameterMap.hpp"
#include "api/StreamingApiHandler.hpp"

StreamRecorder::StreamRecorder(ComponentId id, [[maybe_unused]] StreamRecorderConfig cfg):
    BaseComponent(id, ComponentType::StreamRecorder),
    AudioBufferComponent(0,1),
    AudioProbe()
{
    parameters_->add<ParameterType::RECORD>(false, false);
    parameters_->getParameter(ParameterType::RECORD)->addListener(this);
}

void StreamRecorder::onParameterChanged(ParameterType p, [[maybe_unused]] bool isCollection){
    if ( p != ParameterType::RECORD ) return ;

    collecting_ = parameters_->getParameter<ParameterType::RECORD>()->getValue();

    if ( !collecting_ && AudioBufferComponent::buffers_[0].size() > 0 ){
        notifyDownstream(0);
    }
}

void StreamRecorder::process(const double* data, size_t size, ComponentId id){
    DataApiHeader header = {
        .componentId = static_cast<uint32_t>(id),
        .channel = 0
    };

    // convert down to float for stream
    std::vector<float> output(size) ;
    std::copy(data, data + size, output.data());

    StreamingApiHandler::instance()->send(header, output.data(), output.size());

    // also stash into internal buffer
    auto& internalBuffer = AudioBufferComponent::buffers_[0];
    internalBuffer.insert(internalBuffer.end(), data, data + size);
}