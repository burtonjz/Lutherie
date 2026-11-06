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
#include <span>
#include <vector>

// forward declarations
class BufferIO ;
class AudioIO ;

// read-only can be span
using BufferRead = std::span<const double> ;

// writeable can edit values but not resize
using BufferWrite = std::span<double> ;

// writable and resizeable
// resize nullifies span, so need wrapper class
class BufferResize {
public:
    BufferResize(const BufferResize&) = delete ;
    BufferResize& operator=(const BufferResize&) = delete ;

    BufferRead  read()  const { return *buf_ ; }
    BufferWrite write() const { return *buf_ ; }

    size_t size() const { return buf_->size(); }
    size_t capacity() const { return buf_->capacity(); }
    bool empty() const { return buf_->empty(); }

    void resize(size_t n) { buf_->resize(n); }
    void reserve(size_t n) { buf_->reserve(n); }
    void clear() { buf_->clear(); }

    template <typename It> 
    void assign(It first, It last) { buf_->assign(first, last); }

private:
    friend class BufferIO ;
    
    explicit BufferResize(std::vector<double>& b): 
        buf_(&b)
    {}

    std::vector<double>* buf_ ;
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