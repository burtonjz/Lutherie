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

#ifndef MIDI_OBSERVER_HPP_
#define MIDI_OBSERVER_HPP_

#include "midi/MidiNote.hpp"
#include <cstddef>

class MidiObserver {
public:
    void onKeyEvent(const MidiNote& note){
        switch(note.status()){
        case NoteStatus::PRESSED:
        case NoteStatus::REPRESSED:
            onKeyPressed(note);
            break ;
        case NoteStatus::RELEASED:
            onKeyReleased(note);
            break ;
        case NoteStatus::OFF:
            onKeyOff(note);
            break ;
        default:
            break ;
        }
    }
    
    size_t numMidiConnections() const {
        return numConnections_ ;
    }

    virtual void onKeyPressed(const MidiNote&){} 
    virtual void onKeyReleased(const MidiNote&){}
    virtual void onKeyOff(const MidiNote&){}
    virtual void onPitchbend(uint16_t /*pitchbend*/){};

private:
    friend class MidiIO ; // for connection tracking

    size_t numConnections_ ;

};

#endif // MIDI_OBSERVER_HPP_