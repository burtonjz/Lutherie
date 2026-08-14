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


#ifndef ANALYZER_HPP_
#define ANALYZER_HPP_

#include "core/BaseComponent.hpp"
#include "core/AudioSignalComponent.hpp"

#include <cstddef>
#include <cstring>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

struct AnalysisContext ;
class Analyzer : public AudioSignalComponent {
protected:
    std::unique_ptr<double[]> analysisBuffer_ ;
    AnalysisContext* context_ ;
    bool collecting_ = false ;

public:
    Analyzer();
    ~Analyzer();

    /**
     * @brief define how raw data should be processed and pushed through analysis engine
     */
    virtual void process(const double* data, size_t size, ComponentId id) = 0 ;

    void flush();
    void calculateSample() override ;
    AnalysisContext* getAnalysisContext() const ;

private:
    void createAnalysisContext();
};


#endif // ANALYZER_HPP_