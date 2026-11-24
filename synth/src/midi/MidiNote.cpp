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

#include "midi/MidiNote.hpp"
#include <cassert>
#include <cmath>

// static functions/params
void MidiNote::initialize(){
    for ( uint8_t i = 0 ; i < 128 ; ++i ){
        frequencies_[i] = std::pow(2.0, (static_cast<double>(i) - 69.0) / 12.0) * 440.0 ;
    }
}
std::array<double, 128> MidiNote::frequencies_ ;

MidiNote::MidiNote(uint8_t channel, uint8_t note, uint8_t vel, NoteStatus st):
    channel_(channel),
    note_(note),
    velocity_(vel),
    status_(st)
{}

MidiNote::MidiNote():
    MidiNote(0, 0, 0, NoteStatus::OFF)
{}

uint8_t MidiNote::channel() const {
    return channel_ ;
}

void MidiNote::setChannel(uint8_t channel){
    assert(channel < 127);
    channel_ = channel ;
}

uint8_t MidiNote::note() const {
    return note_ ;
}

void MidiNote::setNote(uint8_t note){
    assert(note < 127);
    note_ = note ;
}

uint8_t MidiNote::velocity() const {
    return velocity_ ;
}

void MidiNote::setVelocity(uint8_t vel){
    assert(vel < 127);
    velocity_ = vel ;
}

NoteStatus MidiNote::status() const {
    return status_ ;
}

void MidiNote::setStatus(NoteStatus st){
    status_ = st ;
}

double MidiNote::frequency(uint8_t note){
    return frequencies_[note] ;
}

double MidiNote::frequency() const {
    return frequency(note_);
}

float MidiNote::time() const {
    return time_ ;
}

void MidiNote::resetTime(){
    time_ = 0.0f ;
}

void MidiNote::updateTime(float dt){
    time_ += dt ;
}