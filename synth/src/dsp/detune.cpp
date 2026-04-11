

#include "dsp/detune.hpp"
#include "types/ParameterType.hpp"
#include <vector>
#include <cmath>

namespace dsp {
    namespace {
        std::vector<double> detuneLUT;
        bool initialized = false;
        size_t maxDetune;
    }
    
    void initializeDetuneLUT() {
        if (initialized) return;
        
        Config::load();
        maxDetune = GET_PARAMETER_TRAIT_MEMBER(ParameterType::DETUNE, maximum);
        
        size_t lutSize = maxDetune * 2 + 1;
        detuneLUT.resize(lutSize);
        
        for (size_t i = 0; i < lutSize; ++i) {
            int c = static_cast<int>(i) - static_cast<int>(maxDetune);
            detuneLUT[i] = std::pow(2.0, c / 1200.0);
        }
        
        initialized = true;
    }
    
    double getDetuneScale(int cents) {
        int index = cents + static_cast<int>(maxDetune);
        assert(index >= 0 && index < static_cast<int>(detuneLUT.size()));
        index = std::clamp(index, 0, static_cast<int>(detuneLUT.size()) - 1);
        return detuneLUT[index];
    }
}