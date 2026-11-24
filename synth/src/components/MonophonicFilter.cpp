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

#include "components/MonophonicFilter.hpp"

#include "capabilities/Capability.hpp"
#include "capabilities/MidiIO.hpp"     // IWYU pragma: keep

MonophonicFilter::MonophonicFilter(int id, MonophonicFilterConfig):
    Node(id, NodeType::COMPONENT),
    ComponentInfo(ComponentType::MonophonicFilter),
    MidiProcessor()
{
    addCapability<Capability::MidiIO>(*this, 1, 1);
}

void MonophonicFilter::handleKeyPressed(MidiNote n, size_t ch){
    uint8_t midiValue = n.note();
    noteStack_.push_back(midiValue);
    
    if ( noteStack_.size() > 1 ){
        // release previous note
        uint8_t lastValue = noteStack_[noteStack_.size() - 2];
        push(MidiNote(
            ch, lastValue, 
            0, NoteStatus::RELEASED
        ));
    } 
    
    push(std::move(n), ch);
}

void MonophonicFilter::handleKeyReleased(MidiNote n, size_t ch){
    uint8_t midiValue = n.note();
    
    auto it = std::find(noteStack_.begin(), noteStack_.end(), midiValue);
    if ( it == noteStack_.end() ) return ;
    
    bool isActiveNote = ( it == noteStack_.end() - 1 );
    noteStack_.erase(it);

    if ( !isActiveNote ) return ;
        
    // release active
    push(std::move(n), ch);    
    if ( noteStack_.empty() ) return ;

    // trigger last
    uint8_t last = noteStack_.back();
    uint8_t vel = note(last,ch).velocity() ;
    vel = vel > 0 ? vel : 100 ;
    push(MidiNote(
        ch, last, 
        vel, NoteStatus::PRESSED
    )); 
}