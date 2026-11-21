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

#ifndef SLOT_BANK_CONFIG_HPP_
#define SLOT_BANK_CONFIG_HPP_

#include <cstdint>
#include <cstddef>

// forward declaration
class SlotManager ;

using SlotIndex = uint8_t ;

enum class SlotMembership: uint8_t { ORDERED, POOLED };
enum class SlotState: uint8_t { FREE, HELD, RELEASING };
enum class SlotOp: uint8_t { ADDED, REMOVED, STOLEN, RELEASING, CLEARED };

struct OrderedBankConfig {
    size_t capacity = 1 ;
    size_t stride = 1 ; 
    size_t initialCount = 0 ; 
};

struct PooledBankConfig {
    size_t capacity = 1;
    SlotManager& manager ; 
};

#endif // SLOT_BANK_CONFIG_HPP_