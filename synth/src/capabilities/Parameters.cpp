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

#include "capabilities/Parameters.hpp"
#include "meta/CollectionDescriptor.hpp"
#include "params/ParameterDefinition.hpp"
#include "params/SlotBank.hpp"
#include <stdexcept>

Parameters::Parameters(ParameterObserver* observer):
    banks_(),
    unitBank_(),
    definitions_(),
    collections_(),
    p2b_(),
    b2d_(),
    active_(),
    single_(),
    structured_(),
    modulatable_(),
    observer_(observer)
{
    auto unit = std::make_unique<OrderedSlotBank>(OrderedBankConfig());
    unitBank_ = unit.get();
    banks_.emplace_back(std::move(unit));
}

ParameterDefinition* Parameters::definition(ParameterType p) const {
    return definitions_[p.to_uint8()].get();
}

std::vector<CollectionDescriptor> Parameters::collectionDescriptors() const {
    std::vector<CollectionDescriptor> out ;
    for ( const auto& [k,_] : collections_ ){
        out.push_back(k);
    }
    return out ;
}

std::span<ParameterDefinition* const> Parameters::collection(const CollectionDescriptor& c) const {
    return collections_.at(c);
}

void Parameters::observe(ParameterType p){
    if ( !observer_ ) return ;
    
    auto* param = definition(p);
    if ( !param ) return ;
    param->addObserver(observer_);
}

bool Parameters::parameterAvailable(uint8_t idx) const {
    if ( active_.test(idx) ){
        SPDLOG_ERROR(
            "Parameter Type {} is already in use",
            ParameterType::from_uint8(idx).toString()
        );
        return false ;
    }
    return true ;
}

ParameterDefinition* Parameters::addPooledParameter(
    ParameterConfig cfg,
    SlotManager& manager,
    size_t capacity,
    const ModulationSource* modulator,
    ModulationData modData
){
    uint8_t idx = cfg.parameter().to_uint8();
    if ( !parameterAvailable(idx) ){
        throw std::invalid_argument("cannot create parameter: already claimed");
    }

    if ( capacity < 2 ){
        auto msg = "pooled parameter capacity must be at least 2" ;
        SPDLOG_ERROR(msg);
        throw std::invalid_argument(msg);
    }

    auto& bank = createPooledBank({
        .capacity = capacity,
        .manager = manager
    });

    definitions_[idx] = std::make_unique<ParameterDefinition>(
        std::move(cfg), bank, modulator, modData
    );

    p2b_[idx] = &bank ;
    b2d_[&bank].set(idx);
    active_.set(idx);
    single_.set(idx);

    return definitions_[idx].get();
}

ParameterDefinition* Parameters::addUnitParameter(
    ParameterConfig cfg, 
    const ModulationSource* modulator,
    ModulationData modData
){
    uint8_t idx = cfg.parameter().to_uint8();
    if ( !parameterAvailable(idx) ){
        throw std::invalid_argument("cannot create parameter: already claimed");
    }
    
    definitions_[idx] = std::make_unique<ParameterDefinition>(
        std::move(cfg), *unitBank_, modulator, modData
    );
    p2b_[idx] = unitBank_ ;
    b2d_[unitBank_].set(idx);
    active_.set(idx);
    single_.set(idx);

    return definitions_[idx].get();
}

std::span<ParameterDefinition*> Parameters::addCollection(
    CollectionDescriptor descriptor,
    std::initializer_list<ParameterConfig> configs,
    OrderedBankConfig bankCfg
){
    // all parameters must be available
    for ( const auto& cfg : configs ){
        uint8_t idx = cfg.parameter().to_uint8();
        if ( active_.test(idx) ){
            auto msg = fmt::format(
                "Parameter Type {} is already in use",
                cfg.parameter().toString()
            );
            SPDLOG_ERROR(msg);
            throw std::invalid_argument(msg);
        }
    }

    auto& bank = createOrderedBank(bankCfg);
    auto* ref = &bank ;
    for ( const auto& cfg : configs ){
        uint8_t idx = cfg.parameter().to_uint8();

        definitions_[idx] = std::make_unique<ParameterDefinition>(
            std::move(cfg), bank
        );
        p2b_[idx] = ref ;
        b2d_[ref].set(idx);
        active_.set(idx);
        structured_.set(idx);
        collections_[descriptor].push_back(definitions_[idx].get());
    }

    return collections_[descriptor];
}

void Parameters::modulate(){
    modulatable_.forEach([&](size_t i){
        definitions_[i]->modulateSlots();
    });
}

ParamStatus Parameters::setModulation(ParameterType p, const ModulationSource* m, ModulationData d){
    auto* param = definition(p);
    if ( !param ) return ParamStatus::NoSuchParameter ;
    if ( !m ) return ParamStatus::NullSource ;
    if ( !param->config().supports().modulatable ) 
        return ParamStatus::NotModulatable ;

    param->setModulation(m, withRequiredDefaults(m, std::move(d)));
    return ParamStatus::Ok ;
}

ParamStatus Parameters::removeModulation(ParameterType p){
    auto* param = definition(p);
    if ( !param ) return ParamStatus::NoSuchParameter ;
    param->removeModulation();
    return ParamStatus::Ok ;
}

ParamStatus Parameters::setDepthModulation(ParameterType p, const ModulationSource* m, ModulationData d ){
    auto* param = definition(p);
    if ( !param ) return ParamStatus::NoSuchParameter ;

    auto* depth = param->depth();
    if ( !depth ) return ParamStatus::NoDepth ;                
    if ( !m ) return ParamStatus::NullSource ;
    if ( !depth->config().supports().modulatable ) 
        return ParamStatus::NotModulatable ;

    depth->setModulation(m, withRequiredDefaults(m, std::move(d)));
    return ParamStatus::Ok ;
}

ParamStatus Parameters::removeDepthModulation(ParameterType p){
    auto* param = definition(p);
    if ( !param ) return ParamStatus::NoSuchParameter ;
    
    auto* depth = param->depth();
    if ( !depth ) return ParamStatus::NoDepth ;                

    depth->removeModulation();
    return ParamStatus::Ok ;
}

OrderedSlotBank& Parameters::createOrderedBank(OrderedBankConfig cfg){
    auto bank = std::make_unique<OrderedSlotBank>(std::move(cfg));
    auto& ref = *bank ;
    banks_.push_back(std::move(bank));
    return ref ;
}

PooledSlotBank& Parameters::createPooledBank(PooledBankConfig cfg){
    auto bank = std::make_unique<PooledSlotBank>(std::move(cfg));
    auto& ref = *bank ;
    banks_.push_back(std::move(bank));
    return ref ;
}

ModulationData Parameters::withRequiredDefaults(const ModulationSource* m, ModulationData d){
    if ( d.isEmpty() ){
        for ( auto mp : m->requiredParameters() ){
            d.set(mp, 0.0f);
        }
    }
    return d ;
}
