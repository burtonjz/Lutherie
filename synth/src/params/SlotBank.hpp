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

#ifndef SLOT_BANK_HPP_
#define SLOT_BANK_HPP_

#include "containers/ActiveSet.hpp"
#include "params/SlotBankConfig.hpp"
#include "interfaces/SlotBankView.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

// forward declarations
class ParameterDefinition ;
class ParameterSlotBase ;
class SlotBankObserver ;

inline constexpr size_t MAX_SLOTS = 255 ;

class SlotBank { 
private:
    friend class ParameterDefinition ; // construction: adopt/column
    using Column = std::vector<std::unique_ptr<ParameterSlotBase>>;

    size_t capacity_ ;
    SlotMembership membership_ ;
    std::vector<Column> columns_ ; // slot ownership
    std::vector<ParameterDefinition*> defs_ ; // parallel to columns, non-owning
    std::vector<SlotBankObserver*> observers_ ;

protected:
    ActiveSet<MAX_SLOTS> active_ ;
    size_t count_ = 0 ;

    SlotBank(size_t capacity, SlotMembership membership);

public:
    virtual ~SlotBank() = default ;
    SlotBank(const SlotBank&) = delete ;
    SlotBank& operator=(const SlotBank&) = delete ;

    size_t capacity() const ;  
    size_t count() const ;
    SlotMembership membership() const ;
    const std::vector<ParameterDefinition*>& definitions() const ; 

    std::span<const uint8_t> active() const ;
    bool isActive(size_t idx) const ;

    void addObserver(SlotBankObserver* obs);
    void removeObserver(SlotBankObserver* obs);

protected:
    // row operations (across each bound definition)
    void initSlot(size_t idx);
    void copySlot(size_t dst, size_t src);
    void notify(SlotOp op, size_t slot) const ;

private:
    // ParameterDefinition only -- gives ownership of a column
    // returns column index
    size_t adopt(ParameterDefinition& d, Column col);

    std::span<const std::unique_ptr<ParameterSlotBase>> column(size_t c) const ;
    
};

class OrderedSlotBank : public SlotBank { 
private:
    size_t stride_ ;
    size_t initialCount_ ;

public:
    explicit OrderedSlotBank(OrderedBankConfig cfg);

    size_t stride() const ;
    size_t groupCount() const ;
    size_t firstSlot(size_t groupIdx) const ;

    std::optional<size_t> insert(std::optional<size_t> group = std::nullopt);
    bool remove(size_t groupIdx);

    void reset();

};

class PooledSlotBank : public SlotBank {
private:
    std::array<SlotState, MAX_SLOTS> state_ ;
    std::array<uint64_t, MAX_SLOTS>  stamp_ ;
    std::array<uint32_t, MAX_SLOTS>  key_ ;

    uint64_t clock_ = 0 ;
    SlotManager* manager_ ;

public:
    explicit PooledSlotBank(PooledBankConfig cfg);

    SlotState state(size_t idx) const ;
    void setManager(SlotManager& m);
    
    std::optional<SlotIndex> acquire(uint32_t key);
    bool release(SlotIndex idx);  
    bool free(SlotIndex idx);

    void reset();

private:
    SlotBankView view() const ;


};


#endif // SLOT_BANK_HPP_