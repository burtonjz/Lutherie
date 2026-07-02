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

#include "components/Chopper.hpp"
#include "params/ParameterMap.hpp"

Chopper::Chopper(ComponentId id,[[maybe_unused]] ChopperConfig cfg):
    BaseComponent(id, ComponentType::Chopper),
    AudioBufferComponent(1,1)
{
    parameters_->addCollection<ParameterType::SAMPLE>({0,0});
    parameters_->getCollection<ParameterType::SAMPLE>()->addListener(this);
}

void Chopper::onInputConnect(){
    onInputUpdated();
}

void Chopper::onInputDisconnect(){
    onInputUpdated();
}

void Chopper::onInputUpdated(){
    auto collection = parameters_->getCollection<ParameterType::SAMPLE>();
    if ( collection->size() != 2 ){
        SPDLOG_ERROR("Chopper collection size is not 2. This is a programming bug.");
        return ;
    }

    const auto& inp = getInputs(0);
    if ( inp.size() > 0 ){
        BufferConnection conn = *inp.begin();
        const auto& buf = conn.component->getBuffer(conn.index);    

        collection->setMaxValue(buf.size()-1);
        collection->setValue(0,0); // start
        collection->setValue(1,buf.size()-1); // end 
        collection->notifyListeners();
    } else {
        collection->setMaxValue(0);
        collection->setValue(0,0); // start
        collection->setValue(1,0); // end
        collection->notifyListeners();
    }

    triggerComponentSync();
}

void Chopper::onParameterChanged(ParameterType p, bool isCollection){
    SPDLOG_DEBUG("onParameterChanged: ParameterType={}, isCollection={}", GET_PARAMETER_TRAIT_MEMBER(p, name), isCollection);
    if ( !isCollection || p != ParameterType::SAMPLE ) return ;

    const auto& inp = getInputs(0);
    if ( inp.size() == 0 ){
        SPDLOG_WARN("parameter update received, but no input buffer is available. ignoring.");
        return ;
    } 
        
    BufferConnection conn = *inp.begin();
    const auto& buf = conn.component->getBuffer(conn.index);    

    // guard against empty inbound buffer
    if ( buf.empty() ){
        SPDLOG_WARN("parameter update received, but inbound buffer is empty.");
        buffers_[0].clear();
        notifyDownstream(0);
        return ;
    }

    auto collection = parameters_->getCollection<ParameterType::SAMPLE>();
    if ( collection->size() != 2 ){
        SPDLOG_ERROR("Chopper collection size is not 2. This is a programming bug.");
        return ;
    }

    int start = collection->getValue(0);
    int end   = collection->getValue(1);

    buffers_[0].assign(
        buf.begin() + start,
        buf.begin() + end
    );

    SPDLOG_DEBUG("Chopper buffer updated: startSample={}, endSample={}, size={}", start, end, buffers_[0].size());
    notifyDownstream(0);
}