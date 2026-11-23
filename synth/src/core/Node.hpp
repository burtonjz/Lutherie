/*
 * Copyright (C) 2025 Jared Burton
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
 
#ifndef NODE_HPP_
#define NODE_HPP_

#include "capabilities/Capability.hpp"
#include "types/NodeType.hpp"
#include "types/ParameterType.hpp"
#include "interfaces/ParameterAccessor.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class Node {
private:
    int id_ ;
    NodeType type_ ; 
    Capability::Flags flags_ ;
    std::array<std::unique_ptr<Capability>, static_cast<size_t>(Capability::Count)> capabilities_ ;

public:
    Node(int id, NodeType typ);
    virtual ~Node() = default ;

    Node(const Node&) = delete ;
    Node& operator=(const Node&) = delete ;
    Node(Node&&) = delete ;
    Node& operator=(Node&&) = delete ;

    int id() const ;
    NodeType type() const ;
    Capability::Flags capabilities() const ;

    template<Capability::Flags Flag> 
    auto* getCapability(){
        assert(Capability::isSingleFlag(Flag));
        using T = typename CapabilityTraits<Flag>::Type ;
        auto c = capabilities_[Capability::index(Flag)].get();
        return static_cast<T*>(c);
    }

    template<Capability::Flags Flag, typename... Args>
    auto& addCapability(Args&&... args){
        assert(Capability::isSingleFlag(Flag));

        size_t idx = Capability::index(Flag);
        assert(!capabilities_[idx]);
        
        using T = typename CapabilityTraits<Flag>::Type ;
        auto capability = std::make_unique<T>(std::forward<Args>(args)...);
        T& result = *capability ;
        capabilities_[idx] = std::move(capability);
        flags_ |= Flag ;
        return result ;
    }

protected:
    // for updating/viewing template parameters
    ParameterAccessor createParameterAccessor() const {
        return ParameterAccessor();
    }

    template<ParameterType::Value typ>
    SlotAccessor<typ> createSlotAccessor(){
        return SlotAccessor<typ>() ;
    }


};

#endif // NODE_HPP_