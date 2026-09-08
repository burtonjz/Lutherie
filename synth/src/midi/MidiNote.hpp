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

#ifndef __NOTE_HPP_
#define __NOTE_HPP_

#include <cstdint>
#include <array>

#include "containers/RTMap.hpp"

/**
 * @brief storage object for a single midi note
 * 
*/
class MidiNote {
private:
    uint8_t midiNote_ ;
    uint8_t midiVelocity_ ;
    bool midiOn_ ;

public:
    static std::array<double, 128> frequencies_ ;
    
    /**
     * @brief pre-compute data for the note class
     * 
     */
    static void initialize();


    /**
     * @brief Constructor for MidiNote class
     * 
     * @param midiNote the midi note number (0-127)
     * @param midiVelocity the midi velocity (0-127)
     * @param midiOn the key press status
    */
    MidiNote(uint8_t midiNote, uint8_t midiVelocity, bool midiOn);

    /**
     * @brief default constructor
     * 
     */
    MidiNote();

    /**
     * @brief returns the midi note (0-127)
    */
    uint8_t getMidiNote() const ;

    /**
     * @brief sets the midi note of this data
    */
    void setMidiNote(uint8_t note );

    /**
     * @brief returns the velocity (0-127) of this MidiNote
    */
    uint8_t getMidiVelocity() const ;

    /**
     * @brief sets the velocity of this note. Must be between 0-127.
    */
    void setMidiVelocity(uint8_t velocity);

    /**
     * @brief returns the midi note status
    */
    bool getStatus() const ;

    /**
     * @brief sets the midi note status
     * 
    */
    void setStatus(bool midiOn);

    /**
     * @brief returns the frequency of the specified note
     * 
    */
    static double getFrequency(uint8_t note);

    /**
     * @brief returns the frequency of this MidiNote
    */
    double getFrequency() const ;

};

// for use in MidiEvent handlers so they can individually track note times
struct ActiveNote {
    MidiNote note ;
    float time = 0.0f ;
    bool pendingOff = false ;

    void resetTime(){
        time = 0.0f ;
    }

    void updateTime(float dt){
        time += dt ;
    }
};

using KeyMap = RTMap<uint8_t, MidiNote, 128> ;

#endif // __NOTE_HPP_