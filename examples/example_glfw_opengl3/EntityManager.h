#pragma once

#include <unordered_map>
#include "Entity.h"
#include <memory>

namespace FakeEngine {
    typedef std::unordered_map<EntityID, std::unique_ptr<Entity>> EntityMap;

    class EntityManager {

    private:
        EntityID nextID = 0;

        EntityMap entities = {};

    private:
        EntityManager() {};
        EntityManager(const EntityManager&) = delete;
        EntityManager& operator=(const EntityManager&) = delete;

    public:
        static EntityManager& GetInstance() {
            static EntityManager instance;
            return instance;
        }

        EntityID CreateEntity() {
            EntityID id = nextID++;
            entities.emplace(id, std::make_unique<Entity>(id));
            return id;
        }

        void DestoryEntity(EntityID id) {
            entities.erase(id);
        }

        Entity* GetEntity(EntityID id) {
            auto it = entities.find(id);
            if (it != entities.end()) {
                return it->second.get();
            }
            return nullptr;
        }

        EntityMap& GetEntities() {
            return entities;
        };

    };
};
