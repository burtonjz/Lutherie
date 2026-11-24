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

#include "components/LinearFader.hpp"

#include "capabilities/MidiIO.hpp"     // IWYU pragma: keep
#include "capabilities/Modulator.hpp"  // IWYU pragma: keep
#include "capabilities/Parameters.hpp" // IWYU pragma: keep
#include "midi/MidiNote.hpp"

LinearFader::LinearFader(int id, LinearFaderConfig cfg):
    Node(id, NodeType::COMPONENT),
    ComponentInfo(ComponentType::LinearFader),
    ModulationSource(
        ModulatorRange::UNIPOLAR, {
            ModulationParameter::MIDI_NOTE, 
            ModulationParameter::INITIAL_VALUE
    }),
    MidiProcessor(),
    attack_(createSlotAccessor<ParameterType::ATTACK>()),
    release_(createSlotAccessor<ParameterType::RELEASE>())
{
    addCapability<Capability::MidiIO>(*this, 1, 1);
    addCapability<Capability::Modulator>(*this);

    auto& params = addCapability<Capability::Parameters>();

    params.addUnitParameter(ParameterType::ATTACK);
    params.addUnitParameter(ParameterType::RELEASE);

    params.bindSlot<ParameterType::ATTACK>(attack_, 0);
    params.bindSlot<ParameterType::RELEASE>(release_, 0);

    params.freezeParameterCreation();
}

double LinearFader::modulate(double, ModulationData* mData) const {
    double output = 0.0 ;

    // check required data / set to defaults
    if ( !mData ) return output ; 
    if ( !mData->has(ModulationParameter::MIDI_NOTE) ) return output ;
    
    if ( !mData->has(ModulationParameter::INITIAL_VALUE) ){ 
        mData->set(ModulationParameter::INITIAL_VALUE, 0.0f) ;     
    }
    if ( !mData->has(ModulationParameter::LAST_OUTPUT) ){ 
        mData->set(ModulationParameter::LAST_OUTPUT, 0.0f );    
    }
    
    uint8_t midiValue = static_cast<uint8_t>(mData->get(ModulationParameter::MIDI_NOTE));
    if ( !noteActive(midiValue) ) return output ;
    const MidiNote& midiNote = note(midiValue);

    float startLevel = mData->get(ModulationParameter::INITIAL_VALUE) ;
    
    switch( midiNote.status() ){
    case NoteStatus::PRESSED:
    case NoteStatus::REPRESSED:
        output = handleAttack(startLevel, midiNote.time());
        break ;
    case NoteStatus::RELEASED:
        output = handleRelease(startLevel, midiNote.time());
        break ;
    default:
        break ;  
    }

    mData->set(ModulationParameter::LAST_OUTPUT, output) ;
    return output ;
}

double LinearFader::handleAttack(float startLevel, float time) const {
    float attack = attack_.modulatedValue();
    if ( time <= attack ) {
        return startLevel + (1.0f - startLevel) * (time / attack) ;
    } 

    return 1.0 ;
}

double LinearFader::handleRelease(float startLevel, float time) const {
    // note is released
    float release = release_.modulatedValue();
    if ( time >= release ){
        return 0.0 ;
    } 
    
    return  startLevel * (1.0f - ( time / release )); 
}

bool LinearFader::shouldKillNote(const MidiNote& note, size_t) const {
    float release = release_.modulatedValue();
    return note.time() > release ;
}