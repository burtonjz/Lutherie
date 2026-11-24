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


enum class NoteStatus {
    PRESSED,
    REPRESSED,
    RELEASED,
    KILLING,
    OFF
};

class MidiNote {
private:
    uint8_t channel_ ;
    uint8_t note_ ;
    uint8_t velocity_ ;
    NoteStatus status_ ;
    float time_ = 0.0f ; 

    static std::array<double, 128> frequencies_ ;

public:
    static constexpr uint8_t INVALID = 255 ;

    MidiNote(uint8_t channel, uint8_t note, uint8_t vel, NoteStatus st);
    MidiNote();

    static void initialize();

    uint8_t channel() const ;
    void setChannel(uint8_t channel);
    
    uint8_t note() const ;
    void setNote(uint8_t note );

    uint8_t velocity() const ;
    void setVelocity(uint8_t vel);

    NoteStatus status() const ;
    void setStatus(NoteStatus st);

    static double frequency(uint8_t note);
    double frequency() const ;

    float time() const ;
    void resetTime();
    void updateTime(float dt);

};

#endif // __NOTE_HPP_