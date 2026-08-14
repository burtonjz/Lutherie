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

#ifndef __MIDI_CONTROLLER_HPP_
#define __MIDI_CONTROLLER_HPP_

#include <set>
#include <array>

#include "midi/MidiEventHandler.hpp"

constexpr float CONFIG_PITCHBEND_MAX_SHIFT = 2 ;



class MidiController {
private:
    std::set<MidiEventHandler*> handlers_ ;
       
    MidiController();

public:
    static MidiController* instance();

    MidiController(const MidiController&) = delete ;
    MidiController& operator=(const MidiController&) = delete ;
    MidiController(MidiController&&) = delete ;
    MidiController& operator=(MidiController&&) = delete ;

    void initialize() ;

    /**
     * @brief Callback function. 
     * 
     * @param deltaTime time past since last midi event
     * @param message midi message
     * @param userData pointer to this class
     */
    static void onMidiEvent(double deltaTime, std::vector<unsigned char> *message, void *userData);

    static std::array<double,16384> pitchbendScaleFactor_ ;
    static void computePitchbendScaleFactor();

    void addHandler(MidiEventHandler* handler);
    void removeHandler(MidiEventHandler* handler);

    /**
     * @brief tick all midi handlers ( to resolve the event queue)
     * 
     * @param dt delta time
     */
    void tick(float dt);

private:
    void processMessage(double deltaTime, std::vector<unsigned char> *message);

};

#endif // __MIDI_CONTROLLER_HPP_