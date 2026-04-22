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

#ifndef OSCILLOSCOPE_COMPONENT_HPP_
#define OSCILLOSCOPE_COMPONENT_HPP_

#include "dsp/Analyzer.hpp"
#include "configs/OscilloscopeConfig.hpp"

class Oscilloscope : public Analyzer {
private:
    std::vector<double> captureBuffer_ ;
    size_t bufferPosition_ ;
    double sampleRate_ ;
    size_t windowSize_ ;             // samples to display
    size_t searchRegion_ ;           // samples to scan for trigger
    size_t captureSize_ ;            // window + search
    double hysteresisRatio_ ;        // fraction of signal range (to ignore false triggers)
    double silenceThreshold_ ;       // min peak-to-peak to attempt trigger
    
public:
    Oscilloscope(ComponentId, OscilloscopeConfig cfg);
    ~Oscilloscope() = default ;
    
    void process(const double* data, size_t size, ComponentId id) override ;

};

#endif // OSCILLOSCOPE_COMPONENT_HPP_