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

#include <cstddef>
#include <functional>
#include <array>
#include <bitset>
#include <type_traits>
#include "core/AudioStreamComponent.hpp"

template<typename T, size_t N>
class FixedPool {
    static_assert(std::is_base_of<AudioStreamComponent, T>::value, "FixedPool<T>: T must derive from Module");
    static_assert(N <= 256, "FixedPool: N must fit in uint8_t for index storage");
    
private:
    std::array<std::aligned_storage_t<sizeof(T), alignof(T)>, N> storage_;
    std::bitset<N> inUse_;
    
    // NEW: Track active indices directly
    std::array<uint8_t, N> activeIndices_;
    uint8_t activeCount_ = 0;
    
    void addToActiveList(uint8_t index) {
        activeIndices_[activeCount_++] = index;
    }
    
    void removeFromActiveList(uint8_t index) {
        for (uint8_t i = 0; i < activeCount_; ++i) {
            if (activeIndices_[i] == index) {
                activeIndices_[i] = activeIndices_[--activeCount_];
                return;
            }
        }
    }
    
public:
    template<typename... Args>
    void initializeAll(Args&&... args) {
        for (size_t i = 0; i < N; ++i) {
            void* place = static_cast<void*>(&storage_[i]);
            new (place) T(std::forward<Args>(args)...);
            inUse_.reset(i);
        }
        activeCount_ = 0;
    }
    
    T* allocate() {
        for (size_t i = 0; i < N; ++i) {
            if (!inUse_.test(i)) {
                inUse_.set(i);
                addToActiveList(static_cast<uint8_t>(i));
                return reinterpret_cast<T*>(&storage_[i]);
            }
        }
        return nullptr; // Pool exhausted
    }
    
    void release(T* ptr) {
        size_t index = static_cast<size_t>(ptr - reinterpret_cast<T*>(&storage_[0]));
        if (index < N && inUse_.test(index)) {
            inUse_.reset(index);
            removeFromActiveList(static_cast<uint8_t>(index));
        }
    }
    
    template <typename Func, typename... Args>
    void forEachActive(Func&& func, Args&&... args) {
        for (uint8_t i = 0; i < activeCount_; ++i) {
            uint8_t idx = activeIndices_[i];
            T& obj = *reinterpret_cast<T*>(&storage_[idx]);
            std::invoke(std::forward<Func>(func), obj, std::forward<Args>(args)...);
        }
    }
    
    std::size_t countActiveVoices() const {
        return activeCount_;  // Now O(1) instead of O(N)
    }
    
    ~FixedPool() {
        for (uint8_t i = 0; i < activeCount_; ++i) {
            uint8_t idx = activeIndices_[i];
            reinterpret_cast<T*>(&storage_[idx])->~T();
        }
    }
};