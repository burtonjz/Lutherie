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

#include "capabilities/MidiListener.hpp"
#include <algorithm>

void MidiListener::addHandler(MidiHandler* handler){
    if ( std::find(handlers_.begin(), handlers_.end(), handler) != handlers_.end() ) return ;
    handlers_.push_back(handler);
    onHandlerAdded();
}

void MidiListener::removeHandler(MidiHandler* handler){
    auto it = std::find(handlers_.begin(), handlers_.end(), handler);
    if ( it != handlers_.end() ){
        handlers_.erase(it);
    }
    onHandlerRemoved();
}

std::vector<MidiHandler*> MidiListener::getHandlers() const {
    return handlers_ ;
}

size_t MidiListener::getNumHandlers() const {
    return handlers_.size();
}

void MidiListener::onKeyPressed([[maybe_unused]] const ActiveNote* note, [[maybe_unused]] bool repress){
} 
void MidiListener::onKeyReleased([[maybe_unused]] ActiveNote anote){
}
void MidiListener::onKeyOff([[maybe_unused]] ActiveNote anote){
}
void MidiListener::onPitchbend([[maybe_unused]] uint16_t pitchbend ){
}

// void MidiListener::onMidiControlEvent(MidiControlMessage messageType, uint8_t value){
// }

void MidiListener::onHandlerAdded(){}
void MidiListener::onHandlerRemoved(){}