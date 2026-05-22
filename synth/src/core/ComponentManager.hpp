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
#include "core/AudioSignalComponent.hpp"
#include "core/AudioBufferComponent.hpp"
#include "core/ModulatorComponent.hpp"
#include "dsp/Analyzer.hpp"
#include "midi/MidiController.hpp"
#include "midi/MidiEventHandler.hpp"

#include "params/ParameterMap.hpp"
#include "types/ParameterType.hpp"
#include "types/ComponentType.hpp"

#include "meta/ComponentRegistry.hpp"
#include "requests/ConnectionRequest.hpp"

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
    std::unordered_set<ComponentId> allIds_ ;
    std::unordered_set<ComponentId> midiHandlers_ ;
    std::unordered_set<ComponentId> midiListeners_ ;
    std::unordered_set<ComponentId> modulators_ ;
    std::unordered_set<ComponentId> audioSignals_ ;
    std::unordered_set<ComponentId> audioBuffers_ ;
    std::unordered_set<ComponentId> analyzers_ ;

public:
    ComponentManager(MidiController* midiCtrl);

    template<ComponentType T>
    ComponentId create([[maybe_unused]] const std::string& name, ComponentConfig_t<T> cfg) {        
        ComponentId id = nextID_++;
        
        components_.emplace(id, std::make_unique<ComponentType_t<T>>(id, cfg));

        auto descriptor = ComponentRegistry::getComponentDescriptor(T);
        SPDLOG_DEBUG("signal={}, buffer={}, modulator={}, midiListener={}, midiHandler={}, analyzer={}",  
            descriptor.isSignalComponent(),
            descriptor.isBufferComponent(),
            descriptor.isModulator(),
            descriptor.isMidiListener(),
            descriptor.isMidiHandler(),
            descriptor.isAnalyzer()
        );
        allIds_.insert(id);
        if ( descriptor.isSignalComponent() ) audioSignals_.insert(id);
        if ( descriptor.isBufferComponent() ) audioBuffers_.insert(id);
        if ( descriptor.isModulator() ) modulators_.insert(id);
        if ( descriptor.isMidiListener() ) midiListeners_.insert(id);
        if ( descriptor.isMidiHandler() ){
            midiHandlers_.insert(id);
            midiController_->addHandler(getMidiHandler(id));
        } 
        if ( descriptor.isAnalyzer() ){
            analyzers_.insert(id);
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
    AudioSignalComponent* getSignalComponent(ComponentId id) const ;
    AudioBufferComponent* getBufferComponent(ComponentId id) const ;
    ModulatorComponent* getModulator(ComponentId id) const ;
    MidiEventHandler* getMidiHandler(ComponentId id) const ;
    MidiEventListener* getMidiListener(ComponentId id) const ;
    Analyzer* getAnalyzer(ComponentId id) const ;

    const std::unordered_set<ComponentId>& getComponentIds() const ;
    const std::unordered_set<ComponentId>& getSignalComponentIds() const ;
    const std::unordered_set<ComponentId>& getModulatorIds() const ;
    const std::unordered_set<ComponentId>& getMidiHandlerIds() const ;
    const std::unordered_set<ComponentId>& getMidiListenerIds() const ;
    const std::unordered_set<ComponentId>& getAnalyzerIds() const ;

    void remove(ComponentId id);

    void reset();

    /**
     * @brief runs parameter modulation all non-module components
     */
    void runParameterModulation();

    void runAnalyzers();

    // saving / loading
    json serializeComponent(BaseComponent* c) const ;
    json serializeComponents() const ;
    void getComponentConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const ;

private:
    void getComponentMidiConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const ;
    void getComponentSignalConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const ;
    void getComponentModulationConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const ;
    void getComponentBufferConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const ;
};

#endif // __COMPONENT_MANAGER_HPP_
