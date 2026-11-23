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
#include "containers/Mask.hpp"
#include "params/SlotBank.hpp"
#include "params/SlotBankConfig.hpp"
#include "interfaces/ModulationSource.hpp"
#include "interfaces/ParameterAccessor.hpp"
#include "types/ParameterType.hpp"
#include "types/ParameterConfig.hpp"
#include "status/ParamStatus.hpp"
#include <stdexcept>

// forward declarations
struct CollectionRequest ;
class ParameterDefinition ;
struct ParameterObserver ;
class OrderedSlotBank ;
class PooledSlotBank ;
class SlotBank ;

class Parameters : public Capability {
private:
    using Definitions = std::array<std::unique_ptr<ParameterDefinition>, ParameterType::count> ;
    using ParameterToBank = std::array<SlotBank*, ParameterType::count> ;
    using BankToDefinitions = std::unordered_map<SlotBank*, Mask<ParameterType::count>> ;
    using Collections = std::unordered_map<CollectionDescriptor, std::vector<ParameterDefinition*>, CollectionHash> ;

    std::vector<std::unique_ptr<SlotBank>> banks_ ;
    OrderedSlotBank* unitBank_ ; // used for single-instance, individual parameters

    Definitions definitions_ ;
    Collections collections_ ;

    // lookups 
    ParameterToBank p2b_ ;
    BankToDefinitions b2d_ ;
    
    // definition masks
    Mask<ParameterType::count> active_ ; // in use
    Mask<ParameterType::count> single_ ; // standalone parameter
    Mask<ParameterType::count> structured_ ; // part of a collection
    Mask<ParameterType::count> modulatable_ ; // capable of modulation

    ParameterObserver* observer_ ;

    bool freezeCreation_ = false ;

public:
    explicit Parameters(ParameterObserver* observer = nullptr);
    ~Parameters() = default ;

    Parameters(const Parameters&) = delete ;
    Parameters& operator=(const Parameters&) = delete ;

    // ==== PARAMETER CREATION ====
    
    void freezeParameterCreation(); 

    void addPooledParameter(
        ParameterConfig cfg,
        SlotManager& manager,
        size_t capacity,
        const ModulationSource* modulator = nullptr,
        ModulationData modData = {}
    );

    void addUnitParameter(
        ParameterConfig cfg, 
        const ModulationSource* modulator = nullptr,
        ModulationData modData = {}
    );

    void addCollection(
        CollectionDescriptor descriptor,
        std::vector<ParameterConfig> configs,
        OrderedBankConfig bankCfg
    );

    void addCollection(
        CollectionDescriptor descriptor
    );

    // ==== PARAMETER VIEW ====
    void bindParameter(ParameterAccessor& accessor, ParameterType p) const ;
    
    template <ParameterType::Value typ>
    void bindSlot(SlotAccessor<typ>& accessor, size_t idx) const {
        auto def = definition(typ);
        if ( !def ){
            auto msg = "parameter not available" ;
            SPDLOG_ERROR(msg);
            throw new std::invalid_argument(msg);
        }

        ParameterAccessor acc ;
        bindParameter(acc, typ);

        if ( idx >= acc.slotCount() ){
            auto msg = "slot out of range" ;
            SPDLOG_ERROR(msg);
            throw new std::invalid_argument(msg);
        }
        acc.bindSlot(accessor, idx);
    }

    // ==== PARAMETER DEFINITION ====
    std::vector<CollectionDescriptor> collectionDescriptors() const ;
    
    std::optional<ParameterSupports> supports(ParameterType p) const ;
    ParamStatus setSupports(ParameterType p, ParameterSupports supports);

    std::optional<ParameterLimits> limits(ParameterType p) const ;
    ParamStatus setLimits(ParameterType p, ParameterLimits limits);

    std::optional<ParameterValue> defaultValue(ParameterType p) const ;
    ParamStatus setDefault(ParameterType p, ParameterValue v);

    // TODO: HANDLE COLLECTION REQUEST CHANGES
    
    void observe(ParameterType p);

    void modulate();
    ParamStatus setModulation(ParameterType p, const ModulationSource* m, ModulationData d = {});
    ParamStatus removeModulation(ParameterType p);
    ParamStatus setDepthModulation(ParameterType p, const ModulationSource* m, ModulationData d = {});
    ParamStatus removeDepthModulation(ParameterType p);

private:
    bool parameterAvailable(uint8_t idx) const ;
    ParameterDefinition* definition(ParameterType p) const ;
    std::span<ParameterDefinition* const> collection(const CollectionDescriptor& c) const ;

    ModulationData withRequiredDefaults(const ModulationSource* m, ModulationData d);

    OrderedSlotBank& createOrderedBank(OrderedBankConfig cfg);
    PooledSlotBank& createPooledBank(PooledBankConfig cfg);
}; 

#endif // PARAMETER_CAPABILITIES_HPP_