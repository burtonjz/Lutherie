#include "core/ComponentFactory.hpp"
#include "configs/ComponentConfig.hpp"
#include "components/Components.hpp"

#define HANDLE_CREATE_COMPONENT(Type) \
    case ComponentType::Type: \
        return store_->create<ComponentType::Type>(name,  j.get<Type##Config>());

ComponentId ComponentFactory::createFromJson(ComponentType type, const std::string& name, const json& j ){
    switch(type){
        #define X(NAME) \
            case ComponentType::NAME: return ComponentManager::instance()->create<ComponentType::NAME>(name, j.get<NAME##Config>());
            COMPONENT_TYPE_LIST
        #undef X
    default: 
        throw std::runtime_error("invalid component requested.");
    }
}