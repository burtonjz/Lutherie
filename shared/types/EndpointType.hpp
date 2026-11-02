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

#ifndef ENDPOINT_TYPE_HPP_
#define ENDPOINT_TYPE_HPP_

#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class EndpointType {
public:
    enum Value : uint8_t {
        DEVICE,
        COMPONENT,
        N_ENDPOINT_TYPES
    };

    EndpointType() = default ;
    constexpr EndpointType(Value v) : value_(v){}

    constexpr operator Value() const { return value_ ; }

    std::string toString() const {
        return std::string(names_[value_]);
    }

    static EndpointType fromString(std::string_view str){
        for (int i = 0; i < N_ENDPOINT_TYPES; ++i){
            if ( names_[i] == str){
                return static_cast<Value>(i);
            }
        }   
        throw std::invalid_argument("unknown EndpointType: " + std::string(str));
    }

    uint8_t to_uint8() const {
        return value_ ;
    }

    static EndpointType from_uint8(uint8_t val){
        return EndpointType(static_cast<Value>(val));
    }

    static const std::array<std::string_view, N_ENDPOINT_TYPES>& getNames(){
        return names_ ;
    }

    static constexpr int count = N_ENDPOINT_TYPES ;

private:
    Value value_{COMPONENT};

    static constexpr std::array<std::string_view, N_ENDPOINT_TYPES> names_{
        "Device",
        "Component"
    };
};

inline void from_json(const json& j, EndpointType& t) {
    t = EndpointType::fromString(j.get_ref<const std::string&>());
}

inline void to_json(json& j, const EndpointType& t) {
    j = t.toString();
}

#endif // ENDPOINT_TYPE_HPP_