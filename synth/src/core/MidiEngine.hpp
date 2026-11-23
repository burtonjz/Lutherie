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

#ifndef MIDI_ENGINE_HPP_
#define MIDI_ENGINE_HPP_

#include "routing/MidiConnection.hpp"
#include <cstdint>
#include <rtmidi/RtMidi.h>
#include <map>

// forward declarations
class MidiIO ;

class MidiEngine {
private:
    RtMidiIn midiIn_ ;

    std::map<int, std::string> available_ ;
    int selectedDevice_ = -1 ;

    std::vector<MidiIO*> capabilities_ ;
    MidiConnectionSet sources_ ;

    MidiEngine();

    static std::array<double,16384> pitchbendScaleFactor_ ;
    static void computePitchbendScaleFactor();

public:
    static MidiEngine* instance();
    

    ~MidiEngine();

    MidiEngine(const MidiEngine&) = delete ;
    MidiEngine& operator=(const MidiEngine&) = delete ;
    MidiEngine(MidiEngine&&) = delete ;
    MidiEngine& operator=(MidiEngine&&) = delete ;

    bool ready() const ;
    bool prepare();
    bool start();
    void stop();

    const std::map<int, std::string>& availableDevices();
    bool selectDevice(int id);

    static void midiCallback(
        double deltaTime, 
        std::vector<unsigned char>* message, 
        void*
    );

    void tick(float dt);

    void registerCapability(MidiIO* io);
    void unregisterCapability(MidiIO* io);

    void addMidiSource(MidiConnection conn);
    void removeMidiSource(const MidiConnection& conn);

private:
    void routeMessage(double deltaTime, std::vector<unsigned char>* message);

    void processMsgNoteOn(uint8_t channel, uint8_t note, uint8_t vel);
    void processMsgNoteOff(uint8_t channel, uint8_t note, uint8_t vel);
    void processMsgPitchbend(uint8_t channel, float pitchbend);
    void processMsgNotePressure(uint8_t channel, uint8_t note, uint8_t pressure);
    void processMsgControl(uint8_t channel, uint8_t ctrlID, uint8_t ctrlValue);
    void processMsgProgram(uint8_t channel, uint8_t program);
    void processMsgChannelPressure(uint8_t channel, uint8_t pressure);
};

#endif // MIDI_ENGINE_HPP_