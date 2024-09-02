#pragma once

#include "EntityManager.h"
#include "ComponentManager.h"
#include "Hierarchy.h"
#include <vector>

namespace FakeEngine {
    class UISystem {
    private:
        EntityManager& entityManager = EntityManager::GetInstance();
        ComponentManager& componentManager= ComponentManager::GetInstance();
    public:
        std::vector<EntityID> HierarchyUpdate() {
            std::vector<EntityID> id;
            for (auto& item : entityManager.GetEntities()) {
                if (item.second->GetComponentMask().test(ComponentManager::GetComponentTypeID<Hierarchy>())) {
                    id.push_back(item.first);
                }
            }
            return id;
        }
    };
}
