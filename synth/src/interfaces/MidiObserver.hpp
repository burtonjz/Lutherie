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

class MidiObserver {
public:
    void onKeyEvent(const MidiNote& note);
    
    virtual void onKeyPressed(const MidiNote&){} 
    virtual void onKeyReleased(const MidiNote&){}
    virtual void onKeyOff(MidiNote){}
    virtual void onPitchbend(uint16_t /*pitchbend*/){};
};

#endif // MIDI_OBSERVER_HPP_