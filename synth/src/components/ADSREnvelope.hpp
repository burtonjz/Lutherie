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

#ifndef __MODULATOR_ADSR_ENVELOPE_HPP_
#define __MODULATOR_ADSR_ENVELOPE_HPP_

#include "core/BaseComponent.hpp"
#include "core/ModulatorComponent.hpp"
#include "midi/MidiEventHandler.hpp"
#include "params/ParameterMap.hpp"
#include "configs/ADSREnvelopeConfig.hpp"

class ADSREnvelope : public ModulatorComponent, public MidiEventHandler {      
public:
    ADSREnvelope(ComponentId id, ADSREnvelopeConfig cfg);
    
    // MODULATOR OVERRIDES
    double modulate(double value, ModulationData* mData) const override ;
    ModulatorRange getModulatorRange() const override { return ModulatorRange::UNIPOLAR ; }

    // MIDI EVENT HANDLER OVERRIDES
    virtual bool shouldKillNote(const ActiveNote& note) const override ;

private:
    uint8_t resolveMonophonicNote(uint8_t currentNote, ModulationData* mData, MonophonicTriggerBehavior behavior) const ;
    void updateMonophonicState(uint8_t midiNote, bool isPressed, ModulationData* mData, MonophonicTriggerBehavior behavior) const ;


};

#endif // __MODULATOR_ADSR_ENVELOPE_HPP_