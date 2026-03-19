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

#ifndef __COMPONENT_MANAGER_HPP_
#define __COMPONENT_MANAGER_HPP_

#include "core/BaseComponent.hpp"
#include "core/BaseModule.hpp"
#include "core/BaseModulator.hpp"
#include "midi/MidiController.hpp"
#include "midi/MidiEventHandler.hpp"

#include "params/ParameterMap.hpp"
#include "types/ParameterType.hpp"
#include "types/ComponentType.hpp"

#include "meta/ComponentRegistry.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class ComponentManager {
private:
    MidiController* midiController_ ;
    int nextID_ = 0 ;
    std::unordered_map<ComponentId, std::unique_ptr<BaseComponent>> components_ ;
    
    // "views" of different component groups
    std::unordered_set<ComponentId> midiHandlers_ ;
    std::unordered_set<ComponentId> midiListeners_ ;
    std::unordered_set<ComponentId> modulators_ ;
    std::unordered_set<ComponentId> modules_ ;

public:
    ComponentManager(MidiController* midiCtrl);

    template<ComponentType T>
    ComponentId create([[maybe_unused]] const std::string& name, ComponentConfig_t<T> cfg) {        
        ComponentId id = nextID_++;
        
        components_.emplace(id, std::make_unique<ComponentType_t<T>>(id, cfg));

        auto descriptor = ComponentRegistry::getComponentDescriptor(T);
        if ( descriptor.isModule() ) modules_.insert(id);
        if ( descriptor.isModulator() ) modulators_.insert(id);
        if ( descriptor.isMidiListener() ) midiListeners_.insert(id);
        if ( descriptor.isMidiHandler() ){
            midiHandlers_.insert(id);
            midiController_->addHandler(getMidiHandler(id));
        } 

        return id;
    }

    template<ComponentType T>
    ComponentType_t<T>* get(ComponentId id) const {
        BaseComponent* m = getRaw(id);
        if (!m) return nullptr ;

        if ( m->getType() == T ){
            return static_cast<ComponentType_t<T>*>(m);
        }

        return nullptr ;
    }

    BaseComponent* getRaw(ComponentId id) const ;
    BaseModule* getModule(ComponentId id) const ;
    const std::unordered_set<ComponentId>& getModuleIds() const ;
    BaseModulator* getModulator(ComponentId id) const ;
    const std::unordered_set<ComponentId>& getModulatorIds() const ;
    MidiEventHandler* getMidiHandler(ComponentId id) const ;
    const std::unordered_set<ComponentId>& getMidiHandlerIds() const ;
    MidiEventListener* getMidiListener(ComponentId id) const ;
    const std::unordered_set<ComponentId>& getMidiListenerIds() const ;

    void remove(ComponentId id);

    void reset();

    void runParameterModulation();

    // saving / loading
    json serializeComponent(BaseComponent* c) const ;
    json serializeComponents() const ;
};

#endif // __COMPONENT_MANAGER_HPP_
