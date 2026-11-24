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
#ifndef __BIQUAD_FILTER_HPP_
#define __BIQUAD_FILTER_HPP_

#include "core/Node.hpp"
#include "interfaces/AudioProcessor.hpp"
#include "interfaces/ComponentInfo.hpp"
#include "interfaces/ParameterAccessor.hpp"
#include "interfaces/ParameterObserver.hpp"
#include "interfaces/ModulationSource.hpp"
#include "configs/BiquadFilterConfig.hpp"
#include "types/ParameterType.hpp"

class BiquadFilter : 
    public Node, 
    public ComponentInfo,
    public AudioProcessor,
    public ModulationSource,
    public ParameterObserver
{
private:
    double state1_ ;
    double state2_ ;
    std::array<double,5> coefficients_ ;
    bool dirty_ ;

    SlotAccessor<ParameterType::FILTER_TYPE> filterType_ ;
    SlotAccessor<ParameterType::FREQUENCY> cutoff_ ;
    SlotAccessor<ParameterType::GAIN> gain_ ;
    SlotAccessor<ParameterType::Q_FACTOR> qFactor_ ;
    SlotAccessor<ParameterType::BANDWIDTH> bandwidth_ ;
    SlotAccessor<ParameterType::SHELF> shelf_ ;

    double sampleRate_ = 0 ;

public:
    BiquadFilter(int id, BiquadFilterConfig cfg);

    BiquadFilter(const BiquadFilter&) = delete ;
    BiquadFilter& operator=(const BiquadFilter&) = delete ;
    BiquadFilter(BiquadFilter&&) = delete ;
    BiquadFilter& operator=(BiquadFilter&&) = delete ;

    // MODULATION SOURCE
    double modulate(double value, ModulationData* mData ) const override ;

    // AUDIO PROCESSOR
    void onPrepare(AudioContext ctx) override ;
    void processFrame(const AudioAccessor& accessor, size_t frame) noexcept override ;

    // PARAMETER OBSERVER
    void onParameterChanged(ParameterType p, uint16_t slot = 0) override ;

private:
    void calculateCoefficients();
    inline double currentOutput(double input);
    inline double currentOutput(double input, double& s1, double& s2) const ;

    // RBJ cookbook amplitude term for peaking/shelf filters:
    // A = 10^(dB/40) = sqrt(linear gain)
    double biquadA(double g) const ;

};

#endif // __BIQUAD_FILTER_HPP_
