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

#include "capabilities/AudioProbe.hpp"

AudioProbe::AudioProbe(
    NodeContext node,
    AudioAnalyzer& analyzer, DataSink& sink, 
    size_t bufferSize, size_t scratchSize
):
    node_(node),
    analyzer_(analyzer),
    sink_(sink),
    buffer_(bufferSize),
    scratch_(scratchSize),
    result_(analyzer.maxOutputSize())
{}

bool AudioProbe::collecting() const {
    return collecting_ ;
}

void AudioProbe::setCollecting(bool c){
    collecting_ = c ;
}

void AudioProbe::onData(std::span<const double> data) noexcept {
    buffer_.push(data.data(), data.size());
}

void AudioProbe::processLoop(){
    size_t count = buffer_.pop(scratch_.data(), scratch_.size());
    if ( count == 0 ) return ;

    size_t numProcessed = analyzer_.process(scratch_, result_);
    if ( numProcessed == 0 ) return ;

    DataApiHeader header = {
        .type = node_.type,
        .id = static_cast<uint32_t>(node_.id),
        .channel = static_cast<uint32_t>(node_.channel),
        .size = numProcessed
    };

    sink_.onResult(header, result_);
}
