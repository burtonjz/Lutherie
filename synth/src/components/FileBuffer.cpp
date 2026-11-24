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
#include "capabilities/BufferIO.hpp" // IWYU pragma: keep
#include "capabilities/File.hpp"     // IWYU pragma: keep
#include "types/ComponentType.hpp"

#include <sndfile.h>
#include <samplerate.h>

FileBuffer::FileBuffer(int id, FileBufferConfig):
    Node(id, NodeType::COMPONENT),
    ComponentInfo(ComponentType::FileBuffer),
    FileListener(),
    BufferProcessor(),
    sampleRate_(0)
{
    auto* buf = &addCapability<Capability::BufferIO>(*this, 0, 2);
    trigger_ = buf ;

    addCapability<Capability::File>(this);
}

FileStatus FileBuffer::onPathSet(const std::filesystem::path& path){
    const char* sPath = path.c_str();

    info_ = {};

    SPDLOG_DEBUG("attempting to load file: {}", sPath);
    file_ = sf_open(path.c_str(), SFM_READ, &info_);
    if ( !file_ ){
        SPDLOG_ERROR("Error opening file: {}", sf_strerror(nullptr));
        return FileStatus::LoadFailed ;
    }
    SPDLOG_DEBUG(
        "file {} loaded. Channels={}, SampleRate={}, Frames={}", 
        sPath, info_.channels, info_.samplerate, info_.frames 
    );

    if ( info_.channels > 2 ){
        SPDLOG_WARN(
            "This application does not support reading "
            "in more than two channels. "
            "Additional channels will be ignored."
        );
    }

    trigger_->trigger(); // runs process

    return FileStatus::Ok ;
}

void FileBuffer::onPrepare(AudioContext ctx){
    sampleRate_ = ctx.sampleRate ;
}

void FileBuffer::process(BufferAccessor& accessor){
    std::vector<double> interleaved(info_.frames * info_.channels);
    
    sf_count_t count = sf_readf_double(
        file_, interleaved.data(), 
        info_.frames
    );
    
    BufferResize res0 = accessor.output(0);
    BufferResize res1 = accessor.output(1);

    BufferWrite ch0 = res0.write();
    BufferWrite ch1 = res1.write();

    res0.resize(info_.frames);
    res1.resize(info_.frames);

    // de-interleave buffers and force to two channels
    for ( sf_count_t frame = 0; frame < count ; ++frame){
        if ( info_.channels == 1){
            ch0[frame] = interleaved[frame];
            ch1[frame] = interleaved[frame];
        } else {
            ch0[frame] = interleaved[frame * info_.channels];
            ch1[frame] = interleaved[frame * info_.channels + 1];
        }
    }

    // resample (if necessary)
    if ( (size_t)info_.samplerate != sampleRate_ ){
        SPDLOG_DEBUG("resampling audio. Rate {} -> {}", info_.samplerate, sampleRate_);
        resample(res0, info_.samplerate, sampleRate_);
        resample(res1, info_.samplerate, sampleRate_);
        SPDLOG_DEBUG("resampling complete.");
    }
    
    sf_close(file_);

    accessor.setAllStatus(BufferStatus::Ready);
}

void FileBuffer::resample(BufferResize& buf, int srcRate, int dstRate){
    if ( srcRate == dstRate ) return ;

    BufferRead read = buf.read();

    double ratio = (double)dstRate / srcRate ;
    std::vector<float> in(read.begin(), read.end());
    std::vector<float> out((size_t)(in.size() * ratio + 1));

    SRC_DATA data{};
    data.data_in       = in.data();
    data.input_frames  = in.size();
    data.data_out      = out.data();
    data.output_frames = out.size();
    data.src_ratio     = ratio ;

    src_simple(&data, SRC_SINC_BEST_QUALITY, 1);
    out.resize(data.output_frames_gen);
    buf.assign(out.begin(), out.end());
}