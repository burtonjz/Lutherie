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

#ifndef MIDI_LISTENER_HPP_
#define MIDI_LISTENER_HPP_

#include "midi/MidiNote.hpp"
#include <vector>

//forward declaration
class MidiHandler ;

class MidiListener {
    friend class MidiHandler ;
private:
    std::vector<MidiHandler*> handlers_ ;

    void addHandler(MidiHandler* handler);
    void removeHandler(MidiHandler* handler);

public:
    virtual ~MidiListener() = default ;

    std::vector<MidiHandler*> getHandlers() const ;
    size_t getNumHandlers() const ;

    virtual void onKeyPressed(const ActiveNote* note, bool rePress = false) ;
    virtual void onKeyReleased(ActiveNote anote) ; 
    virtual void onKeyOff(ActiveNote anote) ;
    virtual void onPitchbend(uint16_t pitchbend );
    // virtual void onMidiControlEvent(MidiControlMessage messageType, uint8_t value);

    // observer functions for responding to connection events
    virtual void onHandlerAdded();
    virtual void onHandlerRemoved();
    
};

#endif // MIDI_LISTENER_HPP_