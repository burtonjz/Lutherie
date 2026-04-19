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

#ifndef SPECTRUM_ANALYZER_COMPONENT_HPP_
#define SPECTRUM_ANALYZER_COMPONENT_HPP_

#include "dsp/Analyzer.hpp"
#include "configs/SpectrumAnalyzerConfig.hpp"

// forward declarations
class kiss_fft_state ;

class SpectrumAnalyzer : public Analyzer {
private:
    size_t fftSize_ ;
    std::vector<double> fftBuffer_ ;
    kiss_fft_state* fftConfig_ ;
    size_t bufferPosition_ ;

public:
    SpectrumAnalyzer(ComponentId, SpectrumAnalyzerConfig cfg);
    ~SpectrumAnalyzer();
    
    void process(const double* data, size_t size, ComponentId id) override ;

};

#endif // SPECTRUM_ANALYZER_COMPONENT_HPP_