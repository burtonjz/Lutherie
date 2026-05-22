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
 
#ifndef __BASE_COMPONENT_HPP_
#define __BASE_COMPONENT_HPP_

#include "types/ParameterType.hpp"
#include "types/ComponentType.hpp"
#include "params/ModulationParameter.hpp"
#include "params/ParameterListener.hpp"

#include <nlohmann/json.hpp>
#include <unordered_set>

// forward declarations (needed to support modulation derived classes)
class ParameterMap ; 
class BaseComponent ;
class ModulatorComponent ;
class AudioSignalComponent ;

using json = nlohmann::json ;
using ComponentId = int ;

class BaseComponent : public ParameterListener {
protected:
    ComponentId id_ ;
    ComponentType type_ ;
    ParameterMap* parameters_ ; 
    std::unordered_set<AudioSignalComponent*> modulationModules_ ; // used only for tracking, see SignalChain for context

public:
    BaseComponent(ComponentId id = -1, ComponentType type = ComponentType::Unknown);

    virtual ~BaseComponent();
    
    ComponentId getId() const { return id_ ; }
    ComponentType getType() const { return type_ ; }
    ParameterMap* getParameters() { return parameters_ ;}
    std::unordered_set<AudioSignalComponent*>& getModulationInputs() ;

    /* depth/modulation are managed from component instead of parameterMap, to allow for
     parent/child component setups. Non-virtual functions in this section have
     default behaviors that should ALWAYS happen and custom functionality happens through
     protected NVI functions 
    */
    void setParameterModulation(ParameterType p, ModulatorComponent* m, ModulationData d = {});
    void setParameterDepthModulation(ParameterType p, ModulatorComponent* m, ModulationData d = {});
    void removeParameterModulation(ParameterType p);
    void removeParameterDepthModulation(ParameterType p);
    virtual ModulatorComponent* getParameterModulator(ParameterType p) const ;
    virtual ModulatorComponent* getParameterDepthModulator(ParameterType p) const ;
    virtual double getParameterDepth(ParameterType p) const ;
    virtual void setParameterDepth(ParameterType p, double depth);
    virtual ModulationStrategy getParameterModulationStrategy(ParameterType p) const ;
    virtual void setParameterModulationStrategy(ParameterType p, ModulationStrategy strat);

    // this function runs modulation on all internal parameters
    virtual void updateParameters();

protected:
    virtual void onSetParameterModulation(ParameterType p, ModulatorComponent* m, ModulationData d );
    virtual void onRemoveParameterModulation(ParameterType p);
    virtual void onSetParameterDepthModulation(ParameterType p, ModulatorComponent* m, ModulationData d );
    virtual void onRemoveParameterDepthModulation(ParameterType p);
    
};

#endif // __BASE_COMPONENT_HPP_