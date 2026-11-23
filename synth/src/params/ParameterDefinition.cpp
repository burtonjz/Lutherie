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

#include "ParameterDefinition.hpp"
#include "interfaces/ParameterObserver.hpp"
#include "params/ParameterSlot.hpp"
#include "params/SlotBank.hpp"
#include <spdlog/spdlog.h>
#include <stdexcept>

ParameterDefinition::ParameterDefinition(
    ParameterConfig cfg, SlotBank& bank,
    const ModulationSource* modulator,
    ModulationData modData
): 
    config_(std::move(cfg)),
    bank_(&bank),
    modulator_(modulator),
    modData_(std::move(modData))
{
    if ( config_.supports().maxSlots > slotCount() ){
        auto msg = "Invalid Bank: slot capacity larger than supported by parameter" ;
        SPDLOG_ERROR(msg);
        throw std::invalid_argument(msg);
    }
    modStrategy_ = config_.parameter().defaultModulationStrategy(ModulatorRange::UNKNOWN);

    // build bank column for this definition
    SlotBank::Column col ;
    col.reserve(bank.capacity());

    ParameterType::visit(config_.parameter(), [&](auto traits){
        constexpr auto T = decltype(traits)::type ;
        for ( size_t i = 0 ; i < bank.capacity(); ++i ){
            col.push_back(std::make_unique<ParameterSlot<T>>(
                *this, static_cast<uint16_t>(i)
            ));
        }
    });

    column_ = bank.adopt(*this, std::move(col));

    if ( 
        config_.parameter() != ParameterType::DEPTH && 
        config_.supports().modulatable 
    ){
        depthDef_ = std::make_unique<ParameterDefinition>(
            ParameterConfig(ParameterType::DEPTH)
        );
    }
}

ParameterDefinition::ParameterDefinition(ParameterConfig cfg):
    config_(std::move(cfg)),
    modulator_(nullptr)
{
    modStrategy_ = config_.parameter().defaultModulationStrategy(ModulatorRange::UNKNOWN);
}

// std::span<const ParameterValue> ParameterDefinition::defaultValues() const {
//     return defaults_ ;
// }

// ParameterValue ParameterDefinition::defaultValue(size_t idx) const {
//     assert (idx < slotCount());
//     return defaults_[idx];
// }

// void ParameterDefinition::setDefaultValue(size_t idx, ParameterValue v){
//     assert(idx < slotCount());

//     const auto& limits = config_.limits();
//     v = v < limits.minimum 
//         ? limits.minimum
//         : v > limits.maximum ? limits.maximum : v ;
//     defaults_[idx] = v ;
// }

size_t ParameterDefinition::slotCount() const { 
    return bank_ ? bank_->capacity() : 0 ;
}

const ParameterConfig& ParameterDefinition::config() const {
    return config_ ;
}

const ParameterSupports& ParameterDefinition::supports() const {
    return config_.supports();
}

ParamStatus ParameterDefinition::setSupports(ParameterSupports supports){
    return config_.setSupports(supports);
}

const ParameterLimits& ParameterDefinition::limits() const {
    return config_.limits();
}

ParamStatus ParameterDefinition::setLimits( ParameterLimits limits){
    return config_.setLimits(limits);
}

const ParameterValue& ParameterDefinition::defaultValue() const {
    return config_.defaultValue();
}

ParamStatus ParameterDefinition::setDefaultValue(ParameterValue v){
    return config_.setDefaultValue(v);
}

uint32_t ParameterDefinition::revision() const { 
    return revision_ ; 
}

const ModulationSource* ParameterDefinition::modulator() const { 
    return modulator_ ; 
}

ModulationStrategy ParameterDefinition::modulationStrategy() const { 
    return modStrategy_ ; 
}

const ModulationData& ParameterDefinition::modulationTemplate() const { 
    return modData_ ; 
}

ParameterDefinition* ParameterDefinition::depth(){ 
    return depthDef_.get(); 
}

const ParameterDefinition* ParameterDefinition::depth() const { 
    return depthDef_.get(); 
}

bool ParameterDefinition::isModulating() const { 
    return config_.supports().modulatable 
        && modulator_ != nullptr ; 
}

void ParameterDefinition::setModulationStrategy(ModulationStrategy s){ 
    modStrategy_ = s ; 
}

void ParameterDefinition::setModulation(const ModulationSource* m, ModulationData d){
    modulator_ = m ;
    modData_ = std::move(d);
    const auto col = bank_->column(column_);
    for ( uint8_t i : bank_->active() ) col[i]->resetModulation();
}

void ParameterDefinition::removeModulation(){ 
    setModulation(nullptr, {}); 
}

void ParameterDefinition::addObserver(ParameterObserver* obs){ 
    observers_.push_back(obs); 
}

void ParameterDefinition::removeObserver(ParameterObserver* obs){
    observers_.erase(
        std::remove(observers_.begin(), observers_.end(), obs), 
        observers_.end()
    );
}

void ParameterDefinition::notifyObservers(uint16_t slot) const {
    for (auto* obs : observers_){
        obs->onParameterChanged(config_.parameter(), slot);
    }
}

ParameterSlotBase* ParameterDefinition::slot(size_t i) const { 
    return ( bank_ && i < bank_->capacity()) 
        ? bank_->column(column_)[i].get() 
        : nullptr ;
}

void ParameterDefinition::modulateSlots() const {
    if ( !bank_ || !isModulating() ) return ;

    const auto col = bank_->column(column_);
    for ( uint8_t i : bank_->active() ) col[i]->modulate();
}