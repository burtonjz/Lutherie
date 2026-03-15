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

#include "params/ParameterMap.hpp"

ParameterMap::ParameterMap():
    modulatable_(),
    reference_()
{}

ParameterBase* ParameterMap::getParameter(ParameterType p) const {
    return parameters_[static_cast<size_t>(p)];
}

ParameterCollectionBase* ParameterMap::getCollection(ParameterType p) const {
    return collections_[static_cast<size_t>(p)];
}

void ParameterMap::addReferences(ParameterMap& other){
    const params& otherParams = other.parameters_ ;
    for ( size_t i = 0 ; i < otherParams.size() ; ++i ){
        if ( !otherParams[i] ) continue ;

        parameters_[i] = otherParams[i] ;
        reference_.insert(ParameterType(i)) ;

        // we won't actually perform the modulation in the child object,
        // but we still need to track modulatable parameters
        if(otherParams[i]->isModulatable()) modulatable_.insert(ParameterType(i)); 
    }
}

std::set<ParameterType> ParameterMap::getModulatableParameters() const {
    return modulatable_ ;
}

void ParameterMap::modulate(){
    for (auto it = modulatable_.begin(); it != modulatable_.end(); ++it ){
        modulateParameter(*it);
    }
}

json ParameterMap::getValueDispatch(ParameterType p) const {
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return ParameterValueToJson(getParameter<ParameterType::NAME>()->getValue());
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Parameter dispatch");
    }
}

bool ParameterMap::setValueDispatch(ParameterType p, const json& value){
    switch (p){
        #define X(NAME) case ParameterType::NAME: return getParameter<ParameterType::NAME>()->setValue(value);
        PARAMETER_TYPE_LIST
        #undef X
    default:
        return false ;
    }
}

json ParameterMap::getDefaultDispatch(ParameterType p) const {
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return ParameterValueToJson(getParameter<ParameterType::NAME>()->getDefaultValue());
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Parameter dispatch");
    }
}

bool ParameterMap::setDefaultDispatch(ParameterType p, const json& value){
    switch (p){
        #define X(NAME) case ParameterType::NAME: return getParameter<ParameterType::NAME>()->setDefaultValue(value);
        PARAMETER_TYPE_LIST
        #undef X
    default:
        return false ;
    }
}

json ParameterMap::getMinDispatch(ParameterType p) const {
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return ParameterValueToJson(getParameter<ParameterType::NAME>()->getMinimum());
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Parameter dispatch");
    }
}

bool ParameterMap::setMinDispatch(ParameterType p, const json& value){
    switch (p){
        #define X(NAME) case ParameterType::NAME: return getParameter<ParameterType::NAME>()->setMinimum(value);
        PARAMETER_TYPE_LIST
        #undef X
    default:
        return false ;
    }
}

json ParameterMap::getMaxDispatch(ParameterType p) const {
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return ParameterValueToJson(getParameter<ParameterType::NAME>()->getMaximum());
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Parameter dispatch");
    }
}

bool ParameterMap::setMaxDispatch(ParameterType p, const json& value){
    switch (p){
        #define X(NAME) case ParameterType::NAME: return getParameter<ParameterType::NAME>()->setMaximum(value);
        PARAMETER_TYPE_LIST
        #undef X
    default:
        return false ;
    }
}

void ParameterMap::addParameterDispatch(ParameterType p, const json& cfg){
    switch(p){
        #define X(NAME) case ParameterType::NAME: \
            add<ParameterType::NAME>( \
                cfg["defaultValue"].get<GET_PARAMETER_VALUE_TYPE(ParameterType::NAME)>(), \
                cfg["modulatable"].get<bool>()                                            \
            );                                                                            \
            break ;
        PARAMETER_TYPE_LIST
        #undef X
    default:
        throw std::runtime_error("Invalid Parameter dispatch");
    };
}

// collection dispatchers
json ParameterMap::getCollectionValueDispatch(ParameterType p, size_t idx) const {
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return ParameterValueToJson(getCollection<ParameterType::NAME>()->getValue(idx));
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Collection dispatch");
    }
}

size_t ParameterMap::addCollectionValueDispatch(ParameterType p, const json& value){
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return getCollection<ParameterType::NAME>()->addValue(value);
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Collection dispatch");
    }
}

size_t  ParameterMap::removeCollectionValueDispatch(ParameterType p, size_t idx){
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return getCollection<ParameterType::NAME>()->removeValue(idx);
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Collection dispatch");
    }
}

void ParameterMap::setCollectionValueDispatch(ParameterType p, size_t idx, const json& value){
    bool success = true ;
    switch (p) {
        #define X(NAME) case ParameterType::NAME: success = getCollection<ParameterType::NAME>()->setValue(idx, value); break ;
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Collection dispatch");
    if ( !success ) throw std::runtime_error("failed to set collection value");
    }
}

json ParameterMap::getCollectionMinDispatch(ParameterType p) const {
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return getCollection<ParameterType::NAME>()->getMinValue();
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Collection dispatch");
    }
}

json ParameterMap::getCollectionMaxDispatch(ParameterType p) const {
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return getCollection<ParameterType::NAME>()->getMaxValue();
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Collection dispatch");
    }
}

bool ParameterMap::setCollectionMinDispatch(ParameterType p, const json& value){
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return getCollection<ParameterType::NAME>()->setMinValue(value);
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Collection dispatch");
    }
}

bool ParameterMap::setCollectionMaxDispatch(ParameterType p, const json& value){
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return getCollection<ParameterType::NAME>()->setMaxValue(value);
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Collection dispatch");
    }
}

void ParameterMap::resetCollectionDispatch(ParameterType p){
    switch (p) {
        #define X(NAME) case ParameterType::NAME: return getCollection<ParameterType::NAME>()->reset();
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Collection dispatch");
    }
}

void ParameterMap::addCollectionDispatch(ParameterType p, const json& cfg){
    switch(p){
        #define X(NAME) case ParameterType::NAME: \
            addCollection<ParameterType::NAME>(   \
                cfg["defaultValue"]               \
            );                                    \
            break ;                               
        PARAMETER_TYPE_LIST
        #undef X
    default:
        throw std::runtime_error("Invalid Parameter dispatch");
    };
}


// serialization
json ParameterMap::toJson() const {
    json output ;
    for ( size_t p = 0 ; p < N_PARAMETER_TYPES ; ++p ){
        ParameterType typ = static_cast<ParameterType>(p);

        if ( !parameters_[p] ) continue ;
        if (reference_.find(typ) != reference_.end()) continue ;
        
        output[GET_PARAMETER_TRAIT_MEMBER(typ, name)] = { 
            {"currentValue", getValueDispatch(typ)},
            {"defaultValue", getDefaultDispatch(typ)},
            {"minimumValue", getMinDispatch(typ)},
            {"maximumValue", getMaxDispatch(typ)},
            {"modulatable", getParameter(typ)->isModulatable()}
        };
    }
    return output ;
}

void ParameterMap::fromJson(const json& j){
    for (const auto& [name, value] : j.items() ){
        ParameterType p = parameterFromString(name);
        addParameterDispatch(p, value);
    }
}

void ParameterMap::modulateParameter(ParameterType typ){
    auto p = getParameter(typ);
    auto r = reference_.find(typ);

    if ( ! p || r != reference_.end() ) return ;

    p->modulate();
}