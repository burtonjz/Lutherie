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

#include "core/MidiEngine.hpp"
#include "capabilities/MidiIO.hpp"
#include "midi/MidiCommand.hpp"
#include "midi/MidiControlRouter.hpp"
#include "midi/MidiNote.hpp"

#include <spdlog/spdlog.h>


MidiEngine* MidiEngine::instance(){
    static MidiEngine engine ;
    return &engine ;
}

void MidiEngine::computePitchbendScaleFactor(){
    constexpr float CONFIG_PITCHBEND_MAX_SHIFT = 2 ;
    float shiftValue ;
    for ( int i = 0; i < 16384; ++i ){
        shiftValue = ( i - 8192.0 ) / 16383.0 * CONFIG_PITCHBEND_MAX_SHIFT * 2.0 ;
        pitchbendScaleFactor_[i] = std::pow(2.0f, shiftValue / 12.0f );
    }
}
std::array<double,16384> MidiEngine::pitchbendScaleFactor_ ;

MidiEngine::MidiEngine():
    midiIn_()
{
    computePitchbendScaleFactor();
}

MidiEngine::~MidiEngine(){
    stop();
}

bool MidiEngine::ready() const {
    return available_.contains(selectedDevice_);
}

bool MidiEngine::prepare(){
    return true ;
}

bool MidiEngine::start(){
    if ( !ready() ){
        SPDLOG_ERROR("Cannot start midi engine: not ready");
        return false ;
    }

    midiIn_.openPort(selectedDevice_);
    midiIn_.setCallback(&MidiEngine::midiCallback);
    midiIn_.ignoreTypes(false, false, false);
    SPDLOG_INFO(
        "Listening for MIDI input on device '{}'",
        available_[selectedDevice_]
    );
    return true ;
}

void MidiEngine::stop(){
    if ( midiIn_.isPortOpen() ){
        SPDLOG_INFO("Stopping MIDI...");
        midiIn_.cancelCallback();
        midiIn_.closePort();
    }
}

const std::map<int, std::string>& MidiEngine::availableDevices(){
    int numPorts = midiIn_.getPortCount();
    for (int i = 0; i < numPorts; ++i){
        available_[i] = midiIn_.getPortName(i);
    }
    return available_ ;
}

bool MidiEngine::selectDevice(int id){
    if ( !available_.contains(id) ){
        SPDLOG_ERROR("invalid device id.");
        return false ;
    }

    selectedDevice_ = id ;
}

void MidiEngine::midiCallback(
    double deltaTime, 
    std::vector<unsigned char> *message, 
    void*
){
    MidiEngine::instance()->routeMessage(deltaTime, message);
}

void MidiEngine::tick(float dt){
    for ( auto* io : capabilities_ ){
        io->tick(dt);
    }
}

void MidiEngine::registerCapability(MidiIO* io){
    if ( !io ) return ;
    auto it = std::find(capabilities_.begin(), capabilities_.end(), io);
    if ( it != capabilities_.end() ) return ;
    capabilities_.push_back(io);
}

void MidiEngine::unregisterCapability(MidiIO* io){
    if ( !io ) return ;
    capabilities_.erase(
        std::remove(capabilities_.begin(), capabilities_.end(), io),
        capabilities_.end()
    );
}

void MidiEngine::addMidiSource(MidiConnection conn){    
    sources_.insert(std::move(conn));
}

void MidiEngine::removeMidiSource(const MidiConnection& conn){
    sources_.erase(conn);
}

void MidiEngine::routeMessage(double, std::vector<unsigned char>* message){
    MidiCommand command = static_cast<MidiCommand>((*message)[0] & 0xF0) ;
    uint8_t channel     = static_cast<uint8_t>((*message)[0] & 0x0F) ;

    switch(command){
        case MidiCommand::MIDI_CMD_NOTE_OFF:
            processMsgNoteOff(channel, (*message)[1],(*message)[2]);
            break ;
        case MidiCommand::MIDI_CMD_NOTE_ON:
            processMsgNoteOn(channel, (*message)[1],(*message)[2]);
            break ;
        case MidiCommand::MIDI_CMD_NOTE_PRESSURE:
            processMsgNotePressure(channel, (*message)[1],(*message)[2]);
            break ;
        case MidiCommand::MIDI_CMD_CONTROL:
            processMsgControl(channel, (*message)[1],(*message)[2]);
            break ;
        case MidiCommand::MIDI_CMD_PROGRAM:
            processMsgProgram(channel, (*message)[1]);
            break ;
        case MidiCommand::MIDI_CMD_CHANNEL_PRESSURE:
            processMsgChannelPressure(channel, (*message)[1]);
            break ;
        case MidiCommand::MIDI_CMD_PITCHBEND:
            processMsgPitchbend(channel, ((*message)[2] << 7 ) | (*message)[1] );
            break ;
        default:
            break ;
    }
}

void MidiEngine::processMsgNoteOn(uint8_t channel, uint8_t note, uint8_t vel){
    SPDLOG_INFO(
        "Received MIDI note on event: channel={}, note={}, vel={}", 
        channel, note, vel
    );

    MidiNote n(channel, note, vel, NoteStatus::PRESSED);
    for ( auto& s : sources_ ){
        s.midi->handleKeyPressed(n, s.channel);
    }
}

void MidiEngine::processMsgNoteOff(uint8_t channel, uint8_t note, uint8_t vel){
    SPDLOG_INFO(
        "Received MIDI note off event: channel={}, note={}, vel={}", 
        channel, note, vel
    );

    MidiNote n(channel, note, vel, NoteStatus::RELEASED);
    for ( auto& s : sources_ ){
        s.midi->handleKeyReleased(n, s.channel);
    }
}

void MidiEngine::processMsgPitchbend(uint8_t channel, float pitchbend){
    SPDLOG_DEBUG(
        "Received MIDI pitchbend event: channel={}, pitchbend={}",
        channel, pitchbend
    );
    for ( auto& s : sources_ ){
        s.midi->handlePitchbend(pitchbend, s.channel);
    }
}

void MidiEngine::processMsgNotePressure(uint8_t channel, uint8_t note, uint8_t pressure){
    SPDLOG_INFO(
        "Received MIDI pressure event: channel={}, note={}, pressure={}", 
        channel, note, pressure
    );
}

void MidiEngine::processMsgControl(uint8_t channel, uint8_t ctrlID, uint8_t ctrlValue){
    SPDLOG_DEBUG(
        "Received MIDI control event: channel={}, identifier={}, value={}", 
        channel, ctrlID, ctrlValue
    );
    // TODO: fold control router into engine.
    MidiControlRouter::instance()->handleEvent(ctrlID, ctrlValue);
}

void MidiEngine::processMsgProgram(uint8_t channel, uint8_t program){
    SPDLOG_INFO(
        "Received MIDI program event: channel={}, program={}", 
        channel, program
    );
}

void MidiEngine::processMsgChannelPressure(uint8_t channel, uint8_t pressure){
    SPDLOG_INFO(
        "Received MIDI channel pressure event: channel={}, pressure={}",
        channel, pressure
    );
}


