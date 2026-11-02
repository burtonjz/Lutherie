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

#ifndef PARAMETER_CAPABILITIES_HPP_
#define PARAMETER_CAPABILITIES_HPP_

#include "params/ParameterMap.hpp"
#include "status/ParamStatus.hpp"
#include "interfaces/ParameterListener.hpp"

// forward declarations
class ParameterMap ;
class AudioIO ;
class Modulator ;

class Parameters {
private:
    ParameterMap map_ ; 
    ParameterListener* listener_ ;

public:
    // owning node's ParameterListener 
    explicit Parameters(ParameterListener* listener = nullptr);

    Parameters(const Parameters&) = delete ;
    Parameters& operator=(const Parameters&) = delete ;

    // templated 
    template <ParameterType typ, typename... Args>
    void add(Args&&... args){
        map_.add<typ>(std::forward<Args>(args)...);
    }

    template <ParameterType typ, typename... Args>
    void addCollection(Args&&... args){
        map_.addCollection<typ>(std::forward<Args>(args)...);
    }

    template <ParameterType typ>
    Parameter<typ>* get() const { return map_.getParameter<typ>(); }

    // end templated

    void markPerVoice(ParameterType p);
    void listen(ParameterType p);
    ParameterBase* get(ParameterType p) const ;
    void modulate();

    ParameterMap& map();
    const ParameterMap& map() const ;

    // Modulation
    ParamStatus setModulation(ParameterType p, const ModulationSource* m, ModulationData d = {});
    ParamStatus removeModulation(ParameterType p);
    ParamStatus setDepthModulation(ParameterType p, const ModulationSource* m, ModulationData d = {});
    ParamStatus removeDepthModulation(ParameterType p);

    const ModulationSource* getModulator(ParameterType p);
    const ModulationSource* getDepthModulator(ParameterType p);

    std::optional<double> getDepth(ParameterType p) const ;
    ParamStatus setDepth(ParameterType p, double v);

    std::optional<ModulationStrategy> getStrategy(ParameterType p) const ;
    ParamStatus setStrategy(ParameterType p, ModulationStrategy s);

    json toJson(json& output) const ;
    void fromJson(const json& j);

private:
    static ModulationData withRequiredDefaults(const ModulationSource* m, ModulationData d);

};

#endif // PARAMETER_CAPABILITIES_HPP_