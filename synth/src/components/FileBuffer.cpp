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

#include "components/FileBuffer.hpp"

#include <sndfile.h>
#include <samplerate.h>

FileBuffer::FileBuffer(ComponentId id,[[maybe_unused]] FileBufferConfig cfg):
    BaseComponent(id, ComponentType::FileBuffer),
    AudioBufferComponent(0,2)
{
}

void FileBuffer::onSetPath(){
    loadWavBuffer();
}

void FileBuffer::loadWavBuffer(){
    SF_INFO sfinfo{};
    SNDFILE* file = sf_open(getPath().c_str(), SFM_READ, &sfinfo);

    if ( !file ){
        SPDLOG_ERROR("Error opening file: {}", sf_strerror(nullptr));
        return ;
    }

    SPDLOG_DEBUG("file {} loaded. Channels={}, SampleRate={}, Frames={}", getPath().string(), sfinfo.channels, sfinfo.samplerate, sfinfo.frames );
    if ( sfinfo.channels > 2 ){
        SPDLOG_WARN("This application does not support reading in more than two channels. Additional channels will be ignored.");
    }

    std::vector<double> interleaved(sfinfo.frames * sfinfo.channels);
    sf_count_t count = sf_readf_double(file, interleaved.data(), sfinfo.frames);

    buffers_.assign(sfinfo.channels, std::vector<double>(sfinfo.frames));

    // de-interleave buffers and force to two channels
    for ( sf_count_t frame = 0; frame < count ; ++frame){
        if ( sfinfo.channels == 1){
            buffers_[0][frame] = interleaved[frame];
            buffers_[1][frame] = interleaved[frame];
        } else {
            buffers_[0][frame] = interleaved[frame * sfinfo.channels];
            buffers_[1][frame] = interleaved[frame * sfinfo.channels + 1];
        }
    }

    // resample (if necessary)
    int i = 0 ;
    for ( auto& ch : buffers_ ){
        SPDLOG_DEBUG("resampling audio for channel {}. Rate {} -> {}", i++, sfinfo.samplerate, sampleRate_);
        resample(ch, sfinfo.samplerate, sampleRate_);
    }
}

void FileBuffer::resample(std::vector<double>& buffer, int srcRate, int dstRate){
    if ( srcRate == dstRate ) return ;

    double ratio = (double)dstRate / srcRate ;
    std::vector<float> in(buffer.begin(), buffer.end());
    std::vector<float> out((size_t)(in.size() * ratio + 1));

    SRC_DATA data{};
    data.data_in       = in.data();
    data.input_frames  = in.size();
    data.data_out      = out.data();
    data.output_frames = out.size();
    data.src_ratio     = ratio ;

    src_simple(&data, SRC_SINC_BEST_QUALITY, 1);
    out.resize(data.output_frames_gen);
    buffer.assign(out.begin(), out.end());
}