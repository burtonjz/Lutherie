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
#include "core/AudioIO.hpp"
#include "core/BufferIO.hpp"
#include "core/ModulatorComponent.hpp"
#include "core/FileComponent.hpp"
#include "core/AudioProbe.hpp"
#include "midi/MidiHandler.hpp"
#include "midi/MidiListener.hpp"

#include "midi/MidiController.hpp"

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
    int nextID_ = 0 ;
    std::unordered_map<int, std::unique_ptr<BaseComponent>> components_ ;
    
    // "views" of different component groups
    std::unordered_set<int> allIds_ ;
    std::unordered_set<int> midiHandlers_ ;
    std::unordered_set<int> midiListeners_ ;
    std::unordered_set<int> modulators_ ;
    std::unordered_set<int> audioSignals_ ;
    std::unordered_set<int> audioBuffers_ ;
    std::unordered_set<int> audioProbes_ ;
    std::unordered_set<int> fileComponents_ ;

    explicit ComponentManager();

public:
    static ComponentManager* instance();

    ComponentManager(const ComponentManager&) = delete ;
    ComponentManager& operator=(const ComponentManager&) = delete ;
    ComponentManager(ComponentManager&&) = delete ;
    ComponentManager& operator=(ComponentManager&&) = delete ;

    template<ComponentType T>
    int create([[maybe_unused]] const std::string& name, ComponentConfig_t<T> cfg) {        
        int id = nextID_++;
        
        components_.emplace(id, std::make_unique<ComponentType_t<T>>(id, cfg));

        auto descriptor = ComponentRegistry::getComponentDescriptor(T);
        allIds_.insert(id);
        if ( descriptor.isSignalComponent() ) audioSignals_.insert(id);
        if ( descriptor.isBufferComponent() ) audioBuffers_.insert(id);
        if ( descriptor.isModulator() ) modulators_.insert(id);
        if ( descriptor.isMidiListener() ) midiListeners_.insert(id);
        if ( descriptor.isMidiHandler() ){
            midiHandlers_.insert(id);
            MidiController::instance()->addHandler(getMidiHandler(id));
        } 
        if ( descriptor.isAudioProbe() ){
            audioProbes_.insert(id);
        }
        if ( descriptor.hasFile ){
            fileComponents_.insert(id);
        }

        return id;
    }

    template<ComponentType T>
    ComponentType_t<T>* get(int id) const {
        BaseComponent* m = getRaw(id);
        if (!m) return nullptr ;

        if ( m->getType() == T ){
            return static_cast<ComponentType_t<T>*>(m);
        }

        return nullptr ;
    }

    BaseComponent* getRaw(int id) const ;
    AudioIO* getSignalComponent(int id) const ;
    BufferIO* getBufferComponent(int id) const ;
    ModulatorComponent* getModulator(int id) const ;
    MidiHandler* getMidiHandler(int id) const ;
    MidiListener* getMidiListener(int id) const ;
    AudioProbe* getAudioProbe(int id) const ;
    FileComponent* getFileComponent(int id) const ;

    const std::unordered_set<int>& getints() const ;
    const std::unordered_set<int>& getSignalints() const ;
    const std::unordered_set<int>& getModulatorIds() const ;
    const std::unordered_set<int>& getMidiHandlerIds() const ;
    const std::unordered_set<int>& getMidiListenerIds() const ;
    const std::unordered_set<int>& getAudioProbeIds() const ;
    const std::unordered_set<int>& getFileints() const ;

    void remove(int id);

    void reset();

    /**
     * @brief runs parameter modulation all non-module components
     */
    void runParameterModulation();

    void flushAudioProbes();

    // saving / loading
    json serializeComponent(BaseComponent* c) const ;
    json serializeComponents() const ;
    void getComponentConnections(int id, std::vector<ConnectionRequest>& requests) const ;

private:
    void getComponentMidiConnections(int id, std::vector<ConnectionRequest>& requests) const ;
    void getComponentAudioConnections(int id, std::vector<ConnectionRequest>& requests) const ;
    void getComponentModulationConnections(int id, std::vector<ConnectionRequest>& requests) const ;
    void getComponentBufferConnections(int id, std::vector<ConnectionRequest>& requests) const ;
};

#endif // __COMPONENT_MANAGER_HPP_
