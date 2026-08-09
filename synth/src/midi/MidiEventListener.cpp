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

#include "midi/MidiEventListener.hpp"
#include <algorithm>

void MidiEventListener::addHandler(MidiEventHandler* handler){
    if ( std::find(handlers_.begin(), handlers_.end(), handler) != handlers_.end() ) return ;
    handlers_.push_back(handler);
    onHandlerAdded();
}

void MidiEventListener::removeHandler(MidiEventHandler* handler){
    auto it = std::find(handlers_.begin(), handlers_.end(), handler);
    if ( it != handlers_.end() ){
        handlers_.erase(it);
    }
    onHandlerRemoved();
}

std::vector<MidiEventHandler*> MidiEventListener::getHandlers() const {
    return handlers_ ;
}

size_t MidiEventListener::getNumHandlers() const {
    return handlers_.size();
}

void MidiEventListener::onKeyPressed([[maybe_unused]] const ActiveNote* note, [[maybe_unused]] bool repress){
} 
void MidiEventListener::onKeyReleased([[maybe_unused]] ActiveNote anote){
}
void MidiEventListener::onKeyOff([[maybe_unused]] ActiveNote anote){
}
void MidiEventListener::onPitchbend([[maybe_unused]] uint16_t pitchbend ){
}

// void MidiEventListener::onMidiControlEvent(MidiControlMessage messageType, uint8_t value){
// }

void MidiEventListener::onHandlerAdded(){}
void MidiEventListener::onHandlerRemoved(){}