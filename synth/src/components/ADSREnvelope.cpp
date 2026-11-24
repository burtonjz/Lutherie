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

#include "components/ADSREnvelope.hpp"

#include "capabilities/MidiIO.hpp"     // IWYU pragma: keep
#include "capabilities/Modulator.hpp"  // IWYU pragma: keep
#include "capabilities/Parameters.hpp" // IWYU pragma: keep

#include "interfaces/ComponentInfo.hpp"
#include "params/ModulationParameter.hpp"
#include "types/MonophonicTriggerType.hpp"
#include "types/ParameterType.hpp"

ADSREnvelope::ADSREnvelope(int id, ADSREnvelopeConfig):
    Node(id, NodeType::COMPONENT),
    ComponentInfo(ComponentType::ADSREnvelope),
    ModulationSource(
        ModulatorRange::UNIPOLAR, {
            ModulationParameter::MIDI_NOTE, 
            ModulationParameter::INITIAL_VALUE
    }),
    MidiProcessor(),
    attack_(createSlotAccessor<ParameterType::ATTACK>()),
    decay_(createSlotAccessor<ParameterType::DECAY>()),
    sustain_(createSlotAccessor<ParameterType::SUSTAIN>()),
    release_(createSlotAccessor<ParameterType::RELEASE>()),
    trigger_(createSlotAccessor<ParameterType::TRIGGER>())
{
    addCapability<Capability::MidiIO>(*this, 1, 1);
    addCapability<Capability::Modulator>(*this);

    auto& params = addCapability<Capability::Parameters>();

    params.addUnitParameter(ParameterType::ATTACK);
    params.addUnitParameter(ParameterType::DECAY);
    params.addUnitParameter(ParameterType::SUSTAIN);
    params.addUnitParameter(ParameterType::RELEASE);
    params.addUnitParameter(ParameterType::TRIGGER);

    params.bindSlot<ParameterType::ATTACK>(attack_, 0);
    params.bindSlot<ParameterType::DECAY>(decay_, 0);
    params.bindSlot<ParameterType::SUSTAIN>(sustain_, 0);
    params.bindSlot<ParameterType::RELEASE>(release_, 0);
    params.bindSlot<ParameterType::TRIGGER>(trigger_, 0);

    params.freezeParameterCreation();
}

double ADSREnvelope::modulate(double, ModulationData* mData) const {
    double output = 0.0 ;

    // check required data
    if ( !mData ) return output ; 

    if ( !mData->has(ModulationParameter::INITIAL_VALUE) ){
        mData->set(ModulationParameter::INITIAL_VALUE, 0.0f);
    }
    if ( !mData->has(ModulationParameter::LAST_OUTPUT) ){ 
        mData->set(ModulationParameter::LAST_OUTPUT, 0.0f) ;    
    }
    
    // get the midi note
    bool monophonicMode = !mData->has(ModulationParameter::MIDI_NOTE);
    auto behavior = MonophonicTriggerType::from_uint8(trigger_.value());

    uint8_t midiValue ;
    if ( monophonicMode ){ 
        midiValue = notesActive() > 0 
            ? lastNotePressed() 
            : lastNoteReleased();

        if ( midiValue == MidiNote::INVALID ) return output ; // no notes ever played!
        midiValue = resolveMonophonicNote(midiValue, mData, behavior);    
    } else {
        midiValue = static_cast<uint8_t>(mData->get(ModulationParameter::MIDI_NOTE)) ;
    }
    
    if ( !noteActive(midiValue) ) return output ;
    const MidiNote& midiNote = note(midiValue);

    // set start level
    if ( monophonicMode ){
        updateMonophonicState(midiValue, midiNote.status(), mData, behavior);
    }
    float startLevel = mData->get(ModulationParameter::INITIAL_VALUE);

    // calculate ADSR
    switch ( midiNote.status() ){
    case NoteStatus::PRESSED:
    case NoteStatus::REPRESSED:
        output = handleADS(startLevel, midiNote.time());
        break ;
    case NoteStatus::RELEASED:
        output = handleR(startLevel, midiNote.time());
        break ;
    default:
        break ;
    } 
    
    mData->set(ModulationParameter::LAST_OUTPUT, output) ;
    return output ;
}

double ADSREnvelope::handleADS(float startLevel, float time) const {
    auto attack = attack_.modulatedValue();
    auto decay = decay_.modulatedValue();
    auto sustain = sustain_.modulatedValue();

    if ( time <= attack ) {
        return startLevel + ( 1.0f - startLevel ) * (time / attack) ;
    } else if ( time <= (attack + decay) ) {
        return 1.0f - (1.0f - sustain) * ((time - attack) / decay );
    } else {
        return sustain ;
    }
}

double ADSREnvelope::handleR(float startLevel, float time) const {
    float release = release_.modulatedValue();
    if ( time >= release ){
        return 0.0 ;
    } else {
        return startLevel * (1 - (time / release)) ;
    }
}

bool ADSREnvelope::shouldKillNote(const MidiNote& note, size_t) const {
    float release = release_.modulatedValue();
    return note.time() > release ;
}

uint8_t ADSREnvelope::resolveMonophonicNote(
    uint8_t currentNote, ModulationData* mData, 
    MonophonicTriggerType behavior) const 
{
    if ( behavior == MonophonicTriggerType::LEGATO ){
        if ( mData->has(ModulationParameter::LAST_MIDI_NOTE) ){
            uint8_t latchedNote = static_cast<uint8_t>(mData->get(ModulationParameter::LAST_MIDI_NOTE));
            if ( noteActive(latchedNote) ){
                return latchedNote ;
            }
        }
    }
    return currentNote ;
}

void ADSREnvelope::updateMonophonicState(
    uint8_t midiNote, NoteStatus st, 
    ModulationData* mData, MonophonicTriggerType behavior) const 
{
    bool isPressed = st == NoteStatus::PRESSED || st == NoteStatus::REPRESSED ;
    
    bool wasPressed = mData->has(ModulationParameter::LAST_STATE) &&
        mData->get(ModulationParameter::LAST_STATE) > 0.0f ;

    bool noteChanged = mData->has(ModulationParameter::LAST_MIDI_NOTE) &&
        mData->get(ModulationParameter::LAST_MIDI_NOTE) != midiNote ;

    switch ( behavior ){
    case MonophonicTriggerType::LEGATO:
        if ( isPressed != wasPressed ){
            mData->set(ModulationParameter::INITIAL_VALUE, mData->get(ModulationParameter::LAST_OUTPUT));
        }
        break ;
    case MonophonicTriggerType::RETRIGGER_LEGATO:
        if (( isPressed != wasPressed ) || noteChanged ){
            mData->set(ModulationParameter::INITIAL_VALUE, mData->get(ModulationParameter::LAST_OUTPUT));
        }
        break ;
    case MonophonicTriggerType::RETRIGGER_RESET:
        if (( isPressed && !wasPressed ) || noteChanged ){
            mData->set(ModulationParameter::INITIAL_VALUE, 0.0f);
        } else if ( !isPressed && wasPressed ){
            mData->set(ModulationParameter::INITIAL_VALUE, mData->get(ModulationParameter::LAST_OUTPUT));
        }
        break ;
    default:
        break ;
    }
    mData->set(ModulationParameter::LAST_MIDI_NOTE, midiNote);
    mData->set(ModulationParameter::LAST_STATE, isPressed ? 1.0f : 0.0f);
}