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

#include "components/BufferReverser.hpp"

BufferReverser::BufferReverser(ComponentId id, [[maybe_unused]] BufferReverserConfig cfg):
    BaseComponent(id, ComponentType::BufferReverser),
    AudioBufferComponent(1,1)
{
}

void BufferReverser::onInputConnect(){
    onInputUpdated();
}

void BufferReverser::onInputDisconnect(){
    onInputUpdated();
}

void BufferReverser::onInputUpdated(){
    const auto& inp = getInputs(0);
    if ( inp.size() == 0 ){
        if ( buffers_.at(0).size() > 0 ){
            buffers_[0].clear();
            notifyDownstream(0);
        }
        return ;
    } 
        
    BufferConnection conn = *inp.begin();
    const auto& buf = conn.component->getBuffer(conn.index);    

    // guard against empty inbound buffer
    if ( buf.empty() ){
        SPDLOG_WARN("inbound buffer is empty.");
        buffers_[0].clear();
        notifyDownstream(0);
        return ;
    }

    
    buffers_[0].assign(buf.rbegin(), buf.rend());
    notifyDownstream(0);
}
