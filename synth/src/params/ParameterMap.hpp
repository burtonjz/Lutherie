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

#ifndef __PARAMETER_MAP_HPP_
#define __PARAMETER_MAP_HPP_

#include "params/Parameter.hpp"
#include "params/ModulationParameter.hpp"
#include "types/ParameterType.hpp"
#include "params/ParameterCollection.hpp"

#include "config/Config.hpp"

#include <nlohmann/json.hpp>
#include <set>
#include <spdlog/spdlog.h>


// forward declarations
template<ParameterType typ>
class Parameter ;
class ParameterBase ;
class BaseModulator ;

using params = std::array<ParameterBase*, N_PARAMETER_TYPES> ;
using collections = std::array<ParameterCollectionBase*, N_PARAMETER_TYPES> ;

using json = nlohmann::json ;

class ParameterMap {
    private:
        params parameters_{} ;
        std::set<ParameterType> modulatable_ ; 
        std::set<ParameterType> reference_ ; // parameters that are reference only to a parent parameter map (like a polyOscillator managing child oscillators)

        bool has_collections = false ;
        collections collections_{} ;

    public:
        ParameterMap();

        ParameterBase* getParameter(ParameterType p) const ;
        ParameterCollectionBase* getCollection(ParameterType p) const ;

        void addReferences(ParameterMap& other);

        std::set<ParameterType> getModulatableParameters() const ;
        void modulate();
        
        // Parameter Dispatcher Functions
        json getValueDispatch(ParameterType p) const ;
        bool setValueDispatch(ParameterType p, const json& value);
        json getDefaultDispatch(ParameterType p) const ;
        bool setDefaultDispatch(ParameterType p, const json& value);
        json getMinDispatch(ParameterType p) const ;
        json getMaxDispatch(ParameterType p) const ;
        bool setMaxDispatch(ParameterType p, const json& value);
        bool setMinDispatch(ParameterType p, const json& value);
        void addParameterDispatch(ParameterType p, const json& cfg);
        
        // collection dispatchers
        json   getCollectionValueDispatch(ParameterType p, size_t idx) const ;
        json   getCollectionValuesDispatch(ParameterType p) const ;
        json   getCollectionDefaultsDispatch(ParameterType p) const ;
        size_t addCollectionValueDispatch(ParameterType p, const json& value);
        size_t removeCollectionValueDispatch(ParameterType p, size_t idx);
        void   setCollectionValueDispatch(ParameterType p, size_t idx, const json& value);
        json   getCollectionMinDispatch(ParameterType p) const ;
        json   getCollectionMaxDispatch(ParameterType p) const ;
        bool   setCollectionMinDispatch(ParameterType p, const json& value);
        bool   setCollectionMaxDispatch(ParameterType p, const json& value);
        void   resetCollectionDispatch(ParameterType p);
        void   addCollectionDispatch(ParameterType p, const json& cfg);


        // serialization
        json toJson() const ;
        void fromJson(const json& j);

        // templates
        template <ParameterType typ>
        Parameter<typ>* getParameter() const {
            return static_cast<Parameter<typ>*>(getParameter(typ)) ;
        }

        
        template <ParameterType typ>
        ParameterCollection<typ>* getCollection() const {
            return static_cast<ParameterCollection<typ>*>(getCollection(typ));
        }

        template <ParameterType typ>
        void add(
            GET_PARAMETER_VALUE_TYPE(typ) defaultValue,
            bool modulatable,
            GET_PARAMETER_VALUE_TYPE(typ) minValue = GET_PARAMETER_TRAIT_MEMBER(typ, minimum), 
            GET_PARAMETER_VALUE_TYPE(typ) maxValue = GET_PARAMETER_TRAIT_MEMBER(typ, maximum),
            BaseModulator* modulator = nullptr, ModulationData modData = {}
        ){
            if ( getParameter(typ) ){
                SPDLOG_ERROR("Parameter {} already in map.", GET_PARAMETER_TRAIT_MEMBER(typ, name));
                return ;
            }
                
            Parameter<typ>* p = new Parameter<typ>(defaultValue, modulatable, minValue, maxValue, modulator, modData);
            parameters_[static_cast<size_t>(typ)] = p ;
            if (modulatable) modulatable_.insert(typ);
        }

        template <ParameterType typ>
        void addCollection(
            std::vector<GET_PARAMETER_VALUE_TYPE(typ)> defaultValues,
            GET_PARAMETER_VALUE_TYPE(typ) minValue = GET_PARAMETER_TRAIT_MEMBER(typ, minimum), 
            GET_PARAMETER_VALUE_TYPE(typ) maxValue = GET_PARAMETER_TRAIT_MEMBER(typ, maximum)
        ){
            if ( getCollection(typ) ){
                SPDLOG_ERROR("Collection already defined for Parameter {}", GET_PARAMETER_TRAIT_MEMBER(typ, name));
                return ;
            }

            ParameterCollection<typ>* c = new ParameterCollection<typ>(defaultValues, minValue, maxValue);
            collections_[static_cast<size_t>(typ)] = c ;
        }

    private:
        void modulateParameter(ParameterType typ);

        template<typename T>
        static json CollectionToJsonArray(const std::vector<T>& values){
            json j = json::array();
            for ( const auto& v : values ){
                if constexpr (std::is_same_v<T, uint8_t>){
                    j.push_back(static_cast<int>(v));
                } else {
                    j.push_back(v);
                }
            }
            return j ;
        }

};

#endif // __PARAMETER_MAP_HPP_