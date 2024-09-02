#pragma once

#include <memory>
#include "Component.h"
#include "EntityManager.h"

namespace FakeEngine {
    typedef size_t ComponentTypeID;

    class ComponentManager {

    public:
        std::unordered_map<ComponentTypeID, std::unordered_map<EntityID, std::unique_ptr<Component>>> components;

    public:
        static ComponentManager& GetInstance() {
            static ComponentManager instance;
            return instance;
        }

        template <typename T>
        void AddComponent(EntityID entityID, std::unique_ptr<T> component) {
            auto& componentMap = components[GetComponentTypeID<T>()];// unordered_map 不考虑越界，自动插入新元素
            componentMap[entityID] = std::move(component);// 不使用move编译不过
            UpdateEntityMask(entityID);
        }

        template <typename T>
        T* GetComponent(EntityID entityID) {
            auto& componentMap = components[GetComponentTypeID<T>()];
            auto it = componentMap.find(entityID);
            if (it != componentMap.end()) {
                return (T*)it->second.get();// 父类转子类
            }
            return nullptr;
        }

        template <typename T>
        void removeComponent(EntityID entityID) {
            auto& componentMap = components[GetComponentTypeID<T>()];
            componentMap.erase(entityID);
            UpdateEntityMask(entityID);
        }

        // 技巧点：模板函数对于每个模板实例（不同的T）都会有一个独立的静态局部变量typeID。
        // 当第一次调用函数时typeID会被初始化，后面同样的T不论调用多少次都保持初始化的值
        template <typename T>
        static ComponentTypeID GetComponentTypeID() {
            static ComponentTypeID typeID = nextTypeID++;
            return typeID;
        }

    private:
        static ComponentTypeID nextTypeID;

    private:
         ComponentManager() {};
         ComponentManager(const ComponentManager&) = delete;
         ComponentManager& operator=(const ComponentManager&) = delete;

        void UpdateEntityMask(EntityID entityID) {
            Entity* entity = EntityManager::GetInstance().GetEntity(entityID);
            if (entity) {
                std::bitset<MAX_COMPONENTS> mask = {};
                for (const auto& item : components) {
                    if (item.second.find(entityID) != item.second.end()) {
                        mask.set(item.first);
                    }
                }
                entity->GetComponentMask() = mask;
            }
        }
    };
    ComponentTypeID ComponentManager::nextTypeID = 0;
};
