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

#ifndef PARAMETER_CONFIG_HPP_
#define PARAMETER_CONFIG_HPP_

#include "types/ParameterTraits.hpp"
#include "types/ParameterType.hpp"

class ParameterConfig {
private:
    ParameterType typ_ ;
    ParameterValue default_ ;
    ParameterLimits limits_ ;
    ParameterSupports supports_ ;

public:
    ParameterConfig(
        ParameterType p, ParameterValue defaultValue, 
        ParameterLimits limits, ParameterSupports supports
    );

    ParameterConfig(ParameterType p);

    ParameterType parameter() const ;
    
    const ParameterValue& defaultValue() const ;
    void setDefaultValue(const ParameterValue& value);

    const ParameterLimits& limits() const ;
    void setLimits(const ParameterLimits& limits);

    const ParameterSupports& supports() const ;
    void setSupports(const ParameterSupports& supports);
};

class CollectionConfig {
private:
    ParameterType typ_ ;
    ParameterLimits limits_ ;
    ParameterSupports supports_ ;

public:
    CollectionConfig(ParameterType p, ParameterLimits limits, ParameterSupports supports);
    CollectionConfig(ParameterType p);

    ParameterType parameter() const ;

    const ParameterLimits& limits() const ;
    void setLimits(const ParameterLimits& limits);

    const ParameterSupports& supports() const ;
    void setSupports(const ParameterSupports& supports);
};

// JSON (de)serialization
namespace nlohmann {
template <>
struct adl_serializer<ParameterConfig> {
    static ParameterConfig from_json(const json& j);
    static void to_json(json& j, const ParameterConfig& cfg);
};

template <>
struct adl_serializer<CollectionConfig> {
    static CollectionConfig from_json(const json& j);
    static void to_json(json& j, const CollectionConfig& cfg);
};
} 

#endif // PARAMETER_CONFIG_HPP_