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

#ifndef SLOT_MANAGER_HPP_
#define SLOT_MANAGER_HPP_

#include "params/SlotBankConfig.hpp"

// forward declarations
struct SlotBankView ;

// defines how pooled slots are made/changed/destroyed
class SlotManager {
public:
    virtual ~SlotManager() = default ;

    virtual std::optional<SlotIndex> acquire(const SlotBankView&, uint32_t /*key*/) = 0 ;
    virtual void onAcquired(SlotIndex, uint32_t /*key*/){}
    virtual void onReleased(SlotIndex){}
};


// Pooled: lowest free index, never steals
class LowestFreeSlotManager : public SlotManager {
public:
    std::optional<SlotIndex> acquire(const SlotBankView& bank, uint32_t /*key*/) override ;
};

// POOLED:
// 1. reuse existing slot (retrigger)
// 2. free slot
// 3. steal oldest (RELEASING over HELD)
class OldestStealSlotManager : public SlotManager {
public:
    std::optional<SlotIndex> acquire(const SlotBankView& bank, uint32_t key) override ;
};


#endif // SLOT_MANAGER_HPP_