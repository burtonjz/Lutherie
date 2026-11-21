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

#ifndef PARAMETER_SLOT_HPP_
#define PARAMETER_SLOT_HPP_

#include "types/ParameterType.hpp"
#include "params/ParameterDefinition.hpp"

#include <cstdint>

class ParameterSlotBase {
protected:
    ParameterDefinition* def_ ;
    uint16_t index_ ;

public:
    ParameterSlotBase(ParameterDefinition& def, uint16_t index): 
        def_(&def), 
        index_(index)
    {}

    virtual void assignFrom(const ParameterSlotBase&) = 0 ;

    virtual ~ParameterSlotBase() = default ;
    ParameterSlotBase(const ParameterSlotBase&) = delete ;
    ParameterSlotBase& operator=(const ParameterSlotBase&) = delete ;
 
    const ParameterDefinition& definition() const { return *def_; }
    uint16_t index() const { return index_; }
 
    virtual ParameterValue varValue() const = 0 ;
    virtual void setValue(ParameterValue v) = 0 ;
    virtual void setValuePercent(double perc) = 0 ;
    virtual void incrementValueWrap() = 0 ;
    virtual void resetValue() = 0 ;
    virtual void modulate() = 0 ;
    virtual void resetModulation() = 0 ;   
};

template <ParameterType::Value typ>
class ParameterSlot final : public ParameterSlotBase {
    using ValueType = typename ParameterTraits<typ>::ValueType ;

private:
    struct DepthState {
        double value = 1.0 ;
        double modulated = 1.0 ;
        ModulationData modData ;
    };

    ValueType value_ ;
    ValueType modulated_ ;
    ValueType minimum_ ;
    ValueType maximum_ ;
    ValueType default_ ;
    ModulationData modData_ ;
    DepthState depth_ ;
    uint32_t seenRevision_ = 0 ;
    bool seenRevisionValid_ = false ;

public:
 
    ParameterSlot(ParameterDefinition& def, uint16_t index = 0): 
        ParameterSlotBase(def, index),
        value_(std::get<ValueType>(def.config().defaultValue())),
        modulated_(value_),
        modData_(def.modulationTemplate())
    {
        refreshCache();
    }
 
    ValueType value() const { return value_ ; }
    ValueType modulatedValue() const { return modulated_ ; }
    ValueType minimum(){ refreshCache(); return minimum_ ; }
    ValueType maximum(){ refreshCache(); return maximum_ ; }
    ValueType defaultValue(){ refreshCache(); return default_ ; }
    ParameterValue varValue() const override { return value_ ; }
 
    void setValue(ValueType v, bool notify = true){
        refreshCache();
        value_ = limitToRange(v);
        if ( !def_->isModulating() ) setModValue(value_, notify);
        else if (notify) def_->notifyObservers(index_);
    }

    void setValue(ParameterValue v) override { 
        setValue(std::get<ValueType>(v)); 
    }
 
    void setValuePercent(double perc) override {
        refreshCache();
        const double raw = static_cast<double>(maximum_ - minimum_) 
            * perc + static_cast<double>(minimum_);
        if constexpr ( std::is_integral_v<ValueType> ){
            setValue(static_cast<ValueType>(std::llround(raw)));
        } else {
            setValue(static_cast<ValueType>(raw));
        }                                        
    }
 
    void incrementValueWrap() override {
        refreshCache();
        if constexpr ( std::is_same_v<ValueType, bool> ){
            setValue(!value_);
        } else {
            ValueType next = static_cast<ValueType>(value_ + 1);
            setValue(next > maximum_ ? minimum_ : next);
        }
    }
 
    void resetValue() override { 
        refreshCache(); 
        setValue(default_); 
    }
 
    double depth() const { 
        return depth_.value ; 
    }

    void setDepth(double d){ 
        depth_.value = d ; 
        depth_.modulated = d ; 
    }
 
    void modulate() override {
        if constexpr ( std::is_same_v<ValueType, bool> ) {
            return ;
        } else {
            if ( !def_->isModulating() ) return ;
            refreshCache();

            // first, modulate depth (if possible)
            double instDepth = 1.0 ;
            if (
                const auto* dd = def_->depth(); 
                dd && dd->isModulating()
            ){
                double out = depth_.value ;
                if ( 
                    dd->applyStrategy<double>(
                        depth_.value, 1.0, 
                        depth_.modData, out
                    )
                ){
                    depth_.modulated = out ;
                }
                instDepth = depth_.modulated ;
            } else {
                instDepth = depth_.value ;
            }
 
            ValueType v ; 
            if (
                def_->applyStrategy<ValueType>(
                    value_, instDepth, 
                    modData_, v
                )
            ){
                setModValue(v);
            }
        }
    }
 
    void resetModulation() override {
        modData_ = def_->modulationTemplate();
        modulated_ = value_ ;
        depth_ = DepthState{depth_.value, depth_.value, {}};
    }

    void assignFrom(const ParameterSlotBase& src) override {
        if ( &src == this ) return ;
        assert(&src.definition() == def_);
        const auto& s = static_cast<const ParameterSlot&>(src);

        value_     = s.value_ ;
        modulated_ = s.modulated_ ;
        modData_   = s.modData_ ;
        depth_     = s.depth_ ;
    }
 
private:
    ValueType limitToRange(ValueType v) const {
        return v < minimum_ 
            ? minimum_ 
            : (v > maximum_ ? maximum_ : v);
    }
 
    void setModValue(ValueType v, bool notify = true){
        v = limitToRange(v);
        if ( v != modulated_ ){
            modulated_ = v ;
            if ( notify ) def_->notifyObservers(index_);
        }
    }
 
    void refreshCache() {
        if ( seenRevision_ == def_->revision() && seenRevisionValid_ ) return ;
        const ParameterLimits& lim = def_->config().limits();
        minimum_ = std::get<ValueType>(lim.minimum);
        maximum_ = std::get<ValueType>(lim.maximum);
        default_ = std::get<ValueType>(def_->config().defaultValue());
        value_   = limitToRange(value_);
        seenRevision_ = def_->revision();
        seenRevisionValid_ = true ;
    }
};

#endif // PARAMETER_SLOT_HPP_