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

#ifndef PARAMETER_ACCESSOR_HPP_
#define PARAMETER_ACCESSOR_HPP_

#include "types/ParameterType.hpp"
#include "params/ParameterSlot.hpp"

/*
    Provide templated access to parameter slot values.
    
    - Only constructable by a Node. 
    - Not Moveable. 
    - Should not be referenced in destructor

    Use Parameters capability/ParameterValue variant when not on hot path.
*/

template <ParameterType::Value typ>
class SlotAccessor {
public:
    using T = ParameterTraits<typ>::ValueType ;

    T value() const noexcept { 
        assert(slot_ != nullptr);
        return slot_->value(); 
    }

    T modulatedValue() const noexcept { 
        assert(slot_ != nullptr);
        return slot_->modulatedValue(); 
    }

    void setValue(T value, bool notify = true){
        slot_->setValue(value, notify);
    }

    SlotAccessor(const SlotAccessor&) = delete ;
    SlotAccessor& operator=(const SlotAccessor&) = delete ;

    SlotAccessor(SlotAccessor&&) = delete ;
    SlotAccessor& operator=(SlotAccessor&&) = delete ;

private:
    friend class ParameterAccessor ; // fills out slot
    ParameterSlot<typ>* slot_ = nullptr ;

    friend class Node ; // construction access
    SlotAccessor() = default ;
};


class ParameterAccessor {
public:
    size_t slotCount() const {
        assert(def_ != nullptr);
        return def_->slotCount();
    }

    template <ParameterType::Value typ>
    void bindSlot(SlotAccessor<typ>& slot, size_t idx) const {
        slot.slot_ = slot<typ>(idx);
    }

    const ParameterLimits& limits() const {
        return def_->limits();
    }
    
    ParamStatus setLimits(ParameterLimits limits){
        return def_->setLimits(std::move(limits));
    }

    const ParameterValue& defaultValue() const {
        return def_->defaultValue();
    }

    ParamStatus setDefaultValue(ParameterValue v){
        return def_->setDefaultValue(std::move(v));
    }

    // ==== SLOT CONVENIENCE WRAPPERS ====
    template <ParameterType::Value typ>
    ParameterTraits<typ>::ValueType slotValue(size_t idx) const noexcept {
        return slot<typ>(idx)->value();
    }

    template <ParameterType::Value typ>
    ParameterTraits<typ>::ValueType slotModulatedValue(size_t idx) const noexcept {
        return slot<typ>(idx)->modulatedValue();
    }

    ParameterAccessor(const ParameterAccessor&) = delete ;
    ParameterAccessor& operator=(const ParameterAccessor&) = delete ;

    ParameterAccessor(ParameterAccessor&&) = delete ;
    ParameterAccessor& operator=(ParameterAccessor&&) = delete ;

private:
    friend class Parameters ; // fills out definition
    ParameterDefinition* def_ = nullptr ;

    friend class Node ; // construction access
    ParameterAccessor() = default ;

    template<ParameterType::Value typ>
    ParameterSlot<typ>* slot(size_t idx) const noexcept {
        assert(def_ != nullptr);
        assert(typ == def_->config().parameter());
        assert(idx < slotCount());
        return static_cast<ParameterSlot<typ>*>(def_->slot(idx));
    }
};

#endif // PARAMETER_ACCESSOR_HPP_