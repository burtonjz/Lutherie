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

#include "core/BaseComponent.hpp"
#include "params/ModulationParameter.hpp"

struct ModulationTarget {
    BaseComponent* component ;
    ParameterType param ;
    bool depth = false ;
};

inline bool operator<(const ModulationTarget& a, const ModulationTarget& b){
    if ( a.component != b.component ){
        return a.component < b.component ;
    }

    if ( a.param != b.param ){
        return a.param < b.param ;
    }

    return a.depth < b.depth ;
}

#include <set>

/**
 * @brief base class for all Modulators.
*/
class BaseModulator : public virtual BaseComponent {
protected:
    std::set<ModulationParameter> requiredParams_ ; 
    std::set<ModulationTarget> modulated_ ; 

    void addModulationTarget(ModulationTarget t){
        modulated_.insert(t);
    }

    void removeModulationTarget(ModulationTarget t){
        modulated_.erase(t);
    }

public:
    BaseModulator(){};

    virtual ~BaseModulator() = default ;

    virtual double modulate(double value, ModulationData* mData ) const = 0 ;

    virtual const std::set<ModulationParameter>& getRequiredModulationParameters() const {
        return requiredParams_ ;
    }
    
    virtual ModulatorRange getModulatorRange() const {
        return ModulatorRange::UNKNOWN ;
    }

    std::set<ModulationTarget>& getModulationTargets(){
        return modulated_ ;
    }

    // allows recipricol tracking of modulation targets
    friend void BaseComponent::removeParameterModulation(ParameterType p);
    friend void BaseComponent::setParameterModulation(ParameterType p, BaseModulator* m, ModulationData d);
    friend void BaseComponent::removeParameterDepthModulation(ParameterType p);
    friend void BaseComponent::setParameterDepthModulation(ParameterType p, BaseModulator* m, ModulationData d);
};

#endif // __MODULATOR_HPP_
