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

#ifndef __SIGNAL_CONTROLLER_HPP_
#define __SIGNAL_CONTROLLER_HPP_

#include "core/BaseModule.hpp"
#include "core/ComponentManager.hpp"
#include "signal/SignalChain.hpp"

class SignalController {
private:
    ComponentManager* components_ ;
    SignalChain signalChain_ ;

public:
    SignalController(ComponentManager* components):
        components_(components)
    {}

    // signal chain functions
    void connect(BaseModule* from, size_t fromIndex, BaseModule* to, size_t toIndex){
        if (!from) return ;
        if (!to) return ;
        to->connectInput(from,fromIndex, toIndex);
        signalChain_.calculateTopologicalOrder();
    }

    void disconnect(BaseModule* from, size_t fromIndex, BaseModule* to, size_t toIndex){
        if (!from) return ;
        if (!to) return ;
        to->disconnectInput(from, fromIndex, toIndex);
        signalChain_.calculateTopologicalOrder();
    }

    void registerSink(BaseModule* output, size_t index){
        signalChain_.addSink(output, index);
        signalChain_.calculateTopologicalOrder();
    }

    void unregisterSink(BaseModule* output, size_t index){
        signalChain_.removeSink(output, index);
        signalChain_.calculateTopologicalOrder();
    }

    const std::unordered_set<SignalConnection, ConnectionHash>& getSinks() const {
        return signalChain_.getSinks() ;
    }

    double processFrame(){
        auto chain = signalChain_.getModuleChain();
        auto sinks = signalChain_.getSinks();

        double output = 0 ; 
        for (BaseModule*  mod : chain){
            mod->updateParameters();
            mod->tick();
            mod->calculateSample();

            // if an output index is a sink, add it to the final output
            for ( size_t i = 0; i < mod->getNumOutputs(); ++i ){
                if ( sinks.count({mod,i}) ){
                    output += mod->getCurrentSample(i) ;
                }
            }
        }

        return output ;
    }

    void clearBuffer(){
        for ( auto id : components_->getModuleIds() ){
            auto m = components_->getModule(id);
            if ( m->isGenerative()){
                m->clearBuffer();
            }
        }
    }

    void updateProcessingGraph(){
        signalChain_.calculateTopologicalOrder();
    }

    void reset(){
        signalChain_.reset() ;
    }
};

#endif // __SIGNAL_CONTROLLER_HPP_