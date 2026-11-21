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

#ifndef PARAMETER_DEFINITION_HPP_
#define PARAMETER_DEFINITION_HPP_

#include "interfaces/ModulationSource.hpp"
#include "types/ParameterConfig.hpp"
#include "types/ParameterType.hpp"

// forward declarations
class ParameterSlotBase ;
class ParameterObserver ;
class SlotBank ;

class ParameterDefinition {
private:
    ParameterConfig config_ ;

    SlotBank* bank_ ; // non owning
    size_t column_ ; // column index of bank for definition's slots

    uint32_t revision_ = 0 ; // tracking for slot cache updates
 
    const ModulationSource* modulator_ ;
    ModulationStrategy modStrategy_ ;
    ModulationData modData_ ; // template to pass to slots
 
    std::vector<ParameterObserver*> observers_ ;
    std::unique_ptr<ParameterDefinition> depthDef_ ; // null for depth/non-modulatable

public:
    explicit ParameterDefinition(
        ParameterConfig cfg,
        SlotBank& slotBank,
        const ModulationSource* modulator = nullptr,
        ModulationData modData = {}
    );

    // use this definition only for depth (no bank, no slots)
    explicit ParameterDefinition(ParameterConfig cfg);

    ~ParameterDefinition();
 
    ParameterDefinition(const ParameterDefinition&) = delete ;
    ParameterDefinition& operator=(const ParameterDefinition&) = delete ;
 
    bool hasBank() const ;
    SlotBank& bank() const ;
    size_t slotCount() const ;
    ParameterSlotBase* slot(size_t idx) const ;
    
    const ParameterConfig& config() const ;
    uint32_t revision() const ;
    void setRange(const ParameterLimits& limits);
    void setDefaultValue(const ParameterValue& v);
 
    const ModulationSource* modulator() const ;
    ModulationStrategy modulationStrategy() const ;
    const ModulationData& modulationTemplate() const ;
    ParameterDefinition* depth();
    const ParameterDefinition* depth() const ;
 
    bool isModulating() const ;
    void setModulationStrategy(ModulationStrategy s);
    void setModulation(const ModulationSource* m, ModulationData d);
    void removeModulation();
    void modulateSlots() const ;
 
    // Stateless strategy application. Per-slot state (md) is passed in.
    template <typename T>
    bool applyStrategy(
        T base, double depth,
        ModulationData& md, T& out
    ) const {
        if constexpr ( std::is_same_v<T, bool> ) return false ;
        else {
            if ( !isModulating() ) return false ;
            const double m = modulator_->modulate(base, &md);
            switch ( +modStrategy_ ){
            case ModulationStrategy::ADDITIVE:
                out = static_cast<T>(base + depth * m);              
                return true ;
            case ModulationStrategy::MULTIPLICATIVE_ZERO:
                out = static_cast<T>(base * depth * m);
                return true ;
            case ModulationStrategy::MULTIPLICATIVE_UNITY:
                out = static_cast<T>(base * (1 + depth * m));
                return true ;
            case ModulationStrategy::EXPONENTIAL:
                out = static_cast<T>(base * std::exp2(depth * m));
                return true ;
            case ModulationStrategy::REPLACE:
                out = static_cast<T>(depth * m);
                return true ;
            default:
                return false ;
            }
        }
    }
 
    void addObserver(ParameterObserver* l);
    void removeObserver(ParameterObserver* l);
    void notifyObservers(uint16_t slot) const ;

private:
    template <typename F> void forEachSlot(F&& f) const ;
};


#endif // PARAMETER_DEFINITION_HPP_