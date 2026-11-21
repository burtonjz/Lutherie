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

template <ParameterType::Value typ>
class ParameterAccessor {
public:
    using T = ParameterTraits<typ>::ValueType ;

    T value() const { 
        assert(slot_ != nullptr);
        return slot_->value(); 
    }

    T modulatedValue() const { 
        assert(slot_ != nullptr);
        return slot_->modulatedValue(); 
    }

    ParameterAccessor() = default ;

    ParameterAccessor(const ParameterAccessor&) = delete ;
    ParameterAccessor& operator=(const ParameterAccessor&) = delete ;

    ParameterAccessor(ParameterAccessor&&) = delete ;
    ParameterAccessor& operator=(ParameterAccessor&&) = delete ;

private:
    friend class Parameters ;

    ParameterSlot<typ>* slot_ = nullptr ;

    ParameterAccessor(ParameterSlot<typ>& slot):
        slot_(&slot)
    {}
};


#endif // PARAMETER_ACCESSOR_HPP_