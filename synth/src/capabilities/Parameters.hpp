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


#include "capabilities/Capability.hpp"
#include "meta/CollectionDescriptor.hpp"
#include "params/ModulationParameter.hpp"
#include "params/ParameterMask.hpp"
#include "params/SlotBank.hpp"
#include "params/SlotBankConfig.hpp"
#include "types/ParameterType.hpp"
#include "types/ParameterConfig.hpp"
#include "status/ParamStatus.hpp"

// forward declarations
class ModulationSource ;
class ParameterDefinition ;
class ParameterObserver ;
class OrderedSlotBank ;
class PooledSlotBank ;
class SlotBank ;

class Parameters : public Capability {
private:
    using Definitions = std::array<std::unique_ptr<ParameterDefinition>, ParameterType::count> ;
    using ParameterToBank = std::array<SlotBank*, ParameterType::count> ;
    using BankToDefinitions = std::unordered_map<SlotBank*, ParameterMask<ParameterType::count>> ;
    using Collections = std::unordered_map<CollectionDescriptor, std::vector<ParameterDefinition*>, CollectionHash> ;

    std::vector<std::unique_ptr<SlotBank>> banks_ ;
    OrderedSlotBank* unitBank_ ; // used for single-instance, individual parameters

    Definitions definitions_ ;
    Collections collections_ ;

    // lookups 
    ParameterToBank p2b_ ;
    BankToDefinitions b2d_ ;
    
    // definition masks
    ParameterMask<ParameterType::count> active_ ; // in use
    ParameterMask<ParameterType::count> single_ ; // standalone parameter
    ParameterMask<ParameterType::count> structured_ ; // part of a collection
    ParameterMask<ParameterType::count> modulatable_ ; // capable of modulation

    ParameterObserver* observer_ ;

public:
    explicit Parameters(ParameterObserver* observer = nullptr);
    ~Parameters() = default ;

    Parameters(const Parameters&) = delete ;
    Parameters& operator=(const Parameters&) = delete ;

    ParameterDefinition* definition(ParameterType p) const ;
    
    std::vector<CollectionDescriptor> collectionDescriptors() const ;
    std::span<ParameterDefinition* const> collection(const CollectionDescriptor& c) const ;

    void observe(ParameterType p);

    ParameterDefinition* addPooledParameter(
        ParameterConfig cfg,
        SlotManager& manager,
        size_t capacity,
        const ModulationSource* modulator = nullptr,
        ModulationData modData = {}
    );

    ParameterDefinition* addUnitParameter(
        ParameterConfig cfg, 
        const ModulationSource* modulator = nullptr,
        ModulationData modData = {}
    );

    std::span<ParameterDefinition*> addCollection(
        CollectionDescriptor descriptor,
        std::initializer_list<ParameterConfig> configs,
        OrderedBankConfig bankCfg
    );

    void modulate();
    ParamStatus setModulation(ParameterType p, const ModulationSource* m, ModulationData d = {});
    ParamStatus removeModulation(ParameterType p);
    ParamStatus setDepthModulation(ParameterType p, const ModulationSource* m, ModulationData d = {});
    ParamStatus removeDepthModulation(ParameterType p);

private:
    bool parameterAvailable(uint8_t idx) const ;

    ModulationData withRequiredDefaults(const ModulationSource* m, ModulationData d);

    OrderedSlotBank& createOrderedBank(OrderedBankConfig cfg);
    PooledSlotBank& createPooledBank(PooledBankConfig cfg);
}; 

#endif // PARAMETER_CAPABILITIES_HPP_