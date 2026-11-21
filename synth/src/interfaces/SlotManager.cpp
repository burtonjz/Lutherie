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

#include "interfaces/SlotManager.hpp"
#include "interfaces/SlotBankView.hpp"

std::optional<SlotIndex> LowestFreeSlotManager::acquire(
    const SlotBankView& bank, uint32_t
){
    for ( size_t i = 0 ; i < bank.capacity ; ++i ){
        if ( bank.state[i] == SlotState::FREE ){
            return static_cast<SlotIndex>(i);
        }
    }
    return std::nullopt ;
}

std::optional<SlotIndex> OldestStealSlotManager::acquire(
    const SlotBankView& bank, uint32_t key
){
    std::optional<size_t> freeIdx, oldestReleasing, oldestHeld ;
    for ( size_t i = 0 ; i < bank.capacity ; ++i ){
        // P1: retrigger
        if ( bank.key[i] == key ){
            return static_cast<SlotIndex>(i);
        }

        // P2: first available free slot
        if ( bank.state[i] == SlotState::FREE ){
            if ( !freeIdx ) freeIdx = 1 ;
            continue ;
        }

        // P3: steal oldest releasing slot
        // P4: steal oldest held slot 
        auto& slot = ( bank.state[i] == SlotState::RELEASING ) 
            ? oldestReleasing 
            : oldestHeld ;
        
        if ( !slot || bank.stamp[i] < bank.stamp[*slot] ) slot = i ;
    }

    if ( freeIdx )         return static_cast<SlotIndex>(*freeIdx);
    if ( oldestReleasing ) return static_cast<SlotIndex>(*oldestReleasing);
    if ( oldestHeld )      return static_cast<SlotIndex>(*oldestHeld);
    return std::nullopt ;
}