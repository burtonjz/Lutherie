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

#include "midi/MidiState.hpp"
#include "midi/MidiControlRouter.hpp"

MidiState* MidiState::instance(){
    static MidiState manager ;
    return &manager ;
}

MidiState::MidiState():
    handlers_(),
    notes_(),
    pitchbend_(0.0f)
{}

void MidiState::addHandler(MidiEventHandler* handler){
    if (std::find(handlers_.begin(), handlers_.end(), handler) != handlers_.end()) return ;
    handlers_.push_back(handler);
}

void MidiState::removeHandler(MidiEventHandler* handler){
    auto it = std::find(handlers_.begin(), handlers_.end(), handler);
    if ( it != handlers_.end() ){
        handlers_.erase(it);
    }
}

const std::vector<MidiEventHandler*>& MidiState::getHandlers() const {
    return handlers_ ;
}

const MidiNote* MidiState::getNote(int midiNote){
    auto it = notes_.find(midiNote);        
    if ( it == notes_.end() ) return nullptr ;
    return &notes_[midiNote] ;
}

void MidiState::processMsgNoteOn(int midiNote, int velocity){
    SPDLOG_DEBUG("Processing NOTE_ON event. MidiNote={}, Velocity={}", midiNote, velocity);
    notes_[midiNote].setMidiNote(midiNote);
    notes_[midiNote].setMidiVelocity(velocity);
    notes_[midiNote].setStatus(true);
    for ( auto* h : handlers_ ){
        h->handleKeyPressed(notes_[midiNote]);
    }
}

void MidiState::processMsgNoteOff(int midiNote, int velocity){
    SPDLOG_DEBUG("Processing NOTE_OFF event. MidiNote={}, Velocity={}", midiNote, velocity);
    notes_[midiNote].setStatus(false);
    for ( auto* h : handlers_ ){
        h->handleKeyReleased(notes_[midiNote]);
    }
    notes_.erase(midiNote);
}

void MidiState::processMsgPitchbend(float pitchbend){
    SPDLOG_DEBUG("Processing PITCHBEND event. pitchbend={}", pitchbend);
    pitchbend_ = pitchbend ;
    for ( auto* h : handlers_ ){
        h->handlePitchbend(pitchbend_);
    }
}

void MidiState::processMsgNotePressure([[maybe_unused]] int midiNote, [[maybe_unused]] int pressure){
}

void MidiState::processMsgControl(int ctrlID, int ctrlValue){
    SPDLOG_DEBUG("Processing CONTROL event. identifier={}, value={}", ctrlID, ctrlValue);
    MidiControlRouter::instance()->handleEvent(ctrlID, ctrlValue);
}

void MidiState::processMsgProgram([[maybe_unused]] int program){

}

void MidiState::processMsgChannelPressure([[maybe_unused]] int pressure){

}

void MidiState::reset(){
    handlers_.clear();
    notes_.clear();
    pitchbend_ = 0.0f ;
}
