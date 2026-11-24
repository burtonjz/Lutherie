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

#ifndef FILE_BUFFER_HPP_
#define FILE_BUFFER_HPP_

#include "core/Node.hpp"
#include "interfaces/FileListener.hpp"
#include "interfaces/BufferProcessor.hpp"
#include "interfaces/ComponentInfo.hpp"
#include "configs/FileBufferConfig.hpp"
#include <sndfile.h>

class FileBuffer : 
    public Node,
    public ComponentInfo,
    public FileListener,
    public BufferProcessor
{
private:
    BufferTrigger* trigger_ = nullptr ;
    SF_INFO info_ ;
    SNDFILE* file_ = nullptr ;
    size_t sampleRate_ ;

public:
    FileBuffer(int id, FileBufferConfig cfg);

    FileBuffer(const FileBuffer&) = delete ;
    FileBuffer& operator=(const FileBuffer&) = delete ;
    FileBuffer(FileBuffer&&) = delete ;
    FileBuffer& operator=(FileBuffer&&) = delete ;

    // File Listener
    FileStatus onPathSet(const std::filesystem::path& path) override ;

    // Buffer Processor
    void process(BufferAccessor& accessor) override ;
    void onPrepare(AudioContext ctx) override ;

private:
    void loadWavBuffer();
    void resample(BufferResize& buf, int srcRate, int dstRate);

};

#endif // FILE_BUFFER_HPP_