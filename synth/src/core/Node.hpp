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

#include "interfaces/Capability.hpp"
#include "types/NodeType.hpp"
#include "capabilities/CapabilityFlags.hpp"

#include <nlohmann/json.hpp>
#include <typeindex>

using json = nlohmann::json ;

// forward declarations



class Node {
private:
    int id_ ;
    NodeType type_ ; 
    CapabilityFlags flags_ ;
    std::unordered_map<std::type_index, std::unique_ptr<Capability*>> capabilities_ ;

public:
    Node(int id, NodeType typ);

    Node(const Node&) = delete ;
    Node& operator=(const Node&) = delete ;
    Node(Node&&) = delete ;
    Node& operator=(Node&&) = delete ;

    int id() const ;
    NodeType type() const ;
    CapabilityFlags capabilities() const ;

    template<CapabilityFlags Flag> 
    auto* getCapability(){
        using T = typename CapabilityTraits<Flag>::Type ;
        return getCapability<T>();
    }

    template<typename T>
    T* getCapability(){
        auto it = capabilities_.find(typeid(T));

        if ( it == capabilities_.end() ) return nullptr ;

        return static_cast<T*>(it->second);
    }

    template<CapabilityFlags Flag, typename... Args>
    auto& AddCapability(Args&&... args){
        using T = typename CapabilityTraits<Flag>::Type ;
        auto capability = std::make_unique<T>(std::forward<Args>(args)...);
        T& result = *capability ;
        capabilities_.emplace(typeid(T), std::move(capability));
        flags_ |= Flag ;
        return result ;
    }

    virtual void serialize(json& msg) const = 0 ;
    virtual void deserialize(const json& msg) = 0 ;
};

#endif // NODE_HPP_