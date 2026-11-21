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

#include "types/ParameterConfig.hpp"
#include "types/ParameterType.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

ParameterConfig::ParameterConfig(
    ParameterType p, ParameterValue defaultValue, 
    ParameterLimits limits, ParameterSupports supports
):
    typ_(p),
    default_(),
    limits_(),
    supports_()
{
    setDefaultValue(defaultValue);
    setSupports(supports);
    setLimits(limits);
}

ParameterConfig::ParameterConfig(ParameterType p):
    ParameterConfig(
        p, p.defaultValue(), 
        p.limits(), p.supports()
    )
{}

ParameterType ParameterConfig::parameter() const {
    return typ_ ;
}

const ParameterValue& ParameterConfig::defaultValue() const {
    return default_ ;
}

void ParameterConfig::setDefaultValue(const ParameterValue& value){
    if ( value <= limits_.minimum ){
        SPDLOG_WARN("specified default value is less than current minimum. Setting to minimum");
        default_ = limits_.minimum ;
        return ;
    }

    if ( value >= limits_.maximum ){
        SPDLOG_WARN("specified default value is greater than current maximum. Setting to maximum");
        default_ = limits_.maximum ;
        return ;
    }

    default_ = value ;
}

const ParameterLimits& ParameterConfig::limits() const {
    return limits_ ;
}

void ParameterConfig::setLimits(const ParameterLimits& limits){
    if ( !supports_.rangeUpdate ){
        SPDLOG_WARN("This parameter does not support range updates. Limits are set to parameter's global limits");
        limits_ = typ_.limits();
        return ;
    }

    if ( limits.maximum < limits.minimum ){
        SPDLOG_ERROR("Invalid parameter limits: maximum is less than minimum.");
        return ;
    }

    if ( typ_.limits().maximum < limits.maximum ){
        SPDLOG_WARN("Cannot set configuration limit maximum higher than supported maximum.");
        limits_.maximum = typ_.limits().maximum ;
    } else {
        limits_.maximum = limits.maximum ;
    }

    if ( typ_.limits().minimum > limits.minimum ){
        SPDLOG_WARN("Cannot set configuration limit minimum lower than supported minimum.");
        limits_.minimum = typ_.limits().minimum ;
    } else {
        limits_.minimum = limits.minimum ;
    }

    setDefaultValue(default_);

}

const ParameterSupports& ParameterConfig::supports() const {
    return supports_ ;
}

void ParameterConfig::setSupports(const ParameterSupports& supports){
    if ( supports_.modulatable != supports.modulatable ){
        if ( !typ_.supports().modulatable && supports.modulatable ){
            SPDLOG_ERROR("This parameter type does not support modulation.");
        } else {
            supports_.modulatable = supports_.modulatable ;
        }
    }
    
    if ( supports_.multiValue != supports.multiValue ){
        if ( !typ_.supports().multiValue && supports.multiValue ){
            SPDLOG_ERROR("This parameter type does not support multi-value.");
        } else {
            supports_.multiValue = supports_.multiValue ;
        }
    }

    if ( supports_.rangeUpdate != supports.rangeUpdate ){
        if ( !typ_.supports().rangeUpdate && supports.rangeUpdate ){
            SPDLOG_ERROR("This parameter type does not support range overrides.");
        } else {
            supports_.rangeUpdate = supports_.rangeUpdate ;
            setLimits(limits_);
        }
    }
}

CollectionConfig::CollectionConfig(
    ParameterType p, ParameterLimits limits, 
    ParameterSupports supports
):
    typ_(p),
    limits_(),
    supports_()
{
    setSupports(supports);
    setLimits(limits);
}

CollectionConfig::CollectionConfig(ParameterType p):
    CollectionConfig(p, p.limits(), p.supports())
{}

ParameterType CollectionConfig::parameter() const {
    return typ_ ;
}

const ParameterLimits& CollectionConfig::limits() const {
    return limits_ ;
}

void CollectionConfig::setLimits(const ParameterLimits& limits){
    if ( !supports_.rangeUpdate ){
        SPDLOG_WARN("This parameter does not support range updates. Limits are set to parameter's global limits");
        limits_ = typ_.limits();
        return ;
    }

    if ( limits.maximum < limits.minimum ){
        SPDLOG_ERROR("Invalid parameter limits: maximum is less than minimum.");
        return ;
    }

    if ( typ_.limits().maximum < limits.maximum ){
        SPDLOG_WARN("Cannot set configuration limit maximum higher than supported maximum.");
        limits_.maximum = typ_.limits().maximum ;
    } else {
        limits_.maximum = limits.maximum ;
    }

    if ( typ_.limits().minimum > limits.minimum ){
        SPDLOG_WARN("Cannot set configuration limit minimum lower than supported minimum.");
        limits_.minimum = typ_.limits().minimum ;
    } else {
        limits_.minimum = limits.minimum ;
    }
}

const ParameterSupports& CollectionConfig::supports() const {
    return supports_ ;
}

void CollectionConfig::setSupports(const ParameterSupports& supports){
    if ( supports_.modulatable != supports.modulatable ){
        if ( !typ_.supports().modulatable && supports.modulatable ){
            SPDLOG_ERROR("This parameter type does not support modulation.");
        } else {
            supports_.modulatable = supports_.modulatable ;
        }
    }
    
    if ( supports_.multiValue != supports.multiValue ){
        if ( !typ_.supports().multiValue && supports.multiValue ){
            SPDLOG_ERROR("This parameter type does not support multi-value.");
        } else {
            supports_.multiValue = supports_.multiValue ;
        }
    }

    if ( supports_.rangeUpdate != supports.rangeUpdate ){
        if ( !typ_.supports().rangeUpdate && supports.rangeUpdate ){
            SPDLOG_ERROR("This parameter type does not support range overrides.");
        } else {
            supports_.rangeUpdate = supports_.rangeUpdate ;
            setLimits(limits_);
        }
    }
}

// JSON (de)serialization
ParameterConfig nlohmann::adl_serializer<ParameterConfig>::from_json(const json& j){
    ParameterType p = j["parameter"];
    ParameterValue val = j["default"];
    ParameterSupports supports = j["supports"];
    ParameterLimits limits = j["limits"];

    return ParameterConfig(p, val, limits, supports);
}

void nlohmann::adl_serializer<ParameterConfig>::to_json(json& j, const ParameterConfig& cfg){
    j["parameter"] = cfg.parameter();
    j["default"] = cfg.defaultValue();
    j["supports"] = cfg.supports();
    j["limits"] = cfg.limits();
}

CollectionConfig nlohmann::adl_serializer<CollectionConfig>::from_json(const json& j){
    ParameterType p = j["parameter"];
    ParameterSupports supports = j["supports"];
    ParameterLimits limits = j["limits"];

    return CollectionConfig(p, limits, supports);
}

void nlohmann::adl_serializer<CollectionConfig>::to_json(json& j, const CollectionConfig& cfg){
    j["parameter"] = cfg.parameter();
    j["supports"] = cfg.supports();
    j["limits"] = cfg.limits();
}