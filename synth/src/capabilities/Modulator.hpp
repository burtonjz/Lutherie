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

#ifndef __MODULATOR_HPP_
#define __MODULATOR_HPP_

#include "params/ModulationParameter.hpp"
#include "routing/ModulationTarget.hpp"

// forward declarations
class Node ;

#include <set>

/**
 * @brief base class for all Modulators.
*/
class Modulator {
protected:
    std::set<ModulationParameter> expectedParams_ ; 
    std::set<ModulationTarget> modulated_ ; 

    void addModulationTarget(ModulationTarget t){
        modulated_.insert(t);
    }

    void removeModulationTarget(ModulationTarget t){
        modulated_.erase(t);
    }

public:
    Modulator(){};

    virtual ~Modulator() = default ;

    virtual double modulate(double value, ModulationData* mData ) const = 0 ;

    const std::set<ModulationParameter>& getRequiredModulationParameters() const {
        return expectedParams_ ;
    }
    
    virtual ModulatorRange getModulatorRange() const {
        return ModulatorRange::UNKNOWN ;
    }

    std::set<ModulationTarget>& getModulationTargets(){
        return modulated_ ;
    }

    // allows recipricol tracking of modulation targets
    friend void BaseComponent::removeParameterModulation(ParameterType p);
    friend void BaseComponent::setParameterModulation(ParameterType p, ModulatorComponent* m, ModulationData d);
    friend void BaseComponent::removeParameterDepthModulation(ParameterType p);
    friend void BaseComponent::setParameterDepthModulation(ParameterType p, ModulatorComponent* m, ModulationData d);
};

#endif // __MODULATOR_HPP_
