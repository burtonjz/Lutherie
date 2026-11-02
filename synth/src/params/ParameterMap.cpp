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
    perVoice_()
{}

ParameterMap::~ParameterMap(){
    for ( auto* p : parameters_ ) delete p ;
    for ( auto* c : collections_ ) delete c ;
}

ParameterBase* ParameterMap::getParameter(ParameterType p) const {
    return parameters_[static_cast<size_t>(p)];
}

ParameterCollectionBase* ParameterMap::getCollection(ParameterType p) const {
    return collections_[static_cast<size_t>(p)];
}

std::set<ParameterType> ParameterMap::getModulatableParameters() const {
    return modulatable_ ;
}

void ParameterMap::modulate(){
    for (auto it = modulatable_.begin(); it != modulatable_.end(); ++it ){
        modulateParameter(*it);
    }
}

void ParameterMap::markPerVoice(ParameterType p){
    if ( !getParameter(p) ){
        SPDLOG_ERROR(
            "Cannot mark missing parameter {} as per-voice.",
            GET_PARAMETER_TRAIT_MEMBER(p, name)
        );
        return ;
    }
    perVoice_.insert(p);
}

bool ParameterMap::isPerVoice(ParameterType p) const {
    return perVoice_.count(p) > 0 ;
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

void ParameterMap::setValuePercentDispatch(ParameterType p, double percent){
    switch(p){
        #define X(NAME) case ParameterType::NAME: {           \
            auto param = getParameter<ParameterType::NAME>(); \
            if ( !param ){                                    \
                throw std::runtime_error(fmt::format(         \
                    "parameter {} not found in map",          \
                    GET_PARAMETER_TRAIT_MEMBER(p, name)       \
                ));                                           \
            }                                                 \
            auto min = param->getMinimum();                   \
            auto max = param->getMaximum();                   \
            auto newVal = ( max - min ) * percent + min ;     \
            param->setValue(newVal);                          \
            break ; } ;                      
        PARAMETER_TYPE_LIST
        #undef X
    default:
        throw std::runtime_error("Invalid Parameter dispatch");
    }   
}

void ParameterMap::setValueTickWrapDispatch(ParameterType p){
    switch(p){
        #define X(NAME) case ParameterType::NAME: {           \
            auto param = getParameter<ParameterType::NAME>(); \
            auto current = param->getValue();                 \
            auto min = param->getMinimum();                   \
            auto max = param->getMaximum();                   \
            if ( current + 1 == max ) param->setValue(min);   \
            else param->setValue(current + 1);                \
            break ; } ;
        PARAMETER_TYPE_LIST
        #undef X
    default:
        throw std::runtime_error("Invalid Parameter dispatch");
    }
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

json ParameterMap::getCollectionValuesDispatch(ParameterType p) const {
    switch (p) {
        #define X(NAME) case ParameterType::NAME:          \
        {                                                  \
            auto c = getCollection<ParameterType::NAME>(); \
            json arr ;                                     \
            if ( !c ) return arr ;                         \
            for ( const auto& idx : c->getIndices() ){     \
                arr.push_back(c->getValue(idx)) ;          \
            }                                              \
            return arr ;                                   \
        }
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Invalid Collection dispatch");
    }
}

json ParameterMap::getCollectionDefaultsDispatch(ParameterType p) const {
    switch (p) {
        #define X(NAME) case ParameterType::NAME:          \
        {                                                  \
            auto c = getCollection<ParameterType::NAME>(); \
            json arr ;                                     \
            if ( !c ) return arr ;                         \
            for ( const auto& idx : c->getIndices() ){     \
                arr[idx] = c->getDefaultValue(idx) ;       \
            }                                              \
            return arr ;                                   \
        }
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

void ParameterMap::addCollectionValuesDispatch(ParameterType p, const json& value){
    bool success = true ;
    switch (p) {
        #define X(NAME) case ParameterType::NAME:                            \
            for ( const auto v : value ){                                    \
                success = getCollection<ParameterType::NAME>()->addValue(v); \
            }                                                                \
            break ;
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
json ParameterMap::toJson(json& output) const {
    /* 
    Note: we don't serialize collections directly -- front end is only expected to understand
    constructed CollectionRequests, which requires some information not available to the 
    ParameterMap. See ComponentManager::serializeComponent for how collection dispatches are utilized
    */
    for ( size_t p = 0 ; p < N_PARAMETER_TYPES ; ++p ){
        ParameterType typ = static_cast<ParameterType>(p);

        // normal parameters
        if ( parameters_[p] ){
            auto& param = output["parameters"][GET_PARAMETER_TRAIT_MEMBER(typ, name)];
            param["currentValue"] = getValueDispatch(typ);
            param["defaultValue"] = getDefaultDispatch(typ);
            param["minimumValue"] = getMinDispatch(typ);
            param["maximumValue"] = getMaxDispatch(typ);
            param["modulatable"] = getParameter(typ)->isModulatable();
        }
    }

    return output ;
}

void ParameterMap::fromJson(const json& j){
    for (const auto& [name, value] : j.items() ){
        ParameterType p = stringToParameter(name);
        addParameterDispatch(p, value);
    }
}

void ParameterMap::modulateParameter(ParameterType typ){
    auto p = getParameter(typ);

    if ( !p || perVoice_.count(typ) ) return ;
    p->modulate();
}