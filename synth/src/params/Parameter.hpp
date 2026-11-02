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
#include "interfaces/ParameterListener.hpp"
#include "interfaces/ModulationSource.hpp"
#include "params/ParameterSlot.hpp"

#include <spdlog/spdlog.h>
#include <variant>
#include <vector>

// forward declaration
template <ParameterType typ> class Parameter ;

class ParameterBase {
protected:
    ParameterType type_ ;
    bool modulatable_ ;
    const ModulationSource* modulator_ ;
    ModulationStrategy modStrategy_ ;
    ModulationData modData_ ;

    bool hasDepth_ ;
    bool depthInitialized_ ;

    std::vector<ParameterListener*> listeners_ ;

    uint8_t midiControl_ = 128 ;

public:
    ParameterBase(
        ParameterType typ, 
        bool modulatable,
        const ModulationSource* modulator = nullptr,
        ModulationData modData = {}
    ):
        type_(typ),
        modulatable_(modulatable),
        modulator_(modulator),
        modStrategy_(ModulationStrategy::NONE),
        modData_(modData)    
    {}

    virtual ~ParameterBase() = default ;
    ParameterBase(const ParameterBase&) = delete ;
    ParameterBase& operator=(const ParameterBase&) = delete ;

    virtual void setModulatable(bool modulatable){
        modulatable_ = modulatable ;
    }

    virtual bool isModulatable(){ 
        return modulatable_ ; 
    }

    void setModulation(const ModulationSource* modulator, ModulationData modData){
        if ( modulator){
            modData_ = modData ;
            modulator_ = modulator ;
            modStrategy_ = GET_PARAMETER_MODULATION_STRATEGY(type_, modulator_->getModulatorRange());
        } else {
            SPDLOG_WARN("Modulation was not set because the modulator was a null pointer.");
        }
    }

    void removeModulation(){
        modulator_ = nullptr ;
        modData_ = {} ;
    }

    const ModulationSource* getModulator() const { 
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
            const ModulationSource* modulator = nullptr, ModulationData modData = {}
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

        bool setValue(ValueType value, bool notify = true){
            value_ = limitToRange(value);
            setInstantaneousValue(value_, notify);
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
            if constexpr ( std::is_same_v<ValueType, bool> ){
                return ;
            } else {
                // modulate this parameter's depth, if it exists
                auto* depth = getDepth();
                double instDepth = 1.0 ;
                if ( depth  && depth->isModulatable() ){
                    depth->modulate();
                    instDepth = depth->getInstantaneousValue();
                }

                // now, modulate this parameter itself
                ValueType v ;
                if ( applyStrategy(value_, instDepth, modData_, v) ){
                    setInstantaneousValue(v);
                }
            }
        }

        Parameter<ParameterType::DEPTH>* getDepth(){
            return depth_ ;
        }

        // Slots allow us to specify just the pieces of a parameter 
        // that are used by individual voices. 
        using Slot = ParameterSlot<ValueType>;

        // Initialise a voice slot from this parameter's configuration.
        void seedSlot(Slot& s, ValueType base) const {
            s.base = limitToRange(base);
            s.value.instantaneous = s.base ;
            s.value.modData = modData_ ;
            if constexpr ( typ != ParameterType::DEPTH ){
                if ( depth_ ){
                    s.depth.instantaneous = depth_->getValue();
                    s.depth.modData = *depth_->getModulationData();
                }
            }
        }

        void evaluate(Slot& s) const {
            if constexpr ( std::is_same_v<ValueType, bool> ){
                s.value.instantaneous = limitToRange(s.base);
            } else {
                double instDepth = 1.0 ;
                if constexpr (typ != ParameterType::DEPTH) {
                    if (depth_ && depth_->isModulatable()) {
                        s.depth.instantaneous =
                            depth_->evaluateWith(depth_->getValue(), 1.0, s.depth.modData);
                        instDepth = s.depth.instantaneous ;
                    }
                }
                s.value.instantaneous = evaluateWith(s.base, instDepth, s.value.modData);
            }
        }

        // Pure evaluation from explicit inputs. Unmodulated returns the limited base.
        ValueType evaluateWith(ValueType base, double instDepth, ModulationData& md) const {
            ValueType out ;
            if ( applyStrategy(base, instDepth, md, out) ) return limitToRange(out);
            return limitToRange(base);
        }

    private:
        void setInstantaneousValue(ValueType v, bool notify = true){
            v = limitToRange(v);
            if ( v != instantaneousValue_ ){
                instantaneousValue_ = v ;
                if ( notify ) notifyListeners();
            }
        }

        bool applyStrategy(
            [[maybe_unused]] ValueType base, [[maybe_unused]] double instDepth,
            [[maybe_unused]] ModulationData& md, [[maybe_unused]] ValueType& out
        ) const {
            if constexpr ( std::is_same_v<ValueType, bool> ){
                return false ;
            } else {
                if ( !modulatable_ || !modulator_ ) return false ;

                switch(modStrategy_){
                case ModulationStrategy::ADDITIVE:
                    out = static_cast<ValueType>(base + instDepth * modulator_->modulate(base, &md));
                    return true ;
                case ModulationStrategy::MULTIPLICATIVE_ZERO:
                    out = static_cast<ValueType>(base * instDepth * modulator_->modulate(base, &md));
                    return true ;
                case ModulationStrategy::MULTIPLICATIVE_UNITY:
                    out = static_cast<ValueType>(base * (1 + instDepth * modulator_->modulate(base, &md)));
                    return true ;
                case ModulationStrategy::EXPONENTIAL:
                    out = static_cast<ValueType>(base * exp2f(instDepth * modulator_->modulate(base, &md)));
                    return true ;
                case ModulationStrategy::REPLACE:
                    out = static_cast<ValueType>(instDepth * modulator_->modulate(base, &md));
                    return true ;
                default:
                    return false ;
                }
            }
        }
};


#endif // __PARAMETER_HPP_