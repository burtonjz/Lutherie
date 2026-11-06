/*
 * Copyright (C) 2025 Jared Burton
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
 
 #ifndef __LOCK_FREE_BUFF_HPP_
 #define __LOCK_FREE_BUFF_HPP_

#include <atomic>
#include <bit>
#include <cstring>
#include <vector>

template<typename T>
class LockFreeRingBuffer {
private:
    std::vector<T> buffer_;
    
    const size_t capacity_ ;
    const size_t mask_ ;

    alignas(64) std::atomic<size_t> writePos_{0};
    alignas(64) std::atomic<size_t> readPos_{0};

public:
    LockFreeRingBuffer(size_t minCapacity): 
        LockFreeRingBuffer(std::bit_ceil(minCapacity), 0)
    {}

    size_t writeSpace() const {
        return capacity_ - 
            writePos_.load(std::memory_order_acquire) -
            readPos_.load(std::memory_order_relaxed);
    }

    size_t readSpace() const {
        return writePos_.load(std::memory_order_acquire)
            - readPos_.load(std::memory_order_relaxed);
    }

    bool pushStrided(const T* src, size_t stride, size_t count){
        return pushStrided<T>(src, stride, count);
    }
    
    // for handling data conversions during the push loop
    template<typename S>
    bool pushStrided(const S* src, size_t stride, size_t count){
        size_t w = writePos_.load(std::memory_order_relaxed);
        size_t r = readPos_.load(std::memory_order_acquire);
        if ( count > capacity_ - (w-r) ) return false ;

        size_t idx = w & mask_ ;
        size_t first = std::min(count, capacity_ - idx);
        for ( size_t i = 0 ; i < first; ++i )    buffer_[idx + i] = static_cast<T>(src[i * stride]);
        for ( size_t i = first; i < count; ++i ) buffer_[i - first] = static_cast<T>(src[i * stride]);

        writePos_.store(w + count, std::memory_order_release);
        return true ;
    };

    template<typename S>
    bool push(const S* data, size_t count){
        return pushStrided(data, 1, count);
    }
    
    bool push(const T* data, size_t count){
        return pushStrided(data, 1, count);
    }
    
    size_t pop(T* data, size_t maxCount){
        size_t r = readPos_.load(std::memory_order_relaxed);
        size_t w = writePos_.load(std::memory_order_acquire);
        size_t n = std::min(maxCount, w - r);

        size_t idx = r & mask_ ;
        size_t first = std::min(n, capacity_ - idx);
        
        std::memcpy(data, &buffer_[idx], (n-first) * sizeof(T));
        std::memcpy(data + first, &buffer_[0], (n - first) * sizeof(T));

        readPos_.store(r + n, std::memory_order_release);
        return n ;
    }

private:
    LockFreeRingBuffer(size_t cap, int):
        buffer_(cap), 
        capacity_(cap),
        mask_(cap - 1)
    {}
};

 #endif // __LOCK_FREE_BUFF_HPP_

