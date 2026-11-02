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

#ifndef BUFFER_RW_HPP_
#define BUFFER_RW_HPP_

#include "status/BufferStatus.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

// forward declarations
class BufferIO ;

// read-only view of a single buffer channel
class BufferRead {
public:
    const double* data() const { return data_.data(); }
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    
    const double& operator[](size_t i) const { return data_[i]; }
    std::span<const double> span() const { return data_ ; }

private:
    friend class BufferIO ;
    explicit BufferRead(std::span<const double> data):
        data_(data)
    {}

    std::span<const double> data_ ;
};

// read-only view of all buffers
class BufferReads {
public:
    size_t size() const { return channels_.size(); }
    const BufferRead* operator[](size_t channel) const {
        if ( channel >= channels_.size() || !channels_[channel] ){
            return nullptr ;
        }

        return &channels_[channel].value();
    }

private:
    friend class BufferIO ;
    BufferReads(std::vector<std::optional<BufferRead>> channels):
        channels_(std::move(channels))
    {}

    std::vector<std::optional<BufferRead>> channels_ ;
};

class BufferWrite {
public:
    double* data(){ return buffer_->data() ; }
    double& operator[](size_t i){ return (*buffer_)[i]; }

    size_t size() const { return buffer_->size(); }
    size_t capacity() const { return buffer_->capacity(); }
    bool empty() const { return buffer_->empty(); }

    template <typename It> 
    void assign(It first, It last){ buffer_->assign(first, last); }

    void resize(size_t n){ buffer_->resize(n); }
    void reserve(size_t capacity){ buffer_->reserve(capacity); }
    void clear(){ buffer_->clear(); }

    std::span<double> span(){ return *buffer_ ; }
    
private:
    friend class BufferIO ;
    explicit BufferWrite(std::vector<double>& buffer):
        buffer_(&buffer)
    {}

    std::vector<double>* buffer_ = nullptr ;

};

class BufferWrites {
public:
    size_t numOutputs() const { return channels_.size(); }
    BufferWrite& output(size_t i){ return channels_.at(i); }
    const BufferWrite& output(size_t i) const { return channels_.at(i); }

private:
    friend class BufferIO ;
    explicit BufferWrites(std::vector<BufferWrite> channels):
        channels_(std::move(channels))
    {}
    
    std::vector<BufferWrite> channels_ ;
};

class BufferResults {
public:
    void set(size_t channel, BufferStatus s){ status_[channel] = s ; }
    void setAll(BufferStatus s){ for ( auto& st : status_ ) st = s ; }
private:
    friend class BufferIO ;
    explicit BufferResults(size_t channels):
        status_(channels, BufferStatus::Failed)
    {}

    std::vector<BufferStatus> status_ ;
};

#endif // BUFFER_RW_HPP_