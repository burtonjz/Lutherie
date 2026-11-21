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

#ifndef SLOT_BANK_OBSERVER_HPP_
#define SLOT_BANK_OBSERVER_HPP_

#include "params/SlotBankConfig.hpp"

// forward declarations
class SlotBank ;

class SlotBankObserver {
public:
    virtual ~SlotBankObserver() = default ;
    virtual void slotsChanged(const SlotBank&, SlotOp, SlotIndex) = 0 ;
};

#endif // SLOT_BANK_OBSERVER_HPP_