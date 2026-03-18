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

#ifndef __PARAMETER_HPP_
#define __PARAMETER_HPP_

#include "types/ParameterType.hpp"
#include "params/ParameterListener.hpp"
#include "core/BaseModulator.hpp"
#include <spdlog/spdlog.h>
#include <variant>
#include <vector>

// forward declaration
template <ParameterType typ> class Parameter ;

class ParameterBase {
protected:
    ParameterType type_ ;
    bool modulatable_ ;
    BaseModulator* modulator_ ;
    ModulationStrategy modStrategy_ ;
    ModulationData modData_ ;

    bool hasDepth_ ;
    bool depthInitialized_ ;
    
    std::vector<ParameterListener*> listeners_ ;

public:
    ParameterBase(
        ParameterType typ, 
        bool modulatable,
        BaseModulator* modulator = nullptr,
        ModulationData modData = {}
    ):
        type_(typ),
        modulatable_(modulatable),
        modulator_(modulator),
        modStrategy_(ModulationStrategy::NONE),
        modData_(modData)    
    {}

    ParameterBase(const ParameterBase&) = delete;
    ParameterBase& operator=(const ParameterBase&) = delete;

    virtual void setModulatable(bool modulatable){
        modulatable_ = modulatable ;
    }

    virtual bool isModulatable(){ 
        return modulatable_ ; 
    }

    void setModulation(BaseModulator* modulator, ModulationData modData){
        if ( modulator){
            modData_ = modData ;
            modulator_ = modulator ;
            modStrategy_ = GET_PARAMETER_MODULATION_STRATEGY(type_, modulator_->getModulatorRange());
            SPDLOG_DEBUG("modulation strategy set to {}", (uint8_t)modStrategy_);
        } else {
            SPDLOG_WARN("Modulation was not set because the modulator was a null pointer.");
        }
    }

    void removeModulation(){
        modulator_ = nullptr ;
        modData_ = {} ;
    }

    BaseModulator* getModulator() const { 
        return modulator_ ;
    }
    
    ModulationData* getModulationData(){
        return &modData_ ;
    }

    ModulationStrategy getModulationStrategy() const {
        return modStrategy_ ;
    }

    void setModulationStrategy(ModulationStrategy strat){
        modStrategy_ = strat ;
    }

    virtual void resetValue() = 0 ;
    virtual void modulate() = 0 ;
    virtual Parameter<ParameterType::DEPTH>* getDepth(){ return nullptr ;}

    void addListener(ParameterListener* listener){
        if (listener) listeners_.push_back(listener);
    }

    void removeListener(ParameterListener* listener){
        listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
    }

    void notifyListeners(){
        for (auto* listener : listeners_ ){
            listener->onParameterChanged(type_);
        }
    }

};

template <ParameterType typ>
struct DepthStorage {
    static constexpr size_t DEPTH_SLOT_SIZE = 512 ;
};

template <ParameterType typ>
class Parameter : public ParameterBase {
    public:
        using ValueType = GET_PARAMETER_VALUE_TYPE(typ);

    private:
        ValueType minValue_ ;
        ValueType maxValue_ ;
        ValueType value_ ;
        ValueType instantaneousValue_ ;
        ValueType defaultValue_ ;

        // fixed size container for a Parameter<ParameterType::DEPTH>, only on non-DEPTH parameters
        static constexpr size_t DEPTH_SLOT_SIZE = 512; 

        struct AlignedStorage {
            alignas(std::max_align_t) std::byte buf[DEPTH_SLOT_SIZE];
        };

        using DepthStorage = std::conditional_t<
            typ == ParameterType::DEPTH,
            std::monostate,
            AlignedStorage
        >;

        DepthStorage depthStorage_ ;
        Parameter<ParameterType::DEPTH>* depth_ = nullptr ;
        
    public:
        Parameter(
            ValueType defaultValue, bool modulatable, 
            ValueType minValue = ParameterTraits<typ>::minimum, 
            ValueType maxValue = ParameterTraits<typ>::maximum,
            BaseModulator* modulator = nullptr, ModulationData modData = {}
        ):
            ParameterBase(typ,modulatable,modulator,modData),
            minValue_(minValue),
            maxValue_(maxValue),
            value_(limitToRange(defaultValue)),
            instantaneousValue_(value_),
            defaultValue_(value_)
        {
            modStrategy_ = GET_PARAMETER_MODULATION_STRATEGY(typ, ModulatorRange::UNKNOWN) ;

            if constexpr (typ != ParameterType::DEPTH){
                depth_ = new (depthStorage_.buf) Parameter<ParameterType::DEPTH>(
                    1.0f,
                    true
                );
            }
        }

        ~Parameter(){
            if constexpr (typ != ParameterType::DEPTH){
                if (depth_){
                    depth_->~Parameter();
                    depth_ = nullptr ;
                }
            }
        }
        Parameter(const Parameter&) = delete ;
        Parameter& operator=(const Parameter&) = delete ;
        Parameter(Parameter&&) = delete ;
        Parameter& operator=(Parameter&&) = delete ;

        /**
        * @brief limit value to Parameter's range
        * 
        * @param value value
        */
        ValueType limitToRange(ValueType value) const {
            if ( value < minValue_ ) return minValue_ ;
            if ( value > maxValue_ ) return maxValue_ ;
            return value ;
        }

        void resetValue(){
            setValue(defaultValue_);
        }

        ValueType getValue() const {
            return value_ ;
        }

        bool setValue(ValueType value){
            value_ = limitToRange(value);
            setInstantaneousValue(value_);
            return true ;
        }

        ValueType getMinimum() const {
            return minValue_ ;
        }

        bool setMinimum(ValueType value){
            minValue_ = value ;
            return true ;
        }

        ValueType getMaximum() const {
            return maxValue_ ;
        }

        bool setMaximum(ValueType value){
            maxValue_ = value ;
            return true ;
        }

        ValueType getDefaultValue() const {
            return defaultValue_ ;
        }

        bool setDefaultValue(ValueType value){
            defaultValue_ = value ;
            return true ;
        }
        
        ValueType getInstantaneousValue() const {
            return instantaneousValue_ ;
        }

        void modulate(){
            // don't modulate booleans
            if constexpr (std::is_same_v<ValueType, bool>) {
                return ;
            } else {
                // modulate this parameter's depth, if it exists
                auto* depth = getDepth();
                double instDepth = 1 ;
                if ( depth  && depth->isModulatable() ){
                    getDepth()->modulate();
                    instDepth = getDepth()->getInstantaneousValue();
                }

                // now, modulate this parameter itself
                if( !modulatable_ || !modulator_ ) return ;

                switch(modStrategy_){
                case ModulationStrategy::ADDITIVE:
                    setInstantaneousValue(value_ + instDepth * modulator_->modulate(value_, &modData_));
                    return ;
                case ModulationStrategy::MULTIPLICATIVE_ZERO:
                    setInstantaneousValue( value_ * instDepth * modulator_->modulate(value_, &modData_));
                    return ;
                case ModulationStrategy::MULTIPLICATIVE_UNITY:
                    setInstantaneousValue( value_ * ( 1 + instDepth * modulator_->modulate(value_, &modData_)));
                    return ;
                case ModulationStrategy::EXPONENTIAL:
                    setInstantaneousValue(value_ * exp2f(instDepth * modulator_->modulate(value_, &modData_)));
                    return ;
                case ModulationStrategy::REPLACE:
                    setInstantaneousValue(instDepth * modulator_->modulate(value_, &modData_));
                    return ;
                default:
                    return ;
                }
            }
        }

        Parameter<ParameterType::DEPTH>* getDepth(){
            return depth_ ;
        }

    private:
        void setInstantaneousValue(ValueType v){
            if ( v != instantaneousValue_ ){
                instantaneousValue_ = v ;
                notifyListeners();
            }
        }
};


#endif // __PARAMETER_HPP_