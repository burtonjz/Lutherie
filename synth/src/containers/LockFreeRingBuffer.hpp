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
#include <vector>

template<typename T>
class LockFreeRingBuffer {
private:
    std::vector<T> buffer_;
    std::atomic<size_t> writePos_{0};
    std::atomic<size_t> readPos_{0};
    size_t capacity_;

public:
    LockFreeRingBuffer(size_t capacity): 
        buffer_(capacity), 
        capacity_(capacity) 
    {}
    
    bool push(const T* data, size_t count) {
        size_t write = writePos_.load(std::memory_order_relaxed);
        size_t read = readPos_.load(std::memory_order_acquire);
        size_t available = (read > write) 
            ? (read - write - 1) 
            : (capacity_ - write + read - 1);
        
        if (count > available) {
            return false;
        }
        
        for (size_t i = 0; i < count; ++i) {
            buffer_[(write + i) % capacity_] = data[i];
        }
        
        writePos_.store((write + count) % capacity_, std::memory_order_release);
        return true;
    }
    
    size_t pop(T* data, size_t maxCount) {
        size_t write = writePos_.load(std::memory_order_acquire);
        size_t read = readPos_.load(std::memory_order_relaxed);
        size_t available = (write >= read) 
            ? (write - read) 
            : (capacity_ - read + write);
        
        size_t toRead = std::min(maxCount, available);
        for (size_t i = 0; i < toRead; ++i) {
            data[i] = buffer_[(read + i) % capacity_];
        }
        
        readPos_.store((read + toRead) % capacity_, std::memory_order_release);
        return toRead;
    }
};

 #endif // __LOCK_FREE_BUFF_HPP_

