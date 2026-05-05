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

#ifndef __MODULE_OSCILLATOR_HPP_
#define __MODULE_OSCILLATOR_HPP_

#include "core/BaseModule.hpp"
#include "core/BaseModulator.hpp"
#include "configs/OscillatorConfig.hpp"
#include "types/Waveform.hpp"
#include <random>

class Oscillator : public BaseModule, public BaseModulator {
private:
    double phase_ ;
    double increment_ ;
    // for noise
    std::mt19937 generator_ ;
    std::uniform_int_distribution<size_t> distribution_ ;

    
public:
    /**
        * @brief Construct a new Oscillator module
        * 
        */
    Oscillator(ComponentId id, OscillatorConfig cfg);

    /**
        * @brief Construct a child oscillator module
        * 
    */
    Oscillator(ParameterMap& map, double frequency);

    // overrides
    bool isGenerative() const override ;
    void calculateSample() override ; 
    double modulate([[maybe_unused]] double value, [[maybe_unused]] ModulationData* mdat ) const override ;
    ModulatorRange getModulatorRange() const override { return ModulatorRange::BIPOLAR ; }

    void tick() override ;
    void reset();
    
    // getters/setters
    void addReferenceParameters(ParameterMap& map);
    void setWaveform(Waveform wave);
    void setFrequency(double freq);
    void setAmplitude(double amp);
    
};

#endif // __MODULE_OSCILLATOR_HPP_