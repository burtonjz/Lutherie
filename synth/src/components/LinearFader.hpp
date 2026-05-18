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

#ifndef __MODULATOR_LINEAR_FADER_HPP_
#define __MODULATOR_LINEAR_FADER_HPP_

#include "core/BaseComponent.hpp"
#include "core/ModulatorComponent.hpp"
#include "midi/MidiEventHandler.hpp"
#include "types/ComponentType.hpp"
#include "configs/LinearFaderConfig.hpp"

class LinearFader : public ModulatorComponent, public MidiEventHandler { 
private:
    KeyMap pending_ ; // MidiModulator holds on to release notes until their release is complete, or the note is pressed again
     
public:
    const static ComponentType staticType ;

    LinearFader(ComponentId id, LinearFaderConfig cfg);
    
    // MODULATOR OVERRIDES
    double modulate(double value, ModulationData* mData) const override;
    ModulatorRange getModulatorRange() const override { return ModulatorRange::UNIPOLAR ; }
    
    // MIDI EVENT HANDLER OVERRIDES
    virtual bool shouldKillNote(const ActiveNote& note) const override ;
};

#endif // __MODULATOR_LINEAR_FADER_HPP_