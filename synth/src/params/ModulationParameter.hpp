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

#ifndef __MODULATION_PARAMETER_HPP_
#define __MODULATION_PARAMETER_HPP_

#include <array>
#include <bitset>

/**
 * @brief valid key values for handling additional modulation variables
 * 
 * The parameter class stores a modulation function in it that can receive a map as a parameter so that additional context can be given to modulators
 * while still allowing the modulation function definition to be standardized. These keys define various modulation function variables.
*/
enum class ModulationParameter : int {
    MIDI_NOTE,
    LAST_MIDI_NOTE,
    INITIAL_VALUE, 
    LAST_OUTPUT,
    LAST_STATE,
    FILTER_STATE_1,
    FILTER_STATE_2,
    N_PARAMETERS
};

constexpr int N_MODULATION_PARAMETERS = static_cast<int>(ModulationParameter::N_PARAMETERS);

struct ModulationData {
    std::array<double, N_MODULATION_PARAMETERS> data ;
    std::bitset<N_MODULATION_PARAMETERS> inUse ;

    bool has(ModulationParameter p) const {
        return inUse.test(static_cast<size_t>(p));
    }

    void set(ModulationParameter p, double value){
        data[static_cast<size_t>(p)] = value ;
        inUse.set(static_cast<size_t>(p));
    }

    double get(ModulationParameter p) const {
        return data[static_cast<size_t>(p)] ;
    }

    bool isEmpty() const {
        return inUse.any() ;
    }
};

#endif // __MODULATION_PARAMETER_HPP_