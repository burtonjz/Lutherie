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

#ifndef MODULATION_SOURCE_HPP_
#define MODULATION_SOURCE_HPP_

#include "params/ModulationParameter.hpp"
#include "types/ModulationStrategy.hpp"
#include <initializer_list>

class ModulationSource {
public:
    virtual double modulate(double value, ModulationData* mdat) const = 0 ;
    
    ModulatorRange range() const {
        return range_ ;
    }
    
    std::span<const ModulationParameter> expectedParameters() const {
        return expected_ ;
    }

    virtual ~ModulationSource() = default ;

protected:
    ModulationSource(ModulatorRange range, std::initializer_list<ModulationParameter> expected):
        expected_(expected),
        range_(range)
    {}

private:
    std::vector<ModulationParameter> expected_ ;
    ModulatorRange range_ ;
};

#endif // MODULATION_SOURCE_HPP_