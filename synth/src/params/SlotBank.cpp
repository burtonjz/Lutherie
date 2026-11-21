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

#include "params/SlotBank.hpp"
#include "interfaces/SlotManager.hpp"
#include "params/ParameterSlot.hpp"
#include "interfaces/SlotBankObserver.hpp"
#include "params/SlotBankConfig.hpp"

#include <spdlog/spdlog.h>

SlotBank::SlotBank(size_t capacity, SlotMembership membership):
    capacity_(capacity),
    membership_(membership),
    columns_(),
    defs_(),
    observers_(),
    active_()
{
    if ( capacity_ < 1 || capacity_ > MAX_SLOTS ){
        auto msg = "SlotBank: capacity must be in [1, 256]" ;
        SPDLOG_ERROR(msg);
        throw std::invalid_argument(msg);
    }
}

size_t SlotBank::capacity() const {
    return capacity_ ;
}

size_t SlotBank::count() const {
    return count_ ;
}

SlotMembership SlotBank::membership() const {
    return membership_ ;
}

const std::vector<ParameterDefinition*>& SlotBank::definitions() const {
    return defs_ ;
}

std::span<const uint8_t> SlotBank::active() const {
    return active_.indices() ;
}
    
bool SlotBank::isActive(size_t idx) const {
    return active_.contains(idx);
}

void SlotBank::addObserver(SlotBankObserver* obs){
    observers_.push_back(obs);
}

void SlotBank::removeObserver(SlotBankObserver* obs){
    observers_.erase(
        std::remove(observers_.begin(), observers_.end(), obs),
        observers_.end()
    );
}

void SlotBank::initSlot(size_t idx){
    for ( auto& col : columns_ ){
        col[idx]->resetValue();
        col[idx]->resetModulation();
    }
}

void SlotBank::copySlot(size_t dst, size_t src){
    for ( auto& col : columns_ ){
        col[dst]->assignFrom(*col[src]);
    }
}

void SlotBank::notify(SlotOp op, size_t slot) const {
    for ( auto* obs : observers_ ){
        obs->slotsChanged(*this, op, slot);
    }
}


size_t SlotBank::adopt(ParameterDefinition& d, Column col){
    if ( col.size() != capacity_ ){
        auto msg = fmt::format(
            "Adopted column size {} does not match bank capacity {}",
            col.size(), capacity_
        );
        SPDLOG_ERROR(msg);
        throw std::invalid_argument(msg);
    }
    columns_.push_back(std::move(col));
    defs_.push_back(&d);
    return columns_.size() - 1 ;
}

std::span<const std::unique_ptr<ParameterSlotBase>> 
    SlotBank::column(size_t c) const {
        return columns_[c] ;
}

OrderedSlotBank::OrderedSlotBank(OrderedBankConfig cfg):
    SlotBank(cfg.capacity, SlotMembership::ORDERED),
    stride_(cfg.stride),
    initialCount_(cfg.initialCount)
{
    if ( 
        stride_ < 1 || 
        cfg.capacity % stride_ != 0 ||
        initialCount_ % stride_ != 0 ||
        initialCount_ > cfg.capacity
    ){
        auto msg = "invalid config: capacity/stride/initial count mismatch" ;
        SPDLOG_ERROR(msg);
        throw std::invalid_argument(msg);
    }

    reset(); // mark initial prefix live
}

size_t OrderedSlotBank::stride() const {
    return stride_ ;
}

size_t OrderedSlotBank::groupCount() const {
    return count_ / stride_ ;
}

size_t OrderedSlotBank::firstSlot(size_t groupIdx) const {
    return groupIdx * stride_ ;
}

std::optional<size_t> OrderedSlotBank::insert(std::optional<size_t> group){
    if ( count_ + stride_ > capacity() ) return std::nullopt ;
    const size_t g = group.value_or(groupCount());
    if ( g > groupCount() ) return std::nullopt ;

    const size_t first = g * stride_ ;
    for ( size_t i = count_; i-- > first ; ){
        copySlot(i + stride_, i); // shift back
    } 

    for ( size_t i = 0 ; i < stride_ ; ++i ){
        initSlot(first + i);
        active_.insert(count_ + i);
    }

    count_ += stride_ ;
    notify(SlotOp::ADDED, first);
    return g ;
}

bool OrderedSlotBank::remove(size_t groupIdx){
    if ( groupIdx >= groupCount() ) return false ;
    const size_t first = groupIdx * stride_ ;

    // shift slots
    for ( size_t i = first + stride_ ; i < count_; ++i ){
        copySlot(i - stride_, i);
    }

    for ( size_t i = 0 ; i < stride_ ; ++i ){
        active_.erase(count_ - 1 - i );
    }

    count_ -= stride_ ;
    notify(SlotOp::REMOVED, first);
    return true ;
}

void OrderedSlotBank::reset(){
    active_.clear();
    count_ = 0 ;
    notify(SlotOp::CLEARED, 0);
    for ( size_t n = 0; n < initialCount_; n += stride_ ) insert();
}


PooledSlotBank::PooledSlotBank(PooledBankConfig cfg):
    SlotBank(cfg.capacity, SlotMembership::POOLED),
    manager_(&cfg.manager)
{
    state_.fill(SlotState::FREE);
    stamp_.fill(0);
    key_.fill(0);
}

SlotState PooledSlotBank::state(size_t idx) const {
    return state_[idx];
}

void PooledSlotBank::setManager(SlotManager& m){
    manager_ = &m ;
}

std::optional<SlotIndex> PooledSlotBank::acquire(uint32_t key){
    const auto picked = manager_->acquire(view(), key);
    if ( !picked || *picked >= capacity() ) return std::nullopt ;
    
    const SlotIndex idx = *picked ;
    if ( state_[idx] != SlotState::FREE ) notify(SlotOp::STOLEN, idx);
    else ++count_ ;

    initSlot(idx);
    state_[idx] = SlotState::HELD ;
    active_.insert(idx);
    stamp_[idx] = ++clock_ ;
    key_[idx] = key ;

    manager_->onAcquired(idx, key);
    notify(SlotOp::ADDED, idx);
    return idx ;
} 

bool PooledSlotBank::release(SlotIndex idx){
    if ( idx >= capacity() || state_[idx] != SlotState::HELD ) return false ;
    state_[idx] = SlotState::RELEASING ;
    notify(SlotOp::RELEASING, idx);
    return true ;
}

bool PooledSlotBank::free(SlotIndex idx){
    if ( idx >= capacity() || state_[idx] == SlotState::FREE ) return false ;
    state_[idx] = SlotState::FREE ;
    active_.erase(idx);
    --count_ ;
    manager_->onReleased(idx);
    notify(SlotOp::REMOVED, idx);
    return true ;
}

void PooledSlotBank::reset(){
    active_.clear();
    state_.fill(SlotState::FREE);
    count_ = 0 ;
    notify(SlotOp::CLEARED, 0);
}

SlotBankView PooledSlotBank::view() const {
    return {
        .capacity = capacity(),
        .count = count_,
        .state = {state_.data(), capacity()},
        .stamp = {stamp_.data(), capacity()},
        .key   = {key_.data(), capacity()}
    };
}