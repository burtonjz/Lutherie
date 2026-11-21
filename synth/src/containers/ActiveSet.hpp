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

#ifndef ACTIVE_SET_HPP_
#define ACTIVE_SET_HPP_

#include <cstdint>
#include <span>

template <size_t N>
class ActiveSet {
    static_assert(N < 256, "ActiveSet: indices stored as uint8_t");
    std::array<uint8_t, N> dense_ ;
    std::array<uint8_t, N> sparse_ ;
    uint8_t count_ = 0 ;   
 
public:
    bool contains(size_t i) const {
        return i < N 
            && sparse_[i] < count_ 
            && dense_[sparse_[i]] == i ;
    }

    bool insert(size_t i) {
        if ( i >= N || contains(i) ) return false ;
        sparse_[i] = static_cast<uint8_t>(count_);
        dense_[count_++] = static_cast<uint8_t>(i);
        return true ;
    }

    bool erase(size_t i){
        if (!contains(i)) return false ;
        const uint8_t pos  = sparse_[i];
        const uint8_t last = dense_[count_ - 1];
        dense_[pos]   = last ;
        sparse_[last] = pos ;
        --count_ ;
        return true ;
    }
    void clear() { count_ = 0 ; }
    size_t size() const { return count_ ; }
    std::span<const uint8_t> indices() const { return {dense_.data(), count_}; }
};

#endif // ACTIVE_SET_HPP_