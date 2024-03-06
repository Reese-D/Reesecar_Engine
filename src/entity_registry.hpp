#pragma once
#include "component.hpp"
#include <unordered_set>
#include <unordered_map>
#include <typeinfo>
#include <vector>
#include <string>
class entity_registry
{
public:
    template<class T> void addComponent(T* component, uint32_t entityID)
    {
        std::string name = typeid(T).name();
        bool exists = components.contains(name);
        if(!exists){
            components.emplace(name, std::vector<Component>{});
        }

        Component* pComp = &components[name].emplace_back(component);
        entities[entityID].push_back(pComp);
    }
    
    uint32_t createEntity()
    {
        entities[idCounter_++] = {};
        return idCounter_ - 1;
    }
    
    
    std::vector<Component*> getComponents(uint32_t entityID)
    {
        return entities[entityID];
    }

    //returns a pair composed of the begin and end iterator
    template<class T> std::pair<std::vector<Component>::iterator, std::vector<Component>::iterator> getComponentIterators()
    {
        return std::make_pair(components[typeid(T).name()].begin(), components[typeid(T).name()].end());
    }
    
    template<class T> const std::vector<Component>& getComponents()
    {
        return components[typeid(T).name()];
    }
private:
    uint32_t idCounter_ = 0;

    //all components are stored in this map
    std::unordered_map<std::string, std::vector<Component>> components{};

    //Entities are just a uint32_t key
    std::unordered_map<uint32_t, std::vector<Component*>> entities{};
};
