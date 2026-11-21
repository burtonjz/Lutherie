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

#ifndef PARAMETER_MASK_HPP_
#define PARAMETER_MASK_HPP_

#include <array>
#include <cstddef>
#include <cstdint>

template <size_t N>
class ParameterMask {
private:
    static constexpr size_t wordBits = 64 ;
    static constexpr size_t wordCount = (N + wordBits - 1) / wordBits ;

    std::array<uint64_t, wordCount> words_ ;

public:
    ParameterMask(): words_(0) {}

    void set(size_t i, bool value = true){
        const auto mask = (uint64_t)1 << (i % wordBits);
        if ( value ) words_[i / wordBits] |= mask ;
        else         words_[i / wordBits] &= ~mask ;
    }

    bool test(size_t i) const {
        return (words_[i / wordBits] >> (i % wordBits)) & 1 ;
    }

    void reset(){
        words_.fill(0);
    }

    template <typename F>
    void forEach(F&& fn) const {
        for ( size_t w = 0; w < wordCount; ++w ){
            auto bits = words_[w];

            while (bits){
                const auto bit = std::countr_zero(bits);
                const auto index = w * wordBits + bit ;
                if ( index < N ) fn(index);
                bits &= bits - 1 ;
            }
        }
    }
};


#endif // PARAMETER_MASK_HPP_
