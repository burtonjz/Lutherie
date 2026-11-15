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

#ifndef CAPABILITY_HPP_
#define CAPABILITY_HPP_

#include <bit>
#include <cstddef>
#include <cstdint>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

#define CAPABILITY_LIST(X) \
    X(Parameters,   0) \
    X(AudioIO,      1) \
    X(BufferIO,     2) \
    X(MidiHandler,  3) \
    X(MidiListener, 4) \
    X(Modulator,    5) \
    X(AudioProbe,   6) \
    X(File,         7)

class Capability {
public:
    enum Flags : uint32_t {
        None         = 0,
        #define X(flag, idx) flag = 1u << idx,
            CAPABILITY_LIST(X)
        #undef X
    };

    static constexpr size_t Count = 0
        #define X(flag, idx) + 1 
            CAPABILITY_LIST(X)
        #undef X
        ;
        
    Capability() = default ;
    Capability(const Capability&) = delete ;
    Capability& operator=(const Capability&) = delete ;
    
    virtual ~Capability() = default ;

    static constexpr size_t index(Capability::Flags flag){
        switch (flag){
        #define X(flag, idx) case Capability::flag: return idx ;
            CAPABILITY_LIST(X)
        #undef X
        default:
            auto msg = fmt::format(
                "flag {} not found in 'CAPABILITY_LIST'. Please investigate", 
                static_cast<uint32_t>(flag)
            );
            SPDLOG_ERROR(msg);
            throw std::runtime_error(msg);
        }  
    }

    static constexpr bool isSingleFlag(Flags flag) {
        auto value = static_cast<uint32_t>(flag);
        return value != 0 && (value & (value - 1)) == 0;
    }
};

constexpr Capability::Flags operator|(Capability::Flags a, Capability::Flags b){
    return static_cast<Capability::Flags>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
    );
}

constexpr Capability::Flags operator&(Capability::Flags a, Capability::Flags b){
    return static_cast<Capability::Flags>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
    );
}

constexpr Capability::Flags& operator|=(Capability::Flags& a, Capability::Flags b){
    return a = a | b ;
}

constexpr bool hasCap(Capability::Flags flags, Capability::Flags cap){
    return (flags & cap) != Capability::None ;
}

constexpr Capability::Flags& operator&=(Capability::Flags& a, Capability::Flags b) {
    return a = a & b ;
}

template<Capability::Flags>
struct CapabilityTraits ;

// generate CapabilityTraits via x-macro
#define X(flag, idx)                            \
    class flag ;                                \
    template<>                                  \
    struct CapabilityTraits<Capability::flag>{  \
        using Type = flag ;                     \
    };                                          
    CAPABILITY_LIST(X)
#undef X 




#endif // CAPABILITY_HPP_