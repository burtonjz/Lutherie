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

#include "midi/MidiController.hpp"
#include "midi/MidiCommand.hpp"
                  
#include <spdlog/spdlog.h>
#include <cmath>

// static functions

void MidiController::computePitchbendScaleFactor(){
    float shiftValue ;
    for ( int i = 0; i < 16384; ++i ){
        shiftValue = ( i - 8192.0 ) / 16383.0 * CONFIG_PITCHBEND_MAX_SHIFT * 2.0 ;
        pitchbendScaleFactor_[i] = std::pow(2.0f, shiftValue / 12.0f );
    }
}

std::array<double,16384> MidiController::pitchbendScaleFactor_ ;

MidiController::MidiController(MidiState* state):
    state_(state)
{}

void MidiController::initialize(){
    MidiNote::initialize(); // precompute note frequencies
    MidiController::computePitchbendScaleFactor() ; // precompute pitchbend
}

void MidiController::addHandler(MidiEventHandler* handler){ 
    if (std::find(handlers_.begin(), handlers_.end(), handler) != handlers_.end()) return ;
    handlers_.insert(handler) ; 
}
 
void MidiController::removeHandler(MidiEventHandler* handler){ 
    handlers_.erase(handler) ;  
}

void MidiController::tick(float dt){
    for ( MidiEventHandler* h : handlers_ ){
        h->tick(dt);
    }
}

void MidiController::onMidiEvent(double deltaTime, std::vector<unsigned char> *message, void *userData){
    MidiController* self = static_cast<MidiController*>(userData);
    self->processMessage(deltaTime, message);    
}

void MidiController::processMessage([[maybe_unused]] double deltaTime, std::vector<unsigned char> *message){    
    MidiCommand command = static_cast<MidiCommand>((*message)[0] & 0xF0) ;
    // int channel         = static_cast<int>((*message)[0] & 0x0F) ;

    switch(command){
        case MidiCommand::MIDI_CMD_NOTE_OFF:
            state_->processMsgNoteOff((*message)[1],(*message)[2]);
            break ;
        case MidiCommand::MIDI_CMD_NOTE_ON:
            state_->processMsgNoteOn((*message)[1],(*message)[2]);
            break ;
        case MidiCommand::MIDI_CMD_NOTE_PRESSURE:
            state_->processMsgNotePressure((*message)[1],(*message)[2]);
            break ;
        case MidiCommand::MIDI_CMD_CONTROL:
            state_->processMsgControl((*message)[1],(*message)[2]);
            break ;
        case MidiCommand::MIDI_CMD_PROGRAM:
            state_->processMsgProgram((*message)[1]);
            break ;
        case MidiCommand::MIDI_CMD_CHANNEL_PRESSURE:
            state_->processMsgChannelPressure((*message)[1]);
            break ;
        case MidiCommand::MIDI_CMD_PITCHBEND:
            state_->processMsgPitchbend(((*message)[2] << 7 ) | (*message)[1] );
            break ;
        default:
            break ;
    }
}

