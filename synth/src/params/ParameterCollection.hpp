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

#ifndef PARAMETER_COLLECTION_HPP_
#define PARAMETER_COLLECTION_HPP_

#include "types/ParameterType.hpp"
#include "params/ParameterListener.hpp"

#include <spdlog/spdlog.h>
#include <vector>
#include <map>

class ParameterCollectionBase {
protected:
    ParameterType type_ ;
    std::vector<ParameterListener*> listeners_ ;

public:
    ParameterCollectionBase(
        ParameterType typ
    ):
        type_(typ)
    {}

    ParameterCollectionBase(const ParameterCollectionBase&) = delete ;
    ParameterCollectionBase& operator=(const ParameterCollectionBase&) = delete ;

    void addListener(ParameterListener* listener){
        if ( listener ) listeners_.push_back(listener);
    }

    void removeListener(ParameterListener* listener){
        listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
    }

    void notifyListeners(){
        for (auto* listener : listeners_ ){
            listener->onParameterChanged(type_, true);
        }
    }
};

template<ParameterType typ>
class ParameterCollection : public ParameterCollectionBase {
public:
    using ValueType = GET_PARAMETER_VALUE_TYPE(typ);

private:
    size_t nextID = 0 ;
    std::vector<size_t> active_ ;
    std::map<size_t, ValueType> values_ ;
    ValueType minValue_ ;
    ValueType maxValue_ ;
    std::map<size_t, ValueType> defaultValues_ ;

public:
    ParameterCollection(
        std::vector<ValueType> defaultValues,
        ValueType minValue = ParameterTraits<typ>::minimum,
        ValueType maxValue = ParameterTraits<typ>::maximum
    ):
        ParameterCollectionBase(typ),
        minValue_(minValue),
        maxValue_(maxValue)
    {
        for ( auto v : defaultValues ){
            addValue(v);
        }   
    }

    ParameterCollection():
        ParameterCollection({}){}

    size_t addValue(ValueType v){
        v = limitToRange(v);
        values_[nextID] = v ;
        defaultValues_[nextID] = v ;
        active_.push_back(nextID);
        return nextID++ ;
    }

    size_t removeValue(size_t idx){
        if ( !values_.contains(idx) ){
            std::string msg = fmt::format("Cannot remove value from collection. idx {} is not in use", idx);
            SPDLOG_ERROR(msg);
            throw std::runtime_error(msg);
        }
        values_.erase(idx);
        defaultValues_.erase(idx);
        active_.erase(std::remove(active_.begin(), active_.end(), idx), active_.end());
        return values_.size();
    } 

    size_t size() const {
        return values_.size() ;
    }

    ValueType getValue(size_t idx) const {
        if ( ! values_.contains(idx) ){
            std::string msg = fmt::format("Cannot get value from collection. idx {} is not in use", idx);
            SPDLOG_ERROR(msg);
            throw std::runtime_error(msg);
        }
        return values_.at(idx) ;
    }

    const std::map<size_t, ValueType>& getValues() const {
        return values_ ;
    }

    bool setValue(size_t idx, ValueType v){
        if ( ! values_.contains(idx) ){
            SPDLOG_ERROR("Cannot set value in collection. idx {} is not in use", idx);
            return false ;
        }
        values_[idx] = limitToRange(v);
        return true ;
    }

    ValueType getDefaultValue(size_t idx) const {
        if ( ! defaultValues_.contains(idx) ){
            std::string msg = fmt::format("Cannot get default value from collection. idx {} is not in use", idx);
            SPDLOG_ERROR(msg);
            throw std::runtime_error(msg);
        }
        return defaultValues_.at(idx) ;
    }

    void setDefaultValue(size_t idx, ValueType v){
        if ( ! defaultValues_.contains(idx) ){
            SPDLOG_ERROR("Cannot set default value in collection. idx {} is not in use", idx);
            return ;
        }
        defaultValues_[idx] = limitToRange(v);
    }

    void resetValue(size_t idx){
        if ( ! values_.contains(idx) ){
            SPDLOG_ERROR("Cannot set value in collection. idx {} is not in use", idx);
            return ;
        }
        values_[idx] = defaultValues_[idx] ;
    }

    ValueType getMinValue() const {
        return minValue_ ;
    }

    bool setMinValue(ValueType v){
        if ( v > maxValue_ ){
            SPDLOG_ERROR("Cannot set minimum value higher than maximum value.");
            return false ;
        }
        minValue_ = v ;
        return true ;
    }

    ValueType getMaxValue() const {
        return maxValue_ ;
    }

    bool setMaxValue(ValueType v){
        if ( v < minValue_ ){
            SPDLOG_ERROR("Cannot set maximum value higher than minimum value");
            return false ;
        }
        maxValue_ = v ;
        return true ;
    }

    void setValueRange(ValueType min, ValueType max){
        if ( min > max ){
            SPDLOG_ERROR("Cannot set maximum value higher than minimum value");
            return ;
        }
        minValue_ = min ;
        maxValue_ = max ;
    }

    void reset(){
        values_ = defaultValues_ ;
    }

    void clear(){
        values_.clear();
        defaultValues_.clear();
        active_.clear();
    }

    void reserve(size_t capacity){
        values_.reserve(capacity);
        defaultValues_.reserve(capacity);
    }

    const std::vector<size_t>& getIndices() const {
        return active_ ;
    }

private:
    ValueType limitToRange(ValueType value) const {
            if ( value < minValue_ ) return minValue_ ;
            if ( value > maxValue_ ) return maxValue_ ;
            return value ;
        }
};

#endif // PARAMETER_COLLECTION_HPP_

